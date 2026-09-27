// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Util/Range.h>
#include <Util/Text.h>

#include <math.h>
#include <mbstring.h>
#include <stdlib.h>

DATA(0x00078778)
char g_filteredText[256];

RVA(0x0000b810, 0x18)
i32 PowerOfTwo(i16 exponent) {
    i32 value = 1;
    i16 i;
    for (i = 0; i < exponent; i++) {
        value <<= 1;
    }
    return value;
}

RVA(0x0000b830, 0x10)
u8* OffsetByWord(u8* base, const u16* offset) {
    return base + *offset;
}

RVA(0x0000b840, 0x10)
void* OffsetBy(void* base, u16 offset) {
    u8* bytes = base;
    return bytes + offset;
}

RVA(0x0000b850, 0x17)
i32 ClampInt(i32 value, i32 lo, i32 hi) {
    if (value < lo) {
        value = lo;
    } else if (value > hi) {
        value = hi;
    }
    return value;
}

RVA(0x0000b870, 0x19)
i16 ClampShort(i16 value, i16 lo, i16 hi) {
    if (value < lo) {
        value = lo;
    } else if (value > hi) {
        value = hi;
    }
    return value;
}

RVA(0x0000b890, 0x19)
u16 ClampUShort(u16 value, u16 lo, u16 hi) {
    if (value < lo) {
        value = lo;
    } else if (value > hi) {
        value = hi;
    }
    return value;
}

RVA(0x0000b8b0, 0x18)
i16 ClampToShort(i32 value) {
    return ClampInt(value, -32768, 32767);
}

RVA(0x0000b8d0, 0x15)
u16 ClampToUShort(i32 value) {
    return ClampInt(value, 0, 0xffff);
}

// Rounds half away from zero.
RVA(0x0000b8f0, 0x30)
i32 RoundToInt(double value) {
    if (value < 0.0) {
        return -(i32)(fabs(value) + 0.5);
    }
    return (i32)(value + 0.5);
}

RVA(0x0000b920, 0x1c)
i16 RoundToShort(double value) {
    return ClampToShort(RoundToInt(value));
}

// 0..range inclusive.
RVA(0x0000b940, 0x17)
i16 RandomUpTo(u8 range) {
    return rand() % (range + 1);
}

// The mean of samples + 1 draws in lo..hi.
RVA(0x0000b960, 0x3f)
i16 RandomAverage(i16 lo, i16 hi, i16 samples) {
    i16 range = hi - lo;
    i16 sum = 0;
    i16 i;
    samples++;
    for (i = 0; i < samples; i++) {
        sum += RandomUpTo(range);
    }
    sum /= samples;
    return sum + lo;
}

// value scaled by a random percentage in 100+lo..100+hi, rounded.
RVA(0x0000b9a0, 0x4a)
i32 RandomPercent(i32 value, i32 lo, i32 hi) {
    i32 scaled = RandomAverage(lo + 100, hi + 100, 0) * value;
    i32 remainder = scaled % 100;
    scaled /= 100;
    if (remainder >= 50) {
        scaled++;
    }
    return scaled;
}

// The part of `delta` that keeps pos + delta within lo..hi.
RVA(0x0000b9f0, 0x2f)
i32 ClampDelta(i16 pos, i16 delta, i16 lo, i16 hi) {
    i32 result = delta;
    i32 upper = hi;
    i32 current = pos;
    i32 lower = lo;
    i32 end = pos + result;
    if (end > upper) {
        result = upper - current;
    } else if (end < lower) {
        result = lower - current;
    }
    return result;
}

RVA(0x0000ba20, 0x21)
u16 CappedIncrease(u16 value, u16 amount, u16 max) {
    u16 room;
    if (max < value) {
        return 0;
    }
    room = max - value;
    if (room >= amount) {
        return amount;
    }
    return room;
}

RVA(0x0000ba50, 0x20)
void AddCapped(u16* value, u16 amount, u16 max) {
    *value += CappedIncrease(*value, amount, max);
}

RVA(0x0000ba70, 0x21)
u16 CappedDecrease(u16 value, u16 amount, u16 min) {
    u16 room;
    if (value < min) {
        return 0;
    }
    room = value - min;
    if (room >= amount) {
        return amount;
    }
    return room;
}

RVA(0x0000baa0, 0x20)
void SubCapped(u16* value, u16 amount, u16 min) {
    *value -= CappedDecrease(*value, amount, min);
}

RVA(0x0000bac0, 0x1e)
i32 AddClampInt(i32 a, i32 b, i32 lo, i32 hi) {
    return ClampInt(a + b, lo, hi);
}

RVA(0x0000bae0, 0x2a)
i16 AddClampShort(i16 a, i16 b, i16 lo, i16 hi) {
    return ClampToShort(AddClampInt(a, b, lo, hi));
}

// Which of four directions (0..3) the offset (dx, dy) points in.
RVA(0x0000bb10, 0x5d)
i16 Direction4(i16 dx, i16 dy) {
    if (dx < 0) {
        if (dy < 0) {
            return dx - dy < 0 ? 3 : 0;
        }
        return dx + dy < 0 ? 3 : 2;
    }
    if (dy <= 0) {
        return dx + dy > 0;
    }
    return (dx - dy <= 0) + 1;
}

// Direction4 of (x1, y1) seen from (x0, y0) by an observer facing `facing`.
RVA(0x0000bb70, 0x62)
i16 RelativeDirection(i16 x0, i16 y0, i16 x1, i16 y1, i16 facing) {
    i16 dx = x1 - x0;
    i16 dy = y1 - y0;
    i16 rx;
    i16 ry;
    switch (facing) {
        case 1:
            rx = dy;
            ry = -dx;
            break;
        case 2:
            rx = -dx;
            ry = -dy;
            break;
        case 3:
            rx = -dy;
            ry = dx;
            break;
        default:
            rx = dx;
            ry = dy;
            break;
    }
    return Direction4(rx, ry);
}

// max(|x1 - x0|, |y1 - y0|).
RVA(0x0000bbe0, 0x2d)
i16 GridDistance(i16 x0, i16 y0, i16 x1, i16 y1) {
    i16 across = abs(x1 - x0);
    i16 down = abs(y1 - y0);
    if (down > across) {
        return down;
    }
    return across;
}

RVA(0x0000bc10, 0x1c)
void SortShortPair(i16* lo, i16* hi) {
    i16 a = *lo;
    i16 b = *hi;
    if (a > b) {
        *lo = b;
        *hi = a;
    }
}

RVA(0x0000bc30, 0x92)
char* FilterTextMarks(const char* text, i16 keepMarks) {
    char* output;
    u16 ch;
    g_filteredText[0] = '\0';
    output = g_filteredText;
    while ((ch = _mbsnextc(text)) != 0) {
        if (ch == 0x8197) {
            if (!keepMarks) {
                break;
            }
        } else {
            AppendTextChar(output, ch);
            output = _mbsinc(output);
        }
        text = _mbsinc(text);
    }
    return g_filteredText;
}
