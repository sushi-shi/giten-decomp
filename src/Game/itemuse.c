// @identity-TODO: the owning TU is unproven; this unit holds the field
// item-use flow's span until link-order evidence names it.

#include <rva.h>

#include <Game/CombatantId.h>

#include <Game/BagItems.h>
#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/Condition.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Game/ItemRecord.h>
#include <Game/ItemUse.h>
#include <Game/Party.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/Skill.h>
#include <Game/TargetFlags.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Input/Mouse.h>
#include <Math/Vec3.h>
#include <Mem/Alloc.h>
#include <Script/EventFlags.h>
#include <Text/TextWindow.h>
#include <Ui/FieldMenus.h>
#include <Ui/Menu.h>
#include <Ui/MenuBox.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>

// The item being used (-1 for none).
DATA(0x00068a54)
static i16 s_useItem = -1;

// The id of the member using it (-1 for none).
DATA(0x00068a58)
static i16 s_useMemberId = -1;

// The party position of the first member able to use it.
DATA(0x0007be54)
static i16 s_usePosition;

// The item list menu.
DATA(0x0007bea8)
static MenuBox* s_itemMenu;

RVA(0x0001a200, 0x40)
MenuBox* OpenItemListMenu(void) {
    ItemStackList* entries = CopyBagEntries(0, 64, NULL);
    MenuBox* menu = CreateMenuBox(NULL, 5, 2);
    menu->flags |= 0x1e;
    SetMenuItems(menu, 7, entries, GetItemListCount(entries), ItemListMenuHandler);
    return menu;
}

#define ItemUseInvokesSkill(kind) ((kind) == ITEM_KIND_WEAPON || (kind) == ITEM_KIND_ACCESSORY)

static __inline void AddItemUseMenuLine(MenuBox* menu, i16 item, i16 disabled) {
    AddMenuLine(menu->plane, g_scratchBuffer, disabled ? 0x2500 : 0x2470, item, disabled);
}

RVA(0x0001a240, 0x1bc)
void ItemListMenuHandler(MenuBox* menu, i16 index, i16 event) {
    ItemStackList* entries = menu->items.itemList;
    ItemRecord* record;
    switch (event) {
        case MENU_EVENT_ADD_ROW:
            sprintf(
                g_scratchBuffer,
                "%-20.20s%2d",
                GetLoadedRecordName(GetItemStackItem(GetItemListEntry(entries, index))),
                GetItemStackCount(GetItemListEntry(entries, index))
            );
            if ((GetItemStackItem(GetItemListEntry(entries, index)) == 0x21
                 && IsEventFlagSet(7, 0xff))
                || (GetItemStackItem(GetItemListEntry(entries, index)) == 0x24
                    && IsEventFlagSet(7, 0xfe))) {
                AddItemUseMenuLine(menu, GetItemStackItem(GetItemListEntry(entries, index)), 1);
                return;
            }
            record = GetLoadedRecord(GetItemStackItem(GetItemListEntry(entries, index)));
            event = GetItemUseModes(record);
            if (ItemUseInvokesSkill(record->kind)) {
                event = GetSkillUseModes(GetSkillView(GetItemSkillId(record)));
            }
            if (CheckSkillArea(GetItemSkillId(record)) != 1) {
                AddItemUseMenuLine(menu, GetItemStackItem(GetItemListEntry(entries, index)), 1);
                return;
            }
            if (IsSkillUsableNow(event) != 1) {
                AddItemUseMenuLine(menu, GetItemStackItem(GetItemListEntry(entries, index)), 1);
                return;
            }
            AddItemUseMenuLine(menu, GetItemStackItem(GetItemListEntry(entries, index)), 0);
            return;
        case MENU_EVENT_BEGIN_PAGE:
            AddMenuLine(menu->plane, "<\203A\203C\203e\203\200>", 0x2450, 0, 1);
            return;
        case MENU_EVENT_DESTROY:
            menu->items.itemList = FreeBlock(entries);
            menu->itemCount = 0;
            return;
    }
}

static __inline void SelectItemUserAsTarget(void) {
    g_targetId = PartyCombatantId(s_usePosition);
    NextGamePhase();
}

