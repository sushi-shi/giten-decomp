#ifndef GITEN_PLATFORM_D3DAPP_H
#define GITEN_PLATFORM_D3DAPP_H

#include <rva.h>

#include <Platform/DeviceSettings.h>
#include <Platform/Direct3D.h>

// The DirectX objects the Direct3D layer creates and releases (C++ only).

extern LPDIRECTDRAW2 g_ddraw2;
extern LPDIRECT3D2 g_d3d;
extern LPDIRECTDRAWSURFACE g_primarySurface;
extern LPDIRECTDRAWSURFACE g_zBuffer;

// The second device on the render target (see g_d3dDevice, declared with C
// linkage in Gfx/D3DState.h through Platform/GameApi.h).
extern IDirect3DDevice2* g_screenDevice;

// The render target both devices draw into, and the viewports of the 3D view
// and of the whole screen.
extern LPDIRECTDRAWSURFACE g_renderTarget;
extern IDirect3DViewport2* g_viewport;
extern IDirect3DViewport2* g_screenViewport;

// The display's 16-bit channel masks.
extern DWORD g_redMask;
extern DWORD g_greenMask;
extern DWORD g_blueMask;

// Set when the device filters textures bilinearly (InitDirect3D copies it
// from g_deviceSettings.caps by device kind); the treasure box draws with nearest
// filtering in between.
extern BOOL g_bilinearFiltering;

// @identity-TODO: a surface or palette the room builder (0x44a100) creates;
// released with the devices.
extern IUnknown* g_roomObject;

BOOL InitDirectDraw(void);
// Enumerates the Direct3D devices into g_deviceSettings.caps (InitDirectDraw).
void QueryD3DDevices(void);
// The z-buffer depth a device supports: 16, else 24, else 32 or 0.
DWORD __fastcall ZBufferDepth(D3DDEVICEDESC* desc);
BOOL InitDirect3D(void);
BOOL InitDirectSound(void);
BOOL InitDirectInput(void);
void AcquireInput(BOOL acquire);
u8 PollMouseButtons(void);
void ClearScreenSurfaces(void);
void ReleaseDirectX(void);
BOOL RestoreSurfaces(BOOL restore);
u16 ReadSurfaceWord(LPDIRECTDRAWSURFACE surface, i32 x, i32 pitch, i32 y);

// D3DCaps.blendMode: the device alpha-blends (otherwise it stipples).
#define BLEND_MODE_ALPHA 2

// The 16-bit display depth, and a 5-bit channel mask.
#define PIXEL_BITS_16 16
#define CHANNEL_MASK_5BIT 0x1f

// The display settings the entry TU loaded (the DirectDraw driver's GUID).
extern struct DisplayConfig g_displayConfig;

// The Direct3D devices' descriptions as the device query keeps them.
// @identity-TODO: the 0x38 bytes before the caps and the tail after the
// descriptions are unrecovered.
struct D3DDeviceInfo {
    u8 reserved1[0x38];
    DDCAPS driverCaps;
    DDCAPS helCaps;
    D3DDEVICEDESC halDesc;
    D3DDEVICEDESC mmxDesc;
    D3DDEVICEDESC rampDesc;
    u8 reserved2[0x1b4];
};

// @identity-TODO: the application instance; owned by the entry TU,
// placeholder extern.
extern HINSTANCE g_instance;

#endif // GITEN_PLATFORM_D3DAPP_H
