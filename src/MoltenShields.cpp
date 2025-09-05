#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "SpellScript.h"

enum MoltenShieldsSpells
{
    SPELL_MOLTEN_SHIELDS_R1 = 11094,
    SPELL_MOLTEN_SHIELDS_R2 = 13043,
    SPELL_FIRE_WARD_R1 = 543,
    SPELL_FIRE_WARD_R2 = 8457,
    SPELL_FIRE_WARD_R3 = 8458,
    SPELL_FIRE_WARD_R4 = 10223,
    SPELL_FIRE_WARD_R5 = 10225,
    SPELL_FIRE_WARD_R6 = 27128,
};

class spell_mage_molten_shields_fire_ward : public AuraScript
{
    PrepareAuraScript(spell_mage_molten_shields_fire_ward);

    void OnAbsorb(AuraEffect* aurEff, DamageInfo& /*dmgInfo*/, uint32& absorbAmount)
    {
        Unit* target = GetTarget();
        if (!target)
            return;

        Player* player = target->ToPlayer();
        if (!player)
            return;

        uint32 manaToRestore = 0;
        if (player->HasAura(SPELL_MOLTEN_SHIELDS_R2))
        {
            manaToRestore = absorbAmount;
        }
        else if (player->HasAura(SPELL_MOLTEN_SHIELDS_R1))
        {
            manaToRestore = absorbAmount / 2;
        }

        if (manaToRestore > 0)
        {
            player->EnergizeBySpell(player, GetSpellInfo()->Id, manaToRestore, POWER_MANA);
        }
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
