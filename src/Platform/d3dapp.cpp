// The Direct3D layer: the matrix helpers, the DirectDraw/Direct3D error
// traces, the device setup and the sound player. One object: its D3D inline
// COMDATs (0x448f90..0x4492a0) follow the sound code and are first used by the
// identity matrix's initializer at its start.

#include <rva.h>

#include <Win32.h>

#include <Gfx/DDError.h>
#include <Gfx/DisplayConfig.h>
#include <Giten/Resource.h>
#include <Platform/Com.h>
#include <Platform/D3DApp.h>
#include <Platform/D3DMath.h>
#include <Platform/GameApi.h>
#include <Platform/InputSound.h>
#include <Platform/Scene3D.h>
#include <Platform/WinMain.h>
#include <Sound/WaveResource.h>

#include <math.h>
#include <string.h>

DATA(0x000841e0)
LPDIRECTDRAWSURFACE g_zBuffer;

// The DirectDraw object (C linkage through Gfx/DDraw.h).
DATA(0x000841e4)
IDirectDraw* g_ddraw;

DATA(0x000841e8)
DDPIXELFORMAT g_textureFormat;

DATA(0x00084208)
LPDIRECTSOUND g_directSound;

DATA(0x0008420c)
IDirect3DDevice2* g_screenDevice;

DATA(0x00084250)
IUnknown* g_roomObject;

DATA(0x00084254)
LPDIRECTSOUNDBUFFER g_shortSoundBuffer;

DATA(0x00084258)
LPDIRECTSOUNDBUFFER g_longSoundBuffer;

DATA(0x0008425c)
LPDIRECT3D2 g_d3d;

DATA(0x00084260)
LPDIRECTDRAW2 g_ddraw2;

DATA(0x00084264)
static DWORD s_shortBufferSize;

DATA(0x00084268)
static DWORD s_longBufferSize;

DATA(0x0008426c)
static u8 s_leftButton;

DATA(0x00084270)
b32 g_soundEnabled;

// Initialized for black colour fills by InitDirectX.
DATA(0x00084278)
DDBLTFX g_clearBltFx;

DATA(0x000842dc)
IDirect3DViewport2* g_screenViewport;

DATA(0x000842e0)
IDirect3DViewport2* g_viewport;

DATA(0x000842e4)
LPDIRECTDRAWSURFACE g_primarySurface;

DATA(0x000842e8)
static u8 s_rightButton;

DATA(0x000842ec)
LPDIRECTDRAWSURFACE g_renderTarget;

DATA(0x000842f0)
D3DDeviceKind g_deviceType;

DATA(0x000842f4)
IDirect3DDevice2* g_d3dDevice;

DATA(0x000842f8)
LPDIRECTINPUT g_directInput;

DATA(0x000842fc)
LPDIRECTINPUTDEVICE g_mouseDevice;

DATA(0x00084300)
LPDIRECTINPUTDEVICE g_keyboardDevice;

DATA(0x00084304)
DWORD g_redMask;

DATA(0x00084308)
DWORD g_greenMask;

DATA(0x0008430c)
DWORD g_blueMask;

DATA(0x00084310)
u8 g_redShift;

DATA(0x00084314)
u8 g_greenShift;

DATA(0x00084318)
u8 g_blueShift;

DATA(0x0008431c)
u8 g_redLoss;

DATA(0x00084320)
u8 g_greenLoss;

DATA(0x00084324)
u8 g_blueLoss;

DATA(0x00084328)
BOOL g_bilinearFiltering;

// The WAVE resource of each sound effect (0 = none), sound 1 first.
DATA(0x0006a6c8)
static u16 s_soundResources[SOUND_COUNT - 1] = {
    IDR_SOUND_1,
    IDR_SOUND_2,
    IDR_SOUND_3,
    IDR_SOUND_4,
    IDR_SOUND_5,
    IDR_SOUND_6,
    IDR_SOUND_7,
    IDR_SOUND_8,
    IDR_SOUND_9,
    IDR_SOUND_10,
    IDR_SOUND_11,
    0,
    IDR_SOUND_13,
    IDR_SOUND_14,
    IDR_SOUND_15,
    IDR_SOUND_16,
    IDR_SOUND_17,
    IDR_SOUND_18,
    IDR_SOUND_19,
    IDR_SOUND_20,
    IDR_SOUND_21,
    IDR_SOUND_22,
    IDR_SOUND_23,
    IDR_SOUND_24,
    IDR_SOUND_25,
    IDR_SOUND_26,
    0,
    IDR_SOUND_28,
    IDR_SOUND_29,
    IDR_SOUND_30,
    IDR_SOUND_31,
    IDR_SOUND_32,
    IDR_SOUND_33,
    IDR_SOUND_34,
    IDR_SOUND_35,
    IDR_SOUND_36,
    IDR_SOUND_37,
    IDR_SOUND_38,
    IDR_SOUND_39,
    IDR_SOUND_40,
    IDR_SOUND_41,
    IDR_SOUND_42,
    IDR_SOUND_43,
    IDR_SOUND_44,
    IDR_SOUND_45,
    IDR_SOUND_46,
    IDR_SOUND_47,
    IDR_SOUND_48,
    IDR_SOUND_49,
    IDR_SOUND_50,
    IDR_SOUND_51,
    IDR_SOUND_52,
    IDR_SOUND_53,
    IDR_SOUND_54,
    IDR_SOUND_55,
    0,
    IDR_SOUND_57,
    IDR_SOUND_58,
    IDR_SOUND_59,
    IDR_SOUND_60,
    IDR_SOUND_61,
    IDR_SOUND_62,
    IDR_SOUND_63,
    IDR_SOUND_64,
    IDR_SOUND_65,
    IDR_SOUND_66,
    IDR_SOUND_67,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    IDR_SOUND_77,
    IDR_SOUND_78,
    IDR_SOUND_79,
    IDR_SOUND_80,
    IDR_SOUND_81,
    IDR_SOUND_82,
    IDR_SOUND_83,
    0,
    IDR_SOUND_85,
    IDR_SOUND_86,
    IDR_SOUND_87,
    IDR_SOUND_88,
    IDR_SOUND_89,
    IDR_SOUND_90,
    0,
    IDR_SOUND_92,
    IDR_SOUND_93,
    IDR_SOUND_94,
    IDR_SOUND_95,
    IDR_SOUND_96,
    IDR_SOUND_97,
    IDR_SOUND_98,
    IDR_SOUND_99,
    IDR_SOUND_100,
    IDR_SOUND_101,
    IDR_SOUND_102,
    IDR_SOUND_103,
    IDR_SOUND_104,
    0,
    IDR_SOUND_106,
    IDR_SOUND_107,
    IDR_SOUND_108,
    0,
    0,
    IDR_SOUND_111,
};

// The identity matrix the Set*Matrix helpers start from.
RVA_DYNINIT(0x00045d90, 0x5, g_identityMatrix)
RVA_DYNINIT(0x00045da0, 0x37, g_identityMatrix)
DATA(0x00084210)
D3DMATRIX g_identityMatrix(
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f
);

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x00045de0, 0x2a)
void SetZeroMatrix(D3DMATRIX& m) {
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            m(i, j) = 0.0f;
        }
    }
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x00045e10, 0x4c)
void SetTranslateMatrix(D3DMATRIX& m, D3DVALUE x, D3DVALUE y, D3DVALUE z) {
    m = g_identityMatrix;
    m(3, 0) = x;
    m(3, 1) = y;
    m(3, 2) = z;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x00045e60, 0x4c)
void SetScaleMatrix(D3DMATRIX& m, D3DVALUE x, D3DVALUE y, D3DVALUE z) {
    m = g_identityMatrix;
    m(0, 0) = x;
    m(1, 1) = y;
    m(2, 2) = z;
}

