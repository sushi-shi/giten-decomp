// The color-packing helper must inline in this span.

#include <rva.h>

#include <Gfx/Bitmap.h>
#include <Platform/GameApi.h>

#include <string.h>

RVA(0x00056f30, 0x1e7)
b32 CopyResourceBitmap16(BitmapResource* bmp, IDirectDrawSurface** surface, i32 x, i32 y) {
    DDSURFACEDESC desc;
    u16 colors[256];
    DDCOLORKEY key;
    RGBQUAD* palette;
    DWORD count;
    DWORD i;
    i32 row;
    i32 col;
    u8* src;
    u8* dst;
    u8* pixels;
    desc.dwSize = sizeof(desc);
    if ((*surface)->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK, NULL)
        != DD_OK) {
        return false;
    }
    memset(colors, 0, sizeof(colors));
    palette = bmp->colors;
    count = bmp->info.biClrUsed;
    if (count == 0) {
        count = 256;
    }
    for (i = 0; i < count; i++) {
        colors[i] = PackSurfaceColor(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
    }
    src = GetResourceBitmapLastRow(bmp);
    dst = static_cast<u8*>(desc.lpSurface) + desc.lPitch * y + x * 2;
    for (row = 0; row < bmp->info.biHeight; row++) {
        pixels = src;
        for (col = 0; col < bmp->info.biWidth * 2; col += 2) {
            WriteSurfacePixel16(dst, col, colors[*pixels]);
            pixels++;
        }
        dst += desc.lPitch;
        src -= bmp->info.biWidth;
    }
    (*surface)->Unlock(NULL);
    memset(&key, 0, sizeof(key));
    (*surface)->SetColorKey(DDCKEY_SRCBLT, &key);
    return true;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00057120, 0x162)
b32 CopyResourceBitmap8(
    BitmapResource* bmp,
    IDirectDrawSurface** surface,
    PALETTEENTRY* entries,
    IDirectDrawPalette** palette,
    i32 x,
    i32 y
) {
    DDSURFACEDESC desc;
    HRESULT result;
    DDCOLORKEY key;
    i32 row;
    u8* src;
    u8* dst;
    // API-forced: RGBQUAD and PALETTEENTRY have different packed channel byte orders.
    ConvertBitmapPalette(reinterpret_cast<DWORD*>(bmp->colors), reinterpret_cast<DWORD*>(entries));
    if (g_ddraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, entries, palette, NULL) != DD_OK) {
        return false;
    }
    result = (*surface)->SetPalette(*palette);
    if (result != DD_OK) {
        return false;
    }
    desc.dwSize = sizeof(desc);
    if ((*surface)->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK, NULL)
        != DD_OK) {
        return false;
    }
    src = GetResourceBitmapLastRow(bmp);
    dst = static_cast<u8*>(desc.lpSurface) + desc.lPitch * y + x;
    for (row = 0; row < bmp->info.biHeight; row++) {
        memcpy(dst, src, bmp->info.biWidth);
        dst += desc.lPitch;
        src -= bmp->info.biWidth;
    }
    (*surface)->Unlock(NULL);
    memset(&key, 0, sizeof(key));
    (*surface)->SetColorKey(DDCKEY_SRCBLT, &key);
    return true;
}

RVA(0x00057290, 0x4c)
b32 BlitImage(LPDIRECTDRAWSURFACE surface, u16 image, i32 x, i32 y) {
    HRSRC resource = FindResource(NULL, MAKEINTRESOURCE(image), RT_BITMAP);
    BitmapResource* bmp;
    if (resource == NULL) {
        return false;
    }
    bmp = static_cast<BitmapResource*>(LockResource(LoadResource(NULL, resource)));
    if (bmp == NULL) {
        return false;
    }
    CopyResourceBitmap16(bmp, &surface, x, y);
    return true;
}
