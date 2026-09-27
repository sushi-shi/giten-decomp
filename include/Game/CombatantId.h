#ifndef GITEN_GAME_COMBATANTID_H
#define GITEN_GAME_COMBATANTID_H

#include <Ints.h>

static __inline i16 PartyCombatantId(i16 position) {
    return -1 - position;
}

static __inline i16 CombatantPartyPosition(i16 id) {
    return -1 - id;
}

#endif // GITEN_GAME_COMBATANTID_H
