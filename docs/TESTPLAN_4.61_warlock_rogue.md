# Alonecraft 4.61 test plan — Destruction / Demonology Warlock, Subtlety Rogue

Every case below is something you run in game. The server writes a structured
trace to its own file, which is what gets verified afterwards — you never have
to read the combat log.

---

## 1. Setup (once)

```bat
REM The new files are picked up by CMake's source glob, so the FIRST build
REM must re-run CMake -- do not pass --skip-cmake here.
build_and_run.bat --skip-dbc --skip-ui --skip-copy
```

Nothing else changes: the harness is off until you turn it on, and every
`ACTEST` site costs one atomic bool read when it is off.

Trace file:

```
<LogsDir>/alonecraft_test.log
```

`.woatest` with no arguments prints the resolved path.

### Before you spend a session in game

```bash
python tools/verify_db.py --spell-ids <the ids the case touches>
```

This reads `alonecraft_spell_dbc` and flags the two attribute bits that make
an aura invisible without any other symptom:

- `SPELL_ATTR0_PASSIVE` (0x40) — `Aura::CanBeSentToClient()` returns false, so
  the aura is never sent to the client at all. It also blocks `Aura::CanBeSaved`,
  so a passive aura does not survive logout.
- `SPELL_ATTR0_DO_NOT_DISPLAY` (0x80) — the aura reaches the client, which then
  hides it in the buff bar.

Both are correct on a passive talent and wrong on anything the player is meant
to see. Two of the three Destruction bugs in 4.61 were exactly this: the buffs
were applied and scaling correctly the whole time, and only the client display
was suppressed. Checking first is much cheaper than testing a working feature
and reporting it broken.

---

## 2. The commands

| Command | What it does |
|---------|--------------|
| `.woatest begin <case-id>` | Turns tracing on, writes a `BEGIN` marker, and takes a full "before" snapshot |
| `.woatest end` | Takes an "after" snapshot, writes `END`, turns tracing off |
| `.woatest note <text>` | Drops a landmark inside a case ("now pulling mob 2") |
| `.woatest state [label]` | Snapshot on demand, without changing the on/off state |
| `.woatest on` / `.woatest off` | Manual toggle |
| `.woatest` | Status + file path + command list |

Every traced line carries the active case id, so cases never get confused with
each other even if you run them back to back.

A snapshot dumps: level/spec, HP/mana/shards/combo points, all five stats, AP,
armor, attack-speed modifier, dodge/parry/block/melee crit/ranged crit, all
seven spell-crit schools, agility's raw contribution to dodge, **every aura
with its per-effect amounts, stacks, charges and duration**, and the same for
the pet. That aura dump is what makes the pure-DBC talent changes checkable —
they have no C++ to instrument, so they are verified as "this aura exists with
this amount and this duration" between two snapshots.

---

## 3. How to run a case

1. `.woatest begin WD-09`
2. Do the steps.
3. `.woatest end`

Run them in any order, skip any you don't care about. When you're done, tell me
which case ids you ran and I'll read the trace and report pass/fail per case.

**Two habits that make the trace far easier to read:**

- Use `.woatest note` between sub-steps within a case.
- For anything with a random roll (shard procs, Enveloping Shadows, Nemesis),
  repeat the action ~20 times. The trace records *every*
  roll including the failures, so I can check the observed rate against the
  configured chance rather than just "did it fire once".

---

## 4. Warlock — Destruction

### WD-01 — Soul Shards stack to 32
**Setup:** empty your shard stack first.
**Steps:** drain-soul kill mobs until you are past 5 shards, then keep going past 32.
**Expect in game:** one bag slot holds the lot; shard 33 is refused.
**Trace:** `WARL.SHARD gain ... before=N after=N+1`; at the cap, `gain BLOCKED ... cap=32`.
> Note: `item_template.stackable` is cached client-side. If the stack still
> shows as 1, clear the client Cache folder — that is a client issue, not a
> server one, and the trace will show the server-side count correctly.

### WD-02 — Soulstone is a direct cast
**Steps:** cast Create Soulstone on yourself. Then try again immediately.
**Expect:** no item is created, the buff lands directly, one shard is spent, 15 min cooldown.
**Trace:** `WARL.SHARD spend` for the reagent; the `before`/`after` snapshots
show a Soulstone Resurrection aura (20707 / 20762-5 / 27239 / 47883) that was
absent before.

### WD-03 — Shadowburn generates a shard instead of costing one
**Steps:** cast Shadowburn on a full-health mob (no kill). Then Shadowburn a mob to death.
**Expect:** no shard cost, +1 shard on cast, +1 more on an XP-worthy kill; ~30s cooldown.
**Verify:** shard count in the before/after snapshots, and `.woatest note` between the two casts.
> Pure DBC (`SPELL_EFFECT_CREATE_ITEM`), so there is no `WARL.SHARD` line for
> the gain — the snapshot delta is the evidence.

