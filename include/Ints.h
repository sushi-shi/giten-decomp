#ifndef GITEN_INTS_H
#define GITEN_INTS_H

typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef __int64 i64;
typedef unsigned __int64 u64;

typedef i8 b8;
typedef i16 b16;
typedef i32 b32;
typedef i64 b64;
typedef u8 ub8;
typedef u16 ub16;
typedef u32 ub32;
typedef u64 ub64;

#ifndef __cplusplus
#define true 1
#define false 0
#endif

#endif // GITEN_INTS_H
