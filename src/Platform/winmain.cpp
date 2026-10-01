// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Gfx/DisplayConfig.h>
#include <Gfx/Texture.h>
#include <Giten/Resource.h>
#include <Platform/Com.h>
#include <Platform/GameApi.h>
#include <Platform/Ime.h>
#include <Platform/InputSound.h>
#include <Platform/Joystick.h>
#include <Platform/Mesh.h>
#include <Platform/Scene3D.h>
#include <Platform/WindowsX.h>
#include <Platform/WinMain.h>
#include <Platform/WinMM.h>
#include <Sound/MidiStream.h>
#include <Text/TextAttr.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The battle input fixes read the game clock. The matching build must not
// include another header: it perturbs MSVC 5's register allocation.
#ifdef GITEN_BUGFIX
#include <Game/Clock.h>
#endif

// Billboard brightness remains full within two cells, then attenuates by distance.
#define SetDistanceLight(light, distance)                                                                                                         \
    do {                                                                                                                                          \
        if ((distance) < 640.0) {                                                                                                                 \
            (light) = 1.0f;                                                                                                                       \
        } else {                                                                                                                                  \
            (light) =                                                                                                                             \
                (DATA_COMPGEN(0x00064a88, 320.0) - (distance) * DATA_COMPGEN(0x00064a80, 1.0 / 6.0)) / ((distance) - DATA_COMPGEN(0x00064a90, 323.2)); \
        }                                                                                                                                         \
    } while (0)

DATA(0x0008778c)
HWND g_mainWindow;

DATA(0x0008fcf0)
RECT g_windowRect;

DATA(0x0006b4e0)
i32 g_selectedHotspot = HOTSPOT_NONE;

DATA(0x00084800)
Texture g_roomTexture;

DATA(0x00084d20)
Texture g_enemyTextures[2][ENEMY_TEXTURE_FRAMES];

DATA(0x00088010)
Hotspot g_hotspots[64];

DATA(0x00088a10)
Picture g_spriteImages[SPRITE_GROUP_COUNT][SPRITE_FRAME_COUNT];

DATA(0x0008d700)
Picture g_scenePicture;

DATA(0x0008d728)
SpriteSlot g_spriteSlots[SPRITE_SLOT_COUNT];

DATA(0x0008d860)
Texture g_objectTextures[OBJECT_TEXTURE_COUNT];

// The layers from the topmost down.
DATA(0x0008f290)
ScreenLayer* g_layerStack[SCREEN_LAYER_COUNT];

DATA(0x0008f588)
i16 g_spriteOrder[SPRITE_SLOT_COUNT];

// Where the dragged layer was grabbed, from its top left (the drop position
// is g_dragRect's top left).
DATA(0x0008faf8)
POINT g_dragOffset;

DATA(0x0008fb04)
u32 g_hotspotCount;

// @identity-TODO: the layer table is filled by CreateScreenLayer in font.cpp.
DATA(0x0008fb10)
ScreenLayer* g_screenLayers[SCREEN_LAYER_COUNT];

DATA(0x0008fda8)
TextPlane g_textPlanes[TEXT_PLANE_COUNT];

// The camera's eye and the point it looks at (the party's position); the view
// matrix is rebuilt from them.
// @identity-TODO: the eye/at roles are read from the ViewMatrix-style call at
// 0x4adff (view, 0x48f2d8, 0x4847c0) only.
RVA_DYNINIT(0x00049530, 0x5, g_cameraFrom)
RVA_DYNINIT(0x00049540, 0x17, g_cameraFrom)
DATA(0x0008f2d8)
D3DVECTOR g_cameraFrom(0.0f, 160.0f, -160.0f);

RVA_DYNINIT(0x00049560, 0x5, g_cameraAt)
RVA_DYNINIT(0x00049570, 0x14, g_cameraAt)
DATA(0x000847c0)
D3DVECTOR g_cameraAt(0.0f, 160.0f, 0.0f);

// The view and projection transforms.
// @identity-TODO: 0x48f470's projection role is inferred from its
// SetTransform use in the device setup (0x447ef0).
RVA_DYNINIT(0x00049590, 0x5, g_viewMatrix)
RVA_DYNINIT(0x000495a0, 0xa, g_viewMatrix)
DATA(0x0008fcb0)
D3DMATRIX g_viewMatrix;

RVA_DYNINIT(0x000495b0, 0x5, g_projectionMatrix)
RVA_DYNINIT(0x000495c0, 0xa, g_projectionMatrix)
DATA(0x0008f470)
D3DMATRIX g_projectionMatrix;

RVA_DYNINIT(0x000495d0, 0xa, g_ime)
RVA_DYNINIT(0x000495e0, 0xa, g_ime)
RVA_DYNINIT(0x000495f0, 0xe, g_ime)
RVA_DYNINIT(0x00049600, 0xa, g_ime)
DATA(0x00084358)
CIme g_ime;

// The 24 quad vertices' screen y, four per quad.
DATA(0x0006bc98)
static i32 s_quadY[24] = {
    320, 320, 0, 0, 320, 320, 0, 0, 320, 320, 0,   0,
    320, 320, 0, 0, 320, 320, 0, 0, 320, 320, 284, 284,
};

DATA(0x0006bcf8)
static D3DVALUE s_quadUV[24][2] = {
    {0.0f, 0.0f},    {0.0f, 0.0f},      {0.0f, 0.0f},   {0.0f, 0.0f},      {0.0f, 0.505f},
    {0.25f, 0.505f}, {0.25f, 1.0f},     {0.0f, 1.0f},   {0.25f, 0.505f},   {0.5f, 0.505f},
    {0.5f, 1.0f},    {0.25f, 1.0f},     {0.0f, 0.505f}, {0.0625f, 0.505f}, {0.0625f, 1.0f},
    {0.0f, 1.0f},    {0.4375f, 0.505f}, {0.5f, 0.505f}, {0.5f, 1.0f},      {0.4375f, 1.0f},
    {0.0f, 0.505f},  {0.5f, 0.505f},    {0.5f, 0.563f}, {0.0f, 0.563f},
};

DATA(0x0006bdb8)
static u16 s_quadIndices[6] = {0, 1, 2, 0, 2, 3};

// The direction the party faces (0..3).
DATA(0x000847ac)
GZ_ENUM_STORAGE(ViewDirection, i32) g_viewDirection;

DATA(0x000847b8)
b32 g_fixedLighting;

// Set while a screen layer is being dragged (see g_dragRect).
DATA(0x0008fb08)
static b32 s_layerDragging;

DATA(0x00090abc)
static GZ_ENUM_STORAGE(SpriteLayerMode, i16) s_spriteMode;

DATA(0x00090ac0)
static i16 s_frameCount;

DATA(0x000847f8)
static GZ_ENUM_STORAGE(BlankRenderStep, i32) s_blankStep;

DATA(0x0008f2e4)
static u16 s_savedRenderMode;

DATA(0x0008f310)
GZ_ENUM_STORAGE(RenderMode, i16) g_renderMode;

DATA(0x0008f60c)
CMidiStream* g_midiStream;

DATA(0x0008fc60)
static b32 s_immediateInput;

DATA(0x00090ab8)
static b32 s_cursorArmed;

DATA(0x00090ac4)
static i32 s_busyFrame;

DATA(0x00090ac8)
static b32 s_screenSaved;

// The busy cursor animates while this is set; it starts set until the first
// frame clears it.
DATA(0x0006b968)
static b32 s_busyCursor = true;

// The music track PlayMusic last started (-1 = none).
DATA(0x0006b96c)
static i16 s_musicTrack = -1;

DATA(0x0006b970)
static const char* s_musicFiles[26] = {
    "s\\sm000.mds", "s\\sm001.mds", "s\\sm002.mds", "s\\sm003.mds", "s\\sm004.mds", "s\\sm005.mds",
    "s\\sm006.mds", "s\\sm007.mds", "s\\sm008.mds", "s\\sm009.mds", "s\\sm00a.mds", "s\\sm00b.mds",
    "s\\sm00c.mds", "s\\sm00d.mds", "s\\sm00e.mds", "s\\sm00f.mds", "s\\sm010.mds", "s\\sm011.mds",
    "s\\bgm00.mds", "s\\bgm01.mds", "s\\bgm02.mds", "s\\bgm03.mds", "s\\bgm04.mds", "s\\bgm05.mds",
    NULL,           NULL,
};

RVA(0x00049610, 0x1)
void DebugTrace(const char* message) {}

RVA(0x00049620, 0x18)
void* ReallocBlock(void* block, u16 size) {
    return realloc(block, size);
}

RVA(0x00049640, 0x3e)
void* AllocCleared(u16 count, u16 size) {
    u32 bytes = count * size;
    void* block = new u8[bytes];

    if (block != NULL) {
        memset(block, 0, bytes);
    }
    return block;
}

RVA(0x00049680, 0x37)
void* AllocClearedLong(u16 count, u32 size) {
    u32 bytes = count * size;
    void* block = new u8[bytes];

    if (block != NULL) {
        memset(block, 0, bytes);
    }
    return block;
}

RVA(0x000496c0, 0x14)
void* FreeBlock(void* block) {
    if (block != NULL) {
        delete static_cast<u8*>(block); // the blocks come from AllocCleared's new u8[]
    }
    return NULL;
}

// Starts the busy cursor unless the party is exploring.
RVA(0x000496e0, 0x1e)
void ShowBusyCursor(void) {
    if (!GetFieldBattleActive() && !s_busyCursor) {
        s_busyCursor = true;
    }
}

RVA(0x00049700, 0xb)
void HideBusyCursor(void) {
    s_busyCursor = false;
}

// Builds the path of save slot `slot` in the Windows directory.
RVA(0x00049710, 0x5e)
void GetSavePath(char* path, i16 slot) {
    char name[64];

    GetWindowsDirectory(path, 256);
    sprintf(name, "\\save%.4x.dds", slot);
    strcat(path, name);
}

RVA(0x00049770, 0xa)
void RebuildViewScene(void) {
    BuildRoomGeometry();
    ResetCamera();
}

RVA(0x00049780, 0xb)
void CancelLayerDrag(void) {
    s_layerDragging = false;
}

RVA(0x00049790, 0x6)
GZ_ENUM_RETURN(ViewDirection, i32) GetViewDirection(void) {
    return g_viewDirection;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000497a0, 0x1b)
void WaitMilliseconds(DWORD ms) {
    DWORD end = timeGetTime() + ms;

    do {
    } while (timeGetTime() < end);
}

// @identity-TODO: a second copy of ClearSelectedHotspot (0x458580); which
// callers use which copy is all that tells them apart.
RVA(0x000497c0, 0xb)
void InvalidateSelectedHotspot(void) {
    g_selectedHotspot = HOTSPOT_NONE;
}

RVA(0x000497d0, 0x25)
void ResetSprites(GZ_ENUM_PARAM(SpriteLayerMode, i16) mode) {
    i16 slot;

    s_spriteMode = mode;
    ClearScenePicture();
    for (slot = 0; slot < SPRITE_GROUP_COUNT; slot++) {
        FreeSpriteImages(slot);
    }
}

RVA(0x00049800, 0x7)
GZ_ENUM_RETURN(SpriteLayerMode, i16) GetSpriteMode(void) {
    return s_spriteMode;
}

// @identity-TODO: the frames counted here gate the message handler's input
// delay (see AllowImmediateInput); the names are provisional.
RVA(0x00049810, 0x8)
void TickFrameCount(void) {
    s_frameCount++;
}

RVA(0x00049820, 0x7)
i16 GetFrameCount(void) {
    return s_frameCount;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00049830, 0xa)
void ResetFrameCount(void) {
    s_frameCount = 0;
}

// The cursor clip rectangle ConfineCursor saved.
DATA(0x0008f5f8)
RECT g_savedClipRect;

// Saves the cursor clip rectangle (the desktop when none) and confines the
// cursor to the window; the saved rectangle is emptied on failure.
RVA(0x00049840, 0x64)
void ConfineCursor(void) {
    if (!GetClipCursor(&g_savedClipRect)) {
        if (!GetWindowRect(GetDesktopWindow(), &g_savedClipRect)) {
            ZeroMemory(&g_savedClipRect, sizeof(g_savedClipRect));
            return;
        }
    }
    if (!ClipCursor(&g_windowRect)) {
        ZeroMemory(&g_savedClipRect, sizeof(g_savedClipRect));
    }
}

// Restores a clip rectangle ConfineCursor saved (an empty one is skipped).
RVA(0x000498b0, 0x13)
void RestoreCursorClip(RECT* rect) {
    if (rect->right != 0) {
        ClipCursor(rect);
    }
}

// Draws the mouse cursor (the animated busy cursor while it is set) at the
// cursor position; the first call only arms it.
RVA(0x000498d0, 0x104)
void DrawMouseCursor(void) {
    POINT position;
    RECT source;

    source = g_cursorPicture.rect;
    GetCursorPos(&position);
    position.x -= 3;
    position.y -= 3;
    if (position.x < 0) {
        position.x = 0;
    }
    if (position.y < 0) {
        position.y = 0;
    }
    if (position.x > 608) {
        source.right = 640 - position.x;
    }
    if (position.y > 448) {
        source.bottom = 480 - position.y;
    }
    if (!s_cursorArmed) {
        s_cursorArmed = true;
        return;
    }
    if (!s_busyCursor) {
        g_renderTarget->BltFast(
            position.x,
            position.y,
            g_cursorPicture.surface,
            &source,
            DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT
        );
        return;
    }
    if (s_busyFrame > 7) {
        g_renderTarget->BltFast(
            position.x,
            position.y,
            g_busyCursorPicture.surface,
            &source,
            DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT
        );
    }
    s_busyFrame = (s_busyFrame + 1) & 15;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000499e0, 0x69)
void DrawDebugText(const char* text, i32 x, i32 y) {
    HDC dc;

    g_renderTarget->GetDC(&dc);
    SetBkColor(dc, 0);
    SetTextColor(dc, RGB(0, 255, 0));
    TextOut(dc, x, y, text, strlen(text));
    g_renderTarget->ReleaseDC(dc);
}

RVA(0x00049a50, 0x10)
void StopMusic(i16 unused) {
    if (g_midiStream != NULL) {
        g_midiStream->Stop();
    }
}

RVA(0x00049a60, 0x106)
i16 PlayMusic(i16 track, i16 loop) {
    i16 previous;

    if (!g_soundEnabled) {
        return 0;
    }
    if (track < 0) {
        StopMusic(0);
    } else {
        if (s_musicTrack == track) {
            return s_musicTrack;
        }
        if (g_midiStream != NULL) {
            g_midiStream->Stop();
            delete g_midiStream;
            g_midiStream = NULL;
        }
        if (track < 24 && s_musicFiles[track] != NULL) {
            g_midiStream = new CMidiStream;
            if (g_midiStream != NULL) {
                g_midiStream->Play(s_musicFiles[track], NULL, true, 100, NULL);
            }
        }
    }
    previous = s_musicTrack;
    s_musicTrack = track;
    return previous;
}

RVA(0x00049b70, 0x7)
i16 CurrentMusicTrack(void) {
    return s_musicTrack;
}

RVA(0x00049b80, 0xb)
void AllowImmediateInput(void) {
    s_immediateInput = true;
}

RVA(0x00049b90, 0xe)
void ResetRenderMode(void) {
    g_renderMode = RENDER_MODE_EVENT;
    UnplaceAllSprites();
}

RVA(0x00049ba0, 0xe)
void SetViewRenderMode(void) {
    g_renderMode = RENDER_MODE_VIEW;
    ClearScenePicture();
}

RVA(0x00049bb0, 0x15)
void SetPanelRenderMode(void) {
    g_renderMode = RENDER_MODE_PANEL;
    g_screenLayers[SCREEN_LAYER_NAVIGATION]->visible = false;
}

RVA(0x00049bd0, 0xa)
void SetSceneRenderMode(void) {
    g_renderMode = RENDER_MODE_SCENE;
}

// @identity-TODO: that mode 15 is the save screen is inferred from its caller
// OpSaveDataCommand.
RVA(0x00049be0, 0x14)
void EndSaveRenderMode(void) {
    if (g_renderMode == RENDER_MODE_SAVE) {
        g_renderMode = RENDER_MODE_SCENE;
    }
}

// @identity-TODO: what render mode 4 shows over the cleared status picture is
// unrecovered.
RVA(0x00049c00, 0x25)
void SetPictureRenderMode(void) {
    g_renderMode = RENDER_MODE_PICTURE;
    ClearDisplaySurface(g_statusPicture.surface, NULL);
}

RVA(0x00049c30, 0x19)
void RefreshScreenMode(void) {
    if (s_savedRenderMode == RENDER_MODE_VIEW) {
        SetViewRenderMode();
        return;
    }
    g_renderMode = RENDER_MODE_VIEW_FRAME;
}

RVA(0x00049c50, 0x21)
void SaveScreenMode(void) {
    s_savedRenderMode = g_renderMode;
    g_selectedHotspot = HOTSPOT_NONE;
    s_screenSaved = true;
}

RVA(0x00049c80, 0x2a)
void RestoreScreenMode(void) {
    g_renderMode = static_cast<GZ_ENUM_STORAGE(RenderMode, i16)>(s_savedRenderMode);
    s_savedRenderMode = 0xffff;
    g_selectedHotspot = HOTSPOT_NONE;
    s_screenSaved = false;
}

RVA(0x00049cb0, 0xa)
void SetFieldRenderMode(void) {
    g_renderMode = RENDER_MODE_FIELD;
}

RVA(0x00049cc0, 0xa)
void SetStatusRenderMode(void) {
    g_renderMode = RENDER_MODE_STATUS;
}

// Selects screen layers and text planes for the automap.
RVA(0x00049cd0, 0xa)
void SetLayersRenderMode(void) {
    g_renderMode = RENDER_MODE_LAYERS;
}

RVA(0x00049ce0, 0x14)
void SetBlankRenderMode(void) {
    g_renderMode = RENDER_MODE_BLANK;
    s_blankStep = BLANK_STEP_NONE;
}

RVA(0x00049d00, 0xb)
void SetBlankStep(GZ_ENUM_PARAM(BlankRenderStep, i16) step) {
    s_blankStep = step;
}

RVA(0x00049d10, 0x35)
void StepBlankRenderMode(void) {
    if (s_blankStep == BLANK_STEP_SECOND) {
        s_blankStep = BLANK_STEP_NONE;
        g_renderMode = RENDER_MODE_SCENE;
    } else if (s_blankStep == BLANK_STEP_FIRST) {
        s_blankStep = BLANK_STEP_SECOND;
        g_renderMode = RENDER_MODE_BLANK;
    }
}

RVA(0x00049d50, 0x7)
GZ_ENUM_RETURN(RenderMode, i16) GetRenderMode(void) {
    return g_renderMode;
}

DATA(0x00084360)
static b32 s_viewDirty;

DATA(0x000847bc)
static i16 s_fadeCountdown;

DATA(0x00084c64)
static i32 s_fadeAlpha;

DATA(0x0008d290)
b32 g_boxOpening;

DATA(0x0008d830)
static b32 s_screenCovered;

DATA(0x0008d834)
static b16 s_pendingKey;

// The running fade's mode (0 = none).
DATA(0x0008f1d4)
GZ_ENUM_STORAGE(ScreenFadeMode, i16) g_fadeMode;

DATA(0x0008f2cc)
D3DVALUE g_billboardX;

DATA(0x0008f2d0)
D3DVALUE g_billboardZ;

DATA(0x0008f2f8)
D3DCOLOR g_fadeColor;

DATA(0x0008f308)
i32 g_hotspotCellX;

DATA(0x0008f30c)
i32 g_hotspotCellY;

DATA(0x0008f300)
u32 g_turnStep;

DATA(0x0008f444)
GZ_ENUM_STORAGE(CameraMoveState, u32) g_moveState;

// The move's progress: camera units slid, or turn steps.
DATA(0x0008f4d4)
static D3DVALUE s_moveProgress;

// The previous frame's move state (a new move restarts its progress).
DATA(0x0008f558)
static GZ_ENUM_STORAGE(CameraMoveState, u32) s_lastMoveState;

DATA(0x0008f414)
TreasureBox* g_openingBox;

DATA(0x0008f580)
static BOOL s_viewChanged;

DATA(0x0008f5c8)
i32 g_hotspotObject;

DATA(0x0008f614)
static i16 s_fadeSteps;

// Starts a fade of `steps` frames per alpha step: odd modes fade in from
// opaque, even modes out to opaque; modes above 4 fade through white. Ignored
// while a fade runs or, for fades out, while the screen is already covered.
RVA(0x00049d60, 0x6a)
void StartScreenFade(GZ_ENUM_PARAM(ScreenFadeMode, i16) mode, i16 steps) {
    if (g_fadeMode != SCREEN_FADE_NONE) {
        return;
    }
    if (s_screenCovered && !IsScreenFadeIn(mode)) {
        return;
    }
    g_fadeMode = mode;
    s_fadeSteps = steps;
    s_fadeCountdown = steps;
    s_fadeAlpha = IsScreenFadeIn(mode) ? 0xff : 0;
    if (mode > 4) {
        g_fadeColor = RGBA_MAKE(0xff, 0xff, 0xff, s_fadeAlpha);
    } else {
        g_fadeColor = RGBA_MAKE(0, 0, 0, s_fadeAlpha);
    }
}

#define SetScreenFadeAlpha(value)                                                                  \
    do {                                                                                           \
        s_fadeAlpha = (value);                                                                     \
        g_fadeColor = RGBA_SETALPHA(g_fadeColor, (value));                                         \
    } while (0)

// Advances the running fade by one frame.
// @early-stop register residue: ecx/edx swap for the zero and the step
// reload, `add -17` for retail's `sub 17`, and the colour's or-operands in
// the other order; the CFG and every store match. Alpha spellings
// (compound assignment, RGBA_SETALPHA, operand order) are flat or worse.
RVA(0x00049dd0, 0xb3)
void StepScreenFade(void) {
    i32 alpha;

    if (g_fadeMode == SCREEN_FADE_NONE) {
        return;
    }
    if (--s_fadeCountdown != 0) {
        return;
    }
    s_screenCovered = false;
    s_fadeCountdown = s_fadeSteps;
    if (IsScreenFadeIn(g_fadeMode)) {
        if (s_fadeAlpha <= 0) {
            g_fadeMode = SCREEN_FADE_NONE;
            return;
        }
        alpha = s_fadeAlpha - 17;
    } else {
        if (s_fadeAlpha >= 0xff) {
            g_fadeMode = SCREEN_FADE_NONE;
            s_screenCovered = true;
            SetScreenFadeAlpha(0xff);
            if (g_renderMode == RENDER_MODE_VIEW && g_scenePicture.visible) {
                ClearScenePicture();
            }
            return;
        }
        alpha = s_fadeAlpha + 17;
    }
    SetScreenFadeAlpha(alpha);
}

RVA(0x00049e90, 0x7)
GZ_ENUM_RETURN(ScreenFadeMode, i16) GetScreenFade(void) {
    return g_fadeMode;
}

// Runs frames until the fade ends, then clears the input and the render
// target.
RVA(0x00049ea0, 0x57)
void FinishScreenFade(void) {
    while (g_fadeMode != SCREEN_FADE_NONE) {
        RenderFrame();
    }
    s_screenCovered = false;
    PollInput();
    ClearMouseClicks();
    SetMouseState(MOUSE_BUTTONS_NONE, 0, 0);
    ClearDisplaySurface(g_renderTarget, NULL);
}

// Leaves the render mode and runs `count` frames.
RVA(0x00049f00, 0x30)
void WaitFrames(i16 count) {
    i16 frame;

    GetTextPlane(0)->visible = false;
    ResetRenderMode();
    for (frame = 0; frame <= count - 1; frame++) {
        PollInput();
        RenderFrame();
    }
}

// Set while the application is active (MainWindowProc).
DATA(0x000847a8)
static BOOL s_appActive;

#ifdef GITEN_COMPAT
// @bug Retail removes window messages only in WinMain's loop. A nested modal
// loop that runs frames itself (RunBagDiscardMenu's, through RunFrame) takes
// none for as long as the player keeps it open: Windows NT marks the window
// Not Responding and ghosts it, a missed WM_ACTIVATEAPP leaves the input
// acquired and the cursor confined after switching away, and wrappers such as
// DxWnd that work through the message queue stall. And while the application
// is inactive, WinMain's loop spins without waiting, dispatching the last
// message again on every pass.
// This removes and dispatches the pending messages as WinMain's loop does and
// exits on WM_QUIT as it does; while the application is inactive it waits for
// the next message instead of spinning. Its callers run outside any window
// procedure, so the dispatch does not re-enter one.
static void PumpMessages(void) {
    MSG message;

    for (;;) {
        if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                exit(0);
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        } else if (s_appActive) {
            return;
        } else {
            WaitMessage();
        }
    }
}
#endif

