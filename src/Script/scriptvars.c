// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/InfoBar.h>
#include <Game/LevelUp.h>
#include <Game/Scene.h>
#include <Game/ScreenEffect.h>
#include <Game/StateStack.h>
#include <Game/WaitLoop.h>
#include <Game/WaitState.h>
#include <Gfx/Background.h>
#include <Gfx/Blit.h>
#include <Gfx/Motion.h>
#include <Gfx/Render.h>
#include <Gfx/Scene.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/Sprite.h>
#include <Gfx/VideoState.h>
#include <Gfx/Vram.h>
#include <Gfx/VramAccess.h>
#include <Gfx/VramCell.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptBlock.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptOperand.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptPanel.h>
#include <Script/ScriptState.h>
#include <Script/ScriptText.h>
#include <Script/ScriptVars.h>
#include <Script/TextState.h>
#include <Script/TextToken.h>
#include <Text/TextPlane.h>
#include <Text/TextPlaneAttr.h>
#include <Text/TextWindow.h>
#include <Ui/Message.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <mbstring.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x00069828)
static i16 s_messageHookFile = -1;

DATA(0x0006982c)
static i16 s_messageHookEntry = -1;

DATA(0x000815a0)
char g_formattedNumber[64] = {0};

// The script's 32-bit variables (script operands address them 0..25).
DATA(0x000815e0)
u32 g_scriptLongVars[26] = {0};

DATA(0x00081648)
static ScriptScratchValue s_scratchValue = {0};

// @identity-TODO: a counter that advances once per call of its tick while
// counting is on; a script opcode reads it into a variable.
DATA(0x0008164c)
u32 g_tickCounter = 0;

DATA(0x00081650)
static i16 s_tickCountOn = 0;

// @identity-TODO: the option word the script's window-opening opcode passes on.
// Set while a script builds a choice list; the text writer takes it with
// every character.
DATA(0x00081654)
b16 g_inChoices = 0;

DATA(0x00081658)
static i16 s_pendingScene = 0;

DATA(0x0008165c)
static i16 s_pendingSceneEntry = 0;

DATA(0x00081660)
static ScriptChoice* s_choices = NULL;

DATA(0x00081664)
static i16 s_choiceIndex = 0;

DATA(0x00081668)
static i16 s_choiceColumns = 0;

DATA(0x0008166c)
static i16 s_choiceColumnWidth = 0;

DATA(0x00081670)
static i16 s_choiceTop = 0;

DATA(0x00081674)
static i16 s_choiceX = 0;

DATA(0x00081678)
static i16 s_choiceY = 0;

DATA(0x0008167c)
static i16 s_choiceDisabled = 0;

DATA(0x00081680)
i16 g_windowOption = 0;

// Whether the field objects were frozen when a script thawed them.
DATA(0x00081684)
static i16 s_objectsWereFrozen = 0;

// The loaded script files, oldest first.
DATA(0x00081688)
ScriptFileEntry* g_scriptFiles = NULL;

// @identity-TODO: a script-set countdown that other code draws down; the
// script fires its pending event once it reaches zero.
DATA(0x0008168c)
u32 g_countdown = 0;

// @identity-TODO: when set, the script's message opcode waits in a loop until
// the wait is released.
// The script file and entry the countdown fires.
DATA(0x00081690)
static i16 s_countdownFile = 0;

DATA(0x00081694)
static i16 s_countdownEntry = 0;

DATA(0x00081698)
static i16 s_holdOn = 0;

// The handle of the block that holds generated script text.
DATA(0x0008169c)
static i32 s_textScript = 0;

RVA(0x0003a350, 0x12)
b16 OpHideSprite(void) {
    UnplaceSprite(ReadScriptValue());
    return false;
}

RVA(0x0003a370, 0x36)
void OpFadeIn(void) {
    i16 variant = ReadScriptValue();
    i16 steps = ReadScriptValue();
    steps = max(1, steps);
    if (!variant) {
        StartScreenFade(SCREEN_FADE_FROM_BLACK, steps);
    } else {
        StartScreenFade(SCREEN_FADE_FROM_WHITE, steps);
    }
}

RVA(0x0003a3b0, 0x36)
void OpFadeOut(void) {
    i16 variant = ReadScriptValue();
    i16 steps = ReadScriptValue();
    steps = max(1, steps);
    if (!variant) {
        StartScreenFade(SCREEN_FADE_TO_BLACK, steps);
    } else {
        StartScreenFade(SCREEN_FADE_TO_WHITE, steps);
    }
}

RVA(0x0003a3f0, 0x4e)
void OpRefreshFieldScreen(void) {
    i16 state = SaveDrawState();
    i16 option = ReadScriptValue();
    FieldScreenNop(option & 7);
    if (!(option & 0x10)) {
        DrawInfoBar(1, 0);
    }
    RestoreDrawState(state);
    if (GetSpriteMode() == SPRITE_LAYERS_PARTY_AND_TEXT) {
        ResetSprites(SPRITE_LAYERS_ALL);
    }
}

RVA(0x0003a440, 0x57)
void OpFadeOutAndClear(void) {
    ReadScriptValue();
    ReadScriptValue();
    StartScreenFade(SCREEN_FADE_TO_BLACK, 1);
    FinishScreenFade();
    if (g_party.field.pos.area == 0x82 && g_party.field.pos.level == 8 && g_party.field.pos.x == 2
        && g_party.field.pos.y == 1 && GetRenderMode() == 2) {
        ResetSprites(SPRITE_LAYERS_PARTY_AND_TEXT);
    }
}

RVA(0x0003a4a0, 0x10)
void OpResetMask(void) {
    ResetMask(ReadScriptValue());
}

// @identity-TODO: the frame, mode and final option are passed to an empty
// Windows blitter; their original drawing roles remain unrecovered.
RVA(0x0003a4b0, 0x73)
void OpDrawImage(void) {
    i16 index = ReadScriptValue();
    i16 frame = ReadScriptValue();
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    i16 mode = ReadScriptValue();
    i16 option = ReadScriptValue();
    u32 image = GetSceneEntry(index);
    if (image) {
        BlitScriptImage(GetPlaneData(0), image, frame, x - 40, y - 112, mode, 1, option);
    }
}

