/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "AlonecraftTestLog.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "Pet.h"
#include "PetDefines.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#include "WarlockShards.h"

#include <algorithm>
#include <limits>

using namespace Alonecraft::Warlock;

// ---------------------------------------------------------------------------
//  Alonecraft Demonology: everything that lives ON the demon
// ---------------------------------------------------------------------------
//
//  Why these auras sit on the pet rather than the warlock:
//  Unit::ProcSkillsAndReactives (Unit.cpp:6789) only ever runs for the actor
//  and the victim of an event, so a proc caused by the pet never reaches the
//  owner's aura list.  Any "when your demon does X" talent therefore needs its
//  own aura on the demon.  spell_sha_spirit_hunt (spell_shaman.cpp:1332) is
//  the core's own example of the pattern.
//
//  Delivery is `spell_pet_auras` (woa_2026_08_02_04.sql):
//      talent dummy effect -> AuraEffect::HandleAuraDummy
//                             (SpellAuraEffects.cpp:5171)
//                          -> Unit::AddPetAura   (Unit.cpp:13746)
//                          -> Pet::CastPetAuras  (Pet.cpp:2354)
//  It re-applies on summon and on talent (re)application, so respecs and
//  re-summons are handled without any bookkeeping here.
//
//  IMPORTANT: Unit::CastPetAura (Unit.cpp:13770) special-cases ONLY spell
//  35696 for custom base points -- every other pet aura is a bare
//  CastSpell(this, auraId, true).  So none of these auras can be handed an
//  amount at cast time; every amount below is computed in DoEffectCalcAmount
//  by reading the owner's talent rank.
//
//  Refresh heartbeat: rather than burning a DBC effect slot on a
//  PERIODIC_DUMMY, the scaling auras use the trick from
//  spell_warl_generic_scaling (spell_warlock.cpp:376) -- DoEffectCalcPeriodic
//  makes an ordinary aura effect periodic from script alone, and
//  OnEffectPeriodic then calls RecalculateAmount().  That keeps the demon's
//  inherited values tracking the owner's gear, buffs and Bloodlust.
// ---------------------------------------------------------------------------

enum WarlockDemonPetSpells
{
    // Talent rank chains (low rank first -- OwnerTalentAmount walks backwards).
    TALENT_DEMONIC_EMBRACE_R1       = 18697,
    TALENT_DEMONIC_EMBRACE_R2       = 18698,
    TALENT_DEMONIC_EMBRACE_R3       = 18699,

    TALENT_IMPERIOUS_FLAMES_R1      = 18694,
    TALENT_IMPERIOUS_FLAMES_R2      = 18695,
    TALENT_IMPERIOUS_FLAMES_R3      = 18696,

    TALENT_FEL_SYNERGY_R1           = 47230,
    TALENT_FEL_SYNERGY_R2           = 47231,

    TALENT_DEMONIC_BRUTALITY_R1     = 18705,
    TALENT_DEMONIC_BRUTALITY_R2     = 18706,
    TALENT_DEMONIC_BRUTALITY_R3     = 18707,

    TALENT_DEMONIC_LASH_R1          = 18754,
    TALENT_DEMONIC_LASH_R2          = 18755,
    TALENT_DEMONIC_LASH_R3          = 18756,

    TALENT_FEL_DOMINATION           = 18708,
    TALENT_MANA_FEED                = 30326,

    TALENT_FEL_ATTUNEMENT_R1        = 18767,
    TALENT_FEL_ATTUNEMENT_R2        = 18768,

    TALENT_DEMONIC_RESILIENCE_R1    = 30319,
    TALENT_DEMONIC_RESILIENCE_R2    = 30320,
    TALENT_DEMONIC_RESILIENCE_R3    = 30321,

    // Alonecraft custom spells (woa_2026_08_02_02.sql).
    SPELL_FELGUARD_IMMOLATION_AURA  = 200407,
    SPELL_FEL_SYNERGY_OWNER_HEAL    = 200424,
    SPELL_NETHER_SCAR               = 200413,
    SPELL_DEMONIC_LASH_DAMAGE       = 200415,
    SPELL_MANA_FEED_ENERGIZE        = 200423,

    // Existing core spells.
    SPELL_LASH_OF_PAIN_R1           = 7814,
};

// Demon creature entries (NPC_IMP 416, NPC_FELHUNTER 417, NPC_VOIDWALKER 1860,
// NPC_SUCCUBUS 1863, NPC_FELGUARD 17252) come from PetDefines.h -- do not
// redeclare them here.

