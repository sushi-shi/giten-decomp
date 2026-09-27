// @identity-TODO: the character status screen's TU (its statics bracket
// FormatCurMax, which sits between this unit's functions); the name is unproven.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/GameState.h>
#include <Game/StatusScreen.h>
#include <Input/Mouse.h>
#include <Text/TextWindow.h>
#include <Ui/Menu.h>
#include <Util/BitSet.h>
#include <Util/CurMax.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

DATA(0x00083b3c)
static MenuBox* s_statusListMenu;

// Which of the five status-line columns the character status line shows.
DATA(0x00083b40)
static i16 s_statusColumn;

// The roster slots the status screen lists and how many there are.
DATA(0x00083ad8)
i16 g_statusSlots[32];

DATA(0x00083b44)
i16 g_statusSlotCount;

RVA(0x00040980, 0x12)
i16 SetStatusColumn(i16 column) {
    i16 prev = s_statusColumn;
    s_statusColumn = column;
    return prev;
}

RVA(0x000409a0, 0x74)
i16 RunStatusListPicker(i16 close) {
    i16 result = -2;
    if (!close) {
        if (!s_statusListMenu) {
            s_statusListMenu = CreateStatusListMenu(NULL);
        }
        result = RunListMenu(s_statusListMenu);
        if (result == -1) {
            return -1;
        }
        if (result != -2) {
            result = g_selectedObjectId;
        }
    }
    s_statusListMenu = CloseListMenu(s_statusListMenu);
    return result;
}

RVA(0x00040a20, 0x57)
MenuBox* CreateStatusListMenu(MenuBox* parent) {
    MenuBox* menu;
    BuildStatusSlots();
    menu = CreateMenuBox(parent, 26, 2);
    MoveMenuBox(menu, 45, 86);
    SetMenuItems(menu, 16, g_statusSlots, g_statusSlotCount, StatusListMenuHandler);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, 1);
    return menu;
}

RVA(0x00040a80, 0xa8)
void StatusListMenuHandler(MenuBox* menu, i16 index, i16 event) {
    i16 slot = g_statusSlots[index];
    i16 result;
    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->items.entries = NULL;
            menu->itemCount = 0;
            break;
        case MENU_EVENT_ADD_ROW:
            result = FormatStatusLine(slot, index);
            if (result == -1) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2500, slot, 1);
            } else if (result >= 0) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2450, slot, 0);
            } else {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2500, slot, 1);
            }
            break;
    }
}

// "CNL" / "DNL": the letters for alignment classes -1/0/1 on the two axes.
DATA(0x00064910)
static const char s_alignmentLetters[2][3] = {{'C', 'N', 'L'}, {'D', 'N', 'L'}};

// Formats roster slot `slot` as status-list row `row` into the scratch
// buffer (a party mark, the name, alignment, level, HP, MP, condition, and in
// column 1 the summoning cost); returns the member's id, -1 when the row
// cannot be picked in this column, -2 when its cost cannot be paid.
RVA(0x00040b30, 0x394)
i16 FormatStatusLine(i16 slot, i16 row) {
    char text[36];
    Character* member;
    i32 cost;

    member = GetRosterCharacter(slot);
    if (member == NULL) {
        sprintf(g_scratchBuffer, " %2d", row + 1);
        return -1;
    }
    sprintf(
        g_scratchBuffer,
        "%c%2d %-17.17s ",
        FindPartySlot(slot) != -1 ? '*' : ' ',
        row + 1,
        FormatFullName(text, member)
    );
    strcat(g_scratchBuffer, FormatAlignmentLetter(1, member->alignmentLevelA, text));
    strcat(g_scratchBuffer, "/");
    strcat(g_scratchBuffer, FormatAlignmentLetter(0, member->alignmentLevelB, text));
    sprintf(text, " %2d ", member->level);
    strcat(g_scratchBuffer, text);
    strcat(g_scratchBuffer, FormatCurMax(member->pools.hp, text, 4));
    strcat(g_scratchBuffer, FormatCurMax(member->pools.mp, text, 3));
    strcat(g_scratchBuffer, " ");
    sprintf(text, "%-6.6s", GetFirstConditionName(GetCharacterConditions(member)));
    strcat(g_scratchBuffer, text);
    switch (s_statusColumn) {
        case 0:
            break;
        case 1:
            cost = GetSummonMagnetiteCost(member);
            sprintf(text, " %6ld", cost);
            strcat(g_scratchBuffer, text);
            if (FindPartySlot(slot) != -1) {
                return -1;
            }
            if (GetFatalCondition(GetCharacterConditions(member))) {
                return -1;
            }
            if (GetRosterCharacter(0)->magnetite < cost) {
                return -2;
            }
            break;
        case 2:
            if (FindPartySlot(slot) != -1) {
                return -1;
            }
            if (TestCharacterFlag(member, 0x40)) {
                return -1;
            }
            break;
        case 3:
            if (TestCharacterFlag(member, 0x40)) {
                return -1;
            }
            break;
        case 4:
            if (FindPartySlot(slot) != -1) {
                return -1;
            }
            break;
    }
    return member->id;
}

RVA(0x00040ed0, 0x2b)
char* FormatAlignmentLetter(i16 axis, i16 value, char* out) {
    i16 column = AlignmentClass(value) + 1;
    out[0] = s_alignmentLetters[axis][column];
    out[1] = '\0';
    return out;
}

RVA(0x00040f00, 0x34)
char* FormatCurMax(CurMax value, char* buf, i16 width) {
    sprintf(buf, " %*d/%*d", width, value.cur, width, value.max);
    return buf;
}

// Lists the roster slots the status screen shows: every slot in column 0,
// otherwise only filled slots holding ids from 32 up.
RVA(0x00040f40, 0x51)
i16 BuildStatusSlots(void) {
    i16 slot;
    Character* character;
    g_statusSlotCount = 0;
    for (slot = 0; slot < 32; slot++) {
        if (s_statusColumn != 0) {
            character = GetRosterCharacter(slot);
            if (character == NULL || IsHumanCharacter(character)) {
                continue;
            }
        }
        g_statusSlots[g_statusSlotCount++] = slot;
    }
    return g_statusSlotCount;
}
