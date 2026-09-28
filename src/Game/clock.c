// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Character.h>
#include <Game/Clock.h>
#include <Game/FieldObject.h>
#include <Game/GameState.h>
#include <Game/SkillUse.h>
#include <Game/SpecialItems.h>
#include <Mem/Handle.h>
#include <Script/EventFlags.h>
#include <Util/BitSet.h>

#include <stdio.h>

// Set on the tick that ends a 24-tick round (HasTurnElapsed), and the round's
// tick count.
DATA(0x0007fe48)
static b16 s_turnElapsed;

DATA(0x0007fe4c)
static i16 s_roundTicks;

// The (bank, index) pairs of the event flags cleared at the full moon, ended
// by 0xff (data file 0x19).
DATA(0x0007fe50)
static i32 s_moonFlags;

DATA(0x00091544)
i16 g_tickElapsed;

DATA(0x00091560)
GameClock g_clock;

// Resets the clock (day 0, 0:00, new moon; 5 frames a tick, 2 of 60 minute
// steps) and loads the full-moon flag list once.
RVA(0x00020b00, 0x79)
void InitClock(void) {
    g_clock.frames = 1;
    g_clock.framesPerTick = 5;
    g_clock.minuteStep = 2;
    g_clock.minuteAcc = 0;
    g_clock.minuteLimit = 60;
    g_clock.days = 0;
    g_clock.moonTicks = 0;
    ResetClockPhaseAndTime(&g_clock);
    g_tickElapsed = 0;
    if (!s_moonFlags) {
        FILE* fp = OpenDataFile(0x19, 0xc, 0);
        s_moonFlags = ReadRawHandle(fp);
        CloseDataFile(fp);
    }
}

// Advances the clock by `minutes` and runs what the change brings (the moon
// flags, the countdown, special items, party timers); returns the change
// bits of TickClock.
RVA(0x00020b80, 0x6c)
GZ_ENUM_STORAGE(ClockUpdate, i16) AdvanceClock(u16 minutes) {
    GZ_ENUM_STORAGE(ClockUpdate, i16) changed = TickClock(minutes);
    ModifyEventFlag(0, 0x23, g_clock.moonPhase != 0xe);
    ModifyEventFlag(0, 0x25, g_clock.moonPhase != 0);
    ApplyClockChanges(changed);
    DrawDownCountdown(minutes);
    ExpireSpecialItems();
    TickPartyTimers(minutes);
    return changed;
}

// Adds `minutes`: 3, | 4 when an hour passed, | 8 a day, | 0x10 a moon phase.
// @early-stop: retail sums the day count in edx and stores it after the moon
// tick load; here it is eax and stored first.
RVA(0x00020bf0, 0xfe)
GZ_ENUM_STORAGE(ClockUpdate, i16) TickClock(u16 minutes) {
    GZ_ENUM_STORAGE(ClockUpdate, i16) changed = CLOCK_UPDATE_TICK | CLOCK_UPDATE_MINUTE;
    u16 total = minutes + g_clock.minute;
    u16 carry = total / 60;
    g_clock.minute = total % 60;
    if (carry) {
        changed = CLOCK_UPDATE_TICK | CLOCK_UPDATE_MINUTE | CLOCK_UPDATE_HOUR;
    }
    total = carry + g_clock.hour;
    carry = total / 24;
    g_clock.hour = total % 24;
    if (carry) {
        changed |= CLOCK_UPDATE_DAY;
    }
    g_clock.days += carry;
    total = g_clock.moonTicks + minutes;
    carry = total / 0x5f0;
    g_clock.moonTicks = total % 0x5f0;
    if (carry) {
        changed |= CLOCK_UPDATE_MOON;
    }
    total = carry + g_clock.moonPhase;
    g_clock.moonPhase = total % 28;
    return changed;
}

