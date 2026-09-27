#ifndef GITEN_MEM_ALLOC_H
#define GITEN_MEM_ALLOC_H

#include <rva.h>

#include <Ints.h>

// The platform layer's heap wrappers (malloc/realloc/free with 16-bit sizes).
// @identity-TODO: their owning TU in the platform band is unclaimed; these are
// label-only declarations.

// count * size bytes, zero-filled; NULL when the heap is exhausted.
RVA_DECL(0x00049640)
void* AllocCleared(u16 count, u16 size);

RVA_DECL(0x00049620)
void* ReallocBlock(void* block, u16 size);

// Frees a block (NULL is ignored) and returns NULL.
RVA_DECL(0x000496c0)
void* FreeBlock(void* block);

#endif // GITEN_MEM_ALLOC_H
