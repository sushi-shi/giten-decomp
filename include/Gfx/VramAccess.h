#ifndef GITEN_GFX_VRAMACCESS_H
#define GITEN_GFX_VRAMACCESS_H

#include <Ints.h>

// A planar screen snapshot: byte offset, byte columns, then scanlines.
// Its payload holds four colour-plane bytes per column and scanline.
typedef struct ScreenSaveHeader {
    i16 offset;
    i16 columns;
    i16 scanlines;
} ScreenSaveHeader;

static __inline void SetScreenSaveSize(ScreenSaveHeader* save, i16 columns, i16 scanlines) {
    save->columns = columns;
    save->scanlines = scanlines;
}

i32 AllocScreenSaveHandle(i16 columns, i16 scanlines);
ScreenSaveHeader* AllocScreenSaveBuffer(i16 columns, i16 scanlines);

// A rectangle on the 8x8-cell grid; a column is one byte in each plane.
typedef struct ScreenSaveRegion {
    i16 column;
    i16 row;
    i16 columns;
    i16 rows;
} ScreenSaveRegion;

ScreenSaveHeader* AllocRegionScreenSave(ScreenSaveRegion* region);

// @identity-TODO: bracket pairs around screen drawing. The begin side returns
// a token the caller hands back to the end side in ECX; the Windows build
// keeps them as empty bodies, so what state they saved is not recovered.
b16 SaveDrawState(void);
void __fastcall RestoreDrawState(i16 token);
b16 SaveScreenState(void);
b16 SaveCellState(void);
void __fastcall RestoreScreenState(i16 token);

void InitCheckerPatterns(void);

#endif // GITEN_GFX_VRAMACCESS_H
