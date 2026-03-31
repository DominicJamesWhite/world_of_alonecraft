#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Improved Rune Tap (48985 / 49488 / 49489)
// Additionally, Rune Tap increases your Heart Strike and Death Strike
// damage by 10/15/20% for 8 seconds.

enum ImprovedRuneTapSpells
{
    IMP_RUNE_TAP_R1   = 48985,
    IMP_RUNE_TAP_R2   = 49488,
    IMP_RUNE_TAP_R3   = 49489,
    RUNE_TAP_BUFF     = 200106,
};

class spell_dk_improved_rune_tap_buff_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_dk_improved_rune_tap_buff_SpellScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ RUNE_TAP_BUFF });
    }

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        int32 buffPct = 0;
        if (player->HasAura(IMP_RUNE_TAP_R3))
            buffPct = 20;
        else if (player->HasAura(IMP_RUNE_TAP_R2))
            buffPct = 15;
        else if (player->HasAura(IMP_RUNE_TAP_R1))
            buffPct = 10;

        if (buffPct == 0)
            return;

        player->CastCustomSpell(RUNE_TAP_BUFF, SPELLVALUE_BASE_POINT0, buffPct, player, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_dk_improved_rune_tap_buff_SpellScript::HandleAfterCast);
    }
};

class spell_dk_improved_rune_tap_buff_loader : public SpellScriptLoader
{
public:
    spell_dk_improved_rune_tap_buff_loader() : SpellScriptLoader("spell_dk_improved_rune_tap_buff") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_improved_rune_tap_buff_SpellScript();
    }
};

void AddSC_dk_improved_rune_tap()
{
    new spell_dk_improved_rune_tap_buff_loader();
}
