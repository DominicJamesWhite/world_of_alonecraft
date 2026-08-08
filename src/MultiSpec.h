/*
 * Shared multi-spec helpers.
 *
 * The MultiSpec addon listens for machine-readable system messages rather than
 * a custom opcode, so every place that changes the spec count or the previewed
 * spec has to re-announce the state. Keep that in one place.
 */

#ifndef ALONECRAFT_MULTISPEC_H
#define ALONECRAFT_MULTISPEC_H

class Player;

namespace Alonecraft
{
    // Send "MULTISPEC:active:count:preview" (e.g. "MULTISPEC:3:8:5") to the
    // player's own session for the MultiSpec addon to parse.
    void SendSpecState(Player* player);

    // Talent points the given spec SLOT has invested in each TalentTab.dbc
    // tabpage (0..2).  Defined in SpecCommand.cpp.
    //
    // Shared rather than duplicated because the two callers genuinely want the
    // same answer: a slot index is not a spec.  With up to 8 slots
    // (Alonecraft.ExtraSpec.MaxSpecs) GetActiveSpec() is just a save-file
    // number, so anything that needs to know what a spec IS -- the MultiSpec
    // addon drawing its icons, the Quartermaster choosing gear -- has to read
    // that slot's talents.
    void GetTalentTreePointsForSpec(Player* player, uint8 spec, uint8 (&points)[3]);
}

#endif // ALONECRAFT_MULTISPEC_H
