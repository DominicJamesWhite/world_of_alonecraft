#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Player.h"
#include "SpellMgr.h"
#include "Unit.h"

class spell_no_swing_reset_arakkoa : public SpellScriptLoader
{
public:
    spell_no_swing_reset_arakkoa() : SpellScriptLoader("spell_no_swing_reset_arakkoa") { }

    class spell_no_swing_reset_arakkoa_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_no_swing_reset_arakkoa_SpellScript);

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            // Arakkoa aura ID
            if (caster->HasAura(33891))
            {
                SpellInfo const* spellInfo = GetSpellInfo();

                // If the spell is instant
                if (spellInfo->CalcCastTime(caster) == 0)
                {
                    // Prevent swing timer reset
                    caster->setAttackTimer(BASE_ATTACK, 0);
                }
            }
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_no_swing_reset_arakkoa_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_no_swing_reset_arakkoa_SpellScript();
    }
};

void AddSC_spell_no_swing_reset_arakkoa()
{
    new spell_no_swing_reset_arakkoa();
}
