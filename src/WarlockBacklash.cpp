#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "SpellInfo.h"

// Backlash (redesigned) -- Warlock Destruction talent
// Ranks: 34935 / 34938 / 34939
//
// "Damage that would otherwise kill you instead consumes your Soulstone and
//  heals you for 20% of your maximum health."
//
// Handled by DBC:
//   Effect1: SPELL_AURA_PROC_TRIGGER_SPELL -> SPELL_AURA_SCHOOL_ABSORB with
//            MiscValue 127 (all schools) and an unlimited pool, which is what
//            gives this script its damage hook.
//   Effect2: the original +1/2/3% spell crit chance, deliberately kept.
//
// Handled by this script:
//   Lethal-damage detection, the Soulstone check/consume, and the heal.
//
// Structural precedent: spell_pri_guardian_spirit
// (src/server/scripts/Spells/spell_priest.cpp).  Ardent Defender
// (spell_paladin.cpp) and Will of the Necropolis (spell_dk.cpp) use the same
// DoEffectCalcAmount(-1) + OnEffectAbsorb pair -- 3.3.5a has no dedicated
// cheat-death aura type, so this IS the path.
//
// NOTE ON ORDERING: absorb runs inside Unit::CalcAbsorbResist, i.e. BEFORE
// the kill is dealt, so Backlash consumes the Soulstone before the death
// path (Player::GetResurrectionSpellId) could ever use it.  Backlash and the
// Soulstone resurrect are therefore mutually exclusive -- which is the
// intended reading of "instead consumes your Soulstone".
//
// Acore::AbsorbAuraOrderPred (SpellAuraEffects.h) sorts warlock/mage
// category-56 wards first, so Shadow Ward has already reduced dmgInfo by the
// time we test for lethality.  Nothing to do here.

enum BacklashSpells
{
    BACKLASH_R1   = 34935,
    BACKLASH_HEAL = 200400,
    BACKLASH_ICD  = 200506,
};

// How Player::GetResurrectionSpellId (Player.cpp) recognises a Soulstone
// Resurrection aura.  Matching on the same two fields rather than on a list
// of ids means this script and the release window's "Use Soulstone" button
// can never disagree about whether a Soulstone is up.
static constexpr uint32 SOULSTONE_SPELL_VISUAL = 99;
static constexpr uint32 SOULSTONE_SPELL_ICON   = 92;

// Fallback only, for the case where a reworked Soulstone ever loses the
// visual/icon pairing above.  Ranks per woa_2026_08_01_00.sql.
static constexpr uint32 SOULSTONE_AURAS[] =
{
    20707, 20762, 20763, 20764, 20765, 27239, 47883
};

// Percentage of maximum health restored when Backlash fires.
static constexpr uint32 BACKLASH_HEAL_PCT = 20;

/// Id of the Soulstone Resurrection aura the player is carrying, or 0.
static uint32 FindSoulstoneAura(Player* player)
{
    Unit::AuraEffectList const& dummyAuras = player->GetAuraEffectsByType(SPELL_AURA_DUMMY);
    for (AuraEffect const* aurEff : dummyAuras)
    {
        SpellInfo const* spellInfo = aurEff->GetSpellInfo();
        if (spellInfo->SpellVisual[0] == SOULSTONE_SPELL_VISUAL && spellInfo->SpellIconID == SOULSTONE_SPELL_ICON)
            return spellInfo->Id;
    }

    for (uint32 id : SOULSTONE_AURAS)
        if (player->HasAura(id))
            return id;

    return 0;
}

class spell_warl_backlash_cheat_death : public AuraScript
{
    PrepareAuraScript(spell_warl_backlash_cheat_death);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ BACKLASH_HEAL, BACKLASH_ICD });
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        // Unlimited pool -- Absorb() decides what, if anything, to soak.
        // Anything >= 0 here would make Unit::CalcAbsorbResist decrement the
        // shield and remove the talent aura on the first hit taken.
        amount = -1;

        // GetTarget() is only valid inside apply/remove/periodic hooks, so
        // the owner is read off the aura directly here.
        ACTEST("WARL.BACKLASH", "absorb pool armed rank={} owner={} amount={}",
            GetId(), GetUnitOwner() ? GetUnitOwner()->GetName() : "none", amount);
    }

    void Absorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        // Only interested in damage that would actually kill -- but while the
        // harness is recording, every call is traced, because otherwise "the
        // hook never ran" and "the hook ran and bailed" look identical in the
        // log.  Enabled() is a relaxed atomic load, so normal play pays for
        // one comparison and one load per hit taken.
        bool const lethal = dmgInfo.GetDamage() >= player->GetHealth();
        if (!lethal && !Alonecraft::TestLog::Enabled())
            return;

        uint32 const soulstoneId = FindSoulstoneAura(player);
        bool const onIcd = player->HasAura(BACKLASH_ICD);

        ACTEST("WARL.BACKLASH",
            "absorb rank={} damage={} health={} lethal={} soulstone={} icd={}",
            GetId(), dmgInfo.GetDamage(), player->GetHealth(), lethal, soulstoneId, onIcd);

        if (!lethal)
            return;

        // Belt-and-braces cooldown.  Consuming the Soulstone is already the
        // real limiter, but this stops a re-soulstone-then-die loop from
        // chaining.  Marker aura rather than a static map: it is per
        // character, survives nothing it should not, and the player can see
        // it.  Ardent Defender's 66233 does the same job.
        if (onIcd)
        {
            ACTEST("WARL.BACKLASH", "ICD active -- death allowed");
            return;
        }

        // Requires a Soulstone -- without one, the warlock simply dies.
        if (!soulstoneId)
        {
            ACTEST("WARL.BACKLASH", "no Soulstone aura -- death allowed");
            return;
        }

        player->RemoveAurasDueToSpell(soulstoneId);
        player->CastSpell(player, BACKLASH_ICD, true);

        int32 healAmount = int32(player->CountPctFromMaxHealth(BACKLASH_HEAL_PCT));
        player->CastCustomSpell(player, BACKLASH_HEAL, &healAmount, nullptr, nullptr, true);

        absorbAmount = dmgInfo.GetDamage();

        ACTEST("WARL.BACKLASH",
            "CHEAT DEATH soulstone={} consumed absorbed={} heal={} ({}% of maxHp={})",
            soulstoneId, absorbAmount, healAmount, BACKLASH_HEAL_PCT, player->GetMaxHealth());
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_backlash_cheat_death::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_warl_backlash_cheat_death::Absorb, EFFECT_0);
    }
};

class spell_warl_backlash_cheat_death_loader : public SpellScriptLoader
{
public:
    spell_warl_backlash_cheat_death_loader() : SpellScriptLoader("spell_warl_backlash_cheat_death") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_backlash_cheat_death();
    }
};

void AddSC_warl_backlash()
{
    new spell_warl_backlash_cheat_death_loader();
}