### WD-04 — Destructive Reach: fire damage, no threat reduction
**Steps:** snapshot, then check the talent aura.
**Verify:** in `STATE.AURA` for 17917/17918, effect 2 must read `aura79` (MOD_DAMAGE_PERCENT_DONE) with amount 5 or 10 — **not** `aura108`.

### WD-05 — Soul Leech
**Steps:** at reduced health, cast ~20 Shadow Bolts / Incinerates into a target dummy.
**Expect:** occasional large self-heal (200% of the hit), at a 5/10/15% chance by rank.
**Verify:** talent aura 30293/30295/30296 effect 1 amount = 200 in the snapshot
(BasePoints 199 + DieSides 1); heals visible as HP jumps between snapshots.
> Briefly retuned to 50% / 10-20-30% in `woa_2026_08_03_17.sql` and reverted in
> `woa_2026_08_03_23.sql`. The 200% / 5-10-15% values are the intended ones.

### WD-06 — Burning Soul (was Intensity)
**Steps:** cast ~30 Fire spells (Incinerate/Searing Pain) at a dummy.
**Trace:** `WARL.SHARDGAIN talent=18135|18136 mode=fixed` followed by `WARL.SHARD gain reason=shardgain`.
**Check:** hit rate should track spell_proc's 15/30% of fire *crits*.

### WD-07 — Aftermath
**Steps:** Immolate a dummy and let it tick for a full duration, twice.
**Trace:** `WARL.SHARDGAIN talent=18119|18120 mode=critchance roll=PASS|FAIL fireCrit=X`.
**Check:** the PASS rate across all rolls should be ≈ the `fireCrit` value shown, and it must match `STATE.SPELLCRIT s2=` from the snapshot. Also confirm Immolate's DoT is +10/20%.

### WD-08 — Molten Rain (was Demonic Power)
**Steps:** Rain of Fire onto a pack of 3+ mobs, full channel.
**Trace:** several `WARL.SHARDGAIN talent=18126|18127` lines within the same channel — one per target per tick that rolls through.

### WD-09 — Ruin: crits spread Immolate
**Steps:**
1. Pull 4+ mobs standing within ~15 yards of each other.
2. Immolate exactly one of them.
3. Cast Incinerate/Chaos Bolt at that one until you crit.
4. `.woatest note cc test` — now sap/repeat with one mob under breakable CC and crit again.
**Expect:** Immolate appears on nearby mobs, never on the CC'd one, max 4 per crit.
**Trace:** `WARL.RUIN crit source=... victim=... spread=N cap=4 skippedCc=N skippedHasImmolate=N`, plus one `spread immolate=<rank> to=<name>` per target and `skip <name> reason=breakable-cc`.
**Also check (the "no longer affects Imp" half):** Imp Firebolt crits should no longer get Ruin's crit-damage bonus — talent effect 1 class mask A1 must be 997, not 5093.

### WD-10 — Backlash cheats death
**Steps:**
1. `.woatest note armed` — take *any* non-lethal hit first. This is the
   diagnostic step; do it before anything else.
2. With a Soulstone on yourself, take lethal damage (a high-level mob, or `.damage` yourself).
3. `.woatest note no soulstone` — die again with no Soulstone.
4. `.woatest note icd` — re-soulstone within 60s and take lethal damage again.
**Expect:** (2) survive at 20% health, Soulstone consumed, a 60s **Backlash**
debuff (200506) appears; (3) die; (4) die (ICD).
**Trace:**
- `WARL.BACKLASH absorb pool armed rank=<34935|34938|34939> owner=<name> amount=-1`
  — written once when the talent aura is applied (login, or learning the talent).
- one `absorb rank=... damage=N health=N lethal=true|false soulstone=<id> icd=true|false`
  **per hit taken** while tracing is on.
- then one of `CHEAT DEATH soulstone=<id> consumed absorbed=N heal=N (20% of maxHp=N)`,
  `no Soulstone aura -- death allowed`, `ICD active -- death allowed`.

**Diagnosing a silent failure.** These two lines split the failure modes that
used to be indistinguishable:
- No `absorb pool armed` line at all → the SCHOOL_ABSORB effect is not on the
  player. Check `.woatest state` for aura 34935/34938/34939, and confirm the
  rank you actually have has the script (`spell_script_names` uses `-34935`,
  which relies on the Talent.dbc rank chain).
- `armed` but no `absorb` lines on hits → the aura exists but
  `Unit::CalcAbsorbResist` is not reaching it.
- `absorb` lines with `lethal=false` on the killing blow → something absorbed
  ahead of us, or the killing damage never went through the absorb path.
- `lethal=true soulstone=0` → the Soulstone was not detected. The script now
  matches the same `SpellVisual[0]==99 && SpellIconID==92` pair the release
  window uses, so this and the "Use Soulstone" button cannot disagree.

### WD-11 — Infernal Bargain (exponential channel)
Reworked in `woa_2026_08_04_00.sql`: 5 ticks, the price **doubles every tick**
(1 / 2 / 4 / 8 / 16 shards, 31 total), buff stacks track the same curve, and the
old -25% damage reduction is now **total damage immunity**
(`SPELL_AURA_SCHOOL_IMMUNITY`, MiscValue 127). Ignore any older expectation of
4 ticks, 1 shard each, or a percentage reduction.