// Degrees to radians.
#define DEGREES_TO_RADIANS 0.017453292519944444

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x00045eb0, 0x89)
void SetRotateXMatrix(D3DMATRIX& m, D3DVALUE degrees) {
    D3DVALUE c = D3DVAL(cos(degrees * DEGREES_TO_RADIANS));
    D3DVALUE s = D3DVAL(sin(degrees * DEGREES_TO_RADIANS));

    m = g_identityMatrix;
    m(1, 1) = c;
    m(1, 2) = s;
    s = -s;
    m(2, 1) = s;
    m(2, 2) = c;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x00045f40, 0x8b)
void SetRotateYMatrix(D3DMATRIX& m, D3DVALUE degrees) {
    D3DVALUE c = D3DVAL(cos(degrees * DEGREES_TO_RADIANS));
    D3DVALUE s = D3DVAL(sin(degrees * DEGREES_TO_RADIANS));
    D3DVALUE ns = -s;

    m = g_identityMatrix;
    m(0, 0) = c;
    m(0, 2) = ns;
    m(2, 0) = s;
    m(2, 2) = c;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x00045fd0, 0x89)
void SetRotateZMatrix(D3DMATRIX& m, D3DVALUE degrees) {
    D3DVALUE c = D3DVAL(cos(degrees * DEGREES_TO_RADIANS));
    D3DVALUE s = D3DVAL(sin(degrees * DEGREES_TO_RADIANS));

    m = g_identityMatrix;
    m(0, 0) = c;
    m(0, 1) = s;
    s = -s;
    m(1, 0) = s;
    m(1, 1) = c;
}

// @early-stop operand order: retail evaluates each row's y and z products
// before the x product. Every grouping and operand order of the three terms
// emits the same canonical x, z, y order here, and unused-declaration probes
// move only the w-row schedule, so the order is translation-unit state.
RVA(0x00046060, 0x1cb)
void ProjectVector(D3DMATRIX* matrix, D3DVECTOR* in, D3DVECTOR* out) {
    D3DVALUE w;

    out->x = in->x * (*matrix)(0, 0) + (in->y * (*matrix)(1, 0) + in->z * (*matrix)(2, 0))
             + (*matrix)(3, 0);
    out->y = in->x * (*matrix)(0, 1) + (in->y * (*matrix)(1, 1) + in->z * (*matrix)(2, 1))
             + (*matrix)(3, 1);
    out->z = in->x * (*matrix)(0, 2) + (in->y * (*matrix)(1, 2) + in->z * (*matrix)(2, 2))
             + (*matrix)(3, 2);
    w = in->x * (*matrix)(0, 3) + (in->y * (*matrix)(1, 3) + in->z * (*matrix)(2, 3))
        + (*matrix)(3, 3);
    out->x /= w;
    out->y /= w;
    out->z /= w;
    out->x = out->x * 160.0f / out->z * DATA_COMPGEN(0x000649ec, 2.0f) - -320.0f;
    out->y = 164.0f - out->y * 160.0f / out->z * 2.0f;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x00046230, 0xdc)
void MultiplyMatrix(D3DMATRIX& a, D3DMATRIX& b, D3DMATRIX& product) {
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            product(i, j) =
                a(i, 0) * b(0, j) + a(i, 1) * b(1, j) + a(i, 2) * b(2, j) + a(i, 3) * b(3, j);
        }
    }
}

RVA(0x00046310, 0x22f)
void SetViewMatrix(D3DMATRIX& m, D3DVECTOR& from, D3DVECTOR& at) {
    D3DVECTOR up;
    D3DVECTOR right;
    D3DVECTOR view;
    D3DVECTOR worldUp(0.0f, 1.0f, 0.0f);

    m = g_identityMatrix;
    view = Normalize(at - from);
    right = CrossProduct(worldUp, view);
    up = CrossProduct(view, right);
    right = Normalize(right);
    up = Normalize(up);
    m(0, 0) = right.x;
    m(1, 0) = right.y;
    m(2, 0) = right.z;
    m(0, 1) = up.x;
    m(1, 1) = up.y;
    m(2, 1) = up.z;
    m(0, 2) = view.x;
    m(1, 2) = view.y;
    m(2, 2) = view.z;
    m(3, 0) = -DotProduct(right, from);
    m(3, 1) = -DotProduct(up, from);
    m(3, 2) = -DotProduct(view, from);
}

RVA(0x00046540, 0x15b)
void SetProjectionMatrix(D3DMATRIX& m, D3DVALUE nearPlane, D3DVALUE scale, D3DVALUE farPlane) {
    m(0, 0) = 1.0f;
    m(0, 1) = 0.0f;
    m(0, 2) = 0.0f;
    m(0, 3) = 0.0f;
    m(1, 0) = 0.0f;
    m(1, 1) = 1.0f;
    m(1, 2) = 0.0f;
    m(1, 3) = 0.0f;
    m(2, 0) = 0.0f;
    m(2, 1) = 0.0f;
    m(2, 2) = farPlane * scale / (nearPlane * (farPlane - nearPlane));
    m(2, 3) = scale / nearPlane;
    m(3, 0) = 0.0f;
    m(3, 1) = 0.0f;
    m(3, 2) = -(farPlane * scale) / (farPlane - nearPlane);
    m(3, 3) = 0.0f;
}

// Keeps the lowest-depth 8-bit-or-deeper colour texture format, and stops at
// a 16-bit format matching the display's masks.
RVA(0x000466a0, 0xa5)
static HRESULT CALLBACK ChooseTextureFormat(LPDDSURFACEDESC desc, LPVOID context) {
    DDPIXELFORMAT format = desc->ddpfPixelFormat;

    if (format.dwFlags & (DDPF_ALPHAPIXELS | DDPF_ALPHA)) {
        return DDENUMRET_OK;
    }
    if (format.dwRGBBitCount <= 8
        && !(format.dwFlags & (DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8))) {
        return DDENUMRET_OK;
    }
    if (format.dwRGBBitCount > 8 && !(format.dwFlags & DDPF_RGB)) {
        return DDENUMRET_OK;
    }
    if (format.dwRGBBitCount < 8) {
        return DDENUMRET_OK;
    }
    if (g_textureFormat.dwRGBBitCount != 0
        && format.dwRGBBitCount - 8 > g_textureFormat.dwRGBBitCount - 8) {
        return DDENUMRET_OK;
    }
    g_textureFormat = desc->ddpfPixelFormat;
    if (format.dwRGBBitCount == PIXEL_BITS_16 && format.dwRBitMask == g_redMask
        && format.dwGBitMask == g_greenMask && format.dwBBitMask == g_blueMask) {
        return DDENUMRET_CANCEL;
    }
    return DDENUMRET_OK;
}

