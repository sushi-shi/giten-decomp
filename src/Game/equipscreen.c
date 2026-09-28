// @identity-TODO: the owning TU is unproven; this unit holds the equipment
// page's span until link-order evidence names it.

#include <rva.h>

#include <Game/BagItems.h>
#include <Game/Character.h>
#include <Game/ClickWait.h>
#include <Game/EquipRequirements.h>
#include <Game/EquipScreen.h>
#include <Game/GameState.h>
#include <Game/GemItems.h>
#include <Game/ItemBag.h>
#include <Game/ItemBonus.h>
#include <Game/ItemRecord.h>
#include <Game/Party.h>
#include <Game/Skill.h>
#include <Game/StateStack.h>
#include <Game/StatusScreen.h>
#include <Game/WaitState.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Script/TextToken.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Ui/Panel.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/Menu.h>
#include <Ui/MenuBox.h>
#include <Util/Range.h>
#include <Util/Scratch.h>
#include <Util/WordList.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The item kind of ammunition, loaded into the gun's magazine.

DATA(0x000649a0)
const i16 g_equipCountSlots[9] = {0, 1, -1, 4, 3, 4, 5, 6, 7};

// The menu of bag items to equip.
DATA(0x0006a208)
static MenuBox* s_equipMenu = NULL;

// The equipment panel.
DATA(0x0006a20c)
static i16 s_panelPlane = -1;

// The picked item's name and description.
DATA(0x0006a20e)
static i16 s_infoPlane = -1;

// The picked bag entry or equipment part; -1 none, -2 cancelled.
DATA(0x0006a210)
static i16 s_pick = -1;

// Set when the equipment changed, so the status screen is redrawn on leaving.
DATA(0x0006a212)
static b16 s_changed = false;

// One object: retail reads `itemBase` and `item` with dword moves that run
// into the next field.
DATA(0x0006a218)
static AttachPage s_attach = {NULL, NULL, NULL, 0, -1, -1, 0, 0, 0, 0};

// The item page: its menu, its info window, the sub-state to resume (and then
// the item picked), and the copy of the bag it lists.
DATA(0x0006a238)
static EquipItemPage s_itemPage = {NULL, -1, -1, NULL};

// The skill page: its menu, its description window, and the sub-state to
// resume (and then the skill picked).
DATA(0x0006a248)
static EquipSkillPage s_skillPage = {NULL, -1, -1};

// The bag entries the attach page lists, and its two header lines.
// The bag entries the equipment menu lists, and its two header lines.
DATA(0x00083b50)
static i16 s_equipEntries[48];

DATA(0x00083c78)
static char s_equipHeaderA[4];

DATA(0x00083c7c)
static char s_equipHeaderB[4];

DATA(0x00083bb0)
static AttachEntry s_attachEntries[48];

DATA(0x00083c80)
static char s_attachHeaderA[4];

DATA(0x00083c84)
static char s_attachHeaderB[4];

// The label of an empty equipment part.
DATA(0x00083c88)
static char s_emptyPartLabel[4];

// The skill page's second header line and the label of an empty skill.
DATA(0x00083c8c)
static char s_skillHeaderLine[4];

DATA(0x00083c90)
static char s_emptySkillLabel[4];

DATA(0x00083c70)
i16 g_previousStatusStep;

DATA(0x00083c74)
char g_emptyBattleSkillLabel[4];

DATA(0x00083c94)
char g_emptyEquipPickLabel[4];

DATA(0x000649b8)
static const i16 s_equipPickCategories[8] = {
    EQUIP_PART_WEAPON,
    EQUIP_PART_GUN,
    EQUIP_PART_AMMO,
    EQUIP_PART_HEAD,
    EQUIP_PART_BODY,
    EQUIP_PART_ARMS,
    EQUIP_PART_LEGS,
    EQUIP_PART_ACCESSORY
};

DATA(0x0006a250)
static i16 s_equipPickPart = -1;

RVA(0x00042cd0, 0x182)
i16 ListEquipCandidates(i16 member, i16 anyEquipped) {
    Character* character = GetRosterCharacter(member);
    i16 count;
    i16 item;
    i16 kind;
    i16 i;

    if (character == NULL) {
        return 0;
    }
    CompactBag();
    count = 0;
    for (i = 0; i < 48; i++) {
        item = GetBagItem(i);
        if (CanEquipItem(character, item) < 0) {
            continue;
        }
        if (GetCharacterEquipment(character)[1].item >= 1
            && GetItemKind(GetCharacterEquipment(character)[1].item) == ITEM_KIND_FULL_BODY_ARMOR) {
            kind = GetItemKind(item);
            if (kind == ITEM_KIND_HEAD_ARMOR || kind == ITEM_KIND_ARM_ARMOR
                || kind == ITEM_KIND_LEG_ARMOR) {
                continue;
            }
        }
        s_equipEntries[count++] = i;
    }
    if (count == 0 && anyEquipped) {
        if (GetCharacterEquipment(character)[0].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[1].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[2].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[3].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[4].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[5].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[6].item >= 1) {
            return 1;
        }
        return GetCharacterEquipment(character)[7].item >= 1;
    }
    return count;
}

static __inline void ClearEquipPreview(void) {
    s_infoPlane = CloseTextWindow(s_infoPlane);
    DrawEquipPanel(GetRosterCharacter(g_statusMember), NULL);
    DrawStatTotals(3, 0x19, GetRosterCharacter(g_statusMember), NULL);
}

