#include <rva.h>

#include <Gfx/Bitmap.h>
#include <Gfx/Sprite.h>
#include <Platform/Com.h>
#include <Platform/GameApi.h>

#include <string.h>

DATA(0x00088a10)
Picture g_spriteImages[SPRITE_GROUP_COUNT][SPRITE_FRAME_COUNT];

DATA(0x0008d728)
SpriteSlot g_spriteSlots[SPRITE_SLOT_COUNT];

DATA(0x0008f588)
i16 g_spriteOrder[SPRITE_SLOT_COUNT];

// @early-stop register/scheduling residue: retail initializes the frame cursor
// in edx before the order-table memset; this build uses eax after it. The
// signed loop edge, stores and ordered referents match. Pointer lifetime,
// counter-width and shared frame-setter controls do not recover the schedule.
RVA(0x000586d0, 0x27)
void UnplaceAllSprites(void) {
    i32 slot;
    memset(g_spriteOrder, -1, sizeof(g_spriteOrder));
    for (slot = 0; slot < SPRITE_SLOT_COUNT; slot++) {
        SetSpriteSlotFrame(GetSpriteSlot(slot), SPRITE_UNPLACED);
    }
}

RVA(0x00058700, 0x7a)
void UnplaceSprite(i16 slot) {
    i16 order;
    if (slot < 0 && slot > SPRITE_SLOT_COUNT - 1) {
        return;
    }
    GetSpriteSlot(slot)->group = 0;
    SetSpriteSlotFrame(GetSpriteSlot(slot), SPRITE_UNPLACED);
    GetSpriteSlot(slot)->x = 0;
    GetSpriteSlot(slot)->y = 0;
    for (order = 0; order < SPRITE_SLOT_COUNT; order++) {
        if (g_spriteOrder[order] == slot) {
            // The byte count is deliberately not scaled by the element size.
            memmove(
                &g_spriteOrder[order],
                &g_spriteOrder[order + 1],
                SPRITE_SLOT_COUNT - 1 - order
            );
            g_spriteOrder[SPRITE_SLOT_COUNT - 1] = SPRITE_UNPLACED;
            return;
        }
    }
}

RVA(0x00058780, 0x71)
void PlaceSprite(i16 id, i16 slot, i16 frame, i16 x, i16 y) {
    i16 order;
    if (slot < 0 || slot > SPRITE_SLOT_COUNT - 1) {
        return;
    }
    GetSpriteSlot(slot)->group = id;
    SetSpriteSlotFrame(GetSpriteSlot(slot), frame);
    GetSpriteSlot(slot)->x = x * 8;
    GetSpriteSlot(slot)->y = y;
    for (order = 0; order < SPRITE_SLOT_COUNT; order++) {
        if (g_spriteOrder[order] == SPRITE_UNPLACED) {
            g_spriteOrder[order] = slot;
            return;
        }
    }
}

RVA(0x00058800, 0x17)
i16 IsSpritePlaced(i16 slot) {
    return GetSpriteSlotFrame(GetSpriteSlot(slot)) != SPRITE_UNPLACED;
}

RVA(0x00058820, 0x44)
void FreeSpriteImages(i16 slot) {
    i32 frame;
    if (slot < 0 || slot > SPRITE_GROUP_COUNT - 1) {
        return;
    }
    for (frame = 0; frame < SPRITE_FRAME_COUNT; frame++) {
        ReleaseComObject(GetSpriteFramePicture(slot, frame)->surface);
        GetSpriteFramePicture(slot, frame)->visible = FALSE;
    }
}

RVA(0x00058870, 0x50)
i16 IsSpriteFrameLoaded(i16 slot, i16 frame) {
    if (IsSpriteFrameIndexOutOfRange(slot, frame)) {
        return FALSE;
    }
    if (GetSpriteFramePicture(slot, frame)->surface == NULL) {
        return FALSE;
    }
    return GetSpriteFramePicture(slot, frame)->visible != FALSE;
}

RVA(0x000588c0, 0xd0)
void LoadSpriteFrames(BmpFile* data, i16 slot, i16 image, i32 size) {
    BmpFile* bmp;
    i32 frame;
    i32 width;
    i32 height;
    i32 displayWidth;
    i32 displayHeight;
    if (slot < 0 || slot > SPRITE_GROUP_COUNT - 1 || data == NULL) {
        return;
    }
    bmp = data;
    for (frame = 0; frame < SPRITE_FRAME_COUNT; frame++) {
        width = bmp->info.biWidth;
        height = bmp->info.biHeight;
        displayWidth = width;
        displayHeight = height;
        if (image >= 0x2000 && image < 0x3000 && (image & 0xf) == SPRITE_IMAGE_HALF_SIZE) {
            displayWidth /= 2;
            displayHeight /= 2;
        }
        if (!CreatePicture(
                GetSpriteFramePicture(slot, frame),
                displayWidth,
                displayHeight,
                width,
                height,
                FALSE
            )) {
            return;
        }
        GetSpriteFramePicture(slot, frame)->id = image;
        LoadBitmapToSurface16(bmp, &GetSpriteFramePicture(slot, frame)->surface, NULL);
        bmp = GetNextBitmap(bmp);
        if (reinterpret_cast<u8*>(data) + size
            <= reinterpret_cast<u8*>(bmp)) { // Byte-forced: stream end.
            return;
        }
    }
}
