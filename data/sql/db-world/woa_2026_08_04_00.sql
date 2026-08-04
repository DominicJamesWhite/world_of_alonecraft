-- Alonecraft 4.61 -- Infernal Bargain: exponential pact + total damage immunity.
--
-- The old shape was a flat 4 sec channel eating 1 Soul Shard per second for 1
-- stack each (peak +16% damage / +8% crit for 4 shards) plus -25% damage taken
-- while channeling. Two problems:
--
--   1. Four shards out of a 32-shard pool is a rounding error, so there was no
--      decision to make -- you pressed it on cooldown.
--   2. -25% is not an "oh shit" button, and it is worse than it reads: spending
--      shards drains Nathrezim Foresight (200507 mirrors the live shard count
--      1:1 as -1% damage taken per stack), so channeling while tanking cut your
--      mitigation on net.
--
-- New shape: the void doubles its price every second -- 1, 2, 4, 8, 16 shards
-- -- and the buff stacks track the same 2^(N-1) curve. Going all the way burns
-- 31 of 32 shards, i.e. the entire Foresight passive, in exchange for 5 sec of
-- invulnerability and a 10 sec +48% damage / +16% spell crit window. The panic
-- button and the burst button are now the same button, which is the point.
--
--   Tick   Cost   Cumulative   Stacks   Damage   Spell crit
--     1      1         1          1       +3%        +1%
--     2      2         3          2       +6%        +2%
--     3      4         7          4      +12%        +4%
--     4      8        15          8      +24%        +8%
--     5     16        31         16      +48%       +16%
--
-- C++ (WarlockInfernalBargain.cpp) charges the escalating price and pins the
-- stack count; everything else is here.
--
-- Ordering note: single-column UPDATEs only, no full-row re-INSERT. This is
-- mandatory, not stylistic -- woa_2026_08_03_19.sql did a DELETE + 234-column
-- INSERT on 63349 and woa_2026_08_03_20.sql had to repair the tooltip
-- afterwards. A fresh full-row insert here would silently revert
-- SpellVisual1 = 200001 and both of those tooltip fixes.

-- ------------------------------------------------------- The channel 63349 ---
--
-- 5 ticks instead of 4. EffectAmplitude1 stays 1000, so DurationIndex 28
-- (5000 ms -- same index Drain Life 689 uses, also at amplitude 1000) lands
-- ticks at 1/2/3/4/5 sec.

UPDATE `alonecraft_spell_dbc` SET `DurationIndex` = 28 WHERE `ID` = 63349;

-- Effect 2: MOD_DAMAGE_PERCENT_TAKEN (87) -> SCHOOL_IMMUNITY (39).
--
-- EffectMiscValue2 is already 127 and aura 39 reads it verbatim as a school
-- mask (SpellAuraEffects.cpp:3993 -> ApplySpellImmune(IMMUNITY_SCHOOL, 127)),
-- so it carries over untouched and this is a one-column swap.
--
-- Why not just set BasePoints to -101 for -100% damage taken? Because aura 87
-- is not total immunity and the holes are the exact situations this button
-- exists for: SPELL_AURA_PERIODIC_DAMAGE_PERCENT ticks never call
-- SpellDamageBonusTaken (the call sits inside an `if (GetAuraType() ==
-- SPELL_AURA_PERIODIC_DAMAGE)` guard, SpellAuraEffects.cpp:6343), and
-- environmental + fall damage gate on isTotalImmune() / IsImmunedToDamageOrSchool
-- (Player.cpp:766, 14007) which read only SCHOOL_IMMUNITY. Aura 87 is also
-- piercable by MOD_IGNORE_TARGET_RESIST_MODIFIERS (Unit.cpp:9008).
--
-- Aura 39 covers every path: white melee returns VICTIMSTATE_IS_IMMUNE before
-- damage is computed at all (Unit.cpp:1745-1764), spells resolve as
-- SPELL_MISS_IMMUNE, existing DoTs are paused rather than removed.
--
-- Base points mean nothing to the immunity handler; zero them so no stale $s2
-- can resolve against this effect.

