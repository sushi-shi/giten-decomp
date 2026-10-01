// @identity-TODO: the owning TU is unproven. One retail object: the .bss
// statics of the state stack, menu cursor, status screen, DDS menu, scene
// cell, level-up, world map, party picker, subscreen and item-use code
// interleave in one run, each read only by its own part's code; their
// initialized data forms one .data run (the world-map position, then
// status-screen, item-use, field-mark, level-up and DDS menu words) ahead of
// one run of literals; and the code, from the state stack through the item-use
// flow, is contiguous in .text. It holds the state stack and main state
// dispatcher, the status screen entry and exit, the DDS menu, the scene record
// the cell events fill with the background loader and moon table, the level-up
// screen, the world-map state, the field-location marks and the field item-use
// flow.

#include <rva.h>

#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <File/DataTableId.h>
#include <Game/AreaMap.h>
#include <Game/Automap.h>
#include <Game/BagItems.h>
#include <Game/Battle.h>
#include <Game/BattleEffect.h>
#include <Game/CharInfo.h>
#include <Game/Character.h>
#include <Game/Clock.h>
#include <Game/CombatantId.h>
#include <Game/Condition.h>
#include <Game/DdsMenu.h>
#include <Game/Field.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/FusionMenu.h>
#include <Game/GameState.h>
#include <Game/GemItems.h>
#include <Game/Growth.h>
#include <Game/ObjectRecordId.h>
#include <Game/InfoBar.h>
#include <Game/ItemId.h>
#include <Game/ItemMenu.h>
#include <Game/ItemRecord.h>
#include <Game/ItemUse.h>
#include <Game/LevelUp.h>
#include <Game/MapArea.h>
#include <Game/MenuCursor.h>
#include <Game/ModeFlags.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/PartyAction.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/PartyReorder.h>
#include <Game/SaveGame.h>
#include <Game/Scene.h>
#include <Game/ScreenEffect.h>
#include <Game/Skill.h>
#include <Game/SkillList.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Game/StatusDraw.h>
#include <Game/StatusScreen.h>
#include <Game/TargetFlags.h>
#include <Game/WaitLoop.h>
#include <Game/WaitState.h>
#include <Game/WorldMap.h>
#include <Gfx/Background.h>
#include <Gfx/Blit.h>
#include <Gfx/Render.h>
#include <Gfx/Scene.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/Shot.h>
#include <Gfx/Sprite.h>
#include <Gfx/VideoState.h>
#include <Gfx/Vram.h>
#include <Gfx/VramAccess.h>
#include <Input/Mouse.h>
#include <Math/Vec3.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptVars.h>
#include <Sound/Sound.h>
#include <Text/Font.h>
#include <Text/TextEvent.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/FieldMenus.h>
#include <Ui/Hotspot.h>
#include <Ui/Menu.h>
#include <Ui/MenuBox.h>
#include <Ui/MenuStep.h>
#include <Ui/Message.h>
#include <Ui/Panel.h>
#include <Ui/PartySlotSelection.h>
#include <Util/Level.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The conditions a level-up cures.
DATA(0x00064540)
static const i16 s_levelUpCures[] = {
    CONDITION_CHARM,
    CONDITION_BERSERK,
    CONDITION_HIGH,
    CONDITION_HAPPY,
    CONDITION_PANIC,
    CONDITION_CONFUSION,
    CONDITION_DANCE,
    CONDITION_TIPSY,
    CONDITION_BLIND,
    CONDITION_HALLUCINATION,
    CONDITION_STUN,
    CONDITION_LIST_END
};

// The party's world-map position (initially 286, 192).
DATA(0x00068a48)
i16 g_worldMapX = 286;

DATA(0x00068a4c)
i16 g_worldMapY = 192;

// @identity-TODO: the menu plane is only read; its creator is unrecovered.
DATA(0x00068a50)
static i16 s_dismissMenuPlane = TEXT_PLANE_NONE;

// The item being used (-1 for none).
DATA(0x00068a54)
static i16 s_useItem = ITEM_ID_EMPTY;

// The id of the member using it (-1 for none).
DATA(0x00068a58)
static i16 s_useMemberId = -1;

DATA(0x00068a5c)
static i16 s_markedArea = -1;

DATA(0x00068a60)
static i16 s_markedLevel = -1;

DATA(0x00068a64)
static i16 s_markedX = -1;

DATA(0x00068a68)
static i16 s_markedY = -1;

DATA(0x00068a6c)
static i16 s_markedDirection = -1;

DATA(0x00068a70)
static i16 s_currentRoomCode = -1;

// The stat-list window of the member levelling up.
DATA(0x00068a74)
static i16 s_statWindow = TEXT_PLANE_NONE;

// The roster slot of the member levelling up.
DATA(0x00068a78)
static i16 s_levelUpSlot = -1;

// The window asking how many stat points are left.
DATA(0x00068a7c)
static i16 s_pointPrompt = -1;

DATA(0x00068a80)
static char* s_ddsCommands[3] = {"CALL", "RETURN", "PURGE"};

DATA(0x0007b818)
i16 g_sceneFramePositions[32][2] = {0};

DATA(0x0007b898)
static u8* s_sceneSpriteStream = NULL;

// The script the cell event runs (NULL: none).
DATA(0x0007b89c)
static u8* s_cellScript = NULL;

// The points of each stat raised this level.
DATA(0x0007b8a0)
static i16 s_statPicks[10] = {0};

// @identity-TODO: 28 bytes per row (one per moon phase), read from data file
// 3 (kind 12); what the values are is unrecovered.
DATA(0x0007b8b8)
static u8 s_moonTableBuffer[0x280] = {0};

DATA(0x0007bb38)
static MenuCursor s_summonCursor = {0};

DATA(0x0007bb40)
u8 g_sceneVideoState[16] = {0};

// The 16-byte record of the cell whose event runs.
DATA(0x0007bb50)
static u8 s_sceneCell[16] = {0};

// @identity-TODO: the video state saved while the world map is shown; the
// Windows build's save/restore bodies are empty, so its layout is unknown.
DATA(0x0007bb60)
static u8 s_videoState[16] = {0};

DATA(0x0007bb70)
static GameState s_gameState = {0};

// The skills the member can learn now.
DATA(0x0007bb78)
static i16 s_learnableSkills[64] = {0};

DATA(0x0007bbf8)
u8 g_sceneFrameIds[32][2] = {0};

DATA(0x0007bc38)
static GameState s_stateStack[GAME_STATE_STACK_DEPTH] = {0};

// @identity-TODO: legacy image handles; no loader writes them in this build.
DATA(0x0007bd38)
static u32 s_sceneEntries[32] = {0};

DATA(0x0007bdb8)
i32 g_sceneFrameSaves[32] = {0};

// Pending transition request: negative exits, positive marks the entry spot.
DATA(0x0007be38)
i16 g_worldMapRequest = 0;

DATA(0x0007be3c)
i16 g_statusMember = 0;

DATA(0x0007be40)
b16 g_statusFixedMember = false;

// @identity-TODO: the status screen's analyze mode flag.
DATA(0x0007be44)
static b16 s_statusAnalyzeMode = false;

DATA(0x0007be48)
static i16 s_stateDepth = 0;

DATA(0x0007be4c)
static i16 s_ddsRosterSlot = 0;

DATA(0x0007be50)
static i16 s_ddsPartySlot = 0;

// The party position of the first member able to use it.
DATA(0x0007be54)
static i16 s_usePosition = 0;

// @identity-TODO: only the reset and exchange access this flag in this build.
DATA(0x0007be58)
static i16 s_subscreenActive = 0;

DATA(0x0007be5c)
static GZ_ENUM_STORAGE(PartyPickerMode, i16) s_partyPickerMode = PARTY_PICKER_ALL;

// The spot marked when the world map is entered with a request.
DATA(0x0007be60)
static i16 s_savedSpotX = 0;

DATA(0x0007be64)
static i16 s_savedSpotY = 0;

// @identity-TODO: the map layer the travel routines work on.
DATA(0x0007be68)
static i16 s_mapLayer = 0;

DATA(0x0007be6c)
static MenuBox* s_partyPicker = NULL;

// Set while the party travels towards a picked destination.
DATA(0x0007be70)
static b16 s_traveling = false;

// Travel ticks since the last party step effect, cleared while idle.
DATA(0x0007be74)
static i16 s_idleFlag = 0;

DATA(0x0007be78)
static i16 s_sceneScript = 0;

DATA(0x0007be7c)
static i16 s_sceneScriptEntry = 0;

// @identity-TODO: a redraw flag and a hold word of the scene code.
DATA(0x0007be80)
static b16 s_sceneDirty = false;

DATA(0x0007be84)
static i16 s_sceneHold = 0;

DATA(0x0007be88)
static i16 s_sceneScreenState = 0;

DATA(0x0007be8c)
static i16 s_sceneObjectsFrozen = 0;

// Set when a battle's rewards wait to be handed out.
DATA(0x0007be90)
static b16 s_rewardsPending = false;

// The stat most recently raised.
DATA(0x0007be94)
static i16 s_raisedStat = 0;

// The levels (then the stat points) still to hand out.
DATA(0x0007be98)
static i16 s_remaining = 0;

// The music that played before the level-up screen.
DATA(0x0007be9c)
static i16 s_savedMusic = 0;

DATA(0x0007bea0)
static PaletteState* s_scenePaletteState = NULL;

DATA(0x0007bea4)
static PaletteState* s_statusPaletteState = NULL;

// The item list menu.
DATA(0x0007bea8)
static MenuBox* s_itemMenu = NULL;

DATA(0x0007beac)
static u8* s_moonTable = NULL;

DATA(0x0007beb0)
static MenuBox* s_ddsMenu = NULL;

// @identity-TODO: the screen area kept while the level-up screen is open; its
// layout is not recovered (the Windows build's save/restore bodies are empty).
DATA(0x0007beb8)
static u8 s_screenSave[16] = {0};

DATA(0x0007bec8)
Character* g_rosterPendingMember = NULL;

DATA(0x0007becc)
i16 g_rosterReturnState = 0;

DATA(0x0007bed0)
u16 g_rosterReturnPhase = 0;

DATA(0x0007bed4)
u16 g_rosterReturnStep = 0;

DATA(0x0007bed8)
static GZ_ENUM_STORAGE(StatusListColumn, i16) s_rosterSavedColumn = STATUS_LIST_ALL;

RVA(0x000169e0, 0xa)
void ClearGameStateStack(void) {
    s_stateDepth = 0;
}

RVA(0x000169f0, 0x32)
void SaveGameState(void) {
    if (s_stateDepth < GAME_STATE_STACK_DEPTH) {
        s_stateStack[s_stateDepth++] = s_gameState;
    }
}

RVA(0x00016a30, 0x31)
void PopGameState(void) {
    if (s_stateDepth != 0) {
        s_gameState = s_stateStack[--s_stateDepth];
    }
}

RVA(0x00016a70, 0x24)
i16 __fastcall SetGameState(GZ_ENUM_PARAM(GameStateId, i16) state) {
    i16 old = s_gameState.state;
    s_gameState.state = state;
    s_gameState.phase = 0;
    s_gameState.step = 0;
    s_gameState.sub = 0;
    return old;
}

