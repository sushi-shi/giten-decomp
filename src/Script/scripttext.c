// @identity-TODO: the owning TU is unproven. One retail object: the script
// panels and the script text code. The panels' list head sits inside the
// text code's .bss run, between its zero-initialized text buffers.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/BagItems.h>
#include <Game/FieldHud.h>
#include <Game/ItemMenu.h>
#include <Game/Scene.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/VramAccess.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptPanel.h>
#include <Script/ScriptText.h>
#include <Script/ScriptVars.h>
#include <Script/TextState.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Ui/MenuBox.h>
#include <Ui/Message.h>
#include <Ui/Panel.h>
#include <Util/List.h>
#include <Util/Scratch.h>
#include <Util/Text.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// @identity-TODO: the original source unit for this shared work buffer is
// unproven; text formatting is its most frequent use.
DATA(0x00091340)
char g_scratchBuffer[0x200];

// The formatted text line the script number/string opcodes write into.
DATA(0x00081018)
char g_textLine[0x100] = {0};

DATA(0x00081118)
static ScriptPanel* s_scriptPanels = NULL;

DATA(0x00081120)
char g_capturedText[0x100] = {0};

// The script's stacked windows, newest last.
DATA(0x00081220)
static ScriptWindowNode* s_windowStack = NULL;

// While set, printable script text is appended to the captured-text buffer
// instead of being drawn.
DATA(0x00081224)
i16 g_textCaptureOn = 0;

RVA(0x0002ece0, 0x36)
void ErasePanelPictures(Panel* panel) {
    i16 i;
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        ErasePictureSurface(GetPanelRowId(GetPanelRow(panel, i)));
    }
}

RVA(0x0002ed20, 0x5b)
void DestroyScriptPanel(ScriptPanel* node) {
    RestoreSavedCursor(HandleReadPtr(node->screenSave));
    FreeHandle(node->screenSave);
    FreeBlock(node->jumps);
    node->flags.drawn = false;
    ErasePanelPictures(node->panel);
    ReleasePanel(node->panel, true);
    FreeBlock(node);
}

RVA(0x0002ed80, 0x29)
void CloseLastScriptPanel(void) {
    ScriptPanel* node = ListLast(s_scriptPanels);
    if (node) {
        ListUnlink(node);
        DestroyScriptPanel(node);
    }
}

RVA(0x0002edb0, 0x18)
void FreeScriptPanels(void) {
    while (s_scriptPanels) {
        CloseLastScriptPanel();
    }
}

RVA(0x0002edd0, 0x5)
void ResetScriptPanels(void) {
    FreeScriptPanels();
}

RVA(0x0002ede0, 0xa)
void CloseScriptInterface(void) {
    FreeScriptPanels();
    CloseItemMenu();
}

RVA(0x0002edf0, 0xb7)
ScriptPanel* CreateScriptPanel(i16 image, i16 count, i16 x, i16 y) {
    ScriptPanel* node = AllocCleared(1, sizeof(ScriptPanel));
    i16 i;
    MapCoord size;
    node->next = NULL;
    node->prev = NULL;
    node->panel = CreateImagePanel(NULL, image, count, 0);
    SetPanelPosition(node->panel, x, y);
    node->jumps = AllocCleared(count, sizeof(ScriptPanelJump));
    for (i = 0; i < count; i++) {
        GetScriptPanelJump(node, i)->value = SCRIPT_PANEL_NO_JUMP;
    }
    node->image = image;
    ListAppend(&s_scriptPanels, node);
    size = GetPanelSize(node->panel);
    node->screenSave = AllocScreenSaveHandle(size.x, size.y);
    node->flags.drawn = false;
    return node;
}

RVA(0x0002eeb0, 0x42)
void DrawScriptPanel(ScriptPanel* node) {
    i16* cell;
    i16 token;
    if (!node->flags.drawn) {
        cell = HandleWritePtr(node->screenSave);
    } else {
        cell = NULL;
    }
    token = SaveDrawState();
    ClearPanel(node->panel, cell);
    RestoreDrawState(token);
    node->flags.drawn = true;
}

