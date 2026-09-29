#ifndef GITEN_GAME_FIELDACTOR_H
#define GITEN_GAME_FIELDACTOR_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/Character.h>
#include <Game/FieldLayerIndex.h>

// The field map's actor slots (enemies and NPCs on the map) and the enemy
// group slots they are spawned from.

// @identity-TODO: that `kind` is an enemy/demon record id and the file loaded via 0x32d0 is
// its picture is inferred; decode 0x57f20/0x58040. Defined in Game/fieldobj.c.
void LoadEnemyGroupSlot(GZ_ENUM_PARAM(FieldLayerIndex, i16) slot, i16 id);

// @identity-TODO: That 0xeca0 spawns a wandering enemy group near the party is inferred (random
// offset around g_party.field, 0xdbf0 with random group); decode 0xdbf0/0xe8f0 to confirm.
i16 TickEnemySpawnTimer(void);

#endif // GITEN_GAME_FIELDACTOR_H