// The warlock that owns the unit this aura is sitting on, or nullptr.
static Player* GetDemonOwner(Unit* pet)
{
    if (!pet)
        return nullptr;

    Unit* owner = pet->GetOwner();
    return owner ? owner->ToPlayer() : nullptr;
}

// Amount stored on `effIndex` of the highest talent rank the owner actually
// has.  Ranks are listed low-to-high and walked backwards so the best one wins
// even if a lower rank's aura somehow lingers.
static int32 OwnerTalentAmount(Unit* pet, std::initializer_list<uint32> ranks, uint8 effIndex)
{
    Player* owner = GetDemonOwner(pet);
    if (!owner)
        return 0;

    for (auto itr = std::rbegin(ranks); itr != std::rend(ranks); ++itr)
        if (AuraEffect const* eff = owner->GetAuraEffect(*itr, effIndex))
            return eff->GetAmount();

    return 0;
}

// ---------------------------------------------------------------------------
//  Shared 2-second recalculation heartbeat
// ---------------------------------------------------------------------------
//  Mirrors spell_warl_generic_scaling::CalcPeriodic / HandlePeriodic
//  (spell_warlock.cpp:376-407).  Attaching this to an aura effect that is not
//  periodic in the DBC is legal -- AuraEffect::CalculatePeriodic asks the
//  script -- and saves an effect slot on every custom pet spell.
#define ALONECRAFT_PET_HEARTBEAT(scriptClass)                                       \
    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude) \
    {                                                                               \
        isPeriodic = true;                                                          \
        amplitude  = 2 * IN_MILLISECONDS;                                           \
    }                                                                               \
                                                                                    \
    void HandlePeriodic(AuraEffect const* aurEff)                                   \
    {                                                                               \
        PreventDefaultAction();                                                     \
        GetEffect(aurEff->GetEffIndex())->RecalculateAmount();                      \
    }

// ---------------------------------------------------------------------------
//  Item 3 -- Demonic Embrace: Voidwalker/Felguard dodge from owner intellect
// ---------------------------------------------------------------------------
//  The TODO asks for "the same rate as bear agi conversion".  Player::
//  GetDodgeFromAgility (Player.cpp:5126) cannot be reused directly: it reads
//  the *owner's* class, and a warlock's coefficient is 0.97/1.15 rather than
//  the druid's 2.00/1.15.  Both dodge_base[] and crit_to_dodge[] are file-local
//  arrays in Player.cpp with no accessor, so the druid constant is mirrored
//  here.  The DBC ratio table itself is public, so only the constant is copied.
class spell_warl_demon_dodge : public AuraScript
{
    PrepareAuraScript(spell_warl_demon_dodge);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = 0;

        Player* owner = GetDemonOwner(GetUnitOwner());
        if (!owner)
            return;

        uint8 level = owner->GetLevel();
        if (level > GT_MAX_LEVEL)
            level = GT_MAX_LEVEL;

        // Dodge per point of a stat is proportional to crit per point of that
        // stat, which is what this table stores -- exactly how
        // Player::GetDodgeFromAgility derives its own numbers.
        GtChanceToMeleeCritEntry const* ratio =
            sGtChanceToMeleeCritStore.LookupEntry((CLASS_DRUID - 1) * GT_MAX_LEVEL + level - 1);
        if (!ratio)
            return;

        // Mirrors Player.cpp:5145 crit_to_dodge[CLASS_DRUID - 1].  Keep in sync
        // if that table is ever retuned.
        float const critToDodgeDruid = 2.00f / 1.15f;

        float const intellect = owner->GetStat(STAT_INTELLECT);
        amount = int32(100.0f * intellect * ratio->ratio * critToDodgeDruid);

        // Recalculated every 2 sec; only a real move is worth a line.
        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("WARL.PET.DODGE", "pet={} ownerInt={:.1f} ratio={:.6f} -> dodgeAmount={}",
                Alonecraft::TestLog::N(GetUnitOwner()), intellect, ratio->ratio, amount);
        }
    }

    int32 _lastLogged = std::numeric_limits<int32>::lowest();

    ALONECRAFT_PET_HEARTBEAT(spell_warl_demon_dodge)

    void Register() override
    {
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_warl_demon_dodge::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_DODGE_PERCENT);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warl_demon_dodge::CalcPeriodic, EFFECT_0, SPELL_AURA_MOD_DODGE_PERCENT);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_warl_demon_dodge::HandlePeriodic, EFFECT_0, SPELL_AURA_MOD_DODGE_PERCENT);
    }
};

