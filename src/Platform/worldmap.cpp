// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Platform/D3DApp.h>
#include <Platform/GameApi.h>
#include <Platform/Scene3D.h>

DATA(0x0006dca0)
static RECT s_cursorPreviewRect = {8, 8, 119, 119};

RVA(0x000581b0, 0x10c)
i16 HitTestWorldMap(i16 x, i16 y, i16 layer) {
    i32 index = LayerIndexAtPoint(x, y);
    u16 color;
    i32 result;
    if (index >= 0 && g_layerStack[index]->slot != SCREEN_LAYER_AUTOMAP) {
        return 0;
    }
    result = 0;
    if (index < 0) {
        if (layer & 1) {
            color = ReadSurfaceWord(g_viewCachePicture.surface, x, y, 640);
        } else {
            color = ReadSurfaceWord(g_scenePicture.surface, x, y, 640);
        }
    } else {
        x -= g_layerStack[index]->x;
        y -= g_layerStack[index]->y;
        result = 1;
        color = ReadSurfaceWord(g_layerStack[index]->canvas, x, y, 128);
    }
    if (color == g_markerColors[0].color) {
        result |= 2;
    } else if (color == g_markerColors[2].color) {
        result |= 2;
    } else if (color == g_markerColors[3].color) {
        result |= 2;
    } else if (color == g_markerColors[1].color) {
        result |= 2;
    } else {
        result = 0;
    }
    return result;
}

// The incoming position is replaced with the current party marker.
RVA(0x000582c0, 0x64)
i16 IsWorldMapMarkerNearEdge(i16 x, i16 y) {
    i16 slot = GetWorldMapMarker(&x, &y);
    if (slot >= 0 && slot < MAP_SCREEN_COUNT) {
        x += g_mapScreenOffsets[slot].x;
        y += g_mapScreenOffsets[slot].y;
        if (x < 16 || x > 624) {
            return 1;
        }
        if (y < 16 || y > 312) {
            return 1;
        }
    }
    return 0;
}

// @identity-TODO: the returned map codes are known by their marker colours.
RVA(0x00058330, 0xbd)
u8 ReadWorldMapTileCode(i16 x, i16 y, i16 slot, i16 layer) {
    u16 color;
    if (slot < 0) {
        return 0;
    }
    if (slot > 5) {
        return 0;
    }
    x += g_mapScreenOffsets[slot].x;
    y += g_mapScreenOffsets[slot].y;
    if (x < 0) {
        x = 0;
    } else if (x >= MAP_MARKER_MAX_X) {
        x = MAP_MARKER_MAX_X;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= MAP_MARKER_MAX_Y) {
        y = MAP_MARKER_MAX_Y;
    }
    if (layer & 1) {
        color = ReadSurfaceWord(g_viewCachePicture.surface, x, y, 640);
    } else {
        color = ReadSurfaceWord(g_scenePicture.surface, x, y, 640);
    }
    if (color == g_markerColors[0].color) {
        return 15;
    }
    if (color == g_markerColors[2].color) {
        return 4;
    }
    if (color == g_markerColors[3].color) {
        return 5;
    }
    if (color == g_markerColors[1].color) {
        return 2;
    }
    return 0;
}

RVA(0x000583f0, 0x10c)
void DrawWorldMapCursor(i16 x, i16 y, i16 layer) {
    RECT source;
    if (LayerIndexAtPoint(x, y) >= 0) {
        return;
    }
    source.left = x - 7;
    source.top = y - 7;
    source.right = source.left + 13;
    source.bottom = source.top + 13;
    if (source.left < 0) {
        source.left = 0;
        source.right = 13;
    } else if (source.right >= 640) {
        source.right = 639;
        source.left = 626;
    }
    if (source.top < 0) {
        source.top = 0;
        source.bottom = 13;
    } else if (source.bottom >= 328) {
        source.bottom = 327;
        source.top = 314;
    }
    g_screenLayers[SCREEN_LAYER_AUTOMAP]
        ->canvas->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
    if (layer & 1) {
        g_screenLayers[SCREEN_LAYER_AUTOMAP]->canvas->Blt(
            &s_cursorPreviewRect,
            g_viewCachePicture.surface,
            &source,
            DDBLT_KEYSRC,
            NULL
        );
    } else {
        g_screenLayers[SCREEN_LAYER_AUTOMAP]->canvas->Blt(
            &s_cursorPreviewRect,
            g_scenePicture.surface,
            &source,
            DDBLT_KEYSRC,
            NULL
        );
    }
}
