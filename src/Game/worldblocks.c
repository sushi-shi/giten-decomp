// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/FieldHud.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/WorldMap.h>
#include <Gfx/Scene.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/Vram.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Util/WordList.h>

#include <stddef.h>

// The world panel: seven rows (ids 0..6) sharing one handler.
DATA(0x000686a8)
static struct {
    Panel panel;
    PanelRow more[6];
} s_worldPanel = {
    {PANEL_HELD_BUTTON_INPUT, 0, 0, 7, 0, 0, 0, {0}, {{0, 0, 0, WorldRowHandler}}},
    {{0, 1, 0, WorldRowHandler},
     {0, 2, 0, WorldRowHandler},
     {0, 3, 0, WorldRowHandler},
     {0, 4, 0, WorldRowHandler},
     {0, 5, 0, WorldRowHandler},
     {0, 6, 0, WorldRowHandler}},
};

DATA(0x00068708)
static WorldMapBlock s_worldBlocks[6] = {
    {0, -1},
    {0, -1},
    {0, -1},
    {0, -1},
    {0, -1},
    {0, -1},
};

DATA(0x00068730)
i16 g_worldMapOverlayFlags[89] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
};

// @identity-TODO: the image is only freed in this build.
DATA(0x0007b804)
static u32 s_overlayImage;

DATA(0x0007b7d8)
static i16 s_viewY;

DATA(0x0007b7e4)
static i16 s_viewX;

// @identity-TODO: no capture writes this handle in the Windows build.
DATA(0x0007b810)
static i32 s_savedCursor;

RVA(0x00016270, 0xd1)
void LoadWorldMapBlockImage(i16 block, i16 slot) {
    ImageRequest request;
    i16 variant = block;
    BmpFile* image;
    ClearWorldMapBlock(slot);
    if (block < 0 || block >= 88) {
        return;
    }
    request.file = block + 0x7e00;
    if (block == 76 && !IsEventFlagSet(1, 13)) {
        request.file = 0x7e58;
        variant = 88;
    }
    request.variant = 0;
    request.flags = 1;
    image = LoadImageData(&request);
    LoadWorldMapTile(image, slot);
    FreeImageFile(image);
    if (g_worldMapOverlayFlags[variant]) {
        image = LoadImageKind1(&request);
        LoadWorldMapOverlay(image, slot);
        FreeImageFile(image);
    }
    s_worldBlocks[slot].index = block;
}

RVA(0x00016350, 0x66)
void LoadWorldMapBlocks(i16 block) {
    if (s_worldBlocks[0].index != block) {
        ClearSceneSurfaces();
        LoadWorldMapBlockImage(block, 0);
        LoadWorldMapBlockImage(block + 1, 1);
        LoadWorldMapBlockImage(block + 2, 2);
        LoadWorldMapBlockImage(block + 8, 3);
        LoadWorldMapBlockImage(block + 9, 4);
        LoadWorldMapBlockImage(block + 10, 5);
    }
}

RVA(0x000163c0, 0x1e)
void ClearWorldMapBlock(i16 slot) {
    s_worldBlocks[slot].reserved = 0;
    s_worldBlocks[slot].index = -1;
}

RVA(0x000163e0, 0x30)
void ResetWorldMapBlocks(i16 freeOverlay) {
    i16 slot;
    for (slot = 0; slot < 6; slot++) {
        ClearWorldMapBlock(slot);
    }
    if (freeOverlay) {
        s_overlayImage = FreeImageHandle(s_overlayImage);
    }
}

RVA(0x00016410, 0x41)
void SwapWorldMapBlocks(i16 first, i16 second) {
    WorldMapBlock saved = s_worldBlocks[first];
    s_worldBlocks[first] = s_worldBlocks[second];
    s_worldBlocks[second] = saved;
}

// @dead-code
// Zero-ref: no retail call, jump or relocated pointer reaches this helper.
// The Windows scene renderer replaces the legacy block and overlay blits.
RVA(0x00016460, 0x1)
void DrawWorldMapBlock(i16 slot, i16 x, i16 y) {}

RVA(0x00016470, 0x2d)
void ScrollWorldMapView(i16 x, i16 y) {
    s_viewX = x;
    s_viewY = y;
    LoadWorldMapBlocks(GetWorldMapBlock(x, y));
    ShowScenePicture();
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000164a0, 0x4c)
MapCoord GetCenteredWorldMapViewOrigin(i16 x, i16 y) {
    MapCoord origin;
    origin.x = x - 320;
    origin.y = y - 164;
    ClampWorldMapViewOrigin(&origin);
    return origin;
}

RVA(0x000164f0, 0x66)
MapCoord GetWorldMapViewOrigin(i16 x, i16 y) {
    MapCoord origin;
    origin.x = x - 320;
    if (y % 200 > 100) {
        origin.y = y;
    } else {
        origin.y = y - 164;
    }
    ClampWorldMapViewOrigin(&origin);
    return origin;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00016560, 0xe3)