RVA(0x0003a530, 0x80)
void OpFillScreenCells(void) {
    CellRow cell[8];
    u8 mask[8];
    i16 masked = ReadScriptValue();
    i16 color;
    i16 state;
    i16 x, y;
    ReadScriptValue();
    color = ReadScriptValue() & 15;
    state = SaveCellState();
    for (y = 0; y < 400; y += 8) {
        for (x = 0; x < 80; x++) {
            FillCell(cell, color);
            if (masked) {
                ReadMaskColumn(mask, x, y);
                AndCellMask(cell, mask);
            }
        }
    }
    RestoreScreenState(state);
}

RVA(0x0003a5b0, 0x8d)
void OpMaskScreenCells(void) {
    CellRow cell[8];
    u8 mask[8];
    i16 state = SaveCellState();
    i16 masked = ReadScriptValue();
    i16 x, y;
    ReadScriptValue();
    for (y = 0; y < 400; y += 8) {
        for (x = 0; x < 80; x++) {
            if (masked) {
                ReadMaskColumn(mask, x, y);
                AndCellMask(cell, mask);
            } else {
                i16 row;
                memset(mask, 0xff, sizeof(mask));
                for (row = 0; row < 8; row++) {
                    cell[row].mask = mask[row];
                }
            }
        }
    }
    RestoreScreenState(state);
}

RVA(0x0003a640, 0x10)
void OpEnableBackground(void) {
    ShowScenePicture();
    SetSceneFlags(7);
}

// @identity-TODO: Reads a value operand and a variable index and does nothing else (a Windows
// stub of a PC-98 opcode); the PC-98 overlay would name it.
RVA(0x0003a650, 0xa)
void OpSkipValueAndVar(void) {
    ReadScriptValue();
    ReadLongVarIndex();
}

RVA(0x0003a660, 0x7d)
void OpPollMouseClick(void) {
    i16 resultIndex = ReadLongVarIndex();
    i16 xIndex = ReadLongVarIndex();
    i16 yIndex = ReadLongVarIndex();
    i16 inputB = ReadScriptValue();
    i16 inputC = ReadScriptValue();
    i16 inputA = ReadScriptValue();
    i16 x, y;
    SetScriptLongVar(resultIndex, PollRewardClick(inputA, inputB, inputC, &x, &y));
    SetScriptLongVar(xIndex, x);
    SetScriptLongVar(yIndex, y);
}

RVA(0x0003a6e0, 0x23)
void OpPlayAnimation(void) {
    i16 animation = ReadScriptValue();
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    PushScriptAnimation(animation, x, y);
}

RVA(0x0003a710, 0x30)
void OpSwapScreenState(void) {
    i16 token = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    i16 previous = SaveScreenState();
    RestoreScreenState(token);
    SetScriptLongVar(index, previous);
}

RVA(0x0003a740, 0xc)
void SetWindowOption(i16 option) {
    g_windowOption = option;
}

RVA(0x0003a750, 0xa1)
b16 OpBeginChoices(i16 window) {
    i16 width;
    TextPoint size;
    PushTextDelay(0);
    s_choiceIndex = -1;
    width = ReadScriptValue();
    g_inChoices = true;
    size = GetTextPlaneSize(window);
    s_choiceTop = GetTextPlaneCursorY(window);
    size.x -= GetActiveTextPlaneIndent(window);
    width += 2;
    s_choiceColumns = size.x / width;
    if (s_choiceColumns < 1) {
        s_choiceColumns = 1;
    }
    s_choiceColumnWidth = size.x / s_choiceColumns;
    s_choices = FreeScriptChoices(s_choices);
    return false;
}

RVA(0x0003a800, 0x6f)
b16 FinishScriptChoice(i16 window) {
    if (s_choiceIndex >= 0) {
        ScriptChoice* choice = AppendScriptChoice(&s_choices);
        choice->x = s_choiceX;
        choice->y = s_choiceY;
        choice->width = GetTextPlaneCursorX(window) - choice->x;
        choice->value = s_choiceIndex;
        choice->disabled = s_choiceDisabled;
    }
    s_choiceIndex++;
    return false;
}

RVA(0x0003a870, 0x12e)
b16 OpNextChoice(i16 window) {
    i16 x, y;
    ScriptChoice* choice = s_choices;
    TextPoint size = GetTextPlaneSize(window);
    i16 lineStep = GetTextPlaneLineStep(window);
    FinishScriptChoice(window);
    for (;;) {
        x = (s_choiceIndex % s_choiceColumns) * s_choiceColumnWidth;
        y = s_choiceIndex / s_choiceColumns;
        x += GetActiveTextPlaneIndent(window);
        y = lineStep * y + s_choiceTop;
        if (y < size.y) {
            break;
        }
        while (choice) {
            choice->y -= lineStep;
            choice = choice->next;
        }
        s_choiceTop -= lineStep;
        ScrollTextWindowLine(window);
    }
    SetTextPlaneCursor(window, x, y);
    s_choiceX = GetTextPlaneCursorX(window);
    s_choiceY = GetTextPlaneCursorY(window);
    s_choiceDisabled = 0;
    return false;
}

RVA(0x0003a9a0, 0x1f)
b16 OpEndChoices(i16 window) {
    FinishScriptChoice(window);
    PopTextDelay();
    g_inChoices = false;
    return false;
}

RVA(0x0003a9c0, 0x26)
b16 OpRunChoiceMenu(i16 window) {
    s_choices = PushScriptChoiceMenu(s_choices, window, 0, g_windowOption);
    return false;
}

// @identity-TODO: 0x919ee is copied from 0x919e0 by 0xa050 when that id's record level lies in
// a range; what the id selects is unproven.
RVA(0x0003a9f0, 0x17)
void OpGetSelectedObjectId(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, g_selectedObjectId);
}

// @identity-TODO: 0x919e0 is set by 0xa050 from 0x585a0 (id of the entry indexed by 0x6b4e0 in
// 0x8802c); that it is the object under the cursor is inferred.
RVA(0x0003aa10, 0x17)
void OpGetHoveredObjectId(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, g_hoveredObjectId);
}

