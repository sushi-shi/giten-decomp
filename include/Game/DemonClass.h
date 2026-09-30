#ifndef GITEN_GAME_DEMONCLASS_H
#define GITEN_GAME_DEMONCLASS_H

#include <EnumDomain.h>
#include <Ints.h>

// The sixteen major races, romanized from the class-name block of the demon
// table (GetDemonClassName).
// clang-format off
GZ_ENUM_BEGIN_SPLIT(DemonClass, u8)
    DEMON_CLASS_UNKNOWN = 0,
    DEMON_CLASS_TENSHIN = 1,
    DEMON_CLASS_KISHIN = 2,
    DEMON_CLASS_MAZOKU = 3,
    DEMON_CLASS_HITEN = 4,
    DEMON_CLASS_RYUUZOKU = 5,
    DEMON_CLASS_CHOUZOKU = 6,
    DEMON_CLASS_JUUZOKU = 7,
    DEMON_CLASS_KIZOKU = 8,
    DEMON_CLASS_SEIREI = 9,
    DEMON_CLASS_JAREI = 10,
    DEMON_CLASS_GEDOU = 11,
    DEMON_CLASS_HITO = 12,
    DEMON_CLASS_MAJIN = 13,
    DEMON_CLASS_MUSEIBUTSU = 14,
    DEMON_CLASS_SOUMOKU = 15
GZ_ENUM_END_SPLIT(DemonClass)
// clang-format on

#endif // GITEN_GAME_DEMONCLASS_H
