#ifndef GITEN_GAME_MAPAREA_H
#define GITEN_GAME_MAPAREA_H

#include <EnumDomain.h>

// Map areas, named after the area names in their map files (translated or
// romanized), or after the shopkeeper debug traces in OpLoadSprite. Several
// map files can carry one place's name; the name goes to the one the code
// tests.
// @identity-TODO: the other area ids, including WallOverrideArea's alternate-wall
// areas, are not named yet.
GZ_ENUM_BEGIN_SPLIT(MapAreaId, u8)
    MAP_AREA_VIRTUAL_DUNGEON = 0x01,
    MAP_AREA_SHINJUKU_UNDERGROUND = 0x06,
    MAP_AREA_SHINJUKU_TOCHO = 0x09,
    MAP_AREA_MY_CITY = 0x0a,
    MAP_AREA_HARAJUKU = 0x10,
    MAP_AREA_SHANSHAN_CITY = 0x13,
    MAP_AREA_ICHIGAYA = 0x14,
    MAP_AREA_MILLENNIUM_HEADQUARTERS = 0x18,
    MAP_AREA_GINZA_UNDERGROUND = 0x1a,
    MAP_AREA_KANDA_UNDERGROUND = 0x1b,
    MAP_AREA_AKIHABARA_STATION_BUILDING = 0x1f,
    MAP_AREA_AMEYA_PLAZA = 0x21,
    MAP_AREA_OHANAYASHIKI = 0x24,
    MAP_AREA_ASAKUSA_SUBWAY_BUILDING = 0x25,
    MAP_AREA_SHINAGAWA_HOTEL_ILLUSION = 0x29,
    MAP_AREA_ENTERTAINMENT_DISTRICT = 0x2d,
    MAP_AREA_ROPPONGI = 0x2e,
    MAP_AREA_SHIBUYA = 0x30,
    MAP_AREA_EBISU_GARDEN = 0x34,
    MAP_AREA_BAEL_CASTLE = 0x35,
    MAP_AREA_CHIYODA_LINE = 0x3d,
    MAP_AREA_MILLENNIUM_HOSPITAL = 0x53,
    MAP_AREA_RINKAI_COLISEUM = 0x56,
    MAP_AREA_HATSUDAI = 0x82,
    MAP_AREA_RESISTANCE_FRONTLINE_BASE = 0x83,
    MAP_AREA_OCHANOMIZU = 0x8a
GZ_ENUM_END_SPLIT(MapAreaId)

#endif // GITEN_GAME_MAPAREA_H