static __inline i16 FinishEquipChange(void) {
    RecalcCharacterStats(GetRosterCharacter(g_statusMember));
    s_changed = true;
    SetGameSub(1);
    s_pick = -1;
    return -1;
}

// Runs the equipment page one step for input `key` (-2 cancels): step 0
// opens it, 1 closes it, 2 waits for a bag item (menu) or an equipped part
// (panel), 3/4 preview and equip a bag item, 5/6 preview and remove an
// equipped one. A key other than -1/-2 restarts it at step 1 with that pick.
// @early-stop register allocation: from the second magazine clamp on, cl
// rotates the scratch registers one place against retail (ax/cx for dx/ax),
// and the equip path joins the removal tail one instruction early; the
// permuter found one compiler island.
RVA(0x00042e60, 0x6d0)
i16 RunEquipScreen(i16 key) {
    ItemSlot slot;
    i16 count;
    i16 part;
    ItemSlot loaded;

    if (key != -1 && key != -2) {
        SetGameSub(1);
        s_pick = -2;
        if (key != 8) {
            s_pick = key;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            SetStatusMenuItemFlag(8, PANEL_ROW_CHECKED, 1);
            s_equipMenu = OpenEquipMenu(g_statusMember, s_equipMenu);
            s_panelPlane = CreateTextPlane(0x12, 0);
            ResetTextPlaneLineStep(s_panelPlane, 3);
            DrawEquipPanel(GetRosterCharacter(g_statusMember), NULL);
            PollEquipPart(g_statusMember, EQUIP_PICK_RESET);
            return -1;

        case 1:
            s_infoPlane = CloseTextWindow(s_infoPlane);
            s_panelPlane = CloseTextWindow(s_panelPlane);
            s_equipMenu = DestroyMenuBox(s_equipMenu);
            if (s_changed) {
                DrawStatusScreen(g_statusMember);
                s_changed = false;
            }
            SetStatusMenuItemFlag(8, PANEL_ROW_CHECKED, 0);
            PollEquipPart(g_statusMember, EQUIP_PICK_CLEAR);
            if (s_pick != -1) {
                return s_pick;
            }
            PrevGameSub();
            return -1;

        case 2:
            if (key == -2) {
                PrevGameSub();
                s_pick = key;
                return -1;
            }
            if (RunListMenu(s_equipMenu) == -1) {
                part = PollEquipPart(g_statusMember, EQUIP_PICK_PART);
                if (part == -2) {
                    PrevGameSub();
                    s_pick = -2;
                    return -1;
                }
                if (part == -1) {
                    return -1;
                }
                s_pick = part;
                SetGameSub(5);
                return -1;
            }
            NextGameSub();
            s_pick = g_selectedObjectId;
            return -1;

        case 3:
            NextGameSub();
            PreviewEquipChange(s_pick, 0);
            s_infoPlane = OpenItemInfoPlane(GetBagItem(s_pick));
            return -1;

        case 4:
            if (key == -2) {
                SetGameSub(2);
                ClearEquipPreview();
                return -1;
            }
            if (TakeClickUnlessCancel(key) <= 0) {
                return -1;
            }
            ReadBagEntry(s_pick, &slot, &count);
            if (GetItemKind(slot.item) == ITEM_KIND_AMMO) {
                slot.quantity = GetGunMagazineSize(
                    GetLoadedRecord(GetRosterEquipSlot(g_statusMember, EQUIP_PART_GUN).item)
                );
                if (GetRosterEquipSlot(g_statusMember, EQUIP_PART_AMMO).item == slot.item) {
                    loaded = GetRosterEquipSlot(g_statusMember, EQUIP_PART_AMMO);
                    slot.quantity -= loaded.quantity;
                    LimitItemSlotToBag(&slot);
                    if (slot.quantity < 0) {
                        slot.quantity = 0;
                    }
                    GetCharacterEquipment(GetRosterCharacter(g_statusMember))[7].quantity +=
                        slot.quantity;
                    TakeBagItems(slot.item, slot.quantity);
                    return FinishEquipChange();
                }
                LimitItemSlotToBag(&slot);
            } else {
                slot.quantity = 1;
            }
            EquipItem(g_statusMember, slot, count, s_pick);
            return FinishEquipChange();

        case 5:
            NextGameSub();
            PreviewEquipChange(s_pick, 1);
            slot = GetRosterEquipSlot(g_statusMember, s_pick);
            s_infoPlane = OpenItemInfoPlane(slot.item);
            return -1;

        case 6:
            if (key == -2) {
                SetGameSub(2);
                ClearEquipPreview();
                PollEquipPart(g_statusMember, EQUIP_PICK_CLEAR);
                return -1;
            }
            if (TakeClickUnlessCancel(key) <= 0) {
                return -1;
            }
            if (s_pick != EQUIP_PART_AMMO) {
                slot = GetRosterEquipSlot(g_statusMember, s_pick);
                if (slot.quantity < 1) {
                    slot.quantity = 1;
                }
                StoreBagItem(slot.item, slot.quantity, slot.attachment);
                ClearItemSlot(&slot);
                SetEquipSlot(g_statusMember, s_pick, slot, 0);
                if (s_pick == EQUIP_PART_GUN) {
                    s_pick = 2;
                }
            }
            if (s_pick == EQUIP_PART_AMMO) {
                slot = GetRosterEquipSlot(g_statusMember, EQUIP_PART_AMMO);
                if (slot.item >= 1) {
                    StoreBagItem(slot.item, slot.quantity, slot.attachment);
                    ClearItemSlot(&slot);
                    SetEquipSlot(g_statusMember, s_pick, slot, 0);
                }
            }
            return FinishEquipChange();
    }
    return -1;
}

static void EquipMenuHandler(MenuBox* menu, i16 index, i16 event);

RVA(0x00043530, 0x6a)
MenuBox* OpenEquipMenu(i16 member, MenuBox* old) {
    i16 count = ListEquipCandidates(member, 0);
    MenuBox* menu = CreateMenuBox(old, 0x13, 2);

    SetMenuItems(menu, 10, s_equipEntries, count, EquipMenuHandler);
    MoveMenuBox(menu, 6, 0x18);
    ResetTextPlaneLineStep(menu->plane, 3);
    SetTextPlaneFlag8(menu->plane, 1);
    return menu;
}

RVA(0x000435a0, 0x278)
static void EquipMenuHandler(MenuBox* menu, i16 index, i16 event) {
    Character* member;
    ItemRecord* record;
    i16 item;
    i16 cursed;

    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            AddMenuLine(menu->plane, s_equipHeaderA, 0x400, 0, 1);
            AddMenuLine(menu->plane, s_equipHeaderB, 0x400, 0, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            item = GetBagItem(s_equipEntries[index]);
            sprintf(
                g_scratchBuffer,
                "%c %-20.20s %2d",
                GetBagEntryAttachment(s_equipEntries[index]) != -1 ? '*' : ' ',
                GetLoadedRecordName(item),
                GetBagEntryCount(s_equipEntries[index])
            );
            member = GetRosterCharacter(g_statusMember);
            if (member != NULL) {
                i16 category = GetItemCategory(item);
                if (IsEquipCurseActive(member, category)) {
                    AddMenuLine(menu->plane, g_scratchBuffer, 0x560, s_equipEntries[index], 1);
                    return;
                }
                record = GetLoadedRecord(item);
                if (record->kind == ITEM_KIND_FULL_BODY_ARMOR) {
                    cursed = IsEquipCurseActive(member, EQUIP_PART_HEAD);
                    cursed |= IsEquipCurseActive(member, EQUIP_PART_BODY);
                    cursed |= IsEquipCurseActive(member, EQUIP_PART_ARMS);
                    cursed |= IsEquipCurseActive(member, EQUIP_PART_LEGS);
                    if (cursed) {
                        AddMenuLine(menu->plane, g_scratchBuffer, 0x560, s_equipEntries[index], 1);
                        return;
                    }
                    record = GetLoadedRecord(item);
                }
                if (record->kind == ITEM_KIND_GUN && GetBattleStatShown(member, 6) > 0) {
                    if (LacksItemRequiredStats(member, record, GetBattleStatShown(member, 6))) {
                        AddMenuLine(menu->plane, g_scratchBuffer, 0x760, s_equipEntries[index], 1);
                        return;
                    }
                    AddMenuLine(menu->plane, g_scratchBuffer, 0x460, s_equipEntries[index], 0);
                    return;
                }
                if (LacksItemRequiredStats(member, record, 0)) {
                    AddMenuLine(menu->plane, g_scratchBuffer, 0x760, s_equipEntries[index], 1);
                    return;
                }
            }
            AddMenuLine(menu->plane, g_scratchBuffer, 0x460, s_equipEntries[index], 0);
            return;
    }
}

static i16 DrawStatColumn(i16 x, i16 y, i16* stats, i16* preview);

RVA(0x00043820, 0x10a)
void DrawEquipPanel(Character* member, Character* preview) {
    i16 x;
    i16 y;
    i16 i;

    y = 0x28;
    for (i = 0; i < 4; i++) {
        DrawPlaneText(s_panelPlane, 8, y, g_statusBattleLabels[i + 1], 0x400);
        y += 0x18;
    }
    if (preview == NULL) {
        x = DrawStatColumn(5, 5, GetBattleStatGroup(member, 0), NULL);
        x = DrawStatColumn(x, 5, GetBattleStatGroup(member, 1), NULL);
        DrawStatColumn(x, 5, GetBattleStatGroup(member, 2), NULL);
    } else {
        x = DrawStatColumn(5, 5, GetBattleStatGroup(member, 0), GetBattleStatGroup(preview, 0));
        x = DrawStatColumn(x, 5, GetBattleStatGroup(member, 1), GetBattleStatGroup(preview, 1));
        DrawStatColumn(x, 5, GetBattleStatGroup(member, 2), GetBattleStatGroup(preview, 2));
    }
    DrawPlaneImage(s_panelPlane, 7, 1, 0);
    DrawPlaneImage(s_panelPlane, 0x10, 1, 1);
    DrawPlaneImage(s_panelPlane, 0x19, 1, 8);
}

static u16 DrawStatCompare(i16 x, i16 y, i16 value, i16 newValue);

RVA(0x00043930, 0xe4)
static i16 DrawStatColumn(i16 x, i16 y, i16* stats, i16* preview) {
    if (preview == NULL) {
        DrawStatCompare(x, y, stats[2], -1);
        DrawStatCompare(x, y + 3, stats[3], -1);
        DrawStatCompare(x, y + 6, stats[4], -1);
        DrawStatCompare(x, y + 9, stats[5], -1);
    } else {
        DrawStatCompare(x, y, stats[2], preview[2]);
        DrawStatCompare(x, y + 3, stats[3], preview[3]);
        DrawStatCompare(x, y + 6, stats[4], preview[4]);
        DrawStatCompare(x, y + 9, stats[5], preview[5]);
    }
    return x + strlen(g_scratchBuffer);
}

RVA(0x00043a20, 0x93)
static u16 DrawStatCompare(i16 x, i16 y, i16 value, i16 newValue) {
    i32 attr = 0x400;

    if (newValue < 0) {
        sprintf(g_scratchBuffer, "  %3d    ", value);
    } else {
        sprintf(g_scratchBuffer, "  %3d>%3d", value, newValue);
        if (value < newValue) {
            attr = 0x600;
        } else if (value > newValue) {
            attr = 0x500;
        }
    }
    DrawPlaneText(s_panelPlane, x * 8, y * 8, g_scratchBuffer, attr);
    return attr;
}

static MenuBox* CreateAttachItemMenu(MenuBox* old);
static MenuBox* CreateAttachEntryMenu(MenuBox* old);
static void AttachTextHook(i16 plane, i16 event, i16 value);

RVA(0x00043ac0, 0x24f)
void PreviewEquipChange(i16 index, i16 fromEquipped) {
    Character* member = GetRosterCharacter(g_statusMember);
    Character* saved;
    ItemSlot slot;
    i16 count;
    i16 result;
    i16 kind;
    i16 gun;

    if (!member) {
        return;
    }
    saved = GetCharacter(14);
    memcpy(saved, member, offsetof(Character, alignmentA));
    if (!fromEquipped) {
        ReadBagEntry(index, &slot, &count);
        kind = GetItemKind(slot.item);
        if (kind == ITEM_KIND_AMMO) {
            slot.quantity =
                GetGunMagazineSize(GetLoadedRecord(GetCharacterEquipment(member)[6].item));
            LimitItemSlotToBag(&slot);
        } else if (kind == ITEM_KIND_FULL_BODY_ARMOR) {
            EmptyItemSlot(&GetCharacterEquipment(member)[0]);
            EmptyItemSlot(&GetCharacterEquipment(member)[2]);
            EmptyItemSlot(&GetCharacterEquipment(member)[3]);
        } else {
            // Kind 19 selects index 7, overwriting returnPosition.area in
            // the saved preview copy; retain this original store.
            saved->ammoCounts[g_equipCountSlots[kind - ITEM_KIND_WEAPON]] = count;
            slot.quantity = 1;
        }
        if (kind == ITEM_KIND_GUN) {
            gun = GetCharacterEquipment(member)[6].item;
            GetCharacterEquipment(member)[6].item = slot.item;
            if (CanEquipItem(member, GetCharacterEquipment(member)[7].item) < 1) {
                EmptyItemSlot(&GetCharacterEquipment(member)[7]);
            }
            GetCharacterEquipment(member)[6].item = gun;
        }
        SwapEquipSlot(g_statusMember, slot, &result);
    } else {
        ClearItemSlot(&slot);
        SetEquipSlot(g_statusMember, index, slot, 0);
        if (index == EQUIP_PART_GUN) {
            SetEquipSlot(g_statusMember, EQUIP_PART_AMMO, slot, 0);
        }
    }
    RecalcCharacterStats(member);
    DrawEquipPanel(saved, member);
    DrawStatTotals(3, 0x19, saved, member);
    memcpy(member, saved, offsetof(Character, alignmentA));
    InitWordList(GetCharacterSkills(saved), 0);
}

RVA(0x00043d10, 0x5e0)
i16 RunAttachScreen(i16 sub) {
    if (sub != -1 && sub != -2) {
        SetGameSub(1);
        s_attach.resume = -2;
        if (sub != 9) {
            s_attach.resume = sub;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            s_attach.itemBase = GetGemItemBase();
            SetStatusMenuItemFlag(9, PANEL_ROW_CHECKED, 1);
            s_attach.itemMenu = CreateAttachItemMenu(s_attach.itemMenu);
            s_attach.plane = CreateTextPlane(0x12, 0);
            ResetTextPlaneLineStep(s_attach.plane, 3);
            s_attach.prevHook = SetTextPlaneHook(AttachTextHook);
            return -1;
        case 1:
            SetTextPlaneHook(s_attach.prevHook);
            s_attach.prevHook = NULL;
            s_attach.plane = CloseTextWindow(s_attach.plane);
            s_attach.entryMenu = DestroyMenuBox(s_attach.entryMenu);
            s_attach.itemMenu = DestroyMenuBox(s_attach.itemMenu);
            PollEquipPart(g_statusMember, EQUIP_PICK_CLEAR);
            if (s_attach.redraw) {
                DrawStatusScreen(g_statusMember);
                s_attach.redraw = 0;
            }
            SetStatusMenuItemFlag(9, PANEL_ROW_CHECKED, 0);
            if (s_attach.resume == -1) {
                PrevGameSub();
                return -1;
            }
            return s_attach.resume;
        case 2:
            if (sub == -2) {
                PrevGameSub();
                s_attach.resume = -2;
                return -1;
            }
            if (RunListMenu(s_attach.itemMenu) == -1 || g_selectedObjectId < 0) {
                break;
            }
            NextGameSub();
            SetTextPlaneHook(s_attach.prevHook);
            s_attach.prevHook = NULL;
            s_attach.plane = CloseTextWindow(s_attach.plane);
            s_attach.itemMenu = DestroyMenuBox(s_attach.itemMenu);
            s_attach.item = g_selectedObjectId;
            return -1;
        case 3:
            NextGameSub();
            s_attach.entryMenu = CreateAttachEntryMenu(s_attach.entryMenu);
            PollEquipPart(g_statusMember, EQUIP_PICK_RESET);
            return -1;
        case 4:
            if (sub == -2) {
                SetGameSub(1);
                s_attach.resume = -2;
                return -1;
            }
            if (RunListMenu(s_attach.entryMenu) == -1) {
                sub = PollEquipPart(g_statusMember, EQUIP_PICK_ATTACH_TARGET);
                if (sub == -2) {
                    SetGameSub(1);
                    s_attach.resume = -2;
                    return -1;
                }
                if (sub == -1) {
                    break;
                }
                s_attach.target = sub;
                s_attach.entryMenu = DestroyMenuBox(s_attach.entryMenu);
                SetGameSub(7);
                return -1;
            }
            NextGameSub();
            s_attach.target = g_selectedObjectId;
            s_attach.entryMenu = DestroyMenuBox(s_attach.entryMenu);
            return -1;
        case 5:
            NextGameSub();
            s_attach.plane = CreateTextPlane(0x12, 0);
            sprintf(g_scratchBuffer, "%s", GetLoadedRecordName(s_attach.item));
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 1);
            TakeBagItems(s_attach.item, 1);
            s_attach.item = AttachBagEntryItem(s_attach.target, s_attach.item);
            if (s_attach.item < 0) {
                // "をはめ込んだ" (fitted in)
                sprintf(g_scratchBuffer, "\202\360\202\315\202\337\215\236\202\361\202\276");
            } else {
                StoreBagItem(s_attach.item, 1, -1);
                // "と%sを付け替えた" (swapped for %s)
                sprintf(
                    g_scratchBuffer,
                    "\202\306%s\202\360\225\164\202\257\221\326\202\246\202\275",
                    GetLoadedRecordName(s_attach.item)
                );
            }
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 1);
            RepaintTextPlane(s_attach.plane, -2);
            PushWaitState(WAIT_INPUT, 0xffff, 0xffff, 0);
            return -1;
        case 6:
            SetGameSub(1);
            s_attach.plane = CloseTextWindow(s_attach.plane);
            s_attach.resume = -1;
            return -1;
        case 7:
            NextGameSub();
            s_attach.plane = CreateTextPlane(0x12, 0);
            sprintf(g_scratchBuffer, "%s", GetLoadedRecordName(s_attach.item));
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 1);
            TakeBagItems(s_attach.item, 1);
            s_attach.item =
                AttachEquipItem(g_statusMember, s_attach.target, s_attach.item - s_attach.itemBase);
            if (s_attach.item < 0) {
                // "をはめ込んだ" (fitted in)
                sprintf(g_scratchBuffer, "\202\360\202\315\202\337\215\236\202\361\202\276");
            } else {
                s_attach.item += s_attach.itemBase;
                StoreBagItem(s_attach.item, 1, -1);
                // "と%sを付け替えた" (swapped for %s)
                sprintf(
                    g_scratchBuffer,
                    "\202\306%s\202\360\225\164\202\257\221\326\202\246\202\275",
                    GetLoadedRecordName(s_attach.item)
                );
            }
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 1);
            RepaintTextPlane(s_attach.plane, -2);
            PushWaitState(WAIT_INPUT, 0xffff, 0xffff, 0);
            return -1;
        case 8:
            SetGameSub(1);
            s_attach.plane = CloseTextWindow(s_attach.plane);
            RecalcCharacterStats(GetRosterCharacter(g_statusMember));
            s_attach.redraw = 1;
            s_attach.resume = -1;
            return -1;
    }
    return -1;
}

