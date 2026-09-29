#ifndef GITEN_GAME_MAPAREA_H
#define GITEN_GAME_MAPAREA_H

#include <EnumDomain.h>

// Map areas, named where the shopkeeper debug traces in OpLoadSprite place
// them.
// @identity-TODO: the other area ids, including WallOverrideArea's alternate-wall
// areas, have no recovered geographical names.
GZ_ENUM_BEGIN_SPLIT(MapAreaId, u8)
    MAP_AREA_ICHIGAYA = 0x14,
    MAP_AREA_GINZA_UNDERGROUND = 0x1a,
    MAP_AREA_KANDA_UNDERGROUND = 0x1b,
    MAP_AREA_AMEYA_PLAZA = 0x21,
    MAP_AREA_SHINAGAWA_HOTEL_ILLUSION = 0x29,
    MAP_AREA_ROPPONGI = 0x2e,
    MAP_AREA_RINKAI_COLISEUM = 0x56,
    MAP_AREA_HATSUDAI = 0x82,
    MAP_AREA_RESISTANCE_FRONTLINE_BASE = 0x83
GZ_ENUM_END_SPLIT(MapAreaId)

#endif // GITEN_GAME_MAPAREA_H