// A fresh block: its code handle holds the 256 ranges, each one zero byte.
RVA(0x0003aa30, 0x59)
ScriptBlock* NewScriptBlock(void) {
    ScriptBlock* block = AllocCleared(1, sizeof(ScriptBlock));
    ScriptCode* code;
    u16 i;
    block->code = AllocArrayHandle(1, 0x500);
    code = HandleWritePtr(block->code);
    for (i = 0; i < 256; i++) {
        GetScriptRange(code, i)->offset = i + 0x400;
        GetScriptRange(code, i)->length = 1;
        code->bytes[i] = 0;
    }
    return block;
}

// Reads a script block from `file` (allocating the block when NULL): its
// byte size (the decryption seed), an entry count and the entries.
RVA(0x0003aa90, 0x7b)
ScriptBlock* ReadScriptBlock(ScriptBlock* block, void* file) {
    u16 size;
    u16 count;
    if (block == NULL) {
        block = NewScriptBlock();
    }
    if ((u16)fread(&size, 2, 1, file) >= 1) {
        SetCryptKey(size);
        count = ReadCryptWord(file);
        size -= 2;
        for (; count > 0; count--) {
            size -= ReadScriptEntry(block, file);
        }
    }
    return block;
}

// Reads one entry: its number and length, then its code into the block. A
// length of 0xffff adds an event-flag condition and the real length; an entry
// whose flag does not match is skipped. Returns the bytes consumed.
RVA(0x0003ab10, 0xab)
u16 ReadScriptEntry(ScriptBlock* block, FILE* fp) {
    u8 entry = ReadCryptByte(fp);
    u16 length = ReadCryptWord(fp);
    u16 header = 3;
    u8 bank;
    u8 index;
    if (length == 0xffff) {
        bank = ReadCryptByte(fp);
        index = ReadCryptByte(fp);
        length = ReadCryptWord(fp);
        header = 7;
        if (!MatchEventFlag(bank, index)) {
            SkipBytes(fp, length);
            return length + 7;
        }
    }
    ReadCryptBytes(fp, length, ResizeScriptEntry(block, entry, length));
    return length + header;
}

// Resizes entry `entry`'s code to `length` bytes (at least 1), moving the
// code after it; returns the entry's code.
RVA(0x0003abc0, 0xd0)
u8* ResizeScriptEntry(ScriptBlock* block, i16 entry, i16 length) {
    ScriptCode* code;
    i32 size;
    i16 i;
    i16 delta;
    if (length == 0) {
        length = 1;
    }
    code = HandleWritePtr(block->code);
    size = 0x400;
    for (i = 0; i < 256; i++) {
        size += GetScriptRange(code, i)->length;
    }
    delta = length - GetScriptRange(code, entry)->length;
    if (delta < 0) {
        code = HandleWritePtr(block->code);
        ShiftScriptEntries(code, entry, delta, size);
        block->code = ResizeHandle(block->code, size + delta);
    } else if (delta > 0) {
        block->code = ResizeHandle(block->code, size + delta);
        code = HandleWritePtr(block->code);
        ShiftScriptEntries(code, entry, delta, size);
    }
    code = HandleWritePtr(block->code);
    return OffsetBy(code, GetScriptRange(code, entry)->offset);
}

// Moves the code after entry `entry` by `delta` bytes (of `size` in use) and
// updates its length and the later offsets.
// @early-stop referent only: the bytes are identical; the move's call resolves
// to _memmove here and to the unnamed FUN_0045ad50 in retail (memcpy and
// memmove share that address in the static-library map).
RVA(0x0003ac90, 0x83)
void ShiftScriptEntries(ScriptCode* code, i16 entry, i16 delta, u16 size) {
    u16 end = GetScriptRange(code, entry)->length + GetScriptRange(code, entry)->offset;
    u8* src = OffsetBy(code, end);
    u8* dst = OffsetBy(code, end + delta);
    u16 count = size - end;
    i16 i;
    if (count != 0) {
        memmove(dst, src, count);
        GetScriptRange(code, entry)->length += delta;
        for (i = entry + 1; i < 256; i++) {
            GetScriptRange(code, i)->offset += delta;
        }
    }
}

// Loads script data file `file` into `block` (a new block when NULL).
RVA(0x0003ad20, 0x38)
ScriptBlock* LoadScriptBlock(ScriptBlock* block, i16 file) {
    FILE* fp = OpenDataFile(file, 9, 0);
    block = ReadScriptBlock(block, fp);
    block->id = file;
    CloseDataFile(fp);
    return block;
}

RVA(0x0003ad60, 0x24)
ScriptBlock* FreeScriptBlock(ScriptBlock* block) {
    if (!block) {
        return NULL;
    }
    FreeHandle(block->code);
    return FreeBlock(block);
}

// Scrolls the window by the operand's number of lines.
RVA(0x0003ad90, 0x22)
void OpScrollWindow(i16 window) {
    i16 lines = ReadScriptValue();
    while (lines) {
        ScrollTextWindowLine(window);
        lines--;
    }
}

RVA(0x0003adc0, 0x24)
void OpGetWindowScrollTop(i16 window) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, GetTextPlaneHeaderRows(window));
}

RVA(0x0003adf0, 0x1f)
void OpSetWindowScrollTop(i16 window) {
    SetTextWindowScrollTop(window, ReadScriptValue());
    SetTextPeriod(window);
}

RVA(0x0003ae10, 0x24)
void OpGetWindowIndent(i16 window) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, GetTextPlaneIndent(window));
}

RVA(0x0003ae40, 0x14)
void OpSetWindowIndent(i16 window) {
    SetTextPlaneIndent(window, ReadScriptValue());
}

RVA(0x0003ae60, 0x24)
void OpGetWindowScrollStep(i16 window) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, GetTextPlaneLineStep(window));
}

// @identity-TODO: 0x52b80 ignores the value operand and always stores 1; whether the opcode was
// meant to set the step is unproven.
RVA(0x0003ae90, 0x1f)
void OpSetWindowScrollStep(i16 window) {
    ResetTextPlaneLineStep(window, ReadScriptValue());
    SetTextPeriod(window);
}

// Stores the window's cursor column and row into two variables.
RVA(0x0003aeb0, 0x43)
void OpGetWindowCursor(i16 window) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, GetTextPlaneCursorX(window));
    index = ReadLongVarIndex();
    SetScriptLongVar(index, GetTextPlaneCursorY(window));
}

