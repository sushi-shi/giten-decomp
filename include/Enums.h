#ifndef GITEN_ENUMS_H
#define GITEN_ENUMS_H

#include <Ints.h>

// Keep enum domains scoped under modern C++ while retaining MSVC 5 and C syntax.
#if defined(__cplusplus) && __cplusplus >= 202002L
#define GZ_ENUM_BEGIN(name) enum class name : i32 {
#define GZ_ENUM_END(name)                                                                          \
    }                                                                                              \
    ;                                                                                              \
    using enum name;
#else
#define GZ_ENUM_BEGIN(name) typedef enum name {
#define GZ_ENUM_END(name)                                                                          \
    }                                                                                              \
    name;
#endif

#endif // GITEN_ENUMS_H
