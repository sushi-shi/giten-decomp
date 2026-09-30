#ifndef GITEN_GAME_WALLKIND_H
#define GITEN_GAME_WALLKIND_H

#include <EnumDomain.h>
#include <Enums.h>

// Each map cell stores one four-bit wall kind per side. Kind 6 is passable and
// has no 3D quad, but retains a solid class in the map and field overlays.
// @identity-TODO: the in-world identities of kinds 6 and 11 are unrecovered.
GZ_ENUM_BEGIN_SPLIT(WallKind, u8)
    WALL_KIND_NONE = 0,
    WALL_KIND_PASSABLE_NO_QUAD = 6,
    WALL_KIND_PLAIN_ATLAS_ALTERNATE = 11
GZ_ENUM_END_SPLIT(WallKind)

#endif // GITEN_GAME_WALLKIND_H
