// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. The scene record the cell
// events fill, the background loader and the moon table.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/AreaMap.h>
#include <Game/Clock.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/Scene.h>
#include <Game/StateStack.h>
#include <Game/StatusDraw.h>
#include <Game/WaitState.h>
#include <Game/WorldMap.h>
#include <Gfx/Background.h>
#include <Gfx/Blit.h>
#include <Gfx/Render.h>
#include <Gfx/Scene.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/Sprite.h>
#include <Gfx/VideoState.h>
#include <Gfx/Vram.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Script/ScriptVars.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Ui/Message.h>

#include <stddef.h>

// @identity-TODO: 28 bytes per row (one per moon phase), read from data file
// 3 (kind 12); what the values are is unrecovered.
DATA(0x0007b8b8)
static u8 s_moonTableBuffer[0x280];

DATA(0x0007beac)
static u8* s_moonTable;

// The script the cell event runs (NULL: none).
DATA(0x0007b89c)
static u8* s_cellScript;

// The 16-byte record of the cell whose event runs.
DATA(0x0007bb50)
static u8 s_sceneCell[16];

DATA(0x0007b818)
i16 g_sceneFramePositions[32][2];

DATA(0x0007bb40)
u8 g_sceneVideoState[16];

DATA(0x0007bbf8)
u8 g_sceneFrameIds[32][2];

DATA(0x0007bdb8)
i32 g_sceneFrameSaves[32];

// @identity-TODO: legacy image handles; no loader writes them in this build.
DATA(0x0007bd38)
static u32 s_sceneEntries[32];

// @identity-TODO: a redraw flag and a hold word of the scene code.
DATA(0x0007be80)
static b16 s_sceneDirty;

DATA(0x0007be84)
static i16 s_sceneHold;

DATA(0x0007be78)
static i16 s_sceneScript;

DATA(0x0007be7c)
static i16 s_sceneScriptEntry;

DATA(0x0007b898)
static u8* s_sceneSpriteStream;

DATA(0x0007bea0)
static PaletteState* s_scenePaletteState;

DATA(0x0007be88)
static i16 s_sceneScreenState;

DATA(0x0007be8c)
static i16 s_sceneObjectsFrozen;

RVA(0x00017960, 0x2a)
u16 TimeUntilMoonPhase(i16 phase) {
    phase -= g_clock.moonPhase;
    if (phase <= 0) {
        phase += 28;
    }
    return phase * 1520 - g_clock.moonTicks;
}

RVA(0x00017990, 0x9)
i16 GetMoonPhase(void) {
    return g_clock.moonPhase;
}

RVA(0x000179a0, 0x27)
u32 GetClockMinutes(void) {
    return (g_clock.days * 24 + g_clock.hour) * 60 + g_clock.minute;
}

RVA(0x000179d0, 0x34)
void LoadMoonTable(void) {
    FILE* fp = OpenDataFile(3, 12, 0);
    ReadRawBlock(fp, s_moonTableBuffer);
    s_moonTable = s_moonTableBuffer;
    CloseDataFile(fp);
}

#define MoonTableAt(row, phase) (s_moonTable[(phase) + (row) * 28])

// Twice the moon table byte of `row` for the current moon phase.
RVA(0x00017a10, 0x25)
i16 GetMoonValue(i16 row) {
    return MoonTableAt(row, GetMoonPhase()) * 2;
}

// `value` scaled by the moon table byte of `row` for the current moon phase
// and by `percent`.
RVA(0x00017a40, 0x45)
i32 ScaleByMoonValue(i32 value, i16 row, i16 percent) {
    return MoonTableAt(row, g_clock.moonPhase) * percent * value / 100;
}

RVA(0x00017a90, 0x12)
i16 ExchangeSceneHold(i16 hold) {
    i16 old = s_sceneHold;
    s_sceneHold = hold;
    return old;
}

RVA(0x00017ab0, 0xa)
void MarkSceneDirty(void) {
    s_sceneDirty = true;
}

RVA(0x00017ac0, 0x43)
void SwapSceneCellParams(void) {
    u8 t;
    t = s_sceneCell[9];
    s_sceneCell[9] = s_sceneCell[0xc];
    s_sceneCell[0xc] = t;
    t = s_sceneCell[0xa];
    s_sceneCell[0xa] = s_sceneCell[0xd];
    s_sceneCell[0xd] = t;
    t = s_sceneCell[0xb];
    s_sceneCell[0xb] = s_sceneCell[0xe];
    s_sceneCell[0xe] = t;
}