// Writes a DirectDraw result's DDERR_* name to the debugger.
RVA(0x00046750, 0xca0)
void TraceDDrawError(HRESULT result) {
    switch (result) {
        case DDERR_UNSUPPORTED:
            OutputDebugString("DDERR_UNSUPPORTED");
            break;
        case DDERR_GENERIC:
            OutputDebugString("DDERR_GENERIC");
            break;
        case DDERR_NOTINITIALIZED:
            OutputDebugString("DDERR_NOTINITIALIZED");
            break;
        case DDERR_OUTOFMEMORY:
            OutputDebugString("DDERR_OUTOFMEMORY");
            break;
        case DDERR_INVALIDPARAMS:
            OutputDebugString("DDERR_INVALIDPARAMS");
            break;
        case DDERR_ALREADYINITIALIZED:
            OutputDebugString("DDERR_ALREADYINITIALIZED");
            break;
        case DDERR_CANNOTATTACHSURFACE:
            OutputDebugString("DDERR_CANNOTATTACHSURFACE");
            break;
        case DDERR_CANNOTDETACHSURFACE:
            OutputDebugString("DDERR_CANNOTDETACHSURFACE");
            break;
        case DDERR_CURRENTLYNOTAVAIL:
            OutputDebugString("DDERR_CURRENTLYNOTAVAIL");
            break;
        case DDERR_EXCEPTION:
            OutputDebugString("DDERR_EXCEPTION");
            break;
        case DDERR_HEIGHTALIGN:
            OutputDebugString("DDERR_HEIGHTALIGN");
            break;
        case DDERR_INCOMPATIBLEPRIMARY:
            OutputDebugString("DDERR_INCOMPATIBLEPRIMARY");
            break;
        case DDERR_INVALIDCAPS:
            OutputDebugString("DDERR_INVALIDCAPS");
            break;
        case DDERR_INVALIDCLIPLIST:
            OutputDebugString("DDERR_INVALIDCLIPLIST");
            break;
        case DDERR_INVALIDMODE:
            OutputDebugString("DDERR_INVALIDMODE");
            break;
        case DDERR_INVALIDOBJECT:
            OutputDebugString("DDERR_INVALIDOBJECT");
            break;
        case DDERR_INVALIDPIXELFORMAT:
            OutputDebugString("DDERR_INVALIDPIXELFORMAT");
            break;
        case DDERR_INVALIDRECT:
            OutputDebugString("DDERR_INVALIDRECT");
            break;
        case DDERR_LOCKEDSURFACES:
            OutputDebugString("DDERR_LOCKEDSURFACES");
            break;
        case DDERR_NO3D:
            OutputDebugString("DDERR_NO3D");
            break;
        case DDERR_NOALPHAHW:
            OutputDebugString("DDERR_NOALPHAHW");
            break;
        case DDERR_NOCLIPLIST:
            OutputDebugString("DDERR_NOCLIPLIST");
            break;
        case DDERR_NOCOLORKEY:
            OutputDebugString("DDERR_NOCOLORKEY");
            break;
        case DDERR_NOCOOPERATIVELEVELSET:
            OutputDebugString("DDERR_NOCOOPERATIVELEVELSET");
            break;
        case DDERR_NOCOLORCONVHW:
            OutputDebugString("DDERR_NOCOLORCONVHW");
            break;
        case DDERR_NOCOLORKEYHW:
            OutputDebugString("DDERR_NOCOLORKEYHW");
            break;
        case DDERR_NODIRECTDRAWSUPPORT:
            OutputDebugString("DDERR_NODIRECTDRAWSUPPORT");
            break;
        case DDERR_NOEXCLUSIVEMODE:
            OutputDebugString("DDERR_NOEXCLUSIVEMODE");
            break;
        case DDERR_NOFLIPHW:
            OutputDebugString("DDERR_NOFLIPHW");
            break;
        case DDERR_NOGDI:
            OutputDebugString("DDERR_NOGDI");
            break;
        case DDERR_NOMIRRORHW:
            OutputDebugString("DDERR_NOMIRRORHW");
            break;
        case DDERR_NOTFOUND:
            OutputDebugString("DDERR_NOTFOUND");
            break;
        case DDERR_NOOVERLAYHW:
            OutputDebugString("DDERR_NOOVERLAYHW");
            break;
        case DDERR_NORASTEROPHW:
            OutputDebugString("DDERR_NORASTEROPHW");
            break;
        case DDERR_NOROTATIONHW:
            OutputDebugString("DDERR_NOROTATIONHW");
            break;
        case DDERR_NOSTRETCHHW:
            OutputDebugString("DDERR_NOSTRETCHHW");
            break;
        case DDERR_NOT8BITCOLOR:
            OutputDebugString("DDERR_NOT8BITCOLOR");
            break;
        case DDERR_NOT4BITCOLORINDEX:
            OutputDebugString("DDERR_NOT4BITCOLORINDEX");
            break;
        case DDERR_NOT4BITCOLOR:
            OutputDebugString("DDERR_NOT4BITCOLOR");
            break;
        case DDERR_NOTEXTUREHW:
            OutputDebugString("DDERR_NOTEXTUREHW");
            break;
        case DDERR_NOVSYNCHW:
            OutputDebugString("DDERR_NOVSYNCHW");
            break;
        case DDERR_NOZBUFFERHW:
            OutputDebugString("DDERR_NOZBUFFERHW");
            break;
        case DDERR_NOZOVERLAYHW:
            OutputDebugString("DDERR_NOZOVERLAYHW");
            break;
        case DDERR_OUTOFCAPS:
            OutputDebugString("DDERR_OUTOFCAPS");
            break;
        case DDERR_OUTOFVIDEOMEMORY:
            OutputDebugString("DDERR_OUTOFVIDEOMEMORY");
            break;
        case DDERR_OVERLAYCANTCLIP:
            OutputDebugString("DDERR_OVERLAYCANTCLIP");
            break;
        case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
            OutputDebugString("DDERR_OVERLAYCOLORKEYONLYONEACTIVE");
            break;
        case DDERR_PALETTEBUSY:
            OutputDebugString("DDERR_PALETTEBUSY");
            break;
        case DDERR_COLORKEYNOTSET:
            OutputDebugString("DDERR_COLORKEYNOTSET");
            break;
        case DDERR_SURFACEALREADYATTACHED:
            OutputDebugString("DDERR_SURFACEALREADYATTACHED");
            break;
        case DDERR_SURFACEALREADYDEPENDENT:
            OutputDebugString("DDERR_SURFACEALREADYDEPENDENT");
            break;
        case DDERR_SURFACEBUSY:
            OutputDebugString("DDERR_SURFACEBUSY");
            break;
        case DDERR_CANTLOCKSURFACE:
            OutputDebugString("DDERR_CANTLOCKSURFACE");
            break;
        case DDERR_SURFACEISOBSCURED:
            OutputDebugString("DDERR_SURFACEISOBSCURED");
            break;
        case DDERR_SURFACELOST:
            OutputDebugString("DDERR_SURFACELOST");
            break;
        case DDERR_SURFACENOTATTACHED:
            OutputDebugString("DDERR_SURFACENOTATTACHED");
            break;
        case DDERR_TOOBIGHEIGHT:
            OutputDebugString("DDERR_TOOBIGHEIGHT");
            break;
        case DDERR_TOOBIGSIZE:
            OutputDebugString("DDERR_TOOBIGSIZE");
            break;
        case DDERR_TOOBIGWIDTH:
            OutputDebugString("DDERR_TOOBIGWIDTH");
            break;
        case DDERR_UNSUPPORTEDFORMAT:
            OutputDebugString("DDERR_UNSUPPORTEDFORMAT");
            break;
        case DDERR_UNSUPPORTEDMASK:
            OutputDebugString("DDERR_UNSUPPORTEDMASK");
            break;
        case DDERR_VERTICALBLANKINPROGRESS:
            OutputDebugString("DDERR_VERTICALBLANKINPROGRESS");
            break;
        case DDERR_WASSTILLDRAWING:
            OutputDebugString("DDERR_WASSTILLDRAWING");
            break;
        case DDERR_XALIGN:
            OutputDebugString("DDERR_XALIGN");
            break;
        case DDERR_INVALIDDIRECTDRAWGUID:
            OutputDebugString("DDERR_INVALIDDIRECTDRAWGUID");
            break;
        case DDERR_DIRECTDRAWALREADYCREATED:
            OutputDebugString("DDERR_DIRECTDRAWALREADYCREATED");
            break;
        case DDERR_NODIRECTDRAWHW:
            OutputDebugString("DDERR_NODIRECTDRAWHW");
            break;
        case DDERR_PRIMARYSURFACEALREADYEXISTS:
            OutputDebugString("DDERR_PRIMARYSURFACEALREADYEXISTS");
            break;
        case DDERR_NOEMULATION:
            OutputDebugString("DDERR_NOEMULATION");
            break;
        case DDERR_REGIONTOOSMALL:
            OutputDebugString("DDERR_REGIONTOOSMALL");
            break;
        case DDERR_CLIPPERISUSINGHWND:
            OutputDebugString("DDERR_CLIPPERISUSINGHWND");
            break;
        case DDERR_NOCLIPPERATTACHED:
            OutputDebugString("DDERR_NOCLIPPERATTACHED");
            break;
        case DDERR_NOHWND:
            OutputDebugString("DDERR_NOHWND");
            break;
        case DDERR_HWNDSUBCLASSED:
            OutputDebugString("DDERR_HWNDSUBCLASSED");
            break;
        case DDERR_HWNDALREADYSET:
            OutputDebugString("DDERR_HWNDALREADYSET");
            break;
        case DDERR_NOPALETTEATTACHED:
            OutputDebugString("DDERR_NOPALETTEATTACHED");
            break;
        case DDERR_NOPALETTEHW:
            OutputDebugString("DDERR_NOPALETTEHW");
            break;
        case DDERR_BLTFASTCANTCLIP:
            OutputDebugString("DDERR_BLTFASTCANTCLIP");
            break;
        case DDERR_NOBLTHW:
            OutputDebugString("DDERR_NOBLTHW");
            break;
        case DDERR_NODDROPSHW:
            OutputDebugString("DDERR_NODDROPSHW");
            break;
        case DDERR_OVERLAYNOTVISIBLE:
            OutputDebugString("DDERR_OVERLAYNOTVISIBLE");
            break;
        case DDERR_NOOVERLAYDEST:
            OutputDebugString("DDERR_NOOVERLAYDEST");
            break;
        case DDERR_INVALIDPOSITION:
            OutputDebugString("DDERR_INVALIDPOSITION");
            break;
        case DDERR_NOTAOVERLAYSURFACE:
            OutputDebugString("DDERR_NOTAOVERLAYSURFACE");
            break;
        case DDERR_EXCLUSIVEMODEALREADYSET:
            OutputDebugString("DDERR_EXCLUSIVEMODEALREADYSET");
            break;
        case DDERR_NOTFLIPPABLE:
            OutputDebugString("DDERR_NOTFLIPPABLE");
            break;
        case DDERR_CANTDUPLICATE:
            OutputDebugString("DDERR_CANTDUPLICATE");
            break;
        case DDERR_NOTLOCKED:
            OutputDebugString("DDERR_NOTLOCKED");
            break;
        case DDERR_CANTCREATEDC:
            OutputDebugString("DDERR_CANTCREATEDC");
            break;
        case DDERR_NODC:
            OutputDebugString("DDERR_NODC");
            break;
        case DDERR_WRONGMODE:
            OutputDebugString("DDERR_WRONGMODE");
            break;
        case DDERR_IMPLICITLYCREATED:
            OutputDebugString("DDERR_IMPLICITLYCREATED");
            break;
        case DDERR_NOTPALETTIZED:
            OutputDebugString("DDERR_NOTPALETTIZED");
            break;
        case DDERR_UNSUPPORTEDMODE:
            OutputDebugString("DDERR_UNSUPPORTEDMODE");
            break;
        case DDERR_NOMIPMAPHW:
            OutputDebugString("DDERR_NOMIPMAPHW");
            break;
        case DDERR_INVALIDSURFACETYPE:
            OutputDebugString("DDERR_INVALIDSURFACETYPE");
            break;
        case DDERR_DCALREADYCREATED:
            OutputDebugString("DDERR_DCALREADYCREATED");
            break;
        case DDERR_CANTPAGELOCK:
            OutputDebugString("DDERR_CANTPAGELOCK");
            break;
        case DDERR_CANTPAGEUNLOCK:
            OutputDebugString("DDERR_CANTPAGEUNLOCK");
            break;
        case DDERR_NOTPAGELOCKED:
            OutputDebugString("DDERR_NOTPAGELOCKED");
            break;
        default:
            OutputDebugString("Unknown Error");
            break;
    }
    OutputDebugString("\n");
}

