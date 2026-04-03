# Encounter Scaling: 1–5 Players

Alonecraft is designed for solo and small-group play (1–5 players). Encounters need mechanic adjustments to be completable at low player counts.

## Interaction with mod-autobalance

**mod-autobalance already handles all stat-based scaling.** It automatically adjusts creature HP, damage (melee + spell), healing, armor, mana, CC duration, and XP/gold rewards based on the number of players in the instance. It uses a tanh curve with configurable inflection points per instance type.

**What autobalance covers (we do NOT need to handle):**
- Creature HP scaling down for fewer players
- Creature melee and spell damage scaling
- Healing output scaling
- CC duration scaling
- Boss vs non-boss separate stat multipliers
- Level scaling to match player level
- XP and gold reward scaling

**What autobalance does NOT cover (this is what we handle):**
- Boss mechanics that are structurally impossible with fewer players (MC, vehicles, split-realm, heal-to-win)
- Add/target count scaling (number of Blood Beasts, Watery Grave targets, cube clicks, wave sizes)
- Encounter structure changes (sequential boss engagement, alternative win conditions)
- Instance player-count gates that reset/cleanup the instance
- Vehicle encounters (player-controlled vehicles aren't creature stats)
- Spell mechanic modifications (polarity shift damage formula, essence immunity, Stone Grip pacify)

**Hook interaction note:** Both autobalance and our module use `UnitScript` hooks. Autobalance scales creature damage output globally; our hooks then further modify specific spell IDs for mechanic-specific adjustments. Both fire independently and should stack correctly, but each encounter should be tested to confirm.

---

## Architecture

### Core: A Shared Player-Count Helper

```cpp
inline uint32 GetGroupSize(Creature* me) {
    return me->GetMap()->GetPlayersCountExceptGMs();
}
```

All scaling formulas below reference `N` = `GetGroupSize()`.

### Hook Strategy

| Hook | Used For |
|------|----------|
| `AllCreatureScript::GetCreatureAI()` | Replace boss AI for encounters needing structural rework |
| `UnitScript::OnAuraApply()` | Intercept MC/charm/possess auras, Stone Grip duration, mark stack caps |
| `AllSpellScript::OnSpellCheckCast()` | Block specific spells at low N (Shadow of Death, Fatal Attraction) |
| `UnitScript::ModifySpellDamageTaken()` | Mechanic-specific damage adjustments (polarity shift, split soak) that autobalance can't handle because the issue is the spell formula, not raw creature damage |

All C++ in `modules/world_of_alonecraft/src/`, all SQL in `modules/world_of_alonecraft/data/sql/db-world/`.

---

## Global Systems

### G1. Mind Control / Charm / Possess Scaling

**Rule:** MC should never target more than `N-1` players simultaneously. At `N=1`, all MC is cancelled. At `N=2`, max 1 MC target.

**Implementation:** `UnitScript::OnAuraApply()` — cancels charm/possess/MC auras when the MC'd count would exceed `N-1`.

**Encounters fixed:** Lady Deathwhisper, Kael'thas, Baroness Anastari, Felmyst, Blood Queen, Kil'jaeden, Jin'do, Shirrak

### G2. "Targets Non-Tank" Mechanics

**Rule:** Spells that skip the tank via `SelectTarget(RANDOM, 1)` should fall back to the tank at N=1.

**Implementation:** Per-encounter AI overrides where the target selection logic lives.

---

## Per-Encounter Changes

### 1. Valithria Dreamwalker (ICC) — Heal-to-Win

**Autobalance impact:** None — the problem is the win condition (must heal), not stats.

| Players | Change |
|---------|--------|
| 1 | Alternative win condition: kill all add waves to complete encounter. Valithria slowly self-heals. |
| 2 | Healing threshold reduced to 40%. One portal spawns. Add spawn rate -60%. |
| 3 | Healing threshold reduced to 60%. Two portals. Add spawn rate -40%. |
| 4 | Healing threshold reduced to 80%. Two portals. |
| 5 | Normal. |

**Implementation:** `GetCreatureAI()` override. At N=1, track add wave kills as win condition instead of Valithria HP.

### 2. Instructor Razuvious (Naxx) — MC Understudy Tanking

**Autobalance impact:** Autobalance scales Razuvious damage, but the core problem remains — you must MC an understudy to tank, leaving no one to DPS.

| Players | Change |
|---------|--------|
| 1 | Understudies auto-engage Razuvious as tanks (friendly AI). Player DPS freely. |
| 2 | One understudy auto-tanks, one available for player MC. |
| 3-5 | Normal MC mechanic. |

**Implementation:** `AllCreatureScript::GetCreatureAI()` override on understudy entry 16803. At N<=2, returns `AutoTankUnderstudyAI` which auto-attacks Razuvious on spawn via `IsSummonedBy()`. At N>=3, returns nullptr (normal MC mechanic).

### 2b. Gothik the Harvester (Naxx) — Side Split / Dead Adds

**Autobalance impact:** Add stats scale, but the living/dead side split is structurally impossible solo. Dead-side adds spawn on the empty side with no target and pile up until the gate opens.

| Players | Change |
|---------|--------|
| 1-2 | Gate opens when the first dead-side add spawns. Living adds fight the player normally for the first waves, then dead adds path through the open gate as they spawn. |
| 3-5 | Normal (split mechanic). |

**Implementation:** `AllCreatureScript::OnCreatureAddWorld` on dead-side add NPCs (16127, 16148-16150). When a dead add spawns at N<=2, opens the inner gate GO (181170) via `SetGoState(GO_STATE_ACTIVE)`. Living add waves proceed normally — the player fights a couple living waves, then the gate opens and dead adds trickle in alongside them.

### 3. Magtheridon — Cube Interrupt

**Autobalance impact:** None — the problem is needing 5 simultaneous cube clicks.

| Players | Change |
|---------|--------|
| 1 | 1 cube click interrupts Blast Nova. Cast time +3s. |
| 2 | 2 clicks needed. Cast time +2s. |
| 3 | 3 clicks. Cast time +1s. |
| 4 | 4 clicks. Normal cast time. |
| 5 | Normal (5 cubes). |

**Implementation:** `GetCreatureAI()` override. Track cube activations; interrupt threshold = N.

### 4. Icecrown Gunship Battle — Vehicle Encounter

**Autobalance impact:** None — vehicles aren't scaled creatures. The encounter structure is fundamentally multi-player.

| Players | Change |
|---------|--------|
| 1 | Auto-complete: encounter state set to DONE, loot chest spawns. |
| 2 | Defend own ship only. NPC allies handle enemy ship. No boarding. |
| 3 | Must board enemy ship. Enemy adds reduced 50%. |
| 4 | Enemy adds reduced 25%. |
| 5 | Normal. |

**Implementation:** `GetCreatureAI()` override on gunship controller NPC.

### 5. Karazhan Chess Event

**Autobalance impact:** None — chess pieces aren't standard creatures. The problem is needing multiple players to control multiple pieces.

| Players | Change |
|---------|--------|
| 1 | Friendly AI pieces buffed +300% damage, +200% HP. Auto-play. |
| 2 | Friendly AI +150% damage, +100% HP. |
| 3 | Friendly AI +75% damage, +50% HP. |
| 4 | Friendly AI +25% damage, +25% HP. |
| 5 | Normal. |

**Implementation:** `GetCreatureAI()` override on chess controller.

### 6. Flame Leviathan (Ulduar) — Vehicles

**Autobalance impact:** Autobalance scales the boss HP, but NOT player-controlled vehicle damage/HP. Solo player in one vehicle can't output enough damage.

| Players | Change |
|---------|--------|
| 1 | Vehicle: +400% damage, +300% HP. Turret auto-fires (no passenger needed). |
| 2 | Vehicle: +200% damage, +150% HP. Turret auto-fires. |
| 3 | Vehicle: +100% damage, +75% HP. |
| 4 | Vehicle: +50% damage. |
| 5 | Normal. |

**Implementation:** `GetCreatureAI()` override on vehicle entries. Apply scaling auras at spawn. Override turret AI to auto-fire at N < 3. **Needs testing** — verify autobalance boss HP reduction is sufficient or if additional boss HP reduction is needed.

### 7. Halion (Ruby Sanctum) �� Dual Realm

**Autobalance impact:** Stats scale, but the dual-realm mechanic is structurally impossible — one player can't be in two realms.

| Players | Change |
|---------|--------|
| 1-2 | Skip Phase 3 split. Boss stays physical realm only. Corporeality locked at 50%. |
| 3 | Phase 3 activates but corporeality rebalances +20% faster. |
| 4 | Corporeality +10% faster. |
| 5 | Normal. |

**Implementation:** `GetCreatureAI()` override. Prevent phase transition at N<=2.

### 8. Four Horsemen (Naxx) — 4 Corners

**Autobalance impact:** Individual horseman damage/HP scales, but the mark stacking mechanic is the killer — exponential damage from being in range of multiple horsemen.

| Players | Change |
|---------|--------|
| 1 | Sequential: one horseman active at a time. Mark cap at 3 stacks. |
| 2 | Two active (front pair, then back). Mark cap at 4 stacks. |
| 3 | Three active. Mark cap at 5 stacks. |
| 4-5 | All four (normal). Mark cap at 6 (normal). |

**Implementation:** Mark stack cap implemented via `UnitScript::OnAuraApply` — caps mark aura stacks (28832-28835) using `SetStackAmount()`. Sequential engagement (staggered activation) is a future TODO requiring `GetCreatureAI()` override.

### 9. Mind Control Encounters — See Global System G1

Handled globally. No per-encounter work needed.

### 10. Yogg-Saron (Ulduar) — Brain Phase

**Autobalance impact:** Add stats scale, but Brain Link needs two players and the brain phase requires raid-splitting.

| Players | Change |
|---------|--------|
| 1 | Brain Link disabled. Outside add spawn rate halved. Sanity loss -50%. |
| 2 | Brain Link disabled. Add spawn -30%. Sanity loss -30%. |
| 3 | Brain Link targets 1 pair. Add spawn -15%. |
| 4-5 | Normal. |

**Implementation:** `GetCreatureAI()` override. Cancel Brain Link via `OnSpellCheckCast` at N < 3. Scale add spawn timer.

### 11. Twin Val'kyr (ToC) — Essences

**Autobalance impact:** Stats scale, but the essence immunity mechanic prevents damage regardless of stats.

| Players | Change |
|---------|--------|
| 1-2 | Essence immunity removed. Both twins share a health pool. |
| 3 | Essence immunity reduced to 50% resistance. |
| 4 | Essence immunity reduced to 75% resistance. |
| 5 | Normal (full immunity). |

**Implementation:** `GetCreatureAI()` override. Remove/reduce immunity auras based on N.

### 12. Kologarn (Ulduar) — Stone Grip

**Autobalance impact:** Arm HP scales, but the player is pacified and can't attack — the mechanic itself is the problem.

| Players | Change |
|---------|--------|
| 1 | Grip lasts 3s max. Pacify removed (player can auto-attack arm). |
| 2 | Grip lasts 5s. Targets 1 player. |
| 3 | Grip lasts 6s. Targets 1 player. |
| 4-5 | Normal. |

**Implementation:** `OnAuraApply` hook for Stone Grip spell. Scale duration, remove pacify at N=1.

### 13. Thaddius (Naxx) — Polarity Shift

**Autobalance impact:** None — the damage comes from the polarity mechanic formula, not creature stats. A solo player with one charge still takes the "wrong polarity" hit from the mechanic itself.

| Players | Change |
|---------|--------|
| 1 | Polarity deals no damage. Charge buff still boosts player damage. |
| 2 | Polarity damage -75%. |
| 3 | Polarity damage -50%. |
| 4 | Polarity damage -25%. |
| 5 | Normal. |

**Implementation:** `ModifySpellDamageTaken` for polarity spell IDs. Formula: `damage * (N-1) / 4`.

### 13b. Gluth (Naxx) — Zombie Chow / Decimate

**Autobalance impact:** Zombie chow and Gluth stats scale, but the core problem is structural — zombie chow attack Gluth, and when he kills them he heals 5% max HP. A solo player can never out-DPS this healing. Additionally, Decimate reduces all players to 5% HP every 90–110s, which is unsurvivable without raid healers.

| Players | Change |
|---------|--------|
| 1 | Zombie chow disabled (no spawns). Decimate damage halved (player left at ~52% HP). |
| 2 | 50% of zombie chow despawned on spawn. Decimate damage 75% (player left at ~28% HP). |
| 3 | 30% of zombie chow despawned. Decimate normal. |
| 4-5 | Normal. |

**Implementation:** `AllCreatureScript::OnCreatureAddWorld` despawns zombie chow (entry 16360) based on N. `ModifySpellDamageTaken` on Decimate Damage (28375) reduces damage at N<=2.

### 14. Faction Champions (ToC) — Champion Count

**Autobalance impact:** Champion stats scale down, but 6-10 champions vs 1 player is still overwhelming regardless of individual stats.

| Players | Change |
|---------|--------|
| 1 | 2 champions spawn. No healers in pool. |
| 2 | 3 champions. Max 1 healer. |
| 3 | 4 champions. Max 1 healer. |
| 4 | 5 champions. |
| 5 | 6 champions (normal). |

**Implementation:** `GetCreatureAI()` override on encounter controller. Modify champion selection/spawn count.

### 15. Halls of Reflection — Wave Reset

**Autobalance impact:** Add stats scale, but the wipe-on-death reset mechanic is the problem.

| Players | Change |
|---------|--------|
| 1 | No wave reset on death. Adds per wave reduced to 2. LK escort 30% slower. |
| 2 | Reset only on full wipe. Adds per wave: 3. LK 20% slower. |
| 3 | Adds per wave: 4. LK 10% slower. |
| 4 | Normal adds. LK 5% slower. |
| 5 | Normal. |

**Implementation:** Override instance AI. Only `HandleWaveWipe()` when all N players dead. Scale add count.

### 16. Eredar Twins (Sunwell) — Paired Kill

**Autobalance impact:** Stats scale, but the empowerment mechanic (+100% when sister dies) is the structural problem.

| Players | Change |
|---------|--------|
| 1-2 | Shared health pool. Empowerment removed. |
| 3 | Empowerment reduced to +25%. |
| 4 | Empowerment +50%. |
| 5 | Normal (+100%). |

**Implementation:** `GetCreatureAI()` override. Sync health at N<=2. Scale empowerment buff.

### 17. Teron Gorefiend (BT) — Shadow of Death

**Autobalance impact:** None — the spell kills the targeted player outright. Stats don't help.

| Players | Change |
|---------|--------|
| 1 | Shadow of Death disabled entirely. |
| 2 | Cooldown doubled. Spirit adds reduced to 2. |
| 3 | Cooldown +50%. Spirit adds: 3. |
| 4-5 | Normal. |

**Implementation:** `OnSpellCheckCast` to block at N=1. Scale cooldown/adds in AI override.

### 18. Leotheras the Blind (SSC) — Inner Demons

**Autobalance impact:** Demon stats scale, but the spawn count is hardcoded to target up to 5 non-tank players.

| Players | Change |
|---------|--------|
| 1 | 1 inner demon. |
| 2 | 2 inner demons. |
| 3 | 3 inner demons. |
| 4-5 | Normal (up to 5). |

**Implementation:** Override demon phase. Cap `innerDemonCount = min(N, 5)`.

### 19. Instance Reset on Solo Player

**Autobalance impact:** None — this is a hardcoded player-count check.

| Players | Change |
|---------|--------|
| 1-5 | Remove cleanup check. Instance persists regardless of count. |

**Instances:** Black Morass, Old Hillsbrad.
**Implementation:** Override instance AI to remove `GetPlayersCountExceptGMs() <= 1` branch.

### ~~20. Patchwerk — Hateful Strike~~

**REMOVED — Autobalance handles this.** Hateful Strike damage scales with creature damage output, which autobalance already reduces for fewer players. If testing shows it's still too high, we can add a specific spell override, but it likely works out of the box.

### 21. Malygos Phase 3 / Oculus Drakes

**Autobalance impact:** Boss stats scale, but player-controlled drake vehicle abilities are NOT creature stats — autobalance doesn't buff them.

| Players | Change |
|---------|--------|
| 1 | Drake abilities +400% damage, +300% HP. |
| 2 | Drake abilities +200% damage, +150% HP. |
| 3 | Drake abilities +100% damage, +75% HP. |
| 4 | Drake abilities +50% damage. |
| 5 | Normal. |

**Implementation:** Apply scaling auras to drake vehicles on spawn based on N. **Needs testing** — verify autobalance boss HP reduction is enough.

### 22. Razorscale (Ulduar) — Harpoon Count

**Autobalance impact:** None — this is a hardcoded interaction count.

| Players | Change |
|---------|--------|
| 1-2 | 1 harpoon needed. |
| 3-4 | 2 harpoons. |
| 5 | 2 harpoons (normal). |

**Implementation:** `GetCreatureAI()` override. `reqChainCount = max(1, N / 2)`.

### 23. Morogrim Tidewalker (SSC) — Watery Graves

**Autobalance impact:** None — target count is hardcoded to 4 non-tank players.

| Players | Change |
|---------|--------|
| 1 | Watery Grave disabled. Murloc spawn: 4 (from 12). |
| 2 | 1 grave target. Murlocs: 6. |
| 3 | 2 grave targets. Murlocs: 8. |
| 4-5 | Normal. |

**Implementation:** Override target selection. `graveTargets = max(0, N - 1)`. Scale murloc count.

### 24. Mother Shahraz (BT) — Fatal Attraction

**Autobalance impact:** None — the mechanic teleports non-tank players together. At N=1 there's no valid target, but the spell may still fire and behave unpredictably.

| Players | Change |
|---------|--------|
| 1 | Fatal Attraction disabled. |
| 2 | Targets 1 player. |
| 3 | Targets 2 players. |
| 4-5 | Normal (3 targets). |

**Implementation:** `OnSpellCheckCast` to cancel at N=1. Scale target count.

### 25. High King Maulgar (Gruul's) — 4 Adds

**Autobalance impact:** Add stats scale, but being attacked by all 4 simultaneously is the problem.

| Players | Change |
|---------|--------|
| 1-2 | Adds engage sequentially (one at a time). |
| 3 | Adds engage in pairs. |
| 4-5 | All engage (normal). |

**Implementation:** `GetCreatureAI()` override. Passive state on inactive adds, activate next on death.

### ~~26. Assembly of Iron — Supercharge~~

**REMOVED — Likely handled by autobalance.** Supercharge is a percentage buff on creature damage, which autobalance already scales down. The encounter should be manageable with autobalance stat reduction alone. Re-add if testing shows otherwise.

### ~~27. Blood Prince Council — Kinetic Bombs~~

**REMOVED — Likely handled by autobalance.** Kinetic Bomb damage and Shock Vortex scale with creature damage. The encounter is stat-dependent rather than mechanically impossible. Re-add if testing shows otherwise.

### 28. Illidari Council (BT) — Shared Healing

**Autobalance impact:** Stats scale, but the shared healing mechanic means damage to one member heals the others — making solo kills take forever or become impossible.

| Players | Change |
|---------|--------|
| 1 | Shared healing disabled. Sequential engagement. |
| 2 | Shared healing 25% effective. Engage in pairs. |
| 3 | Shared healing 50%. All engage. |
| 4-5 | Normal. |

**Implementation:** `GetCreatureAI()` override. Intercept `SPELL_SHARED_RULE_HEAL`.

### 29. Deathbringer Saurfang (ICC) — Blood Beast Count

**Autobalance impact:** Beast stats scale, but the number of beasts is hardcoded and Blood Power generation is a boss mechanic, not a stat.

| Players | Change |
|---------|--------|
| 1 | 1 Blood Beast. Blood Power generation -50%. |
| 2 | 1 Blood Beast. Blood Power -30%. |
| 3 | 2 Blood Beasts. Blood Power -15%. |
| 4-5 | Normal. |

**Implementation:** `GetCreatureAI()` override. Scale beast summon count and Blood Power gain.

### 30. The Lich King (ICC) — Val'kyr / Defile

**Autobalance impact:** Stats scale, but Val'kyr grab = instant death at N=1 (nobody to kill it), and Defile growth is a mechanic formula, not creature damage.

| Players | Change |
|---------|--------|
| 1 | Val'kyr disabled. Defile growth -70%. |
| 2 | 1 Val'kyr. Defile growth -50%. |
| 3 | 1 Val'kyr. Defile growth -30%. |
| 4 | 2 Val'kyrs. Defile growth -15%. |
| 5 | Normal. |

**Implementation:** `GetCreatureAI()` override. Cancel Val'kyr summon at N=1. Scale Defile growth.

### 31. Stratholme Slaughter — Add Count

**Autobalance impact:** Individual add stats scale, but 33 adds spawning simultaneously is the problem.

| Players | Change |
|---------|--------|
| 1 | 8 adds. Spawn interval 500ms. |
| 2 | 12 adds. Interval 400ms. |
| 3 | 18 adds. Interval 300ms. |
| 4 | 25 adds. Interval 250ms. |
| 5 | 33 adds (normal, 210ms). |

**Implementation:** Override instance `ProcessSlaughterEvent()`.

### ~~32. Zul'Farrak Pyramid — Timed Waves~~

**REMOVED — Autobalance handles this.** Add stats scale down. The wave timer is generous enough, and with scaled-down add HP the solo player should be able to clear waves. Re-add if testing shows otherwise.

### 33. C'Thun (AQ40) — Stomach Phase / Eye Beam

**Autobalance impact:** Tentacle stats scale, but the stomach phase structure and Eye Beam chain mechanic are hardcoded.

| Players | Change |
|---------|--------|
| 1 | Stomach phase halved. Eye Beam: no chain (single target). |
| 2 | Stomach -25%. Chain limited to 2. |
| 3 | Chain limited to 3. |
| 4-5 | Normal. |

**Implementation:** `GetCreatureAI()` override. Scale stomach timer and beam chain count.

### 34. Culling of Stratholme — Solo Positioning

**Autobalance impact:** None — this is a hardcoded player-count branch.

| Players | Change |
|---------|--------|
| 1-5 | Remove special solo repositioning logic. Standard Arthas pathing. |

**Implementation:** Override instance AI.

---

## Implementation Architecture

### File Structure

```
modules/world_of_alonecraft/src/
├── ScaledEncounters.h              # GetGroupSize() helper, scaling formula macros
├── ScaledEncounters_Global.cpp     # G1 (MC guard) — IMPLEMENTED
├── ScaledEncounters_ICC.cpp        # Valithria, Gunship, Saurfang, Lich King
├── ScaledEncounters_Ulduar.cpp     # Flame Leviathan, Kologarn, Razorscale, Yogg-Saron
├── ScaledEncounters_Naxx.cpp       # Gluth, Thaddius, Razuvious, Gothik, Four Horsemen — IMPLEMENTED
├── ScaledEncounters_ToC.cpp        # Twin Val'kyr, Faction Champions
├── ScaledEncounters_Outland.cpp    # Magtheridon, Illidari Council, Shahraz,
│                                   # Teron, Leotheras, Morogrim, Maulgar, Eredar Twins
├── ScaledEncounters_Dungeons.cpp   # HoR, Black Morass, Old Hillsbrad, Stratholme, Chess
├── ScaledEncounters_Classic.cpp    # C'Thun, Halion, Malygos/Oculus
└── MP_loader.cpp                   # Register all AddSC_ functions
```

### Implementation Priority

**Phase 1 — Global hooks (done):**
- `ScaledEncounters_Global.cpp` — MC/charm guard. Fixes ~8 encounters.

**Phase 2 — Highest-traffic content (ICC + Ulduar):**
- `ScaledEncounters_ICC.cpp` — Valithria, Gunship, Saurfang, Lich King (4 encounters)
- `ScaledEncounters_Ulduar.cpp` — Flame Leviathan, Kologarn, Razorscale, Yogg-Saron (4 encounters)

**Phase 3 — Remaining raids:**
- `ScaledEncounters_Naxx.cpp` — Razuvious, Four Horsemen, Thaddius (3 encounters)
- `ScaledEncounters_ToC.cpp` — Twin Val'kyr, Faction Champions (2 encounters)
- `ScaledEncounters_Outland.cpp` — Magtheridon, Maulgar, Leotheras, Morogrim, Teron, Shahraz, Illidari Council, Eredar Twins (8 encounters)

**Phase 4 — Dungeons + Classic:**
- `ScaledEncounters_Dungeons.cpp` — HoR, Black Morass, Old Hillsbrad, Stratholme, Chess, Culling of Strat (6 encounters)
- `ScaledEncounters_Classic.cpp` — C'Thun, Halion, Malygos/Oculus (3 encounters)

---

## Encounters Removed (Handled by Autobalance)

These encounters were in the original audit but are expected to work with autobalance stat scaling alone. They should be tested and re-added here only if autobalance proves insufficient:

| Encounter | Why Removed |
|-----------|-------------|
| Patchwerk — Hateful Strike | Damage scales with creature damage, which autobalance reduces |
| Assembly of Iron — Supercharge | Percentage damage buff on scaled-down creature damage |
| Blood Prince Council — Kinetic Bombs | Stat-dependent, not mechanically impossible |
| Zul'Farrak Pyramid — Timed Waves | Add HP scales down, timer is generous |

---

## Verification

- Test each encounter at N=1, N=3, and N=5
- Verify encounter completes and drops loot at all group sizes
- Verify no hook conflicts between autobalance and our mechanic overrides
- Verify mechanics "feel right" — scaling should make the encounter fair, not trivial
- Run `python tools/verify_scripts.py` after adding new scripts
