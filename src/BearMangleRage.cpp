#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Unit.h"

// Cataclysm rage normalization (Rage.Normalized / Rage.FromDamageTaken in
// worldserver.conf) takes damage-taken rage away from bears, which was the
// bulk of their income. Mangle (Bear) becomes the replacement generator, the
// same role Shield Slam and Revenge play for protection warriors.
//
// Those two are pure DBC changes -- they had a free effect slot for an
// ENERGIZE. Mangle (Bear) uses all three (WEAPON_DAMAGE, the bleed-damage
// aura, WEAPON_PERCENT_DAMAGE), so there is nowhere to put one, and this
// script exists only because of that. The rage cost itself is still removed
// in the DBC (woa_2026_08_08_03.sql); this only adds the gain.
enum BearMangleRage
{
    MANGLE_BEAR_RAGE_GAIN = 150   // rage is stored x10, so 15 rage
};

class spell_dru_mangle_bear_rage : public SpellScript
{
    PrepareSpellScript(spell_dru_mangle_bear_rage);

    void HandleAfterHit()
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->IsPlayer())
            return;

        // Bear and Dire Bear are the only forms that cast this, but check
        // the active power rather than the form so a future form that uses
        // rage is handled without touching this script.
        if (!caster->HasActivePowerType(POWER_RAGE))
            return;

        caster->ModifyPower(POWER_RAGE, MANGLE_BEAR_RAGE_GAIN);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_dru_mangle_bear_rage::HandleAfterHit);
    }
};

void AddSC_bear_mangle_rage()
{
    RegisterSpellScript(spell_dru_mangle_bear_rage);
}
