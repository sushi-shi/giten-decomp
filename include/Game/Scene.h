#ifndef GITEN_GAME_SCENE_H
#define GITEN_GAME_SCENE_H

#include <rva.h>

#include <Ints.h>
#include <Game/StateStack.h>

// @identity-TODO: the Windows video-state stubs do not prove its saved extent.
extern u8 g_sceneVideoState[16];
b16 RunCellScene(void);
b16 RunFieldTextScene(void);
b16 RunFrozenFieldScene(void);
b16 RunPictureTransition(void);
b16 RunBackgroundScene(void);
void FreeSceneSprites(void);
void LoadSceneSprites(void);
void PlaceSceneSprites(void);
void SetSceneScript(i16 script, i16 entry);

static __inline void PushFieldTextScene(i16 script, i16 entry) {
    SetSceneScript(script, entry);
    PushGameState(GAME_STATE_FIELD_TEXT_SCENE);
}

// @identity-TODO: What the byte table 0x47bb50 lists (also read at 0x17cb0 for indices 11/15)
// is unrecovered.
void SetSceneScriptByIndex(i16 scriptIndex, i16 entryIndex);

struct CellHead;

// Copies the 16 bytes of the cell record into the scene record 0x47bb50.
void SetSceneCell(const void* cell);

// Copies the scene record into `out`; returns `out`.
u8* GetSceneCell(u8* out);

// Sets the script the cell event runs (NULL for none).
void SetCellScript(u8* script);

// Swaps the scene record's parameter bytes +9..+0xb with +0xc..+0xe.
void SwapSceneCellParams(void);
i16 GetSceneCellKind(void);

u32 GetSceneEntry(i16 index);

// @identity-TODO: legacy frame arrays have no live caller proving their extents.
extern i32 g_sceneFrameSaves[32];
extern u8 g_sceneFrameIds[32][2];
extern i16 g_sceneFramePositions[32][2];
void RestoreSceneFrame(i16 slot);
void DrawSceneFrame(i16 image, i16 slot, i16 frame, i16 x, i16 y, i16 mode);

#endif // GITEN_GAME_SCENE_H
