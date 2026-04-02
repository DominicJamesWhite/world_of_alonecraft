#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Healing Focus (redesigned) — Shaman Restoration talent
// Ranks: 16181 / 16230 / 16232 (SPELL_AURA_DUMMY passives)
// BP = 33/66/100 (chance %)
// ProcFlags in spell_proc: DONE_SPELL_MAGIC_DMG_CLASS_NEG | DONE_SPELL_MAGIC_DMG_CLASS_POS = 81920
//
// Two effects:
//   1. Shock spells (family 11, flags & 0x90100000) have 33/66/100% chance
//      to make next Healing Wave free and instant (buff 200211, 1 charge, 15s).
//   2. Casting a heal (family 11, flags & 0x1C0) on a target that has YOUR
//      Earth Shield, Water Shield or Lightning Shield applies damage buff
//      200212 (1 charge, 25% damage increase, 15s).

enum HealingFocusSpells
{
    INSTANT_HW_BUFF    = 200211,
    SHIELD_DAMAGE_BUFF = 200212,
};

// Shock family flags: Earth=0x100000, Flame=0x10000000, Frost=0x80000000
static constexpr uint32 SHOCK_FAMILY_MASK = 0x90100000;
// Heal family flags: HW=0x40, LHW=0x80, CH=0x100
static constexpr uint32 HEAL_FAMILY_MASK  = 0x1C0;

// Earth Shield player-castable ranks
static constexpr uint32 EARTH_SHIELD_IDS[] = { 974, 32593, 32594, 49283, 49284 };
// Water Shield player-castable ranks
static constexpr uint32 WATER_SHIELD_IDS[] = { 52127, 52129, 52131, 52134, 52136, 52138, 24398, 33736 };
// Lightning Shield player-castable ranks
static constexpr uint32 LIGHTNING_SHIELD_IDS[] = { 324, 325, 905, 945, 8134, 10431, 10432, 25469, 25472, 49280, 49281 };

static bool HasOwnShield(Unit* target, ObjectGuid casterGuid)
{
    for (uint32 id : EARTH_SHIELD_IDS)
        if (target->GetAura(id, casterGuid))
            return true;
    for (uint32 id : WATER_SHIELD_IDS)
        if (target->GetAura(id, casterGuid))
            return true;
    for (uint32 id : LIGHTNING_SHIELD_IDS)
        if (target->GetAura(id, casterGuid))
            return true;
    return false;
}

class spell_sha_healing_focus_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_sha_healing_focus_AuraScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ INSTANT_HW_BUFF, SHIELD_DAMAGE_BUFF });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        SpellInfo const* procSpell = eventInfo.GetSpellInfo();
        if (!procSpell || procSpell->SpellFamilyName != SPELLFAMILY_SHAMAN)
            return false;

        uint32 flags = procSpell->SpellFamilyFlags[0];
        return (flags & SHOCK_FAMILY_MASK) || (flags & HEAL_FAMILY_MASK);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        SpellInfo const* procSpell = eventInfo.GetSpellInfo();
        if (!caster || !procSpell)
            return;

        uint32 flags = procSpell->SpellFamilyFlags[0];

        // Effect 1: Shock — chance to grant free/instant Healing Wave
        if (flags & SHOCK_FAMILY_MASK)
        {
            int32 chance = aurEff->GetAmount();
            if (roll_chance_i(chance))
                caster->CastSpell(caster, INSTANT_HW_BUFF, true);
            return;
        }

        // Effect 2: Heal on target with YOUR shield — grant damage buff
        if (flags & HEAL_FAMILY_MASK)
        {
            Unit* target = eventInfo.GetActionTarget();
            if (!target)
                return;

            if (HasOwnShield(target, caster->GetGUID()))
                caster->CastSpell(caster, SHIELD_DAMAGE_BUFF, true);
        }
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_sha_healing_focus_AuraScript::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_sha_healing_focus_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_sha_healing_focus_loader : public SpellScriptLoader
{
public:
    spell_sha_healing_focus_loader() : SpellScriptLoader("spell_sha_healing_focus") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_sha_healing_focus_AuraScript();
    }
};

void AddSC_sha_healing_focus()
{
    new spell_sha_healing_focus_loader();
}
