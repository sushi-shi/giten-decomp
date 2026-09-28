// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. The field-map routine at
// 0x407390 (reserved for lane 4) and the field state handler at 0x407aa0
// (about forty unclaimed callees) use these statics and belong to it.

#include <rva.h>

#include <Game/CombatantId.h>

#include <Game/Actor.h>
#include <Game/Analyze.h>
#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/EquipEffect.h>
#include <Game/Field.h>
#include <Game/FieldActor.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/LevelUp.h>
#include <Game/PartyAction.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/StateStack.h>
#include <Game/StatusDraw.h>
#include <Game/WaitState.h>
#include <Game/WorldMap.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/Vram.h>
#include <Gfx/VramAccess.h>
#include <Script/EventFlags.h>
#include <Script/ScriptVars.h>
#include <Sound/Sound.h>
#include <Ui/Hotspot.h>
#include <Ui/Message.h>
#include <Util/BitSet.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <stddef.h>

DATA(0x0006840c)
static i16 s_fieldCountA = -1;

DATA(0x00068410)
static i16 s_fieldRateA = 100;

DATA(0x00068414)
static i16 s_fieldCountB = -1;

DATA(0x00068418)
static i16 s_fieldRateB = 100;

// -1 while no field map is active.
DATA(0x0006841c)
static i16 s_fieldMode = -1;

// The music track that was playing when the encounter started (or the field
// map was entered).
DATA(0x00068420)
static i16 s_fieldMusic = -1;

DATA(0x00068424)
static i16 s_fieldOption = 20;

DATA(0x00068428)
static i16 s_fieldParamFirst = 1;

DATA(0x0006842c)
static i16 s_fieldParamSecond = -1;

DATA(0x000784f4)
static i16 s_fieldMap;

DATA(0x000784f8)
static i16 s_fieldEntryState;

DATA(0x000784fc)
static i16 s_fieldMarker;

DATA(0x00078500)
static i16 s_fieldPairFirst;

DATA(0x00078504)
static i16 s_fieldPairSecond;

DATA(0x00078508)
static i16 s_fieldParamThird;

// Set when the map was left by abort or a proximity event; feeds script register 0.
DATA(0x0007850c)
static i16 s_fieldLeftEarly;

DATA(0x00078510)
static i16 s_fieldRefresh;

// The palette snapshot held while an encounter runs.
DATA(0x00078518)
static PaletteState* s_fieldPaletteState;

RVA(0x000070d0, 0xa)
void MarkFieldRefresh(void) {
    s_fieldRefresh = 1;
}

RVA(0x000070e0, 0x12)
i16 ExchangeFieldOption(i16 option) {
    i16 old = s_fieldOption;
    s_fieldOption = option;
    return old;
}

RVA(0x00007100, 0x2a)
i16 SetFieldParams(i16 first, i16 second, i16 third) {
    i16 old = s_fieldParamFirst;
    s_fieldParamFirst = first;
    s_fieldParamSecond = second;
    s_fieldParamThird = third;
    return old;
}

RVA(0x00007130, 0x23)
b16 IsFieldModeAtLeast(i16 anyMode) {
    if (anyMode == 0) {
        return s_fieldMode >= 1;
    }
    return s_fieldMode >= 0;
}

RVA(0x00007160, 0x7)
i16 GetFieldMarker(void) {
    return s_fieldMarker;
}

RVA(0x00007170, 0x18)
void SetFieldPair(i16 first, i16 second) {
    s_fieldPairFirst = first;
    s_fieldPairSecond = second;
}

RVA(0x00007190, 0x59)
void EnterFieldMap(i16 map, i16 countA, i16 rateA, i16 countB, i16 rateB, i16 mode) {
    s_fieldMap = map;
    s_fieldCountA = countA;
    s_fieldRateA = rateA;
    s_fieldCountB = countB;
    s_fieldRateB = rateB;
    s_fieldMode = mode;
    s_fieldEntryState = 0;
    PushGameState(0xb);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000071f0, 0x7)
i16 GetFieldMap(void) {
    return s_fieldMap;
}

RVA(0x00007200, 0x7)
i16 GetFieldEntryState(void) {
    return s_fieldEntryState;
}

