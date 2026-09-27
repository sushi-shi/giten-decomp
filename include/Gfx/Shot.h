#ifndef GITEN_GFX_SHOT_H
#define GITEN_GFX_SHOT_H

#include <rva.h>

#include <Ints.h>
#include <Math/Vec3.h>

#include <stdio.h>

// The shot file block: kind count, byte offsets from the block start, then
// arrays of Body records by strength. The load buffer has 256-byte capacity.
typedef union ShotFile {
    struct {
        i16 count;
        u16 offsets[1];
    } table;
    u8 bytes[256];
} ShotFile;

// The shot kinds: for each, an array of starting motions by strength. The
// pointers follow the count on a 2-byte boundary.
typedef struct ShotTable {
    u16 count;
    Body* kinds[63];
} ShotTable;

static __inline Body* GetShotKindMotion(ShotTable* table, i16 kind) {
    return table->kinds[kind];
}

// @identity-TODO: a flying effect (shot) moved through the field between a
// start and a target point; which battle effects use it is not recovered.
void LoadShotTable(FILE* fp);
i16 SetShotPath(i16 fromX, i16 fromY, i16 toX, i16 toY);
i16 StartShot(i16 kind, i16 strength, i16 height);
i16 StepShot(void);
Vec3* ShotRelativePoint(i16 x, i16 y, i16 depth, Vec3* out);
void StopShot(void);
void GetShotScaledPosition(i16* x, i16* y, i16* z);
void GetShotPosition(i16* x, i16* y, i16* z);
void SetShotPosition(i16 x, i16 y, i16 z);
void SetShotPower(i16 power);
i16 GetShotPower(void);

// Flies effect `effect` from one map cell to another in its own game state.
// @identity-TODO: `mode` and `rise` are unused; their names follow the caller.
void LaunchShot(i16 effect, i16 mode, i16 rise, i16 fromX, i16 fromY, i16 toX, i16 toY);

void LoadEffectTables(void);
i16 RunShotState(void);
i16 RunClosingEffectState(void);

#endif // GITEN_GFX_SHOT_H