RVA(0x00049f30, 0xf)
void RunFrame(void) {
#ifdef GITEN_COMPAT
    PumpMessages();
#endif
    PollInput();
    LatchMouseClicks();
    RenderFrame();
}

// The eighth move command: nothing to do.
RVA(0x00049f40, 0x6)
static b32 NoMoveCommand(i16 nextPhase) {
    return true;
}

// The move commands by command number.
DATA(0x0006b9d8)
static BOOL (*s_moveCommands[8])(i16 nextPhase) = {
    MoveForwardCommand,
    MoveRightCommand,
    MoveBackCommand,
    MoveLeftCommand,
    TurnRightCommand,
    TurnAroundCommand,
    TurnLeftCommand,
    NoMoveCommand,
};

// Hides layer 1's panel and runs move command `command` (0..7).
RVA(0x00049f50, 0x45)
BOOL RunMoveCommand(i16 command, i16 nextPhase) {
    if (g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
        g_screenLayers[SCREEN_LAYER_PANEL]->visible = false;
        ClearPanelLayerSurface();
    }
    return s_moveCommands[command & 7](nextPhase);
}

RVA(0x00049fa0, 0x1e)
void RedrawFieldView(void) {
    if (g_renderMode == RENDER_MODE_VIEW) {
        s_viewDirty = true;
        s_viewChanged = true;
        DrawFieldView();
    }
}

// Opens `box`: its closed frame shows open until the view redraws.
RVA(0x00049fc0, 0x1e)
void OpenTreasureBox(TreasureBox* box) {
    s_viewDirty = true;
    s_viewChanged = true;
    g_boxOpening = true;
    g_openingBox = box;
}

RVA(0x00049fe0, 0x51)
void RequestObjectRedraw(i16 index, i16 x, i16 y) {
    if (g_renderMode == RENDER_MODE_VIEW) {
        s_viewDirty = true;
        s_viewChanged = true;
        if (g_hotspotObject == index && (g_hotspotCellX != x || g_hotspotCellY != y)) {
            g_selectedHotspot = HOTSPOT_NONE;
        }
        DrawFieldView();
    }
}

// Rotates a 16-bit mask right by four bits per step of `direction`.
RVA(0x0004a040, 0x3b)
u16 RotateByDirection(u16 mask, i16 direction) {
    u16 bits;

    for (bits = (direction & 3) << 2; bits > 0; bits--) {
        if (mask & 1) {
            mask = (mask >> 1) | 0x8000;
        } else {
            mask >>= 1;
        }
    }
    return mask;
}

// Set by a right click off the navigation pad (HandleInput) and cleared
// while the right button is up.
// @identity-TODO: the name is kept from before HandleInput was read; what its
// readers do with it (a cancel?) is unconfirmed.
RVA(0x0004a080, 0x7)
i16 GetPendingKey(void) {
    return s_pendingKey;
}

RVA(0x0004a090, 0xa)
void ClearPendingKey(void) {
    s_pendingKey = false;
}

RVA(0x0004a0a0, 0x17)
void HideTextPlane(i16 plane) {
    GetTextPlane(plane)->visible = false;
}

// The joystick bits of this frame and the last, and those newly set.
DATA(0x0008f610)
static u32 s_joystickBits;

DATA(0x0008f2f0)
static u32 s_lastJoystickBits;

// @identity-TODO: written here only; nothing reads it.
DATA(0x0008f304)
static u32 s_joystickPressed;

// Reads the joystick for this frame.
RVA(0x0004a0c0, 0x37)
void PollJoystick(void) {
    JoystickState state;

    s_lastJoystickBits = s_joystickBits;
    s_joystickBits = ReadJoystick(&state);
    s_joystickPressed = (s_lastJoystickBits ^ 0xffff) & s_joystickBits;
}

// Set once DirectX is up (RenderFrame draws only then).
DATA(0x000847f4)
static b32 s_directXReady;

// Releases the display: the automap, the text planes, the sprite images, the
// meshes and textures, every picture's surface, the layers and the glyph
// surface, input, sound and the music stream, then the Direct3D and
// DirectDraw objects (InitDirectX runs it before bringing DirectX back up).
RVA(0x0004a100, 0x3eb)
void ReleaseGraphics(void) {
    i32 i;
    i32 j;

    FreeAutomap();
    for (i = 0; i < TEXT_PLANE_COUNT; i++) {
        FreeTextPlane(i);
    }
    for (i = 0; i < SPRITE_GROUP_COUNT; i++) {
        for (j = 0; j < SPRITE_FRAME_COUNT; j++) {
            ReleaseComObject(GetSpriteFramePicture(i, j)->surface);
        }
    }
    FreeMesh(&g_roomMesh);
    FreeMesh(&g_wallMesh);
    FreeMesh(&g_doorMesh);
    ReleaseTexture(&g_textBoxTexture);
    ReleaseTexture(&g_roomTexture);
    ReleaseTexture(&g_npcTexture);
    ReleaseTexture(&g_stairsUpTexture);
    ReleaseTexture(&g_stairsDownTexture);
    ReleaseTexture(&g_darkWallTexture);
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 5; j++) {
            ReleaseTexture(&g_enemyTextures[i][j]);
        }
    }
    ReleaseComObject(g_viewCachePicture.surface);
    ReleaseComObject(g_targetPicture.surface);
    ReleaseComObject(g_fightBannerPicture.surface);
    ReleaseComObject(g_titleMenuPicture.surface);
    ReleaseComObject(g_statusPortraitPicture.surface);
    ReleaseComObject(g_commandBarPicture.surface);
    ReleaseComObject(g_statusPicture.surface);
    ReleaseComObject(g_leftFieldMessagePicture.surface);
    ReleaseComObject(g_centerFieldMessagePicture.surface);
    ReleaseComObject(g_rightFieldMessagePicture.surface);
    ReleaseComObject(g_mapMarkerPicture.surface);
    ReleaseComObject(g_iconStagingPicture.surface);
    ReleaseComObject(g_backdropPicture.surface);
    ReleaseComObject(g_effectFramePicture.surface);
    ReleaseComObject(g_whitePicture.surface);
    ReleaseComObject(g_cursorPicture.surface);
    ReleaseComObject(g_busyCursorPicture.surface);
    ReleaseComObject(g_spritePicture.surface);
    for (i = 0; i < ENEMY_PICTURE_COUNT; i++) {
        ReleaseComObject(g_enemyPictures[i].surface);
    }
    ReleaseComObject(g_scenePicture.surface);
    FreeScreenLayers();
    FreeGlyphSurface();
    ReleaseDirectInput();
    ReleaseComObject(g_shortSoundBuffer);
    ReleaseComObject(g_longSoundBuffer);
    ReleaseComObject(g_directSound);
    if (g_midiStream != NULL) {
        g_midiStream->Stop();
        delete g_midiStream;
        g_midiStream = NULL;
    }
    ReleaseComObject(g_screenViewport);
    ReleaseComObject(g_screenDevice);
    ReleaseComObject(g_viewport);
    ReleaseComObject(g_d3dDevice);
    ReleaseComObject(g_d3d);
    ReleaseComObject(g_zBuffer);
    ReleaseComObject(g_roomObject);
    ReleaseComObject(g_renderTarget);
    ReleaseComObject(g_primarySurface);
    ReleaseComObject(g_ddraw2);
    ReleaseComObject(g_ddraw);
    s_directXReady = false;
}

// Releases the graphics and brings DirectDraw, Direct3D, input and sound
// (back) up; FALSE when DirectDraw or Direct3D fails.
RVA(0x0004a4f0, 0x3d)
b32 InitDirectX(void) {
    ReleaseGraphics();
    if (!InitDirectDraw()) {
        return false;
    }
    if (!InitDirect3D()) {
        return false;
    }
    ClearScreenSurfaces();
    InitDirectInput();
    InitJoystick();
    g_soundEnabled = InitDirectSound();
    s_directXReady = true;
    return true;
}

// Set while the door ahead opens (AnimateDoor runs instead of the step).
// @identity-TODO: set by 0x44e3d0 with s_doorFrame; read from its uses.
DATA(0x00090ad0)
static b32 s_doorOpening;

DATA(0x00090ad4)
static i32 s_doorFrame;

// The camera's slide per frame of a step, and the cell size it slides across.
#define STEP_SLIDE DATA_COMPGEN(0x00064a38, 40.0f)
#define CELL_UNITS 320.0f

// Slides the camera one frame of a forward step; past a whole cell the party
// moves into it and the camera jumps back. Returns whether the step ended.
RVA(0x0004a530, 0x104)
static b32 SlideForward(D3DVALUE* progress) {
    D3DVALUE step = STEP_SLIDE;
    b32 done = false;

    *progress += STEP_SLIDE;
    if (*progress > CELL_UNITS) {
        step = DATA_COMPGEN(0x00064a44, -320.0f);
        done = true;
        CommitPartyStep();
        BuildRoomGeometry();
    }
    switch (g_viewDirection) {
        case VIEW_NORTH:
            g_cameraFrom.z += step;
            g_cameraAt.z += step;
            break;
        case VIEW_EAST:
            g_cameraFrom.x += step;
            g_cameraAt.x += step;
            break;
        case VIEW_SOUTH:
            g_cameraFrom.z -= step;
            g_cameraAt.z -= step;
            break;
        case VIEW_WEST:
            g_cameraFrom.x -= step;
            g_cameraAt.x -= step;
            break;
    }
    return done;
}

// SlideForward for a step back.
RVA(0x0004a640, 0x104)
static b32 SlideBack(D3DVALUE* progress) {
    D3DVALUE step = STEP_SLIDE;
    b32 done = false;

    *progress += STEP_SLIDE;
    if (*progress > CELL_UNITS) {
        step = -CELL_UNITS;
        done = true;
        CommitPartyStep();
        BuildRoomGeometry();
    }
    switch (g_viewDirection) {
        case VIEW_NORTH:
            g_cameraFrom.z -= step;
            g_cameraAt.z -= step;
            break;
        case VIEW_EAST:
            g_cameraFrom.x -= step;
            g_cameraAt.x -= step;
            break;
        case VIEW_SOUTH:
            g_cameraFrom.z += step;
            g_cameraAt.z += step;
            break;
        case VIEW_WEST:
            g_cameraFrom.x += step;
            g_cameraAt.x += step;
            break;
    }
    return done;
}

// SlideForward for a step to the left.
RVA(0x0004a750, 0x104)
static b32 SlideLeft(D3DVALUE* progress) {
    D3DVALUE step = STEP_SLIDE;
    b32 done = false;

    *progress += STEP_SLIDE;
    if (*progress > CELL_UNITS) {
        step = -CELL_UNITS;
        done = true;
        CommitPartyStep();
        BuildRoomGeometry();
    }
    switch (g_viewDirection) {
        case VIEW_NORTH:
            g_cameraFrom.x -= step;
            g_cameraAt.x -= step;
            break;
        case VIEW_EAST:
            g_cameraFrom.z += step;
            g_cameraAt.z += step;
            break;
        case VIEW_SOUTH:
            g_cameraFrom.x += step;
            g_cameraAt.x += step;
            break;
        case VIEW_WEST:
            g_cameraFrom.z -= step;
            g_cameraAt.z -= step;
            break;
    }
    return done;
}

// SlideForward for a step to the right.
RVA(0x0004a860, 0x104)
static b32 SlideRight(D3DVALUE* progress) {
    D3DVALUE step = STEP_SLIDE;
    b32 done = false;

    *progress += STEP_SLIDE;
    if (*progress > CELL_UNITS) {
        step = -CELL_UNITS;
        done = true;
        CommitPartyStep();
        BuildRoomGeometry();
    }
    switch (g_viewDirection) {
        case VIEW_NORTH:
            g_cameraFrom.x += step;
            g_cameraAt.x += step;
            break;
        case VIEW_EAST:
            g_cameraFrom.z -= step;
            g_cameraAt.z -= step;
            break;
        case VIEW_SOUTH:
            g_cameraFrom.x -= step;
            g_cameraAt.x -= step;
            break;
        case VIEW_WEST:
            g_cameraFrom.z += step;
            g_cameraAt.z += step;
            break;
    }
    return done;
}

// sin(3 * i degrees) for the turn steps 0..30 (the last entry is unused).
DATA(0x0006b7e8)
static D3DVALUE s_turnSine[32] = {
    0.0f,      0.052336f, 0.104528f, 0.156434f, 0.207912f, 0.258819f, 0.309017f, 0.358368f,
    0.406737f, 0.45399f,  0.5f,      0.544639f, 0.587785f, 0.62932f,  0.669131f, 0.707107f,
    0.743145f, 0.777146f, 0.809017f, 0.838671f, 0.866025f, 0.891007f, 0.913545f, 0.93358f,
    0.951057f, 0.965926f, 0.978148f, 0.987688f, 0.994522f, 0.99863f,  1.0f,      0.0f,
};

// The compass image for each facing.
DATA(0x0006b598)
u16 g_compassImages[4] = {IDB_BITMAP5, IDB_BITMAP7, IDB_BITMAP6, IDB_BITMAP8};

// A quarter turn is 30 steps of 3 degrees; the camera circles its target at
// this distance.
#define TURN_STEPS 30
#define TURN_SPEED 3.0f
#define TURN_END 30.0f
#define CAMERA_DISTANCE 160.0f

// Set between the two quarter turns of a turn around.
DATA(0x00090acc)
static b32 s_halfTurned;

// Turns the camera one frame of a quarter turn left; at its end the party
// faces the new way. Returns whether the turn ended.
RVA(0x0004a970, 0x130)
static b32 TurnLeftStep(D3DVALUE* progress) {
    D3DVALUE dx;
    D3DVALUE dz;
    i32 step;

    *progress += TURN_SPEED;
    step = static_cast<i32>(*progress);
    switch (g_viewDirection) {
        case VIEW_NORTH:
            dx = s_turnSine[step];
            dz = -s_turnSine[TURN_STEPS - step];
            break;
        case VIEW_EAST:
            dx = -s_turnSine[TURN_STEPS - step];
            dz = -s_turnSine[step];
            break;
        case VIEW_SOUTH:
            dx = -s_turnSine[step];
            dz = s_turnSine[TURN_STEPS - step];
            break;
        case VIEW_WEST:
            dx = s_turnSine[TURN_STEPS - step];
            dz = s_turnSine[step];
            break;
    }
    g_billboardX = dz;
    g_billboardZ = -dx;
    g_turnStep = step;
    g_cameraFrom.x = dx * CAMERA_DISTANCE;
    g_cameraFrom.z = dz * CAMERA_DISTANCE;
    if (*progress == TURN_END) {
        g_viewDirection =
            static_cast<GZ_ENUM_STORAGE(ViewDirection, i32)>((g_viewDirection - 1) & 3);
        SetPartyDirection(g_viewDirection);
        BlitImage(
            g_screenLayers[SCREEN_LAYER_NAVIGATION]->surface,
            g_compassImages[g_viewDirection],
            32,
            32
        );
        return true;
    }
    return false;
}

// TurnLeftStep for a quarter turn right.
RVA(0x0004aaa0, 0x130)
static b32 TurnRightStep(D3DVALUE* progress) {
    D3DVALUE dx;
    D3DVALUE dz;
    i32 step;

    *progress += TURN_SPEED;
    step = static_cast<i32>(*progress);
    switch (g_viewDirection) {
        case VIEW_NORTH:
            dx = -s_turnSine[step];
            dz = -s_turnSine[TURN_STEPS - step];
            break;
        case VIEW_EAST:
            dx = -s_turnSine[TURN_STEPS - step];
            dz = s_turnSine[step];
            break;
        case VIEW_SOUTH:
            dx = s_turnSine[step];
            dz = s_turnSine[TURN_STEPS - step];
            break;
        case VIEW_WEST:
            dx = s_turnSine[TURN_STEPS - step];
            dz = -s_turnSine[step];
            break;
    }
    g_billboardX = dz;
    g_billboardZ = -dx;
    g_turnStep = step;
    g_cameraFrom.x = dx * CAMERA_DISTANCE;
    g_cameraFrom.z = dz * CAMERA_DISTANCE;
    if (*progress == TURN_END) {
        g_viewDirection =
            static_cast<GZ_ENUM_STORAGE(ViewDirection, i32)>((g_viewDirection + 1) & 3);
        SetPartyDirection(g_viewDirection);
        BlitImage(
            g_screenLayers[SCREEN_LAYER_NAVIGATION]->surface,
            g_compassImages[g_viewDirection],
            32,
            32
        );
        return true;
    }
    return false;
}

// Two quarter turns right; the party's facing is set once, at the end.
RVA(0x0004abd0, 0x154)
static b32 TurnAroundStep(D3DVALUE* progress) {
    D3DVALUE dx;
    D3DVALUE dz;
    i32 step;

    *progress += TURN_SPEED;
    step = static_cast<i32>(*progress);
    switch (g_viewDirection) {
        case VIEW_NORTH:
            dx = -s_turnSine[step];
            dz = -s_turnSine[TURN_STEPS - step];
            break;
        case VIEW_EAST:
            dx = -s_turnSine[TURN_STEPS - step];
            dz = s_turnSine[step];
            break;
        case VIEW_SOUTH:
            dx = s_turnSine[step];
            dz = s_turnSine[TURN_STEPS - step];
            break;
        case VIEW_WEST:
            dx = s_turnSine[TURN_STEPS - step];
            dz = -s_turnSine[step];
            break;
    }
    g_billboardX = dz;
    g_billboardZ = -dx;
    g_turnStep = step;
    g_cameraFrom.x = dx * CAMERA_DISTANCE;
    g_cameraFrom.z = dz * CAMERA_DISTANCE;
    if (*progress == TURN_END) {
        g_viewDirection =
            static_cast<GZ_ENUM_STORAGE(ViewDirection, i32)>((g_viewDirection + 1) & 3);
        if (s_halfTurned) {
            s_halfTurned = false;
            BlitImage(
                g_screenLayers[SCREEN_LAYER_NAVIGATION]->surface,
                g_compassImages[g_viewDirection],
                32,
                32
            );
            SetPartyDirection(g_viewDirection);
            return true;
        }
        *progress = 0.0f;
        s_halfTurned = true;
    }
    return false;
}

// The step of move state 0 and 8..15: nothing moves.
RVA(0x0004ad30, 0x6)
static b32 NoMoveStep(D3DVALUE* progress) {
    return true;
}

RVA(0x0004ad40, 0x1d)
void PressPadButton(GZ_ENUM_PARAM(NavPadButton, i32) button, BOOL pressed) {
    DrawPadButton(g_screenLayers[SCREEN_LAYER_NAVIGATION]->surface, button, pressed);
}

// The step functions by move kind (g_moveState's low nibble).
DATA(0x0006b9f8)
static BOOL (*s_moveSteps[16])(D3DVALUE* progress) = {
    NoMoveStep,
    SlideForward,
    SlideBack,
    SlideLeft,
    SlideRight,
    TurnLeftStep,
    TurnRightStep,
    TurnAroundStep,
    NoMoveStep,
    NoMoveStep,
    NoMoveStep,
    NoMoveStep,
    NoMoveStep,
    NoMoveStep,
    NoMoveStep,
    NoMoveStep,
};

// The pad button each move kind presses.
DATA(0x0006ba38)
static GZ_ENUM_STORAGE(NavPadButton, i32) s_movePadButtons[8] = {
    PAD_NONE,
    PAD_FORWARD,
    PAD_BACK,
    PAD_LEFT,
    PAD_RIGHT,
    PAD_LEFT,
    PAD_RIGHT,
    PAD_BACK,
};

// Runs one frame of the move under way (a new move starts from progress 0),
// then re-aims the camera; at the move's end the pad button comes up and the
// view is redrawn at the party's new cell. Returns whether a move ran.
RVA(0x0004ad60, 0xcb)
b32 AnimateMove(void) {
    b32 moving = false;

    if (g_moveState & MOVE_STATE_KIND) {
        if (s_lastMoveState != g_moveState) {
            s_moveProgress = 0.0f;
        }
        if (s_moveSteps[g_moveState & MOVE_STATE_KIND](&s_moveProgress)) {
            PressPadButton(s_movePadButtons[g_moveState & 7], false);
            DrawFieldView();
            g_moveState = MOVE_STATE_NONE;
            s_doorOpening = false;
            g_selectedHotspot = HOTSPOT_NONE;
            MapPosition* position = GetMapPosition();
            // the pun: this layer's own prototype of RedrawFieldAt takes int
            // arguments (0x44adda sign-extends each; the C callers push words).
            reinterpret_cast<void (*)(i32, i32, i32)>(RedrawFieldAt)(
                position->x,
                position->y,
                position->direction
            );
        }
        SetViewMatrix(g_viewMatrix, g_cameraFrom, g_cameraAt);
        g_d3dDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &g_viewMatrix);
        moving = true;
    }
    s_lastMoveState = g_moveState;
    return moving;
}

// The door frame's corner offsets per facing (x, z pairs for its four corners)
// and, per animation step, the half-width of the opening and the texture u of
// the leaves' inner edges.
DATA(0x0006ba58)
static i32 s_doorCorners[4][4][2] = {
    {{-1, 1}, {1, 1}, {1, 1}, {-1, 1}},
    {{1, 1}, {1, -1}, {1, -1}, {1, 1}},
    {{1, -1}, {-1, -1}, {-1, -1}, {1, -1}},
    {{-1, -1}, {-1, 1}, {-1, 1}, {-1, -1}},
};

DATA(0x0006bad8)
static i32 s_doorWidths[16] = {
    120,
    113,
    105,
    98,
    90,
    83,
    75,
    68,
    60,
    53,
    45,
    38,
    30,
    23,
    15,
    8,
};

DATA(0x0006bb18)
static D3DVALUE s_doorEdgeU[16] = {
    0.2f,
    0.1875f,
    0.175f,
    0.1625f,
    0.15f,
    0.1375f,
    0.125f,
    0.1125f,
    0.1f,
    0.0875f,
    0.075f,
    0.0625f,
    0.05f,
    0.0375f,
    0.025f,
    0.0125f,
};

// The depth past which vertices darken, and the shading curve's constants.
#define SHADE_NEAR 1120.0

// Shades a mesh's vertices by their depth in the view: full brightness up to
// SHADE_NEAR, then darker with distance (every vertex full on the ramp device
// or with fixed lighting).
RVA(0x0004ae30, 0x1dc)
void ShadeMesh(Mesh* mesh) {
    D3DVALUE w;
    D3DVECTOR view;
    u32 i;
    D3DVALUE shade;

    if (g_deviceType == D3D_DEVICE_RAMP) {
        for (i = 0; i < mesh->vertexCount; i++) {
            mesh->vertices[i].color = 0xffffffff;
        }
        return;
    }
    if (g_fixedLighting) {
        for (i = 0; i < mesh->vertexCount; i++) {
            mesh->vertices[i].color = 0xffffffff;
        }
        return;
    }
    for (i = 0; i < mesh->vertexCount; i++) {
        w = g_viewMatrix(0, 3) * mesh->vertices[i].sx + g_viewMatrix(1, 3) * mesh->vertices[i].sy
            + g_viewMatrix(2, 3) * mesh->vertices[i].sz + g_viewMatrix(3, 3);
        view.z =
            (g_viewMatrix(0, 2) * mesh->vertices[i].sx + g_viewMatrix(1, 2) * mesh->vertices[i].sy
             + g_viewMatrix(2, 2) * mesh->vertices[i].sz + g_viewMatrix(3, 2))
            / w;
        if (view.z <= SHADE_NEAR) {
            shade = 1.0f;
        } else {
            shade = (320.0f - view.z * 0.125f) / (view.z - 640.0);
        }
        mesh->vertices[i].color = D3DRGB(shade, shade, shade);
    }
}

