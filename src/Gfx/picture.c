// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Gfx/DDraw.h>
#include <Gfx/Scene.h>
#include <Gfx/Texture.h>
#include <Platform/GameCalls.h>
#include <Text/TextPlane.h>

#include <string.h>

DATA(0x0008d700)
Picture g_scenePicture;

DATA(0x0006dc10)
static WorldMapBlitRegion s_worldMapRegions[6] = {
    {0, 0, 112, 36, 176, 164},
    {176, 0, 0, 36, 288, 164},
    {464, 0, 0, 36, 176, 164},
    {0, 164, 112, 0, 176, 164},
    {176, 164, 0, 0, 288, 164},
    {464, 164, 0, 0, 176, 164},
};

// Reloads a texture whose device surface was lost.
RVA(0x00057b30, 0x41)
void RestoreTexture(Texture* texture) {
    if (texture != NULL && texture->surface != NULL
        && IDirectDrawSurface_IsLost(texture->surface) == DDERR_SURFACELOST) {
        IDirectDrawSurface_Restore(texture->surface);
        IDirect3DTexture2_Load(texture->texture, texture->source);
    }
}

RVA(0x00057b80, 0xb)
void ShowScenePicture(void) {
    g_scenePicture.visible = TRUE;
}

RVA(0x00057b90, 0x59)
void LoadScenePicture(BmpFile* bmp, u16 id) {
    if (bmp != NULL) {
        LoadBitmapToSurface16(bmp, &g_scenePicture.surface, NULL);
        g_scenePicture.id = id;
        GetBitmapRect(g_scenePicture.rect, bmp);
        g_scenePicture.visible = TRUE;
    }
}

RVA(0x00057bf0, 0x4b)
void ClearSceneSurfaces(void) {
    ClearDisplaySurface(g_scenePicture.surface, NULL);
    ClearDisplaySurface(g_viewCachePicture.surface, NULL);
    GetTextPlane(0)->visible = FALSE;
    g_scenePicture.visible = FALSE;
}

RVA(0x00057c40, 0x30)
void ClearScenePicture(void) {
    ClearDisplaySurface(g_scenePicture.surface, NULL);
    GetTextPlane(0)->visible = FALSE;
    g_scenePicture.visible = FALSE;
}

static __inline void GetWorldMapBlitRects(i16 slot, RECT* dest, RECT* source) {
    dest->left = s_worldMapRegions[slot].destX;
    dest->top = s_worldMapRegions[slot].destY;
    dest->right = s_worldMapRegions[slot].destX + s_worldMapRegions[slot].width;
    dest->bottom = s_worldMapRegions[slot].destY + s_worldMapRegions[slot].height;
    source->left = s_worldMapRegions[slot].sourceX;
    source->top = s_worldMapRegions[slot].sourceY;
    source->right = s_worldMapRegions[slot].sourceX + s_worldMapRegions[slot].width;
    source->bottom = s_worldMapRegions[slot].sourceY + s_worldMapRegions[slot].height;
}

RVA(0x00057c70, 0x106)
void LoadWorldMapTile(BmpFile* bmp, i16 slot) {
    if (bmp != NULL && slot <= 5) {
        Picture tile;
        RECT source;
        RECT dest;
        memset(&tile, 0, sizeof(tile));
        CreatePicture(
            &tile,
            bmp->info.biWidth,
            bmp->info.biHeight,
            bmp->info.biWidth,
            bmp->info.biHeight,
            TRUE
        );
        LoadBitmapToSurface16(bmp, &tile.surface, NULL);
        GetWorldMapBlitRects(slot, &dest, &source);
        IDirectDrawSurface_Blt(
            g_scenePicture.surface,
            &dest,
            tile.surface,
            &source,
            DDBLT_WAIT,
            NULL
        );
        g_scenePicture.rect.left = 0;
        g_scenePicture.rect.top = 0;
        g_scenePicture.rect.right = 640;
        g_scenePicture.rect.bottom = 328;
        g_scenePicture.visible = FALSE;
    }
}

RVA(0x00057d80, 0xfc)
void LoadWorldMapOverlay(BmpFile* bmp, i16 slot) {
    if (bmp != NULL && slot <= 5) {
        Picture tile;
        RECT source;
        RECT dest;
        memset(&tile, 0, sizeof(tile));
        CreatePicture(
            &tile,
            bmp->info.biWidth,
            bmp->info.biHeight,
            bmp->info.biWidth,
            bmp->info.biHeight,
            TRUE
        );
        LoadBitmapToSurface16(bmp, &tile.surface, NULL);
        GetWorldMapBlitRects(slot, &dest, &source);
        IDirectDrawSurface_Blt(
            g_viewCachePicture.surface,
            &dest,
            tile.surface,
            &source,
            DDBLT_WAIT | DDBLT_KEYSRC,
            NULL
        );
        g_viewCachePicture.rect.left = 0;
        g_viewCachePicture.rect.top = 0;
        g_viewCachePicture.rect.right = 640;
        g_viewCachePicture.rect.bottom = 328;
    }
}