// ---------------------------------------------------------------------------
//  Item 4 -- Fel Synergy: a share of the demon's damage heals the warlock
// ---------------------------------------------------------------------------
//  Core's spell_warl_fel_synergy (spell_warlock.cpp:942) keeps doing the
//  original half (owner damage heals the pet) off the talent's Effect1.  This
//  is the new half, and it cannot reuse the core heal 54181 -- that spell's
//  implicit target is TARGET_UNIT_PET, so it can only ever heal a pet.  200424
//  is the same heal retargeted to TARGET_UNIT_TARGET_ALLY.
class spell_warl_demon_fel_synergy : public AuraScript
{
    PrepareAuraScript(spell_warl_demon_fel_synergy);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (!GetDemonOwner(GetUnitOwner()))
            return false;

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        return damageInfo && damageInfo->GetDamage() > 0;
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        Player* owner = GetDemonOwner(pet);
        if (!owner || !owner->IsAlive())
            return;

        int32 const pct = OwnerTalentAmount(pet, {TALENT_FEL_SYNERGY_R1, TALENT_FEL_SYNERGY_R2}, EFFECT_1);
        if (pct <= 0)
            return;

        int32 heal = int32(CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), pct));
        if (heal <= 0)
            return;

        uint32 const hpBefore = owner->GetHealth();
        pet->CastCustomSpell(owner, SPELL_FEL_SYNERGY_OWNER_HEAL, &heal, nullptr, nullptr, true);

        ACTEST("WARL.PET.SYNERGY",
            "pet={} damage={} pct={} heal={} ownerHp {} -> {} (max={})",
            Alonecraft::TestLog::N(pet), eventInfo.GetDamageInfo()->GetDamage(), pct, heal,
            hpBefore, owner->GetHealth(), owner->GetMaxHealth());
    }

    void Register() override
    {
        DoCheckProc  += AuraCheckProcFn(spell_warl_demon_fel_synergy::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_warl_demon_fel_synergy::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Item 6 -- Demonic Brutality: Voidwalker melee threat
// ---------------------------------------------------------------------------
//  SPELL_AURA_MOD_THREAT with MiscValue 1 (SPELL_SCHOOL_MASK_NORMAL).
//  ThreatManager::UpdateMySpellSchoolModifiers (ThreatManager.cpp:820) reads
//  the multiplier off the threat-generating unit itself, so it works on a pet;
//  SPELL_AURA_MOD_TOTAL_THREAT would not (SpellAuraEffects.cpp:3570 early-outs
//  for non-players).
//
//  No heartbeat: the talent rank cannot change while the aura is applied --
//  a respec drops and re-applies it through spell_pet_auras.
class spell_warl_demon_brutality : public AuraScript
{
    PrepareAuraScript(spell_warl_demon_brutality);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = OwnerTalentAmount(GetUnitOwner(),
            {TALENT_DEMONIC_BRUTALITY_R1, TALENT_DEMONIC_BRUTALITY_R2, TALENT_DEMONIC_BRUTALITY_R3},
            EFFECT_1);

        ACTEST("WARL.PET.BRUTALITY", "pet={} threatModAmount={}",
            Alonecraft::TestLog::N(GetUnitOwner()), amount);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_demon_brutality::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_THREAT);
    }
};

// ---------------------------------------------------------------------------
//  Item 6b -- Demonic Brutality: Torment / Suffering threat
// ---------------------------------------------------------------------------
//  The MOD_THREAT carrier above cannot reach these two.  Torment (47984) and
//  Suffering (47990) do not generate threat from damage -- they carry a flat
//  SPELL_EFFECT_THREAT, and Spell::EffectThreat (SpellEffects.cpp:3709) passes
//  ignoreModifiers = true, so ThreatManager::AddThreat skips
//  CalculateModifiedThreat entirely (ThreatManager.cpp:39).  That bypasses the
//  school multipliers, spell_threat.pctMod and SPELLMOD_THREAT alike; widening
//  200412's MiscValue from 1 to 127 would change nothing here.
//
//  So the bonus is added as a second, talent-scaled AddThreat of our own.  It
//  passes ignoreModifiers = true for the same reason the core effect does: the
//  base threat was unmodified, and running only the bonus through the school
//  multiplier would double-count the melee 3x on top of itself.
//
//  Registered on -3716 and -17735 (all ranks).  Both chains are Voidwalker-only
//  pet abilities, so no creature-entry check is needed; OwnerTalentAmount
//  returns 0 for anything without a warlock owner.
class spell_warl_demon_brutality_threat : public SpellScript
{
    PrepareSpellScript(spell_warl_demon_brutality_threat);