// Writes a Direct3D result's D3DERR_* name to the debugger.
RVA(0x000473f0, 0x3f0)
void TraceD3DError(HRESULT result) {
    switch (result) {
        case D3DERR_BADMAJORVERSION:
            OutputDebugString("D3DERR_BADMAJORVERSION");
            break;
        case D3DERR_BADMINORVERSION:
            OutputDebugString("D3DERR_BADMINORVERSION");
            break;
        case D3DERR_EXECUTE_CREATE_FAILED:
            OutputDebugString("D3DERR_EXECUTE_CREATE_FAILED");
            break;
        case D3DERR_EXECUTE_DESTROY_FAILED:
            OutputDebugString("D3DERR_EXECUTE_DESTROY_FAILED");
            break;
        case D3DERR_EXECUTE_LOCK_FAILED:
            OutputDebugString("D3DERR_EXECUTE_LOCK_FAILED");
            break;
        case D3DERR_EXECUTE_UNLOCK_FAILED:
            OutputDebugString("D3DERR_EXECUTE_UNLOCK_FAILED");
            break;
        case D3DERR_EXECUTE_LOCKED:
            OutputDebugString("D3DERR_EXECUTE_LOCKED");
            break;
        case D3DERR_EXECUTE_NOT_LOCKED:
            OutputDebugString("D3DERR_EXECUTE_NOT_LOCKED");
            break;
        case D3DERR_EXECUTE_FAILED:
            OutputDebugString("D3DERR_EXECUTE_FAILED");
            break;
        case D3DERR_EXECUTE_CLIPPED_FAILED:
            OutputDebugString("D3DERR_EXECUTE_CLIPPED_FAILED");
            break;
        case D3DERR_TEXTURE_NO_SUPPORT:
            OutputDebugString("D3DERR_TEXTURE_NO_SUPPORT");
            break;
        case D3DERR_TEXTURE_CREATE_FAILED:
            OutputDebugString("D3DERR_TEXTURE_CREATE_FAILED");
            break;
        case D3DERR_TEXTURE_DESTROY_FAILED:
            OutputDebugString("D3DERR_TEXTURE_DESTROY_FAILED");
            break;
        case D3DERR_TEXTURE_LOCK_FAILED:
            OutputDebugString("D3DERR_TEXTURE_LOCK_FAILED");
            break;
        case D3DERR_TEXTURE_UNLOCK_FAILED:
            OutputDebugString("D3DERR_TEXTURE_UNLOCK_FAILED");
            break;
        case D3DERR_TEXTURE_LOAD_FAILED:
            OutputDebugString("D3DERR_TEXTURE_LOAD_FAILED");
            break;
        case D3DERR_TEXTURE_SWAP_FAILED:
            OutputDebugString("D3DERR_TEXTURE_SWAP_FAILED");
            break;
        case D3DERR_TEXTURE_LOCKED:
            OutputDebugString("D3DERR_TEXTURE_LOCKED");
            break;
        case D3DERR_TEXTURE_NOT_LOCKED:
            OutputDebugString("D3DERR_TEXTURE_NOT_LOCKED");
            break;
        case D3DERR_TEXTURE_GETSURF_FAILED:
            OutputDebugString("D3DERR_TEXTURE_GETSURF_FAILED");
            break;
        case D3DERR_MATRIX_CREATE_FAILED:
            OutputDebugString("D3DERR_MATRIX_CREATE_FAILED");
            break;
        case D3DERR_MATRIX_DESTROY_FAILED:
            OutputDebugString("D3DERR_MATRIX_DESTROY_FAILED");
            break;
        case D3DERR_MATRIX_SETDATA_FAILED:
            OutputDebugString("D3DERR_MATRIX_SETDATA_FAILED");
            break;
        case D3DERR_MATRIX_GETDATA_FAILED:
            OutputDebugString("D3DERR_MATRIX_GETDATA_FAILED");
            break;
        case D3DERR_SETVIEWPORTDATA_FAILED:
            OutputDebugString("D3DERR_SETVIEWPORTDATA_FAILED");
            break;
        case D3DERR_MATERIAL_CREATE_FAILED:
            OutputDebugString("D3DERR_MATERIAL_CREATE_FAILED");
            break;
        case D3DERR_MATERIAL_DESTROY_FAILED:
            OutputDebugString("D3DERR_MATERIAL_DESTROY_FAILED");
            break;
        case D3DERR_MATERIAL_SETDATA_FAILED:
            OutputDebugString("D3DERR_MATERIAL_SETDATA_FAILED");
            break;
        case D3DERR_MATERIAL_GETDATA_FAILED:
            OutputDebugString("D3DERR_MATERIAL_GETDATA_FAILED");
            break;
        case D3DERR_LIGHT_SET_FAILED:
            OutputDebugString("D3DERR_LIGHT_SET_FAILED");
            break;
        case D3DERR_SCENE_IN_SCENE:
            OutputDebugString("D3DERR_SCENE_IN_SCENE");
            break;
        case D3DERR_SCENE_NOT_IN_SCENE:
            OutputDebugString("D3DERR_SCENE_NOT_IN_SCENE");
            break;
        case D3DERR_SCENE_BEGIN_FAILED:
            OutputDebugString("D3DERR_SCENE_BEGIN_FAILED");
            break;
        case D3DERR_SCENE_END_FAILED:
            OutputDebugString("D3DERR_SCENE_END_FAILED");
            break;
        default:
            OutputDebugString("Unknown Error");
            break;
    }
    OutputDebugString("\n");
}

