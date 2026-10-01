#ifndef GITEN_GFX_DDERROR_H
#define GITEN_GFX_DDERROR_H

#include <Win32.h>

#include <Util/Debug.h>

void TraceDDrawError(HRESULT result);
void TraceD3DError(HRESULT result);

#define TraceD3DCallError(context, result)                                                         \
    do {                                                                                           \
        DebugTrace(context);                                                                       \
        TraceD3DError(result);                                                                     \
    } while (0)

#endif // GITEN_GFX_DDERROR_H