RVA(0x00016aa0, 0x23)
i16 SetGamePhase(i16 phase) {
    i16 old = s_gameState.phase;
    s_gameState.phase = phase;
    s_gameState.step = 0;
    s_gameState.sub = 0;
    return old;
}

RVA(0x00016ad0, 0x12)
i16 NextGamePhase(void) {
    return SetGamePhase(s_gameState.phase + 1);
}

RVA(0x00016af0, 0x12)
i16 PrevGamePhase(void) {
    return SetGamePhase(s_gameState.phase - 1);
}

RVA(0x00016b10, 0x1c)
i16 SetGamePhaseKeepStep(i16 phase) {
    i16 old = s_gameState.phase;
    s_gameState.phase = phase;
    s_gameState.sub = 0;
    return old;
}

RVA(0x00016b30, 0x12)
i16 PrevGamePhaseKeepStep(void) {
    return SetGamePhaseKeepStep(s_gameState.phase - 1);
}

RVA(0x00016b50, 0x16)
i16 __fastcall SetGameStep(u16 step) {
    i16 old = s_gameState.step;
    s_gameState.step = step;
    s_gameState.sub = 0;
    return old;
}

RVA(0x00016b70, 0xd)
i16 NextGameStep(void) {
    return SetGameStep(s_gameState.step + 1);
}

RVA(0x00016b80, 0xd)
i16 PrevGameStep(void) {
    return SetGameStep(s_gameState.step - 1);
}

RVA(0x00016b90, 0xe)
i16 __fastcall SetGameSub(i16 sub) {
    i16 old = s_gameState.sub;
    s_gameState.sub = sub;
    return old;
}

RVA(0x00016ba0, 0xe)
i16 NextGameSub(void) {
    return SetGameSub(s_gameState.sub + 1);
}

RVA(0x00016bb0, 0xe)
i16 PrevGameSub(void) {
    return SetGameSub(s_gameState.sub - 1);
}

RVA(0x00016bc0, 0x7)
GZ_ENUM_RETURN(GameStateId, i16) GetGameState(void) {
    return s_gameState.state;
}

RVA(0x00016bd0, 0x7)
u16 GetGamePhase(void) {
    return s_gameState.phase;
}

RVA(0x00016be0, 0x7)
u16 GetGameStep(void) {
    return s_gameState.step;
}

RVA(0x00016bf0, 0x7)
u16 GetGameSub(void) {
    return s_gameState.sub;
}

RVA(0x00016c00, 0x16)
void __fastcall PushGameState(GZ_ENUM_PARAM(GameStateId, i16) state) {
    SaveGameState();
    SetGameState(state);
}

// @identity-TODO: a one-line wrapper placed after PushGameState in this TU,
// compiled to a tail jump into PopGameState.
RVA(0x00016c20, 0x5)
void ReturnFromGameState(void) {
    PopGameState();
}

// A four-level cursor: setting a level resets every level below it.
RVA(0x00016c30, 0x12)
i16 SetCursorLevel3(MenuCursor* cursor, i16 value) {
    i16 old = cursor->level[3];
    cursor->level[3] = value;
    return old;
}

RVA(0x00016c50, 0x22)
i16 SetCursorLevel2(MenuCursor* cursor, i16 value) {
    i16 old = cursor->level[2];
    cursor->level[2] = value;
    SetCursorLevel3(cursor, 0);
    return old;
}

RVA(0x00016c80, 0x22)
i16 SetCursorLevel1(MenuCursor* cursor, i16 value) {
    i16 old = cursor->level[1];
    cursor->level[1] = value;
    SetCursorLevel2(cursor, 0);
    return old;
}

RVA(0x00016cb0, 0x20)
i16 SetCursorLevel0(MenuCursor* cursor, i16 value) {
    i16 old = cursor->level[0];
    cursor->level[0] = value;
    SetCursorLevel1(cursor, 0);
    return old;
}

RVA(0x00016cd0, 0x14)
i16 NextCursorLevel0(MenuCursor* cursor) {
    return SetCursorLevel0(cursor, cursor->level[0] + 1);
}

RVA(0x00016cf0, 0x14)
i16 PrevCursorLevel0(MenuCursor* cursor) {
    return SetCursorLevel0(cursor, cursor->level[0] - 1);
}

RVA(0x00016d10, 0x8)
u16 GetCursorLevel0(MenuCursor* cursor) {
    return cursor->level[0];
}

RVA(0x00016d20, 0x9)
i16 GetCursorLevel1(MenuCursor* cursor) {
    return cursor->level[1];
}

RVA(0x00016d30, 0x11c)
i16 PickStatusMember(void) {
    i16 selected;
    switch (GetGameStep()) {
        case PICK_MEMBER_STEP_POLL:
            selected = PollStatusMenu();
            if (selected == STATUS_STEP_CLOSE || selected == STATUS_COMMAND_CANCEL) {
                CheckStatusMenuItem(STATUS_STEP_CLOSE);
                SetGameStep(PICK_MEMBER_STEP_CANCELLED);
            } else {
                selected = RunStatusListPicker(false);
                if (selected >= 0) {
                    PrevGameStep();
                    SetGameSub(selected);
                }
            }
            break;
        case PICK_MEMBER_STEP_OPEN:
            SetGameStep(PICK_MEMBER_STEP_POLL);
            TakeMouseCancelSound();
            SetStatusMenuItemsHidden(1);
            SetStatusMenuItemFlag(STATUS_STEP_ITEMS, PANEL_SKIP_HIT_TEST, !CountBagEntries());
            RedrawPartyStatus();
            RepaintTextPlane(g_infoPlane, 1);
            ClearStatusMenu();
            SetStatusColumn(STATUS_LIST_ALL);
            RunStatusListPicker(false);
            break;
        case PICK_MEMBER_STEP_PICKED:
        case PICK_MEMBER_STEP_CANCELLED:
            SetStatusMenuItemsHidden(0);
            RunStatusListPicker(true);
            if (GetGameStep() == PICK_MEMBER_STEP_CANCELLED) {
                return STATUS_COMMAND_CANCEL_FIXED_MEMBER;
            }
            return GetGameSub();
    }
    return STATUS_COMMAND_NONE;
}

RVA(0x00016e50, 0xc)
void SetStatusAnalyzeMode(b16 on) {
    s_statusAnalyzeMode = on;
}

RVA(0x00016e60, 0x7)
i16 GetStatusAnalyzeMode(void) {
    return s_statusAnalyzeMode;
}

RVA(0x00016e70, 0x144)
b16 RunStatusScreen(void) {
    i16 result;
    switch (GetGamePhase()) {
        case STATUS_PHASE_OPEN:
            SetPictureRenderMode();
            HideScreenLayer(SCREEN_LAYER_PANEL);
            CloseMessageWindow();
            SetGamePhase(STATUS_PHASE_PICK_MEMBER);
            SetStatusMenuItemsHidden(0);
            EnterStatusScreen(0);
            if (s_statusAnalyzeMode) {
                NextGamePhase();
                g_statusMember = 15;
                g_statusFixedMember = true;
            }
            break;
        case STATUS_PHASE_CLOSE:
            ReturnFromGameState();
            RunStatusListPicker(true);
            RequestFieldRefresh();
            LeaveStatusScreen(0);
            g_statusFixedMember = false;
            SetFieldPanelRowChecked(1, 0);
            g_statusMember = 0;
            ErasePictureSurface(0x36);
            ClearStatusPicture();
            break;
        case STATUS_PHASE_PICK_MEMBER:
            g_statusMember = PickStatusMember();
            if (g_statusMember == STATUS_COMMAND_CANCEL_FIXED_MEMBER) {
                PrevGamePhase();
            } else if (g_statusMember != STATUS_COMMAND_NONE) {
                NextGamePhase();
            }
            break;
        case STATUS_PHASE_COMMANDS:
            result = RunStatusCommands();
            if (result == STATUS_COMMAND_CANCEL_FIXED_MEMBER) {
                ErasePictureSurface(0x36);
                SetGamePhase(STATUS_PHASE_CLOSE);
            } else if (result == STATUS_STEP_EXIT || result == STATUS_COMMAND_CANCEL) {
                PrevGamePhase();
                ClearStatusPicture();
                ErasePictureSurface(0x36);
            }
            break;
    }
    return false;
}

RVA(0x00016fc0, 0x2d)
void EnterStatusScreen(i16 nested) {
    ClearStatusPicture();
    if (!nested) {
        SetSubscreenActive(1);
        s_statusPaletteState = SavePaletteState(s_statusPaletteState, 2);
    }
}

RVA(0x00016ff0, 0x31)
void LeaveStatusScreen(i16 nested) {
    ResetStatusMenu();
    if (!nested) {
        if (s_statusPaletteState) {
            s_statusPaletteState = RestorePaletteState(s_statusPaletteState, true);
        }
        SetSubscreenActive(0);
    }
}

RVA(0x00017030, 0x60)
b16 RunDismissMenuState(void) {
    switch (GetGamePhase()) {
        case DISMISS_MENU_RESET_SELECTION:
            NextGamePhase();
            ResetTextPlaneHighlight(s_dismissMenuPlane);
        case DISMISS_MENU_WAIT_INPUT:
            if (PollMenuInput(s_dismissMenuPlane)) {
                NextGamePhase();
            }
            break;
        case DISMISS_MENU_CLOSE:
            CloseTextWindow(s_dismissMenuPlane);
            ReturnFromGameState();
            break;
    }
    return false;
}

// @early-stop load width: retail loads the saved state and step into ecx
// with dword reads for fastcall arguments; this build uses cx. Their sole
// writer stores words, so widening the saved globals would misstate storage.
RVA(0x00017090, 0xc1)
b16 ReplaceRosterMember(void) {
    i16 selected;
    switch (GetGamePhase()) {
        case ROSTER_REPLACEMENT_OPEN_LIST:
            s_rosterSavedColumn = SetStatusColumn(STATUS_LIST_RESERVE_UNFLAGGED);
            NextGamePhase();
        case ROSTER_REPLACEMENT_PICK_MEMBER:
            selected = RunStatusListPicker(false);
            if (selected == LIST_MENU_OPEN || selected == LIST_MENU_CANCELLED) {
                break;
            }
            NextGamePhase();
        case ROSTER_REPLACEMENT_APPLY:
            // Retail leaves selected uninitialized on direct entry to this phase.
            RemoveFromRoster(selected);
            SetStatusColumn(s_rosterSavedColumn);
            RunStatusListPicker(true);
            if (AddToRoster(g_rosterPendingMember) < 0) {
                SetGamePhase(ROSTER_REPLACEMENT_OPEN_LIST);
            }
            RestoreRosterReturnState();
            break;
    }
    return false;
}

