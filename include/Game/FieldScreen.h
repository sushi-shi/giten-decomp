#ifndef GITEN_GAME_FIELDSCREEN_H
#define GITEN_GAME_FIELDSCREEN_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/CellCode.h>
#include <Game/FieldObject.h>
#include <Game/GameState.h>
#include <Game/ViewDirection.h>

// The field screen: its redraw requests and menu mode.

// @identity-TODO: word 0x47b7a0 as a busy flag is inferred (set 1 in command input/enemy
// action, AdvanceObjectAnims count in 0x12d20, 0 gates 0x135b0); name its reader 0x12860.
i16 SetFieldBusy(i16 busy);

b16 CanCharacterOpenAutomap(CharacterCore* character);
i16 PickAnalyzeTarget(void);

void RequestFieldRefresh(void);
void RefreshFieldScene(void);
i16 ExchangeViewHold(i16 hold);
void ResetFieldCursors(void);
// @identity-TODO: empty Windows hook; callers pass a low-three-bit screen option.
void FieldScreenNop(i16 unused);

void InitFieldPanels(void);

// @identity-TODO: what 0x14770 (mask reset, position snapshot) and 0x3980 (view redraw, refresh
// flag latch) complete is only partly decoded; decode 0x14770/0x15b90.
b16 UpdateFieldScreen(i16 force);

static __inline b16 FlushFieldScreen(void) {
    FlushObjectRedraws();
    return UpdateFieldScreen(false);
}

// @identity-TODO: which menu the table 0x4687e8 is and what flag 0x8000 means (disabled?) are
// unrecovered; decode 0x22aa0/0x22ab0.
// Which command rows the field panel permits. The names give the disabled
// command set: skill=2, item=3, fight=5, talk=6, mapping=7.
GZ_ENUM_BEGIN_SPLIT(FieldMenuMode, i16)
    FIELD_MENU_ALL = 0,
    FIELD_MENU_NO_SKILL_ITEM_FIGHT = 1,
    FIELD_MENU_NO_FIGHT_TALK_MAPPING = 2,
    FIELD_MENU_NO_SKILL_ITEM_FIGHT_MAPPING = 3
GZ_ENUM_END_SPLIT(FieldMenuMode)

void SetFieldMenuMode(GZ_ENUM_PARAM(FieldMenuMode, i16) mode);

// @identity-TODO: What the saved point is used for (the spot 0x125d0 restores after a scene) is
// inferred from its only reader pair.
void SaveReturnPoint(void);

void RecordWarpInLeader(void);

void SetReturnPointAhead(void);

// @identity-TODO: That wall kind 1 (preferred over 0 = open) is a door is inferred; confirm
// from the wall renderer.
GZ_ENUM_RETURN(ViewDirection, i16) FindExitDirection(i16 x, i16 y);

// @identity-TODO: That 0x9e30(0,3,1,0) is the party-member pick and 0xeaa0 maps an actor to its
// talk entry is inferred.
i16 RunPendingTalk(void);

void PollFieldCommand(void);

// @identity-TODO: What 0x21680 undoes when the field map is cut short is undecoded.
void CancelFieldMap(void);

// Runs the event of the party's cell (CheckCellEvent's kind), unless it
// stands on the marked cell it came through.
void RunCellEvent(void);

// Applies the cell trap's HP damage to the party when mode is nonzero.
void RunCellTrap(i16 mode, i16 x, i16 y);

// @identity-TODO: What the three images keyed ((id|0x100)<<4, arg) by 0x14460 show is unproven;
// no loader writes 0x47b7b8..0x47b7c0 in this build.
void FreeFieldImageCache(void);

// Retained wall callback arguments; this port only returns the hidden flag.
i16 AreObjectsHidden(i16 view, i16 across, i16 along, i16 side);

// Updates the magnified map region as the held mouse moves between cells.
void TrackWorldMapCursor(i16 layer);

MapCoord GetMouseTravelCell(void);

// @identity-TODO: That 0x1d0e0(area,level,x,y) marks a cell as visited on the automap is
// inferred from its per-step use in 12d20.
void RevealAutomapRoom(i16 x, i16 y);

// @identity-TODO: Which menu 0x468858 is (the one after SetFieldMenuMode's 0x4687e8) is
// unrecovered.
void LoadCommandMenuImage(void);

// @identity-TODO: Same menu identity question as LoadCommandMenuImage.
void FreeCommandMenuImage(void);

i16 GetObjectsHidden(void);
b16 ExchangeObjectsHidden(b16 hidden);

// @identity-TODO: What effect 0x15cc0 plays (sound 6, frames 0..8 of image 0x47b7fc) is
// unrecovered.
void FreeEffectFrames(void);

void FreeEffectBackdrop(void);

// @identity-TODO: Which panel menu 0x4686a8 (image 0x118 via 0x16960) is is unrecovered; it
// sits just before statestack.c (0x169e0) but touches no state-stack data.
void ClearFieldPanelSelection(void);

// @identity-TODO: No reader of word 0x47be58 besides this exchange and its reset 0x1a1d0
// exists, so what the flag gates is unproven; a data-xref of 0x47be58 through a pointer would
// settle it.
i16 SetSubscreenActive(i16 active);

// Loads the offset-indexed text records for field messages.
void LoadFieldMessages(void);

// A count followed by one offset for each field-event message code.
typedef struct FieldMessageOffsets {
    u16 count;
    u16 offsets[96];
} FieldMessageOffsets;

typedef struct FieldMessage {
    u8 prefix; // @identity-TODO: the leading byte is not read by the renderer.
    char text[];
} FieldMessage;

FieldMessage* GetFieldMessage(i16 code);
b16 DrawFieldMessage(GZ_ENUM_PARAM(CellCode, i16) code, i16 band, i16 marked);

void ResetSubscreen(void);

#endif // GITEN_GAME_FIELDSCREEN_H
