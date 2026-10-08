// @identity-TODO: the owning TU is unproven; this unit holds the field
// exploration state's span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <File/DataTableId.h>
#include <Game/AbortFlag.h>
#include <Game/Analyze.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/Automap.h>
#include <Game/Clock.h>
#include <Game/Field.h>
#include <Game/FieldActor.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/LevelUp.h>
#include <Game/MapArea.h>
#include <Game/ModeFlags.h>
#include <Game/MoveCommand.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/PartyStep.h>
#include <Game/Scene.h>
#include <Game/StateStack.h>
#include <Game/TreasureBox.h>
#include <Game/WaitState.h>
#include <Game/WorldMap.h>
#include <Gfx/Background.h>
#include <Gfx/ImageHandle.h>
#include <Gfx/Render.h>
#include <Gfx/Scene.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenMode.h>
#include <Input/Mouse.h>
#include <Mem/Handle.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/ScenarioFlag.h>
#include <Script/ScriptVars.h>
#include <Sound/Sound.h>
#include <Ui/Hotspot.h>
#include <Ui/Message.h>
#include <Util/BitChangeMode.h>
#include <Util/BitSet.h>
#include <Util/Debug.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

DATA(0x00091210)
u8 g_leftFrontWalls[4][3];

DATA(0x000912f0)
u8 g_rightFrontWalls[4][3];

// The level event bits (one per level, MarkLevelEvent).
DATA(0x0007b740)
static u8 s_levelEvents[0x20] = {0};

// The direction the party faces after returning to the field (-1: find the
// exit it came through).
DATA(0x0007b760)
static GZ_ENUM_STORAGE(ViewDirection, i16) s_returnDirection = VIEW_NORTH;

// The return point: the field position restored when the field is re-entered.
DATA(0x0007b764)
static i16 s_returnLevel = 0;

DATA(0x0007b768)
static GZ_ENUM_STORAGE(MapAreaId, i16) s_returnArea = MAP_AREA_HATSUDAI_SHELTER;

DATA(0x0007b76c)
static i16 s_returnY = 0;

DATA(0x0007b770)
static i16 s_returnX = 0;

DATA(0x0007b774)
i16 g_pendingTalk = 0;

// Set to leave through the return point instead of re-entering the area.
DATA(0x0007b778)
static b16 s_leaveToReturnPoint = false;

// Set when the room map must be rebuilt.
DATA(0x0007b77c)
static i16 s_rebuildRoom = 0;

// @identity-TODO: when clear on return, the party takes one step out of the
// exit it came through; a cell event sets it.
DATA(0x0007b780)
static b16 s_stayOnExit = false;

// A sound effect to play once the field is shown.
DATA(0x0007b784)
static i16 s_pendingSound = 0;

// Step counters: every third step the party takes step damage
// (TickStepDamage) and runs its step effects (TickFieldSteps).
DATA(0x0007b788)
static i16 s_damageSteps = 0;

DATA(0x0007b78c)
static i16 s_fieldSteps = 0;

// @identity-TODO: set while a cell event runs; the field state clears it on
// every frame and hands its complement to script register 0 after a battle.
DATA(0x0007b790)
static b16 s_eventRunning = false;

// A sound effect to play when a cell event ends.
DATA(0x0007b794)
static i16 s_eventSound = 0;

// Set by a complete LoadFieldMemory: the loaded event states and level bits
// survive the next reset (TestLevelEvent drops it).
DATA(0x0007b798)
static b16 s_keepEvents = false;

// @identity-TODO: retained from the load request, with no known reader.
DATA(0x0007b79c)
static i16 s_fieldImageMode = 0;

// @identity-TODO: whether an actor is vanishing this frame (commands wait).
DATA(0x0007b7a0)
static i16 s_actorVanishing = 0;

// The queue of automatic moves (a byte per move in a memory handle): its
// capacity, read and write positions.
DATA(0x0007b7a4)
static i16 s_autoMoveCapacity = 0;

DATA(0x0007b7a8)
static i16 s_autoMoveRead = 0;

DATA(0x0007b7ac)
static i16 s_autoMoveCount = 0;

// The saved point (with g_party.savedDirection) the party is put back on after a
// scene.
DATA(0x0007b7b0)
static i16 s_savedX = 0;

DATA(0x0007b7b4)
static i16 s_savedY = 0;

// @identity-TODO: the three cache slots have no loader in this build.
DATA(0x0007b7b8)
static u32 s_fieldImageCacheA = 0;

DATA(0x0007b7bc)
static u32 s_fieldImageCacheB = 0;

DATA(0x0007b7c0)
static u32 s_fieldImageCacheC = 0;

DATA(0x0007b7c4)
static u32 s_fieldImage = 0;

// The field event table (data file 10).
DATA(0x0007b7c8)
static i32 s_eventTable = 0;

DATA(0x0007b7cc)
static i32 s_autoMoves = 0;

// The object event states (a byte per event: 0 none, 1 raised, 3 queued,
// 2 done).
DATA(0x0007b7d0)
static i32 s_eventStates = 0;

#define GetObjectEventState(event) ((u8*)HandleWritePtr(s_eventStates) + (event))

// Counts frames so the enemies act on every fourth.
DATA(0x0007b7d4)
static i16 s_enemyTick = 0;

DATA(0x00091220)
u8 g_centerFrontWalls[5];

DATA(0x00091230)
u8 g_rightSideWalls[5][3];

DATA(0x000912e0)
u8 g_leftSideWalls[5][3];

DATA(0x0006864c)
static i16 s_fieldImageCacheKey = -1;

DATA(0x00068650)
static i16 s_fieldImageCacheVariant = -1;

// Each wall contributes a 16-byte mask within these 48-byte work areas.
DATA(0x00091260)
u8 g_leftViewOcclusion[48];

DATA(0x000912a0)
u8 g_rightViewOcclusion[48];

// Saves the party's cell as the saved point (its direction goes to
// g_party.savedDirection separately).
RVA(0x00012260, 0x1b)
void SaveReturnPoint(void) {
    s_savedX = g_party.field.pos.x;
    s_savedY = g_party.field.pos.y;
}

