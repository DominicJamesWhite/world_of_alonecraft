#include "ScriptMgr.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "Unit.h"
#include "GameTime.h"

enum UnbreakableWillSpells
{
    UNBREAKABLE_WILL_R1 = 14522,
    UNBREAKABLE_WILL_R2 = 14788,
    UNBREAKABLE_WILL_R3 = 14789,
    UNBREAKABLE_WILL_R4 = 14790,
    UNBREAKABLE_WILL_R5 = 14791,
    WEAKENED_SOUL       = 6788,
};

static constexpr Milliseconds UNBREAKABLE_WILL_ICD_MS = 1000ms;

class spell_unbreakable_will_handler : public UnitScript
{
public:
    spell_unbreakable_will_handler() : UnitScript("spell_unbreakable_will_handler") { }

    void OnDamage(Unit* /*attacker*/, Unit* victim, uint32& damage) override
    {
        if (damage == 0)
            return;

        Player* player = victim->ToPlayer();
        if (!player)
            return;

        // Determine rank and reduction amount
        uint32 reductionMs = 0;
        if (player->HasAura(UNBREAKABLE_WILL_R5))
            reductionMs = 5000;
        else if (player->HasAura(UNBREAKABLE_WILL_R4))
            reductionMs = 4000;
        else if (player->HasAura(UNBREAKABLE_WILL_R3))
            reductionMs = 3000;
        else if (player->HasAura(UNBREAKABLE_WILL_R2))
            reductionMs = 2000;
        else if (player->HasAura(UNBREAKABLE_WILL_R1))
            reductionMs = 1000;
        else
            return;

        // Check for Weakened Soul
        Aura* weakenedSoul = player->GetAura(WEAKENED_SOUL);
        if (!weakenedSoul)
            return;

        // Check ICD
        Milliseconds now = GameTime::GetGameTimeMS();
        ObjectGuid guid = player->GetGUID();
        auto it = _lastProcTime.find(guid);
        if (it != _lastProcTime.end() && (now - it->second) < UNBREAKABLE_WILL_ICD_MS)
            return;

        _lastProcTime[guid] = now;

        // Reduce Weakened Soul duration
        int32 newDuration = weakenedSoul->GetDuration() - static_cast<int32>(reductionMs);
        if (newDuration <= 0)
            weakenedSoul->Remove();
        else
            weakenedSoul->SetDuration(newDuration);
    }

private:
    std::unordered_map<ObjectGuid, Milliseconds> _lastProcTime;
};

void AddSC_unbreakable_will()
{
    new spell_unbreakable_will_handler();
}