// The door being opened in front of the party: its frame is laid out on the
// first call and its leaves slide apart over 16 frames; returns the step state
// until the last frame, which returns the step kind (1..4) and finishes the
// step.
// @identity-TODO: the mesh's vertex roles (0..23 the frame, 24..47 the leaves
// mirrored every frame) are read from the offsets only.
RVA(0x0004b010, 0xa52)
i32 AnimateDoor(Mesh* mesh) {
    i32 result;
    i32 direction;
    i32 i;
    D3DVALUE swap;

    result = g_moveState;
    switch (g_moveState) {
        case MOVE_STATE_DOOR_AHEAD:
        case MOVE_STATE_DOOR_BACK:
            direction = g_viewDirection;
            break;
        case MOVE_STATE_DOOR_LEFT:
            direction = (g_viewDirection - 1) & 3;
            break;
        default:
            direction = (g_viewDirection + 1) & 3;
            break;
    }
    if (s_doorFrame == 0) {
        s_doorFrame = 16;
        for (i = 0; i < 4; i++) {
            mesh->vertices[i].sx = s_doorCorners[direction][i][0] * 160;
            mesh->vertices[i].sz = s_doorCorners[direction][i][1] * 160;
            mesh->vertices[i + 20].sx = s_doorCorners[direction][i][0] * 158;
            mesh->vertices[i + 20].sz = s_doorCorners[direction][i][1] * 158;
            mesh->vertices[i + 24].sx = s_doorCorners[direction][i][0] * 160;
            mesh->vertices[i + 24].sz = s_doorCorners[direction][i][1] * 160;
            mesh->vertices[i + 44].sx = s_doorCorners[direction][i][0] * 162;
            mesh->vertices[i + 44].sz = s_doorCorners[direction][i][1] * 162;
        }
        mesh->vertices[4].sx = mesh->vertices[7].sx = mesh->vertices[12].sx =
            mesh->vertices[15].sx = mesh->vertices[0].sx - s_doorCorners[direction][0][0];
        mesh->vertices[4].sz = mesh->vertices[7].sz = mesh->vertices[12].sz =
            mesh->vertices[15].sz = mesh->vertices[0].sz - s_doorCorners[direction][0][1];
        mesh->vertices[9].sx = mesh->vertices[10].sx = mesh->vertices[17].sx =
            mesh->vertices[18].sx = mesh->vertices[1].sx - s_doorCorners[direction][1][0];
        mesh->vertices[9].sz = mesh->vertices[10].sz = mesh->vertices[17].sz =
            mesh->vertices[18].sz = mesh->vertices[1].sz - s_doorCorners[direction][1][1];
        mesh->vertices[28].sx = mesh->vertices[31].sx = mesh->vertices[36].sx =
            mesh->vertices[39].sx = mesh->vertices[24].sx + s_doorCorners[direction][0][0];
        mesh->vertices[28].sz = mesh->vertices[31].sz = mesh->vertices[36].sz =
            mesh->vertices[39].sz = mesh->vertices[24].sz + s_doorCorners[direction][0][1];
        mesh->vertices[33].sx = mesh->vertices[34].sx = mesh->vertices[41].sx =
            mesh->vertices[42].sx = mesh->vertices[25].sx + s_doorCorners[direction][1][0];
        mesh->vertices[33].sz = mesh->vertices[34].sz = mesh->vertices[41].sz =
            mesh->vertices[42].sz = mesh->vertices[25].sz + s_doorCorners[direction][1][1];
        if (direction & 1) {
            mesh->vertices[11].sz = 0.0f;
            mesh->vertices[8].sz = 0.0f;
            mesh->vertices[6].sz = 0.0f;
            mesh->vertices[5].sz = 0.0f;
            mesh->vertices[35].sz = 0.0f;
            mesh->vertices[32].sz = 0.0f;
            mesh->vertices[30].sz = 0.0f;
            mesh->vertices[29].sz = 0.0f;
            mesh->vertices[13].sz = mesh->vertices[14].sz =
                s_doorWidths[0] * s_doorCorners[direction][0][1];
            mesh->vertices[16].sz = mesh->vertices[19].sz =
                s_doorWidths[0] * s_doorCorners[direction][1][1];
            mesh->vertices[37].sz = mesh->vertices[38].sz =
                s_doorWidths[0] * s_doorCorners[direction][0][1];
            mesh->vertices[40].sz = mesh->vertices[43].sz =
                s_doorWidths[0] * s_doorCorners[direction][1][1];
            mesh->vertices[5].sx = mesh->vertices[6].sx = mesh->vertices[8].sx =
                mesh->vertices[11].sx = mesh->vertices[13].sx = mesh->vertices[14].sx =
                    mesh->vertices[16].sx = mesh->vertices[19].sx =
                        s_doorCorners[direction][0][0] * 158;
            mesh->vertices[29].sx = mesh->vertices[30].sx = mesh->vertices[32].sx =
                mesh->vertices[35].sx = mesh->vertices[37].sx = mesh->vertices[38].sx =
                    mesh->vertices[40].sx = mesh->vertices[43].sx =
                        s_doorCorners[direction][0][0] * 162;
        } else {
            mesh->vertices[11].sx = 0.0f;
            mesh->vertices[8].sx = 0.0f;
            mesh->vertices[6].sx = 0.0f;
            mesh->vertices[5].sx = 0.0f;
            mesh->vertices[35].sx = 0.0f;
            mesh->vertices[32].sx = 0.0f;
            mesh->vertices[30].sx = 0.0f;
            mesh->vertices[29].sx = 0.0f;
            mesh->vertices[13].sx = mesh->vertices[14].sx =
                s_doorWidths[0] * s_doorCorners[direction][0][0];
            mesh->vertices[16].sx = mesh->vertices[19].sx =
                s_doorWidths[0] * s_doorCorners[direction][1][0];
            mesh->vertices[37].sx = mesh->vertices[38].sx =
                s_doorWidths[0] * s_doorCorners[direction][0][0];
            mesh->vertices[40].sx = mesh->vertices[43].sx =
                s_doorWidths[0] * s_doorCorners[direction][1][0];
            mesh->vertices[5].sz = mesh->vertices[6].sz = mesh->vertices[8].sz =
                mesh->vertices[11].sz = mesh->vertices[13].sz = mesh->vertices[14].sz =
                    mesh->vertices[16].sz = mesh->vertices[19].sz =
                        s_doorCorners[direction][0][1] * 158;
            mesh->vertices[29].sz = mesh->vertices[30].sz = mesh->vertices[32].sz =
                mesh->vertices[35].sz = mesh->vertices[37].sz = mesh->vertices[38].sz =
                    mesh->vertices[40].sz = mesh->vertices[43].sz =
                        s_doorCorners[direction][0][1] * 162;
        }
        mesh->vertices[4].tu = mesh->vertices[7].tu = 0.01f;
        mesh->vertices[10].tu = 0.49f;
        mesh->vertices[9].tu = 0.49f;
        mesh->vertices[31].tu = 0.01f;
        mesh->vertices[28].tu = 0.01f;
        mesh->vertices[34].tu = 0.49f;
        mesh->vertices[33].tu = 0.49f;
    } else {
        s_doorFrame--;
        mesh->vertices[4].tu = mesh->vertices[7].tu = s_doorEdgeU[s_doorFrame];
        mesh->vertices[9].tu = mesh->vertices[10].tu = 0.49f - s_doorEdgeU[s_doorFrame];
        mesh->vertices[28].tu = mesh->vertices[31].tu = s_doorEdgeU[s_doorFrame];
        mesh->vertices[33].tu = mesh->vertices[34].tu = 0.49f - s_doorEdgeU[s_doorFrame];
        if (direction & 1) {
            mesh->vertices[5].sz = mesh->vertices[6].sz =
                (s_doorWidths[s_doorFrame] - 2) * s_doorCorners[direction][0][1];
            mesh->vertices[8].sz = mesh->vertices[11].sz =
                (s_doorWidths[s_doorFrame] - 2) * s_doorCorners[direction][1][1];
            mesh->vertices[29].sz = mesh->vertices[30].sz =
                (s_doorWidths[s_doorFrame] + 2) * s_doorCorners[direction][0][1];
            mesh->vertices[32].sz = mesh->vertices[35].sz =
                (s_doorWidths[s_doorFrame] + 2) * s_doorCorners[direction][1][1];
        } else {
            mesh->vertices[5].sx = mesh->vertices[6].sx =
                (s_doorWidths[s_doorFrame] - 2) * s_doorCorners[direction][0][0];
            mesh->vertices[8].sx = mesh->vertices[11].sx =
                (s_doorWidths[s_doorFrame] - 2) * s_doorCorners[direction][1][0];
            mesh->vertices[29].sx = mesh->vertices[30].sx =
                (s_doorWidths[s_doorFrame] + 2) * s_doorCorners[direction][0][0];
            mesh->vertices[32].sx = mesh->vertices[35].sx =
                (s_doorWidths[s_doorFrame] + 2) * s_doorCorners[direction][1][0];
        }
        if (s_doorFrame == 0) {
            switch (g_moveState) {
                case MOVE_STATE_DOOR_AHEAD:
                    result = 1;
                    break;
                case MOVE_STATE_DOOR_BACK:
                    result = 2;
                    break;
                default:
                    result = (g_moveState != MOVE_STATE_DOOR_LEFT) + 3;
                    break;
            }
            FinishDoorStep();
        }
    }
    for (i = 24; i < 48; i += 2) {
        swap = mesh->vertices[i].sx;
        mesh->vertices[i].sx = mesh->vertices[i + 1].sx;
        mesh->vertices[i + 1].sx = swap;
        swap = mesh->vertices[i].sz;
        mesh->vertices[i].sz = mesh->vertices[i + 1].sz;
        mesh->vertices[i + 1].sz = swap;
    }
    g_d3dDevice->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        D3DVT_LVERTEX,
        g_doorMesh.vertices,
        g_doorMesh.vertexCount,
        g_doorMesh.indices,
        g_doorMesh.indexCount,
        D3DDP_DONOTUPDATEEXTENTS
    );
    return result;
}

// Outlines the dragged rectangle.
RVA(0x0004ba70, 0x195)
void DrawDragOutline(void) {
    DATA(0x0008fa58)
    static D3DTLVERTEX s_outline[5] = {
        D3DTLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 1.0f, 0xffffffff, 0, 0.0f, 0.0f),
        D3DTLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 1.0f, 0xffffffff, 0, 0.0f, 0.0f),
        D3DTLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 1.0f, 0xffffffff, 0, 0.0f, 0.0f),
        D3DTLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 1.0f, 0xffffffff, 0, 0.0f, 0.0f),
        D3DTLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 1.0f, 0xffffffff, 0, 0.0f, 0.0f),
    };

    s_outline[0].sx = s_outline[3].sx = s_outline[4].sx = D3DVAL(g_dragRect.left);
    s_outline[0].sy = s_outline[1].sy = s_outline[4].sy = D3DVAL(g_dragRect.top);
    s_outline[1].sx = s_outline[2].sx = D3DVAL(g_dragRect.right);
    s_outline[2].sy = s_outline[3].sy = D3DVAL(g_dragRect.bottom);
    g_screenDevice->BeginScene();
    g_screenDevice
        ->DrawPrimitive(D3DPT_LINESTRIP, D3DVT_TLVERTEX, s_outline, 5, D3DDP_DONOTUPDATEEXTENTS);
    g_screenDevice->EndScene();
}

// The atexit callback of DrawDragOutline's vertex table (nothing to destroy).
RVA_DYNINIT(0x0004bc10, 0x1, DrawDragOutline)

// Fills the screen with the fade colour.
RVA(0x0004bc20, 0x19a)
void DrawScreenFade(void) {
    DATA(0x0008f368)
    static D3DTLVERTEX s_quad[4] = {
        D3DTLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 1.0f, 0, 0, 0.0f, 0.0f),
        D3DTLVERTEX(D3DVECTOR(640.0f, 0.0f, 0.0f), 1.0f, 0, 0, 0.0f, 0.0f),
        D3DTLVERTEX(D3DVECTOR(640.0f, 480.0f, 0.0f), 1.0f, 0, 0, 0.0f, 0.0f),
        D3DTLVERTEX(D3DVECTOR(0.0f, 480.0f, 0.0f), 1.0f, 0, 0, 0.0f, 0.0f),
    };
    int i;

    for (i = 0; i < 4; i++) {
        s_quad[i].color = g_fadeColor;
    }
    if (g_screenDevice == NULL) {
        g_d3dDevice->BeginScene();
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, false);
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, false);
        DrawScreenQuad(g_d3dDevice, s_quad);
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, true);
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, true);
        g_d3dDevice->EndScene();
    } else {
        g_screenDevice->BeginScene();
        DrawScreenQuad(g_screenDevice, s_quad);
        g_screenDevice->EndScene();
    }
}

// The atexit callback of DrawScreenFade's vertex table (nothing to destroy).
RVA_DYNINIT(0x0004bdc0, 0x1, DrawScreenFade)

// Texture coordinates for the normal and lower-half treasure-box frames,
// closed then open, clockwise from the top left.
DATA(0x0006bb58)
static D3DVALUE s_boxUV[4][4][2] = {
    {{0.01f, 0.01f}, {0.49f, 0.01f}, {0.49f, 0.49f}, {0.01f, 0.49f}},
    {{0.51f, 0.01f}, {0.99f, 0.01f}, {0.99f, 0.49f}, {0.51f, 0.49f}},
    {{0.01f, 0.51f}, {0.49f, 0.51f}, {0.49f, 0.99f}, {0.01f, 0.99f}},
    {{0.51f, 0.51f}, {0.99f, 0.51f}, {0.99f, 0.99f}, {0.51f, 0.99f}},
};

// Draws the treasure boxes within three cells of the party as billboards and
// makes the box in front of the party a hotspot.
// @early-stop x87 schedule: the billboard corner products and copies differ;
// retail rounds the -x corner through a temporary. Calls and CFG match.
RVA(0x0004bdd0, 0x5dc)
void RenderTBox(void) {
    DATA(0x0008fd00)
    static D3DLVERTEX s_box[4] = {
        D3DLVERTEX(D3DVECTOR(0.0f, 80.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 0.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 80.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 0.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 0.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 0.0f),
    };
    TreasureBox* box;
    MapPosition* position;
    i16 partyX;
    i16 partyY;
    BOOL textureSet;
    BOOL facing;
    i16 cellX;
    i16 cellY;
    int frame;
    i16 dx;
    i16 dz;
    int i;
    double distance;
    D3DVALUE light;
    HRESULT result;

    box = GetMapTreasureBoxes();
    if (box == NULL) {
        return;
    }
    position = GetMapPosition();
    partyX = position->x;
    partyY = position->y;
    textureSet = false;
    for (; box->head.x != TREASURE_BOX_END; box++) {
        facing = false;
        cellX = box->head.x;
        cellY = box->head.y;
        if (cellX < partyX - 3 || cellX > partyX + 3 || cellY < partyY - 3 || cellY > partyY + 3) {
            continue;
        }
        switch (g_viewDirection) {
            case VIEW_NORTH:
                if (cellX == partyX && cellY == partyY - 1) {
                    facing = true;
                }
                break;
            case VIEW_EAST:
                if (cellY == partyY && cellX == partyX + 1) {
                    facing = true;
                }
                break;
            case VIEW_SOUTH:
                if (cellX == partyX && cellY == partyY + 1) {
                    facing = true;
                }
                break;
            case VIEW_WEST:
                if (cellY == partyY && cellX == partyX - 1) {
                    facing = true;
                }
                break;
        }
        if (g_bilinearFiltering) {
            SetTextureFiltering(D3DFILTER_NEAREST);
        }
        if (!textureSet) {
            g_d3dDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                GetTextureHandle(&g_textBoxTexture)
            );
            textureSet = true;
        }
        frame = IsEventFlagSet(box->flagBank, box->flagIndex) != false;
        if (!frame && g_boxOpening && g_openingBox == box) {
            frame = 1;
        }
        if (box->head.code == CELL_TREASURE_BOX_LOWER_TEXTURE_HALF) {
            frame = (frame - 2) & 3;
        }
        dx = (cellX - partyX) * 320;
        dz = (partyY - cellY) * 320;
        switch (g_viewDirection) {
            case VIEW_NORTH:
                dz -= 40;
                break;
            case VIEW_EAST:
                dx -= 40;
                break;
            case VIEW_SOUTH:
                dz += 40;
                break;
            case VIEW_WEST:
                dx += 40;
                break;
        }
        s_box[0].x = g_billboardX * DATA_COMPGEN(0x00064a78, 40.0);
        s_box[1].x = -g_billboardX * 40.0;
        s_box[0].z = g_billboardZ * 40.0;
        s_box[1].z = -g_billboardZ * 40.0;
        s_box[2].x = s_box[1].x;
        s_box[2].z = s_box[1].z;
        s_box[3].x = s_box[0].x;
        s_box[3].z = s_box[0].z;
        for (i = 0; i < 4; i++) {
            s_box[i].x += dx;
            s_box[i].z += dz;
            s_box[i].tu = s_boxUV[frame][i][0];
            s_box[i].tv = s_boxUV[frame][i][1];
        }
        if (g_deviceType != D3D_DEVICE_RAMP) {
            if (!g_fixedLighting) {
                MakeBillboardCameraRelative(dx, dz);
                distance = GetBillboardDistance(dx, dz);
                SetDistanceLight(light, distance);
                SetQuadColor(s_box, D3DRGB(light, light, light));
            }
        }
        if (g_deviceType == D3D_DEVICE_RAMP) {
            result = g_d3dDevice->SetLightState(
                D3DLIGHTSTATE_MATERIAL,
                GetTextureMaterialHandle(&g_textBoxTexture, TEXTURE_SHADE_NORMAL)
            );
            if (result != D3D_OK) {
                TraceD3DCallError("lpD3DDev->SetLightState()@RenderTBox() returns ", result);
            }
        }
        DrawLitQuad(s_box);
        if (g_bilinearFiltering) {
            SetTextureFiltering(D3DFILTER_LINEAR);
        }
        if (IsCellInViewCone(partyX, partyY, box->head.x, box->head.y) && facing) {
            if (g_hotspotCount >= 64) {
                return;
            }
            Hotspot* hotspot = GetHotspot(g_hotspotCount);
            D3DVECTOR corner;
            D3DVECTOR screen;
            ProjectBillboardRect(hotspot->rect, s_box, corner, screen);
            hotspot->kind = HOTSPOT_BOX;
            hotspot->texture = &g_textBoxTexture;
            hotspot->data = box;
            g_hotspotCount++;
        }
    }
    g_boxOpening = false;
}

// The atexit callback of RenderTBox's vertex table (nothing to destroy).
RVA_DYNINIT(0x0004c3b0, 0x1, RenderTBox)

// Draws the NPCs within three cells of the party as billboards and makes the
// ones in view hotspots (kind 15 for the one in front of the party, 14 for one
// on the party's cell); with `ownCellOnly` only an NPC on the party's cell is
// drawn. The two nearest are ordered, then every NPC hotspot becomes kind 2.
// @early-stop x87 schedule: the billboard corner products and their stores
// through float temporaries are scheduled differently; calls, texture
// selection, CFG and the integer code match.
RVA(0x0004c3c0, 0x754)
void RenderNPC(BOOL ownCellOnly) {
    DATA(0x0008f210)
    static D3DLVERTEX s_npc[4] = {
        D3DLVERTEX(D3DVECTOR(0.0f, 256.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 0.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 256.0f, 0.0f), 0xffffffff, 0xff000000, 1.0f, 0.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 0xffffffff, 0xff000000, 1.0f, 1.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 1.0f),
    };
    MapPosition* position;
    i16 partyX;
    i16 partyY;
    i16 count;
    i16 npc;
    GZ_ENUM_LOCAL(UiHotspotKind, u32) kind;
    i16* cell;
    i16 cellX;
    i16 cellY;
    i32 offsetX;
    i32 offsetZ;
    i16 dx;
    i16 dz;
    double distance;
    D3DVALUE light;
    i16 textureSlot;
    Texture* texture;
    D3DVALUE bottomMargin;
    HRESULT result;
    u16 i;

    position = GetMapPosition();
    partyX = position->x;
    partyY = position->y;
    count = GetAreaNpcCount();
    for (npc = 0; npc < count; npc++) {
        kind = HOTSPOT_NPC;
        if (IsAreaNpcGone(npc)) {
            continue;
        }
        cell = GetAreaNpcCell(npc);
        cellX = cell[0];
        cellY = cell[1];
        if (cellX < partyX - 3 || cellX > partyX + 3 || cellY < partyY - 3 || cellY > partyY + 3) {
            continue;
        }
        switch (g_viewDirection) {
            case VIEW_NORTH:
                if (cellX == partyX && cellY == partyY - 1) {
                    kind = HOTSPOT_NPC_AHEAD;
                }
                break;
            case VIEW_EAST:
                if (cellY == partyY && cellX == partyX + 1) {
                    kind = HOTSPOT_NPC_AHEAD;
                }
                break;
            case VIEW_SOUTH:
                if (cellX == partyX && cellY == partyY + 1) {
                    kind = HOTSPOT_NPC_AHEAD;
                }
                break;
            case VIEW_WEST:
                if (cellY == partyY && cellX == partyX - 1) {
                    kind = HOTSPOT_NPC_AHEAD;
                }
                break;
        }
        offsetX = 0;
        offsetZ = 0;
        if (cellX == partyX && cellY == partyY) {
            switch (g_viewDirection) {
                case VIEW_NORTH:
                    offsetZ = 1;
                    break;
                case VIEW_EAST:
                    offsetX = 1;
                    break;
                case VIEW_SOUTH:
                    offsetZ = -1;
                    break;
                case VIEW_WEST:
                    offsetX = -1;
                    break;
            }
            kind = HOTSPOT_NPC_HERE;
        } else if (ownCellOnly) {
            continue;
        }
        if (g_moveState == MOVE_STATE_STEP && kind == HOTSPOT_NPC_HERE) {
            continue;
        }
        dx = (cellX - partyX) * 320 + offsetX;
        dz = (partyY - cellY) * 320 + offsetZ;
        s_npc[1].x = -g_billboardX * DATA_COMPGEN(0x00064a98, 128.0);
        s_npc[2].x = -g_billboardX * 128.0;
        s_npc[1].z = -g_billboardZ * 128.0;
        s_npc[2].z = -g_billboardZ * 128.0;
        s_npc[0].x = g_billboardX * 128.0;
        s_npc[3].x = g_billboardX * 128.0;
        s_npc[0].z = g_billboardZ * 128.0;
        s_npc[3].z = g_billboardZ * 128.0;
        TranslateBillboard(s_npc, dx, dz);
        if (g_deviceType != D3D_DEVICE_RAMP && !g_fixedLighting) {
            MakeBillboardCameraRelative(dx, dz);
            distance = GetBillboardDistance(dx, dz);
            SetDistanceLight(light, distance);
            SetQuadColor(s_npc, D3DRGB(light, light, light));
        }
        textureSlot = GetAreaNpcTextureSlot(npc);
        SelectNpcBillboardTexture(texture, textureSlot);
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, GetTextureHandle(texture));
        bottomMargin = texture->bottomMargin;
        if (g_deviceType == D3D_DEVICE_RAMP) {
            result = g_d3dDevice->SetLightState(
                D3DLIGHTSTATE_MATERIAL,
                GetTextureMaterialHandle(texture, TEXTURE_SHADE_NORMAL)
            );
            if (result != D3D_OK) {
                TraceD3DCallError("lpD3DDev->SetLightState()@RenderNPC() returns ", result);
            }
        }
        s_npc[0].y = s_npc[1].y = DATA_COMPGEN(0x00064aa0, 256.0f) - bottomMargin;
        s_npc[2].y = s_npc[3].y = -bottomMargin;
        DrawLitQuad(s_npc);
        if (IsCellInViewCone(partyX, partyY, cell[0], cell[1]) && kind != 0) {
            if (g_hotspotCount >= 64) {
                continue;
            }
            Hotspot* hotspot = GetHotspot(g_hotspotCount);
            D3DVECTOR corner;
            D3DVECTOR screen;
            ProjectBillboardRect(hotspot->rect, s_npc, corner, screen);
            hotspot->kind = kind;
            SelectNpcBillboardTexture(hotspot->texture, textureSlot);
            hotspot->data = GetAreaNpc(npc);
            cell = GetAreaNpcCell(npc);
            hotspot->targetX = cell[0];
            hotspot->targetY = cell[1];
            g_hotspotCount++;
        }
    }
    if (g_hotspotCount > 1 && GetHotspot(g_hotspotCount - 1)->kind > 3
        && GetHotspot(g_hotspotCount - 2)->kind > 3
        && GetHotspot(g_hotspotCount - 1)->kind > GetHotspot(g_hotspotCount - 2)->kind) {
        Hotspot swap = *GetHotspot(g_hotspotCount - 1);

        *GetHotspot(g_hotspotCount - 1) = *GetHotspot(g_hotspotCount - 2);
        *GetHotspot(g_hotspotCount - 2) = swap;
    }
    for (i = 0; i < g_hotspotCount; i++) {
        if (GetHotspot(i)->kind > 3) {
            GetHotspot(i)->kind = HOTSPOT_NPC;
        }
    }
}