    void HandleThreat(SpellEffIndex effIndex)
    {
        Unit* pet = GetCaster();
        Unit* victim = GetHitUnit();
        if (!victim || !victim->CanHaveThreatList() || !pet)
            return;

        int32 const pct = OwnerTalentAmount(pet,
            {TALENT_DEMONIC_BRUTALITY_R1, TALENT_DEMONIC_BRUTALITY_R2, TALENT_DEMONIC_BRUTALITY_R3},
            EFFECT_1);
        if (pct <= 0)
            return;

        int32 const base = GetEffectValue();
        float const bonus = CalculatePct(float(base), pct);
        if (bonus <= 0.0f)
            return;

        victim->GetThreatMgr().AddThreat(pet, bonus, GetSpellInfo(), true);

        ACTEST("WARL.PET.BRUTALITY",
            "{} effIndex={} on {} baseThreat={} pct={} bonusThreat={:.0f}",
            GetSpellInfo()->SpellName[0], uint32(effIndex),
            Alonecraft::TestLog::N(victim), base, pct, bonus);
    }

    void Register() override
    {
        // Torment carries it on EFFECT_2, Suffering on EFFECT_0 -- match by
        // effect type rather than hardcoding an index per spell.
        OnEffectHitTarget += SpellEffectFn(spell_warl_demon_brutality_threat::HandleThreat, EFFECT_ALL, SPELL_EFFECT_THREAT);
    }
};

// ---------------------------------------------------------------------------
//  Item 7 -- Demonic Lash
// ---------------------------------------------------------------------------
//  Two unrelated behaviours share one carrier aura because they belong to one
//  talent and land on two different demons:
//    * Succubus Lash of Pain  -> applies Nether Scar (200413) to the victim
//    * Felguard melee swing   -> 15% of the hit again as shadow damage
//
//  Nether Scar is deliberately cast BY THE OWNER.  It is
//  SPELL_AURA_MOD_DAMAGE_FROM_CASTER, which Unit.cpp:8939 gates on
//  aurEff->GetCasterGUID() == caster->GetGUID(), so a scar applied by the
//  Succubus would boost nothing the warlock casts.
class spell_warl_demon_lash : public AuraScript
{
    PrepareAuraScript(spell_warl_demon_lash);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        // Our own shadow payload is DONE_SPELL_NONE_DMG_CLASS_NEG and would
        // otherwise re-enter this handler forever.
        if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
            if (spellInfo->Id == SPELL_DEMONIC_LASH_DAMAGE)
                return false;

        return GetDemonOwner(GetUnitOwner()) != nullptr;
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        Player* owner = GetDemonOwner(pet);
        Unit* victim = eventInfo.GetActionTarget();
        if (!owner || !victim || !victim->IsAlive())
            return;

        SpellInfo const* spellInfo = eventInfo.GetSpellInfo();

        // --- Succubus: Lash of Pain leaves a Nether Scar ---
        if (spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK &&
            (spellInfo->SpellFamilyFlags[0] & 0x00002000))
        {
            int32 scar = OwnerTalentAmount(pet,
                {TALENT_DEMONIC_LASH_R1, TALENT_DEMONIC_LASH_R2, TALENT_DEMONIC_LASH_R3}, EFFECT_0);
            if (scar > 0)
                owner->CastCustomSpell(victim, SPELL_NETHER_SCAR, &scar, nullptr, nullptr, true);

            // Cast BY THE OWNER on purpose -- MOD_DAMAGE_FROM_CASTER only
            // boosts the caster's own spells (Unit.cpp:8939).
            ACTEST("WARL.PET.LASH",
                "Lash of Pain -> Nether Scar on {} amount={} castBy={} present={}",
                Alonecraft::TestLog::N(victim), scar, owner->GetName(),
                victim->HasAura(SPELL_NETHER_SCAR, owner->GetGUID()) ? "yes" : "NO");
            return;
        }

        // --- Felguard: bonus shadow damage on melee swings ---
        if (spellInfo || pet->GetEntry() != NPC_FELGUARD)
            return;

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return;

