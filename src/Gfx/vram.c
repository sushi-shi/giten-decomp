// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/FieldSight.h>
#include <Gfx/Background.h>
#include <Gfx/Bitmap.h>
#include <Gfx/SpriteBitmap.h>
#include <Gfx/Vram.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/PlatformApi.h>

#include <io.h>
#include <stdio.h>
#include <string.h>

// @identity-TODO: shared with the map-bit helpers further on; the first
// user in link order is here, which does not prove ownership.
DATA(0x00064280)
const u8 g_pixelMasks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

// Storage for the mask as loaded from its data file and for the working copy.
DATA(0x00071700)
static MaskGrid s_savedMaskData = {0};

DATA(0x00073b00)
static MaskGrid s_maskData = {0};

// The mask as loaded from its data file, and the working copy drawn against.
DATA(0x00075f00)
static MaskGrid* s_savedMask = 0;

DATA(0x00075f04)
static MaskGrid* s_mask = 0;

// The 16 analog palette entries (0xGRB) and how many users hold each one.
// Nothing in this image reads the colours back.
DATA(0x00075f38)
static i16 s_paletteColors[16] = {0};

DATA(0x00075f58)
static i16 s_paletteRefs[16] = {0};

// Bit 0x40: a palette entry or mode changed; bit 0x80: a change awaits
// upload. No reader of the queued bit survives in this image.
DATA(0x00075f7c)
static GZ_ENUM_STORAGE(PaletteUpdateFlags, u8) s_paletteFlags = 0;

// @identity-TODO: palette modes chosen from map-area tests (the area one at
// the party's square, the view one at a derived position); the renderer picks
// its alternate material set when either is set. Their game meaning is open.
DATA(0x00075f80)
static i16 s_areaPaletteMode = 0;

DATA(0x00075f84)
static i16 s_viewPaletteMode = 0;

static __inline void StorePaletteColor(i16 index, i16 color) {
    s_paletteColors[index] = color;
}

RVA(0x00002b70, 0x71)
void ResetMask(i16 copySaved) {
    u16 size;
    s_mask->rect = s_savedMask->rect;
    size = s_mask->rect.height * s_mask->rect.width;
    if (copySaved) {
        memcpy(s_mask->bits, s_savedMask->bits, size);
    } else {
        memset(s_mask->bits, 0, size);
    }
}

// Loads the map mask from its data file into the saved buffer, then copies it.
RVA(0x00002bf0, 0x69)
void LoadMask(void) {
    u16 size;
    FILE* fp = OpenDataFile(2, 12, 0);
    fread(&size, 2, 1, fp);
    s_savedMask = &s_savedMaskData;
    s_mask = &s_maskData;
    fread(s_savedMask, 1, size, fp);
    CloseDataFile(fp);
    ResetMask(1);
}

// @identity-TODO: called right before ResetMask(1) when a map or mode is
// entered; the Windows build keeps an empty body.
RVA(0x00002c60, 0x1)
void ClearMaskView(void) {}

// Clears the pixel pair where byte column `column` meets the next one.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002c70, 0x65)
void ClearMaskSeam(i16 column) {
    i16 i;
    i16 index;
    index = (column - s_mask->rect.left) * s_mask->rect.height;
    for (i = 0; i < s_mask->rect.height; i++) {
        s_mask->bits[index++] &= 0xfe;
    }
    for (i = 0; i < s_mask->rect.height; i++) {
        s_mask->bits[index++] &= 0x7f;
    }
}