static void AttachItemMenuHandler(MenuBox* menu, i16 index, i16 event);

RVA(0x000442f0, 0x49)
static MenuBox* CreateAttachItemMenu(MenuBox* old) {
    MenuBox* menu = CreateMenuBox(old, 0x15, 2);

    SetMenuItems(menu, 16, NULL, 16, AttachItemMenuHandler);
    MoveMenuBox(menu, 0x2a, 0x50);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, 0);
    return menu;
}

RVA(0x00044340, 0x86)
static void AttachItemMenuHandler(MenuBox* menu, i16 index, i16 event) {
    i16 count;

    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_ADD_ROW:
            count = CountGemItemsAt(index);
            sprintf(
                g_scratchBuffer,
                "%-14.14s %2d",
                GetLoadedRecordName(index + s_attach.itemBase),
                count
            );
            if (count == 0) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x560, index + s_attach.itemBase, 2);
            } else {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x460, index + s_attach.itemBase, 0);
            }
            break;
    }
}

static i16 ListAttachEntries(void);
static void AttachEntryMenuHandler(MenuBox* menu, i16 index, i16 event);

RVA(0x000443d0, 0x69)
static MenuBox* CreateAttachEntryMenu(MenuBox* old) {
    MenuBox* menu;

    s_attach.entryCount = ListAttachEntries();
    menu = CreateMenuBox(old, 0x13, 2);
    SetMenuItems(menu, 10, s_attachEntries, s_attach.entryCount, AttachEntryMenuHandler);
    MoveMenuBox(menu, 6, 0x18);
    ResetTextPlaneLineStep(menu->plane, 3);
    SetTextPlaneFlag8(menu->plane, 1);
    return menu;
}

