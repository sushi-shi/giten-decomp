#ifndef GITEN_GFX_BLIT_H
#define GITEN_GFX_BLIT_H

#include <Gfx/SpriteBitmap.h>
#include <Gfx/VideoState.h>
#include <Ints.h>

static __inline i32 GetSceneSpriteOffsetX(SpriteBitmap* image, u32 flags) {
    if (!flags) {
        return image->offsetX;
    }
    return 79 - image->offsetX;
}

static __inline i16 GetSceneSpriteOffsetY(SpriteBitmap* image) {
    return image->offsetY << 3;
}

// @identity-TODO: the Windows build retains only the image-header copy.
// Placement and flags are recovered from SceneSprite; the six global
// origin/clip words are never written in this build.
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
);
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
);
void SceneBlitNop(void);
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
);

// @identity-TODO: the Windows renderer leaves this legacy sprite hit test
// disabled and returns zero; placement comes from the scene sprite record.
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
);

// @identity-TODO: the frame, mode, copy and final option words are unused
// on Windows; the script caller supplies an image and its placement.
void BlitScriptImage(
    VideoViewport* dest,
    u32 image,
    i16 frame,
    i16 x,
    i16 y,
    i16 mode,
    i16 copy,
    i16 option
);

// @identity-TODO: returns zero on Windows; the placement and mode words
// after frame are unrecovered.
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
);

void PrepareFullSceneSprite(SpriteBitmap* image, i32 imageHandle, i16 x, i16 y, u32 flags);
void PrepareOffsetSceneSprite(SpriteBitmap* image, i32 imageHandle, i16 x, i16 y, u32 flags);
void PrepareViewportSceneSprite(
    SpriteBitmap* image,
    i32 imageHandle,
    i16 x,
    i16 y,
    u32 flags,
    VideoViewport* viewport
);
i16 GetSpriteAlignedX(SpriteBitmap* image, i16 x);
i16 GetSpriteAlignedY(SpriteBitmap* image, i16 y);

#endif // GITEN_GFX_BLIT_H
