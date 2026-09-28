#ifndef GITEN_GFX_SPRITE_H
#define GITEN_GFX_SPRITE_H

#include <rva.h>

#include <Enums.h>
#include <Gfx/Picture.h>
#include <Ints.h>

// clang-format off
GZ_ENUM_BEGIN(SpriteLayerMode)
    SPRITE_LAYERS_ALL = 0,
    SPRITE_LAYERS_PARTY_AND_TEXT = 1
GZ_ENUM_END(SpriteLayerMode);

GZ_ENUM_BEGIN(SpriteImageVariant)
    SPRITE_IMAGE_HALF_SIZE = 4
GZ_ENUM_END(SpriteImageVariant);

// @identity-TODO: special image identities are named only by their compositing role.
GZ_ENUM_BEGIN(SpriteCompositeImage)
    SPRITE_IMAGE_CACHED_BACKDROP = 0x4019,
    SPRITE_IMAGE_INSET_EXCLUSIVE_BEGIN = 0x401d,
    SPRITE_IMAGE_INSET_END = 0x4020,
    SPRITE_IMAGE_CLEAR_BACKDROP = 0x409a,
    SPRITE_IMAGE_CACHED_OVERLAY = 0x409b
GZ_ENUM_END(SpriteCompositeImage);
// clang-format on

#define SPRITE_GROUP_COUNT 32
#define SPRITE_FRAME_COUNT 16
#define SPRITE_SLOT_COUNT 32
#define SPRITE_UNPLACED (-1)

#define IsSpriteFrameIndexOutOfRange(group, frame)                                                 \
    ((group) < 0 || (group) > SPRITE_GROUP_COUNT - 1 || (frame) < 0                                \
     || (frame) > SPRITE_FRAME_COUNT - 1)

// A placed sprite: its image group, frame (-1 unplaced), and position.
typedef struct SpriteSlot {
    i16 group;
    i16 frame;
    i16 x;
    i16 y;
} SpriteSlot;

#define GetSpriteSlot(index) (&g_spriteSlots[(index)])

#define GetSpriteSlotFrame(slot) ((slot)->frame)

#define SetSpriteSlotFrame(slot, value) ((slot)->frame = (value))

#define GetSpriteFramePicture(group, frame) (&g_spriteImages[(group)][(frame)])

struct BmpFile;

#ifdef __cplusplus
extern "C" {
#endif

    extern Picture g_spriteImages[SPRITE_GROUP_COUNT][SPRITE_FRAME_COUNT];
    extern SpriteSlot g_spriteSlots[SPRITE_SLOT_COUNT];
    extern i16 g_spriteOrder[SPRITE_SLOT_COUNT];

    // Loads data record 0x4000 + image into a sprite image group.
    void LoadSpriteImage(i16 slot, i16 image, i16 arg);

    // Loads the concatenated bitmap frames into a sprite slot.
    RVA_DECL(0x000588c0)
    void LoadSpriteFrames(struct BmpFile* data, i16 slot, i16 image, i32 size);

    // Clears the scene picture and image groups, selecting the layers drawn over sprites.
    // The mode remains a signed word across the game/platform boundary.
    RVA_DECL(0x000497d0)
    void ResetSprites(i16 mode);
    i16 GetSpriteMode(void);

    // Empties a placed slot.
    RVA_DECL(0x00058700)
    void UnplaceSprite(i16 slot);

    RVA_DECL(0x00058780)
    void PlaceSprite(i16 id, i16 slot, i16 frame, i16 x, i16 y);

    // Nonzero when the requested frame has a surface and initialized picture.
    RVA_DECL(0x00058870)
    b16 IsSpriteFrameLoaded(i16 slot, i16 frame);

    // Nonzero while `slot` holds a placed picture.
    RVA_DECL(0x00058800)
    b16 IsSpritePlaced(i16 slot);

#ifdef __cplusplus
}
#endif

#endif // GITEN_GFX_SPRITE_H
