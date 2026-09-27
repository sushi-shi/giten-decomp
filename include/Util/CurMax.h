#ifndef GITEN_UTIL_CURMAX_H
#define GITEN_UTIL_CURMAX_H

#include <Ints.h>

// A current/maximum counter pair (formatted "cur/max").
typedef struct CurMax {
    u16 cur;
    u16 max;
} CurMax;

#define InitCurMax(pool, value) ((pool)->cur = (pool)->max = (value))

char* FormatCurMax(CurMax value, char* buf, i16 width);

#endif // GITEN_UTIL_CURMAX_H