// Runs the party's move: a command below 4 starts a step in that direction
// (relative to the facing), 4..6 a turn by command - 3 quarter turns (6: one
// back). Returns 1 when the step or turn completes, 2 when the step is
// blocked (with the bump sound), else 0.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00012280, 0x231)
GZ_ENUM_RETURN(PartyMoveOutcome, i16) AdvancePartyMove(i16 command) {
    i16 wall;
    i16 step;
    i16 x;
    i16 y;
    for (;;) {
        switch (g_party.field.moveState) {
            case FIELD_MOVE_IDLE:
                if (command < MOVE_TURN_RIGHT) {
                    g_party.field.moveCommand = command;
                    g_party.field.moveState = FIELD_MOVE_STEPPING;
                } else {
                    command -= 3;
                    g_party.field.moveState = FIELD_MOVE_TURNING;
                    if (command == 3) {
                        g_party.field.turnsLeft = -1;
                    } else {
                        g_party.field.turnsLeft = command;
                    }
                }
                command = 0;
                continue;
            case FIELD_MOVE_STEPPING:
                SaveReturnPoint();
                g_party.field.moveState = FIELD_MOVE_IDLE;
                if (!IsStepBarred(
                        g_party.field.pos.x,
                        g_party.field.pos.y,
                        g_party.field.pos.direction,
                        g_party.field.moveCommand
                    )) {
                    wall = GetCellWall(
                        g_party.field.pos.direction,
                        g_party.field.moveCommand,
                        RevealAreaMapAt(g_party.field.pos.x, g_party.field.pos.y)
                    );
                    if (((wall == WALL_KIND_DOOR || wall == WALL_KIND_FLAG_BARRED_DOOR)
                         && g_party.field.moveCommand != MOVE_FORWARD)
                        || WallStops(wall, WALL_STOP_MOVEMENT) == WALL_STOP_SOLID) {
                        PlaySoundEffect(8);
                        return PARTY_MOVE_BLOCKED;
                    }
                    if (WallStops(wall, WALL_STOP_MOVEMENT)) {
                        PlayWallEffect();
                    }
                    AdvanceClock(RandomUpTo(4) + 3);
                    x = g_party.field.pos.x;
                    y = g_party.field.pos.y;
                    StepMapCoord(&x, &y, g_party.field.pos.direction, g_party.field.moveCommand);
                    g_party.field.pos.x = x;
                    g_party.field.pos.y = y;
                    MarkAutomapCell(g_party.field.pos.area, g_party.field.pos.level, x, y);
                    g_party.savedDirection = OppositeDirection(
                        TurnDirection(g_party.field.pos.direction, g_party.field.moveCommand)
                    );
                    PlaySoundEffect(0xc);
                    return PARTY_MOVE_DONE;
                }
                ShowMessage(
                    "\224\340\202\315\203\215\203b\203N\202\263\202\352\202\304\202\242\202\351",
                    0x3c
                );
                PlaySoundEffect(8);
                return PARTY_MOVE_BLOCKED;
            case FIELD_MOVE_TURNING:
                step = g_party.field.turnsLeft < 0 ? -1 : 1;
                g_party.field.pos.direction = TurnDirection(g_party.field.pos.direction, step);
                g_party.field.turnsLeft -= step;
                if (g_party.field.turnsLeft == 0) {
                    g_party.field.moveState = FIELD_MOVE_IDLE;
                    return PARTY_MOVE_DONE;
                }
                break;
        }
        return PARTY_MOVE_IN_PROGRESS;
    }
}

RVA(0x000124c0, 0x24)
void SetSavedPoint(i16 x, i16 y, i16 direction) {
    s_savedX = x;
    s_savedY = y;
    g_party.savedDirection = direction;
}

RVA(0x000124f0, 0x15)
void SetPartyDirection(i32 direction) {
    g_party.field.pos.direction = direction;
    TickStepDamage();
    TickFieldSteps();
}

RVA(0x00012510, 0xbf)
void CommitPartyStep(void) {
    i16 x;
    i16 y;
    AdvanceClock(RandomUpTo(4) + 3);
    x = g_party.field.pos.x;
    y = g_party.field.pos.y;
    StepMapCoord(&x, &y, g_party.field.pos.direction, g_party.field.moveCommand);
    g_party.field.pos.x = x;
    g_party.field.pos.y = y;
    MarkAutomapCell(g_party.field.pos.area, g_party.field.pos.level, x, y);
    g_party.savedDirection =
        OppositeDirection(TurnDirection(g_party.field.pos.direction, g_party.field.moveCommand));
    PlaySoundEffect(0xc);
    TickStepDamage();
    TickFieldSteps();
}

// Moves the party back to the saved point.
RVA(0x000125d0, 0x2d)
void RestoreSavedPoint(void) {
    g_party.field.pos.x = s_savedX;
    g_party.field.pos.y = s_savedY;
    g_party.field.pos.direction = g_party.savedDirection;
    RebuildViewScene();
}

RVA(0x00012600, 0xde)
GZ_ENUM_RETURN(PartyStepResult, i16) StepParty(GZ_ENUM_PARAM(MoveCommand, i16) direction) {
    i16 wall;
    if (FindObjectAtParty() >= 0) {
        return STEP_BLOCKED;
    }
    g_party.field.moveCommand = direction;
    SaveReturnPoint();
    g_party.field.moveState = FIELD_MOVE_IDLE;
    if (!IsStepBarred(
            g_party.field.pos.x,
            g_party.field.pos.y,
            g_party.field.pos.direction,
            g_party.field.moveCommand
        )) {
        wall = GetCellWall(
            g_party.field.pos.direction,
            g_party.field.moveCommand,
            RevealAreaMapAt(g_party.field.pos.x, g_party.field.pos.y)
        );
        if (((wall == WALL_KIND_DOOR || wall == WALL_KIND_FLAG_BARRED_DOOR)
             && g_party.field.moveCommand != MOVE_FORWARD)
            || WallStops(wall, WALL_STOP_MOVEMENT) == WALL_STOP_SOLID) {
            PlaySoundEffect(8);
            return STEP_BLOCKED;
        }
        return WallStops(wall, WALL_STOP_MOVEMENT) ? STEP_DOOR : STEP_WALK;
    }
    ShowMessage("\224\340\202\315\203\215\203b\203N\202\263\202\352\202\304\202\242\202\351", 0x3c);
    PlaySoundEffect(8);
    return STEP_BLOCKED;
}

// Frees the queued automatic moves.
RVA(0x000126e0, 0x28)
void FreeAutoMoves(void) {
    s_autoMoves = FreeHandle(s_autoMoves);
    s_autoMoveCapacity = 0;
    s_autoMoveRead = 0;
    s_autoMoveCount = 0;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00012710, 0xf)
void ResetAutoMoves(void) {
    s_autoMoves = 0;
    FreeAutoMoves();
}

// Makes room for `more` automatic moves.
RVA(0x00012720, 0x3c)
void GrowAutoMoves(i16 more) {
    if (!s_autoMoves) {
        s_autoMoveRead = 0;
        s_autoMoveCount = 0;
    }
    s_autoMoveCapacity += more;
    s_autoMoves = ResizeHandle(s_autoMoves, s_autoMoveCapacity);
}

RVA(0x00012760, 0x3e)
void PushAutoMove(u8 move) {
    u8* moves;
    if (s_autoMoveCount >= s_autoMoveCapacity) {
        GrowAutoMoves(1);
    }
    moves = HandleWritePtr(s_autoMoves);
    moves[s_autoMoveCount++] = move;
}

RVA(0x000127a0, 0x14)
b16 HasAutoMoves(void) {
    return s_autoMoveRead < s_autoMoveCount;
}

// The next queued move (0xff for none); the queue is freed once empty.
RVA(0x000127c0, 0x58)
u8 PopAutoMove(void) {
    u8 move;
    u8* moves;
    if (!s_autoMoves) {
        return 0xff;
    }
    if (!HasAutoMoves()) {
        FreeAutoMoves();
        return 0xff;
    }
    moves = HandleReadPtr(s_autoMoves);
    move = moves[s_autoMoveRead++];
    if (!HasAutoMoves()) {
        FreeAutoMoves();
    }
    return move;
}

