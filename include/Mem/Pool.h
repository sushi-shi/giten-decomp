#ifndef GITEN_MEM_POOL_H
#define GITEN_MEM_POOL_H

#include <Ints.h>

// Fixed-size element storage: up to 16 chunks of at most 256 elements; an
// element id is (chunk << 8) | slot.
typedef struct ElementPool {
    i32 chunks[16]; // handles
    u16 count;
    i16 elementSize;
} ElementPool;

void AllocatePool(ElementPool* pools, i16 index, u16 count);
void ResetPool(ElementPool* pools, i16 index);
ElementPool* CreatePools(void);
i32 FreePools(i32 pools);
void* GetPoolChunk(i32 pools, i16 index, u32 id);

#endif // GITEN_MEM_POOL_H
