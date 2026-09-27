// @identity-TODO: the owning TU is unproven; this unit holds the DDS menu
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/DdsMenu.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSupport.h>
#include <Game/MenuCursor.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/PartyPick.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Game/StatusScreen.h>
#include <Gfx/ScreenLayer.h>
#include <Input/Mouse.h>
#include <Sound/Sound.h>
#include <Text/TextWindow.h>
#include <Ui/FieldMenus.h>
#include <Ui/Message.h>
#include <Ui/PartySlotSelection.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>

DATA(0x00068a80)
static char* s_ddsCommands[3] = {"CALL", "RETURN", "PURGE"};

DATA(0x0007beb0)
static MenuBox* s_ddsMenu;

DATA(0x0007bb38)
static MenuCursor s_summonCursor;

DATA(0x0007be4c)
static i16 s_ddsRosterSlot;

DATA(0x0007be50)
static i16 s_ddsPartySlot;

RVA(0x00017330, 0x178)
i16 RunDdsMenu(void) {
    i16 result;
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            NextGamePhase();
            RunPartyPicker(-1);
            s_ddsMenu = CreateMenuBox(s_ddsMenu, 25, 2);
            MoveMenuBox(s_ddsMenu, -8, -22);
            SetMenuItems(s_ddsMenu, 9, s_ddsCommands, 3, DdsMenuHandler);
            HideScreenLayer(1);
            break;
        case 1:
            ReturnFromGameState();
            s_ddsMenu = DestroyMenuBox(s_ddsMenu);
            SetFieldPanelRowChecked(4, 0);
            RequestFieldRefresh();
            break;
        case 2:
            result = RunMenu(s_ddsMenu);
            if (result == -1) {
                PrevGamePhase();
            }
            if (result > 0) {
                SetGamePhase(g_selectedObjectId + 3);
                s_ddsMenu = DestroyMenuBox(s_ddsMenu);
            }
            break;
        case 3:
            RunDdsSummon();
            break;
        case 4:
            s_ddsRosterSlot = ReturnDdsMember();
            if (s_ddsRosterSlot) {
                SetGamePhase(1);
            }
            break;
        case 5:
            if (PickDdsPurgeMember() != -1) {
                SetGamePhase(1);
                if (s_ddsRosterSlot >= 0) {
                    RemoveFromRoster(s_ddsRosterSlot);
                    PlaySoundEffect(0x36);
                }
            }
            break;
    }
    return 0;
}

RVA(0x000174b0, 0x70)
i16 RunDdsSummon(void) {
    i16 result;
    switch (GetGameStep()) {
        case 2:
            SetGamePhase(1);
            break;
        case 1:
            result = PickDdsSummon();
            if (result) {
                NextGameStep();
                if (result > 0) {
                    AddTrainingPoints(GetCharacters(), 3, 8);
                }
            }
            break;
        case 0:
            CloseMessageWindow();
            SetCursorLevel0(&s_summonCursor, 0);
            NextGameStep();
            break;
    }
    return 0;
}

RVA(0x00017520, 0x1cc)
i16 PickDdsSummon(void) {
    i16 step;
    i16 previous;
    Character* character;
    switch (GetCursorLevel0(&s_summonCursor)) {
        case 0:
            step = PickDdsRosterMember(GetCursorLevel1(&s_summonCursor));
            SetCursorLevel1(&s_summonCursor, step);
            if (step < 0) {
                if (s_ddsRosterSlot < 0) {
                    return -1;
                }
                NextCursorLevel0(&s_summonCursor);
            }
            break;
        case 1:
            if (!PollPartySlotSelection(1)) {
                break;
            }
            ClearPartySlotSelection();
            if (g_selectedObjectId < 0) {
                PrevCursorLevel0(&s_summonCursor);
                return 0;
            }
            character = GetPartyCharacter(g_selectedObjectId);
            if (character != NULL && IsHumanCharacter(character)) {
                return -1;
            }
            s_ddsPartySlot = g_selectedObjectId;
            NextCursorLevel0(&s_summonCursor);
            break;
        case 2:
            NextCursorLevel0(&s_summonCursor);
            break;
        case 3:
            NextCursorLevel0(&s_summonCursor);
            previous = ExchangePartySlot(s_ddsPartySlot, s_ddsRosterSlot);
            character = GetRosterCharacter(s_ddsRosterSlot);
            if (character != NULL) {
                ClearActionWait(GetCharacterActionWait(character));
                AddMagnetite(GetRosterCharacter(0), -GetSummonMagnetiteCost(character));
                ResetBattleTally(character);
            }
            character = GetRosterCharacter(previous);
            if (character != NULL) {
                ClearBattleConditions(GetCharacterConditions(character));
                ResetBattleTally(character);
            }
            MarkPickDone();
            PlaySoundEffect(0x20);
            break;
        case 4:
            return 1;
    }
    return 0;
}

