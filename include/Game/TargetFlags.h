#ifndef GITEN_GAME_TARGETFLAGS_H
#define GITEN_GAME_TARGETFLAGS_H

#include <Enums.h>
#include <Ints.h>

// clang-format off
GZ_ENUM_BEGIN(TargetFlags)
    TARGET_ACTOR_SIDE = 1,
    TARGET_TARGET_SIDE = 2,
    TARGET_SELF = 4,
    TARGET_EXPAND_GROUP = 8
GZ_ENUM_END(TargetFlags);
// clang-format on

static __inline i32 TargetFlagsSelectSelf(i16 flags) {
    return flags & TARGET_SELF;
}

static __inline i32 TargetFlagsSelectActorGroup(i16 flags) {
    return (flags & (TARGET_ACTOR_SIDE | TARGET_EXPAND_GROUP))
           == (TARGET_ACTOR_SIDE | TARGET_EXPAND_GROUP);
}

#endif // GITEN_GAME_TARGETFLAGS_H