// Keeps the descriptions of the HAL, MMX and ramp devices.
RVA(0x000477e0, 0x86)
static HRESULT CALLBACK KeepDeviceDesc(
    LPGUID guid,
    LPSTR description,
    LPSTR name,
    LPD3DDEVICEDESC hal,
    LPD3DDEVICEDESC hel,
    LPVOID context
) {
    D3DDeviceInfo* info = static_cast<D3DDeviceInfo*>(context);

    if (strcmp(name, "Direct3D HAL") == 0) {
        info->halDesc = *hal;
    } else if (strcmp(name, "MMX Emulation") == 0) {
        info->mmxDesc = *hel;
    } else if (strcmp(name, "Ramp Emulation") == 0) {
        info->rampDesc = *hel;
    }
    return D3DENUMRET_OK;
}

// Releases every DirectX object, the input devices first.
// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x00047870, 0x130)
void ReleaseDirectX(void) {
    ReleaseDirectInput();
    ReleaseComObject(g_shortSoundBuffer);
    ReleaseComObject(g_longSoundBuffer);
    ReleaseComObject(g_directSound);
    ReleaseComObject(g_screenViewport);
    ReleaseComObject(g_screenDevice);
    ReleaseComObject(g_viewport);
    ReleaseComObject(g_d3dDevice);
    ReleaseComObject(g_d3d);
    ReleaseComObject(g_zBuffer);
    ReleaseComObject(g_renderTarget);
    ReleaseComObject(g_roomObject);
    ReleaseComObject(g_primarySurface);
    ReleaseComObject(g_ddraw2);
    ReleaseComObject(g_ddraw);
}

// Creates DirectDraw on the configured driver, picks the device kind, sets
// 640x480x16 full-screen exclusive and creates the flipping primary surface
// (with its back buffer as the render target on the HAL device); derives the
// display's channel masks, shifts and losses.
RVA(0x000479a0, 0x252)
b32 InitDirectDraw(void) {
    DDSCAPS caps;
    DDSURFACEDESC desc;
    DWORD red;
    DWORD green;
    DWORD blue;
    u8 redShift;
    u8 greenShift;
    u8 blueShift;

    if (DirectDrawCreate(&g_displayConfig.driver, &g_ddraw, NULL) != DD_OK) {
        return false;
    }
    // API-forced: QueryInterface hands the interface back through a void**.
    if (g_ddraw->QueryInterface(IID_IDirectDraw2, reinterpret_cast<void**>(&g_ddraw2)) != DD_OK) {
        return false;
    }
    if (g_deviceSettings.caps.hardwareOnly == 1) {
        g_deviceType = D3D_DEVICE_HAL;
    } else {
        QueryD3DDevices();
        g_deviceType = D3D_DEVICE_RAMP;
    }
    if (g_ddraw->SetCooperativeLevel(g_mainWindow, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) != DD_OK) {
        return false;
    }
    if (g_ddraw->SetDisplayMode(g_windowRect.right, g_windowRect.bottom, PIXEL_BITS_16) != DD_OK) {
        return false;
    }
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    desc.ddsCaps.dwCaps =
        DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
    desc.dwBackBufferCount = 1;
    if (g_ddraw->CreateSurface(&desc, &g_primarySurface, NULL) != DD_OK) {
        return false;
    }
    if (g_deviceType == D3D_DEVICE_HAL) {
        caps.dwCaps = DDSCAPS_BACKBUFFER;
        if (g_primarySurface->GetAttachedSurface(&caps, &g_renderTarget) != DD_OK) {
            return false;
        }
    }
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_PIXELFORMAT;
    g_ddraw->GetDisplayMode(&desc);
    g_redMask = desc.ddpfPixelFormat.dwRBitMask;
    g_greenMask = desc.ddpfPixelFormat.dwGBitMask;
    g_blueMask = desc.ddpfPixelFormat.dwBBitMask;
    if (g_redMask != 0 && g_greenMask != 0 && g_blueMask != 0) {
        red = g_redMask;
        green = g_greenMask;
        blue = g_blueMask;
        redShift = 0;
        greenShift = 0;
        blueShift = 0;
        while (!(red & 1)) {
            red >>= 1;
            redShift++;
        }
        while (!(green & 1)) {
            green >>= 1;
            greenShift++;
        }
        while (!(blue & 1)) {
            blue >>= 1;
            blueShift++;
        }
        g_redShift = redShift;
        g_greenShift = greenShift;
        g_blueShift = blueShift;
        if (desc.ddpfPixelFormat.dwRGBBitCount == PIXEL_BITS_16) {
            g_redLoss = red == CHANNEL_MASK_5BIT ? 3 : 2;
            g_greenLoss = green == CHANNEL_MASK_5BIT ? 3 : 2;
            g_blueLoss = blue == CHANNEL_MASK_5BIT ? 3 : 2;
        }
    }
    return true;
}

// Asks DirectDraw and Direct3D what the HAL, MMX and ramp devices support and
// keeps the summary in g_deviceSettings.caps.
RVA(0x00047c00, 0x1a7)
void QueryD3DDevices(void) {
    D3DDeviceInfo info;
    HRESULT result;

    memset(&info, 0, sizeof(info));
    info.helCaps.dwSize = info.driverCaps.dwSize = sizeof(DDCAPS);
    result = g_ddraw2->GetCaps(&info.driverCaps, &info.helCaps);
    if (result == DD_OK) {
        // API-forced: QueryInterface hands the interface back through a void**.
        if (g_ddraw2->QueryInterface(IID_IDirect3D2, reinterpret_cast<void**>(&g_d3d)) == DD_OK) {
            result = g_d3d->EnumDevices(KeepDeviceDesc, &info);
            if (result == DD_OK) {
                ReleaseComObject(g_d3d);
                g_deviceSettings.caps.driverCaps = info.driverCaps.dwCaps;
                g_deviceSettings.caps.surfaceCaps = info.driverCaps.ddsCaps.dwCaps;
                if (info.halDesc.dwFlags & D3DDD_TRICAPS) {
                    g_deviceSettings.caps.primCaps[D3D_DEVICE_HAL] = info.halDesc.dpcTriCaps;
                } else if (info.halDesc.dwFlags & D3DDD_LINECAPS) {
                    g_deviceSettings.caps.primCaps[D3D_DEVICE_HAL] = info.halDesc.dpcLineCaps;
                }
                // @identity-TODO: the line-caps fallback tests the HAL description's
                // flags, as retail does.
                if (info.mmxDesc.dwFlags & D3DDD_TRICAPS) {
                    g_deviceSettings.caps.primCaps[D3D_DEVICE_MMX] = info.mmxDesc.dpcTriCaps;
                } else if (info.halDesc.dwFlags & D3DDD_LINECAPS) {
                    g_deviceSettings.caps.primCaps[D3D_DEVICE_MMX] = info.mmxDesc.dpcLineCaps;
                }
                if (info.rampDesc.dwFlags & D3DDD_TRICAPS) {
                    g_deviceSettings.caps.primCaps[D3D_DEVICE_RAMP] = info.rampDesc.dpcTriCaps;
                } else if (info.rampDesc.dwFlags & D3DDD_LINECAPS) {
                    g_deviceSettings.caps.primCaps[D3D_DEVICE_RAMP] = info.rampDesc.dpcLineCaps;
                }
                g_deviceSettings.caps.zBufferDepth[D3D_DEVICE_HAL] = ZBufferDepth(&info.halDesc);
                g_deviceSettings.caps.zBufferDepth[D3D_DEVICE_MMX] = ZBufferDepth(&info.mmxDesc);
                g_deviceSettings.caps.zBufferDepth[D3D_DEVICE_RAMP] = ZBufferDepth(&info.rampDesc);
            } else {
                TraceD3DCallError("lpD3D->EnumDevices() returns ", result);
            }
        }
    } else {
        TraceDDrawError(result);
    }
}