// The atexit callback of RenderNPC's vertex table (nothing to destroy).
RVA_DYNINIT(0x0004cb20, 0x1, RenderNPC)

// The billboard size of each enemy animation frame.
DATA(0x0006bbd8)
static i32 s_enemySizes[8][2] = {
    {128, 232},
    {128, 184},
    {128, 136},
    {128, 88},
    {112, 40},
    {80, 64},
    {48, 128},
    {16, 256},
};

// Image variants as the view turns; a negative entry mirrors the image.
// Facing image code -1 occupies element zero.
DATA(0x0006bc18)
static i16 s_turnImageCodesRight[8] = {2, -1, 0, 1, -1, -1, 0, 0};
DATA(0x0006bc28)
static i16 s_turnImageCodesLeft[8] = {0, 1, 2, -1, 1, 1, 0, 0};

// Draws the field objects (enemies) within three cells of the party as
// billboards, spreading up to three sharing a cell, and makes each one in
// view a hotspot. `shade` enables distance shading, `anyCell` skips the view
// test and `byDistance` shades by distance (else fully lit).
// @identity-TODO: the flag roles are read from the one call (1, 0, 1) and the
// body; the lit-frame and 2D-fallback paths are undecoded beyond their data.
// @early-stop x87 schedule: the billboard corner products, the height and the
// translation stores are scheduled differently (retail stores the far corners
// first and copies the near z corners through integer moves). Calls, CFG,
// block placement and the integer code match.
RVA(0x0004cb30, 0xaec)
void RenderEnemy(BOOL shade, BOOL anyCell, BOOL byDistance) {
    DATA(0x0008f4d8)
    static D3DLVERTEX s_enemy[4] = {
        D3DLVERTEX(D3DVECTOR(0.0f, 256.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 0.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 256.0f, 0.0f), 0xffffffff, 0xff000000, 1.0f, 0.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 0xffffffff, 0xff000000, 1.0f, 1.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 1.0f),
    };
    MapPosition* position;
    i16 partyX;
    i16 partyY;
    i16 x;
    i16 y;
    i16 i;
    i16 found;
    i16 n;
    i16 objects[16];
    i16 visible;
    i16 index;
    i16 imageCode;
    BOOL lit;
    i32 offsetX;
    i32 offsetZ;
    i32 spread;
    D3DVALUE lift;
    i16 dx;
    i16 dz;
    i32 anim;
    i32 width;
    i32 height;
    i16 layer;
    i32 level;
    double distance;
    D3DVALUE light;
    HRESULT result;
    RECT rect;
    MapCoord* coord;
    BITMAPINFOHEADER* info;

    if (GetObjectsFrozen()) {
        return;
    }
    CheckAllObjects();
    position = GetMapPosition();
    partyX = position->x;
    partyY = position->y;
    if (g_moveState == MOVE_STATE_NONE) {
        UpdateViewCells(partyX, partyY);
    }
    for (x = partyX - 3; x <= partyX + 3; x++) {
        for (y = partyY - 3; y <= partyY + 3; y++) {
            found = 0;
            for (i = 0; i < 16; i++) {
                if (GetObjectLifeStateAt(i) == 0) {
                    continue;
                }
                if (!anyCell) {
                    visible = IsCellInViewCone(partyX, partyY, x, y);
                    if (!visible) {
                        continue;
                    }
                }
                coord = GetObjectCoordPtr(i);
                if (x == coord->x && y == coord->y) {
                    objects[found++] = i;
                }
            }
            for (n = found - 1; n >= 0; n--) {
                index = objects[n];
                imageCode = GetObjectFacingImageCode(index);
                lit = IsFieldObjectImageLit(imageCode);
                if (lit) {
                    imageCode &= FIELD_OBJECT_IMAGE_INDEX_MASK;
                }
                if (g_moveState >= MOVE_STATE_TURN_FIRST
                    && g_moveState <= MOVE_STATE_TURN_LAST && g_turnStep > 15) {
                    if (g_moveState == MOVE_STATE_TURN_LEFT) {
                        imageCode = s_turnImageCodesLeft[imageCode + 1];
                    } else {
                        imageCode = s_turnImageCodesRight[imageCode + 1];
                    }
                }
                if (imageCode < 0) {
                    s_enemy[3].tu = 1.0f;
                    s_enemy[0].tu = 1.0f;
                    s_enemy[2].tu = 0.0f;
                    s_enemy[1].tu = 0.0f;
                    imageCode = -imageCode;
                } else {
                    s_enemy[3].tu = 0.0f;
                    s_enemy[0].tu = 0.0f;
                    s_enemy[2].tu = 1.0f;
                    s_enemy[1].tu = 1.0f;
                }
                offsetX = 0;
                offsetZ = 0;
                lift = 0.0f;
                if (x == partyX && y == partyY) {
                    if (g_renderMode == RENDER_MODE_VIEW_FRAME) {
                        switch (g_viewDirection) {
                            case VIEW_NORTH:
                                offsetZ = 30;
                                partyY++;
                                break;
                            case VIEW_EAST:
                                offsetX = -30;
                                partyX--;
                                break;
                            case VIEW_SOUTH:
                                offsetZ = -30;
                                partyY--;
                                break;
                            case VIEW_WEST:
                                offsetX = 30;
                                partyX++;
                                break;
                        }
                    } else {
                        lift = DATA_COMPGEN(0x00064aa4, 30.0f);
                        switch (g_viewDirection) {
                            case VIEW_NORTH:
                                offsetZ = 40;
                                break;
                            case VIEW_EAST:
                                offsetX = 40;
                                break;
                            case VIEW_SOUTH:
                                offsetZ = -40;
                                break;
                            case VIEW_WEST:
                                offsetX = -40;
                                break;
                        }
                    }
                } else {
                    switch (g_viewDirection) {
                        case VIEW_NORTH:
                            offsetZ = -30;
                            break;
                        case VIEW_EAST:
                            offsetX = -30;
                            break;
                        case VIEW_SOUTH:
                            offsetZ = 30;
                            break;
                        case VIEW_WEST:
                            offsetX = 30;
                            break;
                    }
                }
                dx = (x - partyX) * 320 + offsetX;
                dz = (partyY - y) * 320 + offsetZ;
                spread = 0;
                if (found == CELL_PAIR) {
                    spread = 40;
                    if (n == 0) {
                        spread = -40;
                    }
                } else if (found > 2 && n % SPREAD_SLOTS != 0) {
                    spread = 40;
                    if (n % SPREAD_SLOTS == SPREAD_SLOT_LEFT) {
                        spread = -40;
                    }
                }
                switch (g_viewDirection) {
                    case VIEW_NORTH:
                        dx += spread;
                        break;
                    case VIEW_EAST:
                        dz -= spread;
                        break;
                    case VIEW_SOUTH:
                        dx -= spread;
                        break;
                    case VIEW_WEST:
                        dz += spread;
                        break;
                }
                if (lit) {
                    anim = GetObjectAnim(index);
                    width = s_enemySizes[anim - 1][0];
                    height = s_enemySizes[anim - 1][1];
                    light = D3DVAL(anim) * 0.125f;
                    level = static_cast<int>(light * 255.0f);
                    SetQuadSpecular(s_enemy, RGBA_MAKE(level, level, level, 0xff));
                } else {
                    width = 128;
                    height = 256;
                    SetQuadSpecular(s_enemy, 0xff000000);
                }
                double billboardWidth = width;
                s_enemy[1].x = s_enemy[2].x = -g_billboardX * billboardWidth;
                s_enemy[0].x = s_enemy[3].x = g_billboardX * billboardWidth;
                s_enemy[0].z = s_enemy[3].z = g_billboardZ * billboardWidth;
                s_enemy[1].z = s_enemy[2].z = -g_billboardZ * billboardWidth;
                layer = GetObjectLayer(index) & 1;
                s_enemy[2].y = s_enemy[3].y = lift - g_enemyTextures[layer][0].bottomMargin;
                s_enemy[1].y = s_enemy[2].y + height;
                s_enemy[0].y = s_enemy[1].y;
                TranslateBillboard(s_enemy, dx, dz);
                if (g_deviceType != D3D_DEVICE_RAMP && shade && !g_fixedLighting) {
                    MakeBillboardCameraRelative(dx, dz);
                    if (byDistance) {
                        distance = GetBillboardDistance(dx, dz);
                        SetDistanceLight(light, distance);
                    } else {
                        light = 1.0f;
                    }
                    level = static_cast<int>(light * 255.0f);
                    SetQuadColor(s_enemy, RGBA_MAKE(level, level, level, 0xff));
                } else {
                    SetQuadColor(s_enemy, 0xffffffff);
                }
                g_d3dDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    GetTextureHandle(&g_enemyTextures[layer][imageCode])
                );
                if (g_deviceType == D3D_DEVICE_RAMP) {
                    if (!lit) {
                        result = g_d3dDevice->SetLightState(
                            D3DLIGHTSTATE_MATERIAL,
                            GetTextureMaterialHandle(
                                &g_enemyTextures[layer][imageCode],
                                TEXTURE_SHADE_NORMAL
                            )
                        );
                    } else {
                        result = g_d3dDevice->SetLightState(
                            D3DLIGHTSTATE_MATERIAL,
                            GetTextureMaterialHandle(
                                &g_enemyTextures[layer][imageCode],
                                TEXTURE_SHADE_LIT
                            )
                        );
                    }
                    if (result != D3D_OK) {
                        TraceD3DCallError(
                            "lpD3DDev->SetLightState()@RenderEnemy() returns ",
                            result
                        );
                    }
                } else if (lit) {
                    g_d3dDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, true);
                }
                info = &g_enemyTextures[layer][imageCode].image->info;
                if (info->biWidth > 256) {
                    for (i = 0; i < 4; i++) {
                        s_enemy[i].x *= DATA_COMPGEN(0x00064aa8, 2.0f);
                    }
                }
                if (info->biWidth > 256 && !HasTextureHandle(&g_enemyTextures[layer][imageCode])) {
                    if (!lit) {
                        rect.left = 128;
                        rect.top = 100;
                        rect.right = 512;
                        rect.bottom = 292;
                        g_renderTarget->Blt(
                            &rect,
                            g_enemyPictures[imageCode].surface,
                            &g_enemyPictures[imageCode].rect,
                            DDBLT_KEYSRC | DDBLT_WAIT,
                            NULL
                        );
                    } else {
                        rect.left = 256 - s_enemySizes[anim][0];
                        rect.top = 214 - s_enemySizes[anim][1] / 2;
                        rect.right = 640 - rect.left;
                        rect.bottom = s_enemySizes[anim][1] / 2 + 178;
                        g_renderTarget->Blt(
                            &rect,
                            g_enemyPictures[ENEMY_PICTURE_BIG].surface,
                            &g_enemyPictures[ENEMY_PICTURE_BIG].rect,
                            DDBLT_KEYSRC | DDBLT_WAIT,
                            NULL
                        );
                    }
                } else {
                    DrawLitQuad(s_enemy);
                }
                if (lit) {
                    g_d3dDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, false);
                }
                if (visible) {
                    if (g_hotspotCount >= 64) {
                        break;
                    }
                    Hotspot* hotspot = GetHotspot(g_hotspotCount);
                    D3DVECTOR corner;
                    D3DVECTOR screen;
                    ProjectBillboardRect(hotspot->rect, s_enemy, corner, screen);
                    hotspot->kind = HOTSPOT_TARGET;
                    hotspot->texture = &g_enemyTextures[layer][imageCode];
                    hotspot->value = GetObjectSlot(index);
                    coord = GetObjectCoordPtr(index);
                    hotspot->targetX = coord->x;
                    hotspot->targetY = coord->y;
                    hotspot->data = NULL;
                    g_hotspotCount++;
                }
            }
        }
    }
}

// The atexit callback of RenderEnemy's vertex table (nothing to destroy).
RVA_DYNINIT(0x0004d620, 0x1, RenderEnemy)

// The target reticle picture (w\target.bmp, 96x96): four corners that close
// in on the selected hotspot over MARK_STEPS frames, then the whole reticle.
// @identity-TODO: loaded by LoadGraphics (0x450ac0).
DATA(0x00084ce8)
Picture g_targetPicture;

// The hotspot the reticle marks, the hotspot count when one was last picked,
// the reticle's frame and its centre.
DATA(0x0006b4e4)
static i32 s_markedHotspot = -1;

DATA(0x00090ad8)
static u32 s_markedCount;

DATA(0x00090adc)
static i32 s_markFrame;

DATA(0x0008f208)
static u32 s_markX;

DATA(0x0008f2fc)
static u32 s_markY;

// Picks and marks the hotspot to point at: without a selection (or when the
// hotspots changed) the last target hotspot in view is selected, preferring
// the one at the party's cell while objects are in reach. The reticle closes
// in on it, then stays; picking one also reports its object and cell.
RVA(0x0004d630, 0x37b)
void DrawHotspotMarks(void) {
    RECT source;
    i32 i;
    u8 anyTarget;
    u32 x;
    u32 y;

    if (g_selectedHotspot == HOTSPOT_NONE || s_markedCount != g_hotspotCount) {
        anyTarget = true;
        s_markedCount = g_hotspotCount;
        if (AnyObjectInReach()) {
            anyTarget = false;
        }
        for (i = g_hotspotCount - 1; i >= 0; i--) {
            if (GetHotspot(i)->kind == HOTSPOT_TARGET
                && GetPartyView(GetHotspot(i)->targetX, GetHotspot(i)->targetY)
                && (anyTarget || IsPartyAt(GetHotspot(i)->targetX, GetHotspot(i)->targetY))) {
                s_markedHotspot = -1;
                g_selectedHotspot = i;
                break;
            }
        }
    }
    if (g_selectedHotspot == -1) {
        return;
    }
    if (s_markedHotspot != g_selectedHotspot) {
        s_markedHotspot = g_selectedHotspot;
        s_markedCount = g_hotspotCount;
        s_markFrame = 0;
        s_markX =
            (GetHotspot(g_selectedHotspot)->rect.right - GetHotspot(g_selectedHotspot)->rect.left)
                / 2
            + GetHotspot(g_selectedHotspot)->rect.left;
        if (s_markX > MARK_RIGHT) {
            s_markX = MARK_RIGHT;
        } else if (s_markX < MARK_HALF) {
            s_markX = MARK_HALF;
        }
        if (IsPartyAt(
                GetHotspot(g_selectedHotspot)->targetX,
                GetHotspot(g_selectedHotspot)->targetY
            )) {
            s_markY = MARK_BOTTOM;
        } else {
            s_markY = (GetHotspot(g_selectedHotspot)->rect.bottom
                       - GetHotspot(g_selectedHotspot)->rect.top)
                          / 2
                      + GetHotspot(g_selectedHotspot)->rect.top;
            if (s_markY > MARK_BOTTOM) {
                s_markY = MARK_BOTTOM;
            } else if (s_markY < MARK_HALF) {
                s_markY = MARK_HALF;
            }
        }
        g_hotspotObject = GetHotspot(g_selectedHotspot)->value;
        g_hotspotCellX = GetHotspot(g_selectedHotspot)->targetX;
        g_hotspotCellY = GetHotspot(g_selectedHotspot)->targetY;
    }
    if (s_markFrame < MARK_STEPS) {
        source.top = 0;
        source.left = 0;
        source.bottom = MARK_HALF;
        source.right = MARK_HALF;
        g_renderTarget->BltFast(
            s_markX / MARK_STEPS * s_markFrame,
            s_markY / MARK_STEPS * s_markFrame,
            g_targetPicture.surface,
            &source,
            DDBLTFAST_SRCCOLORKEY
        );
        source.top = 0;
        source.left = MARK_HALF;
        source.right = MARK_SIZE;
        source.bottom = MARK_HALF;
        g_renderTarget->BltFast(
            MARK_RIGHT - (MARK_RIGHT - s_markX) / MARK_STEPS * s_markFrame,
            s_markY / MARK_STEPS * s_markFrame,
            g_targetPicture.surface,
            &source,
            DDBLTFAST_SRCCOLORKEY
        );
        source.top = MARK_HALF;
        source.left = MARK_HALF;
        source.bottom = MARK_SIZE;
        source.right = MARK_SIZE;
        g_renderTarget->BltFast(
            MARK_RIGHT - (MARK_RIGHT - s_markX) / MARK_STEPS * s_markFrame,
            MARK_BOTTOM - (MARK_BOTTOM - s_markY) / MARK_STEPS * s_markFrame,
            g_targetPicture.surface,
            &source,
            DDBLTFAST_SRCCOLORKEY
        );
        source.left = 0;
        source.top = MARK_HALF;
        source.right = MARK_HALF;
        source.bottom = MARK_SIZE;
        g_renderTarget->BltFast(
            s_markX / MARK_STEPS * s_markFrame,
            MARK_BOTTOM - (MARK_BOTTOM - s_markY) / MARK_STEPS * s_markFrame,
            g_targetPicture.surface,
            &source,
            DDBLTFAST_SRCCOLORKEY
        );
        s_markFrame++;
    } else {
        x = max(s_markX - MARK_HALF, 0);
        y = max(s_markY - MARK_HALF, 0);
        source.top = 0;
        source.left = 0;
        source.bottom = MARK_SIZE;
        source.right = MARK_SIZE;
        g_renderTarget->BltFast(x, y, g_targetPicture.surface, &source, DDBLTFAST_SRCCOLORKEY);
    }
}

RVA(0x0004d9b0, 0x4f)
void BlitFieldBackground(void) {
    i16 state = GetGameState();
    if (state == GAME_STATE_SHOT || state == GAME_STATE_CLOSING_EFFECT) {
        RECT source;
        source.top = 0;
        source.left = 0;
        source.right = SCREEN_WIDTH;
        source.bottom = VIEW_HEIGHT;
        g_renderTarget->BltFast(0, 0, g_backdropPicture.surface, &source, DDBLTFAST_SRCCOLORKEY);
    }
}

RVA(0x0004da00, 0x29)
void DrawSceneOverlay(void) {
    if (GetGameState() == GAME_STATE_SCRIPT_ANIMATION) {
        g_renderTarget->BltFast(
            0,
            0,
            g_backdropPicture.surface,
            &g_backdropPicture.rect,
            DDBLTFAST_SRCCOLORKEY
        );
    }
}

// Draws the stairs in the cell ahead of the party (retail's trace names it
// DispStayer): a wall-high quad across the cell, a half cell nearer for the
// near variants, textured up or down by the code's low bit.
RVA(0x0004da30, 0x284)
void DrawStairs(void) {
    DATA(0x00084c68)
    static D3DLVERTEX s_stairs[4] = {
        D3DLVERTEX(D3DVECTOR(0.0f, 320.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 0.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 320.0f, 0.0f), 0xffffffff, 0xff000000, 1.0f, 0.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 0xffffffff, 0xff000000, 1.0f, 1.0f),
        D3DLVERTEX(D3DVECTOR(0.0f, 0.0f, 0.0f), 0xffffffff, 0xff000000, 0.0f, 1.0f),
    };
    i32 dx = 0;
    i32 dy = 0;
    i32 depth = STAIRS_FAR;
    i32 x0;
    i32 x1;
    i32 z0;
    i32 z1;
    i16 code;
    Texture* texture;
    HRESULT result;

    switch (g_viewDirection) {
        case VIEW_NORTH:
            dy = -1;
            break;
        case VIEW_EAST:
            dx = 1;
            break;
        case VIEW_SOUTH:
            dy = 1;
            break;
        case VIEW_WEST:
            dx = -1;
            break;
    }
    code = GetCellAtOffset(dx, dy);
    if (code & CELL_STAIRS_NEAR) {
        code &= 0xff & ~CELL_STAIRS_NEAR; // the code is a byte
        depth = STAIRS_HALF_WIDTH;
    }
    switch (g_viewDirection) {
        case VIEW_NORTH:
            x0 = -STAIRS_HALF_WIDTH;
            x1 = STAIRS_HALF_WIDTH;
            z1 = depth;
            z0 = depth;
            break;
        case VIEW_EAST:
            z0 = STAIRS_HALF_WIDTH;
            z1 = -STAIRS_HALF_WIDTH;
            x1 = depth;
            x0 = depth;
            break;
        case VIEW_SOUTH:
            x0 = STAIRS_HALF_WIDTH;
            x1 = -STAIRS_HALF_WIDTH;
            z1 = -depth;
            z0 = -depth;
            break;
        case VIEW_WEST:
            z0 = -STAIRS_HALF_WIDTH;
            z1 = STAIRS_HALF_WIDTH;
            x1 = -depth;
            x0 = -depth;
            break;
    }
    if (code == CELL_STAIRS_UP || code == CELL_STAIRS_DOWN || code == CELL_STEPS_UP
        || code == CELL_STEPS_DOWN) {
        s_stairs[0].x = s_stairs[3].x = static_cast<D3DVALUE>(x0);
        s_stairs[1].x = s_stairs[2].x = static_cast<D3DVALUE>(x1);
        s_stairs[0].z = s_stairs[3].z = static_cast<D3DVALUE>(z0);
        s_stairs[1].z = s_stairs[2].z = static_cast<D3DVALUE>(z1);
        texture = code & 1 ? &g_stairsDownTexture : &g_stairsUpTexture;
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, GetTextureHandle(texture));
        if (g_deviceType == D3D_DEVICE_RAMP) {
            result = g_d3dDevice->SetLightState(
                D3DLIGHTSTATE_MATERIAL,
                GetTextureMaterialHandle(texture, TEXTURE_SHADE_NORMAL)
            );
            if (result != D3D_OK) {
                TraceD3DCallError("lpD3DDev->SetLightState()@DispStayer() returns ", result);
            }
        }
        DrawLitQuad(s_stairs);
    }
}

// The atexit callback of DrawStairs' vertex table (nothing to destroy).
RVA_DYNINIT(0x0004dcc0, 0x1, DrawStairs)

RVA(0x0004dcd0, 0x3b5)
i32 DrawSprites(void) {
    i32 count = 0;
    i32 order;
    i16 slot;
    i16 group;
    i16 frame;
    RECT rect;
    RECT dest;
    u32 top;
    u32 image;
    if (s_screenSaved) {
        return 0;
    }
    for (order = 0; order < SPRITE_SLOT_COUNT; order++) {
        slot = g_spriteOrder[order];
        if (slot == SPRITE_UNPLACED) {
            continue;
        }
        frame = GetSpriteSlotFrame(GetSpriteSlot(slot));
        if (frame == SPRITE_UNPLACED) {
            continue;
        }
        group = GetSpriteSlot(slot)->group;
        if (IsSpriteFrameIndexOutOfRange(group, frame)) {
            continue;
        }
        if (GetSpriteFramePicture(group, frame)->surface == NULL) {
            continue;
        }
        rect = GetSpriteFramePicture(group, frame)->rect;
        count++;
        image = GetSpriteFramePicture(group, frame)->id;
        if (image == SPRITE_IMAGE_CACHED_BACKDROP || image == SPRITE_IMAGE_CACHED_OVERLAY) {
            if (g_spritePicture.id != image) {
                g_spritePicture.rect = GetSpriteFramePicture(group, frame)->rect;
                g_spritePicture.surface->BltFast(
                    0,
                    0,
                    GetSpriteFramePicture(group, frame)->surface,
                    &rect,
                    DDBLTFAST_WAIT
                );
                g_spritePicture.id = image;
            }
        } else if (image == SPRITE_IMAGE_CLEAR_BACKDROP && frame == 0) {
            if (g_spritePicture.id) {
                ClearDisplaySurface(g_spritePicture.surface, &rect);
                g_spritePicture.id = 0;
            }
        } else if (image < SPRITE_IMAGE_INSET_END && image > SPRITE_IMAGE_INSET_EXCLUSIVE_BEGIN) {
            g_renderTarget->BltFast(
                0,
                0,
                g_spritePicture.surface,
                &g_spritePicture.rect,
                DDBLTFAST_SRCCOLORKEY
            );
        }
        if (image < SPRITE_IMAGE_INSET_END && image > SPRITE_IMAGE_INSET_EXCLUSIVE_BEGIN) {
            if (g_spritePicture.id == SPRITE_IMAGE_CACHED_BACKDROP) {
                g_renderTarget->BltFast(
                    240,
                    176,
                    GetSpriteFramePicture(group, frame)->surface,
                    &rect,
                    DDBLTFAST_SRCCOLORKEY
                );
            }
        } else if (image == SPRITE_IMAGE_CLEAR_BACKDROP) {
            g_renderTarget->BltFast(
                0,
                0,
                GetSpriteFramePicture(group, frame)->surface,
                &rect,
                DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY
            );
            g_scenePicture.visible = false;
            g_renderTarget->BltFast(
                204,
                112,
                g_spritePicture.surface,
                &g_spritePicture.rect,
                DDBLTFAST_SRCCOLORKEY
            );
        } else if (image == SPRITE_IMAGE_CACHED_OVERLAY) {
            g_scenePicture.visible = false;
            g_renderTarget->BltFast(
                204,
                112,
                GetSpriteFramePicture(group, frame)->surface,
                &rect,
                DDBLTFAST_SRCCOLORKEY
            );
        } else {
            if (image != SPRITE_IMAGE_CACHED_BACKDROP) {
                g_scenePicture.visible = true;
                if (g_spritePicture.id) {
                    ClearDisplaySurface(g_spritePicture.surface, &rect);
                    g_spritePicture.id = 0;
                }
            }
            if (GetSpriteSlot(slot)->y != 0
                && GetPictureSurfaceWidth(GetSpriteFramePicture(group, frame)) != SCREEN_WIDTH) {
                top = GetSpriteSlot(slot)->y - rect.bottom / 2;
                if (top > 28) {
                    top -= 28;
                } else {
                    top = 0;
                }
            } else {
                top = 0;
            }
            if (GetPictureSurfaceWidth(GetSpriteFramePicture(group, frame)) != SCREEN_WIDTH) {
                i32 width = rect.right - rect.left;
                i32 height = rect.bottom - rect.top;
                dest.left = GetSpriteSlot(slot)->x - width * 3 / 8;
                dest.top = top + height / 8;
                dest.right = dest.left + width * 3 / 4;
                dest.bottom = top + height * 7 / 8;
                g_renderTarget->Blt(
                    &dest,
                    GetSpriteFramePicture(group, frame)->surface,
                    &rect,
                    DDBLT_KEYSRC,
                    NULL
                );
            } else {
                g_renderTarget->BltFast(
                    0,
                    top,
                    GetSpriteFramePicture(group, frame)->surface,
                    &rect,
                    DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY
                );
            }
        }
    }
    return count;
}

