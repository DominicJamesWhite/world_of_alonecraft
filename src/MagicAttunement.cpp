#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

// Magic Attunement talent — mana refund on spell cast while Invocation is active.
// Ranks: 11247 (100% of mana cost), 12606 (200% of mana cost).
// Refund % is stored in DBC Effect1 BasePoints (CalcValue = 100/200).
// Proc triggered via spell_proc entry (fires on any spell cast).

enum MagicAttunementSpells
{
    MAGIC_ATTUNEMENT_RANK_1 = 11247,
    MAGIC_ATTUNEMENT_RANK_2 = 12606,
};

// Invocation (Evocation) aura IDs — all ranks
static constexpr uint32 INVOCATION_AURAS[] = { 604, 8450, 8451, 10173, 10174, 33944, 43015 };

static bool HasInvocationActive(Player* player)
{
    for (uint32 id : INVOCATION_AURAS)
        if (player->HasAura(id))
            return true;
    return false;
}

// Single AuraScript for both ranks — reads refund % from aura effect amount.
class spell_magic_attunement_proc : public AuraScript
{
    PrepareAuraScript(spell_magic_attunement_proc);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ MAGIC_ATTUNEMENT_RANK_1 });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* actor = eventInfo.GetActor();
        if (!actor || !actor->IsPlayer())
            return false;

        Spell const* procSpell = eventInfo.GetProcSpell();
        if (!procSpell)
        {
            LOG_DEBUG("scripts", "MagicAttunement::CheckProc - no proc spell for player {}",
                     actor->ToPlayer()->GetName());
            return false;
        }

        if (procSpell->GetPowerCost() <= 0)
            return false;

        if (!HasInvocationActive(actor->ToPlayer()))
            return false;

        return true;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Player* player = eventInfo.GetActor()->ToPlayer();
        if (!player)
        {
            LOG_DEBUG("scripts", "MagicAttunement::HandleProc - actor is not a player");
            return;
        }

        Spell const* procSpell = eventInfo.GetProcSpell();
        if (!procSpell)
        {
            LOG_DEBUG("scripts", "MagicAttunement::HandleProc - no proc spell for player {}",
                     player->GetName());
            return;
        }

        int32 manaCost = procSpell->GetPowerCost();
        int32 manaReturnPct = aurEff->GetAmount(); // 100 (R1) or 200 (R2)
        int32 manaReturn = CalculatePct(manaCost, manaReturnPct);

        if (manaReturn <= 0)
        {
            LOG_DEBUG("scripts", "MagicAttunement::HandleProc - manaReturn {} <= 0 (cost={}, pct={}) for player {}",
                     manaReturn, manaCost, manaReturnPct, player->GetName());
            return;
        }

        player->ModifyPower(POWER_MANA, manaReturn);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_magic_attunement_proc::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_magic_attunement_proc::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_magic_attunement_proc_loader : public SpellScriptLoader
{
public:
    spell_magic_attunement_proc_loader() : SpellScriptLoader("spell_magic_attunement_proc") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_magic_attunement_proc();
    }
};

void AddSC_magic_attunement()
{
    new spell_magic_attunement_proc_loader();
}