// Sets the sound effect played once the field is shown; returns the old one.
RVA(0x00012820, 0x12)
i16 SetPendingSound(i16 sound) {
    i16 old = s_pendingSound;
    s_pendingSound = sound;
    return old;
}

RVA(0x00012840, 0x12)
i16 SetFieldBusy(i16 busy) {
    i16 old = s_actorVanishing;
    s_actorVanishing = busy;
    return old;
}

RVA(0x00012860, 0x7)
i16 GetFieldBusy(void) {
    return s_actorVanishing;
}

RVA(0x00012870, 0xc)
i16 RequestTalk(void) {
    return g_pendingTalk = true;
}

RVA(0x00012880, 0x53)
void SetReturnPoint(
    GZ_ENUM_PARAM(MapAreaId, i16) area,
    i16 level,
    i16 x,
    i16 y,
    GZ_ENUM_PARAM(ViewDirection, i16) direction
) {
    s_returnArea = area;
    s_returnLevel = level;
    s_returnX = x;
    s_returnY = y;
    s_returnDirection = direction;
    s_leaveToReturnPoint = true;
    ResetFieldObjects();
    SetSelectedHotspot(-1);
}

RVA(0x000128e0, 0xc)
void SetRebuildRoom(i16 rebuild) {
    s_rebuildRoom = rebuild;
}

RVA(0x000128f0, 0x32)
void MovePartyTo(i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction) {
    g_party.field.pos.x = x;
    g_party.field.pos.y = y;
    g_party.field.pos.direction = direction;
    SetRebuildRoom(1);
    RebuildViewScene();
}

// Copies the return point into `out`; returns the pending world-map request.
RVA(0x00012930, 0x41)
GZ_ENUM_RETURN(WorldMapRequest, i16) GetReturnPoint(ReturnPoint* out) {
    out->area = s_returnArea;
    out->level = s_returnLevel;
    out->x = s_returnX;
    out->y = s_returnY;
    out->direction = s_returnDirection;
    return g_worldMapRequest;
}

// Records the return point in the roster leader (see Character.returnPosition).
RVA(0x00012980, 0x38)
void RecordWarpInLeader(void) {
    CharacterCore* leader = GetRosterCharacter(ROSTER_LEADER);
    SetSavedMapPosition(
        &leader->returnPosition,
        s_returnArea,
        s_returnLevel,
        s_returnX,
        s_returnY,
        s_returnDirection
    );
}

// Saves the cell one step out of the exit the party stands on (and that
// direction) as the saved point.
RVA(0x000129c0, 0x59)
void SetReturnPointAhead(void) {
    i16 x = g_party.field.pos.x;
    i16 y = g_party.field.pos.y;
    i16 direction = FindExitDirection(x, y);
    direction &= 3;
    StepMapCoord(&x, &y, direction, MOVE_FORWARD);
    SetSavedPoint(x, y, direction);
}

// The first side of x/y with a door (wall kind 1), else the first open side;
// -1 for none.
RVA(0x00012a20, 0x58)
GZ_ENUM_RETURN(ViewDirection, i16) FindExitDirection(i16 x, i16 y) {
    i16 cell = RevealAreaMapAt(x, y);
    i16 side;
    for (side = 0; side < 4; side++) {
        if (GetCellWall(VIEW_NORTH, side, cell) == WALL_KIND_DOOR) {
            return side;
        }
    }
    for (side = 0; side < 4; side++) {
        if (GetCellWall(VIEW_NORTH, side, cell) == WALL_KIND_NONE) {
            return side;
        }
    }
    return VIEW_NONE;
}

// With a right click pending off the navigation pad and the automap allowed,
// makes the object under the selected hotspot the analyze target (one
// demon-interaction training point for the player); returns its index or
// FIELD_OBJECT_INDEX_NONE.
RVA(0x00012a80, 0x78)
i16 PickAnalyzeTarget(void) {
    i16 index;
    CharacterCore* target;
    if (!HasPendingNonNavigationRightClick()) {
        return FIELD_OBJECT_INDEX_NONE;
    }
    if (!CanOpenAutomap()) {
        return FIELD_OBJECT_INDEX_NONE;
    }
    index = GetSelectedHotspotObject();
    if (index == FIELD_OBJECT_INDEX_NONE) {
        return index;
    }
    ClearMouseClicks();
    target = GetFieldActorCore(GetFieldActor(index));
    if (!target) {
        return FIELD_OBJECT_INDEX_NONE;
    }
    ClearPendingNonNavigationRightClick();
    SetAnalyzeTarget(target);
    AddTrainingPoints(&GetCharacters()->core, BATTLE_GROUP_DEMON_INTERACTION, 1);
    return index;
}

// The next queued move, or -1.
RVA(0x00012b00, 0x13)
i16 NextAutoMove(void) {
    i16 move = PopAutoMove();
    if (move == 0xff) {
        move = -1;
    }
    return move;
}

// @dead-code
// Zero-ref: no direct call/jmp, relocated reference or data slot reaches it.
// @identity-TODO: the distinct role of this duplicate state accessor is unproven.
RVA(0x00012b20, 0x7)
i16 GetFieldExplorationActive(void) {
    return g_fieldBattleActive;
}

// Runs a treasure box's scene: the box cell's script, then the box opens.
RVA(0x00012b30, 0x53)
void StartBoxScene(TreasureBox* box) {
    SceneScript script;
    SetGamePhase(FIELD_PHASE_EXPLORE);
    script = BeginBoxScene(box);
    PushFieldTextScene(script.script, script.entry);
    CloseFieldWindows();
    OpenTreasureBox(box);
    g_fieldBattleActive = false;
}

// Runs an NPC's scene script.
RVA(0x00012b90, 0x48)
void StartNpcScene(AreaNpc* npc) {
    SceneScript script;
    SetGamePhase(FIELD_PHASE_EXPLORE);
    script = GetNpcScript(npc);
    PushFieldTextScene(script.script, script.entry);
    CloseFieldWindows();
    g_fieldBattleActive = false;
}

// Runs the pending talk: picks a party member (none: the talk is dropped and
// -1 returned; aborted: 0), then starts the selected object's talk scene and
// gives the player a training point of kind 3; returns 1.
RVA(0x00012be0, 0xb3)
i16 RunPendingTalk(void) {
    b16 abort = ExchangeAbortPending(false);
    i16 objects = CountFieldObjects();
    GZ_ENUM_LOCAL(TargetPickResult, i16) picked;
    FieldActor* actor;
    ExchangeAbortPending(abort);
    if (!objects) {
        g_pendingTalk = objects;
        return -1;
    }
    picked = RunPickTargetWindow(0, 3, TARGET_PICK_FIELD_OBJECT, 0);
    if (!picked) {
        return picked;
    }
    if (picked < 0) {
        g_pendingTalk = false;
        return -1;
    }
    actor = GetFieldActor(g_selectedObjectId);
    objects = FindLayerOfKind(actor->core.id);
    CloseMessageWindow();
    StartActorScene(0xe0, 0, objects + 1, actor);
    g_pendingTalk = false;
    AddTrainingPoints(&GetCharacters()->core, BATTLE_GROUP_DEMON_INTERACTION, 3);
    return 1;
}

