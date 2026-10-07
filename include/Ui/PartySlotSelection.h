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

// Filters the selected position in place; the poller register colors require
// the slot to stay one variable rather than an inline parameter copy.
#define FilterPartySlotSelection(slot, mode)                                                       \
    do {                                                                                           \
        if ((mode) == PARTY_SLOT_REQUIRE_OCCUPIED && GetPartySlot(slot) == PARTY_SLOT_EMPTY) {     \
            (slot) = PARTY_POSITION_NONE;                                                          \
        }                                                                                          \
        if ((mode) == PARTY_SLOT_EXCLUDE_HUMANS && GetPartySlot(slot) != PARTY_SLOT_EMPTY          \
            && GetPartyRosterId(slot) < HUMAN_ID_LIMIT) {                                          \
            (slot) = PARTY_POSITION_NONE;                                                          \
        }                                                                                          \
    } while (0)

// Both panel and text-plane pollers wait for a mouse click, then confirm or
// cancel the selected party position.
GZ_ENUM_BEGIN_SPLIT(PartySlotPollResult, i16)
    PARTY_SLOT_POLL_CANCELLED = -1,
    PARTY_SLOT_POLL_WAITING = 0,
    PARTY_SLOT_POLL_CONFIRMED = 1
GZ_ENUM_END_SPLIT(PartySlotPollResult)

GZ_ENUM_RETURN(PartySlotPollResult, i16) PollTextPartySlotSelection(
    GZ_ENUM_PARAM(PartySlotSelectionMode, i16) mode
);

// Mode 0 excludes empty slots; mode 2 excludes occupied human slots.
GZ_ENUM_RETURN(PartySlotPollResult, i16) PollPartySlotSelection(
    GZ_ENUM_PARAM(PartySlotSelectionMode, i16) mode
);

void ClearPartySlotSelection(void);

#endif // GITEN_UI_PARTYSLOTSELECTION_H
