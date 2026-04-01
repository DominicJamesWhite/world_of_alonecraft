#include "ScriptMgr.h"
#include "Player.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "SpellAuraEffects.h"

// Desecration rework (Alonecraft):
//
// Your Blood Strikes and Blood Boil desecrate the ground beneath you.
// Targets in the area are slowed and take more damage from your diseases.
//
// The disease damage increase is stored as a DUMMY aura (Effect2) on the
// persistent area aura (Desecrated Ground).  This UnitScript reads that
// value and applies it as a percentage bonus to disease damage only,
// but only for the DK who cast the Desecrated Ground.
//
// 55741 = Desecrated Ground R1 (5% disease damage increase)
// 68766 = Desecrated Ground R2 (10% disease damage increase)

enum DesecrationSpells
{
    DESECRATED_GROUND_R1 = 55741,
    DESECRATED_GROUND_R2 = 68766,
};

class dk_desecration_rework : public UnitScript
{
public:
    dk_desecration_rework() : UnitScript("dk_desecration_rework", true,
        { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK }) { }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!attacker || !target || !spellInfo || damage <= 0)
            return;

        if (spellInfo->Dispel != DISPEL_DISEASE)
            return;

        int32 bonus = CalcDesecrationBonus(attacker, target, damage);
        if (bonus > 0)
            damage += bonus;
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
    {
        if (!attacker || !target || !spellInfo || damage == 0)
            return;

        if (spellInfo->Dispel != DISPEL_DISEASE)
            return;

        int32 bonus = CalcDesecrationBonus(attacker, target, static_cast<int32>(damage));
        if (bonus > 0)
            damage += static_cast<uint32>(bonus);
    }

private:
    static int32 CalcDesecrationBonus(Unit* attacker, Unit* target, int32 damage)
    {
        // Check each rank of Desecrated Ground on the target (higher rank first)
        static const std::pair<uint32, int32> ranks[] = {
            { DESECRATED_GROUND_R2, 10 },  // R2: 10% disease damage increase
            { DESECRATED_GROUND_R1,  5 },  // R1: 5% disease damage increase
        };

        for (auto const& [spellId, pct] : ranks)
        {
            Aura* aura = target->GetAura(spellId);
            if (!aura)
                continue;

            // Only boost damage for the DK who cast this Desecrated Ground
            if (aura->GetCasterGUID() != attacker->GetGUID())
                continue;

            return damage * pct / 100;
        }

        return 0;
    }
};

void AddSC_dk_desecration_rework()
{
    new dk_desecration_rework();
}