| Tick | Cost | Cumulative | Stacks | Damage | Spell crit |
|---|---|---|---|---|---|
| 1 | 1 | 1 | 1 | +3% | +1% |
| 2 | 2 | 3 | 2 | +6% | +2% |
| 3 | 4 | 7 | 4 | +12% | +4% |
| 4 | 8 | 15 | 8 | +24% | +8% |
| 5 | 16 | 31 | 16 | +48% | +16% |

**Steps:**
1. With 31+ shards, cast Infernal Bargain and let it channel fully.
2. `.woatest note dry` — drop to exactly 7 shards and cast it again.
3. `.woatest note dry2` — drop to exactly 4 shards and cast it again.
4. `.woatest note zero` — with 0 shards, try to cast it.
5. `.woatest note immune` — during a channel, take melee, a direct nuke, a DoT
   tick, and stand in fire.

**Expect:** (1) 5 ticks, a **visible** buff going 1→2→4→8→16 stacks, tooltip
reading +48% damage / +16% crit at the end, buff at a full 10s afterwards;
(2) channel ends cleanly after tick 3 with 4 stacks and **exactly 0 shards
left**; (3) ends after tick 2 with 2 stacks and **1 shard left**; (4) cast
refused; (5) everything reads immune/0, existing DoTs are still on you after the
channel (paused, not stripped) and none of your own buffs were removed.

**Trace:**
- `WARL.BARGAIN cast spell=63349 shards=N maxTicks=N maxStacks=N result=OK|FAILED_REAGENTS`
- one `tick=N spell=63349 paid=1|2|4|8|16 buff=200405 stacks=... dmgDone=... spellCrit=... shardsLeft=N` per tick
- `tick=N ... cost=N have=N CANNOT PAY -- ending channel early` on the dry runs
- `channel over buff=200405 stacks=N durBefore=N refreshedTo=10000`

**Check — the one that matters:** on the dry runs, `shardsLeft` must land on
*exactly* 0 (step 2) and 1 (step 3). `ConsumeSoulShards` clamps a request to
what you carry and still reports success (`WarlockShards.h:86`), so a regression
that drops the pre-check would silently eat 3 of a requested 8 and hand out a
free tick. Also confirm the `paid=` values double and `stacks=` matches `paid=`.

**Also check:**
- The 200405 buff landing at all is the live proof that
  `IgnoresSchoolImmunityFromFriendlyCaster` (`Unit.cpp:9856`) is exempting the
  self-cast from our own immunity. If the trace shows ticks but `stacks=0`,
  someone added `SPELL_ATTR1_IMMUNITY_TO_HOSTILE_AND_FRIENDLY_EFFECTS` (0x10000)
  to 63349.
- Solo, the mob must stay on you and **not evade** during the 5s — school
  immunity suppresses threat rather than dropping it, and it clears on your next
  cast. In a group the mob will swap off you for the channel; that is accepted.
- Being hit must not interrupt or push back the channel (`ChannelInterruptFlags`
  3084 has neither `TAKE_DAMAGE` 0x2 nor `CHANNEL_FLAG_DELAY`). Moving must end
  it, and a kick must land — 0x08 is set on purpose.
- With a full 32-shard hoard, **Nathrezim Foresight (200507) is the real cost**:
  it should track down to 1 stack after a full channel and rebuild as shards
  come back. Channeling to 0 shards must remove 200507 entirely, not leave it
  sitting at 0 stacks.

**Note:** the buff was invisible before `woa_2026_08_03_13.sql` —
`SPELL_ATTR0_DO_NOT_DISPLAY` (0x80) on 200405 meant the client hid a buff that
was in fact applied and scaling correctly. If the trace shows stacks but the
buff bar shows nothing, that flag is back.

### WD-12 — Wailing Souls
**Steps:** with shards in hand, cast Searing Pain ~10 times. Then empty your shards and cast it again.
**Expect:** stacking damage-reduction buff, max 3 stacks, 10s; no shard means no buff.
**Trace:** `WARL.WAILING talent=... Wailing Soul applied stacks=1..3 dur=10000 shardsLeft=N`
and, when out, `proc but no shard -- no Wailing Soul`.
**Check:** rank chance is now the DBC's own 34/67/100% — the trace's PASS rate should match your rank.

### WD-13 — Sacrifice the Weak
Two auras: **Nathrezim Pact** (200403) is a hidden, permanent driver that
means "you sacrificed your demon"; **Nathrezim Foresight** (200507) is the
visible buff whose *stack count is your Soul Shard count*.

**Steps:**
1. Summon a demon, note your shard count (say 17), cast Sacrifice the Weak.
2. `.woatest note shard scaling` — spend a shard (Soul Fire) and gain one
   (Drain Soul); watch the stack count follow within ~1s.
