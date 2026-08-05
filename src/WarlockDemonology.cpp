/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "WarlockShards.h"

using namespace Alonecraft::Warlock;

// ---------------------------------------------------------------------------
//  Alonecraft Demonology: the owner-side talents
// ---------------------------------------------------------------------------
//  Everything here runs on the warlock.  The demon-side half of the tree lives
//  in WarlockDemonPets.cpp, and Mana Feed's Health Funnel half in
//  WarlockHealthFunnel.cpp.
// ---------------------------------------------------------------------------

enum WarlockDemonologySpells
{
    TALENT_IMPROVED_HEALTHSTONE_R1 = 18692,
    TALENT_IMPROVED_HEALTHSTONE_R2 = 18693,

    TALENT_IMPERIOUS_FLAMES_R1     = 18694,
    TALENT_IMPERIOUS_FLAMES_R2     = 18695,
    TALENT_IMPERIOUS_FLAMES_R3     = 18696,

    TALENT_DEMONIC_AEGIS_R1        = 30143,
    TALENT_DEMONIC_AEGIS_R2        = 30144,
    TALENT_DEMONIC_AEGIS_R3        = 30145,

    TALENT_MOLTEN_CORE_R1          = 47245,
    TALENT_MOLTEN_CORE_R2          = 47246,
    TALENT_MOLTEN_CORE_R3          = 47247,

    SPELL_HEALTHSTONE_SURGE        = 200406,
    SPELL_DEMONIC_AEGIS_BUFF       = 200417,
    SPELL_MOLTEN_FURY              = 200420,

    // The "empowers your next Incinerate or Soul Fire" buff the talent
    // triggers, one spell per talent rank.  Carries 3 charges.
    SPELL_MOLTEN_CORE_BUFF_R1      = 47383,
    SPELL_MOLTEN_CORE_BUFF_R2      = 71162,
    SPELL_MOLTEN_CORE_BUFF_R3      = 71165,

    SPELL_CORRUPTION_R1            = 172,
};

// Immolate's SpellFamilyFlags word-A bit, used to spot the warlock's own
// Immolate on a target.  Same predicate WarlockRuin.cpp uses.
static constexpr uint32 IMMOLATE_FAMILY_MASK0 = 0x00000004;
// Shadow Bolt's word-A bit.
static constexpr uint32 SHADOW_BOLT_FAMILY_MASK0 = 0x00000001;
// Corruption's word-A bit, the same 0x2 core's own spell_proc row for -47245
// filters Molten Core on.
static constexpr uint32 CORRUPTION_FAMILY_MASK0 = 0x00000002;

// Highest rank of a talent the player actually has, or 0.
static AuraEffect const* GetTalentEffect(Unit* unit, std::initializer_list<uint32> ranks, uint8 effIndex)
{
    if (!unit)
        return nullptr;

    for (auto itr = std::rbegin(ranks); itr != std::rend(ranks); ++itr)
        if (AuraEffect const* eff = unit->GetAuraEffect(*itr, effIndex))
            return eff;

    return nullptr;
}

// ---------------------------------------------------------------------------
//  Item 1 -- Improved Healthstone also restores mana
// ---------------------------------------------------------------------------
//  The talent's Effect1 already stores 10/20 and is left untouched, because
//  core's spell_warl_create_healthstone (spell_warlock.cpp:601) finds this
//  talent by SpellIconID 284 through GetDummyAuraEffect to pick the improved
//  stone.  The same number is reused as the mana percentage.
//
//  Bound to the healthstone USE spells (family 5, word-A flag 0x00010000),
//  listed explicitly in spell_script_names because the rank chain is not
//  contiguous.
class spell_warl_improved_healthstone_mana : public SpellScript
{
    PrepareSpellScript(spell_warl_improved_healthstone_mana);

    void HandleAfterHit()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;

        AuraEffect const* talent = GetTalentEffect(player,
            {TALENT_IMPROVED_HEALTHSTONE_R1, TALENT_IMPROVED_HEALTHSTONE_R2}, EFFECT_0);
        if (!talent)
            return;

        int32 pct = talent->GetAmount();
        if (pct <= 0)
            return;

        uint32 const manaBefore = player->GetPower(POWER_MANA);
        player->CastCustomSpell(player, SPELL_HEALTHSTONE_SURGE, &pct, nullptr, nullptr, true);