RVA(0x0003af00, 0x1e)
void OpSetWindowCursor(i16 window) {
    i16 x = ReadScriptValue();
    SetTextPlaneCursor(window, x, ReadScriptValue());
}

// Lets the field objects move again, remembering whether they were frozen.
RVA(0x0003af20, 0x11)
void ThawObjectsForScript(void) {
    s_objectsWereFrozen = ExchangeObjectsFrozen(0);
}

RVA(0x0003af40, 0x5a)
void StartDebugScene(i16 scene, i16 arg, i16 phase) {
    if (scene == 0xe0) {
        StartActorScene(0xe0, arg, 1, GetFieldActor(0));
        return;
    }
    PushGameState(GAME_STATE_SCRIPT_SCENE);
    SetGamePhase(phase);
    s_pendingScene = scene;
    s_pendingSceneEntry = arg;
}

// Clears a message window and repaints it (mode 4 with a message hook,
// else 3), then clears the text period.
RVA(0x0003afa0, 0x3c)
void ClearMessageWindow(i16 window) {
    ClearTextPlane(window);
    if (IsTextMessageHookEnabled()) {
        RepaintTextPlane(window, 4);
        ClearTextPeriod();
    } else {
        RepaintTextPlane(window, 3);
        ClearTextPeriod();
    }
}

RVA(0x0003afe0, 0x258)
b16 RunScriptScene(void) {
    i16 window = GetGamePhase();
    switch (GetGameStep()) {
        case 1:
            NextGameStep();
            AdvanceScriptTextWindow(window);
            break;
        case 0:
            ClearFlagBank(12);
            UnplaceAllSprites();
            NextGameStep();
            NextGameStep();
            ResetTextStateDelayed();
            ResetTextPlaneLineStep(window, 2);
            SetTextPlaneIndent(window, 0);
            SetTextPlaneIndentEnabled(window, 1);
            SetTextWindowScrollTop(window, 2);
            HomeTextPlaneCursor(window);
            ResetWindowAltColor(window);
            ResetWindowInstantColor(window);
            SaveAndResetTextPlaneAttrs(window);
            SetTextPeriod(window);
            ClearTextPeriod();
            ForgetTextPlaneAttr(window);
            ResetScriptPanels();
            StartScript(s_pendingScene, s_pendingSceneEntry, NewScriptContext(0, NULL));
            SetTextScrollMode(1);
            SetTextTimedWait(0);
            ClearScriptLongVars();
        case 2: {
            i16 result;
            PollScriptPanels();
            result = TickScript(window);
            if (result == -1) {
                NextGameStep();
            } else if (result < 0 && result != -3) {
                if (!StepOnTextPeriod(window)) {
                    PrevGameStep();
                    WaitForScriptText(window);
                }
            }
            break;
        }
        case 3:
            s_messageHookFile = s_messageHookEntry = -1;
            SetCurrentScript(FreeScriptContext(GetCurrentScript()));
            PurgeScriptFiles();
            SaveAndResetTextPlaneAttrs(window);
            SetTextWindowScrollTop(window, 0);
            SetTextPlaneIndent(window, 0);
            SetTextPlaneIndentEnabled(window, 0);
            ResetTextPlaneLineStep(window, 3);
            CloseScriptInterface();
            ReturnFromGameState();
            break;
        case 4:
            ExchangeObjectsFrozen(s_objectsWereFrozen);
            ExchangeViewHold(GetGameSub());
            SetGameStep(2);
            ReleaseScriptFiles();
            RestoreScriptState();
            ResetTextPlaneLineStep(window, 2);
            ClearTextPlane(window);
            RepaintTextPlane(window, 3);
            ClearTextPeriod();
            break;
        case 5: {
            i16 top = GetScriptWindowOrDefault(window);
            ScrollTextWindowLine(top);
            SetGameStep(2);
            break;
        }
    }
    return false;
}

// In scrolling mode, schedules a line scroll while the text period has
// not expired. Returns 1 when scheduled.
RVA(0x0003b240, 0x26)
b16 StepOnTextPeriod(i16 window) {
    if (g_textState.scrollEnabled && !TickTextPeriod()) {
        SetGameStep(5);
        return true;
    }
    return false;
}

RVA(0x0003b270, 0x9f)
void WaitForScriptText(i16 window) {
    i16 top = GetScriptWindowOrDefault(window);
    if (g_textState.timedWait && g_textState.inputWait) {
        if (g_textState.waitFrames > 1) {
            PushWaitState(WAIT_INPUT_OR_FRAMES, 0xffff, g_textState.waitFrames - 1, top);
        } else {
            PushWaitState(WAIT_INPUT_OR_FRAMES, 0xffff, 1, top);
        }
    } else if (!g_textState.timedWait && g_textState.inputWait) {
        PushWaitState(WAIT_INPUT, 0xffff, g_textState.waitFrames, top);
    } else if (g_textState.timedWait && !g_textState.inputWait) {
        if (g_textState.waitFrames > 1) {
            PushWaitState(WAIT_FRAMES, 0xffff, g_textState.waitFrames - 1, top);
        }
    }
}

RVA(0x0003b310, 0x2c)
void AdvanceScriptTextWindow(i16 window) {
    i16 top = GetScriptWindowOrDefault(window);
    if (!g_textState.scrollEnabled) {
        ClearMessageWindow(top);
    } else {
        ScrollTextWindowLine(top);
    }
}

RVA(0x0003b340, 0x10f)
void StartActorScene(i16 scene, i16 entry, i16 index, Character* actor) {
    i16 window;
    CloseMessageWindow();
    actor->facing = OppositeDirection(g_party.field.pos.direction);
    RequestFieldRefresh();
    RedrawFieldView();
    PushGameState(GAME_STATE_ACTOR_SCENE);
    window = CreateTextPlane(2, 0);
    SetGamePhase(window);
    RepaintTextPlane(window, 1);
    s_pendingScene = scene;
    s_pendingSceneEntry = entry;
    ResetTextStateDelayed();
    ResetTextPlaneLineStep(window, 2);
    SetTextPlaneIndent(window, 0);
    SetTextPlaneIndentEnabled(window, 0);
    SetTextWindowScrollTop(window, 0);
    ClearTextPlane(window);
    ResetWindowAltColor(window);
    ResetWindowInstantColor(window);
    SaveAndResetTextPlaneAttrs(window);
    SetTextPeriod(window);
    ClearTextPeriod();
    SetTextScrollMode(0);
    ForgetTextPlaneAttr(window);
    ResetScriptPanels();
    StartScript(s_pendingScene, s_pendingSceneEntry, NewScriptContext(index, actor));
}

