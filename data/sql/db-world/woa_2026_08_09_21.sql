-- ============================================================
-- Warrior (Protection): Anticipation -- 25%, and the charge is consumed
-- ============================================================
-- Two fixes to woa_2026_08_09_08.sql.
--
-- 1. RETUNE.  The next-attack bonus goes from 2/4/6/8/10% to 5/10/15/20/25%.
--    10% at the end of a five point talent was not worth noticing next to what
--    the rest of the tier offers.
--
-- 2. THE CHARGE NEVER DROPPED.  The bonus applied and then stayed applied.
--
-- Why.  ProcCharges = 1 on a spell modifier is only half the mechanism.  The
-- charge is dropped by Player::RemoveSpellMods (Player.cpp:10028), which only
-- considers modifiers that were registered in Spell::m_appliedMods, and that
-- registration happens in Player::ApplyModToSpell (Player.cpp:10081):
--
--     void Player::ApplyModToSpell(SpellModifier* mod, Spell* spell)
--     {
--         if (!spell)
--             return;                       // <-- here
--         ...
--         spell->m_appliedMods.insert(mod->ownerAura);
--     }
--
-- SPELLMOD_DAMAGE for a melee ability is applied from
-- Unit::MeleeDamageBonusDone (Unit.cpp:10398-10400), which calls ApplySpellMod
-- WITHOUT a Spell pointer -- the parameter defaults to nullptr.  So the modifier
-- multiplies the damage, is never recorded against the cast, and is therefore
-- never charged.  The "next spell consumption buff" pattern in CLAUDE.md is
-- accurate for casters, where the value flows through
-- SpellEffectInfo::CalcValue with the Spell in hand; it does not hold for the
-- melee damage path.
--
-- The fix is the mechanism core itself defers to.  RemoveSpellMods opens with:
--
--     // don't handle spells with spell_proc entry defined
--     if (sSpellMgr->GetSpellProcEntry(mod->spellId))
--         continue;
--
-- Giving each buff its own spell_proc row hands charge management to the proc
-- system, which drops a charge when the aura procs regardless of how the
-- modifier was applied.  The aura needs no proc-trigger effect for this -- the
-- charge drop is the whole point of the row.
--
--   ProcFlags 16 = 0x10 PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS.
--       Deliberately NOT 0x4 (DONE_MELEE_AUTO_ATTACK).  The modifier only
--       affects spells, so a white swing must not eat the charge -- that would
--       be strictly worse than the bug being fixed.
--   SpellPhaseMask 2 is mandatory: 0x10 is a DONE spell flag and therefore in
--       REQ_SPELL_PHASE_PROC_FLAG_MASK (SpellMgr.h:184), so a zero phase mask
--       would drop every proc silently.
--   Charges 1 restates ProcCharges from the DBC; LoadSpellProcs would inherit
--       it anyway (SpellMgr.cpp:2081), but the row is where the behaviour now
--       lives so it is written explicitly.
--
-- Single-column edits use UPDATE rather than a 234-column re-INSERT, so nothing
-- set by woa_2026_08_09_08.sql is silently reverted.
--
-- Spells:
--   200641 - 200645 = Anticipation buff ranks 1-5
-- ============================================================

-- ------------------------------------------------------------
-- Retune to 5/10/15/20/25%
-- ------------------------------------------------------------
-- 200641: 5% damage on the next warrior ability
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 4 WHERE `ID` = 200641;
-- 200642: 10% damage on the next warrior ability
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 9 WHERE `ID` = 200642;
-- 200643: 15% damage on the next warrior ability
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 14 WHERE `ID` = 200643;
-- 200644: 20% damage on the next warrior ability
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 19 WHERE `ID` = 200644;
-- 200645: 25% damage on the next warrior ability
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 24 WHERE `ID` = 200645;

-- ------------------------------------------------------------
-- Hand charge management to the proc system
-- ------------------------------------------------------------
DELETE FROM `spell_proc` WHERE `SpellId` IN (200641, 200642, 200643, 200644, 200645);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `AttributesMask`, `DisableEffectsMask`, `ProcsPerMinute`, `Chance`, `Cooldown`, `Charges`) VALUES
(200641, 0, 0, 0, 0, 0, 16, 0, 2, 0, 0, 0, 0, 100, 0, 1),
(200642, 0, 0, 0, 0, 0, 16, 0, 2, 0, 0, 0, 0, 100, 0, 1),
(200643, 0, 0, 0, 0, 0, 16, 0, 2, 0, 0, 0, 0, 100, 0, 1),
(200644, 0, 0, 0, 0, 0, 16, 0, 2, 0, 0, 0, 0, 100, 0, 1),
(200645, 0, 0, 0, 0, 0, 16, 0, 2, 0, 0, 0, 0, 100, 0, 1);

-- ------------------------------------------------------------
-- Talent descriptions follow the buff via $<id>s1, so only the buff
-- tooltips need no edit -- but the talent prose says "next attack",
-- which is now worth restating as the ability it actually affects.
-- ------------------------------------------------------------
UPDATE `alonecraft_spell_dbc` SET `SpellDescription0` =
    'Increases your Dodge chance by $s1%, and dodging an attack increases the damage of your next Warrior ability by $200641s1%.' WHERE `ID` = 12297;
UPDATE `alonecraft_spell_dbc` SET `SpellDescription0` =
    'Increases your Dodge chance by $s1%, and dodging an attack increases the damage of your next Warrior ability by $200642s1%.' WHERE `ID` = 12750;
UPDATE `alonecraft_spell_dbc` SET `SpellDescription0` =
    'Increases your Dodge chance by $s1%, and dodging an attack increases the damage of your next Warrior ability by $200643s1%.' WHERE `ID` = 12751;
UPDATE `alonecraft_spell_dbc` SET `SpellDescription0` =
    'Increases your Dodge chance by $s1%, and dodging an attack increases the damage of your next Warrior ability by $200644s1%.' WHERE `ID` = 12752;
UPDATE `alonecraft_spell_dbc` SET `SpellDescription0` =
    'Increases your Dodge chance by $s1%, and dodging an attack increases the damage of your next Warrior ability by $200645s1%.' WHERE `ID` = 12753;

-- ------------------------------------------------------------
-- Buff-bar tooltips say "next attack", which overstates the scope: the
-- modifier only reaches spells, so a white swing gets nothing.  The buff bar
-- is the one place a player checks mid-fight, so it should not promise a
-- bonus that a melee swing will not receive.
-- ------------------------------------------------------------
UPDATE `alonecraft_spell_dbc`
   SET `SpellToolTip0` = 'Your next Warrior ability deals $s1% additional damage.'
 WHERE `ID` IN (200641, 200642, 200643, 200644, 200645);

UPDATE `alonecraft_spell_dbc`
   SET `SpellDescription0` = 'Your next Warrior ability deals $s1% additional damage.'
 WHERE `ID` IN (200641, 200642, 200643, 200644, 200645);
