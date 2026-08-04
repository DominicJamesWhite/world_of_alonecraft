-- ===========================================================================
-- Warlock / Demonology: Demonic Embrace tooltip wording fix
-- ===========================================================================
--
-- woa_2026_08_02_03.sql described the new half of Demonic Embrace as
-- increasing pet dodge "by an amount equal to your intellect", which reads as
-- a flat 1 intellect -> 1% dodge.  That is not what the script does.
--
-- spell_warl_demon_dodge (WarlockDemonPets.cpp) feeds the owner's intellect
-- through the same conversion Player::GetDodgeFromAgility uses for agility --
-- the druid crit-per-stat ratio from gtChanceToMeleeCrit times the druid
-- crit_to_dodge coefficient -- so the pet gets dodge *from* intellect, not
-- dodge *equal to* intellect.  The tooltip now says only that, and leaves the
-- magnitude unstated rather than stating it wrongly.
--
-- No mechanical change: only SpellDescription0 on the three ranks.  Felguard
-- stays in the wording because spell_pet_auras still grants 200409 to both
-- 1860 (Voidwalker) and 17252 (Felguard).
-- ===========================================================================

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Increases your total Stamina by 4%, and your intellect increases the chance for your Voidwalker and Felguard to dodge attacks.'
WHERE `ID` = 18697;

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Increases your total Stamina by 7%, and your intellect increases the chance for your Voidwalker and Felguard to dodge attacks.'
WHERE `ID` = 18698;

UPDATE `alonecraft_spell_dbc`
SET `SpellDescription0` = 'Increases your total Stamina by 10%, and your intellect increases the chance for your Voidwalker and Felguard to dodge attacks.'
WHERE `ID` = 18699;