        int32 const pct = OwnerTalentAmount(pet,
            {TALENT_DEMONIC_LASH_R1, TALENT_DEMONIC_LASH_R2, TALENT_DEMONIC_LASH_R3}, EFFECT_1);
        if (pct <= 0)
            return;

        int32 damage = int32(CalculatePct(damageInfo->GetDamage(), pct));
        if (damage > 0)
        {
            pet->CastCustomSpell(victim, SPELL_DEMONIC_LASH_DAMAGE, &damage, nullptr, nullptr, true);

            ACTEST("WARL.PET.LASH",
                "Felguard swing on {} meleeDamage={} pct={} bonusShadow={}",
                Alonecraft::TestLog::N(victim), damageInfo->GetDamage(), pct, damage);
        }
    }

    void Register() override
    {
        DoCheckProc  += AuraCheckProcFn(spell_warl_demon_lash::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_warl_demon_lash::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Item 8 -- Fel Domination: +10% demon damage per owner DoT on the target
// ---------------------------------------------------------------------------
//  Recomputed once a second against whatever the demon is currently attacking.
//  Capped, because an Alonecraft warlock can stack a lot more DoTs than a
//  retail one and this would otherwise scale without limit.
class spell_warl_demon_fel_domination : public AuraScript
{
    PrepareAuraScript(spell_warl_demon_fel_domination);

    static constexpr int32 PCT_PER_DOT = 10;
    static constexpr int32 MAX_DOTS    = 8;

    // Number of the owner's own periodic damage effects on `victim`.
    static int32 CountOwnerDots(Player* owner, Unit* victim)
    {
        if (!owner || !victim)
            return 0;

        int32 count = 0;
        for (AuraType type : {SPELL_AURA_PERIODIC_DAMAGE,
                              SPELL_AURA_PERIODIC_DAMAGE_PERCENT,
                              SPELL_AURA_PERIODIC_LEECH})
        {
            for (AuraEffect const* eff : victim->GetAuraEffectsByType(type))
                if (eff->GetCasterGUID() == owner->GetGUID())
                    ++count;
        }

        return count;
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = 0;

        Unit* pet = GetUnitOwner();
        Player* owner = GetDemonOwner(pet);
        if (!owner)
            return;

        // The demon's own target, falling back to whatever the warlock has
        // selected -- the two diverge often enough to matter.
        Unit* victim = pet->GetVictim();
        if (!victim)
            victim = owner->GetSelectedUnit();

        int32 const dots = CountOwnerDots(owner, victim);
        amount = std::min(dots, MAX_DOTS) * PCT_PER_DOT;

        // 1 Hz heartbeat -- only a real move is worth a line.
        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("WARL.PET.FELDOM",
                "pet={} target={} ownerDots={} (cap={}) -> damageDonePct={}",
                Alonecraft::TestLog::N(pet), Alonecraft::TestLog::N(victim),
                dots, MAX_DOTS, amount);
        }
    }

    int32 _lastLogged = std::numeric_limits<int32>::lowest();

    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        isPeriodic = true;
        amplitude  = 1 * IN_MILLISECONDS;
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        GetEffect(aurEff->GetEffIndex())->RecalculateAmount();
    }

    void Register() override
    {
        DoEffectCalcAmount   += AuraEffectCalcAmountFn(spell_warl_demon_fel_domination::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warl_demon_fel_domination::CalcPeriodic, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_warl_demon_fel_domination::HandlePeriodic, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
    }
};

// ---------------------------------------------------------------------------
//  Item 11a -- Mana Feed: demon damage returns mana to the warlock
// ---------------------------------------------------------------------------
//  The 1s internal cooldown lives in spell_proc, not here: with Immolation
//  Aura ticking on every nearby enemy plus Felguard melee, an unthrottled 5%
//  return would make mana a non-resource.
//
//  The energize is custom spell 200423, not core's 32554.  32554's implicit
//  target is TARGET_UNIT_PET, which Spell.cpp:1794 resolves as
//  m_caster->GetGuardianPet() and which ignores the explicit target entirely --
//  cast by the demon it would look for the DEMON's pet and find nothing.
//  200423 is the same energize retargeted to TARGET_UNIT_TARGET_ALLY.
class spell_warl_demon_mana_feed : public AuraScript
{
    PrepareAuraScript(spell_warl_demon_mana_feed);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Player* owner = GetDemonOwner(GetUnitOwner());
        if (!owner || owner->GetPower(POWER_MANA) >= owner->GetMaxPower(POWER_MANA))
            return false;

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        return damageInfo && damageInfo->GetDamage() > 0;
    }

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        Player* owner = GetDemonOwner(pet);
        if (!owner)
            return;

        int32 const pct = OwnerTalentAmount(pet, {TALENT_MANA_FEED}, EFFECT_0);
        if (pct <= 0)
            return;

        int32 mana = int32(CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), pct));
        if (mana > 0)
        {
            uint32 const before = owner->GetPower(POWER_MANA);
            pet->CastCustomSpell(owner, SPELL_MANA_FEED_ENERGIZE, &mana, nullptr, nullptr, true);

            ACTEST("WARL.PET.MANAFEED",
                "pet={} damage={} pct={} mana={} ownerMana {} -> {} (max={})",
                Alonecraft::TestLog::N(pet), eventInfo.GetDamageInfo()->GetDamage(), pct, mana,
                before, owner->GetPower(POWER_MANA), owner->GetMaxPower(POWER_MANA));
        }
    }

    void Register() override
    {
        DoCheckProc  += AuraCheckProcFn(spell_warl_demon_mana_feed::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_warl_demon_mana_feed::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Item 12 -- Fel Attunement: demon inherits owner haste and crit
// ---------------------------------------------------------------------------
//  Modelled directly on spell_dk_pet_scaling (spell_dk.cpp:790), which is the
//  one place in the core where a pet already inherits its owner's haste: the
//  Death Knight ghoul, via Death Knight Pet Scaling 02 (51996).
//
//  Haste is SPELL_AURA_MELEE_SLOW (193).  Despite the name its handler
//  HandleModCombatSpeedPct applies ApplyCastTimePercentMod plus
//  ApplyAttackTimePercentMod for BASE, OFF and RANGED in a single effect, and
//  is not player-gated.  A positive amount speeds the target up.
//
//  Crit inheritance was deliberately dropped from this talent.  Haste has a
//  shipping precedent on a real pet; crit has none anywhere in the core, so it
//  would have been the only unproven mechanic left in the batch.  The talent
//  compensates with a larger haste share (75/150% rather than 50/100%).
class spell_warl_demon_attunement : public AuraScript
{
    PrepareAuraScript(spell_warl_demon_attunement);

    // How much of the owner's haste the demon inherits, as a percentage.
    int32 InheritPct() const
    {
        return OwnerTalentAmount(GetUnitOwner(),
            {TALENT_FEL_ATTUNEMENT_R1, TALENT_FEL_ATTUNEMENT_R2}, EFFECT_0);
    }

    // Straight from spell_dk_pet_scaling::CalculateHasteAmount (spell_dk.cpp:849).
    // m_modAttackSpeedPct is the resulting attack-speed MULTIPLIER (below 1.0
    // means faster), so inverting it recovers the owner's total haste percent.
    // Reading it rather than GetRatingBonusValue(CR_HASTE_MELEE) matters: the
    // multiplier includes every haste source -- Bloodlust, talents, buffs --
    // whereas the rating only covers haste from gear.
    void CalculateHaste(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = 0;

        Player* owner = GetDemonOwner(GetUnitOwner());
        if (!owner)
            return;

        float modSpeed = owner->m_modAttackSpeedPct[BASE_ATTACK];
        modSpeed = std::clamp(modSpeed, 1e-6f, 1.0f);

        float const ownerHastePct = ((1.0f / modSpeed) - 1.0f) * 100.0f;
        amount = int32(ownerHastePct * InheritPct() / 100.0f);

        // 2 sec heartbeat -- only log when the inherited value actually moves,
        // e.g. when Bloodlust lands on the owner.
        if (amount != _lastLogged)
        {
            _lastLogged = amount;
            ACTEST("WARL.PET.HASTE",
                "pet={} ownerModSpeed={:.4f} ownerHaste={:.2f}% inheritPct={} -> petHaste={} "
                "petAtkTime={}",
                Alonecraft::TestLog::N(GetUnitOwner()), modSpeed, ownerHastePct, InheritPct(),
                amount, GetUnitOwner()->GetAttackTime(BASE_ATTACK));
        }
    }

    int32 _lastLogged = std::numeric_limits<int32>::lowest();

    // Make the inherited haste authoritative.  Without this a haste buff landing
    // on the demon directly (Bloodlust hits pets) would stack on top of the
    // share it already inherited from the owner, double-dipping.  Same guard
    // spell_dk_pet_scaling::HandleEffectApply (spell_dk.cpp:861) uses.
    void HandleEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        if (aurEff->GetAuraType() != SPELL_AURA_MELEE_SLOW)
            return;

        Unit* pet = GetUnitOwner();
        pet->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, true, SPELL_BLOCK_TYPE_POSITIVE);
        pet->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_MELEE_RANGED_HASTE, true, SPELL_BLOCK_TYPE_POSITIVE);
        pet->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MELEE_SLOW, true, SPELL_BLOCK_TYPE_POSITIVE);
    }

    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        if (!GetUnitOwner()->IsPet())
            return;

        isPeriodic = true;
        amplitude  = 2 * IN_MILLISECONDS;
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        PreventDefaultAction();
        GetEffect(aurEff->GetEffIndex())->RecalculateAmount();
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_demon_attunement::CalculateHaste, EFFECT_0, SPELL_AURA_MELEE_SLOW);

        OnEffectApply        += AuraEffectApplyFn(spell_warl_demon_attunement::HandleEffectApply, EFFECT_ALL, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warl_demon_attunement::CalcPeriodic, EFFECT_ALL, SPELL_AURA_ANY);
        OnEffectPeriodic     += AuraEffectPeriodicFn(spell_warl_demon_attunement::HandlePeriodic, EFFECT_ALL, SPELL_AURA_ANY);
    }
};

