// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/ObjectRecord.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>

// Running XOR key of the length-prefixed encrypted records: seeded from the
// record length, then each ciphertext byte keys the next one.
DATA(0x000712d0)
static u8 s_cryptKey;

// The open data file and the one it displaced (OpenDataFile nests one level).
// stdio buffers for the open data file and for one nested open.
DATA(0x000712d8)
static char s_fileBuffer[0x200];

DATA(0x000714d8)
static char s_nestedFileBuffer[0x200];

DATA(0x000716d8)
static FILE* s_dataFile;

DATA(0x000716dc)
static FILE* s_prevDataFile;

// The id last asked for, and the id actually opened (after any fallback).
DATA(0x000716e0)
static i16 s_requestedId;

DATA(0x000716e4)
static i16 s_openedId;

RVA(0x00001b20, 0x11)
void SetCryptKey(u16 seed) {
    s_cryptKey = seed >> 8;
    s_cryptKey ^= seed;
}

RVA(0x00001b40, 0x2b)
u8 ReadCryptByte(FILE* fp) {
    u8 c;
    u8 cipher;
    fread(&c, 1, 1, fp);
    cipher = c;
    c ^= s_cryptKey;
    s_cryptKey = cipher;
    return c;
}

RVA(0x00001b70, 0x2c)
u16 ReadCryptWord(FILE* fp) {
    u16 low = ReadCryptByte(fp);
    u16 word = ReadCryptByte(fp);
    word <<= 8;
    word |= low;
    return word;
}

RVA(0x00001ba0, 0x4a)
u16 ReadCryptBytes(FILE* fp, u16 count, u8* buf) {
    u16 i;
    u8 cipher;
    fread(buf, 1, count, fp);
    for (i = 0; i < count; i++) {
        cipher = buf[i];
        buf[i] ^= s_cryptKey;
        s_cryptKey = cipher;
    }
    return count;
}

// Reads a word (the decryption seed and length), then that many encrypted
// bytes into `out`.
RVA(0x00001bf0, 0x3d)
void* ReadCryptRecord(FILE* fp, void* out) {
    u16 len;
    fread(&len, 1, 2, fp);
    SetCryptKey(len);
    ReadCryptBytes(fp, len, out);
    return out;
}

// Reads a word count, then that many encrypted bytes into a new memory handle.
RVA(0x00001c30, 0x53)
i32 ReadCryptHandle(FILE* fp) {
    u16 len;
    i32 handle;
    fread(&len, 1, 2, fp);
    SetCryptKey(len);
    handle = AllocArrayHandle(len, 1);
    ReadCryptBytes(fp, len, HandleWritePtr(handle));
    return handle;
}

RVA(0x00001c90, 0x2d)
void SkipBytes(FILE* fp, u16 count) {
    u8 c;
    for (; count != 0; count--) {
        fread(&c, 1, 1, fp);
    }
}

// Skips a block that starts with its word length.
RVA(0x00001cc0, 0x27)
void SkipRawBlock(FILE* fp) {
    u16 len;
    fread(&len, 1, 2, fp);
    SkipBytes(fp, len);
}

// Reads a word count, then that many plain bytes into a new block.
RVA(0x00001cf0, 0x45)
void* ReadRawAlloc(FILE* fp) {
    u16 len;
    void* block;
    fread(&len, 1, 2, fp);
    block = AllocCleared(len, 1);
    fread(block, 1, len, fp);
    return block;
}

// Reads a word count, then that many plain bytes into a new memory handle.
RVA(0x00001d40, 0x4e)
i32 ReadRawHandle(FILE* fp) {
    u16 len;
    i32 handle;
    void* data;
    fread(&len, 1, 2, fp);
    handle = AllocArrayHandle(len, 1);
    data = HandleWritePtr(handle);
    fread(data, 1, len, fp);
    return handle;
}

RVA(0x00001d90, 0x34)
void ReadRawBlock(FILE* fp, void* buf) {
    u16 len;
    fread(&len, 1, 2, fp);
    fread(buf, 1, len, fp);
}