void RotateWorldMapBlocks(i16 x, i16 y) {
    i32 count;
    if (x > 0) {
        count = x;
        do {
            SwapWorldMapBlocks(0, 1);
            SwapWorldMapBlocks(1, 2);
            SwapWorldMapBlocks(3, 4);
            SwapWorldMapBlocks(4, 5);
        } while (--count);
    } else if (x < 0) {
        count = -x;
        do {
            SwapWorldMapBlocks(2, 1);
            SwapWorldMapBlocks(1, 0);
            SwapWorldMapBlocks(5, 4);
            SwapWorldMapBlocks(4, 3);
        } while (--count);
    }
    if (y > 0) {
        count = y;
        do {
            SwapWorldMapBlocks(0, 3);
            SwapWorldMapBlocks(1, 4);
            SwapWorldMapBlocks(2, 5);
        } while (--count);
    } else if (y < 0) {
        count = -y;
        do {
            SwapWorldMapBlocks(3, 0);
            SwapWorldMapBlocks(4, 1);
            SwapWorldMapBlocks(5, 2);
        } while (--count);
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00016650, 0x9c)
MapCoord GetWorldViewBlockDelta(i16 x, i16 y) {
    MapCoord delta = {0, 0};
    i16 current = GetWorldMapBlock(s_viewX, s_viewY);
    i16 next = GetWorldMapBlock(x, y);
    if (current == next) {
        return delta;
    }
    delta.x = next % 8;
    delta.x -= current % 8;
    delta.y = next / 8;
    delta.y -= current / 8;
    return delta;
}

RVA(0x000166f0, 0x1e)
MapCoord GetWorldViewOrigin(void) {
    MapCoord origin;
    origin.x = s_viewX;
    origin.y = s_viewY;
    return origin;
}

// A cached block resolves to its slot; an uncached block keeps its index.
#define ResolveWorldMapBlockSlot(block)                                                            \
    do {                                                                                           \
        i16 slot;                                                                                  \
        for (slot = 0; slot < 6; slot++) {                                                         \
            if (s_worldBlocks[slot].index == (block)) {                                            \
                (block) = slot;                                                                    \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
    } while (0)

RVA(0x00016710, 0x92)
u8 GetWorldMapCellCode(i16 layer, i16 x, i16 y) {
    i16 column;
    i16 row;
    i16 block;
    if (!IsWorldCellInMap(x, y)) {
        return 0;
    }
    column = x / 288;
    row = y / 200;
    block = column + row * 8;
    ResolveWorldMapBlockSlot(block);
    return ReadWorldMapTileCode(x - column * 288, y - row * 200, block, layer);
}

RVA(0x000167b0, 0x14)
void FreeWorldMapScreenSave(void) {
    s_savedCursor = FreeHandle(s_savedCursor);
}

RVA(0x000167d0, 0x5)
void DiscardWorldMapScreenSave(void) {
    FreeWorldMapScreenSave();
}

RVA(0x000167e0, 0x1c)
void RestoreWorldMapCursor(void) {
    RestoreSavedCursor(HandleReadPtr(s_savedCursor));
    FreeWorldMapScreenSave();
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00016800, 0xa)
void RestoreWorldMapScreenSave(void) {
    RestoreWorldMapCursor();
    DiscardWorldMapScreenSave();
}

RVA(0x00016810, 0xa9)
i16 GetWorldMapMarker(i16* x, i16* y) {
    i16 column;
    i16 row;
    i16 block;
    if (!IsWorldCellInMap(g_worldMapX, g_worldMapY)) {
        return -1;
    }
    column = g_worldMapX / 288;
    row = g_worldMapY / 200;
    block = column + row * 8;
    *x = g_worldMapX - column * 288 - 3;
    *y = g_worldMapY - row * 200 - 3;
    ResolveWorldMapBlockSlot(block);
    return block;
}

RVA(0x000168c0, 0x7)
i16 GetWorldBlock(void) {
    return s_worldBlocks[0].index;
}

// Seven one-entry word lists (in a ten-slot array) for a layer image.
RVA(0x000168d0, 0x40)
u32 AllocLayerImage(void) {
    WordList** cells = AllocCleared(0x28, 1);
    WordList** cell = cells;
    i16 i;
    for (i = 0; i < 7; i++) {
        *cell = AllocCleared(6, 1);
        (*cell)->count = 1;
        (*cell)->words = NULL;
        cell++;
    }
    return (u32)cells;
}

// @identity-TODO: stubs of the layer-image interface in this build.
RVA(0x00016910, 0x3)
ub32 DropLayerImage(u32 image) {
    return false;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
// @identity-TODO: a seven-band selector beside the layer-image interface;
// the input's coordinate space is unproven.
RVA(0x00016920, 0x23)
i16 GetLayerImageBand(i16 value) {
    i16 band;
    value -= 281;
    for (band = 0; band < 7; ++band) {
        if (value <= 0) {
            return band;
        }
        value -= 225;
    }
    return 6;
}

RVA(0x00016950, 0x3)
ub32 GetLayerFrame(u32 image, i16 a, i16 z) {
    return false;
}

RVA(0x00016960, 0x25)
void InitWorldPanel(void) {
    s_worldPanel.panel.image = LoadMenuImage(0x118);
    ClearPanelFlags(&s_worldPanel.panel, PANEL_HIDDEN);
}

// The world panel's row handler (drops a pending left click first).
RVA(0x00016990, 0x26)
i16 WorldRowHandler(PanelRow* row, i16 value, i16 op) {
    g_mouseLeftClick = 0;
    ApplyRowCheck(row, value, op);
    return value;
}

RVA(0x000169c0, 0x18)
void ClearFieldPanelSelection(void) {
    if (g_field.moveState == 0) {
        ClearPanelChecks(&s_worldPanel.panel);
    }
}