RVA(0x00002ce0, 0x76)
void GetMaskColumn(u8* out, i16 column, i16 line) {
    i16 i;
    i16 start;
    if (!IsMaskColumnInBounds(s_mask, column)) {
        memset(out, 0, 8);
        return;
    }
    start = GetMaskGridOffset(s_mask, column, line);
    for (i = start; i < start + 8; i++) {
        if (line >= s_mask->rect.top && line <= s_mask->rect.bottom) {
            *out++ = s_mask->bits[i];
        } else {
            *out++ = 0;
        }
        line++;
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002d60, 0x47)
u8 GetMaskByte(i16 column, i16 line) {
    i16 index;
    if (!IsMaskColumnInBounds(s_mask, column) || s_mask->rect.top > line
        || s_mask->rect.bottom < line) {
        return 0;
    }
    index = GetMaskGridOffset(s_mask, column, line);
    return s_mask->bits[index];
}

RVA(0x00002db0, 0x18)
void ReadMaskColumn(u8* out, i16 column, i16 line) {
    GetMaskColumn(out, column, line);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002dd0, 0x7f)
void WriteMaskColumn(const u8* src, i16 column, i16 line, i16 set) {
    i16 start;
    i16 k;
    if (!IsMaskColumnInBounds(s_mask, column)) {
        return;
    }
    start = GetMaskGridOffset(s_mask, column, line);
    for (k = 0; k < 8; line++, k++) {
        if (line >= s_mask->rect.top && line <= s_mask->rect.bottom) {
            if (set) {
                s_mask->bits[start + k] |= src[k];
            } else {
                s_mask->bits[start + k] &= ~src[k];
            }
        }
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002e50, 0x68)
i16 TestMaskPixel(i16 x, i16 line) {
    u16 pixel = x;
    i16 column = x / 8;
    if (!IsMaskColumnInBounds(s_mask, column) || s_mask->rect.top > line
        || s_mask->rect.bottom < line) {
        return 0;
    }
    return s_mask->bits[GetMaskGridOffset(s_mask, column, line)] & GetPixelMask(pixel);
}

// @early-stop: retail keeps an 8-step word-store loop through base+offset;
// with no asynchronous reader of the colours, cl merges the zero stores into
// four dword stores. Pointer, int-index and fused-loop forms merge them too;
// an int index passed through StorePaletteColor keeps a sign-extending loop.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002ec0, 0x2b)
void ResetUpperPalette(void) {
    i16 i;
    memset(&s_paletteRefs[8], 0, 8 * sizeof(s_paletteRefs[0]));
    for (i = 8; i < 16; i++) {
        StorePaletteColor(i, 0);
    }
}

RVA(0x00002ef0, 0x5)
PaletteState* SavePaletteState(PaletteState* state, i16 mode) {
    return state;
}

RVA(0x00002f00, 0x5)
PaletteState* RestorePaletteState(PaletteState* state, i16 release) {
    return state;
}

// @dead-code
// Zero-ref: no retail call, jump or relocated pointer reaches this helper.
// The legacy colour lookup is disabled; its fallback entry remains.
RVA(0x00002f10, 0x3)
u8 FindPaletteEntry(i16 color) {
    return 2;
}

RVA(0x00002f20, 0x1d)
void RetainPaletteEntry(u8 index) {
    i16* ref;
    if (index < 16) {
        ref = &s_paletteRefs[index];
        (*ref)++;
    }
}

RVA(0x00002f40, 0x28)
b16 SetPaletteColor(u8 index, i16 color) {
    if (index < 16) {
        StorePaletteColor(index, color);
        MarkPaletteDirty();
        return true;
    }
    return false;
}

// @identity-TODO: this palette hook has no recovered API name or signature;
// its empty body and placement between palette-entry operations are proven.
// @dead-code
// Zero-ref: no effective call/jump, relocated pointer or data slot reaches it.
RVA(0x00002f70, 0x1)
void UnusedPaletteUpdateHook(void) {}

RVA(0x00002f80, 0x23)
void SetPaletteEntry(u8 index, i16 color) {
    if (SetPaletteColor(index, color)) {
        RetainPaletteEntry(index);
    }
}

// PC-98 analog palette words are 0xGRB; swap the G and R nibbles.
RVA(0x00002fb0, 0x21)
u32 GrbToRgb(u32 grb) {
    return ((grb >> 4) & 0xf0) | (((u8)grb & 0xf0) << 4) | (grb & 0x0f);
}

RVA(0x00002fe0, 0x26)
void ReleasePaletteEntry(u8 index) {
    i16* ref;
    if (index < 16) {
        ref = &s_paletteRefs[index];
        if (*ref > 0) {
            (*ref)--;
        }
    }
}

// @early-stop: retail loads, ORs and stores the flag byte separately; cl
// folds the update into one `or byte ptr` unless the byte is volatile, and no
// asynchronous writer or reader exists. A bitfield form folds too.
RVA(0x00003010, 0xd)
void MarkPaletteDirty(void) {
    s_paletteFlags |= PALETTE_UPDATE_DIRTY;
}

RVA(0x00003020, 0x1d)
void SetAreaPaletteMode(i16 mode) {
    i16 changed = s_areaPaletteMode - mode;
    s_areaPaletteMode = mode;
    if (changed) {
        MarkPaletteDirty();
    }
}

RVA(0x00003040, 0x7)
i16 GetAreaPaletteMode(void) {
    return s_areaPaletteMode;
}

RVA(0x00003050, 0xc)
void SetViewPaletteMode(i16 mode) {
    s_viewPaletteMode = mode;
}

RVA(0x00003060, 0x7)
i16 GetViewPaletteMode(void) {
    return s_viewPaletteMode;
}

// Turns a pending change into a queued upload.
// @early-stop: retail re-reads the flag byte for each update; cl merges the
// test and both updates into one load and store unless the byte is volatile,
// and no asynchronous writer or reader exists. A bitfield form merges too.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00003070, 0x25)
void QueuePaletteUpload(void) {
    if (s_paletteFlags & PALETTE_UPDATE_DIRTY) {
        s_paletteFlags |= PALETTE_UPDATE_QUEUED;
        s_paletteFlags &= ~PALETTE_UPDATE_DIRTY;
    }
}

// @identity-TODO: the following disabled palette hooks have no recovered
// signatures or API names; their observed bodies do nothing.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000030a0, 0x1)
void SkipQueuedPaletteUpload(void) {}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000030b0, 0x1)
void SkipPaletteModeChange(void) {}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000030c0, 0x1)
void SkipPaletteSync(void) {}

