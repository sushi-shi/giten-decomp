#ifndef GITEN_GAME_HUMANTITLE_H
#define GITEN_GAME_HUMANTITLE_H

#include <EnumDomain.h>
#include <Ints.h>

// The seven human titles, romanized from the human-title block of the demon
// table (GetHumanTitleName).
// clang-format off
GZ_ENUM_BEGIN_SPLIT(HumanTitle, u8)
    HUMAN_TITLE_GUSHA = 0,
    HUMAN_TITLE_INOUSHA = 1,
    HUMAN_TITLE_KAKUSEISHA = 2,
    HUMAN_TITLE_CHOUJIN = 3,
    HUMAN_TITLE_DOUSHI = 4,
    HUMAN_TITLE_SHINJIN = 5,
    HUMAN_TITLE_KAMI = 6
GZ_ENUM_END_SPLIT(HumanTitle)
// clang-format on

#endif // GITEN_GAME_HUMANTITLE_H