RVA(0x0003b450, 0x134)
b16 RunActorScene(void) {
    i16 window = GetGamePhase();
    for (;;) {
        switch (GetGameStep()) {
            case 0:
                NextGameStep();
                NextGameStep();
                ClearFlagBank(12);
            case 2: {
                i16 result;
                PollScriptPanels();
                result = TickScript(window);
                if (result == -1) {
                    NextGameStep();
                } else if (result < 0 && result != -3) {
                    if (!StepOnTextPeriod(window)) {
                        PrevGameStep();
                        WaitForScriptText(window);
                    }
                } else {
                    return UpdateFieldScreen(0);
                }
                break;
            }
            case 1:
                NextGameStep();
                AdvanceScriptTextWindow(window);
                continue;
            case 5: {
                i16 top = GetScriptWindowOrDefault(window);
                ScrollTextWindowLine(top);
                SetGameStep(2);
                break;
            }
            case 3:
                s_messageHookFile = s_messageHookEntry = -1;
                SetCurrentScript(FreeScriptContext(GetCurrentScript()));
                PurgeScriptFiles();
                CloseTextWindow(window);
                CloseScriptInterface();
                ReturnFromGameState();
                break;
        }
        return false;
    }
}

RVA(0x0003b590, 0x48)
void OpSetMessageHook(void) {
    ReadScriptBytePair(&s_messageHookFile, &s_messageHookEntry);
    if (s_messageHookFile == 0xff && s_messageHookEntry == 0xff) {
        s_messageHookFile = -1;
        s_messageHookEntry = -1;
        g_textState.messageHookEnabled = 0;
    } else {
        g_textState.messageHookEnabled = 1;
    }
}

RVA(0x0003b5e0, 0x5f)
void RunMessageHook(void) {
    if (s_messageHookFile != -1 && s_messageHookEntry != -1) {
        ScriptContext* saved = GetCurrentScript();
        StartScript(s_messageHookFile, s_messageHookEntry, NewScriptContext(0, NULL));
        RunCurrentScript();
        FreeScriptContext(GetCurrentScript());
        SetCurrentScript(saved);
    }
}

RVA(0x0003b640, 0xf)
ScriptFileEntry* ScriptFileListTail(ScriptFileEntry* entry) {
    while (1) {
        if (entry->next == NULL) {
            break;
        }
        entry = entry->next;
    }
    return entry;
}

// Loads script file `file` into a new entry at the end of the cache.
RVA(0x0003b650, 0x61)
ScriptBlock* CacheScriptFile(i16 file) {
    ScriptFileEntry* entry;
    if (!g_scriptFiles) {
        entry = AllocCleared(1, sizeof(ScriptFileEntry));
        g_scriptFiles = entry;
    } else {
        entry = ScriptFileListTail(g_scriptFiles);
        entry->next = AllocCleared(1, sizeof(ScriptFileEntry));
        entry = entry->next;
    }
    return entry->block = LoadScriptBlock(NULL, file);
}

// Preloads the resident script files.
RVA(0x0003b6c0, 0x5a)
void PreloadScriptFiles(void) {
    u16 file;
    for (file = 0x7f00; file <= 0x7f07; file++) {
        CacheScriptFile(file);
    }
    CacheScriptFile(0xde);
    CacheScriptFile(0xdd);
    CacheScriptFile(0xdc);
    CacheScriptFile(0xdb);
    CacheScriptFile(0xdf);
}

// The position of `entry` in the cached script file `file`; a zero entry when
// the file is not cached.
RVA(0x0003b720, 0x7a)
ScriptEntry FindCachedScriptEntry(i16 file, i16 entry) {
    ScriptEntry result;
    ScriptFileEntry* cached;
    ClearScriptEntry(&result);
    for (cached = g_scriptFiles; cached != NULL; cached = cached->next) {
        if (GetScriptBlockId(cached->block) == file) {
            return MakeScriptEntry(cached->block, entry);
        }
    }
    return result;
}

// Loads script data file `file` as cached file `id` (reusing its entry).
RVA(0x0003b7a0, 0x50)
void LoadCachedScriptFile(i16 id, i16 file) {
    ScriptFileEntry* cached;
    for (cached = g_scriptFiles; cached != NULL; cached = cached->next) {
        if (GetScriptBlockId(cached->block) == id) {
            break;
        }
    }
    if (!cached) {
        CacheScriptFile(file)->id = id;
        return;
    }
    cached->block = LoadScriptBlock(cached->block, file);
    cached->block->id = id;
}

RVA(0x0003b7f0, 0x14)
void HoldScriptFiles(void) {
    ScriptFileEntry* entry;
    for (entry = g_scriptFiles; entry != NULL; entry = entry->next) {
        entry->holds++;
    }
}

RVA(0x0003b810, 0x26)
void ReleaseScriptFiles(void) {
    ScriptFileEntry* entry;
    for (entry = g_scriptFiles; entry != NULL; entry = entry->next) {
        if (entry->holds > 0) {
            entry->holds--;
        } else {
            entry->holds = 0;
        }
    }
}

// Frees the cached files after file 0xdf's entry and the held run after it.
RVA(0x0003b840, 0x71)
void PurgeScriptFiles(void) {
    ScriptFileEntry* entry;
    ScriptFileEntry* next;
    ScriptFileEntry* dead;
    entry = g_scriptFiles;
    if (entry == NULL || entry->block == NULL) {
        return;
    }
    while (GetScriptBlockId(entry->block) != 0xdf) {
        entry = entry->next;
        if (entry == NULL || entry->block == NULL) {
            return;
        }
    }
    while (entry->holds != 0) {
        next = entry->next;
        if (next == NULL) {
            return;
        }
        if (next->holds == 0) {
            break;
        }
        entry = next;
    }
    next = entry->next;
    entry->next = NULL;
    while (next != NULL) {
        dead = next;
        next = next->next;
        FreeScriptBlock(dead->block);
        FreeBlock(dead);
    }
}

