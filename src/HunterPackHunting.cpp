/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "ThreatManager.h"
#include "Unit.h"

// Pack Hunting (replaces Freezing Arrow) -- Hunter, trained at level 24
// Spell: 60192 -- woa_2026_08_11_04.sql
//
// TODO.md: "For 10s all threat from your abilities is transferred to your pet.
//  20s cooldown.  (Granted at Level 24 instead of 80)"
//
// Handled by DBC:
//   Almost all of it.  SPELL_EFFECT_REDIRECT_THREAT is fully generic --
//   Spell::EffectRedirectThreat (SpellEffects.cpp:5928) calls
//   RegisterRedirectThreat(m_spellInfo->Id, unitTarget->GetGUID(), damage) on
//   the CASTER's ThreatManager, and ThreatManager::AddThreat reads the
//   threat-gaining unit's own _redirectInfo (ThreatManager.cpp:424).  Base
//   points 99 + DieSides 1 = 100%, exactly as Misdirection (34477) does it.
//   Effect2's dummy aura on the caster carries the 10s duration, again copying
//   Misdirection's layout.
//
// Handled here, and only here:
//
//   * Teardown.  Nothing in the redirect machinery is tied to the aura's
//     lifetime -- registration is permanent until something unregisters it.
//     Core has the same problem and solves it the same way, in
//     spell_hun_misdirection_proc (spell_hunter.cpp:922).  Unlike Misdirection
//     there is no companion proc aura to check for, so the removal here is
//     unconditional.
//
//   * The no-pet case.  TARGET_UNIT_PET does not fail a cast when there is no
//     pet: Spell.cpp:1823 guards with `if (target && target->ToUnit())`, so the
//     redirect effect is silently skipped while the dummy aura still applies
//     and the 20s cooldown still burns.  A hunter would get a buff that does
//     nothing and no explanation.

enum PackHuntingSpells
{
    SPELL_HUNTER_PACK_HUNTING = 60192
};

class spell_hun_pack_hunting : public SpellScript
{
    PrepareSpellScript(spell_hun_pack_hunting);

    SpellCastResult CheckCast()
    {
        if (!GetCaster()->GetGuardianPet())
        {
            return SPELL_FAILED_NO_PET;
        }

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_hun_pack_hunting::CheckCast);
    }
};

class spell_hun_pack_hunting_aura : public AuraScript
{
    PrepareAuraScript(spell_hun_pack_hunting_aura);

    // EFFECT_1 because the dummy is DBC Effect2.  GetTarget() is the hunter --
    // the same unit EffectRedirectThreat registered the redirect on.
    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->GetThreatMgr().UnregisterRedirectThreat(SPELL_HUNTER_PACK_HUNTING);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_hun_pack_hunting_aura::OnRemove,
                                                EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_hunter_pack_hunting()
{
    RegisterSpellScript(spell_hun_pack_hunting);
    RegisterSpellScript(spell_hun_pack_hunting_aura);
}
