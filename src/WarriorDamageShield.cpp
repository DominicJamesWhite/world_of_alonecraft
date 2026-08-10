#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

#include <algorithm>

// Damage Shield (redesigned) -- Warrior Protection talent
// Ranks: 58872 / 58874 -- woa_2026_08_09_17.sql
// Absorb payload: 200649
//
// TODO.md: "Damage Shield (2, 9): Redesigned. When you deal damage you fortify
//  yourself, shielding yourself for an amount equal to 10/20% of the damage
//  caused. (2 ranks)"
//
// Handled by DBC:
//   The talent is SPELL_AURA_DUMMY carrying the percentage, spell_proc supplies
//   the "when you deal damage" event, and 200649 is a plain
//   SPELL_AURA_SCHOOL_ABSORB.  The absorb itself needs no script -- core
//   consumes it in Unit::CalcAbsorbResist like any other shield.
//
// Handled here:
//   Only the arithmetic, because the size of the shield depends on a number
//   that exists solely at proc time.  This is the standard shape for
//   "shield for a fraction of X": a proc that casts an absorb spell with
//   computed base points.  Core does it for the T10 Protection 4-piece
//   (spell_warrior.cpp:1188) and the module does it in DivineAegisDamage.cpp,
//   which is the closer model because it also has to stack with an existing
//   shield and cap the result.
//
// Stacking rather than replacing.  A fresh CastCustomSpell would overwrite the
// existing absorb, so a warrior mid-rotation would repeatedly throw away most
// of the shield they had just built.  The current pool is read off 200649 and
// added first.
//
// The cap is a tenth of maximum health.  Without one, a long fight against a
// large pack turns a percentage of cumulative damage into an unbounded shield;
// with one, the talent is worth roughly one extra Shield Slam of effective
// health at any gear level and scales itself instead of needing a retune.
// Choosing max health over the level * 125 that Divine Aegis uses is
// deliberate: this is a tank talent and stamina is the stat it should follow.

enum DamageShieldSpells
{
    SPELL_WARRIOR_FORTIFY_ABSORB = 200649
};

class spell_warr_damage_shield_absorb : public AuraScript
{
    PrepareAuraScript(spell_warr_damage_shield_absorb);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_FORTIFY_ABSORB });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        return damageInfo && damageInfo->GetDamage() > 0;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Player* player = GetTarget() ? GetTarget()->ToPlayer() : nullptr;
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!player || !damageInfo)
            return;

        int32 shield = CalculatePct(int32(damageInfo->GetDamage()), aurEff->GetAmount());
        if (shield <= 0)
            return;

        // Add whatever is left of the current shield rather than discarding it.
        if (AuraEffect const* existing = player->GetAuraEffect(SPELL_WARRIOR_FORTIFY_ABSORB, EFFECT_0))
            shield += existing->GetAmount();

        int32 const cap = int32(player->GetMaxHealth() / 10);
        shield = std::min(shield, cap);

        player->CastCustomSpell(SPELL_WARRIOR_FORTIFY_ABSORB, SPELLVALUE_BASE_POINT0, shield, player, true, nullptr, aurEff);

        ACTEST("WAR.DAMAGESHIELD", "talent={} damage={} pct={} cap={} -> shield={}",
            GetId(), damageInfo->GetDamage(), aurEff->GetAmount(), cap, shield);
    }

    void Register() override
    {
        DoCheckProc  += AuraCheckProcFn(spell_warr_damage_shield_absorb::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_warr_damage_shield_absorb::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_warr_damage_shield_absorb_loader : public SpellScriptLoader
{
public:
    spell_warr_damage_shield_absorb_loader() : SpellScriptLoader("spell_warr_damage_shield_absorb") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_damage_shield_absorb();
    }
};

void AddSC_war_damage_shield()
{
    new spell_warr_damage_shield_absorb_loader();
}