// Every third step without suppression, the party takes a point of damage;
// nonzero when it did.
// @identity-TODO: the source of the damage is unrecovered.
RVA(0x00012ca0, 0x4f)
b16 TickStepDamage(void) {
    if (IsEventFlagSet(EVENT_FLAG_BANK_SCENARIO_2, SCENARIO_2_STEP_DAMAGE_SUPPRESSED)) {
        s_damageSteps = 0;
        return false;
    }
    if (++s_damageSteps >= 3) {
        s_damageSteps = 0;
        DamageParty(-1, true);
        return true;
    }
    return false;
}

// Every third step runs the party's step effects.
RVA(0x00012cf0, 0x26)
i16 TickFieldSteps(void) {
    if (++s_fieldSteps < 3) {
        return 0;
    }
    s_fieldSteps = 0;
    return TickPartySteps();
}

// Runs one frame of the field (dungeon) exploration, by phase: 0-1 enter the
// area (a cell event may start right away), 2 is the exploration frame
// (battles' rewards, casualties, room rebuilds, pending talks, battles on the
// map, enemy turns, commands), 3 runs a cell event, 4-6 the analyze window, 7
// ends an event, 8 leaves for the world map, 9 returns to the return point, 10
// leaves to the world map, and 11 closes the field.
RVA(0x00012d20, 0x88c)
b16 RunFieldExploration(void) {
    GZ_ENUM_LOCAL(FieldPhase, u16) initialPhase = GetGamePhase();
    if (initialPhase != FIELD_PHASE_LOAD_AREA) {
        if (GetRenderMode() == RENDER_MODE_PANEL
            && g_worldMapRequest == WORLD_MAP_REQUEST_SAVED_SPOT) {
            SetPanelRenderMode();
        } else {
            SetViewRenderMode();
        }
    }
    switch (GetGamePhase()) {
        case FIELD_PHASE_LOAD_AREA:
            s_eventRunning = false;
            SetModeFlags(MODE_FIELD);
            ShowScreenLayer(SCREEN_LAYER_NAVIGATION);
            if (g_worldMapRequest > WORLD_MAP_REQUEST_NONE) {
                if (GetRenderMode() == RENDER_MODE_PANEL) {
                    HideScreenLayer(SCREEN_LAYER_NAVIGATION);
                }
                SetGamePhase(FIELD_PHASE_FADE_TO_WORLD_MAP);
                return false;
            }
            SetViewRenderMode();
            if (g_worldMapRequest < WORLD_MAP_REQUEST_NONE) {
                SetGamePhase(FIELD_PHASE_RETURN_TO_RETURN_POINT);
                g_worldMapRequest = WORLD_MAP_REQUEST_NONE;
                return false;
            }
            NextGamePhase();
            g_worldMapRequest = WORLD_MAP_REQUEST_NONE;
            LoadAreaMap(g_party.field.pos.area, g_party.field.pos.level);
            SaveReturnPoint();
        case FIELD_PHASE_ENTER_CELL:
            s_eventRunning = false;
            g_rewardMagnetite = 0;
            if (s_leaveToReturnPoint) {
                SetGamePhase(FIELD_PHASE_RETURN_TO_RETURN_POINT);
                return FlushFieldScreen();
            }
            NextGamePhase();
            RequestFieldRefresh();
            MarkAutomapCell(
                g_party.field.pos.area,
                g_party.field.pos.level,
                g_party.field.pos.x,
                g_party.field.pos.y
            );
            if (!ModifyEventFlag(0xf, 0xff, BIT_CHANGE_SET)
                && CheckCellEvent(g_party.field.pos.x, g_party.field.pos.y, g_party.field.pos.level)
                       == CELL_EVENT_SCRIPT) {
                CloseMessageWindow();
                SaveFieldPosition();
                SetGamePhase(FIELD_PHASE_ENTER_CELL);
                SetSceneScriptByIndex(7, 8);
                PushGameState(GAME_STATE_CELL_SCENE);
                RevealAutomapRoom(g_party.field.pos.x, g_party.field.pos.y);
                CancelFieldMap();
                ExchangeObjectsHidden(true);
                SetReturnPointAhead();
                return FlushFieldScreen();
            }
            if (s_rebuildRoom) {
                s_rebuildRoom = 0;
                if (BuildRoomMap(1)) {
                    RespawnAreaActors();
                } else {
                    SpawnLevelObjects();
                    UpdateCurrentRoom();
                }
                RequestFieldRefresh();
            }
            if (s_pendingSound) {
                PlaySoundEffect(s_pendingSound);
                s_pendingSound = 0;
            }
            FadeScreenAndWait(SCREEN_FADE_FROM_BLACK, 1);
            if (g_party.field.pos.area == MAP_AREA_CHIYODA_LINE && g_party.field.pos.level == 3
                && g_party.field.pos.x == 5 && g_party.field.pos.y == 0) {
                SaveFieldPosition();
                return FlushFieldScreen();
            }
            break;
        case FIELD_PHASE_EXPLORE:
            if (g_worldMapRequest != WORLD_MAP_REQUEST_NONE) {
                SetGamePhase(FIELD_PHASE_LOAD_AREA);
                s_eventRunning = false;
                return false;
            }
            if (HasTurnElapsed() && TickPartyConditions()) {
                RequestFieldRefresh();
            }
            if (GrantBattleRewards()) {
                SetGamePhase(FIELD_PHASE_ENTER_CELL);
                PushGameState(GAME_STATE_LEVEL_UP);
                PushScreenFade(SCREEN_FADE_TO_BLACK, 1);
                PushWaitState(WAIT_INPUT_OR_FRAMES, WAIT_ON_ANY_INPUT, 0x50, -1);
                ShowPendingLevelUpMessage(g_scratchBuffer);
                s_eventRunning = false;
                return false;
            }
            s_actorVanishing = 0;
            if (ProcessPartyCasualties()) {
                RequestFieldRefresh();
            }
            if (CountFallenHumans()
                && !IsEventFlagSet(EVENT_FLAG_BANK_SCENARIO_2, SCENARIO_2_FALLEN_RESCUE_SUPPRESSED)
                && !g_fieldBattleActive) {
                CloseMessageWindow();
                PushFieldTextScene(0x1a, 6);
                s_eventRunning = false;
                return false;
            }
            if (FindAbleHumanMember() == -1) {
                if (g_party.field.pos.area == MAP_AREA_VIRTUAL_DUNGEON) {
                    CloseMessageWindow();
                    PushFieldTextScene(0x59, 6);
                    ClearRosterConditions();
                    s_eventRunning = false;
                    return false;
                }
                if (g_party.field.pos.area == MAP_AREA_HATSUDAI
                    && (g_party.field.pos.level == 0xc || g_party.field.pos.level == 0xd
                        || g_party.field.pos.level == 0xe)) {
                    CloseMessageWindow();
                    PushFieldTextScene(0x16, 0xd);
                    ClearRosterConditions();
                    s_eventRunning = false;
                    return false;
                }
                CloseMessageWindow();
                PushFieldTextScene(0x2a, 0);
                s_eventRunning = false;
                return false;
            }
            if (s_leaveToReturnPoint) {
                SetGamePhase(FIELD_PHASE_FADE_TO_RETURN_POINT);
                s_eventRunning = false;
                return FlushFieldScreen();
            }
            if (s_rebuildRoom) {
                s_rebuildRoom = 0;
                if (BuildRoomMap(1)) {
                    RespawnAreaActors();
                    RequestFieldRefresh();
                    s_eventRunning = false;
                    return false;
                }
                UpdateCurrentRoom();
                RequestFieldRefresh();
            }
            if (g_pendingTalk) {
                RunPendingTalk();
                s_eventRunning = false;
                return FlushFieldScreen();
            }
            if (g_fieldBattleActive) {
                i16 count = CountFieldObjects();
                if (count <= 0) {
                    if (count < 0) {
                        s_eventRunning = true;
                    }
                    PlaySoundEffect(0x1b);
                    ResetRosterBattleState();
                    CloseFieldWindows();
                    if (g_fieldBattleActive) {
                        MarkRewardsPending();
                        PlayLevelMusic();
                        AccessScriptReg(1, 0, 1 - s_eventRunning);
                        RunMessageScene(0xdd, 0x59, -1);
                        RequestFieldRefresh();
                    }
                    ResetRosterStatModifiers();
                    g_fieldBattleActive = false;
                    s_eventRunning = false;
                    return FlushFieldScreen();
                }
                s_eventRunning = false;
                if (RunPartyTurn(g_tickElapsed) > 0) {
                    break;
                }
            }
            s_eventRunning = false;
            s_actorVanishing = AdvanceObjectAnims();
            if (g_tickElapsed != 0) {
                if ((s_enemyTick = (s_enemyTick + 1) & 3) == 0) {
                    RunFieldIdle();
                }
            }
            if (GetGameState() == GAME_STATE_FIELD_EXPLORATION && s_actorVanishing == 0) {
                PollFieldCommand();
            }
            TickEnemySpawnTimer();
            return FlushFieldScreen();
        case FIELD_PHASE_CELL_EVENT:
            ClearFieldPanelSelection();
            PrevGamePhaseKeepStep();
            if (GetGameStep() == 0) {
                DebugTrace("\n\202\253\202\275\202\327"); // きたべ
                RunCellEvent();
                UpdateCurrentRoom();
            }
            s_eventRunning = true;
            return FlushFieldScreen();
        case FIELD_PHASE_ANALYZE:
            s_eventRunning = false;
            if (!RunAnalyzeWindow()) {
                break;
            }
        case FIELD_PHASE_RESUME:
        case FIELD_PHASE_RESUME_ALIAS:
            SetGamePhase(FIELD_PHASE_EXPLORE);
            return FlushFieldScreen();
        case FIELD_PHASE_END_EVENT:
            s_eventRunning = false;
            NextGamePhase();
            if (s_eventSound) {
                PlaySoundEffect(s_eventSound);
                s_eventSound = 0;
            }
            RequestFieldRefresh();
            return FlushFieldScreen();
        case FIELD_PHASE_FADE_TO_RETURN_POINT:
            s_eventRunning = false;
            NextGamePhase();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            g_party.savedDirection = -1;
            g_worldMapRequest = WORLD_MAP_REQUEST_NONE;
            return FlushFieldScreen();
        case FIELD_PHASE_RETURN_TO_RETURN_POINT:
            s_eventRunning = false;
            s_leaveToReturnPoint = false;
            SetGamePhase(FIELD_PHASE_ENTER_CELL);
            g_party.field.pos.x = s_returnX;
            g_party.field.pos.level = s_returnLevel;
            g_party.field.pos.area = s_returnArea;
            g_party.field.pos.y = s_returnY;
            LoadAreaMap(g_party.field.pos.area, g_party.field.pos.level);
            MarkAutomapCell(
                g_party.field.pos.area,
                g_party.field.pos.level,
                g_party.field.pos.x,
                g_party.field.pos.y
            );
            if (s_returnDirection == VIEW_NONE) {
                s_returnDirection = FindExitDirection(s_returnX, s_returnY);
                if (s_returnDirection >= 0) {
                    g_party.field.pos.direction = (u8)s_returnDirection;
                }
                if (s_stayOnExit == false) {
                    StepMapCoord(
                        &g_party.field.pos.x,
                        &g_party.field.pos.y,
                        g_party.field.pos.direction,
                        MOVE_FORWARD
                    );
                    RebuildViewScene();
                }
                s_stayOnExit = false;
                SaveReturnPoint();
            }
            g_party.field.pos.direction = (u8)s_returnDirection;
            g_party.savedDirection = OppositeDirection(g_party.field.pos.direction);
            RebuildViewScene();
            return FlushFieldScreen();
        case FIELD_PHASE_FADE_TO_WORLD_MAP:
            s_eventRunning = false;
            NextGamePhase();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            return FlushFieldScreen();
        case FIELD_PHASE_CLOSE:
            ClearLayerSurface(SCREEN_LAYER_AUTOMAP);
            s_eventRunning = false;
            CloseMessageWindow();
            ResetFieldObjects();
            ResetFieldScene();
            FreeEffectBackdrop();
            FreeEffectFrames();
            FreeFieldImageCache();
            UnloadAreaMap();
            SetGameState(GAME_STATE_WORLD_MAP);
            ClearModeFlags(MODE_FIELD);
            return false;
    }
    return FlushFieldScreen();
}