RVA(0x00017b10, 0x9)
i16 GetSceneCellKind(void) {
    return s_sceneCell[5];
}

RVA(0x00017b20, 0x1e)
u32 GetSceneEntry(i16 index) {
    if (index >= 0 && index < 32) {
        return s_sceneEntries[index];
    }
    return 0;
}

RVA(0x00017b40, 0x4f)
void ShowBackground(i16 image, i16 arg) {
    ImageRequest request;
    request.file = image + 0x5000;
    request.variant = arg;
    request.flags = 0;
    LoadRequestedScenePicture(request, arg);
}

// @identity-TODO: an empty hook.
RVA(0x00017b90, 0x1)
void SceneNop(void) {}

RVA(0x00017ba0, 0xf3)
void RestoreBackground(void) {
    MapPosition position = g_field.pos;
    if (position.area == 0x82) {
        if (position.x == 12 && position.y == 11 && position.level == 8) {
            s_sceneCell[9] = 0x31;
            s_sceneCell[10] = 2;
            s_sceneCell[11] = 2;
        }
    } else if (position.area == 0x35) {
        if (position.x == 4 && position.y == 3 && position.level == 7) {
            s_sceneCell[13] = 0x31;
            s_sceneCell[14] = 3;
            s_sceneCell[15] = 2;
        }
    }
    if (!IsEventFlagSet(9, 0x7b)) {
        s_sceneCell[9] = 0x31;
        s_sceneCell[10] = 4;
        s_sceneCell[11] = 4;
    }
    if (!s_sceneDirty) {
        ShowBackground(s_sceneCell[9], s_sceneCell[10]);
    } else {
        ShowBackground(s_sceneCell[13], s_sceneCell[14]);
    }
}

RVA(0x00017ca0, 0x1)
void BackgroundParameterNop(i16 parameter, i16 mode) {}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
RVA(0x00017cb0, 0x30)
void RestoreBackgroundParameter(void) {
    if (!s_sceneDirty) {
        BackgroundParameterNop(s_sceneCell[11], 1);
    } else {
        BackgroundParameterNop(s_sceneCell[15], 1);
    }
}

RVA(0x00017ce0, 0x1c)
void SetSceneCell(const void* cell) {
    i16 i;
    for (i = 0; i < 16; i++) {
        s_sceneCell[i] = ((const u8*)cell)[i];
    }
}

RVA(0x00017d00, 0x22)
u8* GetSceneCell(u8* out) {
    i16 i;
    for (i = 0; i < 16; i++) {
        out[i] = s_sceneCell[i];
    }
    return out;
}

RVA(0x00017d30, 0xa)
void SetCellScript(u8* script) {
    s_cellScript = script;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00017d40, 0x3c)
void RestoreSceneFrame(i16 slot) {
    if (g_sceneFrameSaves[slot]) {
        RestoreSavedCursor(HandleReadPtr(g_sceneFrameSaves[slot]));
    }
    g_sceneFrameSaves[slot] = FreeHandle(g_sceneFrameSaves[slot]);
}

RVA(0x00017d80, 0xc8)
void LoadSpriteImage(i16 slot, i16 image, i16 arg) {
    ImageRequest request;
    i32 size;
    struct BmpFile* data;
    if (slot < 0) {
        return;
    }
    FreeSpriteImages(slot);
    if (image == -1) {
        return;
    }
    image += 0x4000;
    if (image == 0x401d && GetRenderMode() == 1) {
        image = 0x40fd;
    }
    request.file = image;
    request.variant = arg;
    request.flags = 0;
    if (image >= 0x2000 && image < 0x3000) {
        if ((image & 15) == 4) {
            request.file = (image & ~13) | 2;
        }
        data = LoadImageFile(&request, &size);
    } else {
        data = LoadImageVariant(&request, &size);
    }
    LoadSpriteFrames(data, slot, image, size);
    FreeImageFile(data);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00017e50, 0x95)
void DrawSceneFrame(i16 image, i16 slot, i16 frame, i16 x, i16 y, i16 mode) {
    u32 handle;
    g_sceneFrameSaves[slot] = FreeHandle(g_sceneFrameSaves[slot]);
    handle = s_sceneEntries[image];
    if (handle && frame != -1) {
        g_sceneFrameIds[slot][0] = image;
        g_sceneFrameIds[slot][1] = frame;
        g_sceneFramePositions[slot][0] = x - 40;
        g_sceneFramePositions[slot][1] = y - 112;
        g_sceneFrameSaves[slot] =
            DrawImageFrame(GetPlaneData(0), handle, frame, x - 40, y - 112, 0, 1, mode, 0);
    }
}