3. `.woatest note zero` — spend down to 0 shards, then gain one back.
4. `.woatest note death` — die and release.
5. `.woatest note relog` — log out and back in.
6. `.woatest note resummon` — summon a new demon.
**Expect:**
- demon gone, **shard count unchanged** (no reagent refund)
- a visible **Nathrezim Foresight** buff showing exactly as many stacks as you
  have shards — 17 shards is 17 stacks, giving -17% damage taken and +17%
  critical strike **damage**. Cap is 32.
- (3) the buff disappears entirely at 0 shards and comes back at 1 stack
- (4) and (5) the pact and the buff both survive death and logout
- (6) both auras vanish when a new demon arrives
**Trace:**
- `WARL.STW cast attempt pet=<name> result=OK`
- `sacrificed pet=<name> shardsBefore=N shardsAfter=N pact=applied` — **the two shard numbers must be equal**
- `Nathrezim Foresight applied shards=N stacks=N` on first application
- `Nathrezim Foresight restacked A -> B (shards=N damageTaken=-N crit=N)` each time the count moves
- `shards=0 -- Nathrezim Foresight removed`
- `new demon summoned -- Nathrezim Pact broken`
**Check:** `stacks` and `shards` are equal on every line, and `damageTaken` is
exactly `-stacks` while `crit` is exactly `stacks` — that is the engine's
`amount *= GetStackAmount()` doing the scaling, so a mismatch means the
per-stack basepoints on 200507 drifted.
> Effect 2 is now `aura163` (MOD_CRIT_DAMAGE_BONUS) with **MiscValue 127**, not
> `aura57` (MOD_SPELL_CRIT_CHANCE). Aura 163 is read by school mask, so a
> MiscValue of 0 there means the bonus silently applies to nothing — check that
> field first if crit damage looks unchanged.

### WD-14 — Emberstorm / Molten Skin tree swap
**Steps:** open the Destruction tree.
**Expect:** Emberstorm at tier 1 col 1, Infernal Bargain (ex-Molten Skin) at tier 5 col 2, no broken prereq arrows, tree renders at all.
**Verify:** visual — screenshot it. (A blank tree means Talent.dbc record ordering broke.)

---

## 5. Warlock — Demonology

### WM-01 — Improved Healthstone also restores mana
**Steps:** at low mana, use a Healthstone.
**Trace:** `WARL.HSTONE healthstone=<id> talentPct=10|20 mana N -> M`.

### WM-02a — Imperious Flames: Imp Firebolt doubles on your Immolate
**Steps:** with an Imp out, attack a mob with no Immolate for ~5 firebolts, then Immolate it and watch ~5 more.
**Trace:** `WARL.IMPFLAMES firebolt on <name> NOT boosted (no owner Immolate) damage=N`
then `firebolt on <name> BOOSTED talentPct=100 damage N -> 2N`.

### WM-02b — Felguard learns *and can cast* Immolation Aura
**Steps:** with the talent, summon a Felguard. Hover Immolation Aura on the pet bar — the tooltip
must **not** say "Requires Metamorphosis". Pull a mob and click the ability: the Felguard should
gain the aura and nearby enemies should take Fire damage every 1s for its duration (15s CD).
Then respec it away and re-summon.
**Trace:** `WARL.PET.IMMOAURA Felguard summoned owner=... hasTalent=true knewBefore=false knowsNow=true`, and the mirror on respec-out.
> Learning the spell is not enough to call this passing — 200407 was cloned from the
> Metamorphosis-only 50589 and inherited `Stances = FORM_METAMORPHOSIS`, so the pet knew the
> ability but every cast failed with `SPELL_FAILED_ONLY_SHAPESHIFT` (fixed in
> `woa_2026_08_04_09.sql`). Always verify the cast, not just the trace.
> Known gap by design: respeccing *into* the talent without re-summoning leaves the Felguard without the ability until next summon.

### WM-03 — Demonic Embrace: demon dodge from your intellect
**Steps:** summon a Voidwalker or Felguard, `.woatest state base`, then equip/remove a big intellect item and wait ~4s.
**Trace:** `WARL.PET.DODGE pet=... ownerInt=X ratio=Y -> dodgeAmount=Z` (logged only when it moves).
**Cross-check:** `STATE.PET ... dodge=` in each snapshot.

### WM-04 — Fel Synergy: demon damage heals you
**Steps:** take damage, then let your demon attack for ~20s.
**Trace:** `WARL.PET.SYNERGY pet=... damage=N pct=P heal=H ownerHp X -> Y`.

### WM-05 — Sacrifice of Blood (Health Funnel)
**No trace output.** This talent has no C++ at all. The demon half is core's `spell_warl_health_funnel` casting buff 60955/60956, which `woa_2026_08_04_07.sql` rewrites; the healing half is an `ADD_PCT_MODIFIER` (`SPELLMOD_DOT`, Health Funnel class mask) on the talent itself. Verify from the buff bar and `.debug`.

