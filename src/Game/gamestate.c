// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Clock.h>
#include <Game/GameState.h>
#include <Game/MapArea.h>
#include <Game/Party.h>

#include <string.h>

DATA(0x00091080)
Party g_party;

RVA(0x0003c6a0, 0x48)
i16 GetMapValue(GZ_ENUM_PARAM(MapValueSelector, i16) which) {
    switch (which) {
        case MAP_VALUE_AREA:
            return g_party.field.pos.area;
        case MAP_VALUE_LEVEL:
            return g_party.field.pos.level;
        case MAP_VALUE_X:
            return g_party.field.pos.x;
        case MAP_VALUE_Y:
            return g_party.field.pos.y;
    }
    return 0;
}

RVA(0x0003c6f0, 0xb8)
void InitNewGame(void) {
    i16 i;
    g_party.field.pos.area = MAP_AREA_HATSUDAI;
    g_party.field.pos.level = 8;
    g_party.field.pos.x = 2;
    g_party.field.pos.y = 1;
    g_party.field.pos.direction = VIEW_NORTH;
    g_party.field.moveState = 0;
    g_party.field.turnsLeft = 0;
    g_party.field.moveCommand = 0;
    g_party.status.rosterCapacity = ROSTER_SIZE;
    memset(g_party.roster, 0, sizeof(g_party.roster));
    ResetClockPhaseAndTime(&g_clock);
    InitCharacters();
    SetRosterEntry(ROSTER_LEADER, GetCharacter(0));
    SetPartySlot(0, ROSTER_LEADER);
    for (i = 1; i < PARTY_SIZE; i++) {
        SetPartySlot(i, PARTY_SLOT_EMPTY);
    }
    g_party.status.automapFixed = false;
    g_party.status.navigationFixed = false;
}

RVA(0x0003c7b0, 0x1e)
MapCoord GetMapCoord(void) {
    MapCoord coord;
    coord.x = g_party.field.pos.x;
    coord.y = g_party.field.pos.y;
    return coord;
}

RVA(0x0003c7d0, 0x6)
MapPosition* GetMapPosition(void) {
    return &g_party.field.pos;
}

RVA(0x0003c7e0, 0xf)
i16 GetRosterCapacity(void) {
    return g_party.status.rosterCapacity;
}

RVA(0x0003c7f0, 0x6)
u8 GetMapArea(void) {
    return g_party.field.pos.area;
}

RVA(0x0003c800, 0x6)
u8 GetMapLevel(void) {
    return g_party.field.pos.level;
}