RVA(0x0002ef00, 0x3e)
b16 CloseScriptPanelByImage(i16 image) {
    ScriptPanel* node;
    for (node = s_scriptPanels; node; node = node->next) {
        if (node->image == image) {
            ListUnlink(node);
            DestroyScriptPanel(node);
            return true;
        }
    }
    return false;
}

RVA(0x0002ef40, 0x3d)
void OpOpenScriptPanel(void) {
    i16 image = ReadScriptValue();
    i16 count = ReadScriptValue();
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    if (image == 0x113) {
        ClearPool();
    }
    CreateScriptPanel(image, count, x, y);
}

RVA(0x0002ef80, 0xf)
void OpCloseScriptPanel(void) {
    CloseScriptPanelByImage(ReadScriptValue());
}

RVA(0x0002ef90, 0x5)
void CloseAllScriptPanels(void) {
    FreeScriptPanels();
}

RVA(0x0002efa0, 0x34)
void OpSetPanelEntryJump(void) {
    ScriptPanel* node = ListLast(s_scriptPanels);
    i16 index = ReadScriptValue();
    u16 jump = ReadScriptWord();
    if (GetPanelRowCount(node->panel) > index) {
        GetScriptPanelJump(node, index)->value = jump;
    }
}

RVA(0x0002efe0, 0x1c)
void DrawScriptPanels(void) {
    ScriptPanel* node;
    for (node = s_scriptPanels; node; node = node->next) {
        DrawScriptPanel(node);
    }
}

// @identity-TODO: the Windows body consumes two operands without using them.
RVA(0x0002f000, 0x18)
void OpSkipPanelOperands(void) {
    ListLast(s_scriptPanels);
    ReadScriptValue();
    ReadScriptValue();
}

RVA(0x0002f020, 0x2f)
void SetLastPanelRowState(i16 index, u16 flags) {
    ScriptPanel* node = ListLast(s_scriptPanels);
    Panel* panel = node->panel;
    if (GetPanelRowCount(panel) > index) {
        AssignPanelRowState(panel, index, flags);
    }
}

RVA(0x0002f050, 0x19)
void OpSetPanelEntryValue(void) {
    i16 index = ReadScriptValue();
    u16 flags = ReadScriptValue();
    SetLastPanelRowState(index, flags);
}

// @early-stop register allocation: retail holds count in esi and last in edi;
// cl swaps them. Declaration, initialization and loop statement order do not
// recover that allocation.
RVA(0x0002f070, 0xd2)
i16 PollScriptPanels(void) {
    ScriptPanel* node;
    ScriptPanel* last;
    i16 count;
    i16 row;
    ScriptPanelJump jump;
    PollScriptItemMenu();
    last = NULL;
    count = 0;
    for (node = s_scriptPanels; node; node = node->next) {
        last = node;
        count++;
    }
    if (last) {
        count++;
        while (count > 1) {
            count--;
            row = PollPanel(last->panel);
            if (row == PANEL_INPUT_NONE) {
                last = last->prev;
            } else {
                jump = *GetScriptPanelJump(last, row);
                if (jump.value != SCRIPT_PANEL_NO_JUMP) {
                    CallScript(jump.parts.file, jump.parts.entry);
                    SetScriptLongVar(0x19, row);
                    SetScriptLongVar(0x18, IsPanelRowChecked(last->panel, row));
                    SetScriptLongVar(0x17, !WasPanelRightClicked(last->panel));
                    SetScriptLongVar(0x16, count);
                }
                return row;
            }
        }
    }
    return PANEL_INPUT_NONE;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x0002f150, 0x30)
b16 DrawScriptPanelByImage(i16 image) {
    ScriptPanel* node;
    for (node = s_scriptPanels; node; node = node->next) {
        if (node->image == image) {
            DrawScriptPanel(node);
            return true;
        }
    }
    return false;
}

