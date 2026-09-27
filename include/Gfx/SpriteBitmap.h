#ifndef GITEN_GFX_SPRITEBITMAP_H
#define GITEN_GFX_SPRITEBITMAP_H

#include <Ints.h>

// The legacy sprite image header, followed by width * height words.
typedef struct SpriteBitmap {
    i16 offsetX;
    i16 offsetY;
    i16 width;
    i16 height;
} SpriteBitmap;

i32 CopySpriteBitmap(SpriteBitmap* image);

#endif // GITEN_GFX_SPRITEBITMAP_H
