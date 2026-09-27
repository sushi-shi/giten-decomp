// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/AbortFlag.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Gfx/Render.h>
#include <Gfx/ScreenLayer.h>
#include <Ui/Hotspot.h>

#include <stdio.h>

DATA(0x00088010)
Hotspot g_hotspots[64];

DATA(0x0008fb04)
u32 g_hotspotCount;

DATA(0x0006b4e0)
i32 g_selectedHotspot = -1;

RVA(0x00058580, 0xb)
void ClearSelectedHotspot(void) {
    g_selectedHotspot = -1;
}

RVA(0x00058590, 0xa)
void SetSelectedHotspot(i32 index) {
    g_selectedHotspot = index;
}

RVA(0x000585a0, 0x1a)
i16 GetSelectedHotspotValue(void) {
    if (g_selectedHotspot < 0) {
        return -1;
    }
    return GetHotspot(g_selectedHotspot)->value;
}

RVA(0x000585c0, 0x2a)
i16 GetSelectedHotspotObject(void) {
    if (g_selectedHotspot < 0) {
        return -1;
    }
    if (g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
        return -1;
    }
    return GetHotspot(g_selectedHotspot)->value;
}

// Whether a kind-1 hotspot leads to (x, y).
// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x000585f0, 0x48)
i16 HasHotspotTo(i16 x, i16 y) {
    u32 i;
    for (i = 0; i < g_hotspotCount; i++) {
        if (GetHotspot(i)->kind == 1 && GetHotspot(i)->targetX == x
            && GetHotspot(i)->targetY == y) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00058640, 0x90)
i16 CountFieldObjects(void) {
    char buffer[128];
    i16 count = IsAbortPending();
    if (count) {
        SetAbortPending(0);
        count = -1;
    } else if (g_renderMode == RENDER_MODE_VIEW) {
        u32 i;
        sprintf(buffer, "\nobjcnt = %d\n", g_hotspotCount);
        OutputDebugString(buffer);
        for (i = 0; i < g_hotspotCount; i++) {
            if (GetHotspot(i)->kind == HOTSPOT_TARGET) {
                count++;
            }
        }
    } else {
        count = CountActiveObjects();
    }
    return count;
}