RVA(0x0002f180, 0x3b)
void OpSetLastPanelFlag(void) {
    ScriptPanel* node = ListLast(s_scriptPanels);
    if (!ReadScriptValue()) {
        ClearPanelFlags(node->panel, PANEL_INPUT_DISABLED);
    } else {
        SetPanelFlags(node->panel, PANEL_INPUT_DISABLED);
    }
}

RVA(0x0002f1c0, 0x10b)
void OpCreateScriptMenu(void) {
    i16 variable = ReadLongVarIndex();
    i16 window = ReadScriptValue();
    i16 pageRows = ReadScriptValue();
    u32 tag = ReadScriptValue();
    i16 itemCount = ReadScriptValue();
    i16 file;
    i16 entry;
    i16 panelRows;
    i16 x;
    i16 y;
    i16 cancelEnabled;
    i16 highlight;
    MenuBox* menu;
    ReadScriptBytePair(&file, &entry);
    panelRows = ReadScriptValue();
    x = ReadScriptValue();
    y = ReadScriptValue();
    cancelEnabled = ReadScriptValue();
    highlight = ReadScriptValue();
    menu = CreateMenuBox(NULL, window, panelRows);
    MoveMenuBox(menu, x, y);
    SetMenuItems(menu, pageRows, (void*)tag, itemCount, RunScriptMenuHandler);
    SetTextPlaneCancelEnabled(menu->plane, cancelEnabled);
    if (!cancelEnabled) {
        SetPanelFlags(menu->list, PANEL_ALLOW_RIGHT_CLICK | PANEL_IGNORE_RIGHT_CLICK);
    } else {
        ClearPanelFlags(menu->list, PANEL_ALLOW_RIGHT_CLICK);
    }
    SetTextPlaneHighlightMode(menu->plane, highlight);
    menu->context.script.file = file;
    menu->context.script.entry = entry;
    SetScriptLongVar(variable, (u32)menu);
}

// A script menu's handler: runs the menu's script with long variables
// 0x12..0x16 set to the menu, the event + 1, the item index, the cursor and
// the index less the cursor.
RVA(0x0002f2d0, 0x6c)
void RunScriptMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    CallScript(menu->context.script.file, menu->context.script.entry);
    SetScriptLongVar(0x12, (u32)menu);
    SetScriptLongVar(0x13, event + 1);
    SetScriptLongVar(0x14, index);
    SetScriptLongVar(0x15, menu->cursor);
    SetScriptLongVar(0x16, index - menu->cursor);
    RunCurrentScript();
}

RVA(0x0002f340, 0x29)
void OpRunScriptMenu(void) {
    MenuBox* menu = (MenuBox*)ReadScriptValue();
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, RunMenu(menu));
}

RVA(0x0002f370, 0x26)
void OpDestroyScriptMenu(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, (u32)DestroyMenuBox((MenuBox*)GetScriptLongVar(index)));
}

// Adds the formatted text line to a script menu.
RVA(0x0002f3a0, 0x35)
void OpAddMenuLine(void) {
    MenuBox* menu = (MenuBox*)ReadScriptValue();
    i16 attr = ReadScriptValue();
    i16 value = ReadScriptValue();
    i16 flags = ReadScriptValue();
    AddMenuLine(menu->plane, g_textLine, attr, value, flags);
}

// @identity-TODO: the menu's `items` word read as a value; for a script
// menu it is the third creation operand.
RVA(0x0002f3e0, 0x1c)
void OpGetMenuTag(void) {
    MenuBox* menu = (MenuBox*)ReadScriptValue();
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, menu->items.tag);
}

// Sets a menu line's text attribute and repaints its text run.
RVA(0x0002f400, 0x54)
void OpSetMenuLineColor(void) {
    MenuBox* menu = (MenuBox*)ReadScriptValue();
    i16 line = ReadScriptValue();
    u16 attr;
    TextPoint pos;
    attr = ReadScriptValue();
    SetMenuLineAttr(menu->plane, line, attr);
    pos = GetMenuLinePos(menu->plane, line);
    PaintTextRun(menu->plane, pos.x, pos.y, attr);
}