**Steps:** damage your demon, channel Health Funnel for the full duration.
**Expect:**
- **Sacrifice of Blood** appears on the demon the instant the channel starts (not a second in), and disappears the tick the channel ends.
- Exactly **one** buff, not two — a second icon means the retired module script is still registered.
- Tooltip renders two numbers from data: damage taken −15/−30%, damage dealt +25/+50%. Check the first is **not** shown as a negative.
- Each funnel tick heals ~25/50% more than an untalented control run on the same target.
- **The healing bonus must be Health Funnel only.** Heal the demon from any other source (a bot healer, a healthstone-equivalent, `.damage` then a direct heal) and confirm that heal is *not* inflated. This is the whole reason the bonus is a class-masked spellmod rather than an aura on the demon.
- Your own health drain per tick is **unchanged** by the healing bonus (the tick self-damage is capped at `ManaPerSecond`), and is now the full untalented cost — the 40/80% cost reduction was removed in the redesign.
- Rank 2 swaps the numbers; unlearning the talent produces no buff at all.

### WM-06 — Demonic Brutality: Voidwalker threat
**Steps:** summon a Voidwalker (this fires on aura application).
**Trace:** `WARL.PET.BRUTALITY pet=... threatModAmount=N` — N must be non-zero and match your rank (67/134/200).

**Direct check (damage-derived threat):** select the pet, `.debug threatinfo`. `Physical` must read 167/234/300% by rank; every other school stays 100.00%. This reads `ThreatManager::_singleSchoolModifiers` in memory, so it proves the engine picked the aura up — the `WARL.PET.BRUTALITY` line alone only proves the amount was computed.

**Felguard yellows:** Cleave (30213 chain) and Intercept (30151 chain) are SchoolMask 1, so they ride the same physical multiplier. Cleave a mob, then `.debug threat` on it — no separate mechanism to test.

**Respec safety:** the amount has no heartbeat. Respec ranks with the pet already out, then re-run `.debug threatinfo` without re-summoning. A stale Physical% here is a bug.

### WM-06b — Demonic Brutality: Torment / Suffering threat
**Why separate:** these two carry a flat `SPELL_EFFECT_THREAT`, which `Spell::EffectThreat` adds with `ignoreModifiers = true` — the `MOD_THREAT` aura cannot touch them. `spell_warl_demon_brutality_threat` adds the bonus as a second `AddThreat`.

**Steps:** with the talent, have the Voidwalker cast Torment and then Suffering on a mob.
**Trace:** `WARL.PET.BRUTALITY Torment effIndex=2 on <name> baseThreat=1175 pct=200 bonusThreat=2350` and `Suffering effIndex=0 ... baseThreat=1675 pct=200 bonusThreat=3350` (rank 8, talent rank 3).
**Cross-check:** `.debug threat` on the mob — the pet's total should be ~3x what an untalented run produces. Unlearn the talent and repeat on the same mob type; the ratio between runs is the real signal.
**Expect no line at all** when the talent is unlearned, or when a non-warlock pet casts something with `SPELL_EFFECT_THREAT`.

### WM-07 — Demonic Lash
**Steps:** (a) with a Succubus, use Lash of Pain on a mob, then hit it with a Shadow Bolt. (b) with a Felguard, let it melee for ~20s.
**Expect:** (a) Nether Scar on the target, cast **by you** so it boosts your own damage; (b) bonus shadow damage on Felguard swings.
**Trace:**
- `WARL.PET.LASH Lash of Pain -> Nether Scar on <name> amount=N castBy=<your name> present=yes`
- `WARL.PET.LASH Felguard swing on <name> meleeDamage=N pct=15 bonusShadow=M`

### WM-08 — Fel Domination: +10% demon damage per DoT
**Steps:** with a demon attacking one mob, apply Corruption, then Immolate, then Curse of Agony, one at a time with `.woatest note` between each.
**Trace:** `WARL.PET.FELDOM pet=... target=... ownerDots=1|2|3 (cap=8) -> damageDonePct=10|20|30`.

### WM-09 — Demonic Aegis empowers Demon Armor only
**Steps:** cast Fel Armor (`.woatest note fel armor`), then Demon Armor (`.woatest note demon armor`), then cancel it.
**Expect:** the armor/crit-avoidance bonus appears only with **Demon Armor**.
**Trace:** `WARL.AEGIS applied on demonArmor=<id> armorPct=N critAvoidPct=M armor X -> Y`, and `removed with demonArmor=<id>`. No `WARL.AEGIS` line at all for Fel Armor.

### WM-09b — Demonic Aegis does not survive a respec
**Steps:** with 3/3 Demonic Aegis and Demon Armor up, note your armor. Then (a) switch spec, and (b) reset talents.
**Expect:** in both cases Demon Armor is stripped along with the bonus, and armor returns to its unbuffed value. Re-taking the talent at a lower rank and recasting Demon Armor gives the **new** rank's amount, not the old one.
**Trace:** `WARL.AEGIS talent <id> lost -- bonus and Demon Armor stripped, armor now X`.