// @dead-code
// Zero-ref: no retail call, jump or relocated pointer reaches this helper.
RVA(0x000030d0, 0x27)
void ReleaseImagePalette(ImagePalette* palette) {
    i16 i;
    for (i = 0; i < 16; i++) {
        if (palette->entries[i] != 0xff) {
            ReleasePaletteEntry(palette->entries[i]);
            palette->entries[i] = 0xff;
        }
    }
}

// @identity-TODO: this empty palette-cleanup hook's signature is unknown.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00003100, 0x1)
void SkipImagePaletteCleanup(void) {}

// @identity-TODO: the unused zero-valued image result's role is unknown.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00003110, 0x3)
b32 GetLegacyImagePaletteResult(void) {
    return false;
}

// @identity-TODO: callers drop a cached image with it (resetting the cache
// key to -1) and store the NULL result back.
RVA(0x00003120, 0x3)
u32 FreeImageHandle(u32 handle) {
    return 0;
}

// Reload a caller-owned size after the allocation.
#define ReadImageBytes(block, file, byteCount)                                                     \
    do {                                                                                           \
        (block) = AllocClearedLong(1, (byteCount));                                                \
        fread((block), (byteCount), 1, (file));                                                    \
    } while (0)

RVA(0x00003130, 0x58)
void* LoadImageData(ImageRequest* request) {
    void* block = NULL;
    FILE* fp = OpenDataFile(request->file, 0, request->variant);
    i32 size;
    if (fp != NULL) {
        size = _filelength(_fileno(fp));
        ReadImageBytes(block, fp, size);
    }
    CloseDataFile(fp);
    return block;
}

RVA(0x00003190, 0x5e)
void* LoadImageVariant(ImageRequest* request, i32* size) {
    void* block = NULL;
    FILE* fp = OpenDataFile(request->file, 0, request->variant);
    if (fp != NULL) {
        *size = _filelength(_fileno(fp));
        ReadImageBytes(block, fp, *size);
    }
    CloseDataFile(fp);
    return block;
}

// Loads the picture `mode` bitmaps into the request's file (via SeekBitmap).
RVA(0x000031f0, 0x79)
void* LoadImageRequest(ImageRequest* request, i16 mode) {
    void* block = NULL;
    FILE* fp = OpenDataFile(request->file, 0, request->variant);
    u32 size;
    i32 length;
    if (fp != NULL) {
        length = _filelength(_fileno(fp));
        fp = SeekBitmap(fp, mode, length, &size);
        if (fp == NULL) {
            return NULL;
        }
        ReadImageBytes(block, fp, size);
    }
    CloseDataFile(fp);
    return block;
}

RVA(0x00003270, 0x55)
void* LoadImageKind1(ImageRequest* request) {
    void* block = NULL;
    FILE* fp = OpenDataFile(request->file, 1, 0);
    i32 size;
    if (fp != NULL) {
        size = _filelength(_fileno(fp));
        ReadImageBytes(block, fp, size);
    }
    CloseDataFile(fp);
    return block;
}

RVA(0x000032d0, 0x5f)
void* LoadImageFile(ImageRequest* request, i32* size) {
    void* block = NULL;
    FILE* fp = OpenDataFile(request->file, 0xf, request->variant);
    if (fp != NULL) {
        *size = _filelength(_fileno(fp));
        ReadImageBytes(block, fp, *size);
    }
    CloseDataFile(fp);
    return block;
}

RVA(0x00003330, 0x10)
void* FreeImageFile(void* data) {
    return FreeBlock(data);
}

// @identity-TODO: three unused image-block results lack recovered signatures
// and original API names; only their zero-valued return bodies are proven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00003340, 0x3)
b32 GetLegacyImageHandleResult(void) {
    return false;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00003350, 0x3)
b32 GetLegacyImageBlockResult(void) {
    return false;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00003360, 0x3)
b32 GetLegacySpriteCopyResult(void) {
    return false;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x00003370, 0x52)
i32 ReadImageBlockHandle(FILE* file) {
    u16 size;
    i32 handle;
    void* data;
    fread(&size, 2, 1, file);
    handle = AllocHandle(size);
    data = HandleWritePtr(handle);
    fread(data, 1, size, file);
    return handle;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x000033d0, 0x12)
i32 ReleaseImageBlockHandle(i32 handle) {
    FreeHandle(handle);
    return handle;
}

RVA(0x000033f0, 0x41)
i32 CopySpriteBitmap(SpriteBitmap* image) {
    i32 size = image->width * image->height * 2 + sizeof(SpriteBitmap);
    i32 handle = AllocHandle(size);
    void* copy = HandleWritePtr(handle);
    u16 copySize = size;
    memmove(copy, image, copySize);
    return handle;
}