RVA(0x0002f460, 0x12)
void OpClearMenuHighlight(void) {
    ClearTextPlaneHighlight(((MenuBox*)ReadScriptValue())->plane);
}

RVA(0x0002f480, 0x1d)
void OpGetMenuCursor(void) {
    MenuBox* menu = (MenuBox*)ReadScriptValue();
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, menu->cursor);
}

RVA(0x0002f4a0, 0xa)
void OpRedrawScriptMenu(void) {
    MenuBox* menu = (MenuBox*)ReadScriptValue();
    menu->flags |= 1;
}

// Sets a script menu's first selectable line and resets its line step
// (-1 preserves either setting).
RVA(0x0002f4b0, 0x40)
void OpSetMenuScroll(void) {
    MenuBox* menu = (MenuBox*)ReadScriptValue();
    i16 pos = ReadScriptValue();
    i16 step = ReadScriptValue();
    if (step != -1) {
        ResetTextPlaneLineStep(menu->plane, step);
    }
    if (pos != -1) {
        SetTextPlaneFirstSelectableRow(menu->plane, pos, true);
    }
}

RVA(0x0002f4f0, 0xc)
void SetTextCapture(i16 on) {
    g_textCaptureOn = on;
}

RVA(0x0002f500, 0x5d)
b16 CaptureTextChar(u16 ch) {
    if (!g_textCaptureOn) {
        return false;
    }
    AppendTextChar(g_capturedText, ch);
    return true;
}

RVA(0x0002f560, 0x1d)
void ClearTextBuffers(void) {
    memset(g_capturedText, 0, sizeof(g_capturedText));
    memset(g_textLine, 0, sizeof(g_textLine));
}

RVA(0x0002f580, 0x11)
void ClearCapturedText(void) {
    memset(g_capturedText, 0, sizeof(g_capturedText));
}

static __inline void WriteFormattedTextLine(i16 column) {
    i16 i;
    for (i = 0; i < column; i++) {
        if (g_textLine[i] == 0) {
            g_textLine[i] = ' ';
        }
    }
    for (i = 0; g_scratchBuffer[i]; i++) {
        g_textLine[column + i] = g_scratchBuffer[i];
    }
}

// Formats a number `width` wide with at least `digits` digits (left-aligned
// with `left`) into the text line at `column`, padding the line up to the
// column with spaces.
RVA(0x0002f5a0, 0x98)
void OpFormatNumber(void) {
    i16 column = ReadScriptValue();
    i16 width = ReadScriptValue();
    i16 digits = ReadScriptValue();
    i16 left = ReadScriptValue();
    i32 value = ReadScriptValue();
    if (left) {
        sprintf(g_scratchBuffer, "%-*.*ld", width, digits, value);
    } else {
        sprintf(g_scratchBuffer, "%*.*ld", width, digits, value);
    }
    WriteFormattedTextLine(column);
}

// The same for the captured text, `width` wide and at most `length` long.
RVA(0x0002f640, 0x93)
void OpFormatCapturedText(void) {
    i16 column = ReadScriptValue();
    i16 width = ReadScriptValue();
    i16 length = ReadScriptValue();
    i16 left = ReadScriptValue();
    if (left) {
        sprintf(g_scratchBuffer, "%-*.*s", width, length, g_capturedText);
    } else {
        sprintf(g_scratchBuffer, "%*.*s", width, length, g_capturedText);
    }
    WriteFormattedTextLine(column);
}

RVA(0x0002f6e0, 0x29)
void SetCapturedText(const char* text) {
    strcpy(g_capturedText, text);
}

// Stores in a long variable a handle to a new array of `count` longs.
RVA(0x0002f710, 0x24)
void OpAllocLongArray(void) {
    i16 index = ReadLongVarIndex();
    i16 count = ReadScriptValue();
    SetScriptLongVar(index, CreateArrayHandle(count, 4));
}

RVA(0x0002f740, 0x26)
void OpFreeLongArray(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, FreeHandle(GetScriptLongVar(index)));
}