### WM-10 — Mana Feed
**Steps:** at low mana, let your demon attack for ~30s. Then channel Health Funnel on a mana-drained demon.
**Trace:**
- `WARL.PET.MANAFEED pet=... damage=N pct=P mana=M ownerMana X -> Y` (throttled to 1/sec by spell_proc)
- `WARL.MANAFEED Mana Feed tick demon=... pct=20 mana=M demonMana X -> Y`

### WM-11 — Fel Attunement: demon inherits your haste
**Steps:** `.woatest state base`, then apply a haste buff to yourself (Bloodlust/Heroism, or a haste trinket) and wait ~4s.
**Trace:** `WARL.PET.HASTE pet=... ownerModSpeed=X ownerHaste=Y% inheritPct=75|150 -> petHaste=Z petAtkTime=T` (only when it moves).
**Also check:** the pet's `atkTime` in `STATE.PET` must not double-dip when Bloodlust hits the pet directly too — the pet is immunised against direct haste auras.

### WM-12 — Molten Core
**Steps:** cast ~20 Shadow Bolts at a dummy, then keep Corruption/Immolate ticking for ~30s.
**Expect:** Shadow Bolt applies a stacking-by-damage DoT (Molten Fury) every time; Corruption/Immolate ticks still proc the Incinerate/Soul Fire buff at 4/8/12%.
**Trace:**
- `WARL.MOLTENCORE Molten Fury on <name> shadowBoltDmg=N pct=P fromHit=A carriedOver=B total=C ticks=T perTick=D` — **every** Shadow Bolt, and `carriedOver` must be non-zero on refreshes
- `WARL.MOLTENCORE effect0 (Incinerate/Soul Fire) talent=... chance=4|8|12 roll=PASS|fail` — only on periodic ticks

### WM-13 — Demonic Resilience: demon takes less damage
**Steps:** summon any demon.
**Trace:** `WARL.PET.RESILIENCE pet=... damageTakenPct=N` — **N must be negative** (-5/-10/-15). A positive number here is a bug.

### WM-14 — Nemesis
**Steps:** at rank 3, spend one timed minute casting at a dummy with the demon
attacking, using `.woatest note` to bracket it. Neither half is crit-gated any
more, and neither rolls in script — both rates come from `spell_proc`'s
`ProcsPerMinute`, so the test is the *rate*, not the roll.
**Trace:**
- `WARL.NEMESIS owner proc talent=63123 (spell_proc PPM gated)` → `WARL.SHARD gain reason=nemesis-owner`, ~6 per minute of casting
- `WARL.NEMESIS pet=... proc carrier=200426 (spell_proc PPM gated)` → `gain reason=nemesis-pet`, ~6 per minute of demon attacks
- The carrier id must track the rank: 200422 / 200425 / 200426 for ranks 1/2/3. The wrong id here means `spell_pet_auras` did not update.
- DoT ticks must **not** appear as procs — `PROC_FLAG_DONE_PERIODIC` is deliberately off.

### WM-15 — Metamorphosis as a shard-fuelled stance
**Steps:**
1. With 3 shards, cast Metamorphosis and stay in form until it drops.
2. `.woatest note recast` — re-cast immediately.
3. `.woatest note empty` — with 0 shards, try to cast it.
**Expect:** 1 shard to enter, 1 more every 6s, drops on empty with a chat warning, **no cooldown**, no armor bonus (that moved to Demonic Aegis), +20% damage, right-clickable off.
**Trace:**
- `WARL.META cast attempt shards=N need=1 result=OK|FAILED_REAGENTS`
- `entered form shardsLeft=N`
- `upkeep tick paid shardsLeft=N` every 6 seconds
- `upkeep tick FAILED (no shards) -- form dropping`
**Check:** the snapshot must show no `MOD_BASE_RESISTANCE_PCT` effect on aura 47241.

### WM-16 — Master Summoner
**Steps:** summon a demon with 0, 1 and 2 points.
**Expect:** 10s → 6s → 2s cast, and -40/-80% mana.
**Verify:** stopwatch + the talent aura amounts in the snapshot.

### WM-17 — Voidwalker Torment is a real taunt
**Steps:** let a mob attack you, then Torment with the Voidwalker.
**Expect:** the mob actually swaps to the Voidwalker, 5s cooldown, threat still added between taunts.
**Verify:** visual. (Pure DBC — modelled on warrior Taunt 355.)

### WM-18 — Succubus Seduction survives damage
**Steps:** Seduce a mob while a DoT is ticking on it; then attack it repeatedly.
**Expect:** a single DoT tick no longer breaks it instantly; damage accumulates toward a break; the Succubus's own auto-attack no longer shatters its own channel.
**Verify:** visual + the Seduction aura's presence across snapshots.

---

## 6. Rogue — Subtlety

### RS-01 — Feint grants parry
**Steps:** `.woatest state base`, cast Feint with **no target selected**, `.woatest state after feint`.
**Expect:** the cast succeeds untargeted; parry jumps; 15s duration; no threat drop.
**Verify:** `STATE.AVOID parry=` between the two snapshots, and the Feint aura in `STATE.AURA` with `aura47` (MOD_PARRY_PERCENT) and a non-zero duration.
> The three traps this catches: DurationIndex 0 (aura never applies), `EffectRealPointsPerLevel` shrinking the value, and the `$s1` off-by-one.