RVA(0x00017160, 0x1d0)
i16 DispatchGameState(void) {
    i16 result;
    u16 state = GetGameState();
    switch (state) {
        case GAME_STATE_RETURN:
            ReturnFromGameState();
            result = 0;
            break;
        case GAME_STATE_WAIT:
            result = StepWaitState();
            break;
        case GAME_STATE_SCRIPT_SCENE:
            RunScriptScene();
            result = 0;
            break;
        case GAME_STATE_SCRIPT_CHOICE:
            RunScriptChoiceState();
            result = 0;
            break;
        case GAME_STATE_DISMISS_MENU:
            RunDismissMenuState();
            result = 0;
            break;
        case GAME_STATE_TEXT_WINDOW:
            RunTextWindowState();
            result = 0;
            break;
        case GAME_STATE_SHOT:
            result = RunShotState();
            break;
        case GAME_STATE_ACTOR_SCENE:
            result = RunActorScene();
            break;
        case GAME_STATE_FIELD_ENCOUNTER:
            result = RunFieldEncounter();
            break;
        case GAME_STATE_CLOSING_EFFECT:
            result = RunClosingEffectState();
            break;
        case GAME_STATE_SCREEN_FADE:
            result = RunScreenFadeState();
            break;
        case GAME_STATE_ITEM_USE:
            result = RunItemUse();
            break;
        case GAME_STATE_SYSTEM_MENU:
            result = RunSystemMenu();
            break;
        case GAME_STATE_FIELD_EXPLORATION:
            result = RunFieldExploration();
            break;
        case GAME_STATE_CELL_SCENE:
            result = RunCellScene();
            break;
        case GAME_STATE_ITEM_BUY_MENU:
            result = RunItemBuyMenu();
            break;
        case GAME_STATE_FIELD_TEXT_SCENE:
            result = RunFieldTextScene();
            break;
        case GAME_STATE_WORLD_MAP:
            result = RunWorldMap();
            break;
        case GAME_STATE_BACKGROUND_SCENE:
            result = RunBackgroundScene();
            break;
        case GAME_STATE_BATTLE_ACTION:
            result = RunBattleAction();
            break;
        case GAME_STATE_STATUS:
            result = RunStatusScreen();
            break;
        case GAME_STATE_FIELD_SKILL_USE:
            result = RunFieldSkillUse();
            break;
        case GAME_STATE_LEVEL_UP:
            result = RunLevelUp();
            break;
        case GAME_STATE_ITEM_SELL_MENU:
            result = RunItemSellMenu();
            break;
        case GAME_STATE_FROZEN_FIELD_SCENE:
            result = RunFrozenFieldScene();
            break;
        case GAME_STATE_DDS_MENU:
            result = RunDdsMenu();
            break;
        case GAME_STATE_FUSION_MENU:
            result = RunFusionMenuState();
            break;
        case GAME_STATE_AUTOMAP:
            result = RunAutomapState();
            break;
        case GAME_STATE_MESSAGE_BOX:
            result = RunMessageBoxState();
            break;
        case GAME_STATE_FIELD:
            result = RunFieldState();
            break;
        case GAME_STATE_MESSAGE_SCENE_END:
            result = FinishMessageScene();
            break;
        case GAME_STATE_PARTY_REORDER:
            result = RunPartyReorder();
            break;
        case GAME_STATE_SCRIPT_ANIMATION:
            result = RunScriptAnimationState();
            break;
        case GAME_STATE_GEM_ITEM_GIFT:
            result = RunGemItemGift();
            break;
        case GAME_STATE_PICTURE_TRANSITION:
            result = RunPictureTransition();
            break;
        case GAME_STATE_REPLACE_ROSTER_MEMBER:
            result = ReplaceRosterMember();
            break;
    }
    // Retail leaves the result uninitialized for unhandled state numbers.
    return result;
}

RVA(0x00017330, 0x178)
b16 RunDdsMenu(void) {
    i16 result;
    switch (GetGamePhase()) {
        case MENU_STEP_OPEN:
            NextGamePhase();
            NextGamePhase();
            RunPartyPicker(PARTY_PICKER_COMMAND_CLOSE);
            s_ddsMenu = CreateMenuBox(s_ddsMenu, 25, 2);
            MoveMenuBox(s_ddsMenu, -8, -22);
            SetMenuItems(s_ddsMenu, 9, s_ddsCommands, 3, DdsMenuHandler);
            HideScreenLayer(SCREEN_LAYER_PANEL);
            break;
        case MENU_STEP_CLOSE:
            ReturnFromGameState();
            s_ddsMenu = DestroyMenuBox(s_ddsMenu);
            SetFieldPanelRowChecked(4, 0);
            RequestFieldRefresh();
            break;
        case MENU_STEP_RUN:
            result = RunMenu(s_ddsMenu);
            if (result == TEXT_EVENT_CANCEL) {
                PrevGamePhase();
            }
            if (result > 0) {
                SetGamePhase(g_selectedObjectId + MENU_STEP_PICK_FIRST);
                s_ddsMenu = DestroyMenuBox(s_ddsMenu);
            }
            break;
        case MENU_STEP_PICK_FIRST + DDS_ROW_CALL:
            RunDdsSummon();
            break;
        case MENU_STEP_PICK_FIRST + DDS_ROW_RETURN:
            s_ddsRosterSlot = ReturnDdsMember();
            if (s_ddsRosterSlot) {
                SetGamePhase(MENU_STEP_CLOSE);
            }
            break;
        case MENU_STEP_PICK_FIRST + DDS_ROW_PURGE:
            if (PickDdsPurgeMember() != ROSTER_SLOT_NONE) {
                SetGamePhase(MENU_STEP_CLOSE);
                if (s_ddsRosterSlot >= 0) {
                    RemoveFromRoster(s_ddsRosterSlot);
                    PlaySoundEffect(0x36);
                }
            }
            break;
    }
    return false;
}

RVA(0x000174b0, 0x70)
b16 RunDdsSummon(void) {
    GZ_ENUM_LOCAL(DdsActionResult, i16) result;
    switch (GetGameStep()) {
        case DDS_SUMMON_STEP_CLOSE:
            SetGamePhase(MENU_STEP_CLOSE);
            break;
        case DDS_SUMMON_STEP_PICK:
            result = PickDdsSummon();
            if (result) {
                NextGameStep();
                if (result > 0) {
                    AddTrainingPoints(GetCharacters(), BATTLE_GROUP_DEMON_INTERACTION, 8);
                }
            }
            break;
        case DDS_SUMMON_STEP_PREPARE:
            CloseMessageWindow();
            SetCursorLevel0(&s_summonCursor, DDS_SUMMON_CURSOR_PICK_ROSTER);
            NextGameStep();
            break;
    }
    return false;
}

RVA(0x00017520, 0x1cc)
GZ_ENUM_RETURN(DdsActionResult, i16) PickDdsSummon(void) {
    i16 step;
    i16 previous;
    Character* character;
    switch (GetCursorLevel0(&s_summonCursor)) {
        case DDS_SUMMON_CURSOR_PICK_ROSTER:
            step = PickDdsRosterMember(GetCursorLevel1(&s_summonCursor));
            SetCursorLevel1(&s_summonCursor, step);
            if (step < 0) {
                if (s_ddsRosterSlot < 0) {
                    return DDS_ACTION_CANCELLED;
                }
                NextCursorLevel0(&s_summonCursor);
            }
            break;
        case DDS_SUMMON_CURSOR_PICK_PARTY_SLOT:
            if (!PollPartySlotSelection(PARTY_SLOT_ANY)) {
                break;
            }
            ClearPartySlotSelection();
            if (g_selectedObjectId < 0) {
                PrevCursorLevel0(&s_summonCursor);
                return DDS_ACTION_PENDING;
            }
            character = GetPartyCharacter(g_selectedObjectId);
            if (character != NULL && IsHumanCharacter(character)) {
                return DDS_ACTION_CANCELLED;
            }
            s_ddsPartySlot = g_selectedObjectId;
            NextCursorLevel0(&s_summonCursor);
            break;
        case DDS_SUMMON_CURSOR_TRANSITION:
            NextCursorLevel0(&s_summonCursor);
            break;
        case DDS_SUMMON_CURSOR_EXCHANGE:
            NextCursorLevel0(&s_summonCursor);
            previous = ExchangePartySlot(s_ddsPartySlot, s_ddsRosterSlot);
            character = GetRosterCharacter(s_ddsRosterSlot);
            if (character != NULL) {
                ClearActionWait(GetCharacterActionWait(character));
                AddMagnetite(GetRosterCharacter(ROSTER_LEADER), -GetSummonMagnetiteCost(character));
                ResetBattleTally(character);
            }
            character = GetRosterCharacter(previous);
            if (character != NULL) {
                ClearBattleConditions(GetCharacterConditions(character));
                ResetBattleTally(character);
            }
            MarkPickDone();
            PlaySoundEffect(0x20);
            break;
        case DDS_SUMMON_CURSOR_FINISHED:
            return DDS_ACTION_COMPLETED;
    }
    return DDS_ACTION_PENDING;
}

RVA(0x000176f0, 0x4a)
i16 PickDdsRosterMember(i16 step) {
    switch (step) {
        case DDS_ROSTER_PICK_CLOSE:
            RunStatusListPicker(true);
            return DDS_ROSTER_PICK_CLOSED;
        case DDS_ROSTER_PICK_OPEN:
            SetStatusColumn(STATUS_LIST_SUMMONABLE);
            step++;
        case DDS_ROSTER_PICK_POLL:
            s_ddsRosterSlot = RunStatusListPicker(false);
            if (s_ddsRosterSlot != ROSTER_SLOT_NONE) {
                step++;
            }
            break;
    }
    return step;
}

RVA(0x00017740, 0x126)
void DdsMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    char** items;
    i16 slot;
    i16 disabled;
    i32 attribute;
    Character* character;
    GetGamePhase();
    items = menu->items.text;
    switch (event) {
        case MENU_EVENT_ADD_ROW:
            attribute = TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
            disabled = MENU_LINE_DISABLED;
            switch (index) {
                case DDS_ROW_PURGE:
                    if (CountRosterEntries(false)) {
                        attribute = TEXT_ATTR_FLAG1
                                    | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_BLACK);
                        disabled = false;
                    }
                    break;
                case DDS_ROW_RETURN:
                    for (slot = 0; slot < PARTY_SIZE; slot++) {
                        character = GetPartyCharacter(slot);
                        if (character != NULL && !IsHumanCharacter(character)) {
                            attribute =
                                TEXT_ATTR_FLAG1
                                | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_BLACK);
                            disabled = false;
                        }
                    }
                    break;
                case DDS_ROW_CALL:
                    for (slot = 0; slot < ROSTER_SIZE; slot++) {
                        character = GetRosterCharacter(slot);
                        if (character != NULL && !IsHumanCharacter(character)
                            && !GetFatalCondition(GetCharacterConditions(character))) {
                            attribute =
                                TEXT_ATTR_FLAG1
                                | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_BLACK);
                            disabled = false;
                        }
                    }
                    break;
            }
            AddMenuLine(menu->plane, items[index], attribute, index, disabled);
            break;
        case MENU_EVENT_BEGIN_PAGE:
            sprintf(g_scratchBuffer, "<DDS>");
            AddMenuLine(menu->plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, -1, MENU_LINE_DISABLED);
            break;
        case MENU_EVENT_DESTROY:
            menu->items.text = NULL;
            menu->itemCount = 0;
            break;
    }
}