UPDATE `alonecraft_spell_dbc`
SET `EffectApplyAuraName2` = 39,
    `EffectBasePoints2`    = 0,
    `EffectDieSides2`      = 0
WHERE `ID` = 63349;

-- Three attribute traps, all silent, all verified against 63349 as it stands
-- (Attributes 272, AttributesEx 64, Mechanic 0). Do NOT set:
--
--   SPELL_ATTR1_IMMUNITY_PURGES_EFFECT (0x8000)
--     -- the purge loop at SpellAuraEffects.cpp:4026 would make the immunity
--        strip hostile auras off the warlock on apply.
--   SPELL_ATTR1_IMMUNITY_TO_HOSTILE_AND_FRIENDLY_EFFECTS (0x10000)
--     -- IgnoresSchoolImmunityFromFriendlyCaster (Unit.cpp:9856) is what lets
--        the 200405 self-buff and incoming heals land through the immunity.
--        This flag switches that exemption off, and the buff would just stop
--        applying with no error anywhere.
--   AURA_INTERRUPT_FLAG_TAKE_DAMAGE (0x2) on ChannelInterruptFlags
--     -- Unit.cpp:1060 fires the take-damage interrupt BEFORE the zero-damage
--        early-out at :1155, so it would break the channel even at 0 damage.
--
-- ChannelInterruptFlags stays 3084 (0x0C0C = CAST | MOVE/INTERRUPT | TALK |
-- USE). No damage interrupt and no pushback, which the channel already relied
-- on. The 0x08 bit stays deliberately: standing still and being kickable is
-- the counterplay that pays for invulnerability.

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Channel the souls you carry into the endless void for $d.  The void doubles its price each second, and the pact ends the instant you cannot pay.  Each payment leaves you with Infernal Bargain stacks equal to the Soul Shards spent.  While channeling you are immune to all damage.',
    `SpellToolTip0`     = 'Immune to all damage.'
WHERE `ID` = 63349;

-- SpellToolTip0 is deliberately plain text rather than a $sN variable: the
-- immunity is a constant with no stack count to scale against, and aura 39 has
-- no base points to read (they were just zeroed above). A variable would render
-- "0".

-- ---------------------------------------------------------- The buff 200405 ---
--
-- StackAmount 4 -> 16 to hold the fifth tick.
--
-- Per-stack values come down from +4%/+2% because the peak is now 4x deeper.
-- Tooltip rule: BasePoints = N-1 with DieSides 1, so
--   +3% damage / stack -> EffectBasePoints1 = 2
--   +1% crit   / stack -> EffectBasePoints2 = 0
-- EffectDieSides1/2 stay 1. AuraEffect::CalculateAmount multiplies by the stack
-- count unconditionally (SpellAuraEffects.cpp:580), so 16 stacks read +48% /
-- +16% both in the game and in the tooltip, for free.
--
-- DurationIndex stays 1 (10000 ms).

UPDATE `alonecraft_spell_dbc`
SET `StackAmount`       = 16,
    `EffectBasePoints1` = 2,
    `EffectBasePoints2` = 0
WHERE `ID` = 200405;

-- SpellToolTip0 needs no change -- it is already
-- 'Damage increased by $s1%.  Spell critical strike chance increased by $s2%.'
-- and scales by stack count on its own.
--
-- SpellDescription0 does. It read "...for each Soul Shard sacrificed to the
-- void", which was true when 1 shard bought 1 stack; under the new curve 31
-- shards buys 16 stacks. The spellbook shows the description out of context
-- with no stack count to scale against, so it gets the flat per-stack
-- explanation while the tooltip keeps the variables.

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Your damage is increased by $s1% and your spell critical strike chance by $s2% for each stack of Infernal Bargain.  The void grants stacks equal to the Soul Shards paid, doubling with every second you keep channeling.'
WHERE `ID` = 200405;

-- No spell_script_names change: (-63349, 'spell_warl_infernal_bargain') from
-- woa_2026_08_01_06.sql still stands, and there is no spell_proc row for this
-- ability -- it has never proc'd.