// One field command step: the next queued automatic move, else (with
// immediate input allowed outside a pick) the analyze pick, else the
// countdown event while exploring is off.
RVA(0x000135b0, 0x4a)
void PollFieldCommand(void) {
    i16 move = NextAutoMove();
    if (move != -1) {
        RunMoveCommand(move, 1);
        return;
    }
    if (!QueryPickMode()) {
        AllowImmediateInput();
    }
    if (PickAnalyzeTarget() >= 0) {
        SetGamePhase(FIELD_PHASE_ANALYZE);
        return;
    }
    if (!g_fieldBattleActive) {
        FireCountdownEvent();
    }
}

// Leaves the field map (with its sound, closing the windows and restoring
// the level music when it was running) and drops the pending talk.
RVA(0x00013600, 0x31)
void CancelFieldMap(void) {
    if (g_fieldBattleActive) {
        PlaySoundEffect(0x1b);
        CloseFieldWindows();
        PlayLevelMusic();
    }
    g_fieldBattleActive = false;
    g_pendingTalk = false;
}

RVA(0x00013640, 0x440)
void RunCellEvent(void) {
    GZ_ENUM_LOCAL(CellEventKind, i16) kind;
    i16 command;
    i16 map;
    u16 step;

    if (IsOnCellMark(false) == CELL_MARK_MATCH && GetGameStep() == 0) {
        return;
    }
    SaveFieldPosition();
    PayStepUpkeep();
    kind = CheckCellEvent(g_party.field.pos.x, g_party.field.pos.y, g_party.field.pos.level);
    switch (kind) {
        case CELL_EVENT_SCRIPT:
            SetGamePhase(FIELD_PHASE_ENTER_CELL);
            SetSceneScriptByIndex(7, 8);
            PushGameState(GAME_STATE_CELL_SCENE);
            RevealAutomapRoom(g_party.field.pos.x, g_party.field.pos.y);
            CancelFieldMap();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            ExchangeObjectsHidden(true);
            break;
        case CELL_EVENT_WORLD_EXIT:
            SetGamePhase(FIELD_PHASE_FADE_TO_WORLD_MAP);
            CancelFieldMap();
            ExchangeObjectsHidden(true);
            break;
        case CELL_EVENT_STAIRS:
            s_stayOnExit = false;
            if (HasAutoMoves()) {
                SetGamePhase(FIELD_PHASE_END_EVENT);
                CancelFieldMap();
                SetReturnPoint(
                    g_cellDestArea,
                    g_cellDestLevel,
                    g_cellDestX,
                    g_cellDestY,
                    VIEW_NONE
                );
                s_eventSound = 4;
                break;
            }
            SetSceneScript(0x7f04, 7);
        case CELL_EVENT_FROZEN_SCENE:
            PushGameState(GAME_STATE_FROZEN_FIELD_SCENE);
            CancelFieldMap();
            break;
        case CELL_EVENT_MARKED_WARP:
            s_stayOnExit = true;
            SetGamePhase(FIELD_PHASE_END_EVENT);
            CancelFieldMap();
            SetReturnPoint(
                g_cellDestArea,
                g_cellDestLevel,
                g_cellDestX,
                g_cellDestY,
                s_stayOnExit == true ? g_party.field.pos.direction : VIEW_NONE
            );
            SetCellMark(
                g_cellDestArea,
                g_cellDestLevel,
                g_cellDestX,
                g_cellDestY,
                s_stayOnExit == true ? g_party.field.pos.direction : -1
            );
            break;
        case CELL_EVENT_TRAP:
            if (!IsEventFlagSet(EVENT_FLAG_BANK_ITEM_EFFECTS, ITEM_EFFECT_CORE_SHIELD)) {
                RunCellTrap(1, g_party.field.pos.x, g_party.field.pos.y);
            }
            break;
        case CELL_EVENT_CHUTE:
            RunCellTrap(1, g_party.field.pos.x, g_party.field.pos.y);
            s_stayOnExit = true;
        case CELL_EVENT_WARP:
            SetGamePhase(FIELD_PHASE_END_EVENT);
            CancelFieldMap();
            SetReturnPoint(
                g_cellDestArea,
                g_cellDestLevel,
                g_cellDestX,
                g_cellDestY,
                s_stayOnExit == true ? g_party.field.pos.direction : -1
            );
            if (g_cellCode == CELL_WARP_HIDING_OBJECTS) {
                ExchangeObjectsHidden(true);
            }
            break;
        case CELL_EVENT_FORCED_MOVE:
            SetGamePhaseKeepStep(FIELD_PHASE_CELL_EVENT);
            if (g_cellCode >= CELL_FORCED_MOVE_NORTH && g_cellCode <= CELL_FORCED_MOVE_WEST) {
                command = TurnDirection(
                    g_cellCode - g_party.field.pos.direction - CELL_FORCED_MOVE_NORTH,
                    0
                );
                SetGameStep(0);
            } else if (g_cellCode == CELL_FORCED_MOVE_BACK) {
                command = TurnDirection(
                    TurnDirection(g_party.savedDirection, 2) - g_party.field.pos.direction,
                    0
                );
                SetGameStep(0);
            } else if (g_cellCode == CELL_SPIN_LEFT) {
                command = MOVE_TURN_LEFT;
                SetGameStep(0);
            } else {
                if ((step = GetGameStep()) == 0) {
                    SetGameStep(g_cellCode - CELL_SPIN_RIGHT);
                } else {
                    SetGameStep(step - 1);
                }
                command = MOVE_TURN_RIGHT;
            }
            RunMoveCommand(command, 0);
            RequestFieldRefresh();
            break;
        case CELL_EVENT_OBJECT:
            PushGameState(GAME_STATE_FIELD_TEXT_SCENE);
            CancelFieldMap();
            break;
        case CELL_EVENT_BATTLE:
            SetGamePhase(FIELD_PHASE_EXPLORE);
            // The destination y passes through `kind`'s slot before its low byte
            // becomes the map's high byte (retail stores it there).
            kind = g_cellDestY;
            map = (u8)kind << 8 | g_cellDestX;
            MarkFieldRefresh();
            EnterFieldMap(map, -1, 100, -1, 100, FIELD_MAP_CELL_EVENT);
            break;
        case CELL_EVENT_FADE_SCENE:
            SetGamePhase(FIELD_PHASE_ENTER_CELL);
            SetSceneScriptByIndex(5, 6);
            PushGameState(GAME_STATE_CELL_SCENE);
            CancelFieldMap();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            ExchangeObjectsHidden(true);
            MarkSceneDirty();
            break;
    }
}

