#include "CellImpl.h"
#include "GridNotifiers.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Mark of Blood Rework (49005)
// Two components:
// A) Parry proc: parrying an attack from the marked enemy grants armor pen buff (200108)
// B) Heal reflect: X% of self-healing is dealt as damage to the marked target

enum MarkOfBloodSpells
{
    MARK_OF_BLOOD      = 49005,
    ARMOR_PEN_BUFF     = 200108,
    MOB_DAMAGE_SPELL   = 200126,
    HEAL_REFLECT_PCT   = 30,  // 30% of self-healing dealt as damage
};

// Component A: AuraScript on Mark of Blood (sits on enemy)
// Procs only on parry via spell_proc (ProcFlags=4, HitMask=32)
class spell_dk_mark_of_blood_rework_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_dk_mark_of_blood_rework_AuraScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ ARMOR_PEN_BUFF });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        // The aura is on the enemy. eventInfo.GetActor() = enemy (attacker).
        // eventInfo.GetActionTarget() = the DK (defender who parried).
        Unit* dk = eventInfo.GetActionTarget();
        if (!dk)
            return;

        dk->CastSpell(dk, ARMOR_PEN_BUFF, true, nullptr, aurEff);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_dk_mark_of_blood_rework_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_dk_mark_of_blood_rework_loader : public SpellScriptLoader
{
public:
    spell_dk_mark_of_blood_rework_loader() : SpellScriptLoader("spell_dk_mark_of_blood_rework") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_mark_of_blood_rework_AuraScript();
    }
};

// Component B: UnitScript for heal-reflects-damage
// When a DK heals themselves, deal 30% of that heal as damage to the marked target.
// Uses ModifyHealReceived (fires BEFORE overheal capping) so overhealing still counts.
// Uses CastCustomSpell with a helper spell (200126) so damage shows in combat log.
class spell_dk_mark_of_blood_heal_reflect : public UnitScript
{
public:
    spell_dk_mark_of_blood_heal_reflect()
        : UnitScript("spell_dk_mark_of_blood_heal_reflect", true, { UNITHOOK_MODIFY_HEAL_RECEIVED }) { }

    void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* /*spellInfo*/) override
    {
        // Self-heal only
        if (!healer || healer != target)
            return;

        Player* dk = healer->ToPlayer();
        if (!dk || dk->getClass() != CLASS_DEATH_KNIGHT)
            return;

        if (heal == 0)
            return;

        // Find a nearby enemy with Mark of Blood from this DK
        Unit* markedTarget = FindMarkedTarget(dk);
        if (!markedTarget)
            return;

        int32 damage = CalculatePct(static_cast<int32>(heal), HEAL_REFLECT_PCT);
        if (damage <= 0)
            return;

        dk->CastCustomSpell(MOB_DAMAGE_SPELL, SPELLVALUE_BASE_POINT0, damage, markedTarget, true);
    }

private:
    Unit* FindMarkedTarget(Player* dk)
    {
        // Scan nearby enemies for Mark of Blood aura from this DK
        std::list<Unit*> enemies;
        Acore::AnyUnfriendlyUnitInObjectRangeCheck check(dk, dk, 40.0f);
        Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(dk, enemies, check);
        Cell::VisitObjects(dk, searcher, 40.0f);

        for (Unit* enemy : enemies)
        {
            if (enemy->GetAura(MARK_OF_BLOOD, dk->GetGUID()))
                return enemy;
        }

        return nullptr;
    }
};

void AddSC_dk_mark_of_blood_rework()
{
    new spell_dk_mark_of_blood_rework_loader();
    new spell_dk_mark_of_blood_heal_reflect();
}
