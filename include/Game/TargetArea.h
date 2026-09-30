#ifndef GITEN_GAME_TARGETAREA_H
#define GITEN_GAME_TARGETAREA_H

#include <EnumDomain.h>
#include <Ints.h>

// Target-area selectors whose behavior is established by the combat target
// collector. Codes 6 and 7 scan the same visible grid; their distinct record
// meanings remain unknown.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(TargetArea, u8)
    TARGET_AREA_SELECTED_ONLY = 0,
    TARGET_AREA_LINE = 2,
    TARGET_AREA_VISIBLE_FIRST = 6,
    TARGET_AREA_VISIBLE_LAST = 7,
    TARGET_AREA_WEAPON_HITS = 0xff
GZ_ENUM_END_SPLIT(TargetArea)
// clang-format on

#endif // GITEN_GAME_TARGETAREA_H
