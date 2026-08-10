#include "Player.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Charge -- the in-combat gate, moved out of the DBC and into a script
// Ranks: 100 / 6178 / 11578 (registered as -100) -- woa_2026_08_08_05.sql
//
// TODO.md, Juggernaut: "Your Charge ability is now usable while in combat."
//
// Stock 3.3.5 implements that entirely in data: Charge carries
// SPELL_ATTR0_NOT_IN_COMBAT_ONLY_PEACEFUL (0x10000000), and Juggernaut (64976)
// carries SPELL_AURA_ABILITY_IGNORE_AURASTATE with EffectMiscValue 1 and a
// class mask matching Charge, which Spell::CheckCast reads at
// Spell.cpp:5731-5744 to clear reqCombat before the block at 5761-5762.
//
// Every part of that data was verified correct here and it still failed in
// game with SPELL_FAILED_AFFECTING_COMBAT ("You are in combat"), so the gate is
// made explicit instead: woa_2026_08_08_05.sql strips 0x10000000 from all three
// Charge ranks -- which makes CanBeUsedInCombat() true and retires line 5762
// for this spell -- and this script re-imposes the same restriction for anyone
// without the talent.
//
// The order is load-bearing and is the reason the attribute has to come out of
// the DBC rather than this script simply granting an exemption.  Spell::CheckCast
// runs CallScriptCheckCastHandlers() at Spell.cpp:6046, nearly 300 lines *after*
// the combat check.  A CheckCast handler can therefore add a failure but can
// never clear one that has already been returned.
//
// Charge's own description still reads "Cannot be used in combat."  That is
// left alone deliberately: it remains true for every warrior without
// Juggernaut, and it is the wording Blizzard ships.

enum ChargeInCombatSpells
{
    SPELL_WARRIOR_JUGGERNAUT = 64976,
};

class spell_warr_charge_in_combat : public SpellScript
{
    PrepareSpellScript(spell_warr_charge_in_combat);

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return SPELL_CAST_OK;

        if (caster->IsInCombat() && !caster->HasAura(SPELL_WARRIOR_JUGGERNAUT))
            return SPELL_FAILED_AFFECTING_COMBAT;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_warr_charge_in_combat::CheckCast);
    }
};

class spell_warr_charge_in_combat_loader : public SpellScriptLoader
{
public:
    spell_warr_charge_in_combat_loader() : SpellScriptLoader("spell_warr_charge_in_combat") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warr_charge_in_combat();
    }
};

void AddSC_war_charge_in_combat()
{
    new spell_warr_charge_in_combat_loader();
}
