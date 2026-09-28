#ifndef GITEN_GFX_BITMAP_H
#define GITEN_GFX_BITMAP_H

#include <Win32.h>

#include <Enums.h>

#include <stdio.h>

// clang-format off
GZ_ENUM_BEGIN(BitmapPaletteIndex)
    BMP_TRANSPARENT_INDEX = 1
GZ_ENUM_END(BitmapPaletteIndex);

GZ_ENUM_BEGIN(BitmapFileSignature)
    BMP_FILE_SIGNATURE = 0x4d42
GZ_ENUM_END(BitmapFileSignature);
// clang-format on

#define HasBitmapFileSignature(header) ((header)->bfType == BMP_FILE_SIGNATURE)

// An 8-bit BMP file image held in memory: headers, a variable palette, then the
// bottom-up pixel rows at file.bfOffBits.
typedef struct BmpFile {
    BITMAPFILEHEADER file;
    BITMAPINFOHEADER info;
    RGBQUAD colors[];
} BmpFile;

#define GetBitmapRect(rect, bmp)                                                                   \
    do {                                                                                           \
        (rect).left = 0;                                                                           \
        (rect).top = 0;                                                                            \
        (rect).right = (bmp)->info.biWidth;                                                        \
        (rect).bottom = (bmp)->info.biHeight;                                                      \
    } while (0)

// Byte-forced: the BMP file header stores the pixel payload offset.
#ifdef __cplusplus
#define GetBitmapPixels(bmp) (reinterpret_cast<u8*>(bmp) + (bmp)->file.bfOffBits)
#else
#define GetBitmapPixels(bmp) ((u8*)(bmp) + (bmp)->file.bfOffBits)
#endif

// Byte-forced: consecutive complete BMP records advance by their file size.
#ifdef __cplusplus
#define GetNextBitmap(bmp)                                                                         \
    reinterpret_cast<BmpFile*>(reinterpret_cast<u8*>(bmp) + (bmp)->file.bfSize)
#else
#define GetNextBitmap(bmp) ((BmpFile*)((u8*)(bmp) + (bmp)->file.bfSize))
#endif

// An 8-bit bitmap resource: DIB header and palette followed by pixel rows.
typedef struct BitmapResource {
    BITMAPINFOHEADER info;
    RGBQUAD colors[256];
    u8 pixels[];
} BitmapResource;

#define GetResourceBitmapLastRow(bmp)                                                              \
    ((bmp)->pixels + ((bmp)->info.biHeight - 1) * (bmp)->info.biWidth)

// Convert a packed BMP RGBQUAD to a reserved Win32 PALETTEENTRY.
#define BMP_PALETTE_ENTRY(rgb)                                                                     \
    (RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)) | (PC_RESERVED << 24))

#define ConvertBitmapPalette(source, destination)                                                  \
    do {                                                                                           \
        DWORD* rgb = (source);                                                                     \
        DWORD* pal = (destination);                                                                \
        i32 color;                                                                                 \
        for (color = 0; color < 256; color++) {                                                    \
            pal[color] = BMP_PALETTE_ENTRY(rgb[color]);                                            \
        }                                                                                          \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

    b32 CopyResourceBitmap16(BitmapResource* bmp, IDirectDrawSurface** surface, i32 x, i32 y);
    b32 BlitImage(LPDIRECTDRAWSURFACE surface, u16 image, i32 x, i32 y);
    b32 CopyResourceBitmap8(
        BitmapResource* bmp,
        IDirectDrawSurface** surface,
        PALETTEENTRY* entries,
        IDirectDrawPalette** palette,
        i32 x,
        i32 y
    );

    BmpFile* ReadBitmapFile(const char* path);
    void FreeBitmap(BmpFile** bmp);

    b32 LoadBitmapToSurface16(BmpFile* bmp, IDirectDrawSurface** surface, DWORD* colorKey);
    b32 LoadBitmapToSurface8(
        BmpFile* bmp,
        IDirectDrawSurface** surface,
        PALETTEENTRY* entries,
        IDirectDrawPalette** palette
    );

    // Seeks `fp` past `skip` bitmaps (of the `limit` bytes in the file) and
    // stores the next one's size; NULL when there is none.
    // @identity-TODO: label-only here; its body is bmpseek's.
    FILE* SeekBitmap(FILE* fp, i16 skip, i32 limit, u32* size);

#ifdef __cplusplus
}
#endif

#endif // GITEN_GFX_BITMAP_H
