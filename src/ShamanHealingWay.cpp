#include "ScriptMgr.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Healing Way (redesigned) -- Shaman Restoration talent
// Ranks: 29206 / 29205 / 29202
//
// DBC effects (passive, no script):
//   Effect1: ADD_FLAT_MODIFIER for crit chance on HW/LHW (3/6/9%)
//   Effect2: MOD_DAMAGE_PERCENT_DONE +10% all spell schools
//
// Script effect (this file): When Healing Wave or Lesser Healing Wave
// heals the caster (self-heal), immediately reduce Chain Lightning's
// cooldown by 3 sec.
//
// SpellScript registered on all ranks of HW (-331) and LHW (-8004).

enum HealingWaySpells
{
    HEALING_WAY_R1       = 29206,
    HEALING_WAY_R2       = 29205,
    HEALING_WAY_R3       = 29202,
    CHAIN_LIGHTNING_BASE = 421,
};

static constexpr int32 CL_CD_REDUCTION_MS = -3000; // 3 seconds

class spell_sha_healing_way_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_sha_healing_way_SpellScript);

    void HandleAfterHit()
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        // Only proc on self-heals
        if (caster->GetGUID() != target->GetGUID())
            return;

        // Must have at least one rank of Healing Way
        if (!caster->HasAura(HEALING_WAY_R1) &&
            !caster->HasAura(HEALING_WAY_R2) &&
            !caster->HasAura(HEALING_WAY_R3))
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        // Reduce Chain Lightning cooldown by 3 sec (all ranks)
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(CHAIN_LIGHTNING_BASE);
        while (spellInfo)
        {
            player->ModifySpellCooldown(spellInfo->Id, CL_CD_REDUCTION_MS);
            spellInfo = spellInfo->GetNextRankSpell();
        }
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_sha_healing_way_SpellScript::HandleAfterHit);
    }
};

class spell_sha_healing_way_loader : public SpellScriptLoader
{
public:
    spell_sha_healing_way_loader() : SpellScriptLoader("spell_sha_healing_way") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_sha_healing_way_SpellScript();
    }
};

void AddSC_sha_healing_way()
{
    new spell_sha_healing_way_loader();
}
