#ifndef GITEN_GAME_GAMESTATE_H
#define GITEN_GAME_GAMESTATE_H

#include <rva.h>

#include <Game/Character.h>
#include <Game/MapCoord.h>
#include <Ints.h>

// The party's place on the map; these ten bytes are also passed by value.
// @identity-TODO: `area`/`level` are named from the new-game values and the
// map checks that compare them (area 0x82, level 8 at start); the bytes at +6
// and +9 are unrecovered.
typedef struct MapPosition {
    i16 x;
    i16 y;
    i16 direction;
    u8 pad06;
    u8 area;
    u8 level;
    u8 pad09;
} MapPosition;

// The field state saved as one 16-byte block.
// @identity-TODO: the three movement words are named from the step/turn
// state machine that drives them (0x412280: state 0 idle, 1 stepping,
// 2 turning; the command; the turns still to make).
typedef struct FieldState {
    MapPosition pos;
    i16 moveState;
    i16 moveCommand;
    i16 turnsLeft;
} FieldState;

// A saved word: a 14-bit signed value (32 on a new game) and the system
// menu's display choices for auto-mapping and auto-navigation (set for
// "fixed display", clear for "free display").
// The companion limit is this roster capacity less six party slots.
typedef struct FieldStatus {
    i16 rosterCapacity : 14;
    i16 automapFixed : 1;
    i16 navigationFixed : 1;
} FieldStatus;

// The party: its place on the field, its six positions and its roster, saved
// and restored member by member. One object: `slots` is two-aligned, which no
// standalone twelve-byte COMMON is, and InitNewGame's roster clear follows the
// field stores only when the roster and the field share an object.
typedef struct Party {
    FieldState field;
    // @identity-TODO: the facing restored with a saved position (-1 when unset).
    i16 savedDirection;
    // The party's six roster indices (-1 = empty slot).
    i16 slots[6];
    FieldStatus status;
    // The roster: characters the party can field; ids below 32 are human members.
    Character* roster[32];
} Party;

extern Party g_party;

// Direct position access; the caller supplies a valid party index.
#define PartySlotAt(index) (g_party.slots[(index)])

// Direct slot access; the caller supplies a valid roster index.
#define RosterMemberAt(slot) (g_party.roster[(slot)])

void InitNewGame(void);

MapPosition* GetMapPosition(void);
u8 GetMapArea(void);
u8 GetMapLevel(void);
MapCoord GetMapCoord(void);
Character* AsCharacter(Character* character);
Character* GetRosterCharacter(i16 slot);
i16 FindPartySlot(i16 slot);
ItemSlot GetEquipSlot(Character* character, i16 part);
Character* GetRosterEntry(i16 slot);
i16 GetPartySlot(i16 index);

// Puts roster slot `slot` into party position `index`; returns the slot it held.
// @identity-TODO: label-only until the party TU claims it.
i16 ExchangePartySlot(i16 index, i16 slot);
Character* GetPartyEntry(i16 index);
Character* GetPartyCharacter(i16 index);
Character* GetRosterLeader(void);

// @identity-TODO: label-only until the party TU claims them.
// The roster member whose id is `id` (NULL when none).
Character* GetCharacterById(i16 id);

// -1 for an empty position, else whether the member has a fatal condition.
i16 IsPartyMemberFallen(i16 index);

i16 GetPartyRosterId(i16 index);

// An empty party position when `inParty`, else a free roster slot (-1: none).
i16 FindEmptySlot(i16 inParty);

// Ages every member's conditions by one and rolls them for recovery; nonzero
// when any wore off.
i16 TickPartyConditions(void);

RVA_DECL(0x0003fb30)
void ResetRosterBattleState(void);

void ResetRosterStatModifiers(void);

void ClearRosterConditions(void);

i16 CountFallenHumans(void);

// @identity-TODO: What 0x9420 does per fallen non-guest member (return/drop hook) and what
// 0x3f0f0 changes (HP/MP-zero conditions) are only partly decoded.
i16 ProcessPartyCasualties(void);

#endif // GITEN_GAME_GAMESTATE_H
