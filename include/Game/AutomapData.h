#ifndef GITEN_GAME_AUTOMAPDATA_H
#define GITEN_GAME_AUTOMAPDATA_H

#include <rva.h>

#include <Game/GameState.h>
#include <Ints.h>

// The automap's explored-cell bitmaps: per area a handle to a level table
// (a count, then one bitmap handle per level); the current level's bitmap is
// kept unpacked in one buffer and written back when the level changes.

// IsAutomapCellHidden's result for an unexplored cell (0 when explored).
#define AUTOMAP_CELL_HIDDEN 0x100

typedef struct AutomapBitmapHeader {
    i16 width;
    i16 height;
    u8 pad04[2];
    i16 size; // bytes of `bits`
} AutomapBitmapHeader;

typedef struct AutomapBitmap {
    AutomapBitmapHeader header;
    u8 bits[0x1000];
} AutomapBitmap;

static __inline u16 GetAutomapBitmapSize(const AutomapBitmapHeader* header) {
    return header->size + sizeof(AutomapBitmapHeader);
}

static __inline i16 AutomapCellIndex(AutomapBitmap* bitmap, i16 x, i16 y) {
    return y * bitmap->header.width + x;
}

typedef struct AutomapLevelHeader {
    i16 count;
    u8 pad02[2];
} AutomapLevelHeader;

typedef struct AutomapLevels {
    AutomapLevelHeader header;
    i32 levels[1];
} AutomapLevels;

static __inline i16 GetAutomapLevelCount(AutomapLevels* levels) {
    return levels->header.count;
}

#ifdef __cplusplus
#define GetAutomapLevelHandle(table, level)                                                        \
    (static_cast<const AutomapLevels*>(table)->levels[(level)])
#else
#define GetAutomapLevelHandle(table, level) (((const AutomapLevels*)(table))->levels[(level)])
#endif

static __inline void SetAutomapLevelHandle(AutomapLevels* levels, i16 level, i32 bitmap) {
    levels->levels[level] = bitmap;
}

static __inline i32 GetAutomapLevelTableSize(i16 count) {
    return count * 4 + 4;
}

void InitAutomap(void);
void AllocAutomapLevels(void);
void FreeAutomap(void);
void StoreAutomapLevel(void);
void LoadAutomapLevel(i16 area, i16 level);
i16 IsAutomapCellHidden(i16 x, i16 y, i16 area, i16 level);

// @identity-TODO: the current area and its level count (0x421700/0x421710,
// from the area map header).
i16 GetCurrentArea(void);

i16 GetAreaLevelCount(void);

#endif // GITEN_GAME_AUTOMAPDATA_H
