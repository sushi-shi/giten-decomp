#ifndef GITEN_GAME_TARGETCOUNTMODE_H
#define GITEN_GAME_TARGETCOUNTMODE_H

#include <EnumDomain.h>
#include <Ints.h>

// The high nibble of a skill or item's target-count byte. Values 1..14
// select that many targets in order; zero scatters hits at random.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(TargetCountMode, u8)
    TARGET_COUNT_RANDOM_TARGETS = 0,
    TARGET_COUNT_ALL_TARGETS = 15
GZ_ENUM_END_SPLIT(TargetCountMode)
// clang-format on

#define TARGET_COUNT_NIBBLE_MASK 0xf
#define TARGET_COUNT_MODE_SHIFT 4

#endif // GITEN_GAME_TARGETCOUNTMODE_H
