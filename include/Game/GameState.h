#ifndef GITEN_GAME_GAMESTATE_H
#define GITEN_GAME_GAMESTATE_H

#include <EnumDomain.h>
#include <Game/Character.h>
#include <Game/MapArea.h>
#include <Game/MapCoord.h>
#include <Game/ViewDirection.h>
#include <Ints.h>

// The party's place on the map; these ten bytes are also passed by value.
// @identity-TODO: `area`/`level` are named from the new-game values and the
// map checks that compare them (area 0x82, level 8 at start); the bytes at +6
// and +9 are unrecovered.
typedef struct MapPosition {
    i16 x;
    i16 y;
    GZ_ENUM_STORAGE(ViewDirection, i16) direction;
    u8 pad06;
    GZ_ENUM_STORAGE(MapAreaId, u8) area;
    u8 level;
    u8 pad09;
} MapPosition;

// The field state saved as one 16-byte block.
// The party's step/turn state machine (AdvancePartyMove): idle, stepping or
// turning.
GZ_ENUM_BEGIN_SPLIT(FieldMoveState, i16)
    FIELD_MOVE_IDLE = 0,
    FIELD_MOVE_STEPPING = 1,
    FIELD_MOVE_TURNING = 2
GZ_ENUM_END_SPLIT(FieldMoveState)

// @identity-TODO: the movement words are named from the step/turn state
// machine that drives them (the state; the command; the turns still to make).
typedef struct FieldState {
    MapPosition pos;
    GZ_ENUM_STORAGE(FieldMoveState, i16) moveState;
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
// The party positions and the roster size.
#define PARTY_SIZE 6
// Positions below PARTY_ROW_SIZE are the front row, the rest the back row.
#define PARTY_ROW_SIZE 3
#define ROSTER_SIZE 32
// The roster slot of the party leader (GetRosterLeader).
#define ROSTER_LEADER 0
// A party position holding no roster member.
#define PARTY_SLOT_EMPTY (-1)
// What the finders return when there is no such roster slot, party position
// or character id.
#define ROSTER_SLOT_NONE (-1)
#define PARTY_POSITION_NONE (-1)
#define CHARACTER_ID_NONE (-1)
// Character ids below this are human members.
#define HUMAN_ID_LIMIT 32

typedef struct Party {
    FieldState field;
    // @identity-TODO: the facing restored with a saved position (-1 when unset).
    i16 savedDirection;
    // The party's six roster indices (-1 = empty slot).
    i16 slots[PARTY_SIZE];
    FieldStatus status;
    // The roster: characters the party can field; ids below 32 are human members.
    Character* roster[ROSTER_SIZE];
} Party;

extern Party g_party;

// Direct position access; the caller supplies a valid party index.
#define PartySlotAt(index) (g_party.slots[(index)])

// Direct slot access; the caller supplies a valid roster index.
#define RosterMemberAt(slot) (g_party.roster[(slot)])

void InitNewGame(void);

GZ_ENUM_BEGIN_SPLIT(MapValueSelector, i16)
    MAP_VALUE_AREA = 0,
    MAP_VALUE_LEVEL = 1,
    MAP_VALUE_X = 2,
    MAP_VALUE_Y = 3
GZ_ENUM_END_SPLIT(MapValueSelector)

i16 GetMapValue(GZ_ENUM_PARAM(MapValueSelector, i16) which);
MapPosition* GetMapPosition(void);
GZ_ENUM_RETURN(MapAreaId, u8) GetMapArea(void);
u8 GetMapLevel(void);
MapCoord GetMapCoord(void);
Character* AsCharacter(Character* character);
Character* GetRosterCharacter(i16 slot);
i16 FindPartySlot(i16 slot);
ItemSlot GetEquipSlot(Character* character, GZ_ENUM_PARAM(EquipPart, i16) part);
Character* GetRosterEntry(i16 slot);
i16 GetPartySlot(i16 index);

// Puts roster slot `slot` into party position `index`; returns the slot it held.
i16 ExchangePartySlot(i16 index, i16 slot);
Character* GetPartyEntry(i16 index);
Character* GetPartyCharacter(i16 index);
Character* GetRosterLeader(void);

// The roster member whose id is `id` (NULL when none).
Character* GetCharacterById(i16 id);

// -1 for an empty position, else whether the member has a fatal condition.
i16 IsPartyMemberFallen(i16 index);

i16 GetPartyRosterId(i16 index);

// An empty party position when `inParty`, else a free roster slot (-1: none).
i16 FindEmptySlot(b16 inParty);

// Ages every member's conditions by one and rolls them for recovery; nonzero
// when any wore off.
i16 TickPartyConditions(void);

void ResetRosterBattleState(void);

void ResetRosterStatModifiers(void);

void ClearRosterConditions(void);

i16 CountFallenHumans(void);

// @identity-TODO: What 0x9420 does per fallen non-guest member (return/drop hook) and what
// 0x3f0f0 changes (HP/MP-zero conditions) are only partly decoded.
i16 ProcessPartyCasualties(void);

#endif // GITEN_GAME_GAMESTATE_H
