// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Clock.h>
#include <Game/GameState.h>
#include <Game/Party.h>
#include <Game/Stats.h>
#include <Util/Range.h>

#include <math.h>
#include <string.h>

DATA(0x00091080)
FieldState g_field;

DATA(0x00091090)
i16 g_savedDirection;

DATA(0x00091092)
i16 g_party[6];

DATA(0x0009109e)
FieldStatus g_fieldStatus;

DATA(0x000910a0)
Character* g_roster[32];

RVA(0x0003c6a0, 0x48)
i16 GetMapValue(i16 which) {
    switch (which) {
        case 0:
            return g_field.pos.area;
        case 1:
            return g_field.pos.level;
        case 2:
            return g_field.pos.x;
        case 3:
            return g_field.pos.y;
    }
    return 0;
}

// @early-stop scheduling: retail clears the roster after the field stores;
// cl hoists rep stos before them. Array-object addressing and an inline roster
// clear helper preserve the hoist. The later instructions agree.
RVA(0x0003c6f0, 0xb8)
void InitNewGame(void) {
    i16 i;
    g_fieldStatus.count = 32;
    g_field.pos.area = 0x82;
    g_field.pos.level = 8;
    g_field.pos.x = 2;
    g_field.pos.y = 1;
    g_field.pos.direction = 0;
    g_field.moveState = 0;
    g_field.turnsLeft = 0;
    g_field.moveCommand = 0;
    memset(g_roster, 0, sizeof(g_roster));
    ResetClockPhaseAndTime(&g_clock);
    InitCharacters();
    SetRosterEntry(0, GetCharacter(0));
    SetPartySlot(0, 0);
    for (i = 1; i < 6; i++) {
        SetPartySlot(i, -1);
    }
    g_fieldStatus.automapFixed = 0;
    g_fieldStatus.navigationFixed = 0;
}

RVA(0x0003c7b0, 0x1e)
MapCoord GetMapCoord(void) {
    MapCoord coord;
    coord.x = g_field.pos.x;
    coord.y = g_field.pos.y;
    return coord;
}

RVA(0x0003c7d0, 0x6)
MapPosition* GetMapPosition(void) {
    return &g_field.pos;
}

RVA(0x0003c7e0, 0xf)
i16 GetFieldCount(void) {
    return g_fieldStatus.count;
}

RVA(0x0003c7f0, 0x6)
u8 GetMapArea(void) {
    return g_field.pos.area;
}

RVA(0x0003c800, 0x6)
u8 GetMapLevel(void) {
    return g_field.pos.level;
}

// Action speed from agility, reduced by armor defense scaled by vitality;
// never below 1.
RVA(0x0003c810, 0x97)
i16 ComputeActionSpeed(Character* character) {
    double base = sqrt(GetStatTotal(character, STAT_AGILITY)) * 4.0;
    double burden = (u16)SumArmorDefenseBonus(character);
    double root = sqrt(GetStatTotal(character, STAT_VITALITY));
    u16 speed;
    if (root < 1.0) {
        root = 1.0;
    }
    burden /= root;
    burden *= 0.25;
    base -= burden;
    speed = RoundToShort(base);
    if (speed < 1) {
        speed = 1;
    }
    return speed;
}
