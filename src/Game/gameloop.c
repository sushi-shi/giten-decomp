// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Attack.h>
#include <Game/AutomapData.h>
#include <Game/BagItems.h>
#include <Game/Clock.h>
#include <Game/DemonTable.h>
#include <Game/DropTable.h>
#include <Game/Familiarity.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSupport.h>
#include <Game/GameLoop.h>
#include <Game/Growth.h>
#include <Game/ItemRecord.h>
#include <Game/Party.h>
#include <Game/PlayTime.h>
#include <Game/Skill.h>
#include <Game/StateStack.h>
#include <Game/WorldMap.h>
#include <Gfx/Motion.h>
#include <Gfx/Shot.h>
#include <Gfx/Vram.h>
#include <Input/Mouse.h>
#include <Script/EventFlags.h>
#include <Script/ScriptVars.h>
#include <Sound/Sound.h>
#include <Text/Font.h>
#include <Text/TextWindow.h>
#include <Ui/Message.h>

#include <string.h>

// @identity-TODO: the owner of the script variables is unproven; the game
// start-up path here is the first code in link order that touches them.
DATA(0x00091580)
i32 g_scriptVars[256];

// The cell event's latch (declared in <Game/AreaMap.h>). Its .bss run lies
// between blit's and gameloop's statics, apart from the area map's object.
// @identity-TODO: blit.c may own it instead.
DATA(0x000712a4)
i16 g_cellX = 0;

DATA(0x000712a8)
i16 g_cellY = 0;

DATA(0x000712ac)
u8 g_cellCode = 0;

DATA(0x000712b0)
u8 g_cellDestDirection = 0;

DATA(0x000712b4)
i16 g_cellDestX = 0;

DATA(0x000712b8)
i16 g_cellDestY = 0;

DATA(0x000712bc)
i16 g_cellDestLevel = 0;

DATA(0x000712c0)
i16 g_cellDestArea = 0;

// Set to 0xc000 by the start-up path in this span; tested by TestFeatureMask.
DATA(0x000919e2)
i16 g_featureMask;

DATA(0x000712c8)
i16 g_quitRequest = 0;

// @identity-TODO: consumed and cleared by the main-loop step, which then
// advances the play clock by 31 ticks instead of 24; set by a script command.
DATA(0x000712cc)
static i16 s_longFrame = 0;

// A matching map position requests exit; all five selectors start disabled.
DATA(0x00068170)
static i16 s_quitArea = -1;
DATA(0x00068174)
static i16 s_quitLevel = -1;
DATA(0x00068178)
static i16 s_quitX = -1;
DATA(0x0006817c)
static i16 s_quitY = -1;
DATA(0x00068180)
static i16 s_quitDirection = -1;

DATA(0x00091548)
i16 g_infoPlane;

RVA(0x00001870, 0xa1)
b16 InitGameData(void) {
    ClearScriptVars();
    ResetSceneInput();
    PlayMusic(5, 1);
    ResetSubscreen();
    ResetTextPlanes();
    g_infoPlane = CreateTextPlane(0, 0x4001);
    LoadMask();
    LoadEffectTables();
    PreloadScriptFiles();
    LoadSkillFiles();
    LoadItemFiles();
    LoadEquipTable();
    ClearBag();
    LoadGunDistributionTable();
    LoadMoonTable();
    LoadDemonTables();
    LoadLearnableSkillTables();
    g_featureMask = 0xc000;
    InitFieldObjects();
    ClearAnalyzed();
    LoadFieldMessages();
    LoadEncounterWeights();
    LoadFieldEventTable();
    InitClock();
    ResetEventFlags();
    ClearDropSlots();
    return false;
}

RVA(0x00001920, 0x11)
void ClearScriptVars(void) {
    memset(g_scriptVars, 0, sizeof(g_scriptVars));
}

RVA(0x00001940, 0x19)
void ResetGameSession(void) {
    ClearGameStateStack();
    ResetFieldCursors();
    InitFieldPanels();
    InitAutomap();
    InitNewGame();
}

RVA(0x00001960, 0x12)
i16 SetLongFrame(i16 longFrame) {
    i16 old = s_longFrame;
    s_longFrame = longFrame;
    return old;
}

RVA(0x00001980, 0x13)
b16 TickGameTasks(void) {
    TickMessageWindow();
    RunMessageHook();
    TickCounter();
    return false;
}

RVA(0x000019a0, 0x26)
b16 StartGame(void) {
    InitGameData();
    ResetGameSession();
    g_mouseLeftClick = 0;
    g_mouseRightClick = 0;
    SetGameState(GAME_STATE_PICTURE_TRANSITION);
    return false;
}

RVA(0x000019d0, 0xc8)
i16 StepGame(void) {
    i16 state;
    LatchMouseClicks();
    g_tickElapsed = TickGameClock(0);
    state = DispatchGameState();
    if (state == -1) {
        g_quitRequest = -1;
    }
    TickGameTasks();
    state |= s_longFrame;
    s_longFrame = 0;
    g_mouseLeftClick = 0;
    g_mouseRightClick = 0;
    AdvancePlayTime(state ? 31 : 24);
    if (g_party.field.pos.area == s_quitArea && g_party.field.pos.level == s_quitLevel
        && g_party.field.pos.x == s_quitX && g_party.field.pos.y == s_quitY
        && g_party.field.pos.direction == s_quitDirection) {
        g_quitRequest = -1;
    }
    return g_quitRequest;
}
