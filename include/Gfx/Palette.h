#ifndef GITEN_GFX_PALETTE_H
#define GITEN_GFX_PALETTE_H

#include <EnumDomain.h>
#include <Enums.h>
#include <Ints.h>

// The hardware palette's entries, and the image-palette mark of an entry it
// does not retain.
#define PALETTE_SIZE 16
#define PALETTE_ENTRY_NONE 0xff

// clang-format off
GZ_ENUM_FLAGS_BEGIN(PaletteUpdateFlags, u8)
    PALETTE_UPDATE_DIRTY = 0x40,
    PALETTE_UPDATE_QUEUED = 0x80
GZ_ENUM_END(PaletteUpdateFlags);
// clang-format on

// The wall material selected for a normal or dark map cell.
GZ_ENUM_BEGIN_SPLIT(CellPaletteMode, i16)
    CELL_PALETTE_NORMAL = 0,
    CELL_PALETTE_DARK = 1
GZ_ENUM_END_SPLIT(CellPaletteMode)

typedef struct PaletteState {
    i16 references[PALETTE_SIZE];
    i16 colors[PALETTE_SIZE];
} PaletteState;

// A selected 16-colour palette and the hardware entries it retains.
typedef struct ImagePalette {
    i16 variant;
    u16 colorMask;
    u16 colors[PALETTE_SIZE];
    u8 entries[PALETTE_SIZE];
} ImagePalette;

#ifdef __cplusplus
extern "C" {
#endif

    void ReleaseImagePalette(ImagePalette* palette);
    b16 SetPaletteColor(u8 index, i16 color);

#ifdef __cplusplus
}
#endif

#endif // GITEN_GFX_PALETTE_H
