#ifndef GITEN_GAME_CHARACTERPOOLS_H
#define GITEN_GAME_CHARACTERPOOLS_H

#include <EnumDomain.h>
#include <Util/CurMax.h>
#include <Enums.h>

// clang-format off
GZ_ENUM_FLAGS_BEGIN(CharacterPoolMask, u8)
    POOL_MASK_NONE = 0,
    POOL_MASK_HP = 1,
    POOL_MASK_MP = 2,
    POOL_MASK_BOTH = POOL_MASK_HP | POOL_MASK_MP
GZ_ENUM_END(CharacterPoolMask);
// clang-format on

typedef struct CharacterPools {
    CurMax hp;
    CurMax mp;
} CharacterPools;

#endif // GITEN_GAME_CHARACTERPOOLS_H
