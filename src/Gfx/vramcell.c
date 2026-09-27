// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Gfx/VramCell.h>

// ORs the four plane bytes of each line into that line's mask byte.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000022e0, 0x32)
u8* BuildCellMask(const u8 (*planes)[4], u8* mask) {
    i16 i;
    i16 j;
    for (i = 0; i < 8; i++) {
        mask[i] = 0;
        for (j = 0; j < 4; j++) {
            mask[i] |= planes[i][j];
        }
    }
    return mask;
}

RVA(0x00002320, 0x24)
void AndCellMask(CellRow* cell, const u8* mask) {
    i16 i;
    for (i = 0; i < 8; i++) {
        cell[i].mask &= mask[i];
    }
}

// @dead-code
// Legacy palette-index remapping of an 8x8 planar cell is disabled on Windows.
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002350, 0x1)
void RemapCellPlanes(u8 (*planes)[4], const u8* entries) {}

// @identity-TODO: the script cell commands call this with a 4-bit colour
// before masking the cell; the Windows build keeps an empty body.
RVA(0x00002360, 0x1)
void FillCell(CellRow* cell, i16 color) {}

// Interleave 8 lines of 4-plane pixels with their mask bytes; the narrower
// forms keep only the first 3, 2 or 1 planes of each source line.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002370, 0x37)
void PackCell(CellRow* cell, const u8 (*planes)[4], const u8* mask) {
    i16 i;
    for (i = 0; i < 8; i++) {
        PackCellRow(&cell[i], planes[i], mask[i], 4);
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000023b0, 0x45)
void PackCell3(CellRow3* cell, const u8 (*planes)[4], const u8* mask) {
    i16 i;
    for (i = 0; i < 8; i++) {
        PackCellRow(&cell[i], planes[i], mask[i], 3);
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002400, 0x46)
void PackCell2(CellRow2* cell, const u8 (*planes)[4], const u8* mask) {
    i16 i;
    for (i = 0; i < 8; i++) {
        PackCellRow(&cell[i], planes[i], mask[i], 2);
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002450, 0x46)
void PackCell1(CellRow1* cell, const u8 (*planes)[4], const u8* mask) {
    i16 i;
    for (i = 0; i < 8; i++) {
        PackCellRow(&cell[i], planes[i], mask[i], 1);
    }
}

// @identity-TODO: the six disabled cell-format entry points have no known
// signatures or API names; these labels only group their return widths.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000024a0, 0x3)
i32 GetLegacyCellPackResult(void) {
    return 0;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000024b0, 0x3)
i32 GetLegacyCellMaskResult(void) {
    return 0;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000024c0, 0x4)
i16 GetLegacyCellWidthResult(void) {
    return 0;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000024d0, 0x4)
i16 GetLegacyCellHeightResult(void) {
    return 0;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000024e0, 0x3)
u8 GetLegacyCellPixelResult(void) {
    return 0;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000024f0, 0x4)
i16 GetLegacyCellPlaneResult(void) {
    return 0;
}
