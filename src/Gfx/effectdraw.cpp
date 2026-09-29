#include <rva.h>

#include <Gfx/Bitmap.h>
#include <Gfx/DDraw.h>
#include <Platform/GameApi.h>
#include <Platform/Scene3D.h>

#include <string.h>

DATA(0x00090bfc)
static EffectImageCode s_effectImage;
DATA(0x00090c00)
static i16 s_effectShotX;
DATA(0x00090c04)
static i16 s_effectShotY;
DATA(0x00090c08)
static i16 s_effectShotZ;

#define CacheEffectFrame(bitmap, imageCode)                                                        \
    do {                                                                                           \
        if (EffectImagesDiffer(s_effectImage, imageCode)) {                                        \
            LoadBitmapToSurface16(bitmap, &g_effectFramePicture.surface, NULL);                    \
            s_effectImage = (imageCode);                                                           \
        }                                                                                          \
    } while (0)

RVA(0x00058990, 0x4c)
void ClearEffectLayer(i16 unused) {
    ClearDisplaySurface(g_backdropPicture.surface, NULL);
    GetShotPosition(&s_effectShotX, &s_effectShotY, &s_effectShotZ);
    s_effectImage.frame = 0xfff;
}

DATA(0x0006dcb0)
static i16 s_effectLateralOffsets[4] = {60, 0, 0, 0};
DATA(0x0006dcb8)
static i16 s_effectHeightOffsets[4] = {0, 80, 56, 55};

// @early-stop register allocation: retail splits the size word into dl and bl,
// which frees ebx and reloads `code` for the mirror tests; here the low byte is
// masked from a word copy and `code` stays in ebx through the offsets.
RVA(0x000589e0, 0x457)
void DrawProjectedEffectSprite(EffectImageCode code, i16 x, i16 y) {
    i16 screenX;
    i16 screenY;
    i16 frame = ProjectEffectFrame(x, y, &screenX, &screenY);
    i32 size;
    BmpFile* bmp = GetEffectFrame(&size, frame);
    u16 i;
    i16 left;
    i16 right;
    i16 top;
    i16 bottom;
    i32 width;
    i32 height;
    u16 dimensions;
    RECT dest;
    RECT source;
    float scaleX;
    float scaleY;
    DDBLTFX fx;
    DDSURFACEDESC desc;
    DDCOLORKEY key;
    for (i = 0; i < code.frame; i++) {
        if (!HasBitmapFileSignature(&bmp->file)) {
            return;
        }
        u32 imageSize = bmp->file.bfSize;
        if (size <= imageSize) {
            break;
        }
        bmp = GetNextBitmap(bmp);
        size -= imageSize;
    }
    if (!HasBitmapFileSignature(&bmp->file)) {
        return;
    }
    CacheEffectFrame(bmp, code);
    left = GetEffectBitmapOffsetX(bmp) * 8;
    top = GetEffectBitmapOffsetY(bmp) * 8;
    dimensions = bmp->file.bfReserved2;
    width = LOBYTE(dimensions) * 8;
    height = HIBYTE(dimensions) * 8;
    if (code.mirrorHorizontal) {
        left = -1 - left;
        right = left - width + 1;
    } else {
        right = left + width - 1;
    }
    if (code.mirrorVertical) {
        top = -1 - top;
        bottom = top - height + 1;
    } else {
        bottom = top + height - 1;
    }
    left += screenX;
    right += screenX;
    top += screenY;
    bottom += screenY;
    SortShortPair(&left, &right);
    SortShortPair(&top, &bottom);
    if (screenX & 7) {
        right++;
    }
    if (left < 0) {
        left = 0;
    }
    if (right > 639) {
        right = 639;
    }
    if (top < 0) {
        top = 0;
    }
    if (bottom > 327) {
        bottom = 327;
    }
    if (left > right || top > bottom) {
        return;
    }
    if (static_cast<i16>(GetEffectImageBase()) == EFFECT_IMAGE_LATERAL_ADJUST && code.frame == 0) {
        dest.left = screenX < 320 ? left + s_effectLateralOffsets[frame / 2]
                                  : left - s_effectLateralOffsets[frame / 2];
        dest.right = screenX < 320 ? right + s_effectLateralOffsets[frame / 2]
                                   : right - s_effectLateralOffsets[frame / 2];
    } else {
        dest.left = left;
        dest.right = right;
    }
    dest.top = top + s_effectHeightOffsets[GetShotPower()];
    dest.bottom = dest.top + bottom - top;
    source.left = source.top = 0;
    source.right = bmp->info.biWidth;
    source.bottom = bmp->info.biHeight;
    scaleX = static_cast<float>(dest.right - dest.left) / bmp->info.biWidth;
    scaleY = static_cast<float>(dest.bottom - dest.top) / bmp->info.biHeight;
    if (dest.left < 0) {
        source.left = -dest.left / scaleX;
        dest.left = 0;
    }
    if (dest.top < 0) {
        source.top = -dest.top / scaleY;
        dest.top = 0;
    }
    if (dest.right > 640) {
        source.right = (dest.right - 640) / scaleX;
        source.right = bmp->info.biWidth - source.right;
        dest.right = 640;
    }
    if (dest.bottom > 328) {
        if (code.mirrorVertical) {
            source.top = (dest.bottom - 328) / scaleY;
        } else {
            source.bottom = bmp->info.biHeight - static_cast<i32>((dest.bottom - 328) / scaleY);
        }
        dest.bottom = 328;
    }
    InitEffectBlitFx(fx, code);
    g_backdropPicture.surface
        ->Blt(&dest, g_effectFramePicture.surface, &source, DDBLT_DDFX | DDBLT_KEYSRC, &fx);
    key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = 0;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_ALL;
    if (g_backdropPicture.surface->GetSurfaceDesc(&desc) == DD_OK && IsPalettizedSurface(desc)) {
        key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = BMP_TRANSPARENT_INDEX;
    }
    g_backdropPicture.surface->SetColorKey(DDCKEY_SRCBLT, &key);
}

