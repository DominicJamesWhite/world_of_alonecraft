-- ============================================================
-- Warrior (Arms): two talent data fixes
-- ============================================================

-- ------------------------------------------------------------
-- Tactical Mastery (12295 / 12676 / 12677): drop the stance gate
-- ------------------------------------------------------------
-- Stock Tactical Mastery carried Stances = 131072 (1 << (FORM_DEFENSIVESTANCE
-- - 1)) because its effects 2 and 3 were the Defensive Stance threat bonus.
-- woa_2026_08_07_11.sql replaced every effect with the parry <-> crit proc
-- triggers but left the gate behind, so:
--
--   * Player::IsNeedCastPassiveSpellAtLearn (Player.cpp:3297) refused to cast
--     the passive unless the player was already in Defensive Stance -- and
--     Attributes 464 carries neither SPELL_ATTR0_NOT_SHAPESHIFTED nor
--     SPELL_ATTR2_ALLOW_WHILE_NOT_SHAPESHIFTED to escape that check;
--   * Aura::IsRemovedOnShapeLost (SpellAuras.cpp:1038) is true for any spell
--     with a non-zero Stances, so every stance change stripped the aura
--     (SpellAuraEffects.cpp:1627) and only re-cast it when the new stance
--     matched (SpellAuraEffects.cpp:1475).
--
-- Net effect: an Arms warrior in Battle or Berserker Stance had no Tactical
-- Mastery aura at all, so neither direction of the proc could fire.  The
-- symptom was symmetric -- no crit buff on parry AND no parry buff on crit --
-- which is what pointed at the aura rather than at the proc routing.
--
-- Stances = 0 is the whole fix: IsRemovedOnShapeLost then returns false, so
-- the aura survives stance swaps, and IsNeedCastPassiveSpellAtLearn returns
-- true unconditionally.  The two payload buffs (Counterpoise 200610-200612,
-- Guard Up 200613-200615) already have Stances = 0.
UPDATE `alonecraft_spell_dbc` SET `Stances` = 0 WHERE `ID` IN (12295, 12676, 12677);

-- ------------------------------------------------------------
-- Two-Handed Weapon Specialization (12163 / 12711 / 12712):
-- restore per-rank damage scaling
-- ------------------------------------------------------------
-- TODO.md line 385 specifies "Increases the damage you deal with two-handed
-- melee weapons by 2/4/6%, and while using a 2h weapon your chance to parry is
-- increased by 33/66/100% of your critical strike chance."
--
-- woa_2026_08_07_13.sql transcribed that as a flat 6% and shipped
-- EffectBasePoints1 = 5 on all three ranks, then justified the flattening in
-- its header comment ("the rank scaling is entirely the parry conversion").
-- That comment is wrong; do not restore the flat value from it.
--
-- EffectDieSides1 = 1, so CalcValue = BasePoints + 1 and BasePoints N renders
-- N+1 in both the tooltip and the aura amount.  1/3/5 -> 2/4/6%.
--
-- Effect 2 (the parry conversion, EffectBasePoints2 = 32/65/99 -> 33/66/100%)
-- was already rank-correct and is untouched, as is spell_warr_2h_spec_parry,
-- which reads that value per-rank.
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 1 WHERE `ID` = 12163;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 3 WHERE `ID` = 12711;
UPDATE `alonecraft_spell_dbc` SET `EffectBasePoints1` = 5 WHERE `ID` = 12712;
