// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Clock.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptVars.h>

RVA(0x00036610, 0x18)
void OpSwitchOnMoonPhase(i16 call) {
    SwitchOnValue(GetMoonPhase() + 1, call, 0);
}

// @identity-TODO: what table 0x47beac (28 bytes per row, row = actor byte +0x1f8, column =
// phase) holds is unrecovered.
RVA(0x00036630, 0x38)
void OpGetActorMoonValue(void) {
    i16 index = ReadLongVarIndex();
    i16 half = GetMoonValue(g_curScript->actor->moonRow) / 2;
    SetScriptLongVar(index, half);
}

RVA(0x00036670, 0xf)
void OpAdvanceClock(void) {
    AdvanceClock(ReadScriptValue());
}

RVA(0x00036680, 0x2b)
void OpGetTicksUntilMoonPhase(void) {
    i16 phase = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, TimeUntilMoonPhase(phase));
}

RVA(0x000366b0, 0x16)
void OpGetDayCount(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, g_clock.days);
}

// Stores the time of day in minutes.
RVA(0x000366d0, 0x2c)
void OpGetTimeOfDay(void) {
    i16 index = ReadLongVarIndex();
    i16 minutes = g_clock.hour * 60 + g_clock.minute;
    SetScriptLongVar(index, minutes);
}