RVA(0x00007210, 0x18)
void SetFieldCounts(i16 countA, i16 countB) {
    s_fieldCountA = countA;
    s_fieldCountB = countB;
}

// Counts the side's countdown down (unless held); returns 1 when inactive,
// -1 when the count is 0, else the count, with the exhausted marker -2 read
// back as 0.
RVA(0x00007230, 0x66)
i16 TickFieldCount(i16 side, i16 hold) {
    i16* count;
    if (s_fieldMode == -1) {
        return 1;
    }
    count = side < 0 ? &s_fieldCountA : &s_fieldCountB;
    if (*count == -1) {
        return 1;
    }
    if (*count == 0) {
        return -1;
    }
    if (hold == 0 && *count > 0) {
        if (--*count == 0) {
            *count = -2;
        }
    }
    if (*count == -2) {
        return 0;
    }
    return *count;
}

// Scales value by the rate of the side the operand signs select (A when the
// first is negative and the second not, B for the reverse).
RVA(0x000072a0, 0x60)
i32 ScaleByFieldRate(i16 first, i16 second, i32 value) {
    double scaled;
    if (s_fieldMode == -1) {
        return value;
    }
    scaled = value;
    if (first < 0 && second >= 0) {
        scaled *= s_fieldRateA;
    } else if (first >= 0 && second < 0) {
        scaled *= s_fieldRateB;
    } else {
        return value;
    }
    return scaled * 0.01;
}

RVA(0x00007300, 0x33)
void SpawnSecondGroupActor(i16 x, i16 y, i16 battle) {
    SpawnFieldObject(
        1,
        x,
        y,
        OppositeDirection(g_field.pos.direction),
        s_fieldParamSecond,
        battle,
        -1,
        0
    );
}

RVA(0x00007340, 0x42)
i16 GetFacingWall(i16 map) {
    i16 width;
    i16 height;
    GetMapSize(&width, &height);
    return GetWallAt(g_field.pos.x, g_field.pos.y, g_field.pos.direction, width, height);
}

