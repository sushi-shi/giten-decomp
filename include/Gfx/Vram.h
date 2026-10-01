#ifndef GITEN_GFX_VRAM_H
#define GITEN_GFX_VRAM_H

#include <Gfx/ImageHandle.h>
#include <Gfx/Palette.h>
#include <Ints.h>
#include <Util/PixelMask.h>

#include <stdio.h>

// @identity-TODO: a 1-bit-per-pixel plane over part of the 640x400 screen,
// stored column-major (one byte covers 8 pixels of one line); what the game
// masks with it is not recovered yet.
typedef struct MaskRect {
    i16 left;   // first byte column
    i16 top;    // first line
    i16 right;  // last byte column
    i16 bottom; // last line
    i16 width;  // right - left + 1
    i16 height; // bottom - top + 1
} MaskRect;

// Stored in two fixed 0x2400-byte buffers (the loaded mask and the working
// copy); width * height bytes are used, column after column.
typedef struct MaskGrid {
    MaskRect rect;
    u8 bits[0x2400 - sizeof(MaskRect)];
} MaskGrid;

#define IsMaskColumnInBounds(mask, column)                                                         \
    ((mask)->rect.left <= (column) && (mask)->rect.right >= (column))

static __inline i16 GetMaskGridOffset(const MaskGrid* mask, i16 column, i16 line) {
    return (column - mask->rect.left) * mask->rect.height - mask->rect.top + line;
}

void ResetMask(i16 copySaved);
void LoadMask(void);
void ClearMaskView(void);
void ClearMaskSeam(i16 column);
void GetMaskColumn(u8* out, i16 column, i16 line);
u8 GetMaskByte(i16 column, i16 line);
void ReadMaskColumn(u8* out, i16 column, i16 line);
void WriteMaskColumn(const u8* src, i16 column, i16 line, i16 set);
i16 TestMaskPixel(i16 x, i16 line);

void ResetUpperPalette(void);
PaletteState* SavePaletteState(PaletteState* state, i16 mode);
PaletteState* RestorePaletteState(PaletteState* state, i16 release);
u8 FindPaletteEntry(i16 color);
void RetainPaletteEntry(u8 index);
void SetPaletteEntry(u8 index, i16 color);
u32 GrbToRgb(u32 grb);
void ReleasePaletteEntry(u8 index);
void MarkPaletteDirty(void);
void SetAreaPaletteMode(GZ_ENUM_PARAM(CellPaletteMode, i16) mode);
GZ_ENUM_RETURN(CellPaletteMode, i16) GetAreaPaletteMode(void);
void SetViewPaletteMode(GZ_ENUM_PARAM(CellPaletteMode, i16) mode);
GZ_ENUM_RETURN(CellPaletteMode, i16) GetViewPaletteMode(void);
void QueuePaletteUpload(void);

// The whole-file picture loaders: each opens the request's data file, reads
// all of it into a new block and closes it again (NULL when the open fails).
struct ImageRequest;

// Picture `request->file` (data file kind 0, the request's variant).
void* LoadImageData(struct ImageRequest* request);

// The same, storing the length in `size`.
void* LoadImageVariant(struct ImageRequest* request, i32* size);

// @identity-TODO: the kind-1 file of the same id (variant 0) that the loader
// at 0x416290 hands to 0x457d80 after the picture; what it holds is
// unrecovered.
void* LoadImageKind1(struct ImageRequest* request);

// The kind-0xf file of the request, storing its length in `size`.
void* LoadImageFile(struct ImageRequest* request, i32* size);

i32 ReadImageBlockHandle(FILE* file);
i32 ReleaseImageBlockHandle(i32 handle);

#endif // GITEN_GFX_VRAM_H