        ACTEST("WARL.HSTONE", "healthstone={} talentPct={} mana {} -> {} (max={})",
            GetSpellInfo()->Id, pct, manaBefore, player->GetPower(POWER_MANA),
            player->GetMaxPower(POWER_MANA));
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_warl_improved_healthstone_mana::HandleAfterHit);
    }
};

// ---------------------------------------------------------------------------
//  Item 2a -- Imperious Flames: Imp Firebolt doubles against your Immolate
// ---------------------------------------------------------------------------
//  Not expressible in the DBC: the condition is a debuff owned by the PET'S
//  OWNER, not by the caster of Firebolt.  Bound to the Firebolt rank chain.
class spell_warl_imperious_flames : public SpellScript
{
    PrepareSpellScript(spell_warl_imperious_flames);

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        Unit* imp = GetCaster();
        Unit* victim = GetHitUnit();
        if (!imp || !victim)
            return;

        Unit* ownerUnit = imp->GetOwner();
        Player* owner = ownerUnit ? ownerUnit->ToPlayer() : nullptr;
        if (!owner)
            return;

        AuraEffect const* talent = GetTalentEffect(owner,
            {TALENT_IMPERIOUS_FLAMES_R1, TALENT_IMPERIOUS_FLAMES_R2, TALENT_IMPERIOUS_FLAMES_R3},
            EFFECT_0);
        if (!talent)
            return;

        // The warlock's OWN Immolate specifically -- another warlock's does not
        // empower this Imp.
        if (!victim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_WARLOCK,
                                   IMMOLATE_FAMILY_MASK0, 0, 0, owner->GetGUID()))
        {
            ACTEST("WARL.IMPFLAMES", "firebolt on {} NOT boosted (no owner Immolate) damage={}",
                Alonecraft::TestLog::N(victim), GetHitDamage());
            return;
        }

        // Talent stores BasePoints + DieSides 1 = +33 / 67 / 100% by rank
        // (base points 32 / 66 / 99, retuned by woa_2026_08_04_01.sql).
        int32 damage = GetHitDamage();
        int32 const before = damage;
        AddPct(damage, talent->GetAmount());
        SetHitDamage(damage);

        ACTEST("WARL.IMPFLAMES", "firebolt on {} BOOSTED talentPct={} damage {} -> {}",
            Alonecraft::TestLog::N(victim), talent->GetAmount(), before, damage);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warl_imperious_flames::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// ---------------------------------------------------------------------------
//  Item 9 -- Demonic Aegis empowers Demon Armor
// ---------------------------------------------------------------------------
//  Gated on Demon Armor specifically, not Fel Armor -- that distinction is the
//  point of the redesign, so the bonus is applied from an AuraScript on the
//  Demon Armor rank chain rather than from the talent itself.
//
//  200417 uses SPELL_AURA_MOD_RESISTANCE_PCT (101, TOTAL_PCT) rather than
//  MOD_BASE_RESISTANCE_PCT (142), which multiplies base armor only and would
//  make "+600%" a trivial number on a cloth wearer.
class spell_warl_demonic_aegis_armor : public AuraScript
{
    PrepareAuraScript(spell_warl_demonic_aegis_armor);

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* player = GetTarget() ? GetTarget()->ToPlayer() : nullptr;
        if (!player)
            return;

        AuraEffect const* armor = GetTalentEffect(player,
            {TALENT_DEMONIC_AEGIS_R1, TALENT_DEMONIC_AEGIS_R2, TALENT_DEMONIC_AEGIS_R3}, EFFECT_0);
        AuraEffect const* crit  = GetTalentEffect(player,
            {TALENT_DEMONIC_AEGIS_R1, TALENT_DEMONIC_AEGIS_R2, TALENT_DEMONIC_AEGIS_R3}, EFFECT_1);
        if (!armor || !crit)
            return;

        int32 armorPct = armor->GetAmount();
        int32 critPct  = -crit->GetAmount();   // stored positive, applied as avoidance

        // Drop any stale copy first: a respec between ranks would otherwise
        // leave the old amounts in place, since the buff never expires.
        uint32 const armorBefore = player->GetArmor();

        player->RemoveAurasDueToSpell(SPELL_DEMONIC_AEGIS_BUFF);
        player->CastCustomSpell(player, SPELL_DEMONIC_AEGIS_BUFF,
                                &armorPct, &critPct, &critPct, true);

