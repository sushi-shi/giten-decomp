#ifndef GITEN_GAME_SKILLID_H
#define GITEN_GAME_SKILLID_H

#include <EnumDomain.h>

// Skills the code singles out, named after their names in the skill file
// (GetSkillName).
GZ_ENUM_CONST_BEGIN(SkillId)
    SKILL_ID_NONE = -1,
    SKILL_SWORD_ATTACK = 1,
    SKILL_AGI = 0x10,
    SKILL_MAHOROGI = 0x57,
    SKILL_NOELEM = 0x5d,
    SKILL_TRAESTO = 0x79,
    SKILL_TRAPORT = 0x7a,
    SKILL_TRAFURI = 0x7b,
    SKILL_SABBATMA = 0x7d,
    SKILL_SELF_RECOVERY = 250,
    SKILL_FUSION = 0x10e
GZ_ENUM_CONST_END(SkillId)

#endif // GITEN_GAME_SKILLID_H
