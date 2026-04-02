#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Riptide Rework — Shaman Restoration talent (Holy Shock pattern)
// Base spells: 61295 / 61299 / 61300 / 61301 (all ranks via -61295)
//
// The base Riptide spells are now DUMMY + TARGET_UNIT_TARGET_ANY (25),
// allowing targeting both friendly and hostile units.
//
// Part 1 (SpellScript on Riptide base):
//   CheckCast validates the target (enemy must be attackable, must face).
//   HandleDummy routes to the appropriate heal or damage sub-spell
//   based on the caster's relation to the target — exactly like Holy Shock.
//
// Part 2 (SpellScript on Chain Lightning, all ranks via -421):
//   First target with Riptide periodic (HoT from friendly cast or DoT
//   from hostile cast) gets consumed for +25% damage on all chain targets.

enum RiptideReworkSpells
{
    // Heal sub-spells (direct heal + HoT)
    RIPTIDE_HEAL_R1 = 200222,
    RIPTIDE_HEAL_R2 = 200223,
    RIPTIDE_HEAL_R3 = 200224,
    RIPTIDE_HEAL_R4 = 200225,

    // Damage sub-spells (direct damage + DoT)
    RIPTIDE_DAMAGE_R1 = 200226,
    RIPTIDE_DAMAGE_R2 = 200227,
    RIPTIDE_DAMAGE_R3 = 200228,
    RIPTIDE_DAMAGE_R4 = 200229,
};

static uint32 GetRiptideHealSpell(uint32 baseSpellId)
{
    switch (baseSpellId)
    {
        case 61295: return RIPTIDE_HEAL_R1;
        case 61299: return RIPTIDE_HEAL_R2;
        case 61300: return RIPTIDE_HEAL_R3;
        case 61301: return RIPTIDE_HEAL_R4;
        default:    return RIPTIDE_HEAL_R4;
    }
}

static uint32 GetRiptideDamageSpell(uint32 baseSpellId)
{
    switch (baseSpellId)
    {
        case 61295: return RIPTIDE_DAMAGE_R1;
        case 61299: return RIPTIDE_DAMAGE_R2;
        case 61300: return RIPTIDE_DAMAGE_R3;
        case 61301: return RIPTIDE_DAMAGE_R4;
        default:    return RIPTIDE_DAMAGE_R4;
    }
}

// --- Part 1: Riptide dual-use (heal friendly, damage hostile) ---

class spell_sha_riptide_dual : public SpellScript
{
    PrepareSpellScript(spell_sha_riptide_dual);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({
            RIPTIDE_HEAL_R1, RIPTIDE_HEAL_R2, RIPTIDE_HEAL_R3, RIPTIDE_HEAL_R4,
            RIPTIDE_DAMAGE_R1, RIPTIDE_DAMAGE_R2, RIPTIDE_DAMAGE_R3, RIPTIDE_DAMAGE_R4
        });
    }

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (Unit* target = GetExplTargetUnit())
        {
            if (!caster->IsFriendlyTo(target))
            {
                if (!caster->IsValidAttackTarget(target))
                    return SPELL_FAILED_BAD_TARGETS;

                if (!caster->isInFront(target))
                    return SPELL_FAILED_UNIT_NOT_INFRONT;
            }
        }
        else
            return SPELL_FAILED_BAD_TARGETS;
        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        uint32 baseId = GetSpellInfo()->Id;
        if (caster->IsFriendlyTo(target))
            caster->CastSpell(target, GetRiptideHealSpell(baseId), true);
        else
            caster->CastSpell(target, GetRiptideDamageSpell(baseId), true);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_sha_riptide_dual::CheckCast);
        OnEffectHitTarget += SpellEffectFn(spell_sha_riptide_dual::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_sha_riptide_dual_loader : public SpellScriptLoader
{
public:
    spell_sha_riptide_dual_loader() : SpellScriptLoader("spell_sha_riptide_dual") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_sha_riptide_dual();
    }
};

// --- Part 2: Chain Lightning consumes Riptide for +25% damage ---

class spell_sha_chain_lightning_riptide : public SpellScript
{
    PrepareSpellScript(spell_sha_chain_lightning_riptide);

    bool Load() override
    {
        _firstTarget = true;
        _hasRiptide = false;
        return true;
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        if (_firstTarget)
        {
            Unit* target = GetHitUnit();
            Unit* caster = GetCaster();
            if (target && caster)
            {
                // Check for Riptide HoT (friendly Riptide periodic heal)
                AuraEffect* riptideHot = target->GetAuraEffect(
                    SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_SHAMAN,
                    0, 0, 0x10, caster->GetGUID());

                if (riptideHot)
                {
                    _hasRiptide = true;
                    target->RemoveAura(riptideHot->GetBase());
                }

                // Also check for Riptide DoT (hostile Riptide periodic damage)
                if (!_hasRiptide)
                {
                    AuraEffect* riptideDot = target->GetAuraEffect(
                        SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_SHAMAN,
                        0, 0, 0x10, caster->GetGUID());

                    if (riptideDot)
                    {
                        _hasRiptide = true;
                        target->RemoveAura(riptideDot->GetBase());
                    }
                }
            }
            _firstTarget = false;
        }

        // Riptide increases Chain Lightning damage by 25%
        if (_hasRiptide)
            SetHitDamage(static_cast<int32>(GetHitDamage() * 1.25f));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_sha_chain_lightning_riptide::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }

    bool _firstTarget;
    bool _hasRiptide;
};

class spell_sha_chain_lightning_riptide_loader : public SpellScriptLoader
{
public:
    spell_sha_chain_lightning_riptide_loader() : SpellScriptLoader("spell_sha_chain_lightning_riptide") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_sha_chain_lightning_riptide();
    }
};

void AddSC_sha_riptide_rework()
{
    new spell_sha_riptide_dual_loader();
    new spell_sha_chain_lightning_riptide_loader();
}
