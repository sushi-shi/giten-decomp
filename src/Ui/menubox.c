// @identity-TODO: the owning TU is unproven; this unit holds the contiguous
// menu-box lifecycle and input span until link-order evidence names it.

#include <rva.h>

#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Text/Font.h>
#include <Text/TextWindow.h>
#include <Ui/Menu.h>
#include <Ui/Panel.h>

#include <stddef.h>

RVA(0x000206e0, 0x42)
MenuBox* DestroyMenuBox(MenuBox* menu) {
    if (menu == NULL) {
        return NULL;
    }
    DispatchMenuEvent(menu, 0, MENU_EVENT_DESTROY);
    menu->list = ReleasePanel(menu->list, 1);
    CloseTextWindow(menu->plane);
    return FreeBlock(menu);
}

RVA(0x00020730, 0x1c)
void DispatchMenuEvent(MenuBox* menu, i16 index, i16 event) {
    if (menu->handler) {
        menu->handler(menu, index, event);
    }
}

RVA(0x00020750, 0x7d)
MenuBox* CreateMenuBox(MenuBox* old, i16 window, i16 panelRows) {
    MenuBox* menu;
    if (old) {
        DestroyMenuBox(old);
    }
    menu = AllocCleared(1, sizeof(MenuBox));
    menu->plane = CreateTextPlane(window, 0);
    menu->pageRows = GetTextPlaneLineCount(menu->plane);
    menu->list = CreatePositionedPanel(NULL, -1, -1, panelRows, window);
    menu->cursor = 0;
    menu->itemCount = 0;
    menu->items.text = NULL;
    menu->handler = NULL;
    menu->context.value = 0;
    RequestMenuRedraw(menu);
    menu->flagBits.repaintMode = -2;
    return menu;
}

RVA(0x000207d0, 0x2d)
void MoveMenuBox(MenuBox* menu, i16 x, i16 y) {
    if (menu) {
        if (x != -1) {
            menu->list->x = x;
        }
        if (y != -1) {
            menu->list->y = y;
        }
    }
}

RVA(0x00020800, 0x2f)
void SetMenuItems(MenuBox* menu, i16 pageRows, void* items, i16 itemCount, MenuHandler handler) {
    if (menu) {
        if (pageRows != -1) {
            menu->pageRows = pageRows;
        }
        menu->itemCount = itemCount;
        menu->items.text = items;
        menu->handler = handler;
    }
}

RVA(0x00020830, 0xbd)
void BuildMenuPage(MenuBox* menu) {
    i16 previous;
    i16 next;
    i16 index;
    i16 end;
    if (menu == NULL) {
        return;
    }
    previous = next = 0;
    FreeMenuLines(menu->plane);
    DispatchMenuEvent(menu, 0, MENU_EVENT_BEGIN_PAGE);
    index = menu->cursor;
    end = index + menu->pageRows;
    if (end >= menu->itemCount) {
        end = menu->itemCount;
        next = PANEL_HIDDEN;
    }
    if (index == 0) {
        previous = PANEL_HIDDEN;
    }
    for (; index < end; index++) {
        DispatchMenuEvent(menu, index, MENU_EVENT_ADD_ROW);
    }
    DispatchMenuEvent(menu, index, MENU_EVENT_END_PAGE);
    SetPanelRowState(menu->list, MENU_CONTROL_PREVIOUS_PAGE, previous);
    SetPanelRowState(menu->list, MENU_CONTROL_NEXT_PAGE, next);
    SetPanelRowState(menu->list, MENU_CONTROL_PREVIOUS_ROW, previous);
    SetPanelRowState(menu->list, MENU_CONTROL_NEXT_ROW, next);
}

RVA(0x000208f0, 0x94)
void PaintMenuBox(MenuBox* menu) {
    if (menu == NULL) {
        return;
    }
    DispatchMenuEvent(menu, 0, MENU_EVENT_BEFORE_TEXT);
    ResetTextPlaneHighlight(menu->plane);
    PrintMenuLines(menu->plane);
    DispatchMenuEvent(menu, 0, MENU_EVENT_AFTER_TEXT);
    RepaintTextPlane(menu->plane, menu->flagBits.repaintMode);
    DispatchMenuEvent(menu, 0, MENU_EVENT_BEFORE_PANEL);
    ClearPanelChecksAgain(menu->list);
    PaintPanel(menu->list, menu->plane);
    DispatchMenuEvent(menu, 0, MENU_EVENT_AFTER_PANEL);
    menu->flagBits.redraw = 0;
}

RVA(0x00020990, 0x46)
i16 PollMenuBox(MenuBox* menu) {
    i16 control;
    if (menu == NULL) {
        return -1;
    }
    control = RunPanelInput(menu->list);
    if (control == -2) {
        return -1;
    }
    if (control >= 0) {
        return HandleMenuControl(menu, control);
    }
    return PollMenuInput(menu->plane);
}

RVA(0x000209e0, 0xa4)
i16 HandleMenuControl(MenuBox* menu, i16 control) {
    i16 cursor = menu->cursor;
    switch (control) {
        case MENU_CONTROL_PREVIOUS_PAGE:
            menu->cursor -= menu->pageRows;
            if (menu->cursor < 0) {
                menu->cursor = 0;
            }
            break;
        case MENU_CONTROL_NEXT_PAGE:
            if (menu->cursor + menu->pageRows < menu->itemCount) {
                menu->cursor += menu->pageRows;
            }
            break;
        case MENU_CONTROL_PREVIOUS_ROW:
            menu->cursor--;
            if (menu->cursor < 0) {
                menu->cursor = 0;
            }
            break;
        case MENU_CONTROL_NEXT_ROW:
            if (menu->cursor + 1 < menu->itemCount) {
                menu->cursor++;
            }
            break;
        default:
            DispatchMenuEvent(menu, control, MENU_EVENT_CONTROL);
    }
    if (cursor != menu->cursor) {
        RequestMenuRedraw(menu);
    }
    return 0;
}

RVA(0x00020a90, 0x32)
i16 RunMenu(MenuBox* menu) {
    if (menu == NULL) {
        return -1;
    }
    if (menu->flagBits.redraw) {
        BuildMenuPage(menu);
        PaintMenuBox(menu);
    }
    return PollMenuBox(menu);
}

RVA(0x00020ad0, 0x1b)
i16 RunListMenu(MenuBox* menu) {
    i16 result = RunMenu(menu);
    if (result <= 0) {
        return result - 1;
    }
    return g_selectedObjectId;
}

RVA(0x00020af0, 0xe)
MenuBox* CloseListMenu(MenuBox* menu) {
    return DestroyMenuBox(menu);
}