// The z-buffer depth a device supports (16, else 24, else 32 or 0).
RVA(0x00047db0, 0x23)
DWORD __fastcall ZBufferDepth(D3DDEVICEDESC* desc) {
    if (desc->dwDeviceZBufferBitDepth & DDBD_16) {
        return 16;
    }
    if (desc->dwDeviceZBufferBitDepth & DDBD_24) {
        return 24;
    }
    return desc->dwDeviceZBufferBitDepth & DDBD_32 ? 32 : 0;
}

// Restores the primary surface, render target and z-buffer if lost; returns
// FALSE when a restore fails.
RVA(0x00047de0, 0x87)
b32 RestoreSurfaces(BOOL restore) {
    b32 ok = true;

    if (restore) {
        if (g_primarySurface != NULL && g_primarySurface->IsLost() == DDERR_SURFACELOST
            && g_primarySurface->Restore() != DD_OK) {
            ok = false;
        }
        if (g_renderTarget != NULL && g_renderTarget->IsLost() == DDERR_SURFACELOST
            && g_renderTarget->Restore() != DD_OK) {
            ok = false;
        }
        if (g_zBuffer != NULL && g_zBuffer->IsLost() == DDERR_SURFACELOST
            && g_zBuffer->Restore() != DD_OK) {
            ok = false;
        }
    }
    return ok;
}

// Reads the 16-bit pixel (x, y) of a surface `pitch` pixels wide.
RVA(0x00047e70, 0x75)
u16 ReadSurfaceWord(LPDIRECTDRAWSURFACE surface, i32 x, i32 pitch, i32 y) {
    DDSURFACEDESC desc;
    u16 value;

    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
    surface->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_READONLY | DDLOCK_NOSYSLOCK, NULL);
    value = static_cast<u16*>(desc.lpSurface)[y * pitch + x];
    surface->Unlock(desc.lpSurface);
    return value;
}

// Creates Direct3D: on the HAL device a z-buffer on the back buffer (falling
// back to software when the HAL device fails), otherwise an offscreen render
// target in system memory with its own z-buffer and the MMX or ramp device;
// then the screen device, both viewports, the background and lighting
// materials, fog, the transforms, the render states and the texture format.
// The surface descriptions are "cleared" with memset's size and fill swapped,
// which clears nothing; the retail code keeps the dead setup of those calls.
RVA(0x00047ef0, 0x9a8)
b32 InitDirect3D(void) {
    LPDIRECT3DMATERIAL2 material;
    LPDIRECT3DMATERIAL2 lighting;
    D3DVIEWPORT2 viewport;
    D3DMATERIALHANDLE lightingHandle;
    D3DMATERIALHANDLE backgroundHandle;
    D3DVALUE fogStart;
    D3DVALUE fogEnd;
    DDSURFACEDESC desc;
    D3DMATERIAL materialDesc;
    D3DMATERIAL lightingDesc;
    HRESULT result;

    // API-forced: QueryInterface hands the interface back through a void**.
    if (g_ddraw->QueryInterface(IID_IDirect3D2, reinterpret_cast<void**>(&g_d3d)) != DD_OK) {
        return false;
    }
    if (g_deviceType == D3D_DEVICE_HAL) {
        memset(&desc, sizeof(desc), 0);
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
        desc.dwWidth = 640;
        desc.dwHeight = 480;
        desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
        desc.dwZBufferBitDepth = 16;
        if (g_ddraw->CreateSurface(&desc, &g_zBuffer, NULL) != DD_OK
            || g_renderTarget->AddAttachedSurface(g_zBuffer) != DD_OK) {
            return false;
        }
    }
    if (g_deviceType == D3D_DEVICE_HAL) {
        result = g_d3d->CreateDevice(IID_IDirect3DHALDevice, g_renderTarget, &g_d3dDevice);
        if (result != D3D_OK) {
            g_renderTarget->DeleteAttachedSurface(0, g_zBuffer);
            ReleaseComObject(g_primarySurface);
            memset(&desc, sizeof(desc), 0);
            desc.dwSize = sizeof(desc);
            desc.dwFlags = DDSD_CAPS;
            desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
            g_ddraw->CreateSurface(&desc, &g_primarySurface, NULL);
            g_deviceType = D3D_DEVICE_MMX;
        }
    } else {
        result = !D3D_OK;
    }
    if (result != D3D_OK) {
        memset(&desc, sizeof(desc), 0);
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        desc.dwWidth = g_viewClearRect.x2;
        desc.dwHeight = g_viewClearRect.y2;
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE;
        if (g_ddraw->CreateSurface(&desc, &g_renderTarget, NULL) != DD_OK) {
            return false;
        }
        memset(&desc, sizeof(desc), 0);
        desc.dwSize = sizeof(desc);
        if (g_renderTarget->GetSurfaceDesc(&desc) != DD_OK) {
            return false;
        }
        desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
        desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY;
        desc.dwZBufferBitDepth = 16;
        if (g_ddraw->CreateSurface(&desc, &g_zBuffer, NULL) != DD_OK) {
            return false;
        }
        if (g_renderTarget->AddAttachedSurface(g_zBuffer) != DD_OK) {
            return false;
        }
        result = !D3D_OK;
        if (g_deviceType == D3D_DEVICE_MMX) {
            result = g_d3d->CreateDevice(IID_IDirect3DMMXDevice, g_renderTarget, &g_d3dDevice);
            if (result != D3D_OK) {
                TraceD3DCallError("lpD3D->CreateDevice()@D3DDT_MMX returns ", result);
                g_deviceType = D3D_DEVICE_RAMP;
            }
        }
        if (result != D3D_OK
            && g_d3d->CreateDevice(IID_IDirect3DRampDevice, g_renderTarget, &g_d3dDevice)
                   != D3D_OK) {
            return false;
        }
    }
    g_bilinearFiltering = g_deviceSettings.caps.bilinearFiltering[static_cast<i32>(g_deviceType)];
    if (g_deviceType == D3D_DEVICE_HAL) {
        if (g_d3d->CreateDevice(IID_IDirect3DHALDevice, g_renderTarget, &g_screenDevice)
            != D3D_OK) {
            return false;
        }
    } else if (g_deviceType == D3D_DEVICE_MMX) {
        if (g_d3d->CreateDevice(IID_IDirect3DMMXDevice, g_renderTarget, &g_screenDevice)
            != D3D_OK) {
            return false;
        }
    } else if (g_d3d->CreateDevice(IID_IDirect3DRGBDevice, g_renderTarget, &g_screenDevice)
               != D3D_OK) {
        return false;
    }
    if (g_d3d->CreateViewport(&g_viewport, NULL) != D3D_OK) {
        return false;
    }
    if (g_d3dDevice->AddViewport(g_viewport) != D3D_OK) {
        return false;
    }
    ZeroMemory(&viewport, sizeof(viewport));
    viewport.dwSize = sizeof(viewport);
    viewport.dwWidth = 640;
    viewport.dwHeight = 328;
    viewport.dvClipX = DATA_COMPGEN(0x000649f8, -1.0f);
    viewport.dvClipWidth = 2.0f;
    viewport.dvClipY = DATA_COMPGEN(0x000649fc, 0.5125f);
    viewport.dvClipHeight = DATA_COMPGEN(0x00064a00, 1.025f);
    viewport.dvMinZ = DATA_COMPGEN(0x00064a04, 0.0f);
    viewport.dvMaxZ = DATA_COMPGEN(0x00064a08, 1.0f);
    if (g_viewport->SetViewport2(&viewport) != D3D_OK) {
        return false;
    }
    if (g_d3dDevice->SetCurrentViewport(g_viewport) != D3D_OK) {
        return false;
    }
    if (g_d3d->CreateViewport(&g_screenViewport, NULL) != D3D_OK) {
        return false;
    }
    if (g_screenDevice->AddViewport(g_screenViewport) != D3D_OK) {
        return false;
    }
    ZeroMemory(&viewport, sizeof(viewport));
    viewport.dwSize = sizeof(viewport);
    viewport.dwWidth = g_viewClearRect.x2;
    viewport.dwHeight = g_viewClearRect.y2;
    viewport.dvClipX = DATA_COMPGEN(0x00064a0c, -1.0f / 3.0f);
    viewport.dvClipWidth = DATA_COMPGEN(0x00064a10, 2.0f / 3.0f);
    viewport.dvMinZ = 0.0f;
    viewport.dvMaxZ = 1.0f;
    viewport.dvClipHeight =
        g_viewClearRect.x2
        * DATA_COMPGEN(0x00064a18, 2.0) / g_viewClearRect.y2 * DATA_COMPGEN(0x00064a20, 1.0 / 3.0);
    viewport.dvClipY = viewport.dvClipHeight * DATA_COMPGEN(0x00064a28, 1.0f / 6.0f);
    if (g_screenViewport->SetViewport2(&viewport) != D3D_OK) {
        return false;
    }
    if (g_screenDevice->SetCurrentViewport(g_screenViewport) != D3D_OK) {
        return false;
    }
    g_d3d->CreateMaterial(&material, NULL);
    ZeroMemory(&materialDesc, sizeof(materialDesc));
    materialDesc.dwSize = sizeof(materialDesc);
    material->SetMaterial(&materialDesc);
    material->GetHandle(g_d3dDevice, &backgroundHandle);
    g_viewport->SetBackground(backgroundHandle);
    g_d3d->CreateMaterial(&lighting, NULL);
    ZeroMemory(&lightingDesc, sizeof(lightingDesc));
    lightingDesc.dwSize = sizeof(lightingDesc);
    lightingDesc.diffuse.r = 1.0f;
    lightingDesc.diffuse.g = 1.0f;
    lightingDesc.diffuse.b = 1.0f;
    lightingDesc.diffuse.a = 1.0f;
    lightingDesc.ambient.r = 1.0f;
    lightingDesc.ambient.g = 1.0f;
    lightingDesc.ambient.b = 1.0f;
    lightingDesc.ambient.a = 1.0f;
    lightingDesc.dwRampSize = 16;
    lightingDesc.power = 1.0f;
    lighting->SetMaterial(&lightingDesc);
    lighting->GetHandle(g_d3dDevice, &lightingHandle);
    g_d3dDevice->SetLightState(D3DLIGHTSTATE_MATERIAL, lightingHandle);
    fogStart = DATA_COMPGEN(0x00064a2c, 800.5f);
    fogEnd = DATA_COMPGEN(0x00064a30, 1280.8f);
    g_d3dDevice->SetLightState(D3DLIGHTSTATE_FOGMODE, D3DFOG_LINEAR);
    // the pun: the light state takes the float's bits
    g_d3dDevice->SetLightState(D3DLIGHTSTATE_FOGSTART, *reinterpret_cast<DWORD*>(&fogStart));
    // the pun: the light state takes the float's bits
    g_d3dDevice->SetLightState(D3DLIGHTSTATE_FOGEND, *reinterpret_cast<DWORD*>(&fogEnd));
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR, RGB_MAKE(0xff, 0xff, 0xff));
    SetViewMatrix(g_viewMatrix, g_cameraFrom, g_cameraAt);
    g_d3dDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &g_viewMatrix);
    SetProjectionMatrix(g_projectionMatrix, 160.0f, 160.0f, 1281.0f);
    g_d3dDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &g_projectionMatrix);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, true);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    if (g_deviceSettings.caps.blendMode[static_cast<i32>(g_deviceType)] == BLEND_MODE_ALPHA) {
        SetDeviceAlphaBlend(g_d3dDevice, true);
    } else {
        SetDeviceAlphaBlend(g_d3dDevice, false);
    }
    if (g_deviceType != D3D_DEVICE_MMX) {
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    } else {
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
    }
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, true);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_ZVISIBLE, false);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, true);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    if (g_deviceSettings.caps.dither[static_cast<i32>(g_deviceType)]) {
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE, true);
    } else {
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE, false);
    }
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, false);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, true);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_ANTIALIAS, false);
    if (g_bilinearFiltering) {
        SetTextureFiltering(D3DFILTER_LINEAR);
    } else {
        SetTextureFiltering(D3DFILTER_NEAREST);
    }
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
    g_d3dDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_NEVER);
    if (g_deviceType == D3D_DEVICE_HAL) {
        g_screenDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
        g_screenDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, true);
        g_screenDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        g_screenDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        if (g_deviceSettings.caps.blendMode[static_cast<i32>(g_deviceType)] == BLEND_MODE_ALPHA) {
            SetDeviceAlphaBlend(g_screenDevice, true);
        } else {
            SetDeviceAlphaBlend(g_screenDevice, false);
        }
    } else {
        g_screenDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, true);
        g_screenDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
        g_screenDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, false);
        g_screenDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_DECAL);
        g_screenDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_NEVER);
        g_screenDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, false);
    }
    ZeroMemory(&g_textureFormat, sizeof(g_textureFormat));
    g_d3dDevice->EnumTextureFormats(ChooseTextureFormat, &g_textureFormat);
    return true;
}