// Sets the return point to the party's cell and preserves area and level
// event flags when that point is re-entered.
RVA(0x00013a80, 0x3e)
void ReturnToCurrentCell(void) {
    SetReturnPoint(
        g_party.field.pos.area,
        g_party.field.pos.level,
        g_party.field.pos.x,
        g_party.field.pos.y,
        g_party.field.pos.direction
    );
    SetAreaFlagPreservation(1, 1);
}

// Allocates the object event states (one byte per event) once and clears
// them unless a loaded save keeps them.
RVA(0x00013ac0, 0x3c)
void ResetFieldMemory(void) {
    if (!s_eventStates) {
        s_eventStates = AllocHandle(0x100);
    }
    if (!s_keepEvents) {
        u32* states = HandleWritePtr(s_eventStates);
        i32 i;
        for (i = 0; i < 0x40; i++) {
            states[i] = 0;
        }
    }
}

// Clears the level event bits (unless kept) and the event states.
RVA(0x00013b00, 0x1f)
void ResetLevelEvents(void) {
    if (!s_keepEvents) {
        memset(s_levelEvents, 0, sizeof(s_levelEvents));
    }
    ResetFieldMemory();
}

RVA(0x00013b20, 0x13)
void MarkLevelEvent(i16 level) {
    SetBit(s_levelEvents, level);
}

// Whether level `level`'s event bit is set (the loaded states stop being
// kept).
RVA(0x00013b40, 0x1c)
b32 TestLevelEvent(i16 level) {
    s_keepEvents = false;
    return TestBit(s_levelEvents, level);
}

// Raises object event `event` (out of range: 0) to state 1, or 3 when
// `queued`; 0 when it was already done (state 2).
RVA(0x00013b60, 0x55)
b16 RaiseObjectEvent(i16 event, i16 queued) {
    u8* state;
    if (!s_eventStates) {
        ResetFieldMemory();
    }
    if (event < 0 || event >= 0x100) {
        event = 0;
    }
    state = GetObjectEventState(event);
    if (*state == OBJECT_EVENT_DONE) {
        return false;
    }
    if (queued) {
        *state = OBJECT_EVENT_QUEUED;
    } else {
        *state = OBJECT_EVENT_RAISED;
    }
    return true;
}

