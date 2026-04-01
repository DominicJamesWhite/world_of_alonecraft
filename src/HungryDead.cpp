#include "ScriptMgr.h"
#include "Player.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "SpellAuraEffects.h"

// Hungry Dead (Ravenous Dead Redesign) — Alonecraft
// Vanilla: Increases your total Strength by X% and ghoul Str contribution.
// New:     Your ghouls deal X% more damage for each of your diseases on the target.
//
// Rank 1 = 48965  (4% per disease)
// Rank 2 = 49571  (7% per disease)
// Rank 3 = 49572  (10% per disease)
//
// Scope: Only DK ghouls (entry 26125) and army ghouls (entry 24207).

enum HungryDeadSpells
{
    HUNGRY_DEAD_R1      = 48965,
};

enum HungryDeadNPCs
{
    NPC_DK_GHOUL        = 26125,
    NPC_ARMY_GHOUL      = 24207,
};

class dk_hungry_dead : public UnitScript
{
public:
    dk_hungry_dead() : UnitScript("dk_hungry_dead", true,
        { UNITHOOK_MODIFY_MELEE_DAMAGE, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (!attacker || !target || damage == 0)
            return;

        int32 bonus = CalcHungryDeadBonus(attacker, target, static_cast<int32>(damage));
        if (bonus > 0)
            damage += static_cast<uint32>(bonus);
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* /*spellInfo*/) override
    {
        if (!attacker || !target || damage <= 0)
            return;

        int32 bonus = CalcHungryDeadBonus(attacker, target, damage);
        if (bonus > 0)
            damage += bonus;
    }

private:
    static int32 CalcHungryDeadBonus(Unit* attacker, Unit* target, int32 damage)
    {
        if (!attacker->IsPet() && !attacker->IsGuardian())
            return 0;

        uint32 entry = attacker->GetEntry();
        if (entry != NPC_DK_GHOUL && entry != NPC_ARMY_GHOUL)
            return 0;

        Unit* owner = attacker->GetOwner();
        if (!owner)
            return 0;

        Player* player = owner->ToPlayer();
        if (!player || player->getClass() != CLASS_DEATH_KNIGHT)
            return 0;

        AuraEffect const* aurEff = player->GetAuraEffectOfRankedSpell(HUNGRY_DEAD_R1, EFFECT_0);
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

void AddSC_dk_hungry_dead()
{
    new dk_hungry_dead();
}