// Opens DirectSound and its two effect buffers (a short one and one of nine
// seconds); 22 kHz 16-bit mono.
RVA(0x000488a0, 0x143)
b32 InitDirectSound(void) {
    WAVEFORMATEX format;
    DSBUFFERDESC desc;

    if (DirectSoundCreate(NULL, &g_directSound, NULL) != DS_OK) {
        return false;
    }
    if (g_directSound->SetCooperativeLevel(g_mainWindow, DSSCL_NORMAL) != DS_OK) {
        g_directSound->Release();
        return false;
    }
    ZeroMemory(&format, sizeof(format));
    format.cbSize = 0;
    ZeroMemory(&desc, sizeof(desc));
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = 22050;
    format.nBlockAlign = 2;
    format.wBitsPerSample = 16;
    format.nAvgBytesPerSec = 44100;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_CTRLDEFAULT;
    s_shortBufferSize = desc.dwBufferBytes = 0x204cc;
    desc.lpwfxFormat = &format;
    if (g_directSound->CreateSoundBuffer(&desc, &g_shortSoundBuffer, NULL) != DS_OK) {
        return false;
    }
    s_longBufferSize = desc.dwBufferBytes = format.nAvgBytesPerSec * 9;
    if (g_directSound->CreateSoundBuffer(&desc, &g_longSoundBuffer, NULL) != DS_OK) {
        return false;
    }
    g_shortSoundBuffer->SetVolume(-300);
    g_longSoundBuffer->SetVolume(-300);
    return true;
}

// Plays sound effect `sound` (a WAVE resource) through the buffer it fits;
// SOUND_STOP stops both buffers.
RVA(0x000489f0, 0x18a)
void PlaySoundEffect(i16 sound) {
    HRSRC resource;
    WaveResource* wave;
    DWORD size;
    DWORD flags;
    LPDIRECTSOUNDBUFFER buffer;
    DWORD capacity;
    DWORD status;
    void* data1;
    DWORD size1;
    void* data2;
    DWORD size2;

    if (!g_soundEnabled || sound < 1 || sound >= SOUND_COUNT) {
        return;
    }
    if (sound == SOUND_STOP) {
        g_shortSoundBuffer->Stop();
        g_longSoundBuffer->Stop();
        return;
    }
    if (s_soundResources[sound - 1] == 0) {
        return;
    }
    resource = FindResource(NULL, MAKEINTRESOURCE(s_soundResources[sound - 1]), "WAVE");
    if (resource == NULL) {
        return;
    }
    wave = static_cast<WaveResource*>(LockResource(LoadResource(NULL, resource)));
    if (wave == NULL) {
        return;
    }
    size = wave->dataSize;
    flags = 0;
    if (sound == SOUND_LOOP_FIRST || sound == SOUND_LOOP_SECOND) {
        flags = DSBPLAY_LOOPING;
    }
    buffer = g_shortSoundBuffer;
    capacity = s_shortBufferSize;
    if (size > 0x19000) {
        buffer = g_longSoundBuffer;
        capacity = s_longBufferSize;
    }
    buffer->GetStatus(&status);
    if (status & DSERR_BUFFERLOST) {
        buffer->Restore();
    }
    if (status & DSBSTATUS_PLAYING) {
        buffer->Stop();
    }
    if (buffer->Lock(0, size, &data1, &size1, &data2, &size2, 0) != DS_OK) {
        return;
    }
    memset(data1, 0, capacity);
    memcpy(data1, wave->samples, size1);
    buffer->Unlock(data1, size1, data2, size2);
    buffer->SetCurrentPosition(0);
    buffer->Play(0, 0, flags);
}