// @early-stop register allocation: retail walks the slots through a pointer
// to their frame words kept in ebx (spilled), with the bottom edge in edi.
// The tests, rectangle spill, coordinate arithmetic and blit match.
RVA(0x0004e090, 0x134)
void DrawSceneSprites(void) {
    i32 i;
    LPDIRECTDRAWSURFACE surface;
    RECT dest;
    RECT source;
    i32 x;
    i32 y;
    i32 lift;

    if (s_screenSaved) {
        return;
    }
    for (i = SPRITE_SLOT_COUNT - 1; i >= 0; i--) {
        if (GetSpriteSlotFrame(GetSpriteSlot(i)) == SPRITE_UNPLACED) {
            continue;
        }
        if (IsSpriteFrameIndexOutOfRange(
                GetSpriteSlot(i)->group,
                GetSpriteSlotFrame(GetSpriteSlot(i))
            )) {
            continue;
        }
        surface =
            GetSpriteFramePicture(GetSpriteSlot(i)->group, GetSpriteSlotFrame(GetSpriteSlot(i)))
                ->surface;
        if (surface == NULL) {
            continue;
        }
        dest = GetSpriteFramePicture(GetSpriteSlot(i)->group, GetSpriteSlotFrame(GetSpriteSlot(i)))
                   ->rect;
        x = GetSpriteSlot(i)->x - dest.right / 2;
        y = GetSpriteSlot(i)->y;
        lift = dest.bottom * 3 / 4;
        if (y > lift) {
            y -= lift;
        } else {
            y = 0;
        }
        dest.left += x;
        dest.right += x;
        dest.top += y;
        dest.bottom += y;
        source.left = 0;
        source.top = 0;
        source.right = GetPictureSurfaceWidth(
            GetSpriteFramePicture(GetSpriteSlot(i)->group, GetSpriteSlotFrame(GetSpriteSlot(i)))
        );
        source.bottom = GetPictureSurfaceHeight(
            GetSpriteFramePicture(GetSpriteSlot(i)->group, GetSpriteSlotFrame(GetSpriteSlot(i)))
        );
        g_renderTarget->Blt(&dest, surface, &source, DDBLT_KEYSRC, NULL);
    }
}

// The order BlitScreenLayers walks the first layers in when asked to.
DATA(0x0006bc38)
static GZ_ENUM_STORAGE(ScreenLayerSlot, i32) s_layerOrder[8] = {
    SCREEN_LAYER_TEXT,
    SCREEN_LAYER_FIRST_PANEL,
    static_cast<GZ_ENUM_STORAGE(ScreenLayerSlot, i32)>(SCREEN_LAYER_FIRST_PANEL + 1),
    static_cast<GZ_ENUM_STORAGE(ScreenLayerSlot, i32)>(SCREEN_LAYER_FIRST_PANEL + 2),
    static_cast<GZ_ENUM_STORAGE(ScreenLayerSlot, i32)>(SCREEN_LAYER_FIRST_PANEL + 3),
    static_cast<GZ_ENUM_STORAGE(ScreenLayerSlot, i32)>(SCREEN_LAYER_FIRST_PANEL + 4),
    static_cast<GZ_ENUM_STORAGE(ScreenLayerSlot, i32)>(SCREEN_LAYER_FIRST_PANEL + 5),
    SCREEN_LAYER_MENU_BAR
};

RVA(0x0004e1d0, 0xfd)
void BlitScreenLayers(i32 first, i32 last, u32 flags) {
    i32 i;
    GZ_ENUM_LOCAL(ScreenLayerSlot, i32) index;

    if (flags & BLIT_LAYERS_ORDERED) {
        for (i = first; i < last; i++) {
            index = s_layerOrder[i];
            BlitScreenLayer(g_renderTarget, g_screenLayers[index]);
        }
    } else {
        for (i = last - 1; i >= first; i--) {
            BlitScreenLayer(g_renderTarget, g_layerStack[i]);
        }
    }
}

// Keypad blits since its pressed-key highlights were last cleared.
DATA(0x00090ae0)
static i32 s_keypadHighlightFrames;

RVA(0x0004e2d0, 0xf8)
i32 BlitTextPlanes(i32 first, i32 last, u32 skip) {
    RECT rows;
    i32 blitted = 0;
    i32 plane;

    for (plane = first; plane < last; plane++) {
        if (!(skip & (1 << plane)) && GetTextPlane(plane)->kind != TEXT_PLANE_FREE
            && GetTextPlane(plane)->visible) {
            g_renderTarget->BltFast(
                GetTextPlane(plane)->left,
                GetTextPlane(plane)->top,
                GetTextPlane(plane)->surface,
                &GetTextPlane(plane)->sourceRect,
                DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT
            );
            g_renderTarget->BltFast(
                GetTextPlane(plane)->left,
                GetTextPlane(plane)->top,
                GetTextPlane(plane)->glyphSurface,
                &GetTextPlane(plane)->sourceRect,
                DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT
            );
            blitted++;
            if (GetTextPlane(plane)->kind == TEXT_PLANE_KIND_KEYPAD
                && s_keypadHighlightFrames++ > 3) {
                s_keypadHighlightFrames = 0;
                rows.left = 0;
                rows.top = 32;
                rows.right = 134;
                rows.bottom = 160;
                GetTextPlane(plane)
                    ->glyphSurface->Blt(&rows, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
            }
        }
    }
    return blitted;
}

// The last rendered 3D view (640x328): RenderViewMode keeps it and redraws
// from it while nothing moves.
DATA(0x0008f5d0)
Picture g_viewCachePicture;

// The room's floor and ceiling mesh (BuildRoomMesh) and the backdrop the
// view shows under fixed lighting (w\white.bmp).
DATA(0x00084d10)
Mesh g_roomMesh;

DATA(0x0008f1d8)
Picture g_whitePicture;

// The fog state last set (it follows g_fixedLighting).
DATA(0x00084350)
static BOOL s_fogEnabled;

// RENDER_MODE_VIEW (retail's trace names it Rend3D): steps the move under way,
// then either renders the room, walls, door, stairs and billboards afresh
// (caching the result once nothing moves) or redraws the cache with the
// door and the NPCs over it; then the sprites, layers and text planes.
// Without `draw` only the door keeps opening.
RVA(0x0004e3d0, 0x53e)
void RenderViewMode(BOOL draw) {
    b32 moved = false;
    Texture* texture;
    HRESULT result;
    MapPosition* position;

    if (AnimateMove()) {
        moved = true;
    }
    if (g_scenePicture.visible) {
        g_renderTarget->BltFast(0, 0, g_scenePicture.surface, &g_scenePicture.rect, DDBLTFAST_WAIT);
    }
    if (!draw) {
        if ((g_moveState == MOVE_STATE_DOOR_AHEAD || g_moveState == MOVE_STATE_DOOR_LEFT
             || g_moveState == MOVE_STATE_DOOR_RIGHT)
            && s_doorFrame > 1) {
            s_doorFrame--;
        }
        return;
    }
    g_viewport->Clear(1, &g_viewClearRect, D3DCLEAR_ZBUFFER);
    if (s_viewDirty) {
        if (!g_fixedLighting) {
            g_renderTarget->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &g_clearBltFx);
        } else {
            g_renderTarget
                ->BltFast(0, 0, g_whitePicture.surface, &g_whitePicture.rect, DDBLTFAST_WAIT);
        }
        ShadeMesh(&g_roomMesh);
        ShadeMesh(&g_wallMesh);
        g_d3dDevice->BeginScene();
        if (g_deviceType != D3D_DEVICE_MMX) {
            g_d3dDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
        }
        if (g_moveState == MOVE_STATE_STEP && s_moveProgress > CAMERA_DISTANCE) {
            texture = GetViewPaletteMode() ? &g_darkWallTexture : &g_roomTexture;
        } else {
            texture = GetAreaPaletteMode() ? &g_darkWallTexture : &g_roomTexture;
        }
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, texture->handle);
        if (g_deviceType == D3D_DEVICE_RAMP) {
            result = g_d3dDevice->SetLightState(
                D3DLIGHTSTATE_MATERIAL,
                GetTextureMaterialHandle(texture, TEXTURE_SHADE_NORMAL)
            );
            if (result != D3D_OK) {
                TraceD3DCallError("lpD3DDev->SetLightState()@Rend3D()-1 returns ", result);
            }
        }
        if (s_fogEnabled != g_fixedLighting) {
            g_d3dDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, g_fixedLighting);
            s_fogEnabled = g_fixedLighting;
        }
        g_d3dDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            D3DVT_LVERTEX,
            g_roomMesh.vertices,
            g_roomMesh.vertexCount,
            g_roomMesh.indices,
            g_roomMesh.indexCount,
            D3DDP_DONOTUPDATEEXTENTS
        );
        g_d3dDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            D3DVT_LVERTEX,
            g_wallMesh.vertices,
            g_wallMesh.vertexCount,
            g_wallMesh.indices,
            g_wallMesh.indexCount,
            D3DDP_DONOTUPDATEEXTENTS
        );
        if (s_doorOpening && (g_moveState == MOVE_STATE_STEP || g_moveState == MOVE_STATE_LEFT)) {
            if (g_bilinearFiltering) {
                g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_NEAREST);
                g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, D3DFILTER_NEAREST);
            }
            if (g_moveState == MOVE_STATE_STEP || g_moveState == MOVE_STATE_LEFT
                || g_moveState == MOVE_STATE_RIGHT) {
                g_d3dDevice->DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST,
                    D3DVT_LVERTEX,
                    g_doorMesh.vertices,
                    g_doorMesh.vertexCount,
                    g_doorMesh.indices,
                    g_doorMesh.indexCount,
                    D3DDP_DONOTUPDATEEXTENTS
                );
            }
            if (g_bilinearFiltering) {
                g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_LINEAR);
                g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, D3DFILTER_LINEAR);
            }
        }
        DrawStairs();
        if (g_deviceType != D3D_DEVICE_MMX) {
            g_d3dDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
        }
        g_hotspotCount = 0;
        memset(g_hotspots, 0, sizeof(g_hotspots));
        RenderTBox();
        RenderNPC(false);
        RenderEnemy(true, false, true);
        if (!moved) {
            g_viewCachePicture.surface->BltFast(0, 0, g_renderTarget, &g_viewCachePicture.rect, 0);
            s_viewDirty = false;
        }
    } else {
        g_renderTarget->BltFast(0, 0, g_viewCachePicture.surface, &g_viewCachePicture.rect, 0);
        g_d3dDevice->BeginScene();
        texture = GetAreaPaletteMode() ? &g_darkWallTexture : &g_roomTexture;
        g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, texture->handle);
        if (g_deviceType == D3D_DEVICE_RAMP) {
            result = g_d3dDevice->SetLightState(
                D3DLIGHTSTATE_MATERIAL,
                GetTextureMaterialHandle(texture, TEXTURE_SHADE_NORMAL)
            );
            if (result != D3D_OK) {
                TraceD3DCallError("lpD3DDev->SetLightState()@Rend3D()-2 returns ", result);
            }
        }
        if (g_moveState >= MOVE_STATE_DOOR_FIRST) {
            if (g_bilinearFiltering) {
                g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_NEAREST);
                g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, D3DFILTER_NEAREST);
            }
            if (g_moveState == MOVE_STATE_DOOR_AHEAD) {
                g_moveState = static_cast<GZ_ENUM_STORAGE(CameraMoveState, u32)>(
                    AnimateDoor(&g_doorMesh));
                if (g_moveState == MOVE_STATE_STEP) {
                    s_doorFrame = 0;
                    s_doorOpening = true;
                    s_viewDirty = true;
                }
            } else if (g_moveState == MOVE_STATE_DOOR_BACK) {
                g_moveState = MOVE_STATE_BACK;
            } else if (g_moveState == MOVE_STATE_DOOR_LEFT) {
                g_moveState = static_cast<GZ_ENUM_STORAGE(CameraMoveState, u32)>(
                    AnimateDoor(&g_doorMesh));
                if (g_moveState == MOVE_STATE_LEFT) {
                    s_doorFrame = 0;
                    s_doorOpening = true;
                    s_viewDirty = true;
                }
            } else if (g_moveState == MOVE_STATE_DOOR_RIGHT) {
                g_moveState = static_cast<GZ_ENUM_STORAGE(CameraMoveState, u32)>(
                    AnimateDoor(&g_doorMesh));
                if (g_moveState == MOVE_STATE_RIGHT) {
                    s_doorFrame = 0;
                    s_doorOpening = true;
                    s_viewDirty = true;
                }
            }
            if (g_bilinearFiltering) {
                g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_LINEAR);
                g_d3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, D3DFILTER_LINEAR);
            }
            RenderNPC(true);
        }
        if (moved) {
            s_viewDirty = true;
        }
        BlitFieldBackground();
        DrawHotspotMarks();
        if (g_moveState == MOVE_STATE_NONE) {
            position = GetMapPosition();
            UpdateFieldHud(position->x, position->y, position->direction);
        }
    }
    g_d3dDevice->EndScene();
    DrawSprites();
    BlitScreenLayers(0, SCREEN_LAYER_COUNT, 0);
    BlitTextPlanes(0, TEXT_PLANE_COUNT, 0);
    if (s_layerDragging) {
        DrawDragOutline();
    }
    if (!s_viewDirty && GetFieldBattleActive()) {
        g_renderTarget->BltFast(
            208,
            0,
            g_fightBannerPicture.surface,
            &g_fightBannerPicture.rect,
            DDBLTFAST_SRCCOLORKEY
        );
    }
}

RVA(0x0004e910, 0xf8)
void RenderEventMode(BOOL draw) {
    if (g_scenePicture.visible) {
        g_renderTarget->BltFast(0, 0, g_scenePicture.surface, &g_scenePicture.rect, DDBLTFAST_WAIT);
    } else {
        ClearDisplaySurface(g_renderTarget, NULL);
    }
    g_renderTarget->BltFast(
        240,
        365,
        g_titleMenuPicture.surface,
        &g_titleMenuPicture.rect,
        DDBLTFAST_SRCCOLORKEY
    );
    if ((g_scenePicture.id > EVENT_CURSOR_DELAY_EXCLUSIVE_BEGIN
         && g_scenePicture.id < EVENT_CURSOR_DELAY_END)
        || g_scenePicture.id == EVENT_CURSOR_DELAY_SINGLE) {
        if (GetFrameCount() < 60) {
            TickFrameCount();
            s_cursorArmed = false;
        } else {
            s_cursorArmed = true;
        }
    } else {
        s_cursorArmed = (g_scenePicture.id > EVENT_CURSOR_ACTIVE_EXCLUSIVE_BEGIN
                         && g_scenePicture.id < EVENT_CURSOR_ACTIVE_END)
                        || (g_scenePicture.id > EVENT_CURSOR_ACTIVE_ALT_EXCLUSIVE_BEGIN
                            && g_scenePicture.id < EVENT_CURSOR_ACTIVE_ALT_END)
                        || g_scenePicture.id == EVENT_CURSOR_ACTIVE_SINGLE;
    }
    if (DrawSprites()) {
        s_cursorArmed = true;
    }
    if (BlitTextPlanes(0, TEXT_PLANE_COUNT, 0)) {
        s_cursorArmed = true;
    }
}

// RENDER_MODE_SCENE: the scene picture (or black), the sprites, the layers
// and the text planes.
RVA(0x0004ea10, 0x7d)
void RenderSceneMode(BOOL draw) {
    if (draw) {
        if (g_scenePicture.visible) {
            g_renderTarget
                ->BltFast(0, 0, g_scenePicture.surface, &g_scenePicture.rect, DDBLTFAST_WAIT);
        } else {
            ClearDisplaySurface(g_renderTarget, NULL);
        }
        DrawSprites();
        if (GetSpriteMode() == SPRITE_LAYERS_PARTY_AND_TEXT) {
            BlitScreenLayers(0, 7, BLIT_LAYERS_ORDERED);
        } else {
            BlitScreenLayers(0, SCREEN_LAYER_COUNT, 0);
        }
        BlitTextPlanes(0, TEXT_PLANE_COUNT, SKIP_TEXT_PLANE(TEXT_PLANE_SCENE_HIDDEN));
    }
}

// Renders the 3D view into the render target and composites the screen
// layers and text planes over it; does nothing unless `draw` is set.
// @identity-TODO: the backdrop quad is initialized and tinted here but never
// drawn; which use was dropped is unrecovered.
RVA(0x0004ea90, 0x237)
void RenderFieldView(BOOL draw) {
    DATA(0x0008d210)
    static D3DTLVERTEX s_backdrop[4] = {
        D3DTLVERTEX(D3DVECTOR(160.0f, 4.0f, 0.0f), 1.0f, 0xffffffff, 0xff000000, 0.0f, 0.0f),
        D3DTLVERTEX(D3DVECTOR(480.0f, 4.0f, 0.0f), 1.0f, 0xffffffff, 0xff000000, 1.0f, 0.0f),
        D3DTLVERTEX(D3DVECTOR(160.0f, 324.0f, 0.0f), 1.0f, 0xffffffff, 0xff000000, 0.0f, 1.0f),
        D3DTLVERTEX(D3DVECTOR(480.0f, 324.0f, 0.0f), 1.0f, 0xffffffff, 0xff000000, 1.0f, 1.0f),
    };
    int level;
    D3DVALUE light;
    D3DCOLOR specular;

    if (!draw) {
        return;
    }
    g_renderTarget->BltFast(0, 0, g_scenePicture.surface, &g_scenePicture.rect, DDBLTFAST_WAIT);
    if (IsFieldObjectImageLit(GetObjectImageCode(0))) {
        level = GetObjectAnim(0);
        if (level > 8) {
            level = 7;
        }
        light = D3DVAL(level) * 0.125f;
        specular = D3DRGB(light, light, light);
        s_backdrop[0].specular = s_backdrop[1].specular = s_backdrop[2].specular =
            s_backdrop[3].specular = specular;
    } else {
        s_backdrop[0].specular = s_backdrop[1].specular = s_backdrop[2].specular =
            s_backdrop[3].specular = 0xff000000;
    }
    g_viewport->Clear(1, &g_viewClearRect, D3DCLEAR_ZBUFFER);
    g_d3dDevice->BeginScene();
    memset(g_hotspots, 0, sizeof(g_hotspots));
    g_hotspotCount = 0;
    RenderEnemy(true, false, true);
    g_d3dDevice->EndScene();
    BlitFieldBackground();
    DrawHotspotMarks();
    BlitScreenLayers(0, 15, 0);
    BlitTextPlanes(0, 37, 0);
    if (GetFieldBattleActive()) {
        g_renderTarget->BltFast(
            208,
            0,
            g_fightBannerPicture.surface,
            &g_fightBannerPicture.rect,
            DDBLTFAST_SRCCOLORKEY
        );
    }
}

// The atexit callback of RenderFieldView's vertex table (nothing to destroy).
RVA_DYNINIT(0x0004ecd0, 0x1, RenderFieldView)

DATA(0x0008f3f0)
Picture g_commandBarPicture;

// RENDER_MODE_PICTURE: the command bar and the status picture on black, the
// first layers on the status screen (except in its phase 3), and the text
// planes from 1.
RVA(0x0004ece0, 0x97)
void RenderPictureMode(BOOL draw) {
    if (draw) {
        ClearDisplaySurface(g_renderTarget, NULL);
        g_renderTarget->BltFast(
            16,
            16,
            g_commandBarPicture.surface,
            &g_commandBarPicture.rect,
            DDBLTFAST_SRCCOLORKEY
        );
        g_renderTarget->BltFast(
            0,
            40,
            g_statusPicture.surface,
            &g_statusPicture.rect,
            DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT
        );
        if (GetGameState() == GAME_STATE_STATUS && GetGamePhase() != STATUS_PHASE_NO_LAYERS) {
            BlitScreenLayers(0, 7, BLIT_LAYERS_ORDERED);
        }
        BlitTextPlanes(1, TEXT_PLANE_COUNT, 0);
    }
}

// Render mode handlers (the table 0x46bc58, indexed by the render mode); each
// takes whether the frame is drawn.

// Where each of the six world-map screens shows its part of the map.
DATA(0x0006b790)
MapScreenOffset g_mapScreenOffsets[MAP_SCREEN_COUNT] = {
    {-112, -36},
    {176, -36},
    {464, -36},
    {-112, 164},
    {176, 164},
    {464, 164},
};

DATA(0x0006b7a8)
JoystickKey g_joystickKeys[8] = {
    {JOY_UP, VK_UP},
    {JOY_DOWN, VK_DOWN},
    {JOY_LEFT, VK_LEFT},
    {JOY_RIGHT, VK_RIGHT},
    {1 << JOY_BUTTON_SHIFT, VK_RETURN},
    {2 << JOY_BUTTON_SHIFT, VK_SPACE},
    {4 << JOY_BUTTON_SHIFT, VK_SHIFT},
    {0, 0},
};

DATA(0x0008f570)
MarkerColor g_markerColors[4];

// The party marker on the world map.
DATA(0x0008fd80)
Picture g_mapMarkerPicture;

// RENDER_MODE_PANEL: the world map (the scene picture, or black), the cached
// 3D view keyed over it under or over the party marker (over on odd map
// layers), the sprites, layers and text planes, and the drag outline.
RVA(0x0004ed80, 0x1ad)
void RenderPanelMode(BOOL draw) {
    i16 x;
    i16 y;
    i16 screen;

    if (draw) {
        if (g_scenePicture.visible) {
            g_renderTarget->BltFast(0, 0, g_scenePicture.surface, &g_windowRect, DDBLTFAST_WAIT);
        } else {
            ClearDisplaySurface(g_renderTarget, NULL);
        }
        screen = GetWorldMapMarker(&x, &y);
        if (screen >= 0 && screen < MAP_SCREEN_COUNT) {
            x += g_mapScreenOffsets[screen].x;
            y += g_mapScreenOffsets[screen].y;
            if (x < 0) {
                x = 0;
            } else if (x >= MAP_MARKER_MAX_X) {
                x = MAP_MARKER_MAX_X;
            }
            if (y < 0) {
                y = 0;
            } else if (y >= MAP_MARKER_MAX_Y) {
                y = MAP_MARKER_MAX_Y;
            }
        }
        if (g_scenePicture.visible) {
            if (IsOddMapLayer()) {
                g_renderTarget->BltFast(
                    0,
                    0,
                    g_viewCachePicture.surface,
                    &g_viewCachePicture.rect,
                    DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT
                );
                if (screen >= 0 && screen < MAP_SCREEN_COUNT) {
                    g_renderTarget->BltFast(
                        x,
                        y,
                        g_mapMarkerPicture.surface,
                        &g_mapMarkerPicture.rect,
                        DDBLTFAST_SRCCOLORKEY
                    );
                }
            } else {
                if (screen >= 0 && screen < MAP_SCREEN_COUNT) {
                    g_renderTarget->BltFast(
                        x,
                        y,
                        g_mapMarkerPicture.surface,
                        &g_mapMarkerPicture.rect,
                        DDBLTFAST_SRCCOLORKEY
                    );
                }
                g_renderTarget->BltFast(
                    0,
                    0,
                    g_viewCachePicture.surface,
                    &g_viewCachePicture.rect,
                    DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT
                );
            }
        }
        DrawSprites();
        BlitScreenLayers(0, SCREEN_LAYER_COUNT, 0);
        BlitTextPlanes(0, TEXT_PLANE_COUNT, 0);
        if (s_layerDragging) {
            DrawDragOutline();
        }
    }
}