        ACTEST("WARL.AEGIS",
            "applied on demonArmor={} armorPct={} critAvoidPct={} armor {} -> {}",
            GetSpellInfo()->Id, armorPct, critPct, armorBefore, player->GetArmor());
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* target = GetTarget())
        {
            target->RemoveAurasDueToSpell(SPELL_DEMONIC_AEGIS_BUFF);
            ACTEST("WARL.AEGIS", "removed with demonArmor={} armor now {}",
                GetSpellInfo()->Id, target->GetArmor());
        }
    }

    void Register() override
    {
        // AFTER, not On: the companion buff is cast onto the same unit the
        // aura is landing on, so the outer application has to have finished
        // first.  Same shape as core's spell_pal_blessing_of_sanctuary
        // (spell_paladin.cpp:589).  REAL_OR_REAPPLY also covers the rank
        // swap when Demon Armor is upgraded.
        AfterEffectApply  += AuraEffectApplyFn(spell_warl_demonic_aegis_armor::HandleApply, EFFECT_0, SPELL_AURA_MOD_RESISTANCE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        AfterEffectRemove += AuraEffectRemoveFn(spell_warl_demonic_aegis_armor::HandleRemove, EFFECT_0, SPELL_AURA_MOD_RESISTANCE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

//  The other half: 200417 has no duration, so once it is up nothing takes it
//  back down on its own.  Losing the talent -- respec, spec swap, unlearn --
//  removes the talent aura, and this drops the bonus with it.
//
//  This is the module's replacement for core's spell_warl_demonic_aegis
//  (spell_warlock.cpp:256), whose registration file 06 had to delete once the
//  talent's Effect1 became SPELL_AURA_DUMMY.  Same shape, same reason: core
//  strips Demon Armor and Fel Armor so the bonus has to be re-earned by
//  recasting.  Stripping Demon Armor here does that AND covers the rank
//  change case -- respeccing 3/3 down to 1/3 re-applies at the new amount
//  when the player recasts, rather than silently keeping the old one.
//
//  Bound to the talent, not to a hook, so no respec-timing question arises:
//  the engine removes the aura and the handler runs at that moment.
class spell_warl_demonic_aegis_talent : public AuraScript
{
    PrepareAuraScript(spell_warl_demonic_aegis_talent);

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        target->RemoveAurasDueToSpell(SPELL_DEMONIC_AEGIS_BUFF);
        // Word-B bit 0x20 is Demon Armor; Fel Armor (0x20000000) is left alone,
        // it never carried the bonus.
        target->RemoveAurasWithFamily(SPELLFAMILY_WARLOCK, 0, 0x20, 0, ObjectGuid::Empty);

        ACTEST("WARL.AEGIS", "talent {} lost -- bonus and Demon Armor stripped, armor now {}",
            GetSpellInfo()->Id, target->GetArmor());
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_warl_demonic_aegis_talent::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// ---------------------------------------------------------------------------
//  Item 13 -- Molten Core also applies a stacking DoT to Shadow Bolt
// ---------------------------------------------------------------------------
//  The talent keeps its original DBC-native proc: Effect1 is a
//  PROC_TRIGGER_SPELL that empowers the next Incinerate/Soul Fire, and
//  Effect2 extends Immolate.  Only Effect3 is new.
//
//  Core ships its own `spell_proc` row for -47245 with ProcFlags 262144
//  (PROC_FLAG_DONE_PERIODIC alone), which overrides the DBC for the whole rank
//  chain.  Shadow Bolt's direct hit is DONE_SPELL_MAGIC_DMG_CLASS_NEG, so
//  Effect2 could never have fired until that row was widened to 327680 in
//  woa_2026_08_02_05.sql.
//
//  Three things then keep the two effects from interfering:
//    * Effect2 carries a class mask limiting it to Shadow Bolt, and
//      AuraEffect::CheckEffectProc evaluates masks per effect.
//    * ProcChance is 100 (spell-wide, and Effect2 must fire on every Shadow
//      Bolt), so the talent's original 4/8/12% is re-rolled here for EFFECT_0.
//    * EFFECT_0 additionally re-imposes the periodic-only restriction that the
//      widened ProcFlags removed, so the Corruption/Immolate proc still sees
//      exactly the events it saw before.
//
//  The DoT follows the DK Unholy Blight model rather than stacking charges:
//  a refresh ADDS the new damage to whatever is still pending and restarts the
//  duration.
//
//  Shadow Bolt is a *spender*, not a passive rider: it needs a Molten Core
//  charge and eats one, exactly like the Incinerate and Soul Fire bonuses.
//  Those two are consumed by the engine, because the buff's ADD_PCT_MODIFIER
//  effects carry a SpellClassMask that matches them and Player::ApplySpellMod
//  drops a charge on every match.  Shadow Bolt is not in that mask -- adding it
//  would also hand Shadow Bolt the damage bonus -- so the charge is spent by
//  hand here instead.
class spell_warl_molten_core : public AuraScript
{
    PrepareAuraScript(spell_warl_molten_core);

    // Original per-rank chance for the Corruption -> Incinerate/Soul Fire proc,
    // preserved after ProcChance was raised to 100 for Effect3's sake.
    static int32 OriginalProcChance(uint32 spellId)
    {
        switch (spellId)
        {
            case TALENT_MOLTEN_CORE_R1: return 4;
            case TALENT_MOLTEN_CORE_R2: return 8;
            case TALENT_MOLTEN_CORE_R3: return 12;
            default:                    return 0;
        }
    }

    // EFFECT_0 keeps the talent's original behaviour and rate: periodic damage
    // only (the restriction core's spell_proc row used to enforce with
    // ProcFlags 262144, before it had to be widened for Effect2), rolled at the
    // talent's own 4/8/12% rather than the spell-wide ProcChance of 100.
    bool CheckOriginalProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
    {
        if (!(eventInfo.GetTypeMask() & PROC_FLAG_DONE_PERIODIC))
            return false;

        // Corruption specifically, not "any periodic damage the warlock deals".
        // Health Funnel is SPELLFAMILY_WARLOCK and ticks periodically, so with
        // only the flag check above it procs Molten Core.
        SpellInfo const* triggering = eventInfo.GetSpellInfo();
        if (!triggering || triggering->SpellFamilyName != SPELLFAMILY_WARLOCK ||
            !(triggering->SpellFamilyFlags[0] & CORRUPTION_FAMILY_MASK0))
            return false;

        int32 const chance = OriginalProcChance(GetId());
        bool const pass = roll_chance_i(chance);

        ACTEST("WARL.MOLTENCORE", "effect0 (Incinerate/Soul Fire) talent={} chance={} roll={}",
            GetId(), chance, pass ? "PASS" : "fail");

        return pass;
    }

    void HandleMoltenFury(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        Unit* victim = eventInfo.GetActionTarget();
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!caster || !victim || !victim->IsAlive() || !damageInfo || !damageInfo->GetDamage())
            return;

        SpellInfo const* triggering = eventInfo.GetSpellInfo();
        if (!triggering || triggering->SpellFamilyName != SPELLFAMILY_WARLOCK ||
            !(triggering->SpellFamilyFlags[0] & SHADOW_BOLT_FAMILY_MASK0))
            return;

        // Gated on the Molten Core buff, and spends a charge of it.  Looked up
        // across all three rank spells rather than off GetId(), so a leftover
        // buff from a lower rank still counts.
        Aura* moltenCore = nullptr;
        for (uint32 buffId : { SPELL_MOLTEN_CORE_BUFF_R1,
                               SPELL_MOLTEN_CORE_BUFF_R2,
                               SPELL_MOLTEN_CORE_BUFF_R3 })
            if ((moltenCore = caster->GetAura(buffId, caster->GetGUID())))
                break;

        if (!moltenCore)
        {
            ACTEST("WARL.MOLTENCORE",
                "Shadow Bolt hit with no Molten Core charge -- no Molten Fury");
            return;
        }

        SpellInfo const* dot = sSpellMgr->GetSpellInfo(SPELL_MOLTEN_FURY);
        if (!dot)
            return;

        // Share of the Shadow Bolt hit, spread over the DoT's ticks.  Same
        // shape as spell_dk_unholy_blight (spell_dk.cpp:2550-2557), which this
        // effect was modelled on.
        int32 perTick = int32(CalculatePct(damageInfo->GetDamage(), aurEff->GetAmount()));
        if (perTick <= 0)
            return;

        // Amplitude is uint32; cast before dividing so the arithmetic stays
        // signed (the build runs with -Werror).
        int32 const amplitude = int32(dot->Effects[EFFECT_0].Amplitude);
        if (amplitude <= 0)
            return;

        int32 const ticks = std::max<int32>(1, dot->GetMaxDuration() / amplitude);

        int32 const total = perTick;
        perTick /= ticks;
        if (perTick <= 0)
            return;

        uint8 const chargesBefore = moltenCore->GetCharges();

        ACTEST("WARL.MOLTENCORE",
            "Molten Fury on {} shadowBoltDmg={} pct={} total={} ticks={} "
            "perTick={} moltenCoreCharges={}->{}",
            Alonecraft::TestLog::N(victim), damageInfo->GetDamage(), aurEff->GetAmount(),
            total, ticks, perTick,
            chargesBefore, chargesBefore ? chargesBefore - 1 : 0);

        // Was a hand-rolled "read the old amount, add it, CastCustomSpell".
        // The core helper does the carry-over properly -- GetOldAmount() is the
        // pre-modifier value and GetTotalTicks()/GetTickNumber() count actual
        // ticks, where the old code multiplied the post-modifier GetAmount() by
        // GetDuration()/amplitude -- and, more importantly, it routes the cast
        // through AuraMunchingQueue when the target is not the caster.  Casting
        // a periodic aura on another unit synchronously from inside a proc is
        // exactly the aura-munching case that queue exists to avoid.
        victim->CastDelayedSpellWithPeriodicAmount(
            caster, SPELL_MOLTEN_FURY, SPELL_AURA_PERIODIC_DAMAGE, perTick);

        // Last, and only once the DoT is actually on the target -- every early
        // return above leaves the charge in the player's pocket.  DropCharge
        // removes the aura when the last one goes, same as the engine does for
        // the Incinerate/Soul Fire consumption.
        moltenCore->DropCharge();
    }

    void Register() override
    {
        DoCheckEffectProc += AuraCheckEffectProcFn(spell_warl_molten_core::CheckOriginalProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        OnEffectProc      += AuraEffectProcFn(spell_warl_molten_core::HandleMoltenFury, EFFECT_2, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Item 15 -- Nemesis: the warlock's own attacks may grant a Soul Shard
// ---------------------------------------------------------------------------
//  The pet half lives in WarlockDemonPets.cpp.  Here the talent rank IS the
//  spell id, so spell_proc's ProcsPerMinute column carries the 2/4/6 PPM rate
//  directly (the Killing Machine pattern), leaving only the shard grant.
class spell_warl_nemesis_shard : public AuraScript
{
    PrepareAuraScript(spell_warl_nemesis_shard);

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Player* player = GetTarget() ? GetTarget()->ToPlayer() : nullptr;
        if (!player)
            return;

        // Cheap early-out so a capped warlock never chat-spams.
        if (GetSoulShardCount(player) >= SOUL_SHARD_MAX)
            return;

        ACTEST("WARL.NEMESIS", "owner proc talent={} (spell_proc PPM gated)", GetId());
        AddSoulShards(player, 1, "nemesis-owner");
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warl_nemesis_shard::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }
};

// ---------------------------------------------------------------------------
//  Loaders
// ---------------------------------------------------------------------------

class spell_warl_improved_healthstone_mana_loader : public SpellScriptLoader
{
public:
    spell_warl_improved_healthstone_mana_loader() : SpellScriptLoader("spell_warl_improved_healthstone_mana") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_improved_healthstone_mana();
    }
};

class spell_warl_imperious_flames_loader : public SpellScriptLoader
{
public:
    spell_warl_imperious_flames_loader() : SpellScriptLoader("spell_warl_imperious_flames") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warl_imperious_flames();
    }
};

class spell_warl_demonic_aegis_armor_loader : public SpellScriptLoader
{
public:
    spell_warl_demonic_aegis_armor_loader() : SpellScriptLoader("spell_warl_demonic_aegis_armor") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_demonic_aegis_armor();
    }
};

class spell_warl_demonic_aegis_talent_loader : public SpellScriptLoader
{
public:
    spell_warl_demonic_aegis_talent_loader() : SpellScriptLoader("spell_warl_demonic_aegis_talent") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_demonic_aegis_talent();
    }
};

class spell_warl_molten_core_loader : public SpellScriptLoader
{
public:
    spell_warl_molten_core_loader() : SpellScriptLoader("spell_warl_molten_core") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_molten_core();
    }
};

class spell_warl_nemesis_shard_loader : public SpellScriptLoader
{
public:
    spell_warl_nemesis_shard_loader() : SpellScriptLoader("spell_warl_nemesis_shard") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_nemesis_shard();
    }
};

void AddSC_warl_demonology()
{
    new spell_warl_improved_healthstone_mana_loader();
    new spell_warl_imperious_flames_loader();
    new spell_warl_demonic_aegis_armor_loader();
    new spell_warl_demonic_aegis_talent_loader();
    new spell_warl_molten_core_loader();
    new spell_warl_nemesis_shard_loader();
}
