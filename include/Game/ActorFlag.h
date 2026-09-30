#ifndef GITEN_GAME_ACTORFLAG_H
#define GITEN_GAME_ACTORFLAG_H

#include <EnumDomain.h>

// A character's own flags (its personalFlags, bank EVENT_FLAG_BANK_ACTOR to
// its script). ET0018 names the early flags; the invisibility skills and
// Estoma identify flags 0x21 and 0x22 with their consumers; Desaman marks
// a demon for removal when the skill resolves. Bits 2-7 record the outcomes
// of a talk. An alternate-script actor is anchored against pursuit and
// knockback. The table's later names (personality degrees from bit 11) do
// not fit the bits the code sets there.
GZ_ENUM_CONST_BEGIN(ActorFlag)
    ACTOR_FLAG_POINTS_READY = 0,
    ACTOR_FLAG_ROBOT = 1,
    ACTOR_FLAG_TALK_BATTLE = 2,
    ACTOR_FLAG_TALK_OVERCHARGE = 3,
    ACTOR_FLAG_TALK_INTRODUCED = 4,
    ACTOR_FLAG_TALK_APPROACHED = 5,
    ACTOR_FLAG_TALK_DISARMED = 6,
    ACTOR_FLAG_TALK_DOUBLE_DEMAND = 7,
    ACTOR_FLAG_BATTLE = 8,
    ACTOR_FLAG_FOUGHT = 9,
    ACTOR_FLAG_NOTICED = 10,
    ACTOR_FLAG_ANCHORED = 0x20,
    ACTOR_FLAG_INVISIBLE = 0x21,
    ACTOR_FLAG_ESTOMA = 0x22,
    ACTOR_FLAG_MOON_ACCURACY_EVASION_UP = 0x23,
    ACTOR_FLAG_MOON_ACCURACY_EVASION_DOWN = 0x24,
    ACTOR_FLAG_MAX_HP_DOUBLE_WEAPON_BOOST = 0x25,
    ACTOR_FLAG_MAX_POOLS_DOUBLE_ASH_PENDING = 0x26,
    ACTOR_FLAG_DESAMAN = 0x3f
GZ_ENUM_CONST_END(ActorFlag)

#endif // GITEN_GAME_ACTORFLAG_H
