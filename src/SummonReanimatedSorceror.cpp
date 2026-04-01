/*
 * Alonecraft: Summon Reanimated Sorceror
 * Replaces the Death Knight's Summon Gargoyle with a ground-based necromancer.
 * Based on npc_pet_dk_ebon_gargoyle from pet_dk.cpp.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "SpellAuraEffects.h"

enum ReanimatedSorcerorSpells
{
    SPELL_GARGOYLE_STRIKE           = 51963,
    SPELL_DK_SUMMON_GARGOYLE_1      = 49206,
    SPELL_DK_SANCTUARY              = 54661,
    SPELL_DK_NIGHT_OF_THE_DEAD      = 62137,
};

struct npc_pet_dk_reanimated_sorceror : ScriptedAI
{
    npc_pet_dk_reanimated_sorceror(Creature* creature) : ScriptedAI(creature)
    {
        _despawnTimer = 32000; // 30 secs + 2 initial attack timer (no fly-out)
        _despawning = false;
        _initialSelection = true;
        _targetGUID.Clear();
    }

    void JustExitedCombat() override
    {
        EngagementOver();
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (!_EnterEvadeMode())
            return;

        me->ClearUnitState(UNIT_STATE_EVADE);

        if (Unit* owner = me->GetOwner())
        {
            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle(), MOTION_SLOT_ACTIVE);
        }
    }

    void InitializeAI() override
    {
        ScriptedAI::InitializeAI();
        Unit* owner = me->GetOwner();
        if (!owner)
            return;

        // Night of the Dead avoidance
        if (Aura* aur = me->GetAura(SPELL_DK_NIGHT_OF_THE_DEAD))
            if (AuraEffect* aurEff = owner->GetAuraEffect(SPELL_AURA_ADD_FLAT_MODIFIER, SPELLFAMILY_DEATHKNIGHT, 2718, 0))
                if (aur->GetEffect(0))
                    aur->GetEffect(0)->SetAmount(-aurEff->GetSpellInfo()->Effects[EFFECT_2].CalcValue());

        // No flying — stay on the ground
        _selectionTimer = 2000;
        _initialCastTimer = 0;
        _decisionTimer = 0;
    }

    void MySelectNextTarget()
    {
        Unit* owner = me->GetOwner();
        if (owner && owner->IsPlayer() && (!me->GetVictim() || me->GetVictim()->IsImmunedToSpell(sSpellMgr->GetSpellInfo(SPELL_GARGOYLE_STRIKE)) || !me->IsValidAttackTarget(me->GetVictim()) || !owner->CanSeeOrDetect(me->GetVictim())))
        {
            Unit* selection = owner->ToPlayer()->GetSelectedUnit();
            if (selection && selection != me->GetVictim() && me->IsValidAttackTarget(selection))
            {
                me->GetMotionMaster()->Clear(false);
                SetGazeOn(selection);
            }
            else if (!me->GetVictim() || !owner->CanSeeOrDetect(me->GetVictim()))
            {
                me->CombatStop(true);
                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, 0.0f);
                RemoveTargetAura();
            }
        }
    }

    void AttackStart(Unit* who) override
    {
        RemoveTargetAura();
        _targetGUID = who->GetGUID();
        me->AddAura(SPELL_DK_SUMMON_GARGOYLE_1, who);
        ScriptedAI::AttackStartCaster(who, 40);
    }

    void RemoveTargetAura()
    {
        if (Unit* target = ObjectAccessor::GetUnit(*me, _targetGUID))
            target->RemoveAura(SPELL_DK_SUMMON_GARGOYLE_1, me->GetGUID());
    }

    void Reset() override
    {
        _selectionTimer = 0;
        me->SetReactState(REACT_PASSIVE);
        MySelectNextTarget();
    }

    void Despawn()
    {
        RemoveTargetAura();

        me->CombatStop(true);
        me->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE, true);
        me->CastSpell(me, SPELL_DK_SANCTUARY, true);
        me->SetReactState(REACT_PASSIVE);

        _despawning = true;
    }

    void UpdateAI(uint32 diff) override
    {
        if (_initialSelection)
        {
            _initialSelection = false;
            // Find victim of Summon Gargoyle spell
            std::list<Unit*> targets;
            Acore::AnyUnfriendlyUnitInObjectRangeCheck u_check(me, me, 50.0f);
            Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(me, targets, u_check);
            Cell::VisitObjects(me, searcher, 50.0f);
            for (auto const& target : targets)
                if (target->GetAura(SPELL_DK_SUMMON_GARGOYLE_1, me->GetOwnerGUID()))
                {
                    target->RemoveAura(SPELL_DK_SUMMON_GARGOYLE_1, me->GetOwnerGUID());
                    SetGazeOn(target);
                    _targetGUID = target->GetGUID();
                    break;
                }
        }

        if (_despawnTimer > 2000)
        {
            _despawnTimer -= diff;
            _decisionTimer -= diff;
            if (!UpdateVictimWithGaze())
            {
                MySelectNextTarget();
                return;
            }

            _initialCastTimer += diff;
            _selectionTimer += diff;
            if (_selectionTimer >= 1000)
            {
                MySelectNextTarget();
                _selectionTimer = 0;
            }

            if (_decisionTimer <= 0)
            {
                _decisionTimer += 400;
                if (_initialCastTimer >= 2000 && !me->HasUnitState(UNIT_STATE_CASTING | UNIT_STATE_LOST_CONTROL) && me->GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_CONTROLLED) == NULL_MOTION_TYPE && rand_chance() > 20.0f)
                {
                    if (me->HasSilenceAura() || me->IsSpellProhibited(SPELL_SCHOOL_MASK_NATURE))
                        me->GetMotionMaster()->MoveChase(me->GetVictim());
                    else
                    {
                        me->GetMotionMaster()->MoveChase(me->GetVictim(), 40);
                        DoCastVictim(SPELL_GARGOYLE_STRIKE);
                    }
                }
            }
        }
        else
        {
            if (!_despawning)
                Despawn();

            if (_despawnTimer > diff)
                _despawnTimer -= diff;
            else
                me->DespawnOrUnsummon();
        }
    }

private:
    ObjectGuid _targetGUID;
    uint32 _despawnTimer;
    uint32 _selectionTimer;
    uint32 _initialCastTimer;
    int32 _decisionTimer;
    bool _despawning;
    bool _initialSelection;
};

void AddSC_dk_reanimated_sorceror()
{
    RegisterCreatureAI(npc_pet_dk_reanimated_sorceror);
}