// Marks queued object event `event` done (state 3 to 2).
RVA(0x00013bc0, 0x3e)
void QueueObjectEvent(i16 event) {
    u8* state;
    if (!s_eventStates) {
        ResetFieldMemory();
    }
    if (event < 0 || event >= 0x100) {
        event = 0;
    }
    state = GetObjectEventState(event);
    if (*state == OBJECT_EVENT_QUEUED) {
        *state = OBJECT_EVENT_DONE;
    }
}

// The number of object events still queued (state 3).
RVA(0x00013c00, 0x33)
i16 HasQueuedObjectEvents(void) {
    i16 count;
    u8* state;
    i32 i;
    if (!s_eventStates) {
        ResetFieldMemory();
    }
    count = 0;
    state = HandleWritePtr(s_eventStates);
    for (i = 0x100; i != 0; i--) {
        if (*state == OBJECT_EVENT_QUEUED) {
            count++;
        }
        state++;
    }
    return count;
}

RVA(0x00013c40, 0x59)
i16 SaveFieldMemory(FILE* fp) {
    u8* states;
    i32 stateErrors;
    i32 bitErrors;
    if (!s_eventStates) {
        ResetFieldMemory();
    }
    states = HandleWritePtr(s_eventStates);
    stateErrors = 0x100 - fwrite(states, 1, 0x100, fp);
    bitErrors = 0x20 - fwrite(s_levelEvents, 1, 0x20, fp);
    return bitErrors + stateErrors;
}

// Reads the event states and level bits; a complete read keeps them through
// the next level switch.
RVA(0x00013ca0, 0x6c)
i16 LoadFieldMemory(FILE* fp) {
    u8* states;
    i16 stateErrors;
    i16 bitErrors;
    if (!s_eventStates) {
        ResetFieldMemory();
    }
    states = HandleWritePtr(s_eventStates);
    stateErrors = 0x100 - fread(states, 1, 0x100, fp);
    bitErrors = 0x20 - fread(s_levelEvents, 1, 0x20, fp);
    s_keepEvents = stateErrors + bitErrors == 0;
    return stateErrors + bitErrors;
}

// Loads the field event table (data file 10).
RVA(0x00013d10, 0x2a)
void LoadFieldEventTable(void) {
    FILE* fp = OpenDataFile(DATA_TABLE_FIELD_EVENTS, DATA_FILE_TABLE, 0);
    s_eventTable = ReadRawHandle(fp);
    CloseDataFile(fp);
}

// The executable dereferences the raw buffer as a pointer and reads a DWORD
// offset at a WORD stride. ET000A.BIN instead contains WORD file offsets;
// preserve this legacy reader's behavior rather than repairing the format.
RVA(0x00013d40, 0x30)
void MergeViewOcclusionMask(u8** table, i16 index, void* destination) {
    i32 count;
    i32 offset;
    void* sourceBytes;
    u16* source;
    u16* output;
    count = 8;
    memcpy(&offset, *table + index * sizeof(u16), sizeof(offset));
    sourceBytes = *table + offset - 2;
    source = sourceBytes;
    output = destination;
    do {
        u16 bits = *source++;
        *output++ |= bits;
    } while (--count);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00013d70, 0x20)
void MarkFieldViewCells(i16 unused, i16 x, i16 y, i16 direction) {
    MarkVisibleFieldCells(0, x, y, direction);
}

RVA(0x00013d90, 0x180)
void MarkVisibleFieldCells(i16 unused, i16 x, i16 y, i16 direction) {
    i16 cellY;
    i16 width;
    i16 height;
    i16 along;
    i16 across;
    i16 opposite;
    i16 index;
    i16 cellX;
    GetMapSize(&width, &height);
    BuildViewOcclusion(x, y, direction, 0);
    for (along = -3; along <= 0; along++) {
        for (across = -3; across <= 0; across++) {
            opposite = -across;
            index = opposite - along * 4;
            if (across != 0 && g_leftViewOcclusion[index] != 0xff) {
                cellX = x;
                cellY = y;
                OffsetMapCoord(&cellX, &cellY, direction, across, along);
                cellY = cellY * width + cellX;
                MarkDrawCell(cellY);
            }
            if (across != 0 && g_rightViewOcclusion[index] != 0xff) {
                cellX = x;
                cellY = y;
                OffsetMapCoord(&cellX, &cellY, direction, -across, along);
                cellY = cellY * width + cellX;
                MarkDrawCell(cellY);
            }
            if (across == 0
                && (g_leftViewOcclusion[index] != 0xff || g_rightViewOcclusion[index] != 0xff)) {
                cellX = x;
                cellY = y;
                OffsetMapCoord(&cellX, &cellY, direction, 0, along);
                cellY = cellY * width + cellX;
                MarkDrawCell(cellY);
            }
        }
    }
}

#define MergeViewOcclusionEntry(masks, index)                                                      \
    MergeViewOcclusionMask(HandleReadPtr(s_eventTable), (index), (masks) + (index))

// @early-stop register allocation: retail holds direction in esi and index in
// edi; here those registers are exchanged and direction loads after the clears.
// Calls, branch destinations and ordered referents agree.
RVA(0x00013f10, 0x183)
void BuildViewOcclusion(
    i16 x,
    i16 y,
    const GZ_ENUM_PARAM(ViewDirection, i16) direction,
    i16 unused
) {
    i16 along;
    i16 across;
    i16 opposite;
    memset(g_leftViewOcclusion, 0, sizeof(g_leftViewOcclusion));
    memset(g_rightViewOcclusion, 0, sizeof(g_rightViewOcclusion));
    for (along = 0; along >= -3; along--) {
        opposite = 0;
        for (across = 0; across <= 3; opposite--, across++) {
            i16 index;
            index = (-along) * 4 + across;
            {
                const u16 rightWall = GetWallAtOffset(x, y, direction, across, along);
                if (GetCellWallStop(direction, 0, rightWall)) {
                    MergeViewOcclusionEntry(g_rightViewOcclusion, index + 16);
                }
                if (GetCellWallStop(direction, 1, rightWall)) {
                    MergeViewOcclusionEntry(g_rightViewOcclusion, index);
                }
            }
            {
                const u16 leftWall = GetWallAtOffset(x, y, direction, opposite, along);
                if (GetCellWallStop(direction, 0, leftWall)) {
                    MergeViewOcclusionEntry(g_leftViewOcclusion, index + 16);
                }
                if (GetCellWallStop(direction, 3, leftWall)) {
                    MergeViewOcclusionEntry(g_leftViewOcclusion, index);
                }
            }
        }
    }
}

RVA(0x000140a0, 0x26)
void RedrawFieldAt(i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction) {
    SampleViewWalls(x, y, direction);
    UpdateViewCells(x, y);
}

