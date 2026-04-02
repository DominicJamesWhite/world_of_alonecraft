#include "DBCStores.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"

// Nature's Guardian (redesigned) — Shaman Restoration talent
// Ranks: 30881 / 30883 / 30884 / 30885 / 30886 (SPELL_AURA_DUMMY passives)
// Mechanic:
//   When any totem is summoned, if the caster has Nature's Guardian,
//   the totem casts a matching elemental guardian summon spell.
//   The guardian is owned by the totem and despawns when the totem dies.
//
// Registered via spell_script_names on all 20 totem spell families
// using negative spell IDs (all ranks).

enum NaturesGuardianSpells
{
    NATURES_GUARDIAN_R1         = 30881,
    NATURES_GUARDIAN_R2         = 30883,
    NATURES_GUARDIAN_R3         = 30884,
    NATURES_GUARDIAN_R4         = 30885,
    NATURES_GUARDIAN_R5         = 30886,
    SUMMON_FIRE_GUARDIAN        = 200230,
    SUMMON_EARTH_GUARDIAN       = 200231,
    SUMMON_WATER_GUARDIAN       = 200232,
    SUMMON_AIR_GUARDIAN         = 200233,
};

static bool HasNaturesGuardian(Unit const* caster)
{
    return caster->HasAura(NATURES_GUARDIAN_R1) ||
           caster->HasAura(NATURES_GUARDIAN_R2) ||
           caster->HasAura(NATURES_GUARDIAN_R3) ||
           caster->HasAura(NATURES_GUARDIAN_R4) ||
           caster->HasAura(NATURES_GUARDIAN_R5);
}

static uint32 GetGuardianSpellForSlot(uint32 slot)
{
    switch (slot)
    {
        case SUMMON_SLOT_TOTEM_FIRE:  return SUMMON_FIRE_GUARDIAN;
        case SUMMON_SLOT_TOTEM_EARTH: return SUMMON_EARTH_GUARDIAN;
        case SUMMON_SLOT_TOTEM_WATER: return SUMMON_WATER_GUARDIAN;
        case SUMMON_SLOT_TOTEM_AIR:   return SUMMON_AIR_GUARDIAN;
        default: return 0;
    }
}

// SpellScript registered on all totem summon spells
class spell_sha_natures_guardian_totem : public SpellScript
{
    PrepareSpellScript(spell_sha_natures_guardian_totem);

    void HandleAfterCast()
    {
        Unit* caster = GetCaster();
        if (!caster || !HasNaturesGuardian(caster))
            return;

        SpellInfo const* si = GetSpellInfo();

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (si->Effects[i].Effect != SPELL_EFFECT_SUMMON)
                continue;

            SummonPropertiesEntry const* props =
                sSummonPropertiesStore.LookupEntry(si->Effects[i].MiscValueB);
            if (!props)
                continue;

            // Verify this is a totem slot (1-4)
            if (props->Slot < SUMMON_SLOT_TOTEM_FIRE ||
                props->Slot > SUMMON_SLOT_TOTEM_AIR)
                continue;

            uint32 guardianSpell = GetGuardianSpellForSlot(props->Slot);
            if (!guardianSpell)
                continue;

            // Find the totem that was just summoned in this slot
            if (Creature* totem = ObjectAccessor::GetCreature(
                    *caster, caster->m_SummonSlot[props->Slot]))
                totem->CastSpell(totem, guardianSpell, true);

            break;
        }
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_sha_natures_guardian_totem::HandleAfterCast);
    }
};

class spell_sha_natures_guardian_totem_loader : public SpellScriptLoader
{
public:
    spell_sha_natures_guardian_totem_loader()
        : SpellScriptLoader("spell_sha_natures_guardian_totem") { }

    SpellScript* GetSpellScript() const override
    {
        return new spell_sha_natures_guardian_totem();
    }
};

void AddSC_sha_natures_guardian()
{
    new spell_sha_natures_guardian_totem_loader();
}
