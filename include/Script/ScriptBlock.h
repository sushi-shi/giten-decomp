#ifndef GITEN_SCRIPT_SCRIPTBLOCK_H
#define GITEN_SCRIPT_SCRIPTBLOCK_H

#include <rva.h>

#include <Ints.h>

#include <stdio.h>

// A loaded block of script code. `id` names it: the script data file it was
// loaded from, 0xe0 + n for a field layer's scripts, 0xff for an object's.
// `code` is a memory handle that starts with a 256-entry table of
// {offset, length} words, one per entry point.
typedef struct ScriptBlock {
    i16 id;
    i32 code;
} ScriptBlock;

static __inline i16 GetScriptBlockId(const ScriptBlock* block) {
    return block->id;
}

// A script position: a code block handle and the offset within it.
typedef struct ScriptEntry {
    u32 code;
    i16 pc;
} ScriptEntry;

static __inline void ClearScriptEntry(ScriptEntry* entry) {
    entry->code = 0;
    entry->pc = 0;
}

// Reads a script block from `file` (allocating the block when NULL).
ScriptBlock* ReadScriptBlock(ScriptBlock* block, void* file);

// Frees the block and its code; returns NULL.
RVA_DECL(0x0003ad60)
ScriptBlock* FreeScriptBlock(ScriptBlock* block);

// One entry point's code in a block's code handle: its offset from the
// handle start and its length. The 256 ranges come first; the code starts at
// 0x400 (one zero byte per entry in a fresh block).
typedef struct ScriptRange {
    u16 offset;
    u16 length;
} ScriptRange;

typedef struct ScriptCode {
    ScriptRange ranges[256];
    u8 bytes[1];
} ScriptCode;

static __inline ScriptRange* GetScriptRange(ScriptCode* code, i16 entry) {
    return &code->ranges[entry];
}

ScriptBlock* NewScriptBlock(void);
u16 ReadScriptEntry(ScriptBlock* block, FILE* fp);
u8* ResizeScriptEntry(ScriptBlock* block, i16 entry, i16 length);
void ShiftScriptEntries(ScriptCode* code, i16 entry, i16 delta, u16 size);

ScriptBlock* LoadScriptBlock(ScriptBlock* block, i16 file);
ScriptEntry MakeScriptEntry(ScriptBlock* block, i16 entry);

#endif // GITEN_SCRIPT_SCRIPTBLOCK_H