RVA(0x00048b80, 0xa5)
i16 MapEffectSoundId(i16 id) {
    if (id == 11) {
        return 25;
    }
    if (id == 19) {
        return 17;
    }
    if (id == 23) {
        return 52;
    }
    if (id == 30) {
        return 10;
    }
    if (id == 32) {
        return 10;
    }
    if (id == 59) {
        return 100;
    }
    if (id == 62) {
        return 60;
    }
    if (id == 63) {
        return 50;
    }
    if (id == 64 || id == 80) {
        return 39;
    }
    if (id == 83) {
        return 61;
    }
    if (id == 86) {
        return 45;
    }
    if (id == 103) {
        return 107;
    }
    if (id == 99) {
        return 83;
    }
    if (id == 106) {
        return 88;
    }
    return id;
}

RVA(0x00048c30, 0xb0)
i16 MapSoundEffectId(i16 id) {
    if (id == 11) {
        return 25;
    }
    if (id == 19) {
        return 17;
    }
    if (id == 23) {
        return 52;
    }
    if (id == 30) {
        return 10;
    }
    if (id == 32) {
        return 10;
    }
    if (id == 59) {
        return 5;
    }
    if (id == 62) {
        return 60;
    }
    if (id == 63) {
        return 50;
    }
    if (id == 64 || id == 80) {
        return 39;
    }
    if (id == 83) {
        return 61;
    }
    if (id == 86) {
        return 45;
    }
    if (id == 103) {
        return 107;
    }
    if (id == 99) {
        return 83;
    }
    if (id == 106) {
        return 88;
    }
    return id;
}

// Opens DirectInput with the system mouse and keyboard, non-exclusive in the
// foreground.
RVA(0x00048ce0, 0xc1)
b32 InitDirectInput(void) {
    if (FAILED(DirectInputCreate(g_instance, DIRECTINPUT_VERSION, &g_directInput, NULL))) {
        return false;
    }
    if (FAILED(g_directInput->CreateDevice(GUID_SysMouse, &g_mouseDevice, NULL))) {
        return false;
    }
    if (FAILED(g_mouseDevice->SetDataFormat(&c_dfDIMouse))) {
        return false;
    }
    if (FAILED(
            g_mouseDevice->SetCooperativeLevel(g_mainWindow, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)
        )) {
        return false;
    }
    if (FAILED(g_directInput->CreateDevice(GUID_SysKeyboard, &g_keyboardDevice, NULL))) {
        return false;
    }
    if (FAILED(g_keyboardDevice->SetDataFormat(&c_dfDIKeyboard))) {
        return false;
    }
    return SUCCEEDED(
        g_keyboardDevice->SetCooperativeLevel(g_mainWindow, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)
    );
}

RVA(0x00048db0, 0x62)
void ReleaseDirectInput(void) {
    ReleaseInputDevice(g_mouseDevice);
    ReleaseInputDevice(g_keyboardDevice);
    ReleaseComObject(g_directInput);
}

RVA(0x00048e20, 0x3d)
void AcquireInput(BOOL acquire) {
    SetInputDeviceAcquired(g_mouseDevice, acquire);
    SetInputDeviceAcquired(g_keyboardDevice, acquire);
}

// The mouse buttons' state and edges: bit 0 left down, 1 left was down, 2 left
// pressed, 3 left released; bits 4-7 the same for the right button.
RVA(0x00048e60, 0xbb)
GZ_ENUM_RETURN(MouseButtonBits, u8) PollMouseButtons(void) {
    DIMOUSESTATE state;
    HRESULT result;
    u8 buttons = MOUSE_BUTTONS_NONE;
    u8 left;
    u8 right;

    if (g_mouseDevice != NULL) {
        for (;;) {
            result = g_mouseDevice->GetDeviceState(sizeof(state), &state);
            if (result != DIERR_INPUTLOST && result != DIERR_NOTACQUIRED
                || SUCCEEDED(result = g_mouseDevice->Acquire())) {
                if (result == DIERR_INPUTLOST) {
                    continue;
                }
                left = state.rgbButtons[0];
                right = state.rgbButtons[1];
            } else {
                right = left = 0;
            }
            break;
        }
        if (FAILED(result)) {
            right = left = 0;
        }
    } else {
        right = left = 0;
    }
    left &= 0x80;
    right &= 0x80;
    if (left) {
        buttons = MOUSE_LEFT_DOWN;
    }
    RecordMouseButtonEdges(buttons, left, s_leftButton, 0);
    if (right) {
        buttons |= MOUSE_RIGHT_DOWN;
    }
    RecordMouseButtonEdges(buttons, right, s_rightButton, MOUSE_RIGHT_SHIFT);
    return static_cast<GZ_ENUM_RETURN(MouseButtonBits, u8)>(buttons);
}

// Clears the primary surface and, on the HAL device, the render target.
RVA(0x00048f20, 0x61)
void ClearScreenSurfaces(void) {
    DDBLTFX fx;

    ZeroMemory(&fx, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = 0;
    if (g_deviceType == D3D_DEVICE_HAL) {
        g_primarySurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        g_renderTarget->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    } else {
        g_primarySurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    }
}

// The DX5 d3dtypes.h/d3dvec.inl inlines this TU instantiates (/Ob0 emits them
// out of line), in retail's COMDAT order.
RVA_COMPGEN(0x00048f90, 0x74, ??0_D3DMATRIX@@QAE@MMMMMMMMMMMMMMMM@Z)
RVA_COMPGEN(0x00049010, 0x11, ??R_D3DMATRIX@@QAEAAMHH@Z)
RVA_COMPGEN(0x00049030, 0x19, ??0_D3DVECTOR@@QAE@MMM@Z)
RVA_COMPGEN(0x00049050, 0x36, ??G@YA?AU_D3DVECTOR@@ABU0@0@Z)
RVA_COMPGEN(0x00049090, 0x20, ?Normalize@@YA?AU_D3DVECTOR@@ABU1@@Z)
RVA_COMPGEN(0x000490b0, 0x34, ??K@YA?AU_D3DVECTOR@@ABU0@M@Z)
RVA_COMPGEN(0x000490f0, 0x10, ?Magnitude@@YAMABU_D3DVECTOR@@@Z)
RVA_COMPGEN(0x00049100, 0x35, ?SquareMagnitude@@YAMABU_D3DVECTOR@@@Z)
RVA_COMPGEN(0x00049140, 0x25, ?DotProduct@@YAMABU_D3DVECTOR@@0@Z)
RVA_COMPGEN(0x00049170, 0x123, ?CrossProduct@@YA?AU_D3DVECTOR@@ABU1@0@Z)
// Both vector index overloads share one retail COMDAT body.
RVA_COMPGEN(0x000492a0, 0xa, ??A_D3DVECTOR@@QAEAAMH@Z)
RVA_COMPGEN(0x000492a0, 0xa, ??A_D3DVECTOR@@QBEABMH@Z)

// The empty vector and vertex constructors share one retail COMDAT body.
RVA_COMPGEN(0x000568c0, 0x3, ??0_D3DVECTOR@@QAE@XZ)
