#ifndef GITEN_GAME_HUMANID_H
#define GITEN_GAME_HUMANID_H

#include <EnumDomain.h>

// The human members' character ids, named by the surnames InitCharacters
// gives them (Newton has no surname).
GZ_ENUM_CONST_BEGIN(HumanId)
    HUMAN_KATSURAGI = 0,
    HUMAN_TACHIBANA = 2,
    HUMAN_ASUKA = 4,
    HUMAN_NISHINO = 5,
    HUMAN_HAYASAKA = 6,
    HUMAN_SONODA = 7,
    HUMAN_KAMIKAWA = 8,
    HUMAN_SOUMA = 9,
    HUMAN_YAMASE = 10,
    HUMAN_KIRISHIMA = 11,
    HUMAN_NEWTON = 13,
    HUMAN_YAMADA = 14,
    HUMAN_TACHIKAWA = 15
GZ_ENUM_CONST_END(HumanId)

#endif // GITEN_GAME_HUMANID_H
