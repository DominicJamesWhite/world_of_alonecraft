-- Alonecraft 4.61 -- Nathrezim Foresight: make the buff tooltip scale.
--
-- woa_2026_08_03_16.sql set a hardcoded "1% ... for each Soul Shard you carry"
-- tooltip on the assumption that the client cannot scale tooltip variables by
-- stack count. That assumption was wrong.
--
-- The 3.3.5a client multiplies $sN in an AURA tooltip by the aura's current
-- stack count. Blizzard relies on this everywhere -- Sunder Armor (7386,
-- 5 stacks) is 'Armor decreased by $s1%.', Deadly Poison (2818, 5 stacks) is
-- 'Target takes $s1 Nature damage every $t1 seconds.' Neither says "per
-- stack", because neither needs to. There is no current-stack-count tooltip
-- variable in the DBC at all ($u is MAX stacks) for the same reason.
--
-- Values: effect 1 is BasePoints -2 + max(1, DieSides 0) = -1, effect 2 is
-- BasePoints 0 + max(1, DieSides 1) = +1. So at 17 Soul Shards the buff bar
-- reads "Damage taken reduced by 17%.  Critical strikes do 17% more damage."
--
-- The spellbook description is left as the flat "1% per Soul Shard"
-- explanation -- that text is shown out of context, where there is no stack
-- count to scale against.
--
-- Sibling buffs already use variables and therefore already scale:
--   200402 Wailing Soul       (3 stacks) 'Damage taken reduced by $s1%.'
--   200405 Infernal Bargain   (4 stacks) 'Damage increased by $s1%. ...'

UPDATE `alonecraft_spell_dbc`
SET `SpellToolTip0` = 'Damage taken reduced by $s1%.  Critical strikes do $s2% more damage.'
WHERE `ID` = 200507;
