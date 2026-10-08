#ifndef GITEN_PLATFORM_WINMAIN_H
#define GITEN_PLATFORM_WINMAIN_H

#include <rva.h>

#include <Win32.h>

// The game window and its client rectangle.
extern HWND g_mainWindow;
// @identity-TODO: placeholder extern until the display rectangle is claimed.
extern RECT g_windowRect;

LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

// Resets the display state (window and view rectangles to 640x480, the
// clear-blit parameters) and seeds rand.
void ResetDisplayGlobals(void);

void RenderSceneMode(BOOL draw);
void RenderPictureMode(BOOL draw);
void RenderBlankMode(BOOL draw);

b32 CreateMainWindow(HINSTANCE instance);

#endif // GITEN_PLATFORM_WINMAIN_H
