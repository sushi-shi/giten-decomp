#ifndef GITEN_PLATFORM_WINMM_H
#define GITEN_PLATFORM_WINMM_H

// Multimedia and the windowsx global-pointer macros for the C++ platform
// layer only: <Win32.h> stays lean so the C units keep their TU state.

#include <Win32.h>

#include <Platform/WindowsX.h>

#include <mmsystem.h>

#endif // GITEN_PLATFORM_WINMM_H
