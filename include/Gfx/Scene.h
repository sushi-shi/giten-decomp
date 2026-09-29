#ifndef GITEN_GFX_SCENE_H
#define GITEN_GFX_SCENE_H

#include <rva.h>

#include <Gfx/Bitmap.h>
#include <Gfx/Picture.h>
#include <Gfx/ScreenLayer.h>

// The full-screen scene picture shown by the event scripts.
extern Picture g_scenePicture;

// The last rendered 3D view (640x328), which the view mode redraws from.
extern Picture g_viewCachePicture;

void ShowScenePicture(void);
void LoadScenePicture(BmpFile* bmp, u16 id);
void ClearScenePicture(void);

#define LoadRequestedScenePicture(request, mode)                                                   \
    do {                                                                                           \
        void* imageData = LoadImageRequest(&(request), (mode));                                    \
        LoadScenePicture(imageData, (request).file);                                               \
        FreeImageFile(imageData);                                                                  \
    } while (0)

typedef struct WorldMapBlitRegion {
    i32 destX;
    i32 destY;
    i32 sourceX;
    i32 sourceY;
    i32 width;
    i32 height;
} WorldMapBlitRegion;

void LoadWorldMapTile(BmpFile* bmp, i16 slot);
void LoadWorldMapOverlay(BmpFile* bmp, i16 slot);
u8 ReadWorldMapTileCode(i16 x, i16 y, i16 slot, i16 layer);
// Zero for no marker colour, two on the world map, three on the automap.
i16 HitTestWorldMap(i16 x, i16 y, i16 layer);
b16 IsWorldMapMarkerNearEdge(i16 x, i16 y);
// Magnifies the map around the mouse into the automap canvas.
void DrawWorldMapCursor(i16 x, i16 y, i16 layer);

#endif // GITEN_GFX_SCENE_H
