#ifndef GITEN_PLATFORM_GAMECALLS_H
#define GITEN_PLATFORM_GAMECALLS_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/Field.h>
#include <Game/GameLoop.h>
#include <Game/GameStateId.h>
#include <Game/PartyAction.h>
#include <Game/ViewDirection.h>
#include <Ints.h>

// Game functions and data the platform layer uses that their owners' headers
// do not declare yet; kept here because adding them there shifts the TU
// state of the C units including those headers (docs/patterns/
// tu-state-probe-family-decides-reachability.md).

// Set when DirectSound opened (InitDirectX); sound effects and music play
// only then.
extern b32 g_soundEnabled;

// Draws the map overlay at the party's position.
void DrawFieldView(void);

i16 GetFieldBattleActive(void);

// Codegen constraint: also declared in <Gfx/Sprite.h>, its owner; kept here
// for the platform layer's TU state.
RVA_DECL(0x000586d0)
void UnplaceAllSprites(void);

// Releases the 16 image surfaces of sprite slot `slot`.
RVA_DECL(0x00058820)
void FreeSpriteImages(i16 slot);

// Completes the party's step through a door once it has opened.
// @identity-TODO: read from its only caller (AnimateDoor) and its g_party.field use.
void FinishDoorStep(void);

// The current area's NPCs.
i16 GetAreaNpcCount(void);

// Whether `npc` has left (its event flag is set).
b32 IsAreaNpcGone(i16 npc);

// The cell (x, y) of `npc`.
i16* GetAreaNpcCell(i16 npc);

// The object-texture slot of `npc` (0..5).
i16 GetAreaNpcTextureSlot(i16 npc);

struct AreaNpc;
struct AreaNpc* GetAreaNpc(i16 npc);

// From Game/StateStack.h and Game/FieldHud.h, declared here instead: including
// those headers shifts winmain's TU state (AllocCleared, AllocClearedLong).
i16 NextGamePhase(void);
GZ_ENUM_RETURN(GameStateId, i16) GetGameState(void);
u16 GetGamePhase(void);
void RedrawFieldAt(i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction);

// The party's world-map marker: which of the six map screens shows it (-1
// off the map) and its point on that screen.
// If no loaded screen contains it, returns the calculated map block index.
i16 GetWorldMapMarker(i16* x, i16* y);

// Nonzero on odd world-map layers (s_mapLayer & 1), where the map overlay
// draws under the party marker.
// @identity-TODO: what the layer bit means is unrecovered.
i16 IsOddMapLayer(void);

// Whether one of the 16 live objects is within reach of the party.
b32 AnyObjectInReach(void);

// Draws visible field messages in the left, center and right bands.
void UpdateFieldHud(i16 x, i16 y, i16 direction);

// The palette modes of the party's cell and of the cell ahead (vram.c).
i16 GetAreaPaletteMode(void);
i16 GetViewPaletteMode(void);

// The picture helpers of the texture TU: create a picture's surface (keyed
// when `colorKey` is set), load a .bmp into one, draw a bitmap resource onto
// a surface.
// @identity-TODO: label-only until 0x457380, 0x457520 and 0x4572e0 are
// reconstructed.
struct Picture;
struct Texture;
struct IDirectDrawSurface;
RVA_DECL(0x00057380)
b32 CreatePicture(
    struct Picture* picture,
    i32 width,
    i32 height,
    i32 surfaceWidth,
    i32 surfaceHeight,
    b32 colorKey
);
RVA_DECL(0x00057520)
b32 LoadPictureFile(struct Picture* picture, const char* path);
RVA_DECL(0x000572e0)
b32 DrawResourceBitmap(struct IDirectDrawSurface* surface, u16 bitmap);
b32 LoadTexture(struct Texture* texture, const char* name, b32 fromFile);

// Releases a texture's surfaces, palette and textures.
// @identity-TODO: label-only (0x4576d0, the texture TU).
RVA_DECL(0x000576d0)
void ReleaseTexture(struct Texture* texture);

// Frees text plane `plane` (returns -1), and the font's glyph scratch
// surface (font.cpp).
i16 FreeTextPlane(u16 plane);
void FreeGlyphSurface(void);

void RedrawPartyStatus(void);

// Selects the nearest opaque hotspot under a view click.
RVA_DECL(0x00059180)
b32 ClickHotspotAt(i32 x, i32 y);

// Copies a square through GDI and uses its first pixel as the source colour key.
struct IDirectDrawSurface;
RVA_DECL(0x000590a0)
void CopySurfaceSquare(
    struct IDirectDrawSurface* dest,
    struct IDirectDrawSurface* source,
    i32 size
);

// From Mem/Handle.h.
void ClearHandleTable(void);

// The font's 1x1 glyph scratch surface, and text drawn onto a layer's canvas
// (font.cpp).
b32 CreateGlyphSurface(void);
void DrawLayerText(i16 layer, i16 x, i16 y, const char* text, i32 attr);

// The colours of the four world-map marker bitmaps, read back from the marker
// picture.
// @identity-TODO: read by the automap code (0x458258); 16-bit colours in
// dword slots.
struct MarkerColor {
    u16 color;
    u16 reserved;
};
extern struct MarkerColor g_markerColors[4];

// Open the field menu and the automap (fieldscreen).
void OpenFieldMenu(void);
void OpenAutomap(void);

// The current map's wall words, one per cell (NULL without a map).
// @identity-TODO: read from its body only (the pointer at +4 of 0x47fe58).
u16* GetWallMap(void);

// The palette switches of the party's cell and of the cell ahead (fieldscreen).
void UpdateAreaPalette(void);
void UpdateViewPalette(void);

// Tries to step the party one cell in `direction` (STEP_FORWARD..STEP_LEFT,
// relative to its facing): 0 when blocked (the bump sound plays), else
// STEP_WALK, or 0x10 when the cell's wall reports a door.
// @identity-TODO: read from its body only; the field TU owns it.
i16 StepParty(i16 direction);

// Completes a step the camera has slid through: advances the clock, moves the
// party's cell and marks it on the automap.
// @identity-TODO: read from its body only; the field TU owns it.
void CommitPartyStep(void);

// Faces the party toward `direction` (VIEW_NORTH..VIEW_WEST). The body reads
// a word; the platform layer's prototype takes an int (0x44acec pushes the
// dword g_viewDirection).
// @identity-TODO: its two callees (0x412ca0, 0x412cf0) are undecoded.
void SetPartyDirection(i32 direction);

#endif // GITEN_PLATFORM_GAMECALLS_H