// Runs one frame of a field encounter, by phase: 0 enters it (step 0 spawns the
// enemy groups at the party's position and starts the music), 1 runs the turns
// until the party or the enemies are beaten, 2 backs out, 3 grants the
// rewards, 4 shows the level-ups, 5 the analyze window and 6 tears it down.
// @identity-TODO: named from its phases (enemy spawns, turns, rewards,
// level-ups); the caller 0x417160 dispatches it as a game state.
RVA(0x00007390, 0x6b0)
b16 RunFieldEncounter(void) {
    i16 x;
    i16 y;
    i16 i;
    i16 allFallen;
    Character* actor;

    if (!(GetGameStep() | GetGamePhase())) {
        SaveScreenMode();
    }
    RefreshScreenMode();
    switch (GetGamePhase()) {
        case 0:
            switch (GetGameStep()) {
                case 0:
                    NextGameStep();
                    LockStatusRedraw(0);
                    SetFieldMenuMode(1);
                    s_fieldMarker = 1;
                    g_fieldBattleActive = 1;
                    ResetFieldObjects();
                    s_fieldPaletteState = SavePaletteState(s_fieldPaletteState, 3);
                    SaveFieldLayer(0);
                    SaveFieldLayer(1);
                    NotifyEncounterStart();
                    if (s_fieldMode == 0) {
                        for (i = 0; i < s_fieldParamFirst; i++) {
                            SpawnFieldObject(
                                0,
                                g_field.pos.x,
                                g_field.pos.y,
                                (g_field.pos.direction - 2) & 3,
                                s_fieldMap,
                                1,
                                -1,
                                0
                            );
                        }
                        if (s_fieldParamSecond >= 0) {
                            for (i = 0; i < s_fieldParamThird; i++) {
                                SpawnSecondGroupActor(g_field.pos.x, g_field.pos.y, 1);
                            }
                        }
                        if (s_fieldRefresh) {
                            s_fieldMusic = PlayMusic(0x15, 1);
                        } else {
                            s_fieldMusic = PlayMusic(0xd, 1);
                        }
                    } else {
                        x = g_field.pos.x;
                        y = g_field.pos.y;
                        if (!GetFacingWall(s_fieldMap)) {
                            OffsetMapCoord(&x, &y, g_field.pos.direction, 0, -1);
                        }
                        for (i = 0; i < s_fieldParamFirst; i++) {
                            SpawnFieldObject(
                                0,
                                x,
                                y,
                                (g_field.pos.direction - 2) & 3,
                                s_fieldMap,
                                1,
                                -1,
                                0
                            );
                        }
                        if (s_fieldParamSecond >= 0) {
                            for (i = 0; i < s_fieldParamThird; i++) {
                                SpawnSecondGroupActor(x, y, 1);
                            }
                        }
                        if (s_fieldRefresh) {
                            s_fieldMusic = PlayMusic(0xd, 1);
                        } else {
                            s_fieldMusic = PlayMusic(s_fieldOption, 1);
                        }
                    }
                    LoadEnemyGroupSlot(0, s_fieldMap);
                    if (s_fieldParamSecond >= 0) {
                        LoadEnemyGroupSlot(1, s_fieldParamSecond);
                    }
                    RequestFieldRefresh();
                    for (i = 0; i < s_fieldParamThird + s_fieldParamFirst; i++) {
                        actor = GetFieldActor(i);
                        AlertActor(actor, 2);
                    }
                    break;
                case 1:
                    NextGamePhase();
                    ResetPartyTurnState();
                    RedrawFieldView();
                    break;
            }
            break;
        case 1:
            if (HasTurnElapsed() && TickPartyConditions()) {
                RequestFieldRefresh();
            }
            if (!AdvanceObjectAnims()) {
                AllowImmediateInput();
            }
            allFallen = 1;
            for (i = 0; i < s_fieldParamThird + s_fieldParamFirst; i++) {
                allFallen &= GetFatalCondition(GetCharacterConditions(GetFieldActor(i)));
            }
            if (s_fieldMode >= 0 && CountFieldObjects() <= 0) {
                LeaveFieldMap(1);
                break;
            }
            if (s_fieldMode == 1 && allFallen) {
                LeaveFieldMap(1);
                break;
            }
            if (FindFirstAblePartyMember() == -1) {
                LeaveFieldMap(-1);
                break;
            }
            if (!TickFieldCount(-1, 1)) {
                LeaveFieldMap(0);
                break;
            }
            if (!TickFieldCount(1, 1)) {
                LeaveFieldMap(0);
                break;
            }
            if (!GetPickMode() && PickAnalyzeTarget() >= 0) {
                SetGamePhase(5);
                break;
            }
            SetFieldBusy(0);
            if (RunPartyTurn(g_tickElapsed)) {
                break;
            }
            if (TickFieldCount(0, 1) <= 0) {
                break;
            }
            for (i = 0; i < s_fieldParamThird + s_fieldParamFirst; i++) {
                actor = GetFieldActor(i);
                if (actor != NULL) {
                    actor->fieldState = 6;
                }
            }
            RunFieldIdle();
            break;
        case 2:
            PrevGamePhase();
            break;
        case 3:
            PlaySoundEffect(0x1b);
            NextGamePhase();
            ResetRosterStatModifiers();
            if (s_fieldEntryState <= 0) {
                break;
            }
            MarkRewardsPending();
            PlayMusic(s_fieldMusic, 1);
            RunMessageScene(0xdd, 0x59, -1);
            if (s_fieldPairFirst != 0 || s_fieldPairSecond != 0) {
                ModifyEventFlag(s_fieldPairFirst, s_fieldPairSecond, 1);
            }
            break;
        case 4:
            if (GrantBattleRewards()) {
                CloseMessageWindow();
                PushScreenFade(SCREEN_FADE_FROM_BLACK, 1);
                PushGameState(0x1b);
                PushScreenFade(SCREEN_FADE_TO_BLACK, 1);
                PushWaitState(2, 0xffff, 0x50, -1);
                MarkRewardsPending();
                FormatLevelUpMessage(g_scratchBuffer, FindLevelUpSlot());
                ShowMessage(g_scratchBuffer, 0x3c);
                return false;
            }
            s_fieldPairFirst = 0;
            s_fieldPairSecond = 0;
            SetGamePhase(6);
            break;
        case 5:
            if (RunAnalyzeWindow()) {
                SetGamePhase(1);
            }
            break;
        case 6:
            PlaySoundEffect(0x1b);
            RestoreScreenMode();
            ClearSelectedHotspot();
            ResetFieldObjects();
            ResetFieldLayer(1);
            ResetFieldLayer(0);
            CloseMessageWindow();
            NotifyEncounterEnd();
            RestoreFieldLayer(1);
            RestoreFieldLayer(0);
            s_fieldPaletteState = RestorePaletteState(s_fieldPaletteState, 1);
            if (s_fieldMode == 0) {
                RespawnAreaActors();
            }
            RequestFieldRefresh();
            s_fieldRefresh = 0;
            ReturnFromGameState();
            s_fieldCountA = -1;
            s_fieldRateA = 100;
            s_fieldCountB = -1;
            s_fieldRateB = 100;
            s_fieldMode = -1;
            ResetRosterBattleState();
            SetFieldMenuMode(0);
            s_fieldMarker = 0;
            s_fieldOption = 20;
            s_fieldParamFirst = 1;
            s_fieldParamSecond = -1;
            s_fieldParamThird = 0;
            break;
    }
    return FlushFieldScreen();
}

