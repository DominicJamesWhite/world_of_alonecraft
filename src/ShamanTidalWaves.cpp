#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Tidal Waves (redesigned) — Shaman Restoration talent
// Ranks: 51562 / 51563 / 51564 / 51565 / 51566
// Effect1: passive spellpower (handled by DBC aura)
// Effect2: SPELL_AURA_DUMMY storing heal crit % (5/10/15/20/25 per rank)
//
// Mechanic:
//   When Lightning Bolt (family 0x01), Chain Lightning (0x02), or
//   Lava Burst (family1 0x1000) is cast, apply heal crit buff 200220
//   with the value from the talent's Effect2 BasePoints.
//   200220 is SPELL_AURA_MOD_SPELL_CRIT_CHANCE, 15s duration, 1 charge.

enum TidalWavesSpells
{
    TIDAL_WAVES_R1     = 51562,
    TIDAL_WAVES_R2     = 51563,
    TIDAL_WAVES_R3     = 51564,
    TIDAL_WAVES_R4     = 51565,
    TIDAL_WAVES_R5     = 51566,
    TIDAL_WAVES_BUFF   = 200220,
};

// LB=0x01, CL=0x02
static constexpr uint32 LB_CL_MASK = 0x03;
// Lava Burst family flags[1] = 0x1000
static constexpr uint32 LAVA_BURST_MASK1 = 0x1000;

class sha_tidal_waves_handler : public PlayerScript
{
public:
    sha_tidal_waves_handler() : PlayerScript("sha_tidal_waves_handler") { }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (!spellInfo)
            return;

        if (spellInfo->SpellFamilyName != SPELLFAMILY_SHAMAN)
            return;

        // Check for LB/CL (flags[0]) or Lava Burst (flags[1])
        bool isLbCl = (spellInfo->SpellFamilyFlags[0] & LB_CL_MASK) != 0;
        bool isLavaBurst = (spellInfo->SpellFamilyFlags[1] & LAVA_BURST_MASK1) != 0;

        if (!isLbCl && !isLavaBurst)
            return;

        // Find which rank of Tidal Waves the player has (check highest first)
        int32 critBonus = 0;
        if (AuraEffect const* eff = player->GetAuraEffect(TIDAL_WAVES_R5, EFFECT_1))
            critBonus = eff->GetAmount();
        else if (AuraEffect const* eff = player->GetAuraEffect(TIDAL_WAVES_R4, EFFECT_1))
            critBonus = eff->GetAmount();
        else if (AuraEffect const* eff = player->GetAuraEffect(TIDAL_WAVES_R3, EFFECT_1))
            critBonus = eff->GetAmount();
        else if (AuraEffect const* eff = player->GetAuraEffect(TIDAL_WAVES_R2, EFFECT_1))
            critBonus = eff->GetAmount();
        else if (AuraEffect const* eff = player->GetAuraEffect(TIDAL_WAVES_R1, EFFECT_1))
            critBonus = eff->GetAmount();

        if (critBonus <= 0)
            return;

        player->CastCustomSpell(TIDAL_WAVES_BUFF, SPELLVALUE_BASE_POINT0, critBonus, player, true);
    }
};

void AddSC_sha_tidal_waves()
{
    new sha_tidal_waves_handler();
}
