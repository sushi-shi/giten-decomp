#ifndef GITEN_SCRIPT_ACTORALERTMODE_H
#define GITEN_SCRIPT_ACTORALERTMODE_H

#include <EnumDomain.h>

// OpSetActorAlert's level: alert the script actor, alert it and cut its wait
// to one tick, or only extend its wait to ACTION_WAIT_EXTENDED ticks.
GZ_ENUM_BEGIN_SPLIT(ActorAlertMode, i16)
    ACTOR_ALERT_NORMAL = 0,
    ACTOR_ALERT_IMMEDIATE = 1,
    ACTOR_ALERT_DELAY = 2
GZ_ENUM_END_SPLIT(ActorAlertMode)

#endif // GITEN_SCRIPT_ACTORALERTMODE_H
