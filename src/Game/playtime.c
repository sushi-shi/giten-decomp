// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/PlayTime.h>

#include <stdio.h>

// The play clock: a 1/10000 ms remainder, milliseconds, seconds, minutes,
// hours, days (0..99) and hundreds of days. Saved field by field.
DATA(0x00075fd0)
static u16 s_playTimeFraction = 0;

DATA(0x00075fd4)
static u16 s_playTimeMilliseconds = 0;

DATA(0x00075fd8)
static u8 s_playTimeSeconds = 0;

DATA(0x00075fdc)
static u8 s_playTimeMinutes = 0;

DATA(0x00075fe0)
static u8 s_playTimeHours = 0;

DATA(0x00075fe4)
static u8 s_playTimeDays = 0;

DATA(0x00075fe8)
static u16 s_playTimeHundredDays = 0;

RVA(0x000035b0, 0x29)
void ResetPlayTime(void) {
    s_playTimeFraction = 0;
    s_playTimeMilliseconds = 0;
    s_playTimeSeconds = 0;
    s_playTimeMinutes = 0;
    s_playTimeHours = 0;
    s_playTimeDays = 0;
    s_playTimeHundredDays = 0;
}

// One tick is 16.7232 ms: 17 ms minus a 0.2768 ms correction carried in
// 1/10000 ms units.
RVA(0x000035e0, 0xcc)
void AdvancePlayTime(i16 ticks) {
    i16 i;
    for (i = 0; i < ticks; i++) {
        s_playTimeFraction += 7232;
        if (s_playTimeFraction >= 10000) {
            s_playTimeFraction -= 10000;
            s_playTimeMilliseconds++;
        }
        s_playTimeMilliseconds += 17;
        if (s_playTimeMilliseconds >= 1000) {
            s_playTimeMilliseconds -= 1000;
            if (++s_playTimeSeconds >= 60) {
                s_playTimeSeconds -= 60;
                if (++s_playTimeMinutes >= 60) {
                    s_playTimeMinutes -= 60;
                    if (++s_playTimeHours >= 24) {
                        s_playTimeHours -= 24;
                        if (++s_playTimeDays >= 100) {
                            s_playTimeDays -= 100;
                            s_playTimeHundredDays++;
                        }
                    }
                }
            }
        }
    }
}

// Both return nonzero when any field failed to transfer.
RVA(0x000036b0, 0x110)
i16 SavePlayTime(FILE* fp) {
    u16 fraction = s_playTimeFraction;
    u16 milliseconds = s_playTimeMilliseconds;
    u8 seconds = s_playTimeSeconds;
    u8 minutes = s_playTimeMinutes;
    u8 hours = s_playTimeHours;
    u8 days = s_playTimeDays;
    u16 hundredDays = s_playTimeHundredDays;
    i16 failed;
    failed = 1 - fwrite(&fraction, 2, 1, fp);
    failed |= 1 - fwrite(&milliseconds, 2, 1, fp);
    failed |= 1 - fwrite(&seconds, 1, 1, fp);
    failed |= 1 - fwrite(&minutes, 1, 1, fp);
    failed |= 1 - fwrite(&hours, 1, 1, fp);
    failed |= 1 - fwrite(&days, 1, 1, fp);
    failed |= 1 - fwrite(&hundredDays, 2, 1, fp);
    return failed;
}

RVA(0x000037c0, 0x117)
i16 LoadPlayTime(FILE* fp) {
    u16 fraction;
    u16 milliseconds;
    u8 seconds;
    u8 minutes;
    u8 hours;
    u8 days;
    u16 hundredDays;
    i16 failed;
    failed = 1 - fread(&fraction, 2, 1, fp);
    failed |= 1 - fread(&milliseconds, 2, 1, fp);
    failed |= 1 - fread(&seconds, 1, 1, fp);
    failed |= 1 - fread(&minutes, 1, 1, fp);
    failed |= 1 - fread(&hours, 1, 1, fp);
    failed |= 1 - fread(&days, 1, 1, fp);
    failed |= 1 - fread(&hundredDays, 2, 1, fp);
    s_playTimeFraction = fraction;
    s_playTimeMilliseconds = milliseconds;
    s_playTimeSeconds = seconds;
    s_playTimeMinutes = minutes;
    s_playTimeHours = hours;
    s_playTimeDays = days;
    s_playTimeHundredDays = hundredDays;
    return failed;
}
