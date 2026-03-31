#include "ScriptMgr.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "Unit.h"

// Chillblains drain mechanic (UnitScript approach)
// When Frost Fever ticks on a target that has Icy Clutch (Chillblains slow),
// drain a percentage of the tick damage as additional Frost damage and heal
// the DK for the drained amount.
// Each talent rank triggers a different Icy Clutch spell:
//   Rank 1 (50040) -> Icy Clutch 50434, drain 33%
//   Rank 2 (50041) -> Icy Clutch 50435, drain 66%
//   Rank 3 (50043) -> Icy Clutch 50436, drain 100%

enum ChillblainsSpells
{
    SPELL_DK_ICY_CLUTCH_R1    = 50434,
    SPELL_DK_ICY_CLUTCH_R2    = 50435,
    SPELL_DK_ICY_CLUTCH_R3    = 50436,
    SPELL_DK_FROST_FEVER      = 55095,
    SPELL_DK_CHILLBLAINS_HEAL = 200112,
};

class spell_dk_chillblains_drain : public UnitScript
{
public:
    spell_dk_chillblains_drain()
        : UnitScript("spell_dk_chillblains_drain", true,
            { UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK }) { }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker,
        uint32& damage, SpellInfo const* spellInfo) override
    {
        if (!attacker || !target || !spellInfo)
            return;

        if (spellInfo->Id != SPELL_DK_FROST_FEVER)
            return;

        // Check which rank of Icy Clutch the target has (each talent rank
        // triggers a different slow spell via ADD_TARGET_TRIGGER)
        int32 pct = 0;
        if (target->HasAura(SPELL_DK_ICY_CLUTCH_R3))
            pct = 100;
        else if (target->HasAura(SPELL_DK_ICY_CLUTCH_R2))
            pct = 66;
        else if (target->HasAura(SPELL_DK_ICY_CLUTCH_R1))
            pct = 33;

        if (pct == 0)
            return;

        int32 drainAmount = CalculatePct(static_cast<int32>(damage), pct);
        if (drainAmount <= 0)
            return;

        // Increase tick damage by the drain amount
        damage += drainAmount;

        // Heal the DK for the drain amount
        attacker->CastCustomSpell(SPELL_DK_CHILLBLAINS_HEAL,
            SPELLVALUE_BASE_POINT0, drainAmount, attacker, true);
    }
};

void AddSC_dk_chillblains()
{
    new spell_dk_chillblains_drain();
}