RVA(0x000176f0, 0x4a)
i16 PickDdsRosterMember(i16 step) {
    switch (step) {
        case 2:
            RunStatusListPicker(1);
            return -1;
        case 0:
            SetStatusColumn(1);
            step++;
        case 1:
            s_ddsRosterSlot = RunStatusListPicker(0);
            if (s_ddsRosterSlot != -1) {
                step++;
            }
            break;
    }
    return step;
}

RVA(0x00017740, 0x126)
void DdsMenuHandler(MenuBox* menu, i16 index, i16 event) {
    char** items;
    i16 slot;
    i16 disabled;
    i32 attribute;
    Character* character;
    GetGamePhase();
    items = menu->items.text;
    switch (event) {
        case MENU_EVENT_ADD_ROW:
            attribute = 0x500;
            disabled = 1;
            switch (index) {
                case 2:
                    if (CountRosterEntries(0)) {
                        attribute = 0x2450;
                        disabled = 0;
                    }
                    break;
                case 1:
                    for (slot = 0; slot < 6; slot++) {
                        character = GetPartyCharacter(slot);
                        if (character != NULL && !IsHumanCharacter(character)) {
                            attribute = 0x2450;
                            disabled = 0;
                        }
                    }
                    break;
                case 0:
                    for (slot = 0; slot < 32; slot++) {
                        character = GetRosterCharacter(slot);
                        if (character != NULL && !IsHumanCharacter(character)
                            && !GetFatalCondition(GetCharacterConditions(character))) {
                            attribute = 0x2450;
                            disabled = 0;
                        }
                    }
                    break;
            }
            AddMenuLine(menu->plane, items[index], attribute, index, disabled);
            break;
        case MENU_EVENT_BEGIN_PAGE:
            sprintf(g_scratchBuffer, "<DDS>");
            AddMenuLine(menu->plane, g_scratchBuffer, 0x400, -1, 1);
            break;
        case MENU_EVENT_DESTROY:
            menu->items.text = NULL;
            menu->itemCount = 0;
            break;
    }
}

RVA(0x00017870, 0x8a)
i16 ReturnDdsMember(void) {
    Character* character;
    if (!PollPartySlotSelection(0)) {
        return 0;
    }
    ClearPartySlotSelection();
    if (g_selectedObjectId < 0) {
        return -1;
    }
    character = GetPartyCharacter(g_selectedObjectId);
    if (character == NULL) {
        return 0;
    }
    if (IsHumanCharacter(character)) {
        return 0;
    }
    ClearBattleConditions(GetCharacterConditions(character));
    ResetBattleTally(character);
    MarkPickDone();
    ClearPartyPosition(g_selectedObjectId);
    PlaySoundEffect(0x55);
    return 1;
}

RVA(0x00017900, 0x5f)
i16 PickDdsPurgeMember(void) {
    switch (GetGameSub()) {
        case 2:
            s_ddsRosterSlot = RunStatusListPicker(0);
            if (s_ddsRosterSlot != -1) {
                PrevGameSub();
            }
            break;
        case 1:
            RunStatusListPicker(1);
            return s_ddsRosterSlot;
        case 0:
            NextGameSub();
            NextGameSub();
            SetStatusColumn(2);
            break;
    }
    return -1;
}
