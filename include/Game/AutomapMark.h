#ifndef GITEN_GAME_AUTOMAPMARK_H
#define GITEN_GAME_AUTOMAPMARK_H

#include <EnumDomain.h>
#include <Ints.h>

// @identity-TODO: the other automap image indices remain unnamed.
GZ_ENUM_BEGIN_SPLIT(AutomapMark, u8)
    MAP_MARK_NPC = 4,
    MAP_MARK_OBJECT = 5,
    MAP_MARK_EXIT = 7,
    MAP_MARK_STAIRS_UP = 8,
    MAP_MARK_STAIRS_DOWN = 9
GZ_ENUM_END_SPLIT(AutomapMark)

#endif // GITEN_GAME_AUTOMAPMARK_H
