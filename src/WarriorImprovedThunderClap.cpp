#include "AlonecraftTestLog.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Util.h"

#include <algorithm>

// Improved Thunder Clap -- Warrior Protection talent
// Talent ranks: 12287 / 12665 / 12666 -- woa_2026_08_10_01.sql
// Registered on -6343, i.e. every rank of Thunder Clap.
//
// Redesign: Thunder Clap gains radius 8 -> 10/12/15 yards and +33/66/100%
// damage, and bleeds every target it hits for 33/66/100% of block value over
// 6 sec.
//
// Handled by DBC:
//   The radius (ADD_FLAT_MODIFIER, SPELLMOD_RADIUS) and the damage
//   (ADD_PCT_MODIFIER, SPELLMOD_DAMAGE), both masked to Thunder Clap's family
//   flag so they cannot leak onto other Warrior abilities.  Effect 3 is a bare
//   SPELL_AURA_DUMMY that exists only to carry the per-rank bleed percentage.
//
// Handled here:
//   The bleed.  Block value is neither a stat nor a rating --
//   Player::GetShieldBlockValue (Player.cpp:5115) derives it from Strength and
//   the SHIELD_BLOCK_VALUE base mods -- so no aura can read it as a damage
//   source.  This is the same constraint that forces WarriorIncite.cpp and
//   WarriorSpellshield.cpp into C++.
//
// No shield equipped means no bleed, for free: GetShieldBlockValue returns 0
// without one, so there is deliberately no explicit weapon check here.
//
// Why OnEffectHitTarget on EFFECT_0.  That is Thunder Clap's SCHOOL_DAMAGE
// effect, so the hook runs once per victim that the (already radius-modified)
// target selection produced, and only for victims the spell actually landed
// on.  Unlike Blood and Thunder -- which needs to know about every victim
// before it can pick a donor, and so snapshots the list -- this effect is
// independent per target and needs no cross-target state.
//
// Why CastDelayedSpellWithPeriodicAmount rather than CastCustomSpell.  The
// helper (Unit.cpp:16312) is the Unholy Blight pattern: it carries over the
// remaining damage of an existing bleed using GetOldAmount() (the pre-modifier
// value) and GetTotalTicks()/GetTickNumber(), and it routes the cast through
// AuraMunchingQueue because the target is not the caster.  Applying a periodic
// aura to another unit synchronously from inside a spell effect handler is
// exactly the aura-munching case that queue exists to prevent.  A recast is
// therefore a carry-over refresh, not a plain overwrite -- leftover damage is
// folded into the new bleed instead of being thrown away.
//
// The payload 200661 carries SpellFamilyName 0 so it can pick up neither
// Warrior spell modifiers nor any Warrior proc filter, and it carries
// ALWAYS_HIT / IGNORE_DAMAGE_TAKEN_MODIFIERS / CANT_CRIT so the ticks do not
// re-roll a hit check, crit a second time, or get scaled twice -- the seeding
// Thunder Clap already went through all of that.

enum ImprovedThunderClapSpells
{
    SPELL_WARRIOR_IMP_THUNDER_CLAP_R1 = 12287,
    SPELL_WARRIOR_IMP_THUNDER_CLAP_R2 = 12665,
    SPELL_WARRIOR_IMP_THUNDER_CLAP_R3 = 12666,
    SPELL_WARRIOR_THUNDERSTRUCK       = 200661
};

class spell_warr_improved_thunder_clap : public SpellScript
{
    PrepareSpellScript(spell_warr_improved_thunder_clap);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_THUNDERSTRUCK });
    }

    void ApplyBleed(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;

        Unit* victim = GetHitUnit();
        if (!victim || !victim->IsAlive())
            return;

        // The percentage is the talent's dummy amount.  Highest rank first, so
        // a warrior who somehow carries more than one takes the better value.
        int32 pct = 0;
        uint32 rank = 0;
        for (uint32 id : { SPELL_WARRIOR_IMP_THUNDER_CLAP_R3,
                           SPELL_WARRIOR_IMP_THUNDER_CLAP_R2,
                           SPELL_WARRIOR_IMP_THUNDER_CLAP_R1 })
        {
            if (AuraEffect const* eff = player->GetAuraEffect(id, EFFECT_2))
            {
                pct = eff->GetAmount();
                rank = id;
                break;
            }
        }

        if (pct <= 0)
            return;

        uint32 const blockValue = player->GetShieldBlockValue();
        if (!blockValue)
            return;

        SpellInfo const* dot = sSpellMgr->GetSpellInfo(SPELL_WARRIOR_THUNDERSTRUCK);
        if (!dot)
            return;

        // Amplitude is uint32; cast before dividing so the arithmetic stays
        // signed (the build runs with -Werror).
        int32 const amplitude = int32(dot->Effects[EFFECT_0].Amplitude);
        if (amplitude <= 0)
            return;

        int32 const total = CalculatePct(int32(blockValue), pct);
        int32 const ticks = std::max<int32>(1, dot->GetMaxDuration() / amplitude);
        int32 const perTick = total / ticks;
        if (perTick <= 0)
            return;

        victim->CastDelayedSpellWithPeriodicAmount(
            player, SPELL_WARRIOR_THUNDERSTRUCK, SPELL_AURA_PERIODIC_DAMAGE, perTick);

        ACTEST("WAR.IMPTC", "rank={} pct={} blockValue={} total={} ticks={} perTick={} target={}",
            rank, pct, blockValue, total, ticks, perTick, Alonecraft::TestLog::N(victim));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(
            spell_warr_improved_thunder_clap::ApplyBleed, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

class spell_warr_improved_thunder_clap_loader : public SpellScriptLoader
{
public:
    spell_warr_improved_thunder_clap_loader() : SpellScriptLoader("spell_warr_improved_thunder_clap") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_warr_improved_thunder_clap();
    }
};

void AddSC_war_improved_thunder_clap()
{
    new spell_warr_improved_thunder_clap_loader();
}