// Runs `text` as script code from a scratch block.
RVA(0x0003b8c0, 0x5d)
void RunTextScript(const char* text) {
    char* code;
    if (s_textScript == 0) {
        s_textScript = NewBlockHandle(0x400);
    }
    code = HandleWritePtr(s_textScript);
    strcpy(code, text);
    ScriptJumpTo(s_textScript, 0);
}

RVA(0x0003b920, 0x1e)
void CallTextScript(const char* text) {
    PushCallFrame(g_curScript, 0);
    RunTextScript(text);
}

// Call the full name of a script object as script text: party slot 0, slot 1,
// the party member 0x43bc70 selects, and the script actor.
// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x0003b940, 0x18)
void CallFirstMemberName(void) {
    CallTextScript(GetTextToken(0, 0, -1));
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x0003b960, 0x18)
void CallSecondMemberName(void) {
    CallTextScript(GetTextToken(0, 0, -2));
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x0003b980, 0x18)
void CallSelectedMemberName(void) {
    CallTextScript(GetTextToken(0, 0, -16));
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x0003b9a0, 0x18)
void CallActorName(void) {
    CallTextScript(GetTextToken(0, 0, -17));
}

RVA(0x0003b9c0, 0x5e)
void OpPrintRosterName(void) {
    Character* character = GetRosterCharacter(ReadScriptValue());
    if (character != NULL) {
        FormatFullName(g_scratchBuffer, character);
    } else {
        strcpy(g_scratchBuffer, "NULL PTR");
    }
    CallTextScript(g_scratchBuffer);
}

RVA(0x0003ba20, 0xf)
void OpPrintOperandText(void) {
    CallTextScript(ReadTextToken());
}

RVA(0x0003ba30, 0xaa)
void OpPrintNumber(void) {
    char number[32];
    char* output;
    i16 index;
    g_numberUnit[0] = 0;
    _snprintf(number, sizeof(number), "%ld", ReadScriptValue());
    index = 0;
    output = g_formattedNumber;
    while (number[index]) {
        u16 ch = _mbbtombc((u8)number[index]);
        if (ch > 0xff) {
            *output++ = ch >> 8;
        }
        *output++ = ch;
        index++;
    }
    *output = 0;
    strcat(g_formattedNumber, g_numberUnit);
    CallTextScript(g_formattedNumber);
}

RVA(0x0003bae0, 0x10)
u32 SetTickCounter(u32 value) {
    u32 prev = g_tickCounter;
    g_tickCounter = value;
    return prev;
}

RVA(0x0003baf0, 0x34)
// Turning counting on from off restarts the counter at 0.
i16 SetTickCountOn(i16 on) {
    i16 prev = s_tickCountOn;
    if (on == 1 && s_tickCountOn == 0) {
        SetTickCounter(0);
    }
    s_tickCountOn = on;
    return prev;
}

RVA(0x0003bb30, 0xb)
void OpStartTickCounter(void) {
    SetTickCountOn(1);
}

RVA(0x0003bb40, 0xb)
void OpPauseTickCounter(void) {
    SetTickCountOn(-1);
}

RVA(0x0003bb50, 0xb)
void OpStopTickCounter(void) {
    SetTickCountOn(0);
}

RVA(0x0003bb60, 0x16)
void OpGetTickCounter(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, g_tickCounter);
}

RVA(0x0003bb80, 0x18)
void OpSetTickCounter(void) {
    SetTickCounter(GetScriptLongVar(ReadLongVarIndex()));
}

// Starts the countdown and saves the script file and entry it fires.
RVA(0x0003bba0, 0x1d)
void OpStartCountdown(void) {
    g_countdown = ReadScriptValue();
    ReadScriptBytePair(&s_countdownFile, &s_countdownEntry);
}

RVA(0x0003bbc0, 0x11)
void TickCounter(void) {
    if (s_tickCountOn > 0) {
        g_tickCounter++;
    }
}

RVA(0x0003bbe0, 0x26)
void DrawDownCountdown(u16 amount) {
    if (g_countdown < amount) {
        amount = (u16)g_countdown;
    }
    g_countdown -= amount;
}

// When the countdown has run out, closes the message window and runs the
// saved script entry as a scene (game state 0x15); 1 when it fired.
RVA(0x0003bc10, 0x5e)
b16 FireCountdownEvent(void) {
    if (g_countdown != 0) {
        return false;
    }
    if (!s_countdownFile || !s_countdownEntry) {
        return false;
    }
    CloseMessageWindow();
    PushFieldTextScene(s_countdownFile, s_countdownEntry);
    s_countdownEntry = 0;
    s_countdownFile = 0;
    return true;
}

// The party position of the class-2 human member with the highest
// familiarity (on a tie, the lowest id) who is free to be picked; -1 when
// none.
RVA(0x0003bc70, 0x7b)
i16 FindFavouredMember(void) {
    i16 best = -1;
    u8 bestFamiliarity = 0;
    i16 bestId = -1;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character && IsHumanCharacter(character) && character->memberClass == 2
            && !GetPickBlockingCondition(GetCharacterConditions(character))) {
            if (best != -1) {
                if (character->familiarity < bestFamiliarity) {
                    continue;
                }
                if (character->familiarity == bestFamiliarity && character->id > bestId) {
                    continue;
                }
            }
            bestId = character->id;
            bestFamiliarity = character->familiarity;
            best = i;
        }
    }
    return best;
}

// The position of entry `entry` in the entry table that starts a code block.
RVA(0x0003bcf0, 0x17)
i16 ScriptEntryPc(u32 code, i16 entry) {
    ScriptCode* data = HandleReadPtr(code);
    return GetScriptRange(data, entry)->offset;
}

RVA(0x0003bd10, 0x26)
ScriptEntry MakeScriptEntry(ScriptBlock* block, i16 entry) {
    ScriptEntry result;
    result.code = block->code;
    result.pc = ScriptEntryPc(result.code, entry);
    return result;
}

