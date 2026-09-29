#ifndef GITEN_SCRIPT_ACTORSPOILKIND_H
#define GITEN_SCRIPT_ACTORSPOILKIND_H

#include <EnumDomain.h>

// What GrantActorSpoil gives from the script actor.
GZ_ENUM_BEGIN_SPLIT(ActorSpoilKind, i16)
    ACTOR_SPOIL_MACCA = 0,
    ACTOR_SPOIL_MAGNETITE = 1,
    ACTOR_SPOIL_EXPERIENCE = 2
GZ_ENUM_END_SPLIT(ActorSpoilKind)

#endif // GITEN_SCRIPT_ACTORSPOILKIND_H
