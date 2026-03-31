#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "SpellScript.h"

enum MoltenShieldsSpells
{
    SPELL_MOLTEN_SHIELDS_R1   = 11094,
    SPELL_MOLTEN_SHIELDS_R2   = 13043,
    MOLTEN_SHIELDS_MANA       = 200104,
};

class spell_mage_molten_shields_fire_ward : public AuraScript
{
    PrepareAuraScript(spell_mage_molten_shields_fire_ward);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ MOLTEN_SHIELDS_MANA });
    }

    void OnAbsorb(AuraEffect* aurEff, DamageInfo& /*dmgInfo*/, uint32& absorbAmount)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        int32 manaToRestore = 0;
        if (player->HasAura(SPELL_MOLTEN_SHIELDS_R2))
            manaToRestore = static_cast<int32>(absorbAmount);
        else if (player->HasAura(SPELL_MOLTEN_SHIELDS_R1))
            manaToRestore = static_cast<int32>(absorbAmount / 2);

        if (manaToRestore > 0)
            player->CastCustomSpell(MOLTEN_SHIELDS_MANA, SPELLVALUE_BASE_POINT0, manaToRestore, player, true, nullptr, aurEff);
    }

    void Register() override
    {
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_mage_molten_shields_fire_ward::OnAbsorb, EFFECT_0);
    }
};

class spell_mage_molten_shields_fire_ward_loader : public SpellScriptLoader
{
public:
    spell_mage_molten_shields_fire_ward_loader() : SpellScriptLoader("spell_mage_molten_shields_fire_ward") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_molten_shields_fire_ward();
    }
};

void AddSC_molten_shields_fire_ward()
{
    new spell_mage_molten_shields_fire_ward_loader();
}
