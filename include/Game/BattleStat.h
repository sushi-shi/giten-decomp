#ifndef GITEN_GAME_BATTLESTAT_H
#define GITEN_GAME_BATTLESTAT_H

#include <Enums.h>

// The derived-stat array's four groups of six words, and the training kind
// that raises each group's level (its first word). The fourth, human-only
// group is trained by analyzing, talking and summoning; its identity is
// unrecovered.
GZ_ENUM_BEGIN(BattleStatGroup)
    BATTLE_GROUP_WEAPON = 0,
    BATTLE_GROUP_GUN = 1,
    BATTLE_GROUP_MAGIC = 2,
    BATTLE_GROUP_COUNT = 4
GZ_ENUM_END(BattleStatGroup)

#define BATTLE_STATS_PER_GROUP 6

// Known combat outputs in the 24-entry derived-stat array.
// clang-format off
GZ_ENUM_BEGIN(BattleStatIndex)
    BATTLE_STAT_WEAPON_LEVEL = 0,
    BATTLE_STAT_WEAPON_ACCURACY = 2,
    BATTLE_STAT_WEAPON_POWER = 3,
    BATTLE_STAT_WEAPON_EVASION = 4,
    BATTLE_STAT_WEAPON_DEFENSE = 5,
    BATTLE_STAT_GUN_LEVEL = 6,
    BATTLE_STAT_GUN_ACCURACY = 8,
    BATTLE_STAT_GUN_POWER = 9,
    BATTLE_STAT_GUN_EVASION = 10,
    BATTLE_STAT_GUN_DEFENSE = 11,
    BATTLE_STAT_MAGIC_LEVEL = 12,
    BATTLE_STAT_MAGIC_ACCURACY = 14,
    BATTLE_STAT_MAGIC_POWER = 15,
    BATTLE_STAT_MAGIC_EVASION = 16,
    BATTLE_STAT_MAGIC_DEFENSE = 17,
GZ_ENUM_END(BattleStatIndex);
// clang-format on

#endif // GITEN_GAME_BATTLESTAT_H