RVA(0x00017870, 0x8a)
GZ_ENUM_RETURN(DdsActionResult, i16) ReturnDdsMember(void) {
    Character* character;
    if (!PollPartySlotSelection(PARTY_SLOT_REQUIRE_OCCUPIED)) {
        return DDS_ACTION_PENDING;
    }
    ClearPartySlotSelection();
    if (g_selectedObjectId < 0) {
        return DDS_ACTION_CANCELLED;
    }
    character = GetPartyCharacter(g_selectedObjectId);
    if (character == NULL) {
        return DDS_ACTION_PENDING;
    }
    if (IsHumanCharacter(character)) {
        return DDS_ACTION_PENDING;
    }
    ClearBattleConditions(GetCharacterConditions(character));
    ResetBattleTally(character);
    MarkPickDone();
    ClearPartyPosition(g_selectedObjectId);
    PlaySoundEffect(0x55);
    return DDS_ACTION_COMPLETED;
}

RVA(0x00017900, 0x5f)
i16 PickDdsPurgeMember(void) {
    switch (GetGameSub()) {
        case DDS_PURGE_PICK:
            s_ddsRosterSlot = RunStatusListPicker(false);
            if (s_ddsRosterSlot != ROSTER_SLOT_NONE) {
                PrevGameSub();
            }
            break;
        case DDS_PURGE_FINISH:
            RunStatusListPicker(true);
            return s_ddsRosterSlot;
        case DDS_PURGE_PREPARE:
            NextGameSub();
            NextGameSub();
            SetStatusColumn(STATUS_LIST_RESERVE_UNFLAGGED);
            break;
    }
    return ROSTER_SLOT_NONE;
}

RVA(0x00017960, 0x2a)
u16 TimeUntilMoonPhase(i16 phase) {
    phase -= g_clock.moonPhase;
    if (phase <= 0) {
        phase += MOON_PHASE_COUNT;
    }
    return phase * MOON_PHASE_TICKS - g_clock.moonTicks;
}

RVA(0x00017990, 0x9)
GZ_ENUM_RETURN(MoonPhase, i16) GetMoonPhase(void) {
    return g_clock.moonPhase;
}

RVA(0x000179a0, 0x27)
u32 GetClockMinutes(void) {
    return (g_clock.days * 24 + g_clock.hour) * 60 + g_clock.minute;
}

