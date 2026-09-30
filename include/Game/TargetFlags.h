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
// Encoded values that open the field/roster, roster, or party/roster picker.
#define TARGET_SELECT_FIELD_OR_ROSTER 0x10
#define TARGET_SELECT_ROSTER_ONLY 0x11
#define TARGET_SELECT_PARTY_OR_ROSTER 0x30
// clang-format on

static __inline i32 TargetFlagsSelectSelf(i16 flags) {
    return flags & TARGET_SELF;
}

static __inline b32 TargetFlagsSelectActorGroup(i16 flags) {
    return (flags & (TARGET_ACTOR_SIDE | TARGET_EXPAND_GROUP))
           == (TARGET_ACTOR_SIDE | TARGET_EXPAND_GROUP);
}

#endif // GITEN_GAME_TARGETFLAGS_H
