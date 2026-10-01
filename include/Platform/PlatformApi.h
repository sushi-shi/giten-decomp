#ifndef GITEN_PLATFORM_PLATFORMAPI_H
#define GITEN_PLATFORM_PLATFORMAPI_H

#include <Gfx/Render.h>
#include <Ints.h>
#include <Platform/ScreenFade.h>

// Platform-layer (winmain) functions the game code calls, declared apart from
// the game headers: adding prototypes to those shifts the TU state of the C
// units that include them (docs/patterns/
// tu-state-probe-family-decides-reachability.md).

// The path of save slot `slot` in the Windows directory (LoadGame).
void GetSavePath(char* path, i16 slot);

// count * size bytes with a 32-bit size, zero-filled; NULL when exhausted.
void* AllocClearedLong(u16 count, u32 size);

// Stops the playing music stream.
// @identity-TODO: the word argument is never read.
void StopMusic(i16 unused);

void ResetRenderMode(void);
void EndSaveRenderMode(void);
void SetPictureRenderMode(void);
void SetLayersRenderMode(void);
void SetBlankStep(GZ_ENUM_PARAM(BlankRenderStep, i16) step);

void WaitFrames(i16 count);
void RunFrame(void);
void ShowBusyCursor(void);
void HideBusyCursor(void);

i16 GetPendingKey(void);
void ClearPendingKey(void);

// The wall code (0..15) on side `side` of map cell (x, y), for GetWallAt.
i32 GetWallCode(i32 x, i32 y, i32 side, i32 width, i32 height);

// Hides layer 1's panel and runs move command `command` (0..7; see
// Scene3D.h), advancing the game phase first when `nextPhase` is set; C
// linkage for the script ops that call it (0x4135be).
b32 RunMoveCommand(i16 command, i16 nextPhase);

// Hides text plane `plane` without freeing its surfaces.
void HideTextPlane(i16 plane);

#endif // GITEN_PLATFORM_PLATFORMAPI_H
