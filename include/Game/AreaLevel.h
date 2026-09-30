#ifndef GITEN_GAME_AREALEVEL_H
#define GITEN_GAME_AREALEVEL_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/MapArea.h>
#include <Ints.h>

struct MapSpawn;

// The area-level switch (areamap.c) and its callees. Codegen constraint: kept
// apart from <Game/FieldMap.h> (field.c's RunFieldEncounter) and
// <Game/AreaMap.h> (reached by winmain through Platform/GameApi.h).

GZ_ENUM_BEGIN(WallTextureKind)
    WALL_TEXTURE_UNLIT = 6,
    WALL_TEXTURE_MAP_OVERRIDE = 10
GZ_ENUM_END(WallTextureKind)

#ifdef __cplusplus
extern "C" {
#endif

    void SetAreaFlagPreservation(i16 area, i16 level);
    void SelectAreaLevel(i16 level, i16 force);

    void SpawnMapObjects(i16 cellCode);
    i16 IsSpawnEnabled(struct MapSpawn* entry);

    // The spawn rate word (+7) of a cell code's object table (0 without one).
    // @identity-TODO: label-only.
    i16 GetCellSpawnRate(i16 code);

    // fieldobj's spawn pacing.
    void SetSpawnInterval(i16 interval);

    // @identity-TODO: label-only; named from their bodies.

    void MarkRoomRegions(u8* walls, u8* doors, i16 width, i16 height);

    i16 RoomRegionsChanged(u8* walls, u8* doors, i16 width, i16 height);

    // Clears the 0x100-byte field memory (unless it is kept).
    void ResetFieldMemory(void);

    RVA_DECL(0x00057e80)
    void LoadWallTextures(GZ_ENUM_PARAM(WallTextureKind, i16) wallSet, i16 variant);

#ifdef __cplusplus
}
#endif

#endif // GITEN_GAME_AREALEVEL_H