RVA(0x0003bd40, 0xf4)
void OpPeekPokeScratch(void) {
    u32 value = 0;
    i16 width;
    i16 index;
    i16 operation;
    ReadScriptValue();
    ReadScriptValue();
    width = ReadScriptValue();
    index = ReadLongVarIndex();
    operation = ReadScriptValue();
    if (operation == -1) {
        switch (width) {
            case 1:
                value = s_scratchValue.value & 0xff;
                break;
            case 2:
                value = s_scratchValue.value & 0xffff;
                break;
            case 3:
            case 4:
                value = s_scratchValue.value;
                break;
        }
        SetScriptLongVar(index, value);
    } else if (operation == -2) {
        value = GetScriptLongVar(index);
        switch (width) {
            case 1:
                s_scratchValue.byte = value;
                break;
            case 2:
                s_scratchValue.word = value;
                break;
            case 3:
            case 4:
                s_scratchValue.value = value;
                break;
        }
    } else {
        SetScriptLongVar(index, g_scriptRegs[operation]);
    }
}

RVA(0x0003be40, 0x29)
i16 AccessScriptReg(i16 write, i16 index, i16 value) {
    if (write) {
        g_scriptRegs[index] = value;
    }
    return g_scriptRegs[index];
}

// Bank 15 flags 0..25 mark which script variables are set.
RVA(0x0003be70, 0x27)
void ClearScriptLongVars(void) {
    i16 i;
    for (i = 0; i < 26; i++) {
        g_scriptLongVars[i] = 0;
        ClearEventFlag(15, i);
    }
}

RVA(0x0003bea0, 0x2a)
void ClearSystemVars(void) {
    i16 i;
    for (i = 0; i < 8; i++) {
        g_scriptLongVars[18 + i] = 0;
        ClearEventFlag(15, i + 18);
    }
}

RVA(0x0003bed0, 0x29)
void ClearScriptLongVar(i16 index) {
    if (index >= 0 && index < 26) {
        g_scriptLongVars[index] = 0;
        ClearEventFlag(15, index);
    }
}

// Copies out script variables 18..25 and the bank-15 settings word.
RVA(0x0003bf00, 0x2c)
void SaveSystemVars(u32* vars, u32* settings) {
    i16 i;
    i16 var;
    for (i = 0; i < 8; i++) {
        var = i + 18;
        vars[i] = g_scriptLongVars[var];
    }
    *settings = GetFlagSettings();
}

RVA(0x0003bf30, 0x2d)
void RestoreSystemVars(u32* vars, u32* settings) {
    i16 i;
    for (i = 0; i < 8; i++) {
        g_scriptLongVars[18 + i] = vars[i];
    }
    SetFlagSettings(*settings);
}

// Sets a variable and marks it set (bank 15); returns `value`.
RVA(0x0003bf60, 0x39)
u32 SetScriptLongVar(i16 index, u32 value) {
    if (index == -1 || index < 0 || index >= 26) {
        return value;
    }
    g_scriptLongVars[index] = value;
    SetEventFlag(15, index);
    return value;
}

RVA(0x0003bfa0, 0x1e)
u32 GetScriptLongVar(i16 index) {
    if (index >= 0 && index < 26) {
        return g_scriptLongVars[index];
    }
    return 0;
}

// Swaps two variables with their set marks.
// @early-stop register residue: retail keeps `a` in ebx, `b` in edi and the
// swap temporary in esi; the order of the swap, a flag local, `!=`, an early
// return and an i32 temporary are all flat, and the permuter found one island.
RVA(0x0003bfc0, 0x79)
void SwapScriptLongVars(i16 a, i16 b) {
    u32 value;
    i32 changed;
    if (a >= 0 && a < 26 && b >= 0 && b < 26) {
        value = g_scriptLongVars[a];
        g_scriptLongVars[a] = g_scriptLongVars[b];
        g_scriptLongVars[b] = value;
        changed = TestEventFlag(15, a);
        changed ^= TestEventFlag(15, b);
        if (changed) {
            ToggleEventFlag(15, a);
            ToggleEventFlag(15, b);
        }
    }
}

// Copies a variable and its set mark.
RVA(0x0003c040, 0x59)
void CopyScriptLongVar(i16 dst, i16 src) {
    if (dst >= 0 && dst < 26 && src >= 0 && src < 26) {
        g_scriptLongVars[dst] = g_scriptLongVars[src];
        if (!TestEventFlag(15, src)) {
            ClearEventFlag(15, dst);
        } else {
            SetEventFlag(15, dst);
        }
    }
}

RVA(0x0003c0a0, 0x12)
i16 SetHold(i16 on) {
    i16 prev = s_holdOn;
    s_holdOn = on;
    return prev;
}

// Waits for input or time (the operand selects the kind: 0 and 2 read a
// frame count, 1 waits on input mask 2). While held, runs the wait here until
// it ends and returns 0; otherwise returns -3 to leave the script loop.
RVA(0x0003c0c0, 0x85)
GZ_ENUM_RETURN(ScriptStatus, i16) OpWaitMessage(i16 window) {
    GZ_ENUM_STORAGE(WaitMode, i16) kind;
    u16 mask;
    u16 frames;
    ReadScriptByte();
    kind = ReadScriptByte();
    frames = 0;
    mask = 0xffff;
    switch (kind) {
        case WAIT_INPUT:
            mask = 2;
            break;
        case WAIT_FRAMES:
        case WAIT_INPUT_OR_FRAMES:
            frames = ReadScriptByte();
            break;
    }
    PushWaitState(kind, mask, frames, window);
    if (s_holdOn) {
        while (GetGameState() == GAME_STATE_WAIT) {
            PollIdle(1, 0x18);
            StepWaitState();
        }
        return SCRIPT_CONTINUE;
    }
    return SCRIPT_YIELD;
}

RVA(0x0003c150, 0xf)
void OpPushGameState(void) {
    PushGameState(ReadScriptByte());
}

RVA(0x0003c160, 0x29)
i32 NewCallFrame(void) {
    i32 frame = AllocArrayHandle(1, sizeof(ScriptFrame));
    ScriptFrame* data = HandleWritePtr(frame);
    data->next = 0;
    data->prev = 0;
    return frame;
}

// Frees every frame of a call stack; returns the (empty) stack.
RVA(0x0003c190, 0x42)
i32 FreeCallFrames(i32 stack) {
    i32 frame;
    stack = PopListTail(stack, &frame);
    while (frame != 0) {
        FreeCallFrame(frame);
        stack = PopListTail(stack, &frame);
    }
    return stack;
}

