#ifndef GITEN_PLATFORM_DIRECT3D_H
#define GITEN_PLATFORM_DIRECT3D_H

// Direct3D for the C++ platform layer only: <Win32.h> stays free of it so the
// C units keep their translation-unit state.
#include <Win32.h>

#define D3D_OVERLOADS
#include <d3d.h>

#endif // GITEN_PLATFORM_DIRECT3D_H