// ---------------------------------------------------------------------------
//  Item 14 -- Demonic Resilience: demon damage reduction
// ---------------------------------------------------------------------------
//  This half of the talent never worked: its DBC effect was an
//  ADD_FLAT_MODIFIER whose class mask matched no spell, and nothing in
//  src/server/ referenced 30319-30321.  The talent rows in file 01 turn that
//  effect into a dummy carrying -5/-10/-15.
class spell_warl_demon_resilience : public AuraScript
{
    PrepareAuraScript(spell_warl_demon_resilience);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        amount = OwnerTalentAmount(GetUnitOwner(),
            {TALENT_DEMONIC_RESILIENCE_R1, TALENT_DEMONIC_RESILIENCE_R2, TALENT_DEMONIC_RESILIENCE_R3},
            EFFECT_1);

        // Negative reduces damage taken; a positive number here is the bug.
        ACTEST("WARL.PET.RESILIENCE", "pet={} damageTakenPct={}",
            Alonecraft::TestLog::N(GetUnitOwner()), amount);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_demon_resilience::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
    }
};

// ---------------------------------------------------------------------------
//  Item 15 -- Nemesis: the demon's attacks may grant a Soul Shard
// ---------------------------------------------------------------------------
//  One carrier aura per talent rank (200422/200425/200426), because the rate
//  lives in spell_proc's ProcsPerMinute column and spell_proc is keyed by
//  spell id -- the same reason Killing Machine's five ranks are five spells.
//  The engine does the whole roll in Aura::CalcProcChance, so all that is left
//  here is handing over the shard.
class spell_warl_demon_nemesis : public AuraScript
{
    PrepareAuraScript(spell_warl_demon_nemesis);

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* pet = GetUnitOwner();
        Player* owner = GetDemonOwner(pet);
        if (!owner)
            return;

