#ifndef GITEN_GAME_CLOCK_H
#define GITEN_GAME_CLOCK_H

#include <rva.h>

#include <Ints.h>
#include <EnumDomain.h>

#include <stdio.h>

GZ_ENUM_BEGIN_SPLIT(ClockUpdate, i16)
CLOCK_UPDATE_NONE = 0, CLOCK_UPDATE_TICK = 1, CLOCK_UPDATE_MINUTE = 2, CLOCK_UPDATE_HOUR = 4,
                       CLOCK_UPDATE_DAY = 8, CLOCK_UPDATE_MOON = 16,
                       GZ_ENUM_END_SPLIT(ClockUpdate)

    // The game clock, saved and loaded as one record: days, the moon's ticks
    // (0x5f0 a phase) and phase (0..27: the column of the moon table and the
    // value OpSwitchOnMoonPhase switches on), the time of day, and the tick
    // pacing (a tick every framesPerTick frames; minuteStep per tick toward
    // minuteLimit per minute).
    typedef struct GameClock {
    i32 days;
    u16 moonTicks;
    u8 moonPhase;
    u8 hour;
    u8 minute;
    u8 frames;
    u8 framesPerTick;
    u8 minuteStep;
    u16 minuteAcc;
    u16 minuteLimit;
} GameClock;

static __inline void ResetClockPhaseAndTime(GameClock* clock) {
    clock->moonPhase = 0;
    clock->hour = 0;
    clock->minute = 0;
}

extern GameClock g_clock;

GZ_ENUM_STORAGE(ClockUpdate, i16) AdvanceClock(u16 minutes);
GZ_ENUM_STORAGE(ClockUpdate, i16) TickClock(u16 minutes);
void ApplyClockChanges(GZ_ENUM_STORAGE(ClockUpdate, i16) changed);
void ClearMoonFlags(void);
GZ_ENUM_STORAGE(ClockUpdate, i16) TickGameClock(i16 paused);

// Steps a character's moon-driven personal flags; the count changed.
i16 ApplyMoonPhase(struct Character* character, i16 keep);

// The party's hourly timers for `minutes` (-1 when off).
i16 TickPartyTimers(u16 minutes);

// scriptvars' countdown.
void DrawDownCountdown(u16 amount);

i16 GetMoonPhase(void);

// The clock as minutes since day 0.
u32 GetClockMinutes(void);

// The time until the moon reaches `phase`.
u16 TimeUntilMoonPhase(i16 phase);

// Twice the moon table's entry for `row` at the current phase (defined in
// Game/scenecell.c).
// @identity-TODO: what the table holds is unrecovered.
i16 GetMoonValue(i16 row);
i32 ScaleByMoonValue(i32 value, i16 row, i16 percent);

// @identity-TODO: What 0x181e0 (stores two words at 0x47be78/0x47be7c) plus PushGameState(0x15)
// do with the script position OpStartCountdown saved at 0x81690/0x81694 is unproven; decoding
// 0x181e0 and state 0x15 would confirm it runs the handler. It is scriptvars.c's function.
b16 FireCountdownEvent(void);

// The clock's save-file section.
i16 LoadClock(FILE* fp);
i16 SaveClock(FILE* fp);

void InitClock(void);
void LoadMoonTable(void);

#endif // GITEN_GAME_CLOCK_H
