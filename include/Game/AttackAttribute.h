#ifndef GITEN_GAME_ATTACKATTRIBUTE_H
#define GITEN_GAME_ATTACKATTRIBUTE_H

#include <EnumDomain.h>
#include <Enums.h>
#include <Ints.h>

// Attack attribute stored as a byte in skill records and as a word in the
// current attack state; used to index resistance.
// The names identify resistance columns; they do not restrict which skill
// kinds can use a column. Attribute 10 uses a fixed resistance value of 50.
// clang-format off
GZ_ENUM_BEGIN(AttackAttribute)
    ATTACK_ATTRIBUTE_SWORD = 0,
    ATTACK_ATTRIBUTE_PHYSICAL = 1,
    ATTACK_ATTRIBUTE_FIRE = 2,
    ATTACK_ATTRIBUTE_ICE = 3,
    ATTACK_ATTRIBUTE_FORCE = 4,
    ATTACK_ATTRIBUTE_ELECTRIC = 5,
    ATTACK_ATTRIBUTE_EXPEL = 6,
    ATTACK_ATTRIBUTE_MENTAL = 7,
    ATTACK_ATTRIBUTE_DARK = 8,
    ATTACK_ATTRIBUTE_RUIN = 9,
    ATTACK_ATTRIBUTE_FIXED_HALF_RESISTANCE = 10
GZ_ENUM_END(AttackAttribute)
// clang-format on

#endif // GITEN_GAME_ATTACKATTRIBUTE_H
