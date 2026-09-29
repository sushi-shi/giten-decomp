#ifndef GITEN_INTS_H
#define GITEN_INTS_H

// The build variants. The matching build defines neither flag; the play
// build defines GITEN_BUGFIX (docs/play.md, docs/bugs.md).
//
// GITEN_COMPAT: fixes for undefined behaviour, crashes, hangs, operating
// system, driver and Wine differences, and frame pacing.
// GITEN_BUGFIX: fixes for defects in the retail game logic (dropped input,
// soft-locks, data errors). It implies GITEN_COMPAT.
//
// Kept here rather than in a header of its own: another #include or macro
// definition in every TU perturbs MSVC 5's register allocation and temporary
// numbering, and the matching build must not see one.
#if defined(GITEN_BUGFIX) && !defined(GITEN_COMPAT)
#define GITEN_COMPAT
#endif

typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef __int64 i64;
typedef unsigned __int64 u64;

typedef i16 b16;
typedef i32 b32;
typedef u8 ub8;
typedef u32 ub32;

#ifndef __cplusplus
#define true 1
#define false 0
#endif

#endif // GITEN_INTS_H
