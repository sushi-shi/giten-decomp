#ifndef GITEN_GAME_FIELDSIGHT_H
#define GITEN_GAME_FIELDSIGHT_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/FieldActor.h>
#include <Game/GameState.h>
#include <Game/ViewDirection.h>
#include <Ints.h>
#include <Math/Vec3.h>

// Callees of the field-object TU's sight grid, spawn timer and encounter
// table (0x40ea50..0x40ff27), label-only until their TUs are claimed.

// The actor and target of the action being resolved (script objects -20 and
// -21): a field object index, or -1 - party position.
extern i16 g_actorId;
extern i16 g_targetId;

// @identity-TODO: pushes game state 0x18 (a prompt/wait window) with five
// parameters; returns 1 when one is already up.
b16 PushPromptState(i16 sub, i16 x, i16 y, i16 z, i16 mode);

static __inline void PushFieldUsePrompt(void) {
    Vec3 position;
    CellToField(0, 0, 4, &position);
    PushPromptState(0, position.x, 0xba, position.z, 0);
}

// @identity-TODO: marks the cells a sight line from x/y reaches along
// `direction` at `step` between the left/right bounds, narrowing them.
void ScanSightRow(
    i16 x,
    i16 y,
    i16 step,
    GZ_ENUM_PARAM(ViewDirection, i16) direction,
    i16* left,
    i16* right
);

void TraceSight(i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction);

// @identity-TODO: the view the field objects are drawn in: the facing and
// the lateral/depth position of the view (depth 0..-4).
extern i16 g_viewFacing;
extern i16 g_viewLateral;
extern i16 g_viewDepth;

// @identity-TODO: a stub (returns 0) that picks a layer frame for depth `z`.
i16 GetLayerImageBand(i16 value);
u32 GetLayerFrame(u32 image, i16 a, i16 z);

i16 DistanceToParty(FieldActor* actor);

// Whether map cell x/y lies inside the current automap viewport.
b16 IsCellInView(i16 x, i16 y);
i16 IsPartyInSight(i16 x, i16 y);

i16 CellCodeDiffers(i16 code, i16 x, i16 y);

// Sets g_selectedHotspot to -1.
RVA_DECL(0x000497c0)
void InvalidateSelectedHotspot(void);

// @identity-TODO: a picture request (data file id and variant, then a flags
// word) as 0x4032d0 reads it; it returns the loaded bytes and their size.
typedef struct ImageRequest {
    i16 file;
    i16 variant;
    i32 flags;
    i32 extra; // @identity-TODO: not set by 0x40e820
} ImageRequest;

RVA_DECL(0x00003330)
void* FreeImageFile(void* data);

// @identity-TODO: allocates a layer's image slots (seven 6-byte cells).
u32 AllocLayerImage(void);

// @identity-TODO: decode a loaded picture into layer `layer`; 0x458040 is
// used for picture base 0xb80, 0x457f20 for the rest.
struct BmpFile;

RVA_DECL(0x00058040)
void DecodeLayerImageAlt(struct BmpFile* data, i16 layer, i32 size);

RVA_DECL(0x00057f20)
b16 DecodeLayerImage(struct BmpFile* data, i16 layer, i32 size);

// @identity-TODO: a helper of the random spawn: the cell code under the party.
i16 GetPartyCellCode(void);

// Defined in the field-object TU after its only caller, DrawFieldObject.
struct FieldObject;
void RefreshObjectDraw(
    i16 sprite,
    i16 x,
    i16 y,
    i16 depth,
    i16 force,
    struct FieldObject* object,
    u32 frame,
    i16 index
);

#endif // GITEN_GAME_FIELDSIGHT_H
