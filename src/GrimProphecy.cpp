#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Item.h"
#include "ItemTemplate.h"

// Grim Prophecy (renamed Corpse Explosion, talent 49158)
// Using Scourge Strike or Death Strike with a two-handed weapon has a 30%
// chance to increase your dodge chance by 1% (stacking, max 5, 15s duration).
// Registered on Scourge Strike (-55090) and Death Strike (-49998).

enum GrimProphecySpells
{
    GRIM_PROPHECY_TALENT = 49158,
    GRIM_PROPHECY_DODGE  = 200118,
    GRIM_PROPHECY_CHANCE = 30,
};

class spell_dk_grim_prophecy_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_dk_grim_prophecy_SpellScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ GRIM_PROPHECY_DODGE });
    }

    bool Load() override
    {
        _executed = false;
        return GetCaster()->IsPlayer();
    }

    void HandleAfterHit()
    {
        if (_executed)
            return;

        Unit* caster = GetCaster();
        if (!caster || !GetHitUnit())
            return;

        // Must have Grim Prophecy talent
        if (!caster->HasAura(GRIM_PROPHECY_TALENT))
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        // Must have a two-handed weapon equipped
        Item* mainHand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        if (!mainHand)
            return;

        ItemTemplate const* proto = mainHand->GetTemplate();
        if (!proto || proto->InventoryType != INVTYPE_2HWEAPON)
            return;

        _executed = true;

        // Roll proc chance
        if (!roll_chance_i(GRIM_PROPHECY_CHANCE))
            return;

        // Apply or refresh flat dodge buff
        caster->CastSpell(caster, GRIM_PROPHECY_DODGE, true);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_dk_grim_prophecy_SpellScript::HandleAfterHit);
    }

    bool _executed;
};

class spell_dk_grim_prophecy_loader : public SpellScriptLoader
{
public:
    spell_dk_grim_prophecy_loader() : SpellScriptLoader("spell_dk_grim_prophecy") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_grim_prophecy_SpellScript();
    }
};

void AddSC_dk_grim_prophecy()
{
    new spell_dk_grim_prophecy_loader();
}
