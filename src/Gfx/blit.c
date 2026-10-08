// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Gfx/Blit.h>
#include <Mem/Handle.h>

#include <string.h>

// @identity-TODO: the original name is unknown; no Windows reader survives.
DATA(0x00068150)
static i16 s_quadCellSprite[] = {0, 0, 2, 2, 1, 2, 3, 4};

// @identity-TODO: the original name and Windows reader are unknown.
// Windows retains four DWORD PC-98 graphics plane segments.
DATA(0x00068160)
static u32 s_vramPlaneSegments[] = {0xa800, 0xb000, 0xb800, 0xe000};

// @identity-TODO: only the whole legacy image header is copied on Windows;
// no interpreted reader of these bytes survives in this build.
DATA(0x00071190)
static u8 s_blitImageHeader[0x110];

RVA(0x00001620, 0x20)
void PrepareSceneSprite(
    SpriteBitmap* image,
    i32 imageHandle,
    i16 x,
    i16 y,
    i16 originX,
    i16 originY,
    i16 clipLeft,
    i16 clipTop,
    i16 clipRight,
    i16 clipBottom,
    u32 flags
) {
    memcpy(s_blitImageHeader, HandleReadPtr(imageHandle), sizeof(s_blitImageHeader));
}

RVA(0x00001640, 0x31)
void PrepareFullSceneSprite(SpriteBitmap* image, i32 imageHandle, i16 x, i16 y, u32 flags) {
    PrepareSceneSprite(image, imageHandle, x, y, 0, 0, 0, 0, 79, 399, flags);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x00001680, 0x43)
void PrepareOffsetSceneSprite(SpriteBitmap* image, i32 imageHandle, i16 x, i16 y, u32 flags) {
    PrepareFullSceneSprite(
        image,
        imageHandle,
        x + GetSceneSpriteOffsetX(image, flags),
        y + GetSceneSpriteOffsetY(image),
        flags
    );
}

// @identity-TODO: two empty sprite hooks have no recovered signatures or API names.
// @dead-code
// Zero-ref: no effective rel32 caller or relocated pointer reaches this body.
RVA(0x000016d0, 0x1)
void SkipOffsetSceneSprite(void) {}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller or relocated pointer reaches this body.
RVA(0x000016e0, 0x1)
void SkipViewportSceneSprite(void) {}

// @dead-code
// Zero-ref: no retail call, jump or relocated pointer reaches this wrapper.
RVA(0x000016f0, 0x65)
void PrepareViewportSceneSprite(
    SpriteBitmap* image,
    i32 imageHandle,
    i16 x,
    i16 y,
    u32 flags,
    VideoViewport* viewport
) {
    PrepareSceneSprite(
        image,
        imageHandle,
        x + GetSceneSpriteOffsetX(image, flags),
        y + GetSceneSpriteOffsetY(image),
        viewport->screenLeft,
        viewport->screenTop,
        viewport->clipLeft,
        viewport->clipTop,
        viewport->clipRight,
        viewport->clipBottom,
        flags
    );
}

RVA(0x00001760, 0x40)
void BlitSceneSprite(
    SpriteBitmap* image,
    i32 imageHandle,
    i16 x,
    i16 y,
    i16 originX,
    i16 originY,
    i16 clipLeft,
    i16 clipTop,
    i16 clipRight,
    i16 clipBottom,
    u32 flags
) {
    PrepareSceneSprite(
        image,
        imageHandle,
        x,
        y,
        originX,
        originY,
        clipLeft,
        clipTop,
        clipRight,
        clipBottom,
        flags
    );
}

// @identity-TODO: an empty hook amid the legacy sprite blitters.
// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
RVA(0x000017a0, 0x1)
void SceneBlitNop(void) {}

RVA(0x000017b0, 0x1)
void BlitObjectSprite(
    SpriteBitmap* image,
    i32 imageHandle,
    i16 x,
    i16 y,
    i16 originX,
    i16 originY,
    i16 clipLeft,
    i16 clipTop,
    i16 clipRight,
    i16 clipBottom,
    u32 flags,
    i16 offsetX,
    i16 offsetY
) {}

RVA(0x000017c0, 0x4)
b16 TestSceneSpritePixel(
    i32 imageHandle,
    SpriteBitmap* image,
    i16 x,
    i16 y,
    u32 flags,
    i16 offsetX,
    i16 offsetY,
    i16 pointX,
    i16 pointY
) {
    return false;
}

RVA(0x000017d0, 0x3)
b32 DrawImageFrame(
    VideoViewport* dest,
    u32 image,
    i16 frame,
    i16 a,
    i16 b,
    i16 c,
    i16 d,
    i16 e,
    i16 f
) {
    return false;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x000017e0, 0x14)
i16 GetSpriteAlignedX(SpriteBitmap* image, i16 x) {
    return image->offsetX - image->width + x;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x00001800, 0x15)
i16 GetSpriteAlignedY(SpriteBitmap* image, i16 y) {
    return image->offsetY - image->height + y;
}

// @identity-TODO: the legacy sprite interface has four uncalled, disabled
// entry points here; their original names and parameter lists are unknown.
// @dead-code
// Zero-ref: no effective rel32 caller or relocated pointer reaches this body.
RVA(0x00001820, 0x3)
b32 GetLegacySpriteTestResult(void) {
    return false;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller or relocated pointer reaches this body.
RVA(0x00001830, 0x3)
b32 GetLegacyImageDrawResult(void) {
    return false;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller or relocated pointer reaches this body.
RVA(0x00001840, 0x1)
void SkipLegacyImageTransfer(void) {}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller or relocated pointer reaches this body.
RVA(0x00001850, 0x1)
void SkipLegacyScriptBlit(void) {}

RVA(0x00001860, 0x1)
void BlitScriptImage(
    VideoViewport* dest,
    u32 image,
    i16 frame,
    i16 x,
    i16 y,
    i16 mode,
    i16 copy,
    i16 option
) {}
