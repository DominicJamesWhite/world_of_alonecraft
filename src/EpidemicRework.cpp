#include "ScriptMgr.h"
#include "Player.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "SpellAuraEffects.h"

// Epidemic (Unholy tier 1) — redesigned for Alonecraft
// Vanilla: Increases duration of Blood Plague and Frost Fever by 3/6s.
// New:     For each of your diseases on an enemy, your shadow damage
//          against them is increased by 8/15%.
//
// Rank 1 = 49036  (8% per disease)
// Rank 2 = 49562  (15% per disease)

enum EpidemicSpells
{
    EPIDEMIC_R1 = 49036,
};

class dk_epidemic_rework : public UnitScript
{
public:
    dk_epidemic_rework() : UnitScript("dk_epidemic_rework", true,
        { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK }) { }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!attacker || !target || !spellInfo || damage <= 0)
            return;

        if (!(spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_SHADOW))
            return;

        int32 bonus = CalcEpidemicBonus(attacker, target, damage);
        if (bonus > 0)
            damage += bonus;
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
    {
        if (!attacker || !target || !spellInfo || damage == 0)
            return;

        if (!(spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_SHADOW))
            return;

        int32 bonus = CalcEpidemicBonus(attacker, target, static_cast<int32>(damage));
        if (bonus > 0)
            damage += static_cast<uint32>(bonus);
    }

private:
    static int32 CalcEpidemicBonus(Unit* attacker, Unit* target, int32 damage)
    {
        Player* player = attacker->ToPlayer();
        if (!player || player->getClass() != CLASS_DEATH_KNIGHT)
            return 0;

        AuraEffect const* aurEff = player->GetAuraEffectOfRankedSpell(EPIDEMIC_R1, EFFECT_0);
        if (!aurEff)
            return 0;

        int32 pctPerDisease = aurEff->GetAmount();
        if (pctPerDisease <= 0)
            return 0;

        uint32 diseaseCount = target->GetDiseasesByCaster(player->GetGUID());
        if (diseaseCount == 0)
            return 0;

        return damage * pctPerDisease * static_cast<int32>(diseaseCount) / 100;
    }
};

void AddSC_dk_epidemic_rework()
{
    new dk_epidemic_rework();
}