RVA(0x00044440, 0x66)
static i16 ListAttachEntries(void) {
    i16 count;
    i16 item;
    i16 i;

    CompactBag();
    count = 0;
    for (i = 0; i < 48; i++) {
        item = GetBagItem(i);
        if (item >= 0 && GetItemStackLimit(item) == 1 && GetItemKind(item) != ITEM_KIND_GUN) {
            s_attachEntries[count].entry = i;
            s_attachEntries[count].count = GetBagEntryCount(i);
            count++;
        }
    }
    return count;
}

RVA(0x000444b0, 0xda)
static void AttachEntryMenuHandler(MenuBox* menu, i16 index, i16 event) {
    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            AddMenuLine(menu->plane, s_attachHeaderA, 0x400, 0, 1);
            AddMenuLine(menu->plane, s_attachHeaderB, 0x400, 0, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            sprintf(
                g_scratchBuffer,
                "%c %-20.20s %2d",
                GetBagEntryAttachment(s_attachEntries[index].entry) != -1 ? '*' : ' ',
                GetLoadedRecordName(GetBagItem(s_attachEntries[index].entry)),
                s_attachEntries[index].count
            );
            AddMenuLine(menu->plane, g_scratchBuffer, 0x460, s_attachEntries[index].entry, 0);
            break;
    }
}

