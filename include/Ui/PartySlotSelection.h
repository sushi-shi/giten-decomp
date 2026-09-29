#ifndef GITEN_UI_PARTYSLOTSELECTION_H
#define GITEN_UI_PARTYSLOTSELECTION_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/GameState.h>
#include <Ints.h>

GZ_ENUM_BEGIN(PartySlotSelectionMode)
    PARTY_SLOT_REQUIRE_OCCUPIED = 0,
    PARTY_SLOT_ANY = 1,
    PARTY_SLOT_EXCLUDE_HUMANS = 2
GZ_ENUM_END(PartySlotSelectionMode)

static __inline i16 FilterPartySlotSelection(i16 slot, i16 mode) {
    if (mode == PARTY_SLOT_REQUIRE_OCCUPIED && GetPartySlot(slot) == PARTY_SLOT_EMPTY) {
        slot = PARTY_POSITION_NONE;
    }
    if (mode == PARTY_SLOT_EXCLUDE_HUMANS && GetPartySlot(slot) != PARTY_SLOT_EMPTY
        && GetPartyRosterId(slot) < HUMAN_ID_LIMIT) {
        slot = PARTY_POSITION_NONE;
    }
    return slot;
}

i16 PollTextPartySlotSelection(i16 mode);

// Polls the hovered party panel and commits or cancels on a mouse click.
// Mode 0 excludes empty slots; mode 2 excludes occupied human slots.
i16 PollPartySlotSelection(GZ_ENUM_PARAM(PartySlotSelectionMode, i16) mode);

void ClearPartySlotSelection(void);

#endif // GITEN_UI_PARTYSLOTSELECTION_H
