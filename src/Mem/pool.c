// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Gfx/VramCell.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Mem/Pool.h>

#include <string.h>

// Eight-row screen cells with four, three, two or one colour planes.
DATA(0x00064288)
static const i16 s_poolElementSizes[5] =
    {8 * sizeof(CellRow), 8 * sizeof(CellRow3), 8 * sizeof(CellRow2), 8 * sizeof(CellRow1), 0};

// @dead-code
// Zero-ref: no direct caller or address-taking reaches this allocator.
// @identity-TODO: retail uses the pool pointer as a numbered memory handle
// when storing a chunk; the unused pool API mixes the two representations.
RVA(0x00003440, 0x93)
void AllocatePool(ElementPool* pools, i16 index, u16 count) {
    u16 chunks;
    u16 remaining;
    u16 size;
    u16 chunk;
    ResetPool(pools, index);
    pools[index].count = count;
    size = pools[index].elementSize;
    chunks = count >> 8;
    if (count & 0xff) {
        chunks++;
    }
    remaining = count;
    for (chunk = 0; chunk < chunks; chunk++) {
        u16 part = remaining < 256 ? remaining : 256;
        i32 handle = AllocArrayHandle(part, size);
        ElementPool* writable = HandleWritePtr((i32)pools);
        writable[index].chunks[chunk] = handle;
        remaining -= part;
    }
}

RVA(0x000034e0, 0x3f)
void ResetPool(ElementPool* pools, i16 index) {
    ElementPool* pool;
    if (index < 0 || index > 4) {
        return;
    }
    pool = &pools[index];
    pool->elementSize = s_poolElementSizes[index];
    pool->count = 0;
    memset(pool->chunks, 0, sizeof(pool->chunks));
}

// Allocates and resets the four element pools.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00003520, 0x2b)
ElementPool* CreatePools(void) {
    ElementPool* pools = AllocCleared(1, 4 * sizeof(ElementPool));
    i16 index;
    for (index = 0; index < 4; index++) {
        ResetPool(pools, index);
    }
    return pools;
}

// @identity-TODO: this zero-valued pool result has no recovered API signature or name.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00003550, 0x3)
b32 GetLegacyPoolResetResult(void) {
    return false;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00003560, 0xe)
b32 FreePools(i32 pools) {
    return FreeHandle(pools);
}

// The chunk holding element `id` of pool `index`; the pool array itself is a
// memory handle.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00003570, 0x32)
void* GetPoolChunk(i32 pools, i16 index, u32 id) {
    ElementPool* poolArray = HandleReadPtr(pools);
    return HandleWritePtr(poolArray[index].chunks[(id >> 8) & 0xf]);
}
