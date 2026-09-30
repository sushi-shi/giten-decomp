#ifndef GITEN_GAME_WALLKIND_H
#define GITEN_GAME_WALLKIND_H

#include <EnumDomain.h>
#include <Enums.h>

// Each map cell stores one four-bit wall kind per side. Kind 2 has a matching
// door record whose flag can bar passage; kind 11 has a matching door record
// but is never barred by that record. Both use the plain-wall atlas region.
// Kind 6 blocks movement but has no 3D quad or map-overlay wall. Kind 12 is
// drawn as a wall but permits movement.
// @identity-TODO: the in-world identities of kinds 6 and 12 are unrecovered.
GZ_ENUM_BEGIN_SPLIT(WallKind, u8)
    WALL_KIND_NONE = 0,
    WALL_KIND_DOOR = 1,
    WALL_KIND_FLAG_BARRED_DOOR = 2,
    WALL_KIND_SOLID = 3,
    WALL_KIND_INVISIBLE_BARRIER = 6,
    WALL_KIND_UNBARRED_DOOR = 11,
    WALL_KIND_PASSABLE_WALL = 12
GZ_ENUM_END_SPLIT(WallKind)

#endif // GITEN_GAME_WALLKIND_H
