// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Gfx/VramAccess.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>

// @identity-TODO: two 32-byte checkerboard tiles (and the inverse) plus a
// cleared word, written at startup; no code in the image reads them.
DATA(0x00091300)
static u32 s_checkerPattern[8];

DATA(0x00091320)
static u32 s_checkerPatternInverse[8];

DATA(0x000716fc)
static i16 s_patternState;

RVA(0x000028a0, 0x4)
i16 SaveDrawState(void) {
    return 0;
}

RVA(0x000028b0, 0x1)
void __fastcall RestoreDrawState(i16 token) {}

// @identity-TODO: the handle allocation reserves 128 bytes beyond the header
// and four bytes per byte column and scanline; the Windows stubs never
// consume that tail.
RVA(0x000028c0, 0x3f)
i32 AllocScreenSaveHandle(i16 columns, i16 scanlines) {
    i32 handle = AllocHandle(sizeof(ScreenSaveHeader) + columns * scanlines * 4 + 128);
    ScreenSaveHeader* save = HandleWritePtr(handle);
    SetScreenSaveSize(save, columns, scanlines);
    return handle;
}

RVA(0x00002900, 0x2c)
ScreenSaveHeader* AllocScreenSaveBuffer(i16 columns, i16 scanlines) {
    ScreenSaveHeader* save = AllocCleared(1, sizeof(ScreenSaveHeader) + columns * scanlines * 4);
    SetScreenSaveSize(save, columns, scanlines);
    return save;
}

// @dead-code
// Zero-ref: no retail call, jump, or relocated pointer reaches this helper.
RVA(0x00002930, 0x32)
ScreenSaveHeader* AllocRegionScreenSave(ScreenSaveRegion* region) {
    ScreenSaveHeader* save = AllocScreenSaveBuffer(region->columns, region->rows * 8);
    save->offset = region->row * 640;
    save->offset += region->column;
    return save;
}

RVA(0x00002970, 0x2e)
void InitCheckerPatterns(void) {
    i16 i;
    for (i = 0; i < 8; i++) {
        s_checkerPattern[i] = 0x55aa55aa;
    }
    for (i = 0; i < 8; i++) {
        s_checkerPatternInverse[i] = 0xaa55aa55;
    }
    s_patternState = 0;
}

RVA(0x000029a0, 0x4)
i16 SaveScreenState(void) {
    return 0;
}

// @identity-TODO: this unused screen-state result's original role is unknown.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000029b0, 0x4)
i16 GetLegacyScreenStateToken(void) {
    return 0;
}

RVA(0x000029c0, 0x4)
i16 SaveCellState(void) {
    return 0;
}

RVA(0x000029d0, 0x1)
void __fastcall RestoreScreenState(i16 token) {}

// @identity-TODO: these empty restore hooks have no recovered signatures.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000029e0, 0x1)
void SkipLegacyScreenRestore(void) {}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000029f0, 0x1)
void SkipLegacyCellRestore(void) {}
