#ifndef GITEN_GAME_DEMONRACE_H
#define GITEN_GAME_DEMONRACE_H

#include <EnumDomain.h>

// The demon races, romanized from the race-name block of the demon table
// (GetDemonRaceName).
// clang-format off
GZ_ENUM_BEGIN_SPLIT(DemonRace, u8)
    RACE_UNKNOWN = 0,
    RACE_MASHIN = 1,
    RACE_MEGAMI = 2,
    RACE_HAKAISHIN = 3,
    RACE_KISHIN = 4,
    RACE_DAITENSHI = 5,
    RACE_JIBOSHIN = 6,
    RACE_RYUUJIN = 7,
    RACE_REICHOU = 8,
    RACE_SHINJUU = 9,
    RACE_SEIJUU = 10,
    RACE_SEIREI = 11,
    RACE_TENSHI = 12,
    RACE_KOUTENSHI = 13,
    RACE_RYUUOU = 14,
    RACE_YOUCHOU = 15,
    RACE_MAJUU = 16,
    RACE_YOUMA = 17,
    RACE_YAMA = 18,
    RACE_SUIYOU = 19,
    RACE_YOUSEI = 20,
    RACE_JUSEI = 21,
    RACE_KIJO = 22,
    RACE_TOUKI = 23,
    RACE_YOUKI = 24,
    RACE_JIREI = 25,
    RACE_MAJIN = 26,
    RACE_DEMONOID = 27,
    RACE_JUUJIN = 28,
    RACE_ISHTAR_BELIEVER = 29,
    RACE_BAEL_BELIEVER = 30,
    RACE_KYOUJIN = 31,
    RACE_HEISHI = 32,
    RACE_HITO = 33,
    RACE_INU = 34,
    RACE_GEDOU = 35,
    RACE_KAIRAI = 36,
    RACE_MACHINE = 37,
    RACE_AKURYOU = 38,
    RACE_SHIKI = 39,
    RACE_YUUKI = 40,
    RACE_JAKI = 41,
    RACE_YOUJU = 42,
    RACE_YOUJUU = 43,
    RACE_KYOUCHOU = 44,
    RACE_JARYUU = 45,
    RACE_GEMA = 46,
    RACE_DATENSHI = 47,
    RACE_AKUMA = 48,
    RACE_JASHIN = 49,
    RACE_MAOU = 50
GZ_ENUM_END_SPLIT(DemonRace)
// clang-format on

#endif // GITEN_GAME_DEMONRACE_H