// Records how the field map ended and advances the owning state two phases.
RVA(0x00007a40, 0x23)
void LeaveFieldMap(i16 result) {
    g_fieldBattleActive = 0;
    s_fieldEntryState = result;
    CloseFieldWindows();
    NextGamePhase();
    NextGamePhase();
}

// Clears every roster member's field marks.
RVA(0x00007a70, 0x2a)
void ResetRosterFieldMarks(void) {
    Character* character;
    i16 slot;
    for (slot = 0; slot < 32; slot++) {
        character = GetRosterCharacter(slot);
        if (character != NULL) {
            ClearActionWait(GetCharacterActionWait(character));
        }
    }
}

// The field state's per-frame handler, one case per phase.
RVA(0x00007aa0, 0x4d4)
b16 RunFieldState(void) {
    i16 key;
    SetFieldRenderMode();
    SetInfoBarLayout(0);
    switch ((u16)GetGamePhase()) {
        case 0:
            switch ((u16)GetGameStep()) {
                case 0:
                    ClearSceneSurfaces();
                    NextGameStep();
                    s_fieldLeftEarly = 0;
                    SetFieldStatusBit0(0);
                    SetFieldStatusBit11(0);
                    SetFieldMenuMode(3);
                    g_fieldBattleActive = 1;
                    ResetFieldScene();
                    s_fieldPaletteState = SavePaletteState(s_fieldPaletteState, 3);
                    ResetFieldObjects();
                    LoadFieldTable();
                    PrepareFieldRandom();
                    s_fieldMusic = PlayMusic(13, 1);
                    ResetRosterFieldMarks();
                    RequestFieldRefresh();
                    return FlushFieldScreen();
                case 1:
                    NextGamePhase();
                    StartScreenFadeAndWait(SCREEN_FADE_FROM_BLACK, 1);
                    break;
            }
            break;
        case 1:
            if (HasTurnElapsed() && TickPartyConditions()) {
                RequestFieldRefresh();
            }
            if (!AdvanceObjectAnims() && !GetPickMode()) {
                AllowImmediateInput();
            }
            key = CountFieldObjects();
            if (key <= 0) {
                LeaveFieldMap(0);
                if (key >= 0) {
                    break;
                }
                s_fieldLeftEarly = 1;
                return FlushFieldScreen();
            }
            if (FindFirstAblePartyMember() == -1) {
                LeaveFieldMap(-1);
                return FlushFieldScreen();
            }
            if (!GetPickMode()) {
                if (PickAnalyzeTarget() >= 0) {
                    SetGamePhase(5);
                    return FlushFieldScreen();
                }
                if (g_pendingTalk) {
                    RunPendingTalk();
                    return FlushFieldScreen();
                }
            }
            SetFieldBusy(0);
            if (!RunPartyTurn(g_tickElapsed)) {
                RunFieldIdle();
            }
            UpdateFieldObjects();
            if (GetFieldBusy()) {
                break;
            }
            if (!GetEncounterPending()) {
                break;
            }
            HideScreenLayer(1);
            if (RollProximityEvent() > 0) {
                LeaveFieldMap(0);
                s_fieldLeftEarly = 1;
                RunMessageScene(0x7f04, 0x10, -1);
                PlaySoundEffect(4);
                ClearEncounterPending();
                return UpdateFieldScreen(0);
            }
            NextGamePhase();
            RunMessageScene(0x7f04, 0x11, -1);
            PlaySoundEffect(3);
            PushWaitState(WAIT_FRAMES, 0x3c, 0x3c, 0);
            ClearEncounterPending();
            return UpdateFieldScreen(0);
        case 2:
            SetFieldStatusBit0(0);
            CloseMessageWindow();
            RestoreDrawState(SaveDrawState());
            PrevGamePhase();
            return UpdateFieldScreen(0);
        case 3:
            PlaySoundEffect(0x1b);
            NextGamePhase();
            ResetRosterStatModifiers();
            if (s_fieldEntryState < 0) {
                break;
            }
            MarkRewardsPending();
            PlayMusic(s_fieldMusic, 1);
            AccessScriptReg(1, 0, 1 - s_fieldLeftEarly);
            RunMessageScene(0xdd, 0x59, -1);
            if (s_fieldPairFirst == 0 && s_fieldPairSecond == 0) {
                break;
            }
            ModifyEventFlag(s_fieldPairFirst, s_fieldPairSecond, 1);
            return FlushFieldScreen();
        case 4:
            if (GrantBattleRewards()) {
                CloseMessageWindow();
                PushScreenFade(SCREEN_FADE_FROM_BLACK, 1);
                PushGameState(0x1b);
                PushScreenFade(SCREEN_FADE_TO_BLACK, 1);
                PushWaitState(WAIT_INPUT_OR_FRAMES, -1, 0x50, -1);
                MarkRewardsPending();
                FormatLevelUpMessage(g_scratchBuffer, FindLevelUpSlot());
                ShowMessage(g_scratchBuffer, 0x3c);
                return false;
            }
            s_fieldPairFirst = 0;
            s_fieldPairSecond = 0;
            SetGamePhase(6);
            return FlushFieldScreen();
        case 5:
            if (RunAnalyzeWindow()) {
                SetGamePhase(1);
                return FlushFieldScreen();
            }
            break;
        case 6:
            PlaySoundEffect(0x1b);
            CloseMessageWindow();
            ResetFieldObjects();
            ResetFieldLayer(1);
            ResetFieldLayer(0);
            RequestFieldRefresh();
            ReturnFromGameState();
            SetFieldStatusBit11(1);
            ResetRosterBattleState();
            SetFieldMenuMode(2);
            ReleaseFieldImage();
            s_fieldPaletteState = RestorePaletteState(s_fieldPaletteState, 1);
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            break;
    }
    return FlushFieldScreen();
}