### RS-02 — Sleight of Hand
**Steps:** snapshot with the talent, then Feint and snapshot again.
**Verify:** talent aura 30892/30893 effects 2 and 3 must be `aura184`/`aura185` (attacker melee/ranged **hit** chance) at -3/-6 — not `aura187`/`aura188`. Feint's parry amount should be 20/40% larger than in RS-01.

### RS-03 — Filthy Tricks
**Steps:** use Evasion, note the cooldown; use Feint, note the cooldown.
**Expect:** both reduced; **Preparation's cooldown is no longer touched** (it's Bladework now, and Bladework has no cooldown).

### RS-04 — Shadowstep reduces chance to be hit
**Steps:** `.woatest state base`, Shadowstep to a target, `.woatest state after step`, then use an ability and snapshot once more.
**Expect:** 44373 lands as a -10% attacker-hit buff for 15s and is **not** consumed by your next ability.
**Verify:** aura 44373 in `STATE.AURA` with `aura184`/`aura185` at -10, `charges=0`, still present in the third snapshot.

### RS-05 — Camouflage shortens Shadowstep and Shadow Dance
**Steps:** use each, time the cooldown, compare against an untalented spec.
**Expect:** at 3/3, Shadowstep 30s → **20s** and Shadow Dance 60s → **50s** (-10s from effect 3).

### RS-06 — Elusiveness shortens Evasion and Shadow Dance
**Steps:** same method.
**Expect:** at 2/2, Evasion 180s → **150s**, Shadow Dance 60s → **50s**, Cloak of Shadows 90s → **60s**.
With Camouflage 3/3 as well, Shadow Dance bottoms out at **40s**.
> **The regression to watch:** Elusiveness 2/2 originally cut Shadow Dance by the full 60s, which zeroed it entirely — `Player.cpp:10985` clamps a negative `rec` to 0 and then returns without setting any cooldown. Shadow Dance must show a real cooldown at 2/2 with no Camouflage.
> **The one line most worth confirming in game:** Evasion's cooldown lives in `CategoryRecoveryTime`, not `RecoveryTime`. `SPELLMOD_COOLDOWN` should apply to both, but this is the case to actually watch. Evasion now rides **effect 2** alongside Cloak of Shadows (`woa_2026_08_04_25.sql`), so check Cloak is still 60s too — a mask slip there would show up on Cloak, not Evasion.

### RS-07 — Waylay
**Steps:** Ambush or Backstab a mob 5 times.
**Expect:** the debuff lands **every** time (100% at both ranks now), 8s, -2/4% of the target's hit chance and +2/4% crit taken.
**Verify:** the target carries 200502 or 200503 — snapshot while it's up, or watch the debuff icon. Rank 1 must no longer be a 50% roll.

### RS-08 — Ghostly Strike
**Steps:** use it, snapshot immediately.
**Verify:** the dodge buff duration in `STATE.AURA` reads 20000, not 7000.
> The dagger bonus here is core's +50% (187.5%), deliberately not overridden.

### RS-09 — Tricks of the Trade, self-cast
**Steps:** target yourself and cast Tricks of the Trade. Then cast it on an ally (if you have one).
**Expect:** self-cast is now allowed, gives dodge = 20% of your crit chance and +100% threat for 30s. Ally case unchanged.
**Trace:**
- `ROG.TRICKS SELF-CAST detected buff=200500 applied=yes`
- `ROG.TRICKS self-buff meleeCrit=X pctOfCrit=20 -> dodgeBonus=Y dodgeField=Z`
- `ROG.TRICKS cast on ally <name> -- core redirect only, no self buff`
**Check:** `dodgeBonus` must equal 20% of `meleeCrit`, and it re-computes every 2s — swap in a crit trinket and confirm it moves.

### RS-10 — Master of Deception: agility → dodge
**Steps:** `.woatest state base`, equip a large agility item, wait ~4s, `.woatest state after agi`.
**Trace:** `ROG.MOD rank=... agi=X dodgeDim=A dodgeNonDim=B classBase=C fromAgility=D bonusPct=33|66|100 -> extraDodge=E totalDodgeField=F`.
**Check:** `extraDodge` ≈ `fromAgility × bonusPct/100`, and it moves when agility does.

### RS-11 — Dirty Tricks: Ambush/Backstab from the front
**Steps:**
1. With the talent, stand **in front of** a mob and Backstab, then Ambush.
2. `.woatest note untalented` — respec it away and repeat.
**Expect:** (1) both land; (2) both are refused with "You must be behind your target".
**Trace:** `ROG.DIRTY spell=<id> target=<name> talented=true|false casterInFrontOfTarget=true result=OK|FAILED_NOT_BEHIND`.
> This one is global-state sensitive: the attribute is cleared for *every*
> spell at load and re-imposed per cast, so the untalented half is the
> important half to run.