// @early-stop register allocation: retail extracts the vertical offset byte
// before halving the height, which keeps the left edge in edi and the right
// edge in ecx; every ordering of that sum emits the halving first here.
RVA(0x00058e40, 0x252)
void DrawScreenEffectSprite(BmpFile* imageData, EffectImageCode code, i16 x, i16 y) {
    BmpFile* bmp = imageData;
    u16 i;
    i16 screenX;
    i16 screenY;
    RECT dest;
    RECT source;
    DDBLTFX fx;
    DDSURFACEDESC desc;
    DDCOLORKEY key;
    for (i = 0; i < code.frame; i++) {
        if (!HasBitmapFileSignature(&bmp->file)) {
            return;
        }
        bmp = GetNextBitmap(bmp);
    }
    if (!HasBitmapFileSignature(&bmp->file)) {
        return;
    }
    CacheEffectFrame(bmp, code);
    GetScriptAnimationPosition(x, y, &screenX, &screenY);
    source.left = source.top = 0;
    source.right = bmp->info.biWidth;
    source.bottom = bmp->info.biHeight;
    dest.left = screenX + GetEffectBitmapOffsetX(bmp) * 8;
    dest.top = (screenY + GetEffectBitmapOffsetY(bmp) - bmp->info.biHeight / 2) * 11 / 10;
    dest.right = dest.left + bmp->info.biWidth;
    dest.bottom = dest.top + bmp->info.biHeight * 11 / 10;
    if (dest.left < 0) {
        source.left = -dest.left;
        dest.left = 0;
    }
    if (dest.top < 0) {
        dest.bottom -= dest.top;
        dest.top = 0;
    }
    if (dest.right > 640) {
        source.right = bmp->info.biWidth - dest.right + 640;
        dest.right = 640;
    }
    if (dest.bottom > 480) {
        source.bottom = (bmp->info.biHeight - dest.bottom + 480) * 10 / 11;
        dest.bottom = 480;
    }
    InitEffectBlitFx(fx, code);
    g_backdropPicture.surface
        ->Blt(&dest, g_effectFramePicture.surface, &source, DDBLT_DDFX | DDBLT_KEYSRC, &fx);
    key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = 0;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_ALL;
    if (g_backdropPicture.surface->GetSurfaceDesc(&desc) == DD_OK && IsPalettizedSurface(desc)) {
        key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = BMP_TRANSPARENT_INDEX;
    }
    g_backdropPicture.surface->SetColorKey(DDCKEY_SRCBLT, &key);
}
