// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/PartyAction.h>
#include <Game/SkillUse.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Ui/FieldMenus.h>
#include <Ui/Panel.h>
#include <Util/Scratch.h>

#include <stddef.h>

DATA(0x0007be6c)
static MenuBox* s_partyPicker;

DATA(0x0007be5c)
static i16 s_partyPickerMode;

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
    s_partyPickerMode = 0;
    return DestroyMenuBox(menu);
}

RVA(0x00019ea0, 0x9e)
i16 RunPartyPicker(i16 command) {
    PartyMemberList* entries;
    i16 result;
    if (command != 0) {
        s_partyPicker = ClosePickerMenu(s_partyPicker);
    }
    if (command >= 0) {
        if (!s_partyPicker) {
            if (!CountPickablePartyMembers()) {
                return -2;
            }
            entries = ListPickableMembers(NULL, 6, 1);
            if (!entries->count) {
                FreeBlock(entries);
                return -2;
            }
            s_partyPicker = OpenPartyPicker(entries);
        }
        result = RunPickerMenu(s_partyPicker);
        if (result != -1 && result != -2) {
            s_partyPicker = ClosePickerMenu(s_partyPicker);
            return g_selectedObjectId;
        }
    }
    return -1;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00019f40, 0xc)
void SetPartyPickerMode(i16 mode) {
    s_partyPickerMode = mode;
}

RVA(0x00019f50, 0x52)
MenuBox* OpenPartyPicker(PartyMemberList* entries) {
    MenuBox* menu = CreateMenuBox(NULL, 5, 2);
    SetMenuItems(menu, 7, entries, entries->count, PartyPickerHandler);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, 1);
    if (!s_partyPickerMode) {
        menu->list->flags |= 2;
    }
    menu->flags |= 0x1e;
    return menu;
}

RVA(0x00019fb0, 0xd4)
void PartyPickerHandler(MenuBox* menu, i16 index, i16 event) {
    PartyMemberList* entries = menu->items.memberList;
    Character* character;
    i16 enabled;
    switch (event) {
        case MENU_EVENT_ADD_ROW:
            enabled = 1;
            character = GetCharacterById(entries->ids[index]);
            switch (s_partyPickerMode) {
                case 0:
                    break;
                case 1:
                    enabled = CountUsableMemberSkills(character, 1);
                    break;
                case 2:
                    if (!IsHumanCharacter(character)) {
                        enabled = 0;
                    }
                    break;
            }
            FormatFullName(g_scratchBuffer, character);
            if (enabled) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2460, entries->ids[index], 0);
            } else {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2500, entries->ids[index], 1);
            }
            break;
        case MENU_EVENT_DESTROY:
            menu->items.memberList = FreeBlock(entries);
            menu->itemCount = 0;
            break;
    }
}