// RENDER_MODE_FIELD: RenderFieldView's composition with the objects fully lit
// (no distance shading) and culling off except on the MMX device.
RVA(0x0004ef30, 0xdd)
void RenderFieldMode(BOOL draw) {
    if (draw) {
        g_renderTarget->BltFast(0, 0, g_scenePicture.surface, &g_scenePicture.rect, DDBLTFAST_WAIT);
        g_viewport->Clear(1, &g_viewClearRect, D3DCLEAR_ZBUFFER);
        g_d3dDevice->BeginScene();
        if (g_deviceType != D3D_DEVICE_MMX) {
            g_d3dDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
        }
        memset(g_hotspots, 0, sizeof(g_hotspots));
        g_hotspotCount = 0;
        RenderEnemy(true, false, false);
        g_d3dDevice->EndScene();
        BlitFieldBackground();
        DrawHotspotMarks();
        BlitScreenLayers(0, SCREEN_LAYER_COUNT, 0);
        BlitTextPlanes(0, TEXT_PLANE_COUNT, 0);
        if (GetFieldBattleActive()) {
            g_renderTarget->BltFast(
                208,
                0,
                g_fightBannerPicture.surface,
                &g_fightBannerPicture.rect,
                DDBLTFAST_SRCCOLORKEY
            );
        }
    }
}

// RENDER_MODE_BLANK: black, then the status picture (first blank step) or
// the scene picture with its sprites (second), and the text planes from 1.
RVA(0x0004f010, 0x98)
void RenderBlankMode(BOOL draw) {
    if (draw) {
        ClearDisplaySurface(g_renderTarget, NULL);
        if (s_blankStep == BLANK_STEP_FIRST) {
            g_renderTarget->BltFast(
                0,
                40,
                g_statusPicture.surface,
                &g_statusPicture.rect,
                DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT
            );
            BlitTextPlanes(1, TEXT_PLANE_COUNT, 0);
            return;
        }
        if (s_blankStep == BLANK_STEP_SECOND) {
            g_renderTarget
                ->BltFast(0, 0, g_scenePicture.surface, &g_scenePicture.rect, DDBLTFAST_WAIT);
            DrawSceneSprites();
            DrawSceneOverlay();
        }
        BlitTextPlanes(1, TEXT_PLANE_COUNT, 0);
    }
}

// Draws the status picture and the text planes.
RVA(0x0004f0b0, 0x4f)
void RenderStatusMode(BOOL draw) {
    if (draw) {
        ClearDisplaySurface(g_renderTarget, NULL);
        g_renderTarget->BltFast(
            0,
            40,
            g_statusPicture.surface,
            &g_statusPicture.rect,
            DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT
        );
        BlitTextPlanes(1, TEXT_PLANE_COUNT, 0);
    }
}

// Draws the screen layers and the text planes only.
RVA(0x0004f100, 0x28)
void RenderLayersMode(BOOL draw) {
    if (draw) {
        BlitScreenLayers(0, 7, BLIT_LAYERS_ORDERED);
        BlitTextPlanes(1, TEXT_PLANE_COUNT, 0);
    }
}

// The handler of the unused render modes.
RVA(0x0004f130, 0x1)
void RenderNothing(BOOL draw) {}

// The frame handlers by render mode.
DATA(0x0006bc58)
static void (*s_renderModes[16])(BOOL draw) = {
    RenderEventMode,
    RenderViewMode,
    RenderSceneMode,
    RenderFieldView,
    RenderPictureMode,
    RenderPanelMode,
    RenderFieldMode,
    RenderBlankMode,
    RenderStatusMode,
    RenderLayersMode,
    RenderNothing,
    RenderNothing,
    RenderNothing,
    RenderNothing,
    RenderNothing,
    RenderNothing,
};

// When the last frame was drawn (timeGetTime).
DATA(0x0008f2e8)
static DWORD s_lastDrawTime;

// The minimum time between drawn frames of an animated mode, in ms.
#define FRAME_INTERVAL 50

#ifdef GITEN_COMPAT
// The display refresh the game was paced for, in passes of the main loop a second.
#define REFRESH_RATE 60
// One pass at REFRESH_RATE, in ms, rounded up.
#define FRAME_PERIOD ((1000 + REFRESH_RATE - 1) / REFRESH_RATE)
// A frame later than this restarts the clock instead of running to catch up.
#define FRAME_MAX_LAG 100

static BOOL s_frameClockStarted;
static DWORD s_frameClockStart;
static DWORD s_frameClockCount;

// @bug Retail paces the main loop, one game step per pass, on the vertical blank
// alone, so the game's speed follows the display's refresh rate: a 120 Hz
// display runs it twice as fast, and under Wine, whose WaitForVerticalBlank
// returns at once, it runs as fast as the host allows. Called before that
// wait, so the frame is still shown on the vertical blank, this holds the loop
// to REFRESH_RATE passes a second on its own clock. A deadline more than a
// frame ahead or FRAME_MAX_LAG behind (the first call, a timeGetTime wrap, a
// stall) restarts the clock at now.
static void WaitForFrame(void) {
    DWORD now;
    LONG ahead;

    if (!s_frameClockStarted) {
        // Sleep otherwise rounds up to the system tick, about 15.6 ms.
        timeBeginPeriod(1);
        s_frameClockStarted = true;
    }
    now = timeGetTime();
    if (s_frameClockCount == REFRESH_RATE) {
        s_frameClockStart += 1000;
        s_frameClockCount = 0;
    }
    ahead = static_cast<LONG>(s_frameClockStart + s_frameClockCount * 1000 / REFRESH_RATE - now);
    if (ahead > FRAME_PERIOD || ahead < -FRAME_MAX_LAG) {
        s_frameClockStart = now;
        s_frameClockCount = 0;
    } else if (ahead > 0) {
        Sleep(ahead);
    }
    s_frameClockCount++;
}

#endif

// One frame: restores lost surfaces, steps the fade and runs the render mode's
// handler (drawing only if the mode is static, the view changed or the frame
// interval passed), the fade and the cursor, then waits for the vertical blank
// and shows the frame (a flip on the HAL device, else a copy to the primary).
RVA(0x0004f140, 0x11f)
void RenderFrame(void) {
    BOOL draw = true;
    DWORD now;
    HRESULT result;

    if (s_directXReady && s_appActive) {
        RestoreSurfaces(true);
        StepScreenFade();
        now = timeGetTime();
        if ((g_renderMode & RENDER_MODE_MASK) && now - s_lastDrawTime < FRAME_INTERVAL
            && !s_viewChanged) {
            draw = false;
        } else {
            s_lastDrawTime = now;
        }
        s_renderModes[g_renderMode & RENDER_MODE_MASK](draw);
        if (draw && (g_fadeMode != SCREEN_FADE_NONE || s_screenCovered)) {
            DrawScreenFade();
        }
        s_viewChanged = false;
        DrawMouseCursor();
#ifdef GITEN_COMPAT
        WaitForFrame();
#endif
        g_ddraw->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);
        if (draw) {
            if (g_deviceType == D3D_DEVICE_HAL) {
                while ((result = g_primarySurface->Flip(NULL, DDFLIP_WAIT)) != DD_OK) {
                    if (result == DDERR_SURFACELOST && !RestoreSurfaces(s_appActive)) {
                        break;
                    }
                    if (result != DDERR_WASSTILLDRAWING) {
                        break;
                    }
                }
            } else {
                g_primarySurface->BltFast(0, 0, g_renderTarget, &g_windowRect, DDBLTFAST_WAIT);
            }
        }
    }
}

RVA(0x0004f260, 0x145)
void BuildQuadMesh(Mesh* mesh) {
    i32 pass;
    i32 quad;
    i32 k;

    mesh->vertices = new D3DTLVERTEX[48];
    mesh->indices = new u16[72];
    for (pass = 0; pass < 2; pass++) {
        for (quad = 0; quad < 6; quad++) {
            for (k = 0; k < 6; k++) {
                mesh->indices[mesh->indexCount] = mesh->vertexCount + s_quadIndices[k];
                mesh->indexCount++;
            }
            for (k = 0; k < 4; k++, mesh->vertexCount++) {
                mesh->vertices[mesh->vertexCount].sx = 0;
                mesh->vertices[mesh->vertexCount].sz = 0;
                mesh->vertices[mesh->vertexCount].sy = D3DVAL(s_quadY[quad * 4 + k]);
                mesh->vertices[mesh->vertexCount].color = 0xffffffff;
                mesh->vertices[mesh->vertexCount].specular = 0;
                mesh->vertices[mesh->vertexCount].tu = s_quadUV[quad * 4 + k][0];
                mesh->vertices[mesh->vertexCount].tv = s_quadUV[quad * 4 + k][1];
            }
        }
    }
}

RVA(0x0004f3b0, 0x34)
void FreeMesh(Mesh* mesh) {
    if (mesh->vertices != NULL) {
        delete[] mesh->vertices;
    }
    if (mesh->indices != NULL) {
        delete[] mesh->indices;
    }
    mesh->indexCount = 0;
    mesh->vertexCount = 0;
}

// The room texture quarters: [quarter][corner][tu, tv].
DATA(0x0006b868)
static D3DVALUE s_hardwareAtlasUV[4][4][2] = {
    {{0.502f, 0.502f}, {0.998f, 0.502f}, {0.998f, 0.998f}, {0.502f, 0.998f}},
    {{0.002f, 0.002f}, {0.498f, 0.002f}, {0.498f, 0.498f}, {0.002f, 0.498f}},
    {{0.502f, 0.002f}, {0.998f, 0.002f}, {0.502f, 0.498f}, {0.998f, 0.498f}},
    {{0.002f, 0.502f}, {0.498f, 0.502f}, {0.002f, 0.998f}, {0.498f, 0.998f}}
};

DATA(0x0006b8e8)
static D3DVALUE s_softwareAtlasUV[4][4][2] = {
    {{0.506f, 0.506f}, {0.994f, 0.506f}, {0.994f, 0.994f}, {0.506f, 0.994f}},
    {{0.006f, 0.006f}, {0.494f, 0.006f}, {0.494f, 0.494f}, {0.006f, 0.494f}},
    {{0.506f, 0.006f}, {0.994f, 0.006f}, {0.506f, 0.494f}, {0.994f, 0.494f}},
    {{0.006f, 0.506f}, {0.494f, 0.506f}, {0.006f, 0.994f}, {0.494f, 0.994f}}
};

// A room cell's four corners around its centre: {x, z}.
DATA(0x0006bdc8)
static i32 s_cellCorner[4][2] = {{-160, 160}, {160, 160}, {160, -160}, {-160, -160}};

// The two triangles of a floor cell, and of a ceiling cell (reversed winding).
DATA(0x0006bde8)
static u16 s_floorIndices[6] = {0, 1, 2, 0, 2, 3};

DATA(0x0006bdf8)
static u16 s_ceilingIndices[6] = {0, 3, 2, 0, 2, 1};

// Builds a cols x rows grid of 320-unit floor cells and, 320 units up, the
// matching ceiling cells.
RVA(0x0004f3f0, 0x270)
void BuildRoomMesh(Mesh* mesh, i32 cols, i32 rows) {
    i32 layer;
    i32 col;
    i32 row;
    i32 k;
    i32 x;
    i32 z;
    i32 x0;
    i32 z0;

    mesh->vertices = new D3DTLVERTEX[cols * rows * 8];
    mesh->indices = new u16[cols * rows * 12];
    mesh->vertexCount = 0;
    mesh->indexCount = 0;
    z0 = (rows / 2) * 320;
    for (layer = 0; layer < 2; layer++) {
        z = z0;
        for (col = 0; col < cols; col++) {
            x0 = -(cols / 2) * 320;
            x = x0;
            for (row = 0; row < rows; row++) {
                for (k = 0; k < 6; k++) {
                    if (layer == 0) {
                        mesh->indices[mesh->indexCount++] = s_floorIndices[k] + mesh->vertexCount;
                    } else {
                        mesh->indices[mesh->indexCount++] = s_ceilingIndices[k] + mesh->vertexCount;
                    }
                }
                for (k = 0; k < 4; k++, mesh->vertexCount++) {
                    mesh->vertices[mesh->vertexCount].sx = D3DVAL(s_cellCorner[k][0] + x);
                    mesh->vertices[mesh->vertexCount].sz = D3DVAL(z + s_cellCorner[k][1]);
                    mesh->vertices[mesh->vertexCount].specular = 0;
                    if (layer == 0) {
                        mesh->vertices[mesh->vertexCount].sy = 0;
                        if (g_deviceType == D3D_DEVICE_HAL) {
                            mesh->vertices[mesh->vertexCount].tu = s_hardwareAtlasUV[0][k][0];
                            mesh->vertices[mesh->vertexCount].tv = s_hardwareAtlasUV[0][k][1];
                        } else {
                            mesh->vertices[mesh->vertexCount].tu = s_softwareAtlasUV[0][k][0];
                            mesh->vertices[mesh->vertexCount].tv = s_softwareAtlasUV[0][k][1];
                        }
                    } else {
                        mesh->vertices[mesh->vertexCount].sy = 320.0f;
                        if (g_deviceType == D3D_DEVICE_HAL) {
                            mesh->vertices[mesh->vertexCount].tu = s_hardwareAtlasUV[1][k][0];
                            mesh->vertices[mesh->vertexCount].tv = s_hardwareAtlasUV[1][k][1];
                        } else {
                            mesh->vertices[mesh->vertexCount].tu = s_softwareAtlasUV[1][k][0];
                            mesh->vertices[mesh->vertexCount].tv = s_softwareAtlasUV[1][k][1];
                        }
                    }
                }
                x += 320;
            }
            z -= 320;
        }
    }
}

// Allocates the wall mesh: a quad per side of the 11x11 cells around the
// party (1936 vertices, 2904 indices).
RVA(0x0004f660, 0x7c)
void AllocWallMesh(Mesh* mesh) {
    mesh->vertices = new D3DTLVERTEX[1936];
    mesh->indices = new u16[2904];
}

// The camera's offset from its target and the billboards' axis per facing.
DATA(0x0006be08)
static i32 s_cameraOffsets[4][2] = {{0, -160}, {-160, 0}, {0, 160}, {160, 0}};

DATA(0x0006be28)
static D3DVALUE s_billboardAxes[4][2] = {{-1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, -1.0f}};

// Faces the camera the party's way: its offset, the billboard axis, the view
// transform and the compass.
RVA(0x0004f6e0, 0x91)
void ResetCamera(void) {
    g_viewDirection = GetMapPosition()->direction;
    g_cameraFrom.x = s_cameraOffsets[g_viewDirection][0];
    g_cameraFrom.z = s_cameraOffsets[g_viewDirection][1];
    g_billboardX = s_billboardAxes[g_viewDirection][0];
    g_billboardZ = s_billboardAxes[g_viewDirection][1];
    SetViewMatrix(g_viewMatrix, g_cameraFrom, g_cameraAt);
    g_d3dDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &g_viewMatrix);
    BlitImage(
        g_screenLayers[SCREEN_LAYER_NAVIGATION]->surface,
        g_compassImages[g_viewDirection],
        32,
        32
    );
}

// The walls of the 11x11 cells around the party, one quad per wall side
// (BuildRoomGeometry), rebuilt whenever the party changes cell.
DATA(0x0008fc90)
Mesh g_wallMesh;

DATA(0x0008fca0)
Mesh g_doorMesh;

// A wall quad's corners per side (north, east, south, west: two corners each,
// x then z) and the quad's two triangles.
DATA(0x0006be48)
static i32 s_wallCorners[8][2] = {
    {-161, 161},
    {161, 161},
    {161, 161},
    {161, -161},
    {161, -161},
    {-161, -161},
    {-161, -161},
    {-161, 161},
};

DATA(0x0006be88)
static u16 s_wallQuadIndices[6] = {0, 1, 3, 0, 3, 2};

// Rebuilds g_wallMesh from the wall words of the cells within ROOM_RADIUS of
// the party (wrapping around the map edge in the areas that wrap): each
// nonzero wall side except WALL_KIND_INVISIBLE_BARRIER becomes a quad textured
// with the wall quarter (kinds up to 2 and 11) or the door quarter of the atlas.
// @early-stop register selection: the wrapped-start LEAs exchange base/index
// operands; their addresses and the ordered referents agree.
RVA(0x0004f780, 0x40b)
void BuildRoomGeometry(void) {
    i32 partyX;
    i32 partyY;
    u16 width;
    u16 height;
    u16* walls;
    i16 area;
    i32 startX;
    i32 countX;
    i32 offsetX;
    i32 startY;
    i32 countY;
    i32 offsetY;
    i32 mapX;
    i32 mapY;
    i32 wrapX;
    i32 wrapY;
    i32 cellX;
    i32 cellZ;
    u16 cell;
    u16 kind;
    u32 side;
    u32 corner;
    u32 k;
    MapPosition* position;

    s_viewChanged = s_viewDirty = true;
    position = GetMapPosition();
    partyX = position->x;
    partyY = position->y;
    // the pun: this layer reads the map size as unsigned words
    GetMapSize(reinterpret_cast<i16*>(&width), reinterpret_cast<i16*>(&height));
    walls = GetWallMap();
    area = GetCurrentArea();
    if (area != AREA_WRAP_X && area != AREA_WRAP_XY) {
        if (partyX <= ROOM_RADIUS) {
            offsetX = -partyX;
            startX = 0;
            countX = width > partyX + ROOM_RADIUS + 1 ? partyX + ROOM_RADIUS + 1 : width;
        } else {
            startX = partyX - ROOM_RADIUS;
            offsetX = -ROOM_RADIUS;
            countX =
                width - partyX + ROOM_RADIUS < ROOM_SPAN ? width - partyX + ROOM_RADIUS : ROOM_SPAN;
        }
    } else {
        if (partyX <= ROOM_RADIUS) {
            startX = partyX + width - ROOM_RADIUS;
        } else {
            startX = partyX - ROOM_RADIUS;
        }
        countX = ROOM_SPAN;
        offsetX = -ROOM_RADIUS;
    }
    if (area == AREA_WRAP_XY) {
        if (partyY <= ROOM_RADIUS) {
            startY = partyY + height - ROOM_RADIUS;
        } else {
            startY = partyY - ROOM_RADIUS;
        }
        countY = ROOM_SPAN;
        offsetY = ROOM_RADIUS;
    } else if (partyY <= ROOM_RADIUS) {
        startY = 0;
        offsetY = partyY;
        countY = height > partyY + ROOM_RADIUS + 1 ? partyY + ROOM_RADIUS + 1 : height;
    } else {
        startY = partyY - ROOM_RADIUS;
        offsetY = ROOM_RADIUS;
        countY =
            height - partyY + ROOM_RADIUS < ROOM_SPAN ? height - partyY + ROOM_RADIUS : ROOM_SPAN;
    }
    g_wallMesh.indexCount = 0;
    g_wallMesh.vertexCount = 0;
    for (mapY = startY, cellZ = offsetY * CELL_SIZE; mapY < startY + countY;
         mapY++, cellZ -= CELL_SIZE) {
        for (mapX = startX, cellX = offsetX * CELL_SIZE; mapX < startX + countX;
             mapX++, cellX += CELL_SIZE) {
            wrapY = mapY;
            if (mapY >= height) {
                wrapY -= height;
            }
            wrapX = mapX;
            if (mapX >= width) {
                wrapX -= width;
            }
            cell = walls[wrapY * width + wrapX];
            for (side = 0; side < 8; side += 2) {
                kind = cell & 0xf;
                if (kind > WALL_KIND_NONE && kind != WALL_KIND_INVISIBLE_BARRIER) {
                    for (k = 0; k < 6; k++) {
                        g_wallMesh.indices[g_wallMesh.indexCount++] =
                            g_wallMesh.vertexCount + s_wallQuadIndices[k];
                    }
                    for (corner = 0; corner < 4; corner++, g_wallMesh.vertexCount++) {
                        g_wallMesh.vertices[g_wallMesh.vertexCount].sx =
                            D3DVAL(s_wallCorners[(corner & 1) + side][0] + cellX);
                        g_wallMesh.vertices[g_wallMesh.vertexCount].sz =
                            D3DVAL(s_wallCorners[(corner & 1) + side][1] + cellZ);
                        g_wallMesh.vertices[g_wallMesh.vertexCount].sy =
                            corner < 2 ? WALL_TOP : 0.0f;
                        g_wallMesh.vertices[g_wallMesh.vertexCount].specular = 0;
                        if (kind > WALL_KIND_PLAIN_ATLAS_LAST && kind != WALL_KIND_UNBARRED_DOOR) {
                            if (g_deviceType == D3D_DEVICE_HAL) {
                                g_wallMesh.vertices[g_wallMesh.vertexCount].tu =
                                    s_hardwareAtlasUV[2][corner][0];
                                g_wallMesh.vertices[g_wallMesh.vertexCount].tv =
                                    s_hardwareAtlasUV[2][corner][1];
                            } else {
                                g_wallMesh.vertices[g_wallMesh.vertexCount].tu =
                                    s_softwareAtlasUV[2][corner][0];
                                g_wallMesh.vertices[g_wallMesh.vertexCount].tv =
                                    s_softwareAtlasUV[2][corner][1];
                            }
                        } else if (g_deviceType == D3D_DEVICE_HAL) {
                            g_wallMesh.vertices[g_wallMesh.vertexCount].tu =
                                s_hardwareAtlasUV[3][corner][0];
                            g_wallMesh.vertices[g_wallMesh.vertexCount].tv =
                                s_hardwareAtlasUV[3][corner][1];
                        } else {
                            g_wallMesh.vertices[g_wallMesh.vertexCount].tu =
                                s_softwareAtlasUV[3][corner][0];
                            g_wallMesh.vertices[g_wallMesh.vertexCount].tv =
                                s_softwareAtlasUV[3][corner][1];
                        }
                    }
                }
                cell >>= 4;
            }
        }
    }
    UpdateAreaPalette();
}

// The wall code (0..15) on `side` of map cell (x, y) of a map `width` cells
// wide: one nibble of the cell's word per side.
RVA(0x0004fb90, 0x70)
i32 GetWallCode(i32 x, i32 y, i32 side, i32 width, i32 height) {
    i32 wall;
    u16 cell = GetWallMap()[y * width + x];

    switch (side % 4) {
        case VIEW_NORTH:
            wall = cell & 0xf;
            break;
        case VIEW_EAST:
            wall = (cell >> 4) & 0xf;
            break;
        case VIEW_SOUTH:
            wall = (cell >> 8) & 0xf;
            break;
        case VIEW_WEST:
            wall = cell >> 12;
            break;
    }
    return wall;
}

// The pad button held down (PAD_FORWARD..PAD_RIGHT; PAD_RELEASED once let go).
// @identity-TODO: set by the layer TU's mouse handling (0x4554e5).
DATA(0x00090a64)
GZ_ENUM_STORAGE(NavPadButton, i32) g_heldPadButton;

// Repeats the held pad button's move while the 3D view shows and nothing
// covers it: steps, or with `turn` the turns (forward still steps).
RVA(0x0004fc00, 0xd0)
void RepeatPadMove(BOOL turn) {
    if (g_heldPadButton && g_renderMode == RENDER_MODE_VIEW && !s_screenSaved
        && !g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
        if (turn) {
            switch (g_heldPadButton) {
                case PAD_RELEASED:
                    break;
                case PAD_FORWARD:
                    MoveForwardCommand(true);
                    break;
                case PAD_BACK:
                    TurnAroundCommand(true);
                    break;
                case PAD_LEFT:
                    TurnLeftCommand(true);
                    break;
                case PAD_RIGHT:
                    TurnRightCommand(true);
                    break;
            }
        } else {
            switch (g_heldPadButton) {
                case PAD_RELEASED:
                    break;
                case PAD_FORWARD:
                    MoveForwardCommand(true);
                    break;
                case PAD_BACK:
                    MoveBackCommand(true);
                    break;
                case PAD_LEFT:
                    MoveLeftCommand(true);
                    break;
                case PAD_RIGHT:
                    MoveRightCommand(true);
                    break;
            }
        }
    }
}

