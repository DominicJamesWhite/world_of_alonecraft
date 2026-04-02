#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"

// Defence of Nature (redesigned) — Shaman Restoration talent
// Ranks: 16184 / 16209 (passive SPELL_AURA_DUMMY, BP = 25/50)
// Mechanic:
//   When Water Shield, Earth Shield, or Lightning Shield procs,
//   25/50% chance to deal instant AoE nature damage (200214) to all
//   enemies within 15 yards.
//
// Pattern: SpellScript on the shield triggered spells (like Gift of
// the Earthmother on Lifebloom's final heal in mod_lifebloom.cpp).
// Fires only when a shield actually procs — no global PlayerScript.
//
// Registered on:
//   Earth Shield heal:       379
//   Lightning Shield damage: 26364-26372, 49280, 49281
//   Water Shield mana:       52128, 52130, 52132, 52133, 52135, 52137,
//                            23575, 33737, 57961

enum DefenceOfNatureSpells
{
    DEFENCE_OF_NATURE_R1 = 16184,
    DEFENCE_OF_NATURE_R2 = 16209,
    NATURE_AOE_DAMAGE    = 200214,
};

class spell_sha_defence_of_nature_trigger : public SpellScript
{
    PrepareSpellScript(spell_sha_defence_of_nature_trigger);

    void HandleAfterHit()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        int32 chance = 0;
        if (AuraEffect const* eff = caster->GetAuraEffect(DEFENCE_OF_NATURE_R2, EFFECT_0))
            chance = eff->GetAmount();
        else if (AuraEffect const* eff = caster->GetAuraEffect(DEFENCE_OF_NATURE_R1, EFFECT_0))
            chance = eff->GetAmount();

        if (chance <= 0)
            return;

        if (!roll_chance_i(chance))
            return;

        caster->CastSpell(caster, NATURE_AOE_DAMAGE, true);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_sha_defence_of_nature_trigger::HandleAfterHit);
    }
};

class spell_sha_defence_of_nature_loader : public SpellScriptLoader
{
public:
    spell_sha_defence_of_nature_loader() : SpellScriptLoader("spell_sha_defence_of_nature") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_sha_defence_of_nature_trigger();
    }
};

void AddSC_sha_defence_of_nature()
{
    new spell_sha_defence_of_nature_loader();
}