RVA(0x00044590, 0xb8)
static void AttachTextHook(i16 plane, i16 event, i16 value) {
    if (plane == -1) {
        return;
    }
    switch (event) {
        case -1:
        case 1:
        case 2:
            return;
        case 3:
            ClearTextPlane(s_attach.plane);
            break;
        case 4:
            strcpy(g_scratchBuffer, GetItemDescription(value + s_attach.itemBase));
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 0);
            break;
    }
    RepaintTextPlane(s_attach.plane, -2);
}

static void ItemListHandler(MenuBox* menu, i16 index, i16 event);

RVA(0x00044650, 0x1e0)
i16 RunItemPage(i16 sub) {
    if (sub != -1 && sub != -2) {
        SetGameSub(1);
        s_itemPage.pick = -2;
        if (sub != 3) {
            s_itemPage.pick = sub;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            CompactBag();
            SetStatusMenuItemFlag(3, PANEL_ROW_CHECKED, 1);
            s_itemPage.menu = CreateMenuBox(s_itemPage.menu, 0x19, 2);
            MoveMenuBox(s_itemPage.menu, -8, -0x16);
            s_itemPage.list = CopyBagEntries(0, 64, NULL);
            SetMenuItems(
                s_itemPage.menu,
                8,
                s_itemPage.list,
                GetItemListCount(s_itemPage.list),
                ItemListHandler
            );
            SetTextPlaneFirstSelectableRow(s_itemPage.menu->plane, 1, 1);
            return -1;
        case 1:
            s_itemPage.plane = CloseTextWindow(s_itemPage.plane);
            s_itemPage.menu = CloseListMenu(s_itemPage.menu);
            SetStatusMenuItemFlag(3, PANEL_ROW_CHECKED, 0);
            return s_itemPage.pick;
        case 2:
            if (sub == -2) {
                PrevGameSub();
                s_itemPage.pick = sub;
                return -1;
            }
            if (RunListMenu(s_itemPage.menu) == -1) {
                break;
            }
            NextGameSub();
            s_itemPage.pick = g_selectedObjectId;
            return -1;
        case 3:
            if (sub == -2) {
                PrevGameSub();
                return -1;
            }
            NextGameSub();
            s_itemPage.plane = OpenItemInfoPlane(s_itemPage.pick);
            return -1;
        case 4:
            if (sub != -2 && !TakeMouseLeftClick()) {
                break;
            }
            s_itemPage.plane = CloseTextWindow(s_itemPage.plane);
            SetGameSub(2);
            break;
    }
    return -1;
}

RVA(0x00044830, 0x109)
static void ItemListHandler(MenuBox* menu, i16 index, i16 event) {
    i16 item;

    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->items.table = NULL;
            menu->itemCount = 0;
            s_itemPage.list = FreeBlock(s_itemPage.list);
            break;
        case MENU_EVENT_BEGIN_PAGE:
            // "所持アイテム %1d/8" (items held, page %d of 8)
            sprintf(
                g_scratchBuffer,
                "\217\212\216\235\203\101\203\103\203\145\203\200 %1d/8",
                menu->cursor / 8 + 1
            );
            AddMenuLine(menu->plane, g_scratchBuffer, 0x400, -1, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            item = GetItemStackItem(GetItemListEntry(s_itemPage.list, index));
            sprintf(
                g_scratchBuffer,
                "%c %-30.30s%2d",
                HasItemStackAttachment(GetItemListEntry(s_itemPage.list, index)) ? '*' : ' ',
                GetLoadedRecordName(item),
                GetItemStackCount(GetItemListEntry(s_itemPage.list, index))
            );
            AddMenuLine(menu->plane, g_scratchBuffer, 0x460, item, 0);
            break;
    }
}

static void
DrawEquipLine(i16 part, i16 item, i16 attach, i16 x, i16 y, Character* character, i16 gunItem);

RVA(0x00044940, 0x18a)
void DrawEquipLines(Character* character, i16 x, i16 y) {
    DrawEquipLine(
        0,
        GetCharacterEquipment(character)[5].item,
        GetCharacterEquipment(character)[5].attachment,
        x,
        y,
        character,
        0
    );
    DrawEquipLine(
        1,
        GetCharacterEquipment(character)[6].item,
        GetCharacterEquipment(character)[6].attachment,
        x,
        y + 4,
        character,
        0
    );
    DrawEquipLine(
        2,
        GetCharacterEquipment(character)[7].item,
        GetCharacterEquipment(character)[7].attachment,
        x,
        y + 8,
        character,
        0
    );
    DrawEquipLine(
        3,
        GetCharacterEquipment(character)[0].item,
        GetCharacterEquipment(character)[0].attachment,
        x,
        y + 12,
        character,
        GetCharacterEquipment(character)[1].item
    );
    DrawEquipLine(
        4,
        GetCharacterEquipment(character)[1].item,
        GetCharacterEquipment(character)[1].attachment,
        x,
        y + 16,
        character,
        0
    );
    DrawEquipLine(
        5,
        GetCharacterEquipment(character)[2].item,
        GetCharacterEquipment(character)[2].attachment,
        x,
        y + 20,
        character,
        GetCharacterEquipment(character)[1].item
    );
    DrawEquipLine(
        6,
        GetCharacterEquipment(character)[3].item,
        GetCharacterEquipment(character)[3].attachment,
        x,
        y + 24,
        character,
        GetCharacterEquipment(character)[1].item
    );
    DrawEquipLine(
        7,
        GetCharacterEquipment(character)[4].item,
        GetCharacterEquipment(character)[4].attachment,
        x,
        y + 28,
        character,
        0
    );
}

RVA(0x00044ad0, 0xd4)
static void
DrawEquipLine(i16 part, i16 item, i16 attach, i16 x, i16 y, Character* character, i16 gunItem) {
    char mark;

    if (attach >= 0 && attach <= 15) {
        mark = '*';
    } else {
        mark = ' ';
    }
    DrawStatusImage(x, y, part);
    if (item >= 1) {
        sprintf(g_scratchBuffer, "%c%-20.20s", mark, GetLoadedRecordName(item));
    } else if (gunItem >= 1 && GetItemKind(gunItem) == ITEM_KIND_FULL_BODY_ARMOR) {
        sprintf(g_scratchBuffer, "%c%-20.20s", ' ', "--------------------");
    } else {
        sprintf(g_scratchBuffer, "%c%-20.20s", ' ', s_emptyPartLabel);
    }
    if (item >= 1 && IsEquipCurseActive(character, GetItemCategory(item))) {
        DrawStatusLine(x + 3, y + 1, g_scratchBuffer, 0x1500);
    } else {
        DrawStatusLine(x + 3, y + 1, g_scratchBuffer, 0x1400);
    }
}

static MenuBox* CreateSkillMenu(i16 member, MenuBox* old);

RVA(0x00044bb0, 0x7c)
i16 OpenItemInfoPlane(i16 item) {
    i16 plane = CreateTextPlane(0x20, 0);
    ItemRecord* record;

    ClearTextPlane(plane);
    record = GetLoadedRecord(item);
    PrintWindowText(plane, GetItemRecordName(record), 0x400, 0, 1);
    PrintWindowText(plane, "\n", 0x400, 0, 1);
    PrintWindowText(plane, record->description, 0x400, 0, 1);
    RepaintTextPlane(plane, -2);
    return plane;
}

RVA(0x00044c30, 0x1c0)
i16 RunSkillPage(i16 sub) {
    if (sub != -1 && sub != -2) {
        SetGameSub(1);
        s_skillPage.pick = -2;
        if (sub != 4) {
            s_skillPage.pick = sub;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            SetStatusMenuItemFlag(4, PANEL_ROW_CHECKED, 1);
            s_skillPage.menu = CreateSkillMenu(g_statusMember, s_skillPage.menu);
            return -1;
        case 1:
            s_skillPage.plane = CloseTextWindow(s_skillPage.plane);
            s_skillPage.menu = DestroyMenuBox(s_skillPage.menu);
            SetStatusMenuItemFlag(4, PANEL_ROW_CHECKED, 0);
            return s_skillPage.pick;
        case 2:
            if (sub == -2) {
                PrevGameSub();
                s_skillPage.pick = sub;
                return -1;
            }
            if (RunListMenu(s_skillPage.menu) == -1) {
                break;
            }
            NextGameSub();
            s_skillPage.pick = g_selectedObjectId;
            return -1;
        case 3:
            NextGameSub();
            s_skillPage.plane = CreateTextPlane(0x20, 0);
            ClearTextPlane(s_skillPage.plane);
            PrintWindowText(
                s_skillPage.plane,
                FilterTextMarks(GetSkillDescription(s_skillPage.pick), 1),
                0x400,
                0,
                1
            );
            RepaintTextPlane(s_skillPage.plane, -2);
            return -1;
        case 4:
            if (!TakeClickUnlessCancel(sub)) {
                break;
            }
            s_skillPage.plane = CloseTextWindow(s_skillPage.plane);
            SetGameSub(2);
            break;
    }
    return -1;
}

static void SkillListHandler(MenuBox* menu, i16 index, i16 event);

RVA(0x00044df0, 0x55)
static MenuBox* CreateSkillMenu(i16 member, MenuBox* old) {
    Character* character = GetRosterCharacter(member);
    MenuBox* menu = CreateMenuBox(old, 9, 2);

    SetMenuItems(
        menu,
        8,
        GetWordArray(GetCharacterSkills(character)),
        GetWordCount(GetCharacterSkills(character)),
        SkillListHandler
    );
    MoveMenuBox(menu, 7, 0xc);
    return menu;
}

RVA(0x00044e50, 0x156)
static void SkillListHandler(MenuBox* menu, i16 index, i16 event) {
    i16* skills = menu->items.entries;
    SkillView* view;
    char* unit;
    i16 skill;

    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            // "%-16.16s  MP  効果" (effect), "魔法名称" (magic name)
            sprintf(
                g_scratchBuffer,
                "%-16.16s  MP  \214\370\211\312",
                "\226\202\226\100\226\274\217\314"
            );
            AddMenuLine(menu->plane, g_scratchBuffer, 0x400, -1, 1);
            AddMenuLine(menu->plane, s_skillHeaderLine, 0x400, -1, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            skill = skills[index];
            view = GetSkillView(skill);
            if (skill < 1) {
                AddMenuLine(menu->plane, s_emptySkillLabel, 0x460, skill, 1);
                return;
            }
            if (SkillCostsFullPool(&view->parameters)) {
                unit = GetSkillParameterCost(&view->parameters) < 0 ? "hp" : "mp";
                sprintf(
                    g_scratchBuffer,
                    "%-16.16sMAX%s %-26.26s",
                    view->name,
                    unit,
                    FilterTextMarks(view->description, 0)
                );
            } else {
                unit = GetSkillParameterCost(&view->parameters) < 0 ? "hp" : "mp";
                sprintf(
                    g_scratchBuffer,
                    "%-16.16s%3d%s %-26.26s",
                    view->name,
                    abs(GetSkillParameterCost(&view->parameters)),
                    unit,
                    FilterTextMarks(view->description, 0)
                );
            }
            AddMenuLine(menu->plane, g_scratchBuffer, 0x460, skills[index], 0);
            break;
    }
}

static __inline void UnhighlightEquipPart(i16 member) {
    if (s_equipPickPart >= 0) {
        DrawEquipPickRow(member, s_equipPickPart, 0x1400);
    }
}

// @early-stop tail merge: retail keeps the negative-mode reset and return
// at entry; this build shares the final reset. Assignment-return and
// returned-state forms retain the merge, as does the unhighlight macro.
RVA(0x00044fb0, 0x17f)
i16 PollEquipPart(i16 member, i16 mode) {
    i16 x;
    i16 y;
    i16 part;

    if (mode < EQUIP_PICK_PART) {
        s_equipPickPart = -1;
        return -1;
    }
    if (mode < EQUIP_PICK_CLEAR) {
        if (s_equipPickPart >= 0 && g_mouseLeftClick) {
            return s_equipPickPart;
        }
        x = g_mousePosition.x / 8 - 0x37;
        if (x >= 0 && x < 0x15) {
            y = (g_mousePosition.y - 40) / 8 - 3;
            if (y >= 0 && y % 4 != 2 && y % 4 != 3) {
                part = y / 4;
                if (part < 8) {
                    if (s_equipPickPart == part) {
                        return -1;
                    }
                    if (mode != EQUIP_PICK_ATTACH_TARGET
                        || (part != EQUIP_PART_GUN && part != EQUIP_PART_AMMO
                            && part != EQUIP_PART_ACCESSORY)) {
                        if (!IsEquipCurseActive(
                                GetRosterCharacter(member),
                                s_equipPickCategories[part]
                            )) {
                            UnhighlightEquipPart(member);
                            s_equipPickPart = part;
                            if (DrawEquipPickRow(member, part, 0x1600) < 1) {
                                s_equipPickPart = -1;
                            }
                            return -1;
                        }
                    }
                }
            }
        }
    }
    UnhighlightEquipPart(member);
    s_equipPickPart = -1;
    return -1;
}

RVA(0x00045130, 0x9a)
i16 DrawEquipPickRow(i16 member, i16 part, i32 attr) {
    ItemSlot slot = GetRosterEquipSlot(member, part);
    char mark;

    if (slot.attachment == -1) {
        mark = ' ';
    } else {
        mark = '*';
    }

    if (slot.item >= 1) {
        sprintf(g_scratchBuffer, "%c%s", mark, GetLoadedRecordName(slot.item));
    } else {
        sprintf(g_scratchBuffer, "%c%s", 0, g_emptyEquipPickLabel);
    }
    DrawStatusLine(0x37, part * 4 + 3, g_scratchBuffer, attr);
    return slot.item >= 1 ? 1 : -1;
}