// On a new moon phase: clears flag 7/0xfd and the leader's flag 0x22, sets
// flag 10 of every live object, applies the phase to the party and the
// objects, and handles the full (0xe), new (0) and waning (0xf) moons.
RVA(0x00020cf0, 0x149)
void ApplyClockChanges(GZ_ENUM_STORAGE(ClockUpdate, i16) changed) {
    i16 i;
    u8* flags;
    Character* character;
    if (!(changed & CLOCK_UPDATE_MOON)) {
        return;
    }
    ModifyEventFlag(7, 0xfd, 0);
    flags = GetCharacterFlags(GetRosterCharacter(0));
    ClearBit(flags, 0x22);
    for (i = 0; i < 16; i++) {
        i16 object = GetLiveObject(i);
        if (object >= 0) {
            flags = GetCharacterFlags(GetCombatant(object));
            SetBit(flags, 10);
        }
    }
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (ApplyMoonPhase(character, g_clock.moonPhase)) {
            RecalcCharacterStats(character);
        }
    }
    for (i = 0; i < 16; i++) {
        i16 object = GetLiveObject(i);
        if (object >= 0) {
            character = GetCombatant(object);
            if (ApplyMoonPhase(character, g_clock.moonPhase)) {
                RecalcCharacterStats(character);
            }
        }
    }
    if (g_clock.moonPhase == 0xe) {
        ModifyEventFlag(0, 0x24, 0);
    }
    if (g_clock.moonPhase == 0) {
        ModifyEventFlag(0, 0x26, 0);
    }
    if (g_clock.moonPhase == 0xe) {
        ModifyEventFlag(7, 0xff, 0);
        ModifyEventFlag(7, 0xfe, 0);
    }
    if (g_clock.moonPhase == 0xf) {
        ClearMoonFlags();
    }
}

// Clears the full-moon flag list's event flags.
// @early-stop: retail addresses the list as [index + base]; the spellings
// tried give [base + index].
RVA(0x00020e40, 0x48)
void ClearMoonFlags(void) {
    u8* list;
    i16 i;
    if (!s_moonFlags) {
        return;
    }
    list = HandleReadPtr(s_moonFlags);
    for (i = 0; list[i] != 0xff; i += 2) {
        ModifyEventFlag(list[i], list[i + 1], 0);
    }
}

// One frame of the game clock: every framesPerTick frames a tick passes (24
// ticks end a round, HasTurnElapsed); unless `paused`, the ticks add
// minuteStep to the minute accumulator and each minuteLimit of it advances
// the clock a minute. Returns 1 on a tick, else 0 (or AdvanceClock's bits).
RVA(0x00020e90, 0x90)
GZ_ENUM_STORAGE(ClockUpdate, i16) TickGameClock(i16 paused) {
    s_turnElapsed = false;
    if (--g_clock.frames != 0) {
        return CLOCK_UPDATE_NONE;
    }
    g_clock.frames = g_clock.framesPerTick;
    if (++s_roundTicks >= 24) {
        s_roundTicks = 0;
        s_turnElapsed = true;
    }
    if (paused) {
        return CLOCK_UPDATE_TICK;
    }
    g_clock.minuteAcc += g_clock.minuteStep;
    if (g_clock.minuteAcc < g_clock.minuteLimit) {
        return CLOCK_UPDATE_TICK;
    }
    g_clock.minuteAcc -= g_clock.minuteLimit;
    return AdvanceClock(1);
}

RVA(0x00020f20, 0x7)
i16 HasTurnElapsed(void) {
    return s_turnElapsed;
}

RVA(0x00020f30, 0x21)
i16 SaveClock(FILE* fp) {
    return 1 - fwrite(&g_clock, sizeof(g_clock), 1, fp);
}

// Reads a saved clock (the time and moon only).
RVA(0x00020f60, 0x59)
i16 LoadClock(FILE* fp) {
    GameClock clock;
    i16 errors = 1 - fread(&clock, sizeof(clock), 1, fp);
    g_clock.days = clock.days;
    g_clock.moonTicks = clock.moonTicks;
    g_clock.moonPhase = clock.moonPhase;
    g_clock.hour = clock.hour;
    g_clock.minute = clock.minute;
    return errors;
}