### RS-12 — Hemorrhage
**Steps:** Hemorrhage a dummy, then hit it 15+ times over the 15s window.
**Expect:** the physical-damage debuff no longer expires after 10 hits, and its amount scales with your AP.
**Trace:** `ROG.HEMO hemorrhage=<id> target=<name> dbcBase=13|21|29|42|75 ap=A apPct=5 fromAp=B total=C`.
**Check:** `total = dbcBase + 5% of ap`, and the debuff's `charges=0` in a snapshot taken while it is up.

### RS-13 — Heightened Senses
**Steps:** at reduced health, tank 2–3 melee mobs until you have taken ~20 dodges. Include mobs that use melee *abilities*, not just white swings. Then cast several finishers and confirm none of them heal — the old combo-point path is gone.
**Trace:** none — this talent is pure DBC plus one `spell_proc` row, so there is no script to instrument. Verify from the **combat log** instead: each dodge must be followed by a "Heightened Senses" heal. Bracket the run with `.woatest begin`/`end` so the `DumpState` snapshots record max health for the arithmetic check.
**Check:** one heal per dodge with no gaps — there is no roll and no internal cooldown, so *every* dodge must produce one. The heal equals 2% (rank 1) / 4% (rank 2) of max health.
> If nothing heals, suspect the `spell_proc` row: `HitMask` at 0 defaults to normal+crit and never matches a dodge.
> A dodged **ability** healing as well as white swings is what `ProcFlags` bit 32 buys — if only autoattacks heal, the flag was dropped.
> If the heal fires but for a flat/absurd amount, 200501 lost `Effect1 = 136` (`SPELL_EFFECT_HEAL_PCT`) and is treating the percentage as raw hit points.

### RS-14 — Enveloping Shadows
**Steps:** with 3+ mobs clustered, spend ~20 finishers at 5 combo points. Include a few finishers
that land the killing blow, and a few at 1 combo point.
**Trace:** `ROG.FINISH enveloping ratePerCp=6|12|20 cp=5 chance=C roll=PASS`, then
`enveloping Slice and Dice rank=<id> present=yes duration=21000` and
`enveloping Expose Armor rank=<id> cp=N bp=<value> radius=10 candidates=N applied=M`.
**Check — both buffs must arrive at full 5-point strength, whatever was actually spent.** That is the
design intent: one combo point buys back the points and energy two manual finishers would have cost.
- `present=yes` on every PASS, **including** the finishers that killed the target — the buff is
  applied straight to the rogue, so it must not depend on the victim surviving.
- `duration` is **always 21000**, at 1 combo point as much as at 5 (24000 with Glyph of Slice and
  Dice). A value that tracks the points spent means the duration override was lost.
- Expose Armor lands at **−3925 armour** at every combo point count. Inspect the debuff on the mob,
  don't just trust the log. `bp` in the trace is the compensated base value, so it *should* vary with
  `cp` (−3141 at 1 point, −1 at 5) — that is the compensation working, not a bug. The resulting
  armour reduction is what must stay constant.
- Rank matters: a level 30-41 rogue who only knows rank 1 must get 5171 (+20% haste), not 6774.
  Pre-Wrath Expose Armor ranks reduce armour by a *percentage* and never scaled with combo points, so
  `bp` will equal the base value unchanged on those ranks.
- No runaway recursion — the triggered Expose Armor casts must produce **no** further
  `ROG.FINISH finisher=` lines.

### RS-15 — Bladework (was Preparation)
**Steps:**
1. Build 5 combo points, stand in a pack of 3+ mobs, cast Bladework.
2. Let them attack you and count the counterattacks.
3. `.woatest note cp1` — repeat at 1 combo point, then at 3.
4. `.woatest note dagger` — repeat with a dagger equipped, then with a non-dagger.
5. `.woatest note behind` — let a mob attack you from behind, and get stunned.
**Expect:** 30 energy (20/10 with Dirty Deeds), combo points consumed, no cooldown, everything in 10 yards marked for 30s, **one counter per combo point spent** (1 point → 1 counter, 5 points → 5), 125% weapon damage → 180% with a dagger, no counter while stunned or from behind.
**Trace:**
- `ROG.BLADEWORK marked <name> comboPoints=5 charges=5 dur=30000` — one per mob hit
- `ROG.BLADEWORK countering <name> chargesBefore=5|4|3|2|1`
- `ROG.BLADEWORK counterattack damage X -> Y weapon=dagger pct=180` / `weapon=non-dagger pct=125`
- `ROG.BLADEWORK attack from <name> NOT countered inFront=false stunned=false`
**Check:** `charges=` matches `comboPoints=` on every mark, and exactly that many `countering` lines per marked mob, then no more.

---

## 7. Handing results back

Just tell me which case ids you ran. I'll read
`<LogsDir>/alonecraft_test.log`, match each case against the expectations
above, and report per case: pass, fail with the offending line, or
inconclusive with what to re-run.

If you want the trace trimmed between sessions, deleting the file is safe —
it is reopened on the next write.