RVA(0x00017ef0, 0x218)
b16 RunCellScene(void) {
    switch (GetGamePhase()) {
        case 0:
            SetSceneRenderMode();
            UnplaceAllSprites();
            s_sceneScreenState = SaveScreenState();
            CloseMessageWindow();
            NextGamePhase();
            SaveVideoState(g_sceneVideoState);
            SetSubscreenActive(1);
            g_fieldRedrawRequest = 1;
            ClearMaskView();
            ResetMask(1);
            s_scenePaletteState = SavePaletteState(s_scenePaletteState, 1);
            RestoreBackground();
            LoadSceneSprites();
            PlaceSceneSprites();
            ClearTextPlane(g_infoPlane);
            LockStatusRedraw(1);
            RedrawScreen(1, 0);
            StartScreenFadeAndWait(SCREEN_FADE_FROM_BLACK, 1);
            return true;
        case 1:
            NextGamePhase();
            StartDebugScene(s_sceneScript, s_sceneScriptEntry, g_infoPlane);
            break;
        case 2:
            NextGamePhase();
            PushWaitState(WAIT_INPUT, 0xffff, 0xffff, 0);
            break;
        case 3:
            NextGamePhase();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            break;
        case 4:
            s_sceneDirty = false;
            FreeSceneSprites();
            SceneNop();
            s_scenePaletteState = RestorePaletteState(s_scenePaletteState, 1);
            RestoreVideoState(g_sceneVideoState);
            SetSubscreenActive(0);
            ReturnFromGameState();
            PlayLevelMusic();
            LockStatusRedraw(0);
            SetRebuildRoom(1);
            if (g_worldMapRequest < 0) {
                SetGameState(16);
            } else if (g_worldMapRequest > 0) {
                SetGameState(16);
            } else if (!s_sceneHold) {
                RestoreSavedPoint();
            }
            s_sceneHold = 0;
            RestoreScreenState(s_sceneScreenState);
            break;
    }
    if (g_fieldRedrawRequest) {
        RedrawScreen(0, 0);
    }
    return false;
}

RVA(0x00018110, 0x15)
void FreeSceneSprites(void) {
    i16 slot;
    for (slot = 0; slot < 32; slot++) {
        FreeSpriteImages(slot);
    }
}

RVA(0x00018130, 0x3e)
void LoadSceneSprites(void) {
    u8* cursor = s_cellScript;
    i16 slot = 31;
    if (cursor != NULL) {
        while (*cursor != 0xff) {
            i16 image = *cursor++;
            i16 variant = *cursor++;
            LoadSpriteImage(slot, image, variant);
            slot--;
        }
        cursor++;
    }
    s_sceneSpriteStream = cursor;
}

RVA(0x00018170, 0x6e)
void PlaceSceneSprites(void) {
    i16 slot = 31;
    if (s_sceneSpriteStream != NULL) {
        while (*s_sceneSpriteStream != 0xff) {
            i16 image = *s_sceneSpriteStream++ + 16;
            i16 frame = *s_sceneSpriteStream++;
            i16 x = *s_sceneSpriteStream;
            s_sceneSpriteStream += 2;
            if (IsSpriteFrameLoaded(image, frame)) {
                PlaceSprite(image, slot, frame, x + 40, 0);
            }
            slot--;
        }
    }
}

RVA(0x000181e0, 0x18)
void SetSceneScript(i16 script, i16 entry) {
    s_sceneScript = script;
    s_sceneScriptEntry = entry;
}

RVA(0x00018200, 0x25)
void SetSceneScriptByIndex(i16 scriptIndex, i16 entryIndex) {
    SetSceneScript(s_sceneCell[scriptIndex], s_sceneCell[entryIndex]);
}

