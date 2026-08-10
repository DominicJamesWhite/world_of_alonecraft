#include "AlonecraftTestLog.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Util.h"

// Incite (redesigned) -- Warrior Protection talent
// Ranks: 50685 / 50686 / 50687 -- woa_2026_08_09_44.sql
//
// TODO.md: "Incite (1, 1): Redesigned. Your Heroic Strike, Thunder Clap and
//  Cleave have a chance equal to your shield block chance to do additional
//  damage equal to 33/66/100% of your shield block value. (3 ranks)"
//
// Handled by DBC:
//   The per-rank percentage (SPELL_AURA_DUMMY base points, which $s1 also
//   prints) and the proc event.  The spell_proc row filters to the three
//   abilities by SpellFamilyMask 4194496 -- stock Incite's own mask, since
//   retail's version buffed exactly these three.
//
// Handled here:
//   The roll and the damage.
//
// Why a script at all.  Two independent reasons:
//
//   * No spell_proc column can refer to a character stat.  Chance is a
//     constant, and this talent's chance IS the player's block chance.
//
//   * Block value is neither a stat nor a rating -- Player::GetShieldBlockValue
//     (Player.cpp:5115) derives it from Strength and the SHIELD_BLOCK_VALUE
//     base mods -- so no aura can read it as a damage source.
//
// This is Spellshield (WarriorSpellshield.cpp) from the offensive side, and it
// reuses that script's reasoning verbatim:
//
//   PLAYER_BLOCK_PERCENTAGE rather than
//   GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_PERCENT), because the field is
//   the final figure the melee attack table itself uses -- it already includes
//   defense skill, block rating and every aura, including Shield Block while it
//   is up, which is what makes pressing it feel like an offensive cooldown too.
//
//   No shield equipped means no proc, for free: the field is 0 without one
//   (Player::UpdateBlockPercentage, StatSystem.cpp:633), so the roll simply
//   never succeeds.  There is no explicit weapon check here on purpose.
//
// The damage goes out as a helper spell rather than as raw damage so that it is
// attributed in the combat log; 200660 carries SpellFamilyName 0 so it can
// neither match this talent's own proc filter nor pick up warrior spell
// modifiers.  MarkOfBloodRework.cpp uses the same helper-spell shape.
//
// The amount cannot live in the payload's base points because it is unknown
// until the proc fires, hence CastCustomSpell.

enum InciteSpells
{
    SPELL_WARRIOR_INCITE_DAMAGE = 200660
};

class spell_warr_incite : public AuraScript
{
    PrepareAuraScript(spell_warr_incite);

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Player* player = GetTarget() ? GetTarget()->ToPlayer() : nullptr;
        if (!player)
            return;

        Unit* victim = eventInfo.GetProcTarget();
        if (!victim || !player->IsValidAttackTarget(victim))
            return;

        int32 const pct = aurEff->GetAmount();
        if (pct <= 0)
            return;

        float const blockChance = player->GetFloatValue(PLAYER_BLOCK_PERCENTAGE);
        if (blockChance <= 0.0f || !roll_chance_f(blockChance))
            return;

        uint32 const blockValue = player->GetShieldBlockValue();
        if (!blockValue)
            return;

        int32 const damage = CalculatePct(int32(blockValue), pct);
        if (damage <= 0)
            return;

        player->CastCustomSpell(SPELL_WARRIOR_INCITE_DAMAGE, SPELLVALUE_BASE_POINT0, damage, victim, true);

        ACTEST("WAR.INCITE", "rank={} trigger={} blockChance={:.2f} blockValue={} pct={} -> damage={}",
            GetId(), eventInfo.GetSpellInfo() ? eventInfo.GetSpellInfo()->Id : 0,
            blockChance, blockValue, pct, damage);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warr_incite::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_warr_incite_loader : public SpellScriptLoader
{
public:
    spell_warr_incite_loader() : SpellScriptLoader("spell_warr_incite") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warr_incite();
    }
};

void AddSC_war_incite()
{
    new spell_warr_incite_loader();
}