        // Cheap early-out so a capped warlock never chat-spams.
        if (GetSoulShardCount(owner) >= SOUL_SHARD_MAX)
            return;

        ACTEST("WARL.NEMESIS", "pet={} proc carrier={} (spell_proc PPM gated)",
            Alonecraft::TestLog::N(pet), GetId());
        AddSoulShards(owner, 1, "nemesis-pet");
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warl_demon_nemesis::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Item 2b -- Imperious Flames: the Felguard learns Immolation Aura
// ---------------------------------------------------------------------------
//  spell_pet_auras can only apply auras, never grant an actionbar ability, so
//  this one talent needs a PetScript.  OnPetAddToWorld (Pet.cpp:118) fires on
//  every summon, which also cleans up after a respec: a warlock who drops the
//  talent has the spell unlearned on the next summon.
//
//  KNOWN GAP: respeccing INTO Imperious Flames without re-summoning leaves the
//  Felguard without the ability until it is next summoned.  Deliberate -- the
//  alternative is polling talent state on every pet update.
class WarlockDemonPetScript : public PetScript
{
public:
    WarlockDemonPetScript() : PetScript("WarlockDemonPetScript") { }

    void OnPetAddToWorld(Pet* pet) override
    {
        if (!pet || pet->GetEntry() != NPC_FELGUARD)
            return;

        Player* owner = GetDemonOwner(pet);
        if (!owner)
            return;

        bool const hasTalent = owner->HasAura(TALENT_IMPERIOUS_FLAMES_R1)
                            || owner->HasAura(TALENT_IMPERIOUS_FLAMES_R2)
                            || owner->HasAura(TALENT_IMPERIOUS_FLAMES_R3);

        bool const knows = pet->HasSpell(SPELL_FELGUARD_IMMOLATION_AURA);

        if (hasTalent && !knows)
            pet->learnSpell(SPELL_FELGUARD_IMMOLATION_AURA);
        else if (!hasTalent && knows)
            pet->unlearnSpell(SPELL_FELGUARD_IMMOLATION_AURA, false);

        ACTEST("WARL.PET.IMMOAURA",
            "Felguard summoned owner={} hasTalent={} knewBefore={} knowsNow={}",
            owner->GetName(), hasTalent, knows,
            pet->HasSpell(SPELL_FELGUARD_IMMOLATION_AURA));
    }
};

// ---------------------------------------------------------------------------
//  Loaders
// ---------------------------------------------------------------------------

//  Written out one class at a time rather than generated from a macro.  A
//  macro would hide each loader's script-name string literal from
//  tools/verify_scripts.py, which build_and_run.bat runs as a pre-build step
//  to catch exactly the mismatch these names are prone to.

class spell_warl_demon_dodge_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_dodge_loader() : SpellScriptLoader("spell_warl_demon_dodge") { }