RVA(0x000179d0, 0x34)
void LoadMoonTable(void) {
    FILE* fp = OpenDataFile(DATA_TABLE_MOON, DATA_FILE_TABLE, 0);
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
    MapPosition position = g_party.field.pos;
    if (position.area == MAP_AREA_HATSUDAI) {
        if (position.x == 12 && position.y == 11 && position.level == 8) {
            s_sceneCell[9] = 0x31;
            s_sceneCell[10] = 2;
            s_sceneCell[11] = 2;
        }
    } else if (position.area == MAP_AREA_BAEL_CASTLE) {
        if (position.x == 4 && position.y == 3 && position.level == 7) {
            s_sceneCell[13] = 0x31;
            s_sceneCell[14] = 3;
            s_sceneCell[15] = 2;
        }
    }
    if (!IsEventFlagSet(EVENT_FLAG_BANK_AREA, AREA_FIXED_BACKGROUND)) {
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
    if (image == 0x401d && GetRenderMode() == RENDER_MODE_VIEW) {
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
        case CELL_SCENE_PREPARE:
            SetSceneRenderMode();
            UnplaceAllSprites();
            s_sceneScreenState = SaveScreenState();
            CloseMessageWindow();
            NextGamePhase();
            SaveVideoState(g_sceneVideoState);
            SetSubscreenActive(1);
            g_fieldRedrawRequest = true;
            ClearMaskView();
            ResetMask(1);
            s_scenePaletteState = SavePaletteState(s_scenePaletteState, 1);
            RestoreBackground();
            LoadSceneSprites();
            PlaceSceneSprites();
            ClearTextPlane(g_infoPlane);
            LockStatusRedraw(true);
            RedrawScreen(1, 0);
            StartScreenFadeAndWait(SCREEN_FADE_FROM_BLACK, 1);
            return true;
        case CELL_SCENE_RUN_SCRIPT:
            NextGamePhase();
            StartDebugScene(s_sceneScript, s_sceneScriptEntry, g_infoPlane);
            break;
        case CELL_SCENE_WAIT_INPUT:
            NextGamePhase();
            PushWaitState(WAIT_INPUT, WAIT_ON_ANY_INPUT, 0xffff, 0);
            break;
        case CELL_SCENE_FADE_OUT:
            NextGamePhase();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            break;
        case CELL_SCENE_RETURN_FIELD:
            s_sceneDirty = false;
            FreeSceneSprites();
            SceneNop();
            s_scenePaletteState = RestorePaletteState(s_scenePaletteState, true);
            RestoreVideoState(g_sceneVideoState);
            SetSubscreenActive(0);
            ReturnFromGameState();
            PlayLevelMusic();
            LockStatusRedraw(false);
            SetRebuildRoom(1);
            if (g_worldMapRequest < 0) {
                SetGameState(GAME_STATE_FIELD_EXPLORATION);
            } else if (g_worldMapRequest > 0) {
                SetGameState(GAME_STATE_FIELD_EXPLORATION);
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
        case FIELD_TEXT_SCENE_RETURN_FIELD:
            s_sceneDirty = false;
            FreeSceneSprites();
            ReturnFromGameState();
            LockStatusRedraw(false);
            RefreshStatusPanel(1);
            RepaintTextPlane(g_infoPlane, 3);
            g_fieldRedrawRequest = true;
            if (g_worldMapRequest < 0) {
                SetGameState(GAME_STATE_FIELD_EXPLORATION);
                SetGamePhase(FIELD_PHASE_FADE_TO_RETURN_POINT);
            } else if (g_worldMapRequest > 0) {
                SetGameState(GAME_STATE_FIELD_EXPLORATION);
            }
            s_sceneHold = 0;
            RestoreScreenState(s_sceneScreenState);
            RedrawFieldView();
            break;
        case FIELD_TEXT_SCENE_START:
            s_sceneScreenState = SaveScreenState();
            NextGamePhase();
            PrepareFieldRedraw(1);
            ClearTextPlane(g_infoPlane);
            LockStatusRedraw(true);
            RepaintTextPlane(g_infoPlane, 3);
            StartDebugScene(s_sceneScript, s_sceneScriptEntry, g_infoPlane);
            break;
    }
    return false;
}

RVA(0x00018340, 0x157)
b16 RunFrozenFieldScene(void) {
    switch (GetGamePhase()) {
        case FROZEN_FIELD_SCENE_RETURN_FIELD:
            s_sceneDirty = false;
            FreeSceneSprites();
            ReturnFromGameState();
            ExchangeObjectsFrozen(s_sceneObjectsFrozen);
            LockStatusRedraw(false);
            s_scenePaletteState = RestorePaletteState(s_scenePaletteState, true);
            RefreshStatusPanel(1);
            g_fieldRedrawRequest = true;
            if (g_worldMapRequest < 0 || g_worldMapRequest > 0) {
                SetGameState(GAME_STATE_FIELD_EXPLORATION);
                StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            }
            s_sceneHold = 0;
            RestoreScreenState(s_sceneScreenState);
            RedrawFieldView();
            break;
        case FROZEN_FIELD_SCENE_START:
            s_sceneObjectsFrozen = ExchangeObjectsFrozen(true);
            s_sceneScreenState = SaveScreenState();
            NextGamePhase();
            s_scenePaletteState = SavePaletteState(s_scenePaletteState, 3);
            PrepareFieldRedraw(1);
            ClearTextPlane(g_infoPlane);
            LockStatusRedraw(true);
            StartDebugScene(s_sceneScript, s_sceneScriptEntry, g_infoPlane);
            g_fieldRedrawRequest = true;
            RedrawFieldView();
            break;
    }
    return UpdateFieldScreen(false);
}

// @identity-TODO: the role of the fixed scene script is unrecovered.
RVA(0x000184a0, 0xe8)
b16 RunPictureTransition(void) {
    switch (GetGamePhase()) {
        case PICTURE_TRANSITION_FADE_OUT:
            NextGamePhase();
            LockStatusRedraw(true);
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            break;
        case PICTURE_TRANSITION_SHOW_PICTURE:
            NextGamePhase();
            StartScreenFade(SCREEN_FADE_FROM_BLACK, 1);
            ShowScenePicture();
            PushWaitState(WAIT_FADE, 0, 0, -1);
            break;
        case PICTURE_TRANSITION_RUN_SCRIPT:
            NextGamePhase();
            StartDebugScene(0x2d, 0, 0);
            break;
        case PICTURE_TRANSITION_RETURN_FIELD:
            s_sceneDirty = false;
            FreeSceneSprites();
            LockStatusRedraw(false);
            s_scenePaletteState = RestorePaletteState(s_scenePaletteState, true);
            SetGameState(GAME_STATE_FIELD_EXPLORATION);
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            s_sceneHold = 0;
            break;
    }
    return false;
}

RVA(0x00018590, 0x19c)
b16 RunBackgroundScene(void) {
    switch (GetGamePhase()) {
        case BACKGROUND_SCENE_PREPARE:
            ClearSceneSurfaces();
            s_sceneScreenState = SaveScreenState();
            NextGamePhase();
            SaveVideoState(g_sceneVideoState);
            g_fieldRedrawRequest = true;
            ClearMaskView();
            ResetMask(1);
            s_scenePaletteState = SavePaletteState(s_scenePaletteState, 1);
            RestoreBackground();
            LockStatusRedraw(true);
            ClearTextPlane(g_infoPlane);
            SetInfoBarLayout(0);
            RedrawScreen(0, 1);
            StartScreenFadeAndWait(SCREEN_FADE_FROM_BLACK, 1);
            return true;
        case BACKGROUND_SCENE_RUN_SCRIPT:
            NextGamePhase();
            StartDebugScene(s_sceneScript, s_sceneScriptEntry, g_infoPlane);
            break;
        case BACKGROUND_SCENE_WAIT_INPUT:
            NextGamePhase();
            PushWaitState(WAIT_INPUT, WAIT_ON_ANY_INPUT, 0xffff, 0);
            break;
        case BACKGROUND_SCENE_FADE_OUT:
            NextGamePhase();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            break;
        case BACKGROUND_SCENE_RESTORE:
            s_sceneDirty = false;
            FreeSceneSprites();
            SceneNop();
            s_scenePaletteState = RestorePaletteState(s_scenePaletteState, true);
            RestoreVideoState(g_sceneVideoState);
            ReturnFromGameState();
            LockStatusRedraw(false);
            s_sceneHold = 0;
            RestoreScreenState(s_sceneScreenState);
            break;
    }
    return false;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
// @identity-TODO: an empty hook at the start of the level-up family.
RVA(0x00018730, 0x1)
void LevelUpNop(void) {}

// The reward screen's click: 2 for the right button, 0 for a left click in
// the OK box, -1 otherwise.
RVA(0x00018740, 0x3e)
GZ_ENUM_RETURN(RewardClickResult, i16) PollRewardClick(i16 inputA, i16 inputB, i16 inputC, i16* x, i16* y) {
    if (g_mousePosition.buttons & MOUSE_RIGHT_DOWN) {
        return REWARD_CLICK_RIGHT_BUTTON;
    }
    if ((g_mousePosition.buttons & MOUSE_LEFT_DOWN) && g_mousePosition.x >= 0xea
        && g_mousePosition.x <= 0x192 && g_mousePosition.y >= 0xc0 && g_mousePosition.y <= 0xf6) {
        return REWARD_CLICK_CONFIRM;
    }
    return REWARD_CLICK_NONE;
}

// The experience at which `level` begins: 4(n^3)+6 for a human (id below
// 0x20), 5(n^3+1) for a demon, n = level - 1.
RVA(0x00018780, 0x25)
u32 ExperienceForLevel(i16 level, i16 id) {
    i16 n = level - 1;
    i32 cube = n * n * n;
    if (id < 0x20) {
        return cube * 4 + 6;
    }
    return (cube + 1) * 5;
}

// How far `experience` is past the start of `level` (-1 from level 100).
RVA(0x000187b0, 0x25)
i32 ExperienceToLevel(i16 level, u32 experience, i16 id) {
    if (level >= 100) {
        return -1;
    }
    experience -= ExperienceForLevel(level, id);
    return experience;
}

// Adds experience to a living character; how far it is past the next level
// (negative: not reached), 0 for none or a disabled one, -1 at level 99.
RVA(0x000187e0, 0x54)
i32 AddExperience(Character* character, i32 amount) {
    if (character == NULL) {
        return 0;
    }
    if (GetDisablingCondition(GetCharacterConditions(character))) {
        return 0;
    }
    if (character->level >= 99) {
        return -1;
    }
    character->experience += amount;
    return ExperienceToLevel(character->level + 1, character->experience, character->id);
}

// Shares twice `amount` among the party's able members; how many reached a
// new level.
RVA(0x00018840, 0x4a)
i32 ShareExperience(i32 amount) {
    i16 count = CountPartyMembers(1);
    i32 share = amount * 2 / count;
    i32 reached = 0;
    i16 i;
    for (i = 0; i < PARTY_SIZE; i++) {
        reached += AddExperience(GetPartyCharacter(i), share) >= 0;
    }
    return reached;
}

// The levels roster member `slot` has earned but not taken yet.
RVA(0x00018890, 0x79)
i16 CountPendingLevels(i16 slot) {
    Character* character = GetRosterCharacter(slot);
    u16 level;
    if (character == NULL) {
        return 0;
    }
    if (GetDisablingCondition(GetCharacterConditions(character))) {
        return 0;
    }
    for (level = character->level;
         ExperienceToLevel(level + 1, character->experience, character->id) >= 0;
         level++) {
    }
    return level - character->level;
}

// The pending levels of the whole party.
RVA(0x00018910, 0x27)
i16 CountPartyPendingLevels(void) {
    i16 total = 0;
    i16 i;
    for (i = 0; i < PARTY_SIZE; i++) {
        total += CountPendingLevels(GetPartySlot(i));
    }
    return total;
}

// The roster slot of the first party member with a pending level (-1: none).
RVA(0x00018940, 0x32)
i16 FindLevelUpSlot(void) {
    i16 i;
    for (i = 0; i < PARTY_SIZE; i++) {
        if (CountPendingLevels(GetPartySlot(i)) > 0) {
            return GetPartySlot(i);
        }
    }
    return -1;
}

// Raises fortune by one on every second level of a human and every third
// level of a demon; 1 when it grew.
RVA(0x00018980, 0x4b)
b16 ApplyLevelStatGrowth(Character* character) {
    if (IsHumanCharacter(character)) {
        if (character->level & 1) {
            return false;
        }
    } else if (character->level % 3) {
        return false;
    }
    character->stats.base[STAT_FORTUNE] = ClampTo100(GetBaseStat(character, STAT_FORTUNE) + 1);
    return true;
}

// Nonzero when one more point would take `stat` past its cap.
RVA(0x000189d0, 0x24)
i16 IsStatCapped(Character* character, i16 stat) {
    i16 raised = GetBaseStat(character, stat) + 1;
    return raised - ClampTo100(raised);
}

// How many of the ten stats can still take a point.
RVA(0x00018a00, 0x2b)
i16 CountRaisableStats(Character* character) {
    i16 count = 0;
    i16 i;
    for (i = 0; i < STAT_FORTUNE; i++) {
        count += !IsStatCapped(character, i);
    }
    return count;
}

// `stat`, or a random stat when negative, re-rolled until one can take a
// point; -1 when none can.
RVA(0x00018a30, 0x67)
i16 ResolveRaisableStat(Character* character, i16 stat) {
    if (!CountRaisableStats(character)) {
        return -1;
    }
    if (stat < 0) {
        stat = RandomAverage(0, 9, 0);
    }
    while (IsStatCapped(character, stat)) {
        stat = RandomAverage(0, 9, 0);
    }
    return stat;
}

// Raises `experience` to at least the start of the character's level.
RVA(0x00018aa0, 0x23)
void RaiseExperienceToLevel(Character* character) {
    u32 floor = ExperienceForLevel(character->level, character->id);
    if (character->experience < floor) {
        character->experience = floor;
    }
}

// Pays a pending battle's macca and magnetite to the leader and shares its
// experience; the party's pending level count (0 with nothing pending).
RVA(0x00018ad0, 0x76)
i16 GrantBattleRewards(void) {
    Character* leader;
    if (s_rewardsPending) {
        leader = GetRosterCharacter(ROSTER_LEADER);
        AddMacca(leader, g_rewardMacca);
        g_rewardMacca = 0;
        AddMagnetite(leader, g_rewardMagnetite);
        g_rewardMagnetite = 0;
        DrawInfoBar(0, false);
        ShareExperience(g_rewardExperience);
        g_rewardExperience = 0;
        s_rewardsPending = false;
        return CountPartyPendingLevels();
    }
    return 0;
}

RVA(0x00018b50, 0xa)
void MarkRewardsPending(void) {
    s_rewardsPending = true;
}

static __inline void FinishLevelGain(Character* character) {
    ApplyLevelStatGrowth(character);
    FullyRestoreCharacter(character);
}

static __inline void ApplyPickedStatGain(Character* member) {
    member->stats.base[s_raisedStat]++;
    SaveGameState();
    SetGamePhase(LEVEL_UP_PHASE_REDRAW_STAT);
}

static __inline void ShowRaisedStat(Character* member, i16 highlighted) {
    FullyRestoreCharacter(member);
    DrawStatLine(member, s_raisedStat, highlighted, s_statWindow);
    PushWaitState(WAIT_FRAMES, WAIT_ON_ANY_INPUT, 10, 0);
}

// Runs one frame of the level-up screen, by phase: 0 opens it, 2 picks the
// member and shows their stats, 4 hands out the levels and stat points and
// teaches new skills, 5 does the same without a choice (demons), 6 redraws the
// raised stat, and 1 closes the screen.
RVA(0x00018b60, 0x780)
b16 RunLevelUp(void) {
    Character* member;
    GZ_ENUM_LOCAL(TextEvent, i16) key;
    i16 skill;

    SetStatusRenderMode();
    switch (GetGamePhase()) {
        case LEVEL_UP_PHASE_OPEN:
            s_savedMusic = PlayMusic(0x17, true);
            CloseMessageWindow();
            SetGamePhase(LEVEL_UP_PHASE_PICK_MEMBER);
            AllocScreenSave(s_screenSave);
            CaptureScreenSaveWithState(s_screenSave);
            return false;
        case LEVEL_UP_PHASE_CLOSE:
            switch (GetGameStep()) {
                case LEVEL_UP_CLOSE_FADE:
                    NextGameStep();
                    s_pointPrompt = CloseTextWindow(s_pointPrompt);
                    PushScreenFade(SCREEN_FADE_TO_BLACK, 1);
                    PushWaitState(WAIT_INPUT, WAIT_ON_ANY_INPUT, 0xffff, 0);
                    s_statWindow = CloseTextWindow(s_statWindow);
                    DrawStatusVitals(s_levelUpSlot);
                    return false;
                case LEVEL_UP_CLOSE_RESTORE:
                    ReturnFromGameState();
                    g_rewardExperience = 0;
                    RequestFieldRefresh();
                    LeaveStatusScreen(0);
                    ErasePictureSurface(0x36);
                    ClearStatusPicture();
                    RestoreScreenSave(s_screenSave);
                    FreeScreenSave(s_screenSave);
                    PlayMusic(s_savedMusic, true);
                    return false;
            }
            break;
        case LEVEL_UP_PHASE_PICK_MEMBER:
            CloseMessageWindow();
            EnterStatusScreen(0);
            member = GetRosterCharacter(s_levelUpSlot = FindLevelUpSlot());
            ClearConditionList(GetCharacterConditions(member), s_levelUpCures);
            DrawStatusScreen(s_levelUpSlot);
            s_statWindow = OpenStatListWindow(member);
            SetGamePhase(LEVEL_UP_PHASE_CLOSE);
            SaveGameState();
            SetGamePhase(LEVEL_UP_PHASE_DISTRIBUTE);
            if (!IsHumanCharacter(member)) {
                NextGamePhase();
            }
            FadeScreenAndWait(SCREEN_FADE_FROM_BLACK, 1);
            return false;
        case LEVEL_UP_PHASE_SKIP:
            NextGamePhase();
            return false;
        case LEVEL_UP_PHASE_DISTRIBUTE:
            member = GetRosterCharacter(s_levelUpSlot);
            switch (GetGameStep()) {
                case LEVEL_UP_HUMAN_COUNT_LEVELS:
                    NextGameStep();
                    s_remaining = CountPendingLevels(s_levelUpSlot);
                    return false;
                case LEVEL_UP_HUMAN_PICK_GROWTH:
                    NextGameStep();
                    if (s_remaining == 0 || !CountRaisableStats(member)) {
                        NextGameStep();
                        return false;
                    }
                    PickGrowthStats(member, s_statPicks, member->level + s_remaining);
                    DropTopStatPicks(member, s_statPicks);
                    s_remaining--;
                    return false;
                case LEVEL_UP_HUMAN_APPLY_GROWTH:
                    if (GetGameSub() >= 3) {
                        PrevGameStep();
                        return false;
                    }
                    s_raisedStat = ResolveRaisableStat(member, s_statPicks[GetGameSub()]);
                    NextGameSub();
                    if (s_raisedStat < 0) {
                        PrevGameStep();
                        return false;
                    }
                    ApplyPickedStatGain(member);
                    return false;
                case LEVEL_UP_HUMAN_OPEN_POINT_PICKER:
                    NextGameStep();
                    ResetTextPlaneMenu(s_statWindow, 0, 0);
                    SetTextPlaneHighlightMode(s_statWindow, TEXT_HIGHLIGHT_OUTER);
                    s_remaining = CountPendingLevels(s_levelUpSlot);
                    memset(s_statPicks, 0, sizeof(s_statPicks));
                    if (!CountRaisableStats(member)) {
                        NextGameStep();
                        return false;
                    }
                    ShowStatPointPrompt(s_remaining);
                    return false;
                case LEVEL_UP_HUMAN_DISTRIBUTE_POINTS:
                    key = PollMenuInput(s_statWindow);
                    if (key == TEXT_EVENT_NONE) {
                        break;
                    }
                    if (key == TEXT_EVENT_CHOOSE && !IsStatCapped(member, g_selectedObjectId)) {
                        s_statPicks[g_selectedObjectId]++;
                        member->stats.base[g_selectedObjectId]++;
                        s_remaining--;
                        if (s_pointPrompt != -1) {
                            ShowStatPointPrompt(s_remaining);
                        }
                    } else if (key == TEXT_EVENT_CHOOSE_RIGHT
                               && s_statPicks[g_selectedObjectId] > 0) {
                        s_statPicks[g_selectedObjectId]--;
                        member->stats.base[g_selectedObjectId]--;
                        s_remaining++;
                        if (s_pointPrompt != -1) {
                            ShowStatPointPrompt(s_remaining);
                        }
                    } else {
                        break;
                    }
                    if (s_remaining == 0 || !CountRaisableStats(member)) {
                        NextGameStep();
                    }
                    ClearTextPlaneHighlight(s_statWindow);
                    s_raisedStat = g_selectedObjectId;
                    SaveGameState();
                    SetGamePhase(LEVEL_UP_PHASE_REDRAW_STAT);
                    return false;
                case LEVEL_UP_HUMAN_FINISH_LEVELS:
                    while (CountPendingLevels(s_levelUpSlot)) {
                        member->level++;
                        FinishLevelGain(member);
                    }
                    s_statWindow = CloseTextWindow(s_statWindow);
                    DrawStatusVitals(s_levelUpSlot);
                    RaiseAffiliationLevels(member);
                    FullyRestoreCharacter(member);
                    NextGameStep();
                    if (!CollectLearnableSkills(member, -1)) {
                        ReturnFromGameState();
                        return false;
                    }
                    break;
                case LEVEL_UP_HUMAN_LEARN_SKILL:
                    skill = TakeLearnableSkill(member, s_learnableSkills);
                    if (skill == -1) {
                        ReturnFromGameState();
                        return false;
                    }
                    AddSkill(GetCharacterSkills(member), skill);
                    sprintf(
                        g_scratchBuffer,
                        "%s\202\360\211\357\223\276\202\265\202\275\201I",
                        GetSkillName(skill)
                    ); // %sを会得した！
                    PushMessageBox(0x19, g_scratchBuffer);
                    return false;
            }
            break;
        case LEVEL_UP_PHASE_DISTRIBUTE_DEMON:
            member = GetRosterCharacter(s_levelUpSlot);
            switch (GetGameStep()) {
                case LEVEL_UP_DEMON_CHECK_PENDING:
                    if (!CountPendingLevels(s_levelUpSlot)) {
                        ReturnFromGameState();
                        return false;
                    }
                    NextGameStep();
                    return false;
                case LEVEL_UP_DEMON_APPLY_GROWTH:
                    if (GetGameSub() >= 4) {
                        NextGameStep();
                        return false;
                    }
                    NextGameSub();
                    s_raisedStat = RollWeightedStat(member);
                    if (s_raisedStat == -1) {
                        NextGameStep();
                        return false;
                    }
                    ApplyPickedStatGain(member);
                    return false;
                case LEVEL_UP_DEMON_FINISH_LEVEL:
                    SetGameStep(LEVEL_UP_DEMON_CHECK_PENDING);
                    member->levelBonus += 2;
                    member->level++;
                    FinishLevelGain(member);
                    skill = LearnLevelSkill(member);
                    if (skill) {
                        _snprintf(
                            g_scratchBuffer,
                            0xff,
                            "%s\202\360\211\357\223\276\202\265\202\275\201I",
                            GetSkillName(skill)
                        ); // %sを会得した！
                        PushMessageBox(0x19, g_scratchBuffer);
                        return false;
                    }
                    break;
            }
            break;
        case LEVEL_UP_PHASE_REDRAW_STAT:
            member = GetRosterCharacter(s_levelUpSlot);
            switch (GetGameStep()) {
                case LEVEL_UP_REDRAW_RAISED:
                    NextGameStep();
                    ShowRaisedStat(member, 1);
                    break;
                case LEVEL_UP_REDRAW_NORMAL:
                    NextGameStep();
                    ShowRaisedStat(member, 0);
                    break;
                case LEVEL_UP_REDRAW_RETURN:
                    ReturnFromGameState();
                    return false;
            }
            break;
    }
    return false;
}

RVA(0x000192e0, 0x8c)
void ShowStatPointPrompt(i16 points) {
    if (s_pointPrompt == -1) {
        s_pointPrompt = CreateTextPlane(0x10, 0);
    }
    ClearTextPlane(s_pointPrompt);
    PrintWindowText(
        s_pointPrompt,
        "\203|"
        "\203C\203\223\203g\202\360\220U\202\350\225\252\202\257\202\304\202\255\202\276\202\263"
        "\202\242\n",
        TEXT_ATTR_DEFAULT,
        0,
        true
    );
    sprintf(g_scratchBuffer, "\214\343 %.1d \203|\203C\203\223\203g  \n", points);
    PrintWindowText(s_pointPrompt, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
    RepaintTextPlane(s_pointPrompt, -2);
}

static i16* BuildStatWeightRanges(Character* character, i16* ranges);

RVA(0x00019370, 0x5e)
i16 RollWeightedStat(Character* character) {
    i16 stat;
    i16 draw;
    if (BuildStatWeightRanges(character, s_statPicks) == NULL) {
        return -1;
    }
    draw = rand() * 10000 / RAND_MAX;
    for (stat = 0; stat < STAT_FORTUNE && s_statPicks[stat] < draw; stat++) {
    }
    if (stat >= STAT_FORTUNE) {
        return -1;
    }
    return stat;
}

// @early-stop register allocation: the range cursor, total and stat index
// rotate across ebx, esi and edi. Calls, branches, stores and arithmetic
// align; cursor initialization order does not change the allocation.
RVA(0x000193d0, 0x9a)
static i16* BuildStatWeightRanges(Character* character, i16* ranges) {
    i16 total = 0;
    i16* range = ranges;
    i16 stat;
    stat = 0;
    while (stat < 10) {
        if (IsStatCapped(character, stat)) {
            *range = -1;
        } else {
            if (GetBaseStat(character, stat) == 0) {
                *range = ++total;
            } else {
                *range = total += GetBaseStat(character, stat) * 10;
            }
        }
        stat++;
        range++;
    }
    if (total == 0) {
        return NULL;
    }
    for (stat = 0; stat < 10; stat++) {
        if (ranges[stat] != -1) {
            ranges[stat] = ranges[stat] * 10000 / total;
        }
    }
    return ranges;
}

static i16 AppendLearnableSkills(Character* character, i16 count, i16 source);

RVA(0x00019470, 0x4e)
i16 CollectLearnableSkills(Character* character, i16 source) {
    i16 count;
    if (source == -1) {
        count = AppendLearnableSkills(character, 0, 0);
        if (character->id != HUMAN_KATSURAGI) {
            return count;
        }
        count = AppendLearnableSkills(character, count, 1);
        return AppendLearnableSkills(character, count, 2);
    }
    return AppendLearnableSkills(character, 0, source);
}

RVA(0x000194c0, 0x87)
static i16 AppendLearnableSkills(Character* character, i16 count, i16 source) {
    i16* skills;
    i16 index;
    s_learnableSkills[count] = -1;
    if (character == NULL) {
        return count;
    }
    skills = GetLearnableSkillList(character->id, source);
    if (skills == NULL) {
        return count;
    }
    index = 0;
    while (skills[index] != -1) {
        if (count >= 63) {
            count = 63;
            break;
        }
        s_learnableSkills[count++] = skills[index++];
    }
    s_learnableSkills[count] = -1;
    return count;
}

// Teaches `character` every skill it can learn from `source`; returns how
// many it learned.
RVA(0x00019550, 0x63)
i16 LearnAllSkills(Character* character, i16 source) {
    i16 count = 0;
    i16 skill;

    if (CollectLearnableSkills(character, source) < 1) {
        return 0;
    }
    for (skill = TakeLearnableSkill(character, s_learnableSkills); skill != -1;
         skill = TakeLearnableSkill(character, s_learnableSkills)) {
        count++;
        AddSkill(GetCharacterSkills(character), skill);
    }
    return count;
}

RVA(0x000195c0, 0x199)
void GainLevels(Character* character, i16 count) {
    i16 index;
    i16 stat;
    u8 level;
    if (!character) {
        return;
    }
    if (!IsHumanCharacter(character)) {
        while (count > 0) {
            level = ClampLevel(character->level + 1);
            if (level == character->level) {
                break;
            }
            for (index = 0; index < 4; index++) {
                stat = RollWeightedStat(character);
                if (stat == -1) {
                    break;
                }
                character->stats.base[stat]++;
                FullyRestoreCharacter(character);
            }
            character->level++;
            character->levelBonus += 2;
            FinishLevelGain(character);
            LearnLevelSkill(character);
            count--;
        }
    } else {
        while (count > 0) {
            level = ClampLevel(character->level + 1);
            if (level == character->level || !CountRaisableStats(character)) {
                break;
            }
            PickGrowthStats(character, s_statPicks, character->level + s_remaining);
            DropTopStatPicks(character, s_statPicks);
            s_statPicks[3] = -1;
            for (index = 0; index < 4; index++) {
                stat = ResolveRaisableStat(character, s_statPicks[index]);
                if (stat >= 0) {
                    character->stats.base[stat]++;
                }
            }
            character->level++;
            FinishLevelGain(character);
            RaiseAffiliationLevels(character);
            count--;
        }
        LearnAllSkills(character, -1);
    }
    RaiseExperienceToLevel(character);
}

RVA(0x00019760, 0x40)
char* FormatLevelUpMessage(char* buf, i16 slot) {
    char name[64];
    FormatFullName(name, GetRosterCharacter(slot));
    sprintf(
        buf,
        "%s\202\315\203\214\203\170\203\213\202\252\217\343\202\252\202\301\202\275\n",
        name
    );
    return buf;
}

RVA(0x000197a0, 0x9)
i16 IsOddMapLayer(void) {
    return s_mapLayer & 1;
}

RVA(0x000197b0, 0x3b)
void SetWorldMapSpot(i16 layer, i16 x, i16 y) {
    MapCoord origin;
    s_mapLayer = layer;
    origin = GetLayerOrigin(layer);
    s_savedSpotX = origin.x + x;
    s_savedSpotY = origin.y + y;
}

// Runs one frame of the world map, by phase: 0-2 enter it, 3-4 draw the view
// around the party, 5 waits for a destination, 6 travels (advancing the clock
// and checking events and encounters), 7-8 leave it, 9 leaves for an event or
// an encounter's state, 10 re-enters after one.
RVA(0x000197f0, 0x620)
b16 RunWorldMap(void) {
    MapCoord origin;
    i16 steps;
    i16 state;

    SetPanelRenderMode();
    switch (GetGamePhase()) {
        case WORLD_MAP_PHASE_LOAD:
            ClearSceneSurfaces();
            NextGamePhase();
            s_traveling = false;
            g_party.field.pos.area = MAP_AREA_WORLD_MAP;
            g_party.field.pos.level = 0;
            g_party.field.pos.x = 3;
            g_party.field.pos.y = 3;
            g_party.field.pos.direction = VIEW_NORTH;
            LoadAreaMap(MAP_AREA_WORLD_MAP, 0);
            SetModeFlags(MODE_WORLD_MAP);
            if (g_worldMapRequest > 0) {
                g_worldMapX = s_savedSpotX;
                g_worldMapY = s_savedSpotY;
                MarkWorldMapEventSpot(s_savedSpotX, s_savedSpotY);
            }
            LoadCommandMenuImage();
            LoadWorldMapPlaces();
            LoadWorldMapEvents();
            LoadEncounterTables();
            g_worldMapRequest = 0;
            SetFieldMenuMode(FIELD_MENU_NO_FIGHT_TALK_MAPPING);
        case WORLD_MAP_PHASE_SAVE_VIDEO:
            NextGamePhase();
            SaveVideoState(s_videoState);
        case WORLD_MAP_PHASE_ENTER:
            SetWorldMapActive(1);
            if (g_worldMapRequest < 0) {
                SetGamePhase(WORLD_MAP_PHASE_CLOSE);
                return false;
            }
            if (g_worldMapRequest > 0) {
                g_worldMapX = s_savedSpotX;
                g_worldMapY = s_savedSpotY;
                MarkWorldMapEventSpot(s_savedSpotX, s_savedSpotY);
                g_worldMapRequest = 0;
            }
            NextGamePhase();
            PlayMusic(0, true);
            ClearMaskView();
            ResetMask(1);
            SetFieldStatusBit11(true);
            ClearTextPlane(g_infoPlane);
            SetInfoBarLayout(0);
            RedrawScreen(0, 1);
            FlushStatusRedraw(true);
            StartScreenFadeAndWait(SCREEN_FADE_FROM_BLACK, 1);
            return true;
        case WORLD_MAP_PHASE_LOAD_BLOCKS:
            NextGamePhase();
            LoadWorldMapBlocks(GetWorldMapBlock(g_worldMapX, g_worldMapY));
            return false;
        case WORLD_MAP_PHASE_SCROLL_VIEW:
            NextGamePhase();
            origin = GetWorldMapViewOrigin(g_worldMapX, g_worldMapY);
            ScrollWorldMapView(origin.x, origin.y);
            FlushStatusRedraw(true);
            RefreshInfoBar(1);
            ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 1);
            DiscardWorldMapScreenSave();
            return false;
        case WORLD_MAP_PHASE_WAIT_DESTINATION:
            AllowImmediateInput();
            if (g_worldMapRequest < 0) {
                SetGamePhase(WORLD_MAP_PHASE_FADE_OUT);
                return false;
            }
            if (g_fieldRedrawRequest) {
                g_fieldRedrawRequest = false;
                SetGamePhase(WORLD_MAP_PHASE_ENTER);
                return false;
            }
            if (FindAbleHumanMember() == -1) {
                CloseMessageWindow();
                PushFieldTextScene(0x2a, 0);
                return false;
            }
            TrackWorldMapCursor(s_mapLayer);
            if (ProcessPartyCasualties()) {
                RequestStatusRedraw();
            }
            FlushStatusRedraw(false);
            s_idleFlag = 0;
            if (s_traveling) {
                NextGamePhase();
                if (g_tickElapsed >= CLOCK_UPDATE_MOON) {
                    DrawInfoBar(1, true);
                }
                ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 0);
                return false;
            }
            if (PickWorldMapDestination(s_mapLayer)) {
                s_traveling = true;
                NextGamePhase();
                if (g_tickElapsed >= CLOCK_UPDATE_MOON) {
                    DrawInfoBar(1, true);
                }
                ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 0);
                return false;
            }
            if (g_tickElapsed >= CLOCK_UPDATE_MOON) {
                DrawInfoBar(1, true);
            }
            ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 0);
            FireCountdownEvent();
            return false;
        case WORLD_MAP_PHASE_TRAVEL:
            AllowImmediateInput();
            steps = StepWorldMapTravel(s_mapLayer, 2);
            if (steps == 0) {
                PrevGamePhase();
                s_traveling = false;
            } else {
                AdvanceClock(steps * 5);
                if (TickStepCounter()) {
                    RequestStatusRedraw();
                }
            }
            if (ProcessPartyCasualties()) {
                RequestStatusRedraw();
            }
            FlushStatusRedraw(false);
            RefreshInfoBar(0);
            ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 0);
            if (FindAbleHumanMember() == -1) {
                CloseMessageWindow();
                PushFieldTextScene(0x2a, 0);
                return false;
            }
            if (CheckWorldMapEvent(g_worldMapX, g_worldMapY)) {
                CloseMessageWindow();
                SetGamePhase(WORLD_MAP_PHASE_LEAVE_FOR_STATE);
                s_traveling = false;
                SetGameStep(GAME_STATE_BACKGROUND_SCENE);
                StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
                return false;
            }
            if (RollWorldMapEncounter(g_worldMapX, g_worldMapY) > 0) {
                CloseMessageWindow();
                g_party.field.pos.x = 3;
                g_party.field.pos.y = 3;
                g_party.field.pos.direction = VIEW_NORTH;
                SetGamePhase(WORLD_MAP_PHASE_LEAVE_FOR_STATE);
                SetGameStep(GAME_STATE_FIELD);
                StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
                return false;
            }
            break;
        case WORLD_MAP_PHASE_FADE_OUT:
            NextGamePhase();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            return false;
        case WORLD_MAP_PHASE_CLOSE:
            SetWorldMapActive(0);
            FreeWorldMapScreenSave();
            ResetWorldMapBlocks(1);
            FreeWorldMapPlaces();
            FreeCommandMenuImage();
            FreeFieldImageCache();
            RestoreVideoState(s_videoState);
            SetSubscreenActive(0);
            SetGameState(GAME_STATE_FIELD_EXPLORATION);
            ClearModeFlags(MODE_WORLD_MAP);
            SetFieldMenuMode(FIELD_MENU_ALL);
            RecordWarpInLeader();
            return false;
        case WORLD_MAP_PHASE_LEAVE_FOR_STATE:
            ClearLayerSurface(SCREEN_LAYER_AUTOMAP);
            CancelLayerDrag();
            state = GetGameStep();
            NextGamePhase();
            SetWorldMapActive(0);
            FreeWorldMapScreenSave();
            ResetWorldMapBlocks(1);
            PushGameState(state);
            return false;
        case WORLD_MAP_PHASE_REENTER:
            LoadWorldMapEvents();
            LoadEncounterTables();
            SetGamePhase(WORLD_MAP_PHASE_ENTER);
            ClearSceneSurfaces();
            ClearSelectedHotspot();
            break;
    }
    return false;
}

