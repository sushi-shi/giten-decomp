// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Gfx/ScreenLayer.h>

// Converts a screen point to an automap cell offset from the map centre
// (8-pixel cells, 14 x 14 cells).
RVA(0x00058500, 0x7c)
void ScreenToAutomapCell(i16* x, i16* y) {
    *x += -8 - g_screenLayers[SCREEN_LAYER_AUTOMAP]->x;
    *y += -8 - g_screenLayers[SCREEN_LAYER_AUTOMAP]->y;
    if (*x < 0) {
        *x = 0;
    }
    if (*x >= 0x70) {
        *x = 0x6f;
    }
    if (*y < 0) {
        *y = 0;
    }
    if (*y >= 0x70) {
        *y = 0x6f;
    }
    *x = *x / 8 - 7;
    *y = *y / 8 - 7;
}
