#ifndef GITEN_UI_MENUBOX_H
#define GITEN_UI_MENUBOX_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

struct Character;
struct MenuBox;
struct Panel;
struct ItemStackList;
struct PartyMemberList;

typedef union MenuContext {
    u32 value;
    struct {
        i16 entry;
        i16 file;
    } script;
    struct {
        u8 priceDivisor;
        u8 mode;
        i16 totalVar;
    } item;
} MenuContext;

// A row of a fixed menu table: whether the row is restricted, the value it
// selects, and its label.
// @identity-TODO: `restricted` rows show disabled while mode flag 2 is set.
typedef struct MenuEntry {
    i16 restricted : 8;
    i16 value : 8;
    char* label;
} MenuEntry;

// A row of the system menu's tables (savegame): its label, shown disabled
// while mode flag 2 is set when `restricted`.
typedef struct SystemMenuEntry {
    i16 restricted;
    char* label;
} SystemMenuEntry;

GZ_ENUM_BEGIN(MenuEvent)
    MENU_EVENT_DESTROY = -1,
    MENU_EVENT_BEGIN_PAGE = 0,
    MENU_EVENT_ADD_ROW = 1,
    MENU_EVENT_END_PAGE = 2,
    MENU_EVENT_CONTROL = 3,
    MENU_EVENT_BEFORE_TEXT = 4,
    MENU_EVENT_AFTER_TEXT = 5,
    MENU_EVENT_BEFORE_PANEL = 6,
    MENU_EVENT_AFTER_PANEL = 7
GZ_ENUM_END(MenuEvent)

GZ_ENUM_BEGIN(MenuControl)
    MENU_CONTROL_PREVIOUS_PAGE = 0,
    MENU_CONTROL_NEXT_PAGE = 1,
    MENU_CONTROL_PREVIOUS_ROW = 2,
    MENU_CONTROL_NEXT_ROW = 3
GZ_ENUM_END(MenuControl)

typedef void (*MenuHandler)(struct MenuBox* menu, i16 index, i16 event);

// A menu owns a text plane and a panel of paging controls. cursor is the
// first displayed item. Script menus retain a packed file/entry reference.
typedef struct MenuBox {
    i16 plane;
    i16 pageRows;
    struct Panel* list;
    i16 cursor;
    i16 itemCount;
    // What the handler lists, read as its handler needs: item strings, bag
    // entry indices (the bag discard menu), or a script menu's tag value.
    union {
        struct Character* character;
        char** text;
        i16* entries;
        struct MenuEntry* table;
        struct SystemMenuEntry* systemTable;
        struct ItemStackList* itemList;
        struct PartyMemberList* memberList;
        u32 tag;
    } items;
    MenuHandler handler;
    MenuContext context;
    union {
        u16 flags;
        struct {
            i16 redraw : 1;
            i16 repaintMode : 4;
        } flagBits;
    };
} MenuBox;

static __inline void RequestMenuRedraw(MenuBox* menu) {
    menu->flagBits.redraw = 1;
}

MenuBox* DestroyMenuBox(MenuBox* menu);

MenuBox* CreateMenuBox(MenuBox* old, i16 window, i16 panelRows);

// Moves the menu's list to (x, y); -1 keeps a coordinate.
void MoveMenuBox(MenuBox* menu, i16 x, i16 y);

// A pageRows of -1 keeps the plane's own.
void SetMenuItems(MenuBox* menu, i16 pageRows, void* items, i16 itemCount, MenuHandler handler);

i16 RunMenu(MenuBox* menu);
void DispatchMenuEvent(MenuBox* menu, i16 index, i16 event);
void BuildMenuPage(MenuBox* menu);
void PaintMenuBox(MenuBox* menu);
i16 PollMenuBox(MenuBox* menu);
b16 HandleMenuControl(MenuBox* menu, i16 control);

#endif // GITEN_UI_MENUBOX_H