RVA(0x00018230, 0x10e)
b16 RunFieldTextScene(void) {
    switch (GetGamePhase()) {
        case 1:
            s_sceneDirty = false;
            FreeSceneSprites();
            ReturnFromGameState();
            LockStatusRedraw(0);
            RefreshStatusPanel(1);
            RepaintTextPlane(g_infoPlane, 3);
            g_fieldRedrawRequest = 1;
            if (g_worldMapRequest < 0) {
                SetGameState(16);
                SetGamePhase(8);
            } else if (g_worldMapRequest > 0) {
                SetGameState(16);
            }
            s_sceneHold = 0;
            RestoreScreenState(s_sceneScreenState);
            RedrawFieldView();
            break;
        case 0:
            s_sceneScreenState = SaveScreenState();
            NextGamePhase();
            PrepareFieldRedraw(1);
            ClearTextPlane(g_infoPlane);
            LockStatusRedraw(1);
            RepaintTextPlane(g_infoPlane, 3);
            StartDebugScene(s_sceneScript, s_sceneScriptEntry, g_infoPlane);
            break;
    }
    return false;
}

RVA(0x00018340, 0x157)
b16 RunFrozenFieldScene(void) {
    switch (GetGamePhase()) {
        case 1:
            s_sceneDirty = false;
            FreeSceneSprites();
            ReturnFromGameState();
            ExchangeObjectsFrozen(s_sceneObjectsFrozen);
            LockStatusRedraw(0);
            s_scenePaletteState = RestorePaletteState(s_scenePaletteState, 1);
            RefreshStatusPanel(1);
            g_fieldRedrawRequest = 1;
            if (g_worldMapRequest < 0 || g_worldMapRequest > 0) {
                SetGameState(16);
                StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            }
            s_sceneHold = 0;
            RestoreScreenState(s_sceneScreenState);
            RedrawFieldView();
            break;
        case 0:
            s_sceneObjectsFrozen = ExchangeObjectsFrozen(1);
            s_sceneScreenState = SaveScreenState();
            NextGamePhase();
            s_scenePaletteState = SavePaletteState(s_scenePaletteState, 3);
            PrepareFieldRedraw(1);
            ClearTextPlane(g_infoPlane);
            LockStatusRedraw(1);
            StartDebugScene(s_sceneScript, s_sceneScriptEntry, g_infoPlane);
            g_fieldRedrawRequest = 1;
            RedrawFieldView();
            break;
    }
    return UpdateFieldScreen(0);
}

// @identity-TODO: the role of the fixed scene script is unrecovered.
RVA(0x000184a0, 0xe8)
b16 RunPictureTransition(void) {
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            LockStatusRedraw(1);
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            break;
        case 1:
            NextGamePhase();
            StartScreenFade(SCREEN_FADE_FROM_BLACK, 1);
            ShowScenePicture();
            PushWaitState(WAIT_FADE, 0, 0, -1);
            break;
        case 2:
            NextGamePhase();
            StartDebugScene(0x2d, 0, 0);
            break;
        case 3:
            s_sceneDirty = false;
            FreeSceneSprites();
            LockStatusRedraw(0);
            s_scenePaletteState = RestorePaletteState(s_scenePaletteState, 1);
            SetGameState(16);
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            s_sceneHold = 0;
            break;
    }
    return false;
}

RVA(0x00018590, 0x19c)
b16 RunBackgroundScene(void) {
    switch (GetGamePhase()) {
        case 0:
            ClearSceneSurfaces();
            s_sceneScreenState = SaveScreenState();
            NextGamePhase();
            SaveVideoState(g_sceneVideoState);
            g_fieldRedrawRequest = 1;
            ClearMaskView();
            ResetMask(1);
            s_scenePaletteState = SavePaletteState(s_scenePaletteState, 1);
            RestoreBackground();
            LockStatusRedraw(1);
            ClearTextPlane(g_infoPlane);
            SetInfoBarLayout(0);
            RedrawScreen(0, 1);
            StartScreenFadeAndWait(SCREEN_FADE_FROM_BLACK, 1);
            return true;
        case 1:
            NextGamePhase();
            StartDebugScene(s_sceneScript, s_sceneScriptEntry, g_infoPlane);
            break;
        case 2:
            NextGamePhase();
            PushWaitState(WAIT_INPUT, 0xffff, 0xffff, 0);
            break;
        case 3:
            NextGamePhase();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            break;
        case 4:
            s_sceneDirty = false;
            FreeSceneSprites();
            SceneNop();
            s_scenePaletteState = RestorePaletteState(s_scenePaletteState, 1);
            RestoreVideoState(g_sceneVideoState);
            ReturnFromGameState();
            LockStatusRedraw(0);
            s_sceneHold = 0;
            RestoreScreenState(s_sceneScreenState);
            break;
    }
    return false;
}
