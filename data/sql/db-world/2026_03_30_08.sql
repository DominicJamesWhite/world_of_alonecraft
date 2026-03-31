-- Fix spell_custom_attr for Aegis of Antonidas (formerly Amplify Magic)
-- EFFECT_0 was removed by Alonecraft DBC changes, so POSITIVE_EFF0 flag is invalid.
-- Update from POSITIVE_EFF0|POSITIVE_EFF1 (100663296) to POSITIVE_EFF1 only (67108864).
DELETE FROM `spell_custom_attr` WHERE `spell_id` IN (1008, 8455, 10169, 10170, 27130, 33946, 43017);
INSERT INTO `spell_custom_attr` (`spell_id`, `attributes`) VALUES
(1008, 67108864),
(8455, 67108864),
(10169, 67108864),
(10170, 67108864),
(27130, 67108864),
(33946, 67108864),
(43017, 67108864);