// The image each layer slot is painted with (0 for the slots painted
// otherwise).
DATA(0x0006b4e8)
u16 g_layerImages[16] = {
    IDB_BITMAP68,
    0,
    0,
    IDB_BITMAP1,
    IDB_BITMAP2,
    IDB_BITMAP3,
    IDB_BITMAP4,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    IDB_BITMAP11,
    0
};

// The party panels' images by member state.
DATA(0x0006b508)
u16 g_panelImages[8] =
    {IDB_BITMAP326, IDB_BITMAP327, IDB_BITMAP329, IDB_BITMAP328, IDB_BITMAP330, 0, 0, 0};

// The command images of the character panel (layer 1), up and pressed.
DATA(0x0006b538)
MenuButtonImages g_commandImages[10] = {
    {IDB_BITMAP33, IDB_BITMAP34},
    {IDB_BITMAP35, IDB_BITMAP36},
    {IDB_BITMAP39, IDB_BITMAP40},
    {IDB_BITMAP37, IDB_BITMAP38},
    {IDB_BITMAP31, IDB_BITMAP32},
    {IDB_BITMAP43, IDB_BITMAP44},
    {IDB_BITMAP29, IDB_BITMAP30},
    {IDB_BITMAP41, IDB_BITMAP42},
    {IDB_BITMAP56, IDB_BITMAP57},
    {0, 0},
};

// The icon layer's (slot 2) images.
DATA(0x0006b560)
u16 g_iconLayerImages[28] = {
    IDB_BITMAP9,   IDB_BITMAP301, IDB_BITMAP302, IDB_BITMAP303, IDB_BITMAP304, IDB_BITMAP305,
    IDB_BITMAP306, IDB_BITMAP307, IDB_BITMAP308, IDB_BITMAP309, IDB_BITMAP310, IDB_BITMAP311,
    IDB_BITMAP312, IDB_BITMAP313, IDB_BITMAP10,  IDB_BITMAP314, IDB_BITMAP315, IDB_BITMAP316,
    IDB_BITMAP317, IDB_BITMAP318, IDB_BITMAP319, IDB_BITMAP320, IDB_BITMAP321, IDB_BITMAP322,
    IDB_BITMAP323, IDB_BITMAP324, IDB_BITMAP325, IDB_BITMAP300,
};

// The navigation pad buttons' images (up, pressed).
DATA(0x0006b5a0)
u16 g_padImages[4][2] = {
    {IDB_BITMAP20, IDB_BITMAP19},
    {IDB_BITMAP21, IDB_BITMAP22},
    {IDB_BITMAP25, IDB_BITMAP26},
    {IDB_BITMAP23, IDB_BITMAP24}
};

// @identity-TODO: the status image set roles remain unnamed.
DATA(0x0006b600)
u16 g_statusImages[10] = {
    IDB_BITMAP67,
    IDB_BITMAP62,
    IDB_BITMAP58,
    IDB_BITMAP63,
    IDB_BITMAP59,
    IDB_BITMAP64,
    IDB_BITMAP61,
    IDB_BITMAP66,
    IDB_BITMAP65,
    IDB_BITMAP60
};

// The marks DrawStatBar uses for base, bonus, empty and equipment segments.
DATA(0x0006b618)
u16 g_statBarMarkImages[5] = {IDB_BITMAP47, IDB_BITMAP46, IDB_BITMAP45, IDB_BITMAP49, IDB_BITMAP48};

// The fusion summary grid: result icons and keyed level/growth overlays.
DATA(0x0006b628)
u16 g_fusionSummaryImages[17][3] = {
    {IDB_BITMAP81, IDB_BITMAP83, IDB_BITMAP82},
    {IDB_BITMAP84, IDB_BITMAP86, IDB_BITMAP85},
    {IDB_BITMAP87, IDB_BITMAP89, IDB_BITMAP88},
    {IDB_BITMAP90, IDB_BITMAP92, IDB_BITMAP91},
    {IDB_BITMAP93, IDB_BITMAP95, IDB_BITMAP94},
    {IDB_BITMAP96, IDB_BITMAP98, IDB_BITMAP97},
    {IDB_BITMAP99, IDB_BITMAP101, IDB_BITMAP100},
    {IDB_BITMAP102, IDB_BITMAP104, IDB_BITMAP103},
    {IDB_BITMAP105, IDB_BITMAP107, IDB_BITMAP106},
    {IDB_BITMAP108, IDB_BITMAP110, IDB_BITMAP109},
    {IDB_BITMAP111, IDB_BITMAP113, IDB_BITMAP112},
    {IDB_BITMAP114, IDB_BITMAP116, IDB_BITMAP115},
    {IDB_BITMAP117, IDB_BITMAP119, IDB_BITMAP118},
    {IDB_BITMAP120, IDB_BITMAP122, IDB_BITMAP121},
    {IDB_BITMAP123, IDB_BITMAP125, IDB_BITMAP124},
    {IDB_BITMAP126, IDB_BITMAP126, IDB_BITMAP126},
    {IDB_BITMAP127, IDB_BITMAP127, IDB_BITMAP127},
};

// The text plane kinds' frame images (CreateTextPlane; 0 for none).
DATA(0x0006b5b0)
u16 g_textPlaneImages[40] = {
    IDB_BITMAP12,
    IDB_BITMAP172,
    IDB_BITMAP173,
    IDB_BITMAP174,
    IDB_BITMAP175,
    IDB_BITMAP176,
    IDB_BITMAP177,
    IDB_BITMAP178,
    IDB_BITMAP178,
    IDB_BITMAP179,
    IDB_BITMAP180,
    IDB_BITMAP178,
    IDB_BITMAP346,
    0,
    IDB_BITMAP16,
    IDB_BITMAP17,
    IDB_BITMAP13,
    IDB_BITMAP181,
    IDB_BITMAP182,
    IDB_BITMAP183,
    0,
    IDB_BITMAP184,
    0,
    0,
    0,
    IDB_BITMAP185,
    IDB_BITMAP186,
    0,
    IDB_BITMAP187,
    0,
    IDB_BITMAP18,
    IDB_BITMAP299,
    IDB_BITMAP188,
    IDB_BITMAP189,
    IDB_BITMAP190,
    IDB_BITMAP331,
    IDB_BITMAP191,
    0,
    0,
    0
};

// The automap's tile images (by tile) and mark images (by mark).
DATA(0x0006b690)
u16 g_mapTileImages[84] = {
    IDB_BITMAP272,
    IDB_BITMAP193,
    IDB_BITMAP194,
    IDB_BITMAP195,
    IDB_BITMAP196,
    IDB_BITMAP197,
    IDB_BITMAP198,
    IDB_BITMAP199,
    IDB_BITMAP200,
    IDB_BITMAP201,
    IDB_BITMAP202,
    IDB_BITMAP203,
    IDB_BITMAP204,
    IDB_BITMAP205,
    IDB_BITMAP206,
    IDB_BITMAP207,
    IDB_BITMAP208,
    IDB_BITMAP209,
    IDB_BITMAP210,
    IDB_BITMAP211,
    IDB_BITMAP212,
    IDB_BITMAP213,
    IDB_BITMAP214,
    IDB_BITMAP215,
    IDB_BITMAP216,
    IDB_BITMAP217,
    IDB_BITMAP218,
    IDB_BITMAP219,
    IDB_BITMAP220,
    IDB_BITMAP221,
    IDB_BITMAP222,
    IDB_BITMAP223,
    IDB_BITMAP224,
    IDB_BITMAP225,
    IDB_BITMAP226,
    IDB_BITMAP227,
    IDB_BITMAP228,
    IDB_BITMAP229,
    IDB_BITMAP230,
    IDB_BITMAP231,
    IDB_BITMAP232,
    IDB_BITMAP233,
    IDB_BITMAP234,
    IDB_BITMAP235,
    IDB_BITMAP236,
    IDB_BITMAP237,
    IDB_BITMAP238,
    IDB_BITMAP239,
    IDB_BITMAP240,
    IDB_BITMAP241,
    IDB_BITMAP242,
    IDB_BITMAP243,
    IDB_BITMAP244,
    IDB_BITMAP245,
    IDB_BITMAP246,
    IDB_BITMAP247,
    IDB_BITMAP248,
    IDB_BITMAP249,
    IDB_BITMAP250,
    IDB_BITMAP251,
    IDB_BITMAP252,
    IDB_BITMAP253,
    IDB_BITMAP254,
    IDB_BITMAP255,
    IDB_BITMAP256,
    IDB_BITMAP257,
    IDB_BITMAP258,
    IDB_BITMAP259,
    IDB_BITMAP260,
    IDB_BITMAP261,
    IDB_BITMAP262,
    IDB_BITMAP263,
    IDB_BITMAP264,
    IDB_BITMAP265,
    IDB_BITMAP266,
    IDB_BITMAP267,
    IDB_BITMAP268,
    IDB_BITMAP269,
    IDB_BITMAP270,
    IDB_BITMAP271,
    IDB_BITMAP192,
    0,
    0,
    0,
};

DATA(0x0006b738)
u16 g_mapMarkImages[28] = {
    IDB_BITMAP296,
    IDB_BITMAP294,
    IDB_BITMAP295,
    IDB_BITMAP293,
    IDB_BITMAP285,
    IDB_BITMAP279,
    IDB_BITMAP287,
    IDB_BITMAP280,
    IDB_BITMAP291,
    IDB_BITMAP276,
    IDB_BITMAP278,
    IDB_BITMAP284,
    IDB_BITMAP275,
    IDB_BITMAP273,
    IDB_BITMAP281,
    IDB_BITMAP277,
    IDB_BITMAP283,
    IDB_BITMAP282,
    IDB_BITMAP288,
    IDB_BITMAP286,
    IDB_BITMAP292,
    IDB_BITMAP274,
    IDB_BITMAP290,
    IDB_BITMAP289,
    IDB_BITMAP272,
    0,
    0,
    0,
};

// The menu bar's buttons: their images up and pressed, and their left edges.
DATA(0x0006b518)
MenuButtonImages g_menuButtonImages[MENU_BUTTON_COUNT] = {
    {IDB_BITMAP72, IDB_BITMAP71},
    {IDB_BITMAP76, IDB_BITMAP75},
    {IDB_BITMAP74, IDB_BITMAP73},
    {IDB_BITMAP78, IDB_BITMAP77},
    {IDB_BITMAP80, IDB_BITMAP79},
    {IDB_BITMAP297, IDB_BITMAP298},
    {IDB_BITMAP70, IDB_BITMAP69},
};

DATA(0x0006b770)
u32 g_menuButtonX[MENU_BUTTON_COUNT] = {8, 40, 72, 104, 136, 168, 200};

// A click at (x, y) on the menu bar: draws the button pressed, then toggles
// the layer of buttons 0..4 (in the panel mode, button 4 only hides its
// layer), or opens the automap (5) or the field menu (6), which close the
// bar. Returns the button's layer (button + 3), or 0 off the buttons.
RVA(0x0004fcd0, 0x11c)
i32 ClickMenuBar(u32 x, u32 y) {
    i32 button;

    if (y < MENU_BAR_TOP || y >= MENU_BAR_BOTTOM) {
        return 0;
    }
    for (button = 0; button < MENU_BUTTON_COUNT; button++) {
        if (x >= g_menuButtonX[button] && x < g_menuButtonX[button] + MENU_BUTTON_WIDTH) {
            BlitImage(
                g_screenLayers[SCREEN_LAYER_MENU_BAR]->surface,
                g_menuButtonImages[button].pressed,
                g_menuButtonX[button],
                MENU_BAR_TOP
            );
            if (button >= 0 && button < MENU_BUTTON_AUTOMAP) {
                if (g_renderMode == RENDER_MODE_PANEL
                    && button + MENU_LAYER_OFFSET == SCREEN_LAYER_NAVIGATION) {
                    g_screenLayers[SCREEN_LAYER_NAVIGATION]->visible = false;
                } else {
                    g_screenLayers[button + MENU_LAYER_OFFSET]->visible =
                        !g_screenLayers[button + MENU_LAYER_OFFSET]->visible;
                }
                return button + MENU_LAYER_OFFSET;
            }
            if (button == MENU_BUTTON_AUTOMAP) {
                OpenAutomap();
                BlitImage(
                    g_screenLayers[SCREEN_LAYER_MENU_BAR]->surface,
                    IDB_BITMAP297,
                    g_menuButtonX[MENU_BUTTON_AUTOMAP],
                    MENU_BAR_TOP
                );
                g_screenLayers[SCREEN_LAYER_MENU_BAR]->visible = false;
            } else if (button == MENU_BUTTON_FIELD_MENU) {
                OpenFieldMenu();
                BlitImage(
                    g_screenLayers[SCREEN_LAYER_MENU_BAR]->surface,
                    IDB_BITMAP70,
                    g_menuButtonX[MENU_BUTTON_FIELD_MENU],
                    MENU_BAR_TOP
                );
                g_screenLayers[SCREEN_LAYER_MENU_BAR]->visible = false;
            }
            return button + MENU_LAYER_OFFSET;
        }
    }
    return 0;
}

// Reloads every texture whose surface was lost.
RVA(0x0004fdf0, 0x95)
void RestoreTextures(void) {
    Texture* texture;
    int i;
    int j;

    RestoreTexture(&g_textBoxTexture);
    RestoreTexture(&g_roomTexture);
    texture = g_enemyTextures[0];
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 5; j++) {
            RestoreTexture(texture);
            texture++;
        }
    }
    texture = g_objectTextures;
    for (i = 0; i < 6; i++) {
        RestoreTexture(texture);
        texture++;
    }
    RestoreTexture(&g_npcTexture);
    RestoreTexture(&g_stairsUpTexture);
    RestoreTexture(&g_stairsDownTexture);
    RestoreTexture(&g_darkWallTexture);
}

// The game window's procedure: Escape and the close command ask before
// quitting, the screen saver and the key menu are refused, the cursor is
// hidden, and losing or regaining activation releases or re-confines the
// cursor (and reloads lost textures).
RVA(0x0004fe90, 0x1ff)
LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_DESTROY:
            ReleaseGraphics();
            RestoreCursorClip(&g_savedClipRect);
            PostQuitMessage(0);
            break;
        case WM_ACTIVATEAPP:
            s_appActive = wparam;
            AcquireInput(s_appActive);
            if (s_appActive) {
                if (g_savedClipRect.right != 0) {
                    RestoreCursorClip(&g_windowRect);
                }
                RestoreTextures();
            } else {
                RestoreCursorClip(&g_savedClipRect);
            }
            break;
        case WM_SETCURSOR:
            SetCursor(NULL);
            break;
        case WM_KEYDOWN:
            if (wparam == VK_ESCAPE) {
                RestoreCursorClip(&g_savedClipRect);
                ClearScreenSurfaces();
                g_ddraw->FlipToGDISurface();
                if (MessageBox(
                        g_mainWindow,
                        "\217\111\227\271\202\265\202\334\202\267\202\251\201\110" /* 終了しますか？ */
                        ,
                        "DDSWIN",
                        MB_YESNO | MB_DEFBUTTON2 | MB_SYSTEMMODAL
                    )
                    != IDYES) {
                    RestoreCursorClip(&g_windowRect);
                } else {
                    DestroyWindow(window);
                }
            }
            break;
        case WM_SYSCOMMAND:
            if (wparam == SC_SCREENSAVE) {
                return 0;
            }
            if (wparam == SC_KEYMENU) {
                return 0;
            }
            if (wparam == SC_CLOSE) {
                RestoreCursorClip(&g_savedClipRect);
                ClearScreenSurfaces();
                g_ddraw->FlipToGDISurface();
                if (MessageBox(
                        g_mainWindow,
                        "\217\111\227\271\202\265\202\334\202\267\202\251\201\110" /* 終了しますか？ */
                        ,
                        "DDSWIN",
                        MB_YESNO | MB_DEFBUTTON2 | MB_SYSTEMMODAL
                    )
                    != IDYES) {
                    RestoreCursorClip(&g_windowRect);
                    return 0;
                }
            }
            break;
    }
    return DefWindowProc(window, message, wparam, lparam);
}

// Resets the display state before DirectX comes up: the window and the view
// clear rectangle to 640x480, the clear-blit parameters, and seeds rand.
RVA(0x00050090, 0x72)
void ResetDisplayGlobals(void) {
    s_directXReady = false;
    s_viewDirty = false;
    g_windowRect.left = 0;
    g_windowRect.top = 0;
    g_viewClearRect.x1 = 0;
    g_viewClearRect.y1 = 0;
    s_appActive = true;
    g_viewClearRect.x2 = g_windowRect.right = 640;
    g_viewClearRect.y2 = g_windowRect.bottom = 480;
    srand(timeGetTime());
    ZeroMemory(&g_clearBltFx, sizeof(g_clearBltFx));
    g_clearBltFx.dwSize = sizeof(g_clearBltFx);
}

// Registers the window class and creates the game window.
RVA(0x00050110, 0xd0)
b32 CreateMainWindow(HINSTANCE instance) {
    WNDCLASS windowClass;
    LONG width;
    LONG height;

    ResetDisplayGlobals();
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 4;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIcon(instance, IDI_APPLICATION);
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = GetStockBrush(BLACK_BRUSH);
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = "CLASSSDDSWIN";
    if (!RegisterClass(&windowClass)) {
        return false;
    }
    width = g_windowRect.right;
    height = g_windowRect.bottom;
    g_mainWindow = CreateWindowEx(
        0,
        "CLASSSDDSWIN",
        "DDSWIN",
        WS_POPUP | WS_VISIBLE | WS_SYSMENU,
        0,
        0,
        width,
        height,
        NULL,
        NULL,
        instance,
        NULL
    );
    if (g_mainWindow == NULL) {
        return false;
    }
    g_ime.Enable(g_mainWindow, false);
    return true;
}

// The cursor's screen position, read every frame.
DATA(0x0008d828)
POINT g_cursorPos;

// Reads the mouse buttons and position and the joystick for this frame;
// returns PollMouseButtons' bits.
RVA(0x000501e0, 0x3c)
GZ_ENUM_RETURN(MouseButtonBits, u8) PollInput(void) {
    GZ_ENUM_LOCAL(MouseButtonBits, u8) buttons = PollMouseButtons();

    GetCursorPos(&g_cursorPos);
    // the pun: this layer's prototype of SetMouseState takes the buttons as a
    // byte and the position as LONGs (0x450203 pushes the byte's dword slot).
    reinterpret_cast<void (*)(GZ_ENUM_PARAM(MouseButtonBits, u8), LONG, LONG)>(SetMouseState)(
        buttons,
        g_cursorPos.x,
        g_cursorPos.y
    );
    PollJoystick();
    return buttons;
}

// The move commands: a step starts only if StepParty lets the party through
// (a door step opens the door first); a turn only when no object stands in
// the party's cell.
RVA(0x00050220, 0x5d)
b32 MoveForwardCommand(i16 nextPhase) {
    b32 moved = false;
    GZ_ENUM_LOCAL(PartyStepResult, i16) step = StepParty(MOVE_FORWARD);

    if (step) {
        UpdateAreaPalette();
        UpdateViewPalette();
        if (nextPhase) {
            NextGamePhase();
        }
        g_viewDirection = GetMapPosition()->direction;
        g_moveState = step == STEP_WALK ? MOVE_STATE_STEP : MOVE_STATE_DOOR_AHEAD;
        moved = true;
        PressPadButton(PAD_FORWARD, true);
    }
    return moved;
}

RVA(0x00050280, 0x45)
b32 TurnAroundCommand(i16 nextPhase) {
    b32 moved = false;

    if (FindObjectAtParty() < 0) {
        if (nextPhase) {
            NextGamePhase();
        }
        g_viewDirection = GetMapPosition()->direction;
        moved = true;
        g_moveState = MOVE_STATE_TURN_AROUND;
        PressPadButton(PAD_BACK, true);
    }
    return moved;
}

RVA(0x000502d0, 0x61)
b32 MoveBackCommand(i16 nextPhase) {
    b32 moved = false;
    GZ_ENUM_LOCAL(PartyStepResult, i16) step = StepParty(MOVE_BACK);

    if (step) {
        UpdateAreaPalette();
        UpdateViewPalette();
        if (nextPhase) {
            NextGamePhase();
        }
        g_viewDirection = GetMapPosition()->direction;
        g_moveState = step == STEP_WALK ? MOVE_STATE_BACK : MOVE_STATE_DOOR_BACK;
        moved = true;
        PressPadButton(PAD_BACK, true);
    }
    return moved;
}

RVA(0x00050340, 0x45)
b32 TurnLeftCommand(i16 nextPhase) {
    b32 moved = false;

    if (FindObjectAtParty() < 0) {
        if (nextPhase) {
            NextGamePhase();
        }
        g_viewDirection = GetMapPosition()->direction;
        moved = true;
        g_moveState = MOVE_STATE_TURN_LEFT;
        PressPadButton(PAD_LEFT, true);
    }
    return moved;
}

RVA(0x00050390, 0x61)
b32 MoveLeftCommand(i16 nextPhase) {
    b32 moved = false;
    GZ_ENUM_LOCAL(PartyStepResult, i16) step = StepParty(MOVE_LEFT);

    if (step) {
        UpdateAreaPalette();
        UpdateViewPalette();
        if (nextPhase) {
            NextGamePhase();
        }
        g_viewDirection = GetMapPosition()->direction;
        g_moveState = step == STEP_WALK ? MOVE_STATE_LEFT : MOVE_STATE_DOOR_LEFT;
        moved = true;
        PressPadButton(PAD_LEFT, true);
    }
    return moved;
}

RVA(0x00050400, 0x45)
b32 TurnRightCommand(i16 nextPhase) {
    b32 moved = false;

    if (FindObjectAtParty() < 0) {
        if (nextPhase) {
            NextGamePhase();
        }
        g_viewDirection = GetMapPosition()->direction;
        moved = true;
        g_moveState = MOVE_STATE_TURN_RIGHT;
        PressPadButton(PAD_RIGHT, true);
    }
    return moved;
}

RVA(0x00050450, 0x61)
b32 MoveRightCommand(i16 nextPhase) {
    b32 moved = false;
    GZ_ENUM_LOCAL(PartyStepResult, i16) step = StepParty(MOVE_RIGHT);

    if (step) {
        UpdateAreaPalette();
        UpdateViewPalette();
        if (nextPhase) {
            NextGamePhase();
        }
        g_viewDirection = GetMapPosition()->direction;
        g_moveState = step == STEP_WALK ? MOVE_STATE_RIGHT : MOVE_STATE_DOOR_RIGHT;
        moved = true;
        PressPadButton(PAD_RIGHT, true);
    }
    return moved;
}

// Starts the joystick's move: up steps forward; down, left and right step
// with its third button held, else turn. Returns whether a move started.
RVA(0x000504c0, 0x71)
b32 RunJoystickMove(void) {
    BOOL moved;

    if (!s_screenSaved) {
        if (s_joystickBits & JOY_UP) {
            moved = MoveForwardCommand(true);
        } else if (s_joystickBits & JOY_DOWN) {
            if (s_joystickBits & JOY_SIDESTEP) {
                moved = MoveBackCommand(true);
            } else {
                moved = TurnAroundCommand(true);
            }
        } else if (s_joystickBits & JOY_LEFT) {
            if (s_joystickBits & JOY_SIDESTEP) {
                moved = MoveLeftCommand(true);
            } else {
                moved = TurnLeftCommand(true);
            }
        } else if (s_joystickBits & JOY_RIGHT) {
            if (s_joystickBits & JOY_SIDESTEP) {
                moved = MoveRightCommand(true);
            } else {
                moved = TurnRightCommand(true);
            }
        } else {
            return false;
        }
        if (moved) {
            return true;
        }
    }
    return false;
}

// The frames the input waits once TickFrameCount starts counting (unless
// AllowImmediateInput lets the next frame through); a click after them
// releases the graphics and quits.
#define INPUT_DELAY_FRAMES 60

// The menu bar shows while the cursor is at the top of the screen and hides
// once it is below this.
#define MENU_BAR_HIDE_Y 32

