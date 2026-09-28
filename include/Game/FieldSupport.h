#ifndef GITEN_GAME_FIELDSUPPORT_H
#define GITEN_GAME_FIELDSUPPORT_H

#include <rva.h>

#include <Ints.h>
#include <Platform/ScreenFade.h>
#include <Script/ScriptBlock.h>

// Callees of the field per-phase handler whose TUs are not claimed yet; names
// come from a first look at each body. All are label-only declarations.
// @identity-TODO: every name here is provisional; move each prototype to its
// owner header when its TU is claimed.

// Set every main-loop step (0x4019d0) from the time step 0x420e90(0): 1 when
// the frame divider (g_clock.framesPerTick) ran out and the game clock
// ticked, else 0. The party turn and the party picker key on it. Defined in
// clock.c (InitClock clears it).
extern i16 g_tickElapsed;

// Set while a talk with a map object is pending (RequestTalk); the field
// state runs it (RunPendingTalk) when no event fires, and the talk's abort
// handling clears it.
extern i16 g_pendingTalk;

i16 GetFieldBusy(void);

// Drops the cached field image (FreeImageHandle on it).
void ReleaseFieldImage(void);

void ClearEncounterPending(void);

i16 GetEncounterPending(void);

// Sets bit 11 / bit 0 of the flags of item 1 of the list 0x468858; returns the
// old bit.
// @identity-TODO: that the list is the field command menu (drawn by 0x15940)
// is inferred; what the two bits mean is unrecovered.
b16 SetFieldStatusBit11(i16 on);

b16 SetFieldStatusBit0(i16 on);

RVA_DECL(0x00049cb0)
void SetFieldRenderMode(void);

// @identity-TODO: What layouts 0 and 1 of the top bar are (0x1e790 draws the money at y 8 vs y
// 0x20) is unproven.
i16 SetInfoBarLayout(i16 layout);

// @identity-TODO: What surface 0x48f5e4 holds (mode 1/5 overlay blitted with rect 0x48f5d4) is
// unrecovered.
RVA_DECL(0x00057bf0)
void ClearSceneSurfaces(void);

// @identity-TODO: Same layer-table naming gap as ShowScreenLayer.
RVA_DECL(0x00054390)
void HideScreenLayer(i16 layer);

// Rolls an event from the nearest field object's distance; > 0 when it fires.
b16 RollProximityEvent(void);

// @identity-TODO: event hooks the field-object code triggers when an object
// with an event id is removed (the queue lives at 0x47b7d0; 0x413b20 sets bit
// `level` of 0x47b740).
void MarkLevelEvent(i16 level);

void QueueObjectEvent(i16 event);

i16 HasQueuedObjectEvents(void);

// @identity-TODO: map-cell object lookups (cell code at x/y, whether it holds
// an object, its table, the object kind for a layer).
i16 GetMapCellCode(i16 x, i16 y);

i16 IsObjectCell(i16 code);

// Marks map cell x/y with `kind` on the automap.
void MarkMapCell(i16 kind, i16 x, i16 y);

// @identity-TODO: sets the flag 0x480d10 (instead of removing a hidden object).
void DeferObjectRemoval(void);

// @identity-TODO: a stub that drops a layer image (returns 0).
ub32 DropLayerImage(u32 image);

// @identity-TODO: requests a redraw of the area of object `index`.
RVA_DECL(0x00049fe0)
void RequestObjectRedraw(i16 index, i16 a, i16 b);

#endif // GITEN_GAME_FIELDSUPPORT_H
