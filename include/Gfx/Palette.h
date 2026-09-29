#ifndef GITEN_GFX_PALETTE_H
#define GITEN_GFX_PALETTE_H

#include <EnumDomain.h>
#include <Ints.h>
#include <Enums.h>

// clang-format off
GZ_ENUM_FLAGS_BEGIN(PaletteUpdateFlags, u8)
    PALETTE_UPDATE_DIRTY = 0x40,
    PALETTE_UPDATE_QUEUED = 0x80
GZ_ENUM_END(PaletteUpdateFlags);
// clang-format on

typedef struct PaletteState {
    i16 references[16];
    i16 colors[16];
} PaletteState;

// A selected 16-colour palette and the hardware entries it retains.
typedef struct ImagePalette {
    i16 variant;
    u16 colorMask;
    u16 colors[16];
    u8 entries[16];
} ImagePalette;

void ReleaseImagePalette(ImagePalette* palette);

#endif // GITEN_GFX_PALETTE_H