RVA(0x00007f80, 0xcf)
b16 RollProximityEvent(void) {
    i16 nearest = 0x7fff;
    i16 object = -1;
    i16 index;
    i16 distance;
    MapCoord pos;
    for (index = 15; index >= 0; index--) {
        if (GetLiveObject(index) >= 0) {
            pos = GetObjectCoord(index);
            distance = GridDistance(g_field.pos.x, g_field.pos.y, pos.x, pos.y);
            if (distance <= nearest) {
                nearest = distance;
                object = index;
            }
        }
    }
    if (object < 0) {
        return true;
    }
    switch (nearest) {
        case 0:
            index = 0x7fff;
            break;
        case 1:
            index = RandomAverage(20, 40, 0);
            break;
        case 2:
            index = RandomAverage(12, 22, 0);
            break;
        default:
            index = RandomAverage(5, 15, 0);
            break;
    }
    distance = GetStatTotal(GetRosterLeader(), STAT_FORTUNE);
    return distance >= index;
}

RVA(0x00008050, 0x10)
void UpdatePartyActionWaits(void) {
    if (g_tickElapsed) {
        TickPartyActionWaits();
    }
}

RVA(0x00008060, 0xd5)
b16 HasObjectInReach(i16 mode, i16 first, i16 second) {
    MapCoord pos = GetMapCoord();
    FieldObject* object;
    switch (mode) {
        case 0:
            if (first >= 0) {
                object = GetFieldObject(first);
                if (pos.x != object->pos.x || pos.y == object->pos.y) {
                    return false;
                }
            } else if (!CountObjectsAt(pos.x, pos.y, 0, 0)) {
                return false;
            }
            break;
        case 1:
            if (first >= 0 && second >= 0) {
                return false;
            }
            if (first < 0 && second < 0) {
                break;
            }
            if (first >= 0) {
                object = GetFieldObject(first);
            } else {
                object = GetFieldObject(second);
            }
            if (pos.x != object->pos.x || pos.y != object->pos.y) {
                return false;
            }
            break;
        default:
            return false;
    }
    return true;
}