RVA(0x00019e10, 0x26)
i16 TickStepCounter(void) {
    s_idleFlag++;
    if (s_idleFlag < 50) {
        return 0;
    }
    s_idleFlag = 0;
    return TickPartySteps();
}

RVA(0x00019e40, 0x12)
i16 GetPickerSelection(void) {
    if (!s_partyPicker) {
        return -1;
    }
    return s_partyPicker->plane;
}

RVA(0x00019e60, 0x1b)
i16 RunPickerMenu(MenuBox* menu) {
    i16 result = RunMenu(menu);
    if (result <= 0) {
        return result - 1;
    }
    return g_selectedObjectId;
}

RVA(0x00019e80, 0x17)
MenuBox* ClosePickerMenu(MenuBox* menu) {
    s_partyPickerMode = PARTY_PICKER_ALL;
    return DestroyMenuBox(menu);
}

RVA(0x00019ea0, 0x9e)
i16 RunPartyPicker(i16 command) {
    PartyMemberList* entries;
    i16 result;
    if (command != PARTY_PICKER_COMMAND_CONTINUE) {
        s_partyPicker = ClosePickerMenu(s_partyPicker);
    }
    if (command >= PARTY_PICKER_COMMAND_CONTINUE) {
        if (!s_partyPicker) {
            if (!CountPickablePartyMembers()) {
                return PARTY_PICKER_RESULT_CANCELLED;
            }
            entries = ListPickableMembers(NULL, PARTY_SIZE, true);
            if (!entries->count) {
                FreeBlock(entries);
                return PARTY_PICKER_RESULT_CANCELLED;
            }
            s_partyPicker = OpenPartyPicker(entries);
        }
        result = RunPickerMenu(s_partyPicker);
        if (result != PARTY_PICKER_RESULT_NONE && result != PARTY_PICKER_RESULT_CANCELLED) {
            s_partyPicker = ClosePickerMenu(s_partyPicker);
            return g_selectedObjectId;
        }
    }
    return PARTY_PICKER_RESULT_NONE;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00019f40, 0xc)
