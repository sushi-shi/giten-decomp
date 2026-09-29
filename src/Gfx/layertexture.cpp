#include <rva.h>

#include <Gfx/Bitmap.h>
#include <Gfx/Texture.h>
#include <Platform/Com.h>
#include <Platform/GameApi.h>
#include <Platform/Scene3D.h>

RVA(0x00057f20, 0x112)
b16 DecodeLayerImage(BmpFile* data, i16 layer, i32 size) {
    b32 loaded = true;
    BmpFile* bmp = data;
    i32 frame;
    u32 consumed;
    u32 bitmapSize;
    if (data == NULL) {
        return loaded;
    }
    for (frame = 0; frame < 5; frame++) {
        ReleaseTexture(&g_enemyTextures[layer][frame]);
    }
    consumed = 0;
    for (frame = 0; frame < 5; frame++) {
        if (consumed > size) {
            break;
        }
        bitmapSize = bmp->file.bfSize;
        if (!LoadTexture(
                &g_enemyTextures[layer][frame],
                // API-forced: borrowed BMP input, selected by fromFile = FALSE.
                reinterpret_cast<const char*>(bmp),
                false
            )) { // API-forced: borrowed BMP input.
            loaded = false;
        }
        // Byte-forced: packed complete BMP files advance by bfSize.
        bmp = reinterpret_cast<BmpFile*>(
            reinterpret_cast<u8*>(bmp) + bitmapSize
        ); // Byte-forced: packed BMPs.
        consumed += bitmapSize;
    }
    if (!loaded) {
        for (frame = 0; frame < 5; frame++) {
            Texture* texture = &g_enemyTextures[layer][frame];
            ReleaseTextureSurfaces(texture);
        }
    }
    return loaded;
}

RVA(0x00058040, 0xc4)
void DecodeLayerImageAlt(BmpFile* data, i16 layer, i32 size) {
    BmpFile* bmp;
    i32 frame;
    u32 consumed;
    u32 bitmapSize;
    if (g_enemyPictures[0].id != 0) {
        return;
    }
    bmp = data;
    if (bmp == NULL) {
        return;
    }
    for (frame = 0; frame < 5; frame++) {
        ReleaseTexture(&g_enemyTextures[layer][frame]);
    }
    consumed = 0;
    for (frame = 0; frame < 5; frame++) {
        if (consumed > size) {
            break;
        }
        bitmapSize = bmp->file.bfSize;
        OpenTextureBitmap(
            &g_enemyTextures[layer][frame],
            // API-forced: borrowed BMP input, selected by fromFile = FALSE.
            reinterpret_cast<const char*>(bmp),
            false
        ); // API-forced: borrowed BMP input.
        // Byte-forced: packed complete BMP files advance by bfSize.
        bmp = reinterpret_cast<BmpFile*>(
            reinterpret_cast<u8*>(bmp) + bitmapSize
        ); // Byte-forced: packed BMPs.
        consumed += bitmapSize;
    }
    consumed = 0;
    for (frame = 0; frame < 6; frame++) {
        if (consumed > size) {
            break;
        }
        bmp = data;
        bitmapSize = bmp->file.bfSize;
        LoadBitmapToSurface16(bmp, &g_enemyPictures[frame].surface, NULL);
        data = reinterpret_cast<BmpFile*>(reinterpret_cast<u8*>(data) + bitmapSize);
        consumed += bitmapSize;
    }
}