    AuraScript* GetAuraScript() const override { return new spell_warl_demon_dodge(); }
};

class spell_warl_demon_fel_synergy_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_fel_synergy_loader() : SpellScriptLoader("spell_warl_demon_fel_synergy") { }

    AuraScript* GetAuraScript() const override { return new spell_warl_demon_fel_synergy(); }
};

class spell_warl_demon_brutality_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_brutality_loader() : SpellScriptLoader("spell_warl_demon_brutality") { }

    AuraScript* GetAuraScript() const override { return new spell_warl_demon_brutality(); }
};

class spell_warl_demon_brutality_threat_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_brutality_threat_loader() : SpellScriptLoader("spell_warl_demon_brutality_threat") { }

    SpellScript* GetSpellScript() const override { return new spell_warl_demon_brutality_threat(); }
};

class spell_warl_demon_lash_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_lash_loader() : SpellScriptLoader("spell_warl_demon_lash") { }

    AuraScript* GetAuraScript() const override { return new spell_warl_demon_lash(); }
};

class spell_warl_demon_fel_domination_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_fel_domination_loader() : SpellScriptLoader("spell_warl_demon_fel_domination") { }

    AuraScript* GetAuraScript() const override { return new spell_warl_demon_fel_domination(); }
};

class spell_warl_demon_mana_feed_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_mana_feed_loader() : SpellScriptLoader("spell_warl_demon_mana_feed") { }

    AuraScript* GetAuraScript() const override { return new spell_warl_demon_mana_feed(); }
};

class spell_warl_demon_attunement_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_attunement_loader() : SpellScriptLoader("spell_warl_demon_attunement") { }

    AuraScript* GetAuraScript() const override { return new spell_warl_demon_attunement(); }
};

class spell_warl_demon_resilience_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_resilience_loader() : SpellScriptLoader("spell_warl_demon_resilience") { }

    AuraScript* GetAuraScript() const override { return new spell_warl_demon_resilience(); }
};

class spell_warl_demon_nemesis_loader : public SpellScriptLoader
{
public:
    spell_warl_demon_nemesis_loader() : SpellScriptLoader("spell_warl_demon_nemesis") { }

    AuraScript* GetAuraScript() const override { return new spell_warl_demon_nemesis(); }
};

void AddSC_warl_demon_pets()
{
    new spell_warl_demon_dodge_loader();
    new spell_warl_demon_fel_synergy_loader();
    new spell_warl_demon_brutality_loader();
    new spell_warl_demon_brutality_threat_loader();
    new spell_warl_demon_lash_loader();
    new spell_warl_demon_fel_domination_loader();
    new spell_warl_demon_mana_feed_loader();
    new spell_warl_demon_attunement_loader();
    new spell_warl_demon_resilience_loader();
    new spell_warl_demon_nemesis_loader();
    new WarlockDemonPetScript();
}
