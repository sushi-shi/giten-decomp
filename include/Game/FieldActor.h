#ifndef GITEN_GAME_FIELDACTOR_H
#define GITEN_GAME_FIELDACTOR_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/CharacterCore.h>
#include <Game/FieldLayerIndex.h>
#include <Game/MapCoord.h>
#include <Game/ViewDirection.h>

// @identity-TODO: this aggregate boundary is inferred from borrower accesses;
// no standalone allocation or whole-record copy proves its original extent
// or type name.
typedef struct FieldActor {
    CharacterCore core;
    MapCoord pos;
    GZ_ENUM_STORAGE(ViewDirection, i16) direction;
    u8 pad219;
    u8 byte21a;
    u8 byte21b;
    u8 pad21c;
    i16 word21d;
    i16 word21f;
    i16 word221;
    i16 hidden;
} FieldActor;

static __inline CharacterCore* GetFieldActorCore(FieldActor* character) {
    return character ? &character->core : NULL;
}

// The field map's actor slots (enemies and NPCs on the map) and the enemy
// group slots they are spawned from.

// @identity-TODO: that `kind` is an enemy/demon record id and the file loaded via 0x32d0 is
// its picture is inferred; decode 0x57f20/0x58040. Defined in Game/fieldobj.c.
void LoadEnemyGroupSlot(GZ_ENUM_PARAM(FieldLayerIndex, i16) slot, i16 id);

// @identity-TODO: That 0xeca0 spawns a wandering enemy group near the party is inferred (random
// offset around g_party.field, 0xdbf0 with random group); decode 0xdbf0/0xe8f0 to confirm.
i16 TickEnemySpawnTimer(void);

#endif // GITEN_GAME_FIELDACTOR_H