// The layer the left and the right button went down on (SCREEN_LAYER_NONE
// for none).
DATA(0x0006be94)
static GZ_ENUM_STORAGE(ScreenLayerSlot, i32) s_pressedLayer = SCREEN_LAYER_NONE;

DATA(0x0006be98)
static GZ_ENUM_STORAGE(ScreenLayerSlot, i32) s_rightPressedLayer = SCREEN_LAYER_NONE;

// Set while a navigation pad button is held down.
DATA(0x0008f608)
static b32 s_padHeld;

// Set once the dragged layer has moved since the button went down.
DATA(0x00090ab4)
static b32 s_dragMoved;

// The menu bar button's layer (ClickMenuBar) or the character panel's
// command result (ClickPanelCommand) of the last left click.
DATA(0x00090ae4)
static i32 s_clickedButton;

// The dragged layer's screen rectangle.
DATA(0x0008f560)
RECT g_dragRect;

#ifdef GITEN_BUGFIX
// The joystick's direction bits.
#define JOY_DIRECTIONS (JOY_UP | JOY_DOWN | JOY_LEFT | JOY_RIGHT)

// A left click released in place on a party panel in battle, held until a
// pass on which a tick passes (SCREEN_LAYER_NONE for none).
static GZ_ENUM_STORAGE(ScreenLayerSlot, i32) s_heldPanelRelease = SCREEN_LAYER_NONE;

// Set when the left button went down while no command was being entered.
static b32 s_pressedOutsidePick;

// The joystick bits of a battle pass the move gate turned away (0 for none).
static u32 s_heldJoystickBits;

// Whether this pass's TickGameClock (StepGame, before HandleInput) passed a
// tick: it reloads the frame countdown on one.
static b32 ClockTickedThisPass(void) {
    return g_clock.frames == g_clock.framesPerTick;
}

// Whether input held for a later tick pass may still stand: the battle goes on
// in the 3D view, no command is being entered and the command panel is not
// shown.
static b32 CanHoldBattleInput(void) {
    return GetFieldBattleActive() && GetPickMode() == 0 && g_renderMode == RENDER_MODE_VIEW
           && !g_screenLayers[SCREEN_LAYER_PANEL]->visible;
}

// Whether a left press on a party panel would start its drag now: HandleInput
// refuses one while the command panel or any text plane is shown.
static b32 CanPressPartyPanel(void) {
    i32 i;

    if (g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
        return false;
    }
    for (i = 0; i < TEXT_PLANE_COUNT; i++) {
        if (GetTextPlane(i)->visible) {
            return false;
        }
    }
    return true;
}

// Opens the held party panel release's command panel on the first pass a tick
// passes, through ReleasePartyPanel's gate as retail would have on that pass.
// Drops it when a check that let it be held or its press through fails.
static void ApplyHeldPanelRelease(void) {
    GZ_ENUM_LOCAL(ScreenLayerSlot, i32) slot = s_heldPanelRelease;

    if (slot == SCREEN_LAYER_NONE) {
        return;
    }
    if (!CanHoldBattleInput() || !CanPressPartyPanel()) {
        s_heldPanelRelease = SCREEN_LAYER_NONE;
        return;
    }
    if (!ClockTickedThisPass()) {
        return;
    }
    s_heldPanelRelease = SCREEN_LAYER_NONE;
    ReleasePartyPanel(slot, false);
}

// @bug In battle ReleasePartyPanel opens a member's command panel only when
// GetTickElapsed is set, and StepGame sets it only on the one pass in
// framesPerTick (5) on which the game clock ticks. The release is a one-pass
// event, so about four clicks in five on a party panel do nothing. The gate
// also refuses the click while the command machine has stopped the clock
// (RunPartyCommandInput clears GetTickElapsed while it runs): removing it
// lets a click that picks an ally as a target open that ally's panel in the
// middle of the pick. Retail lets that through too on a tick pass after the
// pick completes, since the pick is taken on the press and the machine is
// idle again by the release.
// A release in place in the battle view is kept only when neither its press
// nor its release came while a command was being entered, and then waits for
// the next pass on which a tick passes and goes through the retail gate there.
static void ReleasePartyPanelOnTick(GZ_ENUM_PARAM(ScreenLayerSlot, i32) slot, b32 dragged) {
    if (dragged || !GetFieldBattleActive() || g_renderMode != RENDER_MODE_VIEW) {
        ReleasePartyPanel(slot, dragged);
        return;
    }
    if (s_pressedOutsidePick && GetPickMode() == 0) {
        s_heldPanelRelease = slot;
        ApplyHeldPanelRelease();
    }
}
#endif

// Handles this frame's mouse (PollInput's bits): the joystick's move when
// no layer is in the way, the menu bar's auto-hide and the right button's
// pad turns and cancel, and the left button's menu bar, character panel,
// navigation pad and layer dragging; the 3D view's hotspots get the other
// clicks.
RVA(0x00050540, 0x580)
void HandleInput(GZ_ENUM_PARAM(MouseButtonBits, u8) buttons) {
    ScreenLayer* layer;
    b32 busy;
    i32 i;

    if (!s_immediateInput && GetFrameCount() < INPUT_DELAY_FRAMES) {
        return;
    }
#ifdef GITEN_BUGFIX
    ApplyHeldPanelRelease();
    // @bug In battle the stick (and the arrow keys standing in for it) moves
    // only on a pass with GetTickElapsed set, one in framesPerTick (5), so a
    // tap let go before the next tick is lost. A tap turned away on a pass
    // without a tick, while no command is being entered, is held and stands in
    // for the stick's bits on the next pass the gate lets through if the
    // stick is let go by then. A tick pass the gate refuses, the end of the
    // battle, command entry, the command panel and leaving the view drop it.
    if (s_layerDragging || !CanHoldBattleInput()) {
        s_heldJoystickBits = 0;
    }
    if (!s_layerDragging && g_renderMode == RENDER_MODE_VIEW
        && !g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
        if (!GetFieldBattleActive() || GetTickElapsed()) {
            if (s_heldJoystickBits != 0 && !(s_joystickBits & JOY_DIRECTIONS)) {
                s_joystickBits = s_heldJoystickBits;
            }
            s_heldJoystickBits = 0;
            if (RunJoystickMove()) {
                return;
            }
        } else if (ClockTickedThisPass()) {
            s_heldJoystickBits = 0;
        } else if (CanHoldBattleInput() && (s_joystickBits & JOY_DIRECTIONS)) {
            s_heldJoystickBits = s_joystickBits;
        }
    }
#else
    if (!s_layerDragging && g_renderMode == RENDER_MODE_VIEW
        && !g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
        if (!GetFieldBattleActive() || GetTickElapsed()) {
            if (RunJoystickMove()) {
                return;
            }
        }
    }
#endif
    busy = false;
    switch (buttons & MOUSE_STATE_MASK) {
        case MOUSE_UP:
            if (s_layerDragging) {
                s_pressedLayer = SCREEN_LAYER_NONE;
                s_layerDragging = false;
            }
            break;
        case MOUSE_HELD:
            if (s_layerDragging) {
                if (s_pressedLayer < SCREEN_LAYER_COUNT) {
                    if (g_dragRect.left != g_cursorPos.x - g_dragOffset.x
                        || g_dragRect.top != g_cursorPos.y - g_dragOffset.y) {
                        s_dragMoved = true;
                    }
                    g_dragRect.left = g_cursorPos.x - g_dragOffset.x;
                    g_dragRect.top = g_cursorPos.y - g_dragOffset.y;
                    layer = g_screenLayers[s_pressedLayer];
                    g_dragRect.right = layer->source.right + g_dragRect.left;
                    g_dragRect.bottom = layer->source.bottom + g_dragRect.top;
                }
            } else if (s_pressedLayer == SCREEN_LAYER_NAVIGATION) {
                s_padHeld = PadButtonAtPoint(g_cursorPos.x, g_cursorPos.y);
                if (s_padHeld) {
                    RepeatPadMove(true);
                }
            }
            return;
        case MOUSE_CLICK:
            if (GetFrameCount() >= INPUT_DELAY_FRAMES) {
                ReleaseGraphics();
                RestoreCursorClip(&g_savedClipRect);
                PostQuitMessage(0);
                return;
            }
#ifdef GITEN_BUGFIX
            s_heldPanelRelease = SCREEN_LAYER_NONE;
            s_pressedOutsidePick = GetPickMode() == 0;
#endif
            s_dragMoved = false;
            s_layerDragging = false;
            s_pressedLayer = LayerAtPoint(g_cursorPos.x, g_cursorPos.y);
            if (g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
                if (s_pressedLayer == SCREEN_LAYER_PANEL) {
                    s_clickedButton = ClickPanelCommand(g_cursorPos.y);
                    if (s_clickedButton > 0) {
                        g_screenLayers[SCREEN_LAYER_PANEL]->visible = false;
                        ClearPanelLayerSurface();
                    }
                }
                return;
            }
            switch (s_pressedLayer) {
                case SCREEN_LAYER_MENU_BAR:
                    s_clickedButton = ClickMenuBar(g_cursorPos.x, g_cursorPos.y);
                    return;
                case SCREEN_LAYER_PANEL:
                    if (s_clickedButton > 0) {
                        g_screenLayers[SCREEN_LAYER_PANEL]->visible = false;
                        ClearPanelLayerSurface();
                    }
                    return;
                case SCREEN_LAYER_ICON:
                    g_screenLayers[SCREEN_LAYER_MOON_PHASE]->visible =
                        !g_screenLayers[SCREEN_LAYER_MOON_PHASE]->visible;
                    return;
                case SCREEN_LAYER_NAVIGATION:
                    s_dragMoved = false;
                    s_layerDragging = false;
                    RepeatPadMove(true);
                    if (g_heldPadButton != PAD_RELEASED) {
                        s_padHeld = true;
                        return;
                    }
                    g_heldPadButton = PAD_NONE;
                case SCREEN_LAYER_LOCATION:
                case SCREEN_LAYER_CURRENCY:
                case SCREEN_LAYER_AUTOMAP:
                case SCREEN_LAYER_FIRST_PANEL:
                case SCREEN_LAYER_FIRST_PANEL + 1:
                case SCREEN_LAYER_FIRST_PANEL + 2:
                case SCREEN_LAYER_FIRST_PANEL + 3:
                case SCREEN_LAYER_FIRST_PANEL + 4:
                case SCREEN_LAYER_FIRST_PANEL + 5:
                    for (i = 0; i < TEXT_PLANE_COUNT; i++) {
                        busy |= GetTextPlane(i)->visible;
                    }
                    s_dragMoved = false;
                    s_layerDragging = false;
                    if (busy) {
                        return;
                    }
                    layer = g_screenLayers[s_pressedLayer];
                    g_dragRect.left = layer->source.left + layer->x;
                    g_dragRect.top = layer->source.top + layer->y;
                    g_dragRect.right = layer->source.right + layer->x;
                    g_dragRect.bottom = layer->source.bottom + layer->y;
                    g_dragOffset.x = g_cursorPos.x - layer->x;
                    g_dragOffset.y = g_cursorPos.y - layer->y;
                    s_layerDragging = true;
                    return;
                default:
                    ClickHotspotAt(g_cursorPos.x, g_cursorPos.y);
                    return;
            }
        case MOUSE_LET_GO:
            if (s_pressedLayer == SCREEN_LAYER_MENU_BAR) {
                if (s_clickedButton > 0) {
                    BlitImage(
                        g_screenLayers[SCREEN_LAYER_MENU_BAR]->surface,
                        g_menuButtonImages[s_clickedButton - MENU_LAYER_OFFSET].up,
                        g_menuButtonX[s_clickedButton - MENU_LAYER_OFFSET],
                        MENU_BAR_TOP
                    );
                }
            } else if (s_layerDragging && s_pressedLayer < SCREEN_LAYER_COUNT) {
                if (s_pressedLayer > SCREEN_LAYER_NONPARTY_LAST) {
#ifdef GITEN_BUGFIX
                    ReleasePartyPanelOnTick(s_pressedLayer, s_dragMoved);
#else
                    ReleasePartyPanel(s_pressedLayer, s_dragMoved);
#endif
                } else {
                    PlaceDraggedLayer(s_pressedLayer);
                }
            }
            s_pressedLayer = SCREEN_LAYER_NONE;
            s_padHeld = false;
            return;
        default:
            return;
    }
    switch (buttons >> MOUSE_RIGHT_SHIFT) {
        case MOUSE_UP:
            s_pendingKey = false;
            if (!g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
                if (g_screenLayers[SCREEN_LAYER_MENU_BAR]->visible) {
                    if (g_cursorPos.y > MENU_BAR_HIDE_Y) {
                        g_screenLayers[SCREEN_LAYER_MENU_BAR]->visible = false;
                    }
                } else if (g_cursorPos.y <= 0) {
                    g_screenLayers[SCREEN_LAYER_MENU_BAR]->visible = true;
                }
            }
            break;
        case MOUSE_HELD:
            if (GetFrameCount() >= INPUT_DELAY_FRAMES) {
                ReleaseGraphics();
                RestoreCursorClip(&g_savedClipRect);
                PostQuitMessage(0);
                return;
            }
            if (!s_dragMoved && s_rightPressedLayer == SCREEN_LAYER_NAVIGATION) {
                s_padHeld = PadButtonAtPoint(g_cursorPos.x, g_cursorPos.y);
                if (s_padHeld) {
                    RepeatPadMove(false);
                }
            }
            break;
        case MOUSE_CLICK:
            s_rightPressedLayer = LayerAtPoint(g_cursorPos.x, g_cursorPos.y);
            if (s_rightPressedLayer == SCREEN_LAYER_NAVIGATION) {
                RepeatPadMove(false);
                if (g_heldPadButton != PAD_RELEASED) {
                    s_padHeld = true;
                }
            } else {
                s_pendingKey = true;
            }
            break;
        case MOUSE_LET_GO:
            if (g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
                g_screenLayers[SCREEN_LAYER_PANEL]->visible = false;
                ClearPanelLayerSurface();
            } else {
                s_rightPressedLayer = SCREEN_LAYER_NONE;
                s_padHeld = false;
            }
            break;
    }
    return;
}

// The pictures LoadGraphics creates besides the scene, view and mode ones.
DATA(0x00090a90)
Picture g_statusPicture;

DATA(0x000847d0)
Picture g_titleMenuPicture;

DATA(0x0008d298)
Picture g_iconStagingPicture;

DATA(0x0008f318)
Picture g_cursorPicture;

DATA(0x0008f340)
Picture g_busyCursorPicture;

DATA(0x0008fb78)
D3DRECT g_viewClearRect;

DATA(0x0008fb88)
Picture g_enemyPictures[ENEMY_PICTURE_COUNT];

DATA(0x0008fc68)
Picture g_fightBannerPicture;

// The staging surface DrawStatusPortrait copies onto the status picture.
DATA(0x0008fb50)
Picture g_statusPortraitPicture;

// The left, centered and right field-message bands drawn by DrawFieldMessage.
DATA(0x0008f420)
Picture g_centerFieldMessagePicture;

DATA(0x0008f448)
Picture g_leftFieldMessagePicture;

DATA(0x0008f4b0)
Picture g_rightFieldMessagePicture;

// The cached effect BMP scaled and mirrored by the effect renderer.
DATA(0x0008d838)
Picture g_effectFramePicture;

// The full-screen backdrop BlitFieldBackground and DrawSceneOverlay blit.
DATA(0x00084c40)
Picture g_backdropPicture;

// The 640x400 picture DrawSprites composes large sprites through.
// @identity-TODO: its id word tracks the image drawn into it.
DATA(0x00090a68)
Picture g_spritePicture;

DATA(0x00084368)
Texture g_npcTexture;

DATA(0x00087790)
Texture g_stairsDownTexture;

DATA(0x00087bd0)
Texture g_darkWallTexture;

DATA(0x0008d2c0)
Texture g_stairsUpTexture;

DATA(0x0008f618)
Texture g_textBoxTexture;

// Creates the display's pictures, meshes, textures and layers: FALSE when
// the game setup, the glyph surface or a layer fails.
RVA(0x00050ac0, 0x48e)
b32 LoadGraphics(void) {
    i32 i;
    i32 layer;
    b32 failed;

    ClearHandleTable();
    CreatePicture(&g_scenePicture, 640, 480, 640, 480, true);
    if (StartGame()) {
        return false;
    }
    if (!CreateGlyphSurface()) {
        return false;
    }
    BuildRoomMesh(&g_roomMesh, ROOM_SPAN, ROOM_SPAN);
    AllocWallMesh(&g_wallMesh);
    BuildQuadMesh(&g_doorMesh);
    CreatePicture(&g_viewCachePicture, 640, 328, 640, 328, true);
    CreatePicture(&g_targetPicture, MARK_SIZE, MARK_SIZE, MARK_SIZE, MARK_SIZE, false);
    LoadPictureFile(&g_targetPicture, "w\\target.bmp");
    CreatePicture(&g_fightBannerPicture, 224, 48, 224, 48, false);
    LoadPictureFile(&g_fightBannerPicture, "w\\fight.bmp");
    CreatePicture(&g_titleMenuPicture, 128, 56, 128, 56, false);
    CreatePicture(&g_statusPortraitPicture, 256, 256, 256, 256, false);
    CreatePicture(&g_commandBarPicture, 608, 24, 608, 24, false);
    CreatePicture(&g_statusPicture, 640, 440, 640, 440, false);
    CreatePicture(&g_leftFieldMessagePicture, 176, 16, 176, 16, false);
    CreatePicture(&g_centerFieldMessagePicture, 176, 16, 176, 16, false);
    CreatePicture(&g_rightFieldMessagePicture, 176, 16, 176, 16, false);
    CreatePicture(&g_iconStagingPicture, 24, 16, 24, 16, false);
    CreatePicture(&g_backdropPicture, 640, 480, 640, 480, true);
    CreatePicture(&g_effectFramePicture, 640, 328, 640, 328, false);
    CreatePicture(&g_whitePicture, 640, 328, 640, 328, false);
    LoadPictureFile(&g_whitePicture, "w\\white.bmp");
    CreatePicture(&g_mapMarkerPicture, 6, 6, 6, 6, false);
    DrawResourceBitmap(g_mapMarkerPicture.surface, MARKER_BITMAP_FIRST);
    g_markerColors[0].color = ReadSurfaceWord(g_mapMarkerPicture.surface, 0, 0, 6);
    DrawResourceBitmap(g_mapMarkerPicture.surface, MARKER_BITMAP_FIRST + 1);
    g_markerColors[2].color = ReadSurfaceWord(g_mapMarkerPicture.surface, 0, 0, 6);
    DrawResourceBitmap(g_mapMarkerPicture.surface, MARKER_BITMAP_FIRST + 2);
    g_markerColors[3].color = ReadSurfaceWord(g_mapMarkerPicture.surface, 0, 0, 6);
    DrawResourceBitmap(g_mapMarkerPicture.surface, MARKER_BITMAP_FIRST + 3);
    g_markerColors[1].color = ReadSurfaceWord(g_mapMarkerPicture.surface, 0, 0, 6);
    DrawResourceBitmap(g_mapMarkerPicture.surface, MARKER_BITMAP_PARTY);
    CreatePicture(&g_cursorPicture, 32, 32, 32, 32, false);
    BlitImage(g_cursorPicture.surface, CURSOR_IMAGE, 0, 0);
    CreatePicture(&g_busyCursorPicture, 32, 32, 32, 32, false);
    BlitImage(g_busyCursorPicture.surface, BUSY_CURSOR_IMAGE, 0, 0);
    CreatePicture(&g_spritePicture, 640, 400, 640, 400, false);
    for (i = 0; i < ENEMY_PICTURE_COUNT; i++) {
        CreatePicture(&g_enemyPictures[i], 512, 256, 512, 256, false);
    }
    LoadTexture(&g_textBoxTexture, "w\\txrtbox.bmp", true);
    LoadTexture(&g_stairsUpTexture, "w\\up.bmp", true);
    LoadTexture(&g_stairsDownTexture, "w\\dn.bmp", true);
    LoadTexture(&g_darkWallTexture, "w\\darkwall.bmp", true);
    LoadTexture(&g_npcTexture, "w\\npc.bmp", true);
    ResetRenderMode();
    failed = false;
    for (layer = 0; layer < SCREEN_LAYER_COUNT; layer++) {
        failed |= CreateScreenLayer(static_cast<GZ_ENUM_PARAM(ScreenLayerSlot, i32)>(layer));
    }
    if (failed) {
        return false;
    }
    DrawLayerText(
        3,
        24,
        8,
        "\201^28" /* ／28 */,
        TEXT_ATTR_OPAQUE | TEXT_ATTR_FLAG1
            | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
    return true;
}

// The captions of WinMain's failure boxes: empty.
// @identity-TODO: zeroed arrays in .bss; their declared form is unrecovered.
DATA(0x00090ae8)
static char s_directXErrorCaption[4];

DATA(0x00090aec)
static char s_loadErrorCaption[4];

// The application instance (WinMain).
DATA(0x0008fb0c)
HINSTANCE g_instance;

// The application: one instance only; reads the display and device
// settings, creates the window, brings DirectX up and loads the graphics
// (reporting a failure), then runs the message loop. While active it runs a
// game frame each millisecond tick, unless a move or a fade is under way,
// and renders one.
// @identity-TODO: the two message box captions are empty zeroed arrays in
// .bss (0x490ae8, 0x490aec); their declaration is unrecovered.
RVA(0x00050f50, 0x1ad)
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show) {
    MSG message;
    DWORD nextTick;
    GZ_ENUM_LOCAL(MouseButtonBits, u8) buttons;

    if (FindWindow("CLASSSDDSWIN", "DDSWIN")) {
        return 0;
    }
    LoadDisplayConfig(&g_displayConfig);
    if (!LoadDeviceSettings(&g_deviceSettings)) {
        return 0;
    }
    g_deviceType = g_deviceSettings.caps.hardwareOnly == 1 ? D3D_DEVICE_HAL : D3D_DEVICE_MMX;
    g_instance = instance;
    if (!CreateMainWindow(instance)) {
        return 1;
    }
    ConfineCursor();
    if (!InitDirectX()) {
        ReleaseGraphics();
        MessageBox(
            NULL,
            "\202\143\202\211\202\222\202\205\202\203\202\224\202\167\217\211\212\372\211\273\202"
            "\311\216\270\224\163\202\265\202\334\202\265\202\275\201\102" /* ＤｉｒｅｃｔＸ初期化に失敗しました。 */
            ,
            s_directXErrorCaption,
            MB_ICONEXCLAMATION
        );
        return 2;
    }
    if (!LoadGraphics()) {
        ReleaseGraphics();
        MessageBox(
            NULL,
            "\217\211\212\372\211\273\202\311\216\270\224\163\202\265\202\334\202\265\202\275\201"
            "\102" /* 初期化に失敗しました。 */,
            s_loadErrorCaption,
            MB_ICONEXCLAMATION
        );
        return 3;
    }
    nextTick = 0;
    for (;;) {
#ifdef GITEN_COMPAT
        PumpMessages();
#else
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) || !s_appActive) {
            if (message.message == WM_QUIT) {
                exit(0);
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
#endif
        if (timeGetTime() > nextTick) {
            nextTick = timeGetTime() + 1;
            if (g_moveState == MOVE_STATE_NONE && g_fadeMode == SCREEN_FADE_NONE) {
                buttons = PollInput();
                if (StepGame()) {
                    ReleaseGraphics();
                    RestoreCursorClip(&g_savedClipRect);
                    PostQuitMessage(0);
                }
                HandleInput(buttons);
                s_immediateInput = false;
            }
            UpdateLayerPanels();
            RenderFrame();
        }
    }
    return message.wParam;
}

// The vector constructor iterator that this TU's new[] expressions emit.
// The value D3DTLVERTEX ctor the local vertex tables are built with.
RVA_COMPGEN(0x00051160, 0x3c, ??0_D3DTLVERTEX@@QAE@ABU_D3DVECTOR@@MKKMM@Z)

// The value D3DLVERTEX ctor.
RVA_COMPGEN(0x000511a0, 0x3c, ??0_D3DLVERTEX@@QAE@ABU_D3DVECTOR@@KKMM@Z)

RVA_COMPGEN(0x000511e0, 0x2a, ??_H@YGXPAXIHP6EX0@Z@Z)

// The empty D3DTLVERTEX ctor that new[] passes to the iterator.
RVA_COMPGEN(0x000568c0, 0x3, ??0_D3DTLVERTEX@@QAE@XZ)