RVA(0x000140d0, 0x269)
void SampleViewWalls(i16 x, i16 y, i16 direction) {
    i16 along;
    i16 across;
    u16 wall;
    memset(g_centerFrontWalls, 0, sizeof(g_centerFrontWalls));
    memset(g_leftSideWalls, 0, sizeof(g_leftSideWalls));
    memset(g_rightSideWalls, 0, sizeof(g_rightSideWalls));
    memset(g_leftFrontWalls, 0, 4 * sizeof(g_leftFrontWalls[0]));
    memset(g_rightFrontWalls, 0, 4 * sizeof(g_rightFrontWalls[0]));
    for (along = 0; along >= -4; along--) {
        wall = GetWallAtOffsetClamped(x, y, direction, 0, along);
        wall = RotateByDirection(wall, direction);
        g_centerFrontWalls[-along] = WallStops(wall, WALL_STOP_GEOMETRY);
    }
    for (along = 0; along >= -4; along--) {
        for (across = 0; across > -3; across--) {
            wall = GetWallAtOffsetClamped(x, y, direction, across, along);
            wall = RotateByDirection(wall, direction + 3);
            g_leftSideWalls[-along][-across] = WallStops(wall, WALL_STOP_GEOMETRY);
        }
    }
    for (along = 0; along >= -3; along--) {
        for (across = -1; across >= -3; across--) {
            wall = GetWallAtOffsetClamped(x, y, direction, across, along);
            wall = RotateByDirection(wall, direction);
            g_leftFrontWalls[-along][-across - 1] = WallStops(wall, WALL_STOP_GEOMETRY);
        }
    }
    for (along = 0; along >= -4; along--) {
        for (across = 0; across < 3; across++) {
            wall = GetWallAtOffsetClamped(x, y, direction, across, along);
            wall = RotateByDirection(wall, direction + 1);
            g_rightSideWalls[-along][across] = WallStops(wall, WALL_STOP_GEOMETRY);
        }
    }
    for (along = 0; along >= -3; along--) {
        for (across = 1; across <= 3; across++) {
            wall = GetWallAtOffsetClamped(x, y, direction, across, along);
            wall = RotateByDirection(wall, direction);
            g_rightFrontWalls[-along][across - 1] = WallStops(wall, WALL_STOP_GEOMETRY);
        }
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00014340, 0xc6)
i16 GetViewVisibility(i16 across, i16 along, i16 side) {
    i16 index;
    if (along > 0) {
        return 0;
    }
    if (along < 0) {
        along = -along;
    }
    index = along * 4;
    if (across < 0) {
        index -= across;
    } else {
        index += across;
    }
    if (across < 0) {
        if (side == 0) {
            return (g_leftViewOcclusion[index] & 0xf0) - 0xf0;
        }
        return (g_leftViewOcclusion[index] & 0x0f) - 0x0f;
    }
    if (across > 0) {
        if (side == 0) {
            return (g_rightViewOcclusion[index] & 0xf0) - 0xf0;
        }
        return (g_rightViewOcclusion[index] & 0x0f) - 0x0f;
    }
    if (side < 0) {
        return (g_leftViewOcclusion[index] & 0x0f) - 0x0f;
    }
    if (side > 0) {
        return (g_rightViewOcclusion[index] & 0x0f) - 0x0f;
    }
    return ((g_leftViewOcclusion[index] & 0xf0) - 0xf0)
           | ((g_rightViewOcclusion[index] & 0xf0) - 0xf0);
}

RVA(0x00014410, 0x4b)
void FreeFieldImageCache(void) {
    s_fieldImageCacheKey = -1;
    s_fieldImageCacheVariant = -1;
    s_fieldImageCacheA = FreeImageHandle(s_fieldImageCacheA);
    s_fieldImageCacheB = FreeImageHandle(s_fieldImageCacheB);
    s_fieldImageCacheC = FreeImageHandle(s_fieldImageCacheC);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00014460, 0x3c)
void SelectFieldImageCache(i16 id, i16 variant) {
    id = (id | 0x100) << 4;
    if (s_fieldImageCacheKey != id || s_fieldImageCacheVariant != variant) {
        FreeFieldImageCache();
        s_fieldImageCacheKey = id;
        s_fieldImageCacheVariant = variant;
    }
}

RVA(0x000144a0, 0x14)
void ReleaseFieldImage(void) {
    s_fieldImage = FreeImageHandle(s_fieldImage);
}

RVA(0x000144c0, 0x70)
void LoadFieldImage(i16 image, i16 variant, i16 mode) {
    ImageRequest request;
    if (image == 0x5000) {
        image = 0x5049;
        variant = 0;
        mode = 0;
    }
    ReleaseFieldImage();
    s_fieldImageMode = mode;
    request.file = image;
    request.variant = variant;
    request.flags = 1;
    LoadRequestedScenePicture(request, variant);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00014530, 0x1fb)
void VisitVisibleCellWalls(i16 view, i16 across, i16 along, i16 direction, u16 cell) {
    i16 width;
    i16 depth;
    i16 index;
    i16 stop;
    if (cell == 0xffff) {
        return;
    }
    width = across;
    if (width < 0) {
        width = -width;
    }
    depth = along;
    if (depth < 0) {
        depth = -depth;
    }
    index = width + depth * 4;
    if (across <= 0 && (g_leftViewOcclusion[index] & 0x0f) != 0x0f) {
        stop = GetCellWallStop(direction, 3, cell);
        stop = WallStops(stop, WALL_STOP_GEOMETRY);
        if (stop == true) {
            AreObjectsHidden(view, across, along, -1);
        } else {
            GetCellWallStop(direction, 3, cell);
        }
    }
    if (across >= 0 && (g_rightViewOcclusion[index] & 0x0f) != 0x0f) {
        stop = GetCellWallStop(direction, 1, cell);
        stop = WallStops(stop, WALL_STOP_GEOMETRY);
        if (stop == true) {
            AreObjectsHidden(view, across, along, 1);
        } else {
            GetCellWallStop(direction, 1, cell);
        }
    }
    if (across < 0 && (g_leftViewOcclusion[index] & 0xf0) != 0xf0) {
        stop = GetCellWallStop(direction, 0, cell);
        stop = WallStops(stop, WALL_STOP_GEOMETRY);
        if (stop == true) {
            AreObjectsHidden(view, across, along, 0);
        } else {
            GetCellWallStop(direction, 0, cell);
        }
    }
    if (across > 0 && (g_rightViewOcclusion[index] & 0xf0) != 0xf0) {
        stop = GetCellWallStop(direction, 0, cell);
        stop = WallStops(stop, WALL_STOP_GEOMETRY);
        if (stop == true) {
            AreObjectsHidden(view, across, along, 0);
        } else {
            GetCellWallStop(direction, 0, cell);
        }
    }
    if (across == 0) {
        stop = GetCellWallStop(direction, 0, cell);
        stop = WallStops(stop, WALL_STOP_GEOMETRY);
        if (stop == true) {
            AreObjectsHidden(view, 0, along, 0);
        } else {
            GetCellWallStop(direction, 0, cell);
        }
        if (along == 0) {
            GetWarpCodeAtOffset(0, 0);
        }
    }
}
