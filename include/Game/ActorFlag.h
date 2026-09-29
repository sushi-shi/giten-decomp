#ifndef GITEN_GAME_ACTORFLAG_H
#define GITEN_GAME_ACTORFLAG_H

#include <EnumDomain.h>

// A character's own flags (its personalFlags, bank EVENT_FLAG_BANK_ACTOR to
// its script), named after the developers' event-flag name table on the disc
// (ET0018). Bits 2-7 record the outcomes of a talk. The table's later names
// (personality degrees from bit 11) do not fit the bits the code sets there.
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
    ACTOR_FLAG_NOTICED = 10
GZ_ENUM_CONST_END(ActorFlag)

#endif // GITEN_GAME_ACTORFLAG_H
