#ifndef GITEN_SCRIPT_SCENARIOFLAG_H
#define GITEN_SCRIPT_SCENARIOFLAG_H

#include <EnumDomain.h>

// Scenario flags the code tests or sets, named after the developers'
// event-flag name table on the disc (ET0018), which numbers them on through
// bank EVENT_FLAG_BANK_SCENARIO into EVENT_FLAG_BANK_SCENARIO_2.
GZ_ENUM_CONST_BEGIN(ScenarioFlag)
    SCENARIO_FULL_MOON_1 = 0x23,
    SCENARIO_FULL_MOON_2 = 0x24,
    SCENARIO_NEW_MOON_1 = 0x25,
    SCENARIO_NEW_MOON_2 = 0x26
GZ_ENUM_CONST_END(ScenarioFlag)

// Flags of bank EVENT_FLAG_BANK_SCENARIO_2 from the same table.
GZ_ENUM_CONST_BEGIN(ScenarioFlag2)
    SCENARIO_2_HEROINE_REVIVAL_1 = 0xc,
    SCENARIO_2_RAINBOW_BRIDGE = 13
GZ_ENUM_CONST_END(ScenarioFlag2)

#endif // GITEN_SCRIPT_SCENARIOFLAG_H
