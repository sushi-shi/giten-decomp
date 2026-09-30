#ifndef GITEN_SCRIPT_SCENARIOFLAG_H
#define GITEN_SCRIPT_SCENARIOFLAG_H

#include <EnumDomain.h>

// Scenario flags the code tests or sets, mostly named after the developers'
// event-flag name table on the disc (ET0018), which numbers them on through
// bank EVENT_FLAG_BANK_SCENARIO into EVENT_FLAG_BANK_SCENARIO_2. A new game
// starts with every flag set; the event clears its flag (AdvanceClock clears
// SCENARIO_FULL_MOON_1 while the moon is full).
GZ_ENUM_CONST_BEGIN(ScenarioFlag)
    SCENARIO_FULL_MOON_1 = 0x23,
    SCENARIO_FULL_MOON_2 = 0x24,
    SCENARIO_NEW_MOON_1 = 0x25,
    SCENARIO_NEW_MOON_2 = 0x26,
    SCENARIO_WITHERED_LOVER_LEFT_ARM = 137,
    SCENARIO_WITHERED_LOVER_RIGHT_ARM = 138,
    SCENARIO_WITHERED_LOVER_LEFT_LEG = 139,
    SCENARIO_WITHERED_LOVER_RIGHT_LEG = 140,
    SCENARIO_WITHERED_LOVER_CHEST = 141,
    SCENARIO_WITHERED_LOVER_ABDOMEN = 142
GZ_ENUM_CONST_END(ScenarioFlag)

// Flags of bank EVENT_FLAG_BANK_SCENARIO_2; the head flag comes from the
// timed-item table's correspondence with ItemId.
GZ_ENUM_CONST_BEGIN(ScenarioFlag2)
    SCENARIO_2_HEROINE_REVIVAL_1 = 0xc,
    SCENARIO_2_RAINBOW_BRIDGE = 13,
    SCENARIO_2_STEP_DAMAGE_SUPPRESSED = 0x2b,
    SCENARIO_2_PARTY_COMBAT_EFFECTS_SUPPRESSED = 0x2d,
    SCENARIO_2_FALLEN_RESCUE_SUPPRESSED = 0x4f,
    SCENARIO_2_WITHERED_LOVER_HEAD = 93,
    SCENARIO_2_KATSURAGI_CIVILIAN_PORTRAIT = 0x5e,
    SCENARIO_2_TACHIBANA_CIVILIAN_PORTRAIT = 0x75
GZ_ENUM_CONST_END(ScenarioFlag2)

#endif // GITEN_SCRIPT_SCENARIOFLAG_H
