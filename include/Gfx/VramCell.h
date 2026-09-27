#ifndef GITEN_GFX_VRAMCELL_H
#define GITEN_GFX_VRAMCELL_H

#include <Ints.h>

// One line of an 8x8 screen cell: the colour-plane bytes and the mask. The
// narrower rows hold the first 3, 2 or 1 planes.
typedef struct CellRow {
    u8 planes[4];
    u8 mask;
} CellRow;

typedef struct CellRow3 {
    u8 planes[3];
    u8 mask;
} CellRow3;

typedef struct CellRow2 {
    u8 planes[2];
    u8 mask;
} CellRow2;

typedef struct CellRow1 {
    u8 planes[1];
    u8 mask;
} CellRow1;

#define PackCellRow(row, sourcePlanes, sourceMask, planeCount)                                     \
    do {                                                                                           \
        i16 plane;                                                                                 \
        for (plane = 0; plane < (planeCount); plane++) {                                           \
            (row)->planes[plane] = (sourcePlanes)[plane];                                          \
        }                                                                                          \
        (row)->mask = (sourceMask);                                                                \
    } while (0)

u8* BuildCellMask(const u8 (*planes)[4], u8* mask);
void AndCellMask(CellRow* cell, const u8* mask);
void RemapCellPlanes(u8 (*planes)[4], const u8* entries);
void FillCell(CellRow* cell, i16 color);
void PackCell(CellRow* cell, const u8 (*planes)[4], const u8* mask);
void PackCell3(CellRow3* cell, const u8 (*planes)[4], const u8* mask);
void PackCell2(CellRow2* cell, const u8 (*planes)[4], const u8* mask);
void PackCell1(CellRow1* cell, const u8 (*planes)[4], const u8* mask);

#endif // GITEN_GFX_VRAMCELL_H