RVA(0x0003c1e0, 0xe)
b32 FreeCallFrame(i32 frame) {
    return FreeHandle(frame);
}

// Saves the return position, the call arguments and the system variables
// (which are then cleared unless `keepVars`), and pushes the frame.
RVA(0x0003c1f0, 0x6a)
i32 PushCallFrame(ScriptContext* script, i16 keepVars) {
    i32 frame = NewCallFrame();
    ScriptFrame* data = HandleWritePtr(frame);
    data->codeBase = script->codeBase;
    data->pc = script->pc;
    data->argA = g_scriptArgA;
    data->argB = g_scriptArgB;
    SaveSystemVars(data->sysVars, &data->settings);
    if (!keepVars) {
        ClearSystemVars();
    }
    script->callStack = AppendList(script->callStack, frame);
    return script->callStack;
}

// Pops a frame; unless `discard`, returns to it and restores what it saved.
// With no frame left and not discarding, the script ends.
RVA(0x0003c260, 0x89)
void PopCallFrame(ScriptContext* script, i16 discard) {
    i32 frame;
    ScriptFrame* data;
    script->callStack = PopListTail(script->callStack, &frame);
    if (frame == 0) {
        if (discard == 0) {
            ClearScriptPosition(script);
        }
        return;
    }
    data = HandleReadPtr(frame);
    if (discard == 0) {
        RestoreSystemVars(data->sysVars, &data->settings);
        script->codeBase = data->codeBase;
        script->pc = data->pc;
        g_scriptArgA = data->argA;
        g_scriptArgB = data->argB;
    }
    FreeCallFrame(frame);
}

// Discards every call frame of `script`.
RVA(0x0003c2f0, 0x20)
void UnwindCallFrames(ScriptContext* script) {
    while (script->callStack) {
        PopCallFrame(script, 1);
    }
}

// Swaps the two newest call frames.
RVA(0x0003c310, 0x5c)
void SwapTopCallFrames(ScriptContext* script) {
    i32 first;
    i32 second;
    script->callStack = PopListTail(script->callStack, &first);
    if (first != 0) {
        script->callStack = PopListTail(script->callStack, &second);
        script->callStack = AppendList(script->callStack, first);
        if (second != 0) {
            script->callStack = AppendList(script->callStack, second);
        }
    }
}

// Moves the system variables between the running script and a call frame:
// `load` restores them from the frame, otherwise they are saved into it.
RVA(0x0003c370, 0x44)
void TransferFrameVars(i32 frame, i16 load) {
    ScriptFrame* data;
    if (!load) {
        data = HandleWritePtr(frame);
        SaveSystemVars(data->sysVars, &data->settings);
    } else {
        data = HandleReadPtr(frame);
        RestoreSystemVars(data->sysVars, &data->settings);
    }
}

RVA(0x0003c3c0, 0x11)
i32 TopCallFrame(ScriptContext* script) {
    return GetListTail(script->callStack);
}

// Shows a background picture read from the script (its first two operands;
// two more are read and ignored). Without event flag 9:123 picture 0x31 is
// shown instead; pictures 0x4c and 7 also empty sprite slot 31 or 0.
RVA(0x0003c3e0, 0x6a)
void OpShowBackground(void) {
    i16 image = ReadScriptValue();
    i16 arg = ReadScriptValue();
    ReadScriptValue();
    ReadScriptValue();
    if (!IsEventFlagSet(9, 0x7b)) {
        image = 0x31;
        arg = 4;
    }
    if (image == 0x4c) {
        UnplaceSprite(0x1f);
    } else if (image == 7) {
        UnplaceSprite(0);
    }
    ShowBackground(image, arg);
    RefreshPalette();
    StepBlankRenderMode();
}

RVA(0x0003c450, 0xf)
void OpRestoreBackground(void) {
    RestoreBackground();
    RefreshPalette();
}

RVA(0x0003c460, 0x77)
void OpShowPicture(void) {
    ImageRequest request;
    i16 image = ReadScriptValue();
    i16 variant = ReadScriptValue();
    i16 mode = ReadScriptValue();
    ReadScriptValue();
    image += 0x5000;
    request.file = image;
    request.variant = variant;
    request.flags = 1;
    LoadRequestedScenePicture(request, mode);
    if (image != 0x505c) {
        WaitFrames(10);
    }
}

RVA(0x0003c4e0, 0xb7)
void OpShowEventPicture(void) {
    ImageRequest request;
    i16 image = ReadScriptValue();
    i16 variant = ReadScriptValue();
    i16 mode = ReadScriptValue();
    ReadScriptValue();
    ReadScriptValue();
    image += 0x5000;
    request.file = image;
    request.variant = variant;
    request.flags = 1;
    LoadRequestedScenePicture(request, mode);
    if ((image > 0x507f && image < 0x5090) || (image > 0x5091 && image < 0x5099)
        || (image > 0x5099 && image < 0x50c8)) {
        WaitFrames(0);
    } else if (image == 0x506e) {
        UnplaceSprite(1);
    }
}

RVA(0x0003c5a0, 0xfd)
void OpSaveRestoreScreen(void) {
    i16 index = ReadLongVarIndex();
    i16 flags = ReadScriptValue();
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    i16 width = ReadScriptValue();
    i16 height = ReadScriptValue();
    i16 state = 0;
    i32 handle;
    ScreenSaveHeader* save;
    if (flags & 1) {
        handle = AllocScreenSaveHandle(width, height);
        save = HandleWritePtr(handle);
        if (!(flags & 2)) {
            state = SaveDrawState();
        }
        SetTextCursorOffset(&save->offset, x, y);
        if (!(flags & 2)) {
            RestoreDrawState(state);
        }
    } else {
        handle = GetScriptLongVar(index);
        save = HandleWritePtr(handle);
        save->offset = y * 80 + x;
        if (!(flags & 2)) {
            state = SaveDrawState();
        }
        ApplyTextCursor(save);
        if (!(flags & 2)) {
            RestoreDrawState(state);
        }
        handle = FreeHandle(handle);
    }
    SetScriptLongVar(index, handle);
}
