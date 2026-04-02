#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Focused Mind (redesigned) — Shaman Elemental talent
// Ranks: 30864 / 30865 / 30866 (SPELL_AURA_DUMMY passives)
// Mechanic:
//   Casting Healing Wave (family flag 0x40), Lesser HW (0x80), or Chain Heal (0x100)
//   applies "Cleared Mind" buff (200209) with bonus% = talent BasePoints.
//   Casting Lightning Bolt (0x01) or Chain Lightning (0x02) while Cleared Mind is
//   active multiplies damage by (1 + bonus%) and removes the buff.
//
// Two scripts:
//   1. AuraScript on talent ranks — procs on heal cast, applies 200209
//   2. SpellScript on LB/CL — consumes 200209 and multiplies damage

enum FocusedMindSpells
{
    FOCUSED_MIND_R1    = 30864,
    FOCUSED_MIND_R2    = 30865,
    FOCUSED_MIND_R3    = 30866,
    CLEARED_MIND_BUFF  = 200209,
};

// --- AuraScript on talent: proc on heal cast, apply Cleared Mind ---

class spell_sha_focused_mind_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_sha_focused_mind_AuraScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ CLEARED_MIND_BUFF });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        if (!caster)
            return;

        int32 bonus = aurEff->GetAmount();
        if (bonus <= 0)
            return;

        caster->CastCustomSpell(CLEARED_MIND_BUFF, SPELLVALUE_BASE_POINT0, bonus, caster, true);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_sha_focused_mind_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_sha_focused_mind_loader : public SpellScriptLoader
{
public:
    spell_sha_focused_mind_loader() : SpellScriptLoader("spell_sha_focused_mind") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_sha_focused_mind_AuraScript();
    }
};

// --- SpellScript on Lightning Bolt / Chain Lightning: consume Cleared Mind ---

class spell_sha_cleared_mind_consume_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_sha_cleared_mind_consume_SpellScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ CLEARED_MIND_BUFF });
    }

    void HandleOnHit()
    {
        Unit* caster = GetCaster();
        if (!caster || !GetHitUnit())
            return;

        Aura* buff = caster->GetAura(CLEARED_MIND_BUFF);
        if (!buff)
            return;

        AuraEffect const* eff = buff->GetEffect(EFFECT_0);
        if (!eff)
            return;

        int32 bonusPct = eff->GetAmount();
        if (bonusPct <= 0)
            return;

        int32 damage = GetHitDamage();
        if (damage <= 0)
            return;

        SetHitDamage(damage + CalculatePct(damage, bonusPct));

        caster->RemoveAurasDueToSpell(CLEARED_MIND_BUFF);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_sha_cleared_mind_consume_SpellScript::HandleOnHit);
    }
};

class spell_sha_cleared_mind_consume_loader : public SpellScriptLoader
{
public:
    spell_sha_cleared_mind_consume_loader() : SpellScriptLoader("spell_sha_cleared_mind_consume") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_sha_cleared_mind_consume_SpellScript();
    }
};

void AddSC_sha_focused_mind()
{
    new spell_sha_focused_mind_loader();
    new spell_sha_cleared_mind_consume_loader();
}
