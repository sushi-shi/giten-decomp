#include <rva.h>

#include <Gfx/Texture.h>
#include <Platform/GameApi.h>

#include <string.h>

RVA(0x00057a70, 0xb1)
BmpFile* OpenTextureBitmap(Texture* texture, const char* name, b32 fromFile) {
    BmpFile* bmp;
    u8* pixels;
    i32 row;
    i32 col;
    if (HasTextureHandle(texture)) {
        return NULL;
    }
    if (fromFile == TRUE) {
        bmp = ReadBitmapFile(name);
        if (bmp == NULL) {
            return NULL;
        }
    } else {
        // API-forced: borrowed BMP input when the argument is not a path.
        bmp = reinterpret_cast<BmpFile*>(const_cast<char*>(name));
    }
    texture->image = static_cast<BmpFile*>(AllocClearedLong(1, bmp->file.bfSize));
    if (texture->image == NULL) {
        return NULL;
    }
    memcpy(texture->image, bmp, bmp->file.bfSize);
    pixels = GetBitmapPixels(bmp);
    texture->bottomMargin = 0;
    for (row = 0; row < bmp->info.biHeight; row++) {
        for (col = 0; col < bmp->info.biWidth; pixels++, col++) {
            if (*pixels != BMP_TRANSPARENT_INDEX) {
                texture->bottomMargin = row;
                row = bmp->info.biHeight;
                break;
            }
        }
    }
    return bmp;
}
