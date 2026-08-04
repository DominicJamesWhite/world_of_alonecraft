#include "AlonecraftTestLog.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "WarlockShards.h"

using namespace Alonecraft::Warlock;

// Wailing Souls (redesigned from Nether Protection) -- Warlock Destruction
// Ranks: 30299 / 30301 / 30302
//
// "Casting Searing Pain transforms a Soul Shard into a Wailing Soul. Wailing
//  Souls reduce damage taken by 15% for 10 seconds. Max 3 stacks."
//
// Handled by DBC / spell_proc:
//   Talent ranks become SPELL_AURA_DUMMY proc carriers.  spell_proc filters
//   to Searing Pain (SpellFamilyMask0 = 256) with a per-rank chance of
//   34/67/100%.  Spell 200402 "Wailing Soul" carries the damage reduction,
//   the 10s duration, and StackAmount = 3 -- so the stack cap costs no code.
//
// Handled by this script:
//   Only the shard consumption.  Proc-time reagent cost has no DBC
//   representation, so the conversion "a Soul Shard becomes a Wailing Soul"
//   has to happen here.  No shard means no Wailing Soul.

enum NetherProtectionSpells
{
    WAILING_SOULS_R1  = 30299,
    WAILING_SOUL_BUFF = 200402,
};

class spell_warl_wailing_soul : public AuraScript
{
    PrepareAuraScript(spell_warl_wailing_soul);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ WAILING_SOUL_BUFF });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        if (!caster)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        // The shard IS the Wailing Soul -- no shard, no soul.
        if (!ConsumeSoulShards(player, 1, "wailing-soul"))
        {
            ACTEST("WARL.WAILING", "talent={} proc but no shard -- no Wailing Soul", GetId());
            return;
        }

        player->CastSpell(player, WAILING_SOUL_BUFF, true, nullptr, aurEff);

        Aura const* buff = player->GetAura(WAILING_SOUL_BUFF, player->GetGUID());
        ACTEST("WARL.WAILING", "talent={} Wailing Soul applied stacks={} dur={} shardsLeft={}",
            GetId(), buff ? uint32(buff->GetStackAmount()) : 0u,
            buff ? buff->GetDuration() : 0, GetSoulShardCount(player));
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warl_wailing_soul::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_warl_wailing_soul_loader : public SpellScriptLoader
{
public:
    spell_warl_wailing_soul_loader() : SpellScriptLoader("spell_warl_wailing_soul") { }

    AuraScript* GetAuraScript() const override
    {
        return new spell_warl_wailing_soul();
    }
};

void AddSC_warl_wailing_soul()
{
    new spell_warl_wailing_soul_loader();
}