void SetPartyPickerMode(GZ_ENUM_PARAM(PartyPickerMode, i16) mode) {
    s_partyPickerMode = mode;
}

RVA(0x00019f50, 0x52)
MenuBox* OpenPartyPicker(PartyMemberList* entries) {
    MenuBox* menu = CreateMenuBox(NULL, 5, 2);
    SetMenuItems(menu, 7, entries, entries->count, PartyPickerHandler);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, true);
    if (s_partyPickerMode == PARTY_PICKER_ALL) {
        menu->list->flags |= 2;
    }
    menu->flags |= 0x1e;
    return menu;
}

RVA(0x00019fb0, 0xd4)
void PartyPickerHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    PartyMemberList* entries = menu->items.memberList;
    Character* character;
    i16 enabled;
    switch (event) {
        case MENU_EVENT_ADD_ROW:
            enabled = true;
            character = GetCharacterById(entries->ids[index]);
            switch (s_partyPickerMode) {
                case PARTY_PICKER_ALL:
                    break;
                case PARTY_PICKER_USABLE_SKILLS:
                    enabled = CountUsableMemberSkills(character, true);
                    break;
                case PARTY_PICKER_HUMANS:
                    if (!IsHumanCharacter(character)) {
                        enabled = false;
                    }
                    break;
            }
            FormatFullName(g_scratchBuffer, character);
            if (enabled) {
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    TEXT_ATTR_FLAG1
                        | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                    entries->ids[index],
                    MENU_LINE_NORMAL
                );
            } else {
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
                    entries->ids[index],
                    MENU_LINE_DISABLED
                );
            }
            break;
        case MENU_EVENT_DESTROY:
            menu->items.memberList = FreeBlock(entries);
            menu->itemCount = 0;
            break;
    }
}

RVA(0x0001a090, 0x3b)
void SetCellMark(i16 area, i16 level, i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction) {
    s_markedArea = area;
    s_markedLevel = level;
    s_markedX = x;
    s_markedY = y;
    s_markedDirection = direction;
}

