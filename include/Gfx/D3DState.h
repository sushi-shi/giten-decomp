#ifndef GITEN_GFX_D3DSTATE_H
#define GITEN_GFX_D3DSTATE_H

#include <Win32.h>
#include <Enums.h>

#include <d3d.h>

// The Direct3D state the C texture code shares with the platform layer, with
// C linkage (the platform layer includes this inside extern "C").

// @identity-TODO: g_d3dDevice renders the 3D view (viewport g_viewport);
// g_screenDevice is a second device on the same back buffer with a
// full-screen viewport, used for 2D overlays.
extern IDirect3DDevice2* g_d3dDevice;

#define SetDeviceAlphaBlend(device, enabled)                                                       \
    do {                                                                                           \
        IDirect3DDevice2_SetRenderState((device), D3DRENDERSTATE_ALPHABLENDENABLE, (enabled));     \
        IDirect3DDevice2_SetRenderState((device), D3DRENDERSTATE_STIPPLEDALPHA, !(enabled));       \
    } while (0)

#define SetTextureFiltering(filter)                                                                \
    do {                                                                                           \
        IDirect3DDevice2_SetRenderState(g_d3dDevice, D3DRENDERSTATE_TEXTUREMAG, filter);           \
        IDirect3DDevice2_SetRenderState(g_d3dDevice, D3DRENDERSTATE_TEXTUREMIN, filter);           \
    } while (0)

// The texture pixel format the setup chose.
extern DDPIXELFORMAT g_textureFormat;

// The device kind the setup chose (the ramp device takes the ambient light
// state instead of per-vertex shading).
// clang-format off
GZ_ENUM_BEGIN(D3DDeviceKind)
    D3D_DEVICE_HAL = 0,
    D3D_DEVICE_MMX = 1,
    D3D_DEVICE_RAMP = 2
GZ_ENUM_END(D3DDeviceKind);
// clang-format on

extern D3DDeviceKind g_deviceType;

#endif // GITEN_GFX_D3DSTATE_H
