#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Bloody Lesions (replaces Vendetta: 49015 / 50154 / 55136)
// Blood Boil applies a bleed DoT and refreshes diseases on each target hit.

enum BloodyLesionsSpells
{
    BLOODY_LESIONS_R1  = 49015,
    BLOODY_LESIONS_R2  = 50154,
    BLOODY_LESIONS_R3  = 55136,
    BLEED_DOT          = 200107,
};

// Disease IDs: Blood Plague, Frost Fever, Crypt Fever (R1-R3), Ebon Plague (R1-R3)
static void RefreshDKDiseases(Unit* target, ObjectGuid casterGuid)
{
    for (uint32 id : {55078u, 55095u,
                      50508u, 50509u, 50510u,
                      51726u, 51734u, 51735u})
        if (Aura* aura = target->GetAura(id, casterGuid))
            aura->RefreshTimersWithMods();
}

class spell_dk_bloody_lesions_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_dk_bloody_lesions_SpellScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ BLEED_DOT });
    }

    void HandleAfterHit()
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        // Determine talent rank and bleed damage scaling
        int32 bleedDmg = 0;
        if (player->HasAura(BLOODY_LESIONS_R3))
            bleedDmg = GetHitDamage() * 30 / 100;  // 30% of Blood Boil damage
        else if (player->HasAura(BLOODY_LESIONS_R2))
            bleedDmg = GetHitDamage() * 20 / 100;  // 20%
        else if (player->HasAura(BLOODY_LESIONS_R1))
            bleedDmg = GetHitDamage() * 10 / 100;  // 10%

        if (bleedDmg <= 0)
            return;

        // Apply bleed DoT (damage is total over 12s / 4 ticks = per-tick amount set by BP)
        // BasePoints = total damage, the periodic handler divides by tick count
        caster->CastCustomSpell(BLEED_DOT, SPELLVALUE_BASE_POINT0, bleedDmg, target, true);

        // Refresh all diseases on this target
        RefreshDKDiseases(target, caster->GetGUID());
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_dk_bloody_lesions_SpellScript::HandleAfterHit);
    }
};

class spell_dk_bloody_lesions_loader : public SpellScriptLoader
{
public:
    spell_dk_bloody_lesions_loader() : SpellScriptLoader("spell_dk_bloody_lesions") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_bloody_lesions_SpellScript();
    }
};

void AddSC_dk_bloody_lesions()
{
    new spell_dk_bloody_lesions_loader();
}