RVA(0x00008140, 0x2b)
b32 IsPartyAt(i32 x, i32 y) {
    MapCoord pos = GetMapCoord();
    return pos.x == x && pos.y == y;
}

RVA(0x00008170, 0x35)
i16 FindFirstAblePartyMember(void) {
    i16 index;
    Character* member;
    for (index = 0; index < 6; index++) {
        member = GetPartyEntry(index);
        if (member && !GetDisablingCondition(GetCharacterConditions(member))) {
            return index;
        }
    }
    return -1;
}

RVA(0x000081b0, 0x4b)
i16 FindAbleHumanMember(void) {
    i16 index;
    Character* member;
    for (index = 0; index < 6; index++) {
        member = GetPartyCharacter(index);
        if (member && (member->id == 38 || member->id == 399 || IsHumanCharacter(member))
            && !GetDisablingCondition(GetCharacterConditions(member))) {
            return index;
        }
    }
    return -1;
}

RVA(0x00008200, 0xbc)
void TickPartyConditionActions(void) {
    i16 index;
    i16 action;
    Character* actor;
    for (index = 0; index < 6; index++) {
        actor = GetPartyCharacter(index);
        if (actor && !GetPickState(actor) && !IsActionWaitMarked(GetCharacterActionWait(actor))) {
            if (!GetActionCondition(actor)) {
                actor->conditionActionTicks = 0;
            } else {
                actor->conditionActionTicks++;
                if (actor->conditionActionTicks >= 36) {
                    actor->conditionActionTicks = 0;
                    g_actorId = PartyCombatantId(index);
                    action = PickActorAction(actor);
                    if (action >= 1) {
                        if ((action & 15) == 4) {
                            action = (action & 0xf0) | 1;
                        }
                        action = AdjustActorAction(PartyCombatantId(index), action);
                        if (action != 0) {
                            ChangeCharacterFlag(actor, 32, 1);
                            MarkActorActionReady(actor);
                        }
                        return;
                    }
                }
            }
        }
    }
}

RVA(0x000082c0, 0x67)
void MarkActorActionReady(Character* actor) {
    GetCharacterActionWait(actor)->ready = 1;
    switch (actor->mode) {
        case 1:
            MarkPickDone();
            break;
        case 2:
            if (!IsHumanCharacter(actor)) {
                actor->pickRole = 7;
                MarkPickDone();
                break;
            }
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            actor->pickRole = 8;
            MarkPickDone();
            break;
    }
}

RVA(0x00008330, 0x11e)
i16 RunPartyTurn(i16 ticks) {
    i16 index;
    i16 id;
    i16 action;
    Character* actor;
    if (!RunPartyCommandInput()) {
        return -1;
    }
    TickPartyConditionActions();
    UpdatePartyActionWaits();
    index = FindReadyMember(1);
    if (index < 0) {
        return 0;
    }
    actor = GetPartyCharacter(index);
    id = PartyCombatantId(index);
    actor->conditionActionTicks = 0;
    g_actorId = id;
    ApplyEquipmentEffects(actor, EQUIP_EFFECT_ACTION);
    if (!TestCharacterFlag(actor, 32)) {
        action = PickActorAction(actor);
        if (action > 0 && AdjustActorAction(id, action) > 0) {
            MarkActorActionReady(actor);
        }
    }
    ChangeCharacterFlag(actor, 32, 0);
    if (PushPromptState(0, 0, 200, 450, 0)) {
        return 0;
    }
    g_actionId = 1;
    g_actorId = id;
    g_targetId = actor->pickObject;
    ResetActionWait(GetCharacterActionWait(actor));
    CheckPickTarget(index);
    g_tickElapsed = 0;
    return index + 1;
}
