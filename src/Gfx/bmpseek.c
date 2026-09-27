// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Win32.h>

#include <Gfx/Bitmap.h>
#include <Gfx/DDraw.h>

#include <stdio.h>
#include <string.h>

// Positions `fp` at the start of bitmap `skip` in a file of concatenated BMPs
// (`limit` bytes long) and reports that bitmap's size; NULL when the chain is
// broken.
RVA(0x00056b00, 0xc1)
FILE* SeekBitmap(FILE* fp, i16 skip, i32 limit, u32* size) {
    BITMAPFILEHEADER header;
    if (fread(&header, sizeof(header), 1, fp) < 1) {
        return NULL;
    }
    if (!HasBitmapFileSignature(&header)) {
        return NULL;
    }
    while (skip > 0) {
        if (fseek(fp, header.bfSize - sizeof(header), SEEK_CUR) != 0) {
            return NULL;
        }
        limit -= header.bfSize;
        if (limit <= 0) {
            return NULL;
        }
        if (fread(&header, sizeof(header), 1, fp) < 1) {
            return NULL;
        }
        if (!HasBitmapFileSignature(&header)) {
            return NULL;
        }
        skip--;
    }
    if (fseek(fp, -(i32)sizeof(header), SEEK_CUR) != 0) {
        return NULL;
    }
    *size = header.bfSize;
    return fp;
}

// Converts the 8-bit bitmap into the locked 16-bit surface through the
// surface pixel format; palette entry 1 becomes the source colour key.
RVA(0x00056bd0, 0x20a)
b32 LoadBitmapToSurface16(BmpFile* bmp, IDirectDrawSurface** surface, DWORD* colorKey) {
    DDSURFACEDESC desc;
    u16 colors[256];
    DDCOLORKEY key;
    u8* src;
    u8* dst;
    u8* p;
    RGBQUAD* palette;
    DWORD count;
    DWORD i;
    i32 row;
    i32 col;
    DWORD value;

    desc.dwSize = sizeof(desc);
    if (IDirectDrawSurface_Lock(
            *surface,
            NULL,
            &desc,
            DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK,
            NULL
        )
        != DD_OK) {
        return FALSE;
    }
    dst = desc.lpSurface;
    src = GetBitmapPixels(bmp) + (bmp->info.biHeight - 1) * bmp->info.biWidth;
    memset(colors, 0, sizeof(colors));
    palette = bmp->colors;
    count = bmp->info.biClrUsed ? bmp->info.biClrUsed : 256;
    for (i = 0; i < count; i++) {
        colors[i] = PackSurfaceColor(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
    }
    for (row = 0; row < bmp->info.biHeight; row++) {
        p = src;
        for (col = 0; col < bmp->info.biWidth * 2; col += 2) {
            WriteSurfacePixel16(dst, col, colors[*p]);
            p++;
        }
        dst += desc.lPitch;
        src -= bmp->info.biWidth;
    }
    IDirectDrawSurface_Unlock(*surface, NULL);
    // The surface key uses RRGGBB rather than COLORREF channel order.
    value =
        RGB(palette[BMP_TRANSPARENT_INDEX].rgbBlue & 0x1f,
            palette[BMP_TRANSPARENT_INDEX].rgbGreen & 0x1f,
            palette[BMP_TRANSPARENT_INDEX].rgbRed & 0x1f);
    key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = value;
    IDirectDrawSurface_SetColorKey(*surface, DDCKEY_SRCBLT, &key);
    if (colorKey != NULL) {
        *colorKey = value;
    }
    return TRUE;
}

// Loads the 8-bit bitmap into the surface with its own palette; colour 1 is
// the source colour key.
RVA(0x00056de0, 0x141)
b32 LoadBitmapToSurface8(
    BmpFile* bmp,
    IDirectDrawSurface** surface,
    PALETTEENTRY* entries,
    IDirectDrawPalette** palette
) {
    DDSURFACEDESC desc;
    HRESULT result;
    DDCOLORKEY key;
    u8* src;
    u8* dst;
    DWORD row;
    ConvertBitmapPalette((DWORD*)bmp->colors, (DWORD*)entries);
    if (IDirectDraw_CreatePalette(g_ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256, entries, palette, NULL)
        != DD_OK) {
        return FALSE;
    }
    result = IDirectDrawSurface_SetPalette(*surface, *palette);
    if (result != DD_OK) {
        return FALSE;
    }
    desc.dwSize = sizeof(desc);
    if (IDirectDrawSurface_Lock(
            *surface,
            NULL,
            &desc,
            DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK,
            NULL
        )
        != DD_OK) {
        return FALSE;
    }
    dst = desc.lpSurface;
    src = GetBitmapPixels(bmp) + (desc.dwHeight - 1) * desc.dwWidth;
    for (row = 0; row < desc.dwHeight; row++) {
        memcpy(dst, src, desc.dwWidth);
        dst += desc.lPitch;
        src -= desc.dwWidth;
    }
    IDirectDrawSurface_Unlock(*surface, NULL);
    key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = BMP_TRANSPARENT_INDEX;
    IDirectDrawSurface_SetColorKey(*surface, DDCKEY_SRCBLT, &key);
    return TRUE;
}