RVA(0x0002f770, 0x3e)
void OpGetLongArrayItem(void) {
    i16 array = ReadLongVarIndex();
    i16 item = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, ((u32*)HandleReadPtr(GetScriptLongVar(array)))[item]);
}

RVA(0x0002f7b0, 0x34)
void OpSetLongArrayItem(void) {
    i16 array = ReadLongVarIndex();
    i16 item = ReadScriptValue();
    u32 value = ReadScriptValue();
    ((u32*)HandleWritePtr(GetScriptLongVar(array)))[item] = value;
}

// Loads data file `id` (of `kind`) into a handle stored in a long variable:
// encrypted (ReadCryptHandle) or raw.
// @identity-TODO: a nonzero `skip` never ends its loop over SkipRawBlock
// (nothing counts it down); what the operand was meant for is unrecovered.
RVA(0x0002f7f0, 0x7f)
void OpLoadDataFile(void) {
    i16 index = ReadLongVarIndex();
    i16 kind = ReadScriptValue();
    i16 id = ReadScriptValue();
    i16 skip = ReadScriptValue();
    i16 encrypted = ReadScriptValue();
    FILE* fp = OpenDataFile(id, kind, 0);
    i32 handle;
    while (skip) {
        SkipRawBlock(fp);
    }
    if (!encrypted) {
        handle = ReadRawHandle(fp);
    } else {
        handle = ReadCryptHandle(fp);
    }
    CloseDataFile(fp);
    SetScriptLongVar(index, handle);
}

// The same body as OpFreeLongArray; the split follows the opcode table.
RVA(0x0002f870, 0x26)
void OpFreeDataFile(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, FreeHandle(GetScriptLongVar(index)));
}

static __inline i32 ReadSizedDataInt(const u8* data, i16 offset, i16 size, i16 sign) {
    i32 value;
    value = 0;
    while (size) {
        size--;
        value = (value << 8) + data[offset + size];
        if (sign < 0) {
            sign = 1;
            if (data[offset + size] >= 0x80) {
                value -= 0x100;
            }
        }
    }
    return value;
}

// Reads a little-endian integer of `size` bytes (sign-extended when `size`
// is negative) at `offset` into record `record` of a loaded data file (a
// table of word offsets first).
RVA(0x0002f8a0, 0xa9)
void OpReadRecordInt(void) {
    i16 array = ReadLongVarIndex();
    i16 record = ReadScriptValue();
    i16 offset = ReadScriptValue();
    i16 size = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    i16 sign = 1;
    u8* data;
    i32 value;
    if (size < 0) {
        sign = -1;
        size = -size;
    }
    data = HandleReadPtr(GetScriptLongVar(array));
    offset += ((i16*)data)[record];
    value = ReadSizedDataInt(data, offset, size, sign);
    SetScriptLongVar(index, value);
}

// The same at `offset` into the data itself.
// @early-stop: retail keeps the value in ecx and a pointer (data + offset +
// size, decremented per byte) in edx; this spelling swaps the two registers
// and the permuter's search is flat.
RVA(0x0002f950, 0x93)
void OpReadDataInt(void) {
    i16 array = ReadLongVarIndex();
    i16 offset = ReadScriptValue();
    i16 size = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    i16 sign = 1;
    u8* data;
    i32 value;
    if (size < 0) {
        sign = -1;
        size = -size;
    }
    data = HandleReadPtr(GetScriptLongVar(array));
    value = ReadSizedDataInt(data, offset, size, sign);
    SetScriptLongVar(index, value);
}

// Stores in a long variable a handle to a copy of the running scene cell's
// record.
RVA(0x0002f9f0, 0x36)
void OpSaveSceneCell(void) {
    i16 index = ReadLongVarIndex();
    i32 handle = CreateArrayHandle(0x10, 1);
    GetSceneCell(HandleWritePtr(handle));
    SetScriptLongVar(index, handle);
}

RVA(0x0002fa30, 0x33)
void OpCaptureDataString(void) {
    i16 array = ReadLongVarIndex();
    i16 offset = ReadScriptValue();
    SetCapturedText((char*)HandleReadPtr(GetScriptLongVar(array)) + offset);
}

