#ifndef GITEN_GAME_SKILLFAMILY_H
#define GITEN_GAME_SKILLFAMILY_H

#include <EnumDomain.h>
#include <Ints.h>

// clang-format off
GZ_ENUM_BEGIN_SPLIT(SkillFamily, u8)
    SKILL_FAMILY_AGI = 1,
    SKILL_FAMILY_ZAN = 2,
    SKILL_FAMILY_DAWM = 3,
    SKILL_FAMILY_ZIO = 4,
    SKILL_FAMILY_BUFU = 5
GZ_ENUM_END_SPLIT(SkillFamily)
// clang-format on

#endif // GITEN_GAME_SKILLFAMILY_H
