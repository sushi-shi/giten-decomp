#ifndef GITEN_GAME_ATTACKATTRIBUTE_H
#define GITEN_GAME_ATTACKATTRIBUTE_H

#include <EnumDomain.h>
#include <Ints.h>

// Attack attribute stored in skill records and used to index resistance.
// The skill and item tables identify the elemental, expulsion, mental and
// dark groups. Attribute 10 uses a fixed resistance value of 50 instead of
// indexing the actor's resistance array.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(AttackAttribute, u8)
    ATTACK_ATTRIBUTE_FIRE = 2,
    ATTACK_ATTRIBUTE_ICE = 3,
    ATTACK_ATTRIBUTE_FORCE = 4,
    ATTACK_ATTRIBUTE_ELECTRIC = 5,
    ATTACK_ATTRIBUTE_EXPEL = 6,
    ATTACK_ATTRIBUTE_MENTAL = 7,
    ATTACK_ATTRIBUTE_DARK = 8,
    ATTACK_ATTRIBUTE_FIXED_HALF_RESISTANCE = 10
GZ_ENUM_END_SPLIT(AttackAttribute)
// clang-format on

#endif // GITEN_GAME_ATTACKATTRIBUTE_H
