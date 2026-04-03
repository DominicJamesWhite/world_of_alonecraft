#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"
#include "Log.h"

// Tidal Waves (redesigned) — Shaman Restoration talent
// Ranks: 51562 / 51563 / 51564 / 51565 / 51566
// Effect1 (idx 0): passive spellpower (handled by DBC aura)
// Effect2 (idx 1): SPELL_AURA_DUMMY storing heal crit % (5/10/15/20/25)
//
// When Lightning Bolt, Chain Lightning, or Lava Burst is cast,
// apply heal crit buff 200220 with the value from the talent's Effect2.
//
// Implementation: SpellScript registered on LB/CL/Lava Burst via spell_script_names.
// AfterCast checks for talent aura, reads crit bonus, casts buff.

enum TidalWavesSpells
{
    TIDAL_WAVES_R1   = 51562,
    TIDAL_WAVES_R2   = 51563,
    TIDAL_WAVES_R3   = 51564,
    TIDAL_WAVES_R4   = 51565,
    TIDAL_WAVES_R5   = 51566,
    TIDAL_WAVES_BUFF = 200220,
};

static constexpr uint32 TIDAL_WAVES_RANKS[] = {
    TIDAL_WAVES_R5, TIDAL_WAVES_R4, TIDAL_WAVES_R3, TIDAL_WAVES_R2, TIDAL_WAVES_R1
};

class spell_sha_tidal_waves_trigger : public SpellScript
{
    PrepareSpellScript(spell_sha_tidal_waves_trigger);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ TIDAL_WAVES_BUFF });
    }

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
        {
            LOG_DEBUG("scripts", "TidalWaves::HandleAfterCast - no caster");
            return;
        }

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        // Find highest rank of Tidal Waves talent (check highest first)
        int32 critBonus = 0;
        for (uint32 rankId : TIDAL_WAVES_RANKS)
        {
            if (AuraEffect const* eff = player->GetAuraEffect(rankId, EFFECT_1))
            {
                critBonus = eff->GetAmount();
                break;
            }
        }

        if (critBonus <= 0)
            return;

        player->CastCustomSpell(TIDAL_WAVES_BUFF, SPELLVALUE_BASE_POINT0, critBonus, player, true);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_sha_tidal_waves_trigger::HandleAfterCast);
    }
};

class spell_sha_tidal_waves_trigger_loader : public SpellScriptLoader
{
public:
    spell_sha_tidal_waves_trigger_loader() : SpellScriptLoader("spell_sha_tidal_waves_trigger") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_sha_tidal_waves_trigger();
    }
};

void AddSC_sha_tidal_waves()
{
    new spell_sha_tidal_waves_trigger_loader();
}
