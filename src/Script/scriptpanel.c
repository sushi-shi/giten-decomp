// @identity-TODO: the owning TU is unproven; this unit holds the contiguous
// script-panel span until link-order evidence names it.

#include <rva.h>

#include <Game/BagItems.h>
#include <Game/FieldHud.h>
#include <Game/ItemMenu.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/VramAccess.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Script/Script.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptPanel.h>
#include <Script/ScriptVars.h>
#include <Ui/Panel.h>
#include <Util/List.h>

#include <stddef.h>

DATA(0x00081118)
static ScriptPanel* s_scriptPanels;

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
    node->flags.drawn = 0;
    ErasePanelPictures(node->panel);
    ReleasePanel(node->panel, 1);
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
        GetScriptPanelJump(node, i)->value = 0xffff;
    }
    node->image = image;
    ListAppend(&s_scriptPanels, node);
    size = GetPanelSize(node->panel);
    node->screenSave = AllocScreenSaveHandle(size.x, size.y);
    node->flags.drawn = 0;
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
    node->flags.drawn = 1;
}

RVA(0x0002ef00, 0x3e)
i16 CloseScriptPanelByImage(i16 image) {
    ScriptPanel* node;
    for (node = s_scriptPanels; node; node = node->next) {
        if (node->image == image) {
            ListUnlink(node);
            DestroyScriptPanel(node);
            return 1;
        }
    }
    return 0;
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
            if (row == -1) {
                last = last->prev;
            } else {
                jump = *GetScriptPanelJump(last, row);
                if (jump.value != 0xffff) {
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
    return -1;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x0002f150, 0x30)
i16 DrawScriptPanelByImage(i16 image) {
    ScriptPanel* node;
    for (node = s_scriptPanels; node; node = node->next) {
        if (node->image == image) {
            DrawScriptPanel(node);
            return 1;
        }
    }
    return 0;
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
