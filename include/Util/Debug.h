#ifndef GITEN_UTIL_DEBUG_H
#define GITEN_UTIL_DEBUG_H

#include <rva.h>

#ifdef __cplusplus
extern "C" {
#endif

    // A developer trace; the release build compiles it to an empty body.
    RVA_DECL(0x00049610)
    void DebugTrace(const char* message);

#ifdef __cplusplus
}
#endif

#endif // GITEN_UTIL_DEBUG_H