// @identity-TODO: `kind` selects the file family (face, map, event, sound,
// ...); the enum names are not recovered, so the cases stay numeric.
// A failed open retries once with the family's fallback id when it has one.
RVA(0x00001dd0, 0x478)
FILE* OpenDataFile(i16 id, i32 kind, i16 variant) {
    char name[64];
    FILE* fp;
    FILE* prev;
    i16 fallback;
    i16 retried;

    s_requestedId = id;
    retried = 0;
    sprintf(name, "fc\\fc%.4x.bmp", id);
    fp = fopen(name, "rb");
retry:
    fallback = -1;
    switch (kind) {
        case 15:
            sprintf(name, "fc\\fc%.4x%01d.bin", id, variant);
            fp = fopen(name, "rb");
            if (fp == NULL) {
                sprintf(name, "fc\\fc%.4x%01d.bin", 0x2262, 0);
                fp = fopen(name, "rb");
            }
            break;
        case 0:
            if (id >= 0x2000 && id <= 0x3fff) {
                fallback = (id & 0xf) + 0x2000;
            } else if (id >= 0x4000 && id <= 0x4fff) {
                fallback = 0x4000;
            } else if (id >= 0x5000 && id <= 0x5fff) {
                fallback = 0x5000;
            } else if (id >= 0x6000 && id <= 0x6fff) {
                fallback = 0x6050;
            }
            sprintf(name, "fc\\fc%.4x.bin", id);
            fp = fopen(name, "rb");
            if (fp == NULL) {
                sprintf(name, "fc\\fc%.4x.bin", id & 0xfff0);
                fp = fopen(name, "rb");
            }
            break;
        case 1:
            sprintf(name, "fc\\fch%.4x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 2:
            if (id >= 0x2000 && id <= 0x3fff) {
                fallback = 0x2000;
            }
            sprintf(name, "et\\ca%.4x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 3:
            fallback = 0;
            sprintf(name, "m\\m%.4x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 4:
            sprintf(name, "et\\a%.4x.bin", id);
            fp = fopen(name, "rb");
            s_prevDataFile = s_dataFile;
            s_dataFile = fp;
            return fp;
        case 5:
            fallback = 1;
            sprintf(name, "s\\sm%.3x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 6:
            fallback = 1;
            sprintf(name, "s\\sb%.3x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 7:
            sprintf(name, "s\\st%.3x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 8:
            sprintf(name, "s\\se%.3x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 9:
            if (id >= 0 && id <= 0xdf) {
                fallback = 1;
            }
            sprintf(name, "m\\ms%.4x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 10:
            if (id >= 0x2000 && id <= 0x3fff) {
                fallback = 0x2020;
            }
            sprintf(name, "p\\p%.4x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 11:
            sprintf(name, "et\\et%.4x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 12:
            sprintf(name, "et\\et%.4x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 13:
            sprintf(name, "gd%.4x.bin", id);
            fp = fopen(name, "rb");
            break;
        case 14:
            sprintf(name, "et\\id%.4x.bin", id);
            fp = fopen(name, "rb");
            s_prevDataFile = s_dataFile;
            s_dataFile = fp;
            return fp;
    }
    if (fp == NULL && retried != 1 && fallback != -1) {
        id = fallback;
        retried = 1;
        goto retry;
    }
    prev = s_dataFile;
    s_dataFile = fp;
    s_prevDataFile = prev;
    if (fp != NULL) {
        s_openedId = id;
        if (prev == NULL) {
            setvbuf(fp, s_fileBuffer, _IOFBF, sizeof(s_fileBuffer));
        } else {
            setvbuf(fp, s_nestedFileBuffer, _IOFBF, sizeof(s_nestedFileBuffer));
        }
    }
    return fp;
}

RVA(0x00002250, 0x28)
b32 CloseDataFile(FILE* fp) {
    if (fp != NULL) {
        fclose(fp);
    }
    s_dataFile = s_prevDataFile;
    s_prevDataFile = NULL;
    return false;
}

// @identity-TODO: these uncalled data-file results have no recoverable API
// names or signatures; the labels describe their neighboring interface only.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00002280, 0x3)
b32 GetLegacyDataFileOpenResult(void) {
    return false;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00002290, 0x3)
b32 GetLegacyDataFileReadResult(void) {
    return false;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000022a0, 0x3)
b32 GetLegacyDataFileSeekResult(void) {
    return false;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000022b0, 0x3)
b32 GetLegacyDataFileStatus(void) {
    return false;
}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000022c0, 0x3)
b32 GetLegacyDataFileLength(void) {
    return false;
}

// @identity-TODO: the one-valued legacy data-file result's role is unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000022d0, 0x5)
b16 IsLegacyDataFileReady(void) {
    return true;
}