// Runs the field item-use flow one phase: open the item list, pick an item,
// pick its target (a skill-bearing item, kind 11 or 19, targets as its skill),
// then hand the user's pick to the action prompt. Returns 0.
// @early-stop instruction scheduling: in the final phase retail loads
// g_targetId before storing g_actorId and reads the pick flags early into bl;
// cl here keeps source order (statement reorders score lower), and the
// permuter found one compiler island.
RVA(0x0001a400, 0x3e0)
i16 RunItemUse(void) {
    ItemRecord* record;
    Character* user;
    i16 flags;
    i16 range;
    i16 kind;
    i16 picked;
    i16 position;

    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            NextGamePhase();
            s_itemMenu = OpenItemListMenu();
            HideScreenLayer(1);
            return 0;

        case 1:
            ReturnFromGameState();
            s_itemMenu = CloseListMenu(s_itemMenu);
            RestoreSwappedMember();
            s_useMemberId = -1;
            return 0;

        case 2:
            picked = RunListMenu(s_itemMenu);
            if (picked == -2) {
                PrevGamePhase();
            }
            if (picked < 0) {
                break;
            }
            s_useItem = g_selectedObjectId;
            DecodeItemRecord(&g_loadedItem, s_useItem);
            NextGamePhase();
            s_usePosition = FindFirstAbleMemberPosition();
            return 0;

        case 3:
            record = GetLoadedRecord(s_useItem);
            kind = record->kind;
            if (ItemUseInvokesSkill(kind)) {
                flags = GetSkillTargetFlags(GetItemSkillId(record));
                range = GetSkillAttackRange(GetItemSkillId(record));
            } else {
                flags = GetItemTargetFlags(record);
                range = GetItemAttackRange(record);
            }
            if (TargetFlagsSelectSelf(flags)) {
                SelectItemUserAsTarget();
                return 0;
            }
            if (TargetFlagsSelectActorGroup(flags)) {
                SelectItemUserAsTarget();
                return 0;
            }
            if (flags == 0x10) {
                picked = RunPickTargetWindow(0, range, 5, GetPartyRosterId(s_usePosition));
            } else if (flags == 0x11) {
                picked = RunPickTargetWindow(0, range, 4, GetPartyRosterId(s_usePosition));
            } else if (flags == 0x30) {
                picked = RunPickTargetWindow(0, range, 6, GetPartyRosterId(s_usePosition));
            } else {
                flags = 0;
                picked = RunPickTargetWindow(0, range, 3, GetPartyRosterId(s_usePosition));
            }
            if (picked == -1) {
                PrevGamePhase();
                return 0;
            }
            if (picked == 0) {
                break;
            }
            NextGamePhase();
            if (flags) {
                g_selectedObjectId = SwapInForPick(s_usePosition, g_selectedObjectId);
            }
            g_targetId = g_selectedObjectId;
            return 0;

        case 4:
            NextGamePhase();
            s_itemMenu = DestroyMenuBox(s_itemMenu);
            return 0;

        case 5:
            NextGamePhase();
            position = FindPartyPositionOfId(s_useMemberId);
            user = GetPartyCharacter(position);
            record = GetLoadedRecord(s_useItem);
            kind = record->kind;
            if (ItemUseInvokesSkill(kind)) {
                g_actorId = PartyCombatantId(position);
                user->pickRole = 4;
                user->pickObject = g_targetId;
                g_actionId = GetItemSkillId(record);
                user->pickTarget = GetItemSkillId(record);
                user->pickFlags |= PICK_ITEM_SKILL;
                user->pickItem = s_useItem;
            } else {
                g_actorId = PartyCombatantId(position);
                user->pickObject = g_targetId;
                user->pickRole = 5;
                g_actionId = s_useItem;
                user->pickTarget = s_useItem;
            }
            PushFieldUsePrompt();
            return 0;

        case 6:
            SetGamePhase(1);
            break;
    }
    return 0;
}

// The party position of the first of the sixteen member ids in the party
// whose conditions let it act; position of id 0 when none can.
RVA(0x0001a7e0, 0x50)
i16 FindFirstAbleMemberPosition(void) {
    i16 id;
    i16 slot;

    for (id = 0; id < 16; id++) {
        slot = RosterSlotOfId(id);
        if (slot >= 0
            && !GetPickBlockingCondition(GetCharacterConditions(GetRosterCharacter(slot)))) {
            return FindPartyPositionOfId(id);
        }
    }
    return FindPartyPositionOfId(0);
}

RVA(0x0001a830, 0x30)
i16 CancelItemTargetMenu(i16 command) {
    if (command == -1) {
        s_itemMenu = DestroyMenuBox(s_itemMenu);
    }
    return g_selectedObjectId;
}

RVA(0x0001a860, 0x10)
void SetUseMemberId(i16 id) {
    s_useMemberId = id;
}
