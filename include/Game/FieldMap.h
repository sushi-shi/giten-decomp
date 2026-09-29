#ifndef GITEN_GAME_FIELDMAP_H
#define GITEN_GAME_FIELDMAP_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/AreaMap.h>
#include <Game/ViewDirection.h>

// Field-map events around an encounter.

// @identity-TODO: the body is a bare ret; the name comes only from its single call site (battle
// entry after SaveFieldLayer); a debug/PC-98 counterpart would name it.
void NotifyEncounterStart(void);

// @identity-TODO: the body is a bare ret; the name comes only from its single call site (battle
// exit after CloseMessageWindow).
void NotifyEncounterEnd(void);

// Whether this frame's clock tick ended a 24-tick round (clock.c).
i16 HasTurnElapsed(void);

// @identity-TODO: that 0x211b0/0x21010/0x211f0/0x1f070 re-place the current area's map objects
// is inferred from their use of g_party.field and 0xdbf0/0xda00; decode 0x21010.
void RespawnAreaActors(void);

// Marks the cell (area, level, x, y, facing) the party must leave before a
// cell event there runs again.
void SetCellMark(i16 area, i16 level, i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction);

void SaveFieldPosition(void);

// -1 off the marked cell, else 0 (or, with `checkDirection`, 1 when facing
// another way).
i16 IsOnCellMark(i16 checkDirection);

// The region code the field last entered (EnterRoom sets it).
i16 GetCurrentRoomCode(void);
i16 SetCurrentRoomCode(i16 code);

// Resets field layers 1 and 0 between 0x1f540 and 0x1a190(-1).
// @identity-TODO: what 0x1f540 and 0x1a190 reset is undecoded.
void ResetFieldScene(void);

// @identity-TODO: That the per-cell byte from 0x1ee70/0x1eb80 is a room region (flood-filled
// between wall/door codes 0x1f/0xd by 0x1ed20) is inferred; decode 0x1ec10 to confirm. Also what the pictures
// 0x53/0x4c loaded at area 0x85 level 3 (3,4)/(3,5) show is unrecovered.
void UpdateCurrentRoom(void);

void UnloadAreaMap(void);

// @identity-TODO: Same room-region inference as UpdateCurrentRoom.
i16 BuildRoomMap(i16 detectChanges);

// @identity-TODO: What the kinds 0x48..0x4e (0x1f350) of the level's 8-byte object list +0x14
// are is unrecovered.
void SpawnLevelObjects(void);

void LoadAreaMap(i16 area, i16 level);

void PlayLevelMusic(void);

// @identity-TODO: The meaning of each returned event kind (3 leads to SaveFieldPosition and
// phase 1; 2, 0xb and record bytes are others) is unrecovered.
RVA_DECL(0x00021880)
GZ_ENUM_RETURN(CellEventKind, i16) CheckCellEvent(i16 x, i16 y, i16 level);

#endif // GITEN_GAME_FIELDMAP_H
