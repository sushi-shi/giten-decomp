#ifndef GITEN_GAME_EQUIPREQUIREMENTS_H
#define GITEN_GAME_EQUIPREQUIREMENTS_H

#include <Game/Character.h>
#include <Game/ItemRecord.h>

// Keep the comparisons in the caller: their short-circuit exits differ.
#define LacksItemRequiredStats(member, record, bonus)                                              \
    (GetStatTotal((member), STAT_VITALITY) + (bonus) < GetItemRequiredVitality(record)             \
     || GetStatTotal((member), STAT_DEXTERITY) + (bonus) < GetItemRequiredDexterity(record))

#endif // GITEN_GAME_EQUIPREQUIREMENTS_H