RVA(0x0002fa70, 0x46)
void OpCaptureRecordString(void) {
    i16 array = ReadLongVarIndex();
    i16 record = ReadScriptValue();
    i16 offset = ReadScriptValue();
    char* data = HandleReadPtr(GetScriptLongVar(array));
    SetCapturedText(data + (i16)(((i16*)data)[record] + offset));
}

// Runs the formatted text line (0) or the captured text as a text script.
RVA(0x0002fac0, 0x26)
void OpCallTextScript(void) {
    i16 source = ReadScriptValue();
    if (source == 0) {
        CallTextScript(g_textLine);
        return;
    }
    CallTextScript(g_capturedText);
}

// Stacks `window` as the script's newest window.
RVA(0x0002faf0, 0x29)
i16 StackScriptWindow(i16 window) {
    ScriptWindowNode* node = AllocCleared(1, sizeof(ScriptWindowNode));
    node->window = window;
    ListAppend(&s_windowStack, node);
    return window;
}

// Opens a text window and stacks it as the script's newest window.
RVA(0x0002fb20, 0x35)
i16 OpenScriptWindow(u16 kind, i16 arg) {
    i16 window = CreateTextPlane(kind, arg);
    SetTextWindowScrollTop(window, 2);
    SetTextScrollMode(true);
    return StackScriptWindow(window);
}

// Closes every stacked window.
RVA(0x0002fb60, 0x3f)
void CloseScriptWindows(void) {
    ScriptWindowNode* node;
    while (s_windowStack) {
        node = ListPopLast(s_windowStack);
        if (node) {
            if (node->window >= 0) {
                CloseTextWindow(node->window);
            }
            FreeBlock(node);
        }
    }
}

// The newest stacked window; -1 when none is.
RVA(0x0002fba0, 0x1c)
i16 TopScriptWindow(void) {
    ScriptWindowNode* node = ListLast(s_windowStack);
    if (!node) {
        return -1;
    }
    return node->window;
}

// Operand 0 stacks the message window; 1 drops the newest stacked window
// without closing it.
RVA(0x0002fbc0, 0x3b)
void OpStackMessageWindow(void) {
    ScriptWindowNode* node;
    i16 op = ReadScriptValue();
    switch (op) {
        case 0:
            StackScriptWindow(OpenMessageWindow());
            break;
        case 1:
            node = ListPopLast(s_windowStack);
            if (node) {
                FreeBlock(node);
            }
            break;
    }
}

// @identity-TODO: window kind numbers (5 is remapped to 0xe) are unrecovered.
RVA(0x0002fc00, 0x27)
void OpPushScriptWindow(void) {
    i16 window;
    i16 kind = ReadScriptValue();
    if (kind == 5) {
        kind = 0xe;
    }
    window = OpenScriptWindow(kind, 0);
    RepaintTextPlane(window, 1);
}

// Closes the newest stacked window.
RVA(0x0002fc30, 0x32)
void PopScriptWindow(void) {
    ScriptWindowNode* node = ListPopLast(s_windowStack);
    if (node) {
        if (node->window >= 0) {
            CloseTextWindow(node->window);
        }
        FreeBlock(node);
    }
}

RVA(0x0002fc70, 0x73)
b16 OpStepListMenu(void) {
    i16 mode = ReadScriptValue();
    i16 stepVar = ReadLongVarIndex();
    i16 step = GetScriptLongVar(stepVar);
    i16 resultVar = ReadLongVarIndex();
    i16 result = 0;
    switch (mode) {
        case 0:
            result = StepItemBuyMenu(&step);
            break;
        case 1:
            result = StepItemSellMenu(&step);
            break;
    }
    SetScriptLongVar(stepVar, step);
    SetScriptLongVar(resultVar, result);
    return false;
}

RVA(0x0002fcf0, 0xf)
void OpSetMenuCharacter(void) {
    SetItemMenuCharacter(ReadScriptValue());
}