RVA(0x0001a0d0, 0x32)
void SaveFieldPosition(void) {
    SetCellMark(
        g_party.field.pos.area,
        g_party.field.pos.level,
        g_party.field.pos.x,
        g_party.field.pos.y,
        g_party.field.pos.direction
    );
}

RVA(0x0001a110, 0x67)
GZ_ENUM_RETURN(CellMarkMatch, i16) IsOnCellMark(i16 checkDirection) {
    if (g_party.field.pos.x == s_markedX && g_party.field.pos.y == s_markedY
        && g_party.field.pos.area == s_markedArea && g_party.field.pos.level == s_markedLevel) {
        if (checkDirection && g_party.field.pos.direction != s_markedDirection) {
            return CELL_MARK_FACING_DIFFERS;
        }
        return CELL_MARK_MATCH;
    }
    return CELL_MARK_OFF_CELL;
}

RVA(0x0001a180, 0x7)
i16 GetCurrentRoomCode(void) {
    return s_currentRoomCode;
}

RVA(0x0001a190, 0x12)
i16 SetCurrentRoomCode(i16 code) {
    i16 previous = s_currentRoomCode;
    s_currentRoomCode = code;
    return previous;
}

RVA(0x0001a1b0, 0x1d)
void InitFieldPanels(void) {
    u32 image = LoadMenuImage(1);
    InitWorldPanel();
    SetFieldPanelImage(image);
}

RVA(0x0001a1d0, 0xa)
void ResetSubscreen(void) {
    s_subscreenActive = 0;
}

RVA(0x0001a1e0, 0x12)
i16 SetSubscreenActive(i16 active) {
    i16 old = s_subscreenActive;
    s_subscreenActive = active;
    return old;
}

RVA(0x0001a200, 0x40)
MenuBox* OpenItemListMenu(void) {
    ItemStackList* entries = CopyBagEntries(0, 64, NULL);
    MenuBox* menu = CreateMenuBox(NULL, 5, 2);
    menu->flags |= 0x1e;
    SetMenuItems(menu, 7, entries, GetItemListCount(entries), ItemListMenuHandler);
    return menu;
}

#define ItemUseInvokesSkill(kind) ((kind) == ITEM_KIND_WEAPON || (kind) == ITEM_KIND_ACCESSORY)

static __inline void AddItemUseMenuLine(MenuBox* menu, i16 item, i16 disabled) {
    AddMenuLine(
        menu->plane,
        g_scratchBuffer,
        disabled
            ? TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
            : TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_YELLOW, TEXT_COLOR_BLACK),
        item,
        disabled
    );
}

RVA(0x0001a240, 0x1bc)
void ItemListMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    ItemStackList* entries = menu->items.itemList;
    ItemRecord* record;
    switch (event) {
        case MENU_EVENT_ADD_ROW:
            sprintf(
                g_scratchBuffer,
                "%-20.20s%2d",
                GetLoadedRecordName(GetItemStackItem(GetItemListEntry(entries, index))),
                GetItemStackCount(GetItemListEntry(entries, index))
            );
            if ((GetItemStackItem(GetItemListEntry(entries, index)) == ITEM_KUSHINADA_JAR
                 && IsEventFlagSet(EVENT_FLAG_BANK_ITEM_EFFECTS, ITEM_EFFECT_KUSHINADA_JAR_USED))
                || (GetItemStackItem(GetItemListEntry(entries, index)) == ITEM_SOMA_CUP
                    && IsEventFlagSet(EVENT_FLAG_BANK_ITEM_EFFECTS, ITEM_EFFECT_SOMA_CUP_USED))) {
                AddItemUseMenuLine(
                    menu,
                    GetItemStackItem(GetItemListEntry(entries, index)),
                    MENU_LINE_DISABLED
                );
                return;
            }
            record = GetLoadedRecord(GetItemStackItem(GetItemListEntry(entries, index)));
            event = GetItemUseModes(record);
            if (ItemUseInvokesSkill(record->kind)) {
                event = GetSkillUseModes(GetSkillView(GetItemSkillId(record)));
            }
            if (CheckSkillArea(GetItemSkillId(record)) != SKILL_AREA_ALLOWED) {
                AddItemUseMenuLine(
                    menu,
                    GetItemStackItem(GetItemListEntry(entries, index)),
                    MENU_LINE_DISABLED
                );
                return;
            }
            if (IsSkillUsableNow(event) != 1) {
                AddItemUseMenuLine(
                    menu,
                    GetItemStackItem(GetItemListEntry(entries, index)),
                    MENU_LINE_DISABLED
                );
                return;
            }
            AddItemUseMenuLine(menu, GetItemStackItem(GetItemListEntry(entries, index)), 0);
            return;
        case MENU_EVENT_BEGIN_PAGE:
            AddMenuLine(
                menu->plane,
                "<\203A\203C\203e\203\200>",
                TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_BLACK),
                0,
                MENU_LINE_DISABLED
            );
            return;
        case MENU_EVENT_DESTROY:
            menu->items.itemList = FreeBlock(entries);
            menu->itemCount = 0;
            return;
    }
}

static __inline void SelectItemUserAsTarget(void) {
    g_targetId = PartyCombatantId(s_usePosition);
    NextGamePhase();
}

// Runs the field item-use flow one phase: open the item list, pick an item,
// pick its target (a skill-bearing item, kind 11 or 19, targets as its skill),
// then hand the user's pick to the action prompt. Returns 0.
// @early-stop instruction scheduling: in the final phase's skill arm retail
// reads the pick flags into bl before storing pickTarget; cl here keeps the
// flags update after the store, the statement order PC-98 shows.
RVA(0x0001a400, 0x3e0)
b16 RunItemUse(void) {
    ItemRecord* record;
    Character* user;
    i16 flags;
    i16 range;
    i16 kind;
    i16 picked;
    i16 position;

    switch (GetGamePhase()) {
        case ITEM_USE_PHASE_OPEN:
            NextGamePhase();
            NextGamePhase();
            s_itemMenu = OpenItemListMenu();
            HideScreenLayer(1);
            return false;

        case ITEM_USE_PHASE_CLOSE:
            ReturnFromGameState();
            s_itemMenu = CloseListMenu(s_itemMenu);
            RestoreSwappedMember();
            s_useMemberId = -1;
            return false;

        case ITEM_USE_PHASE_PICK_ITEM:
            picked = RunListMenu(s_itemMenu);
            if (picked == LIST_MENU_CANCELLED) {
                PrevGamePhase();
            }
            if (picked < 0) {
                break;
            }
            s_useItem = g_selectedObjectId;
            DecodeItemRecord(&g_loadedItem, s_useItem);
            NextGamePhase();
            s_usePosition = FindFirstAbleMemberPosition();
            return false;

        case ITEM_USE_PHASE_PICK_TARGET:
            record = GetLoadedRecord(s_useItem);
            kind = record->kind;
            if (ItemUseInvokesSkill(kind)) {
                flags = GetSkillTargetFlags(GetItemSkillId(record));
                range = GetSkillAttackRange(GetItemSkillId(record));
            } else {
                flags = GetItemTargetFlags(record);
                range = GetItemAttackRange(record);
            }
            if (TargetFlagsSelectSelf(flags)) {
                SelectItemUserAsTarget();
                return false;
            }
            if (TargetFlagsSelectActorGroup(flags)) {
                SelectItemUserAsTarget();
                return false;
            }
            if (flags == TARGET_SELECT_FIELD_OR_ROSTER) {
                picked = RunPickTargetWindow(
                    0,
                    range,
                    TARGET_PICK_FIELD_OBJECT | TARGET_PICK_ROSTER_LIST,
                    GetPartyRosterId(s_usePosition)
                );
            } else if (flags == TARGET_SELECT_ROSTER_ONLY) {
                picked = RunPickTargetWindow(
                    0,
                    range,
                    TARGET_PICK_ROSTER_LIST,
                    GetPartyRosterId(s_usePosition)
                );
            } else if (flags == TARGET_SELECT_PARTY_OR_ROSTER) {
                picked = RunPickTargetWindow(
                    0,
                    range,
                    TARGET_PICK_PARTY_SLOT | TARGET_PICK_ROSTER_LIST,
                    GetPartyRosterId(s_usePosition)
                );
            } else {
                flags = 0;
                picked = RunPickTargetWindow(
                    0,
                    range,
                    TARGET_PICK_FIELD_OBJECT | TARGET_PICK_PARTY_SLOT,
                    GetPartyRosterId(s_usePosition)
                );
            }
            if (picked == TARGET_PICK_CANCELLED) {
                PrevGamePhase();
                return false;
            }
            if (picked == TARGET_PICK_WAITING) {
                break;
            }
            NextGamePhase();
            if (flags) {
                g_selectedObjectId = SwapInForPick(s_usePosition, g_selectedObjectId);
            }
            g_targetId = g_selectedObjectId;
            return false;

        case ITEM_USE_PHASE_DESTROY_MENU:
            NextGamePhase();
            s_itemMenu = DestroyMenuBox(s_itemMenu);
            return false;

        case ITEM_USE_PHASE_PROMPT_ACTION:
            NextGamePhase();
            position = FindPartyPositionOfId(s_useMemberId);
            user = GetPartyCharacter(position);
            record = GetLoadedRecord(s_useItem);
            kind = record->kind;
            if (ItemUseInvokesSkill(kind)) {
                g_actorId = PartyCombatantId(position);
                user->pickObject = g_targetId;
                user->pickRole = PICK_ROLE_MAGIC;
                g_actionId = GetItemSkillId(record);
                user->pickTarget = GetItemSkillId(record);
                user->pickFlags |= PICK_ITEM_SKILL;
                user->pickItem = s_useItem;
            } else {
                g_actorId = PartyCombatantId(position);
                user->pickObject = g_targetId;
                user->pickRole = PICK_ROLE_ITEM;
                g_actionId = s_useItem;
                user->pickTarget = s_useItem;
            }
            PushFieldUsePrompt();
            return false;

        case ITEM_USE_PHASE_FINISH:
            SetGamePhase(ITEM_USE_PHASE_CLOSE);
            break;
    }
    return false;
}

// The party position of the first of the sixteen member ids in the party
// whose conditions let it act; position of id 0 when none can.
RVA(0x0001a7e0, 0x50)
i16 FindFirstAbleMemberPosition(void) {
    i16 id;
    i16 slot;

    for (id = 0; id < 16; id++) {
        slot = RosterSlotOfId(id);
        if (slot >= 0
            && !GetPickBlockingCondition(GetCharacterConditions(GetRosterCharacter(slot)))) {
            return FindPartyPositionOfId(id);
        }
    }
    return FindPartyPositionOfId(0);
}

RVA(0x0001a830, 0x30)
i16 CancelItemTargetMenu(i16 command) {
    if (command == -1) {
        s_itemMenu = DestroyMenuBox(s_itemMenu);
    }
    return g_selectedObjectId;
}

RVA(0x0001a860, 0x10)
void SetUseMemberId(i16 id) {
    s_useMemberId = id;
}
