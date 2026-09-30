#ifndef GITEN_UTIL_RANGE_H
#define GITEN_UTIL_RANGE_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/ViewDirection.h>
#include <Ints.h>

i32 PowerOfTwo(i16 exponent);
u8* OffsetByWord(u8* base, const u16* offset);
void* OffsetBy(void* base, u16 offset);
i32 ClampInt(i32 value, i32 lo, i32 hi);
i16 ClampShort(i16 value, i16 lo, i16 hi);
u16 ClampUShort(u16 value, u16 lo, u16 hi);
i16 ClampToShort(i32 value);
u16 ClampToUShort(i32 value);
i32 RoundToInt(double value);
i16 RoundToShort(double value);
i16 RandomUpTo(u8 range);
i16 RandomAverage(i16 lo, i16 hi, i16 samples);
i32 RandomPercent(i32 value, i32 lo, i32 hi);
i32 ClampDelta(i16 pos, i16 delta, i16 lo, i16 hi);
u16 CappedIncrease(u16 value, u16 amount, u16 max);
void AddCapped(u16* value, u16 amount, u16 max);
u16 CappedDecrease(u16 value, u16 amount, u16 min);
void SubCapped(u16* value, u16 amount, u16 min);
i32 AddClampInt(i32 a, i32 b, i32 lo, i32 hi);
i16 AddClampShort(i16 a, i16 b, i16 lo, i16 hi);
GZ_ENUM_RETURN(ViewDirection, i16) Direction4(i16 dx, i16 dy);
i16 RelativeDirection(i16 x0, i16 y0, i16 x1, i16 y1, GZ_ENUM_PARAM(ViewDirection, i16) facing);
i16 GridDistance(i16 x0, i16 y0, i16 x1, i16 y1);
void SortShortPair(i16* lo, i16* hi);

// A copy of `text` in a static buffer: '＠' ends the text when `keepMarks`
// is clear, and is skipped when it is set.
char* FilterTextMarks(const char* text, b16 keepMarks);

// The 256-byte filter buffer.
extern char g_filteredText[256];

#endif // GITEN_UTIL_RANGE_H
