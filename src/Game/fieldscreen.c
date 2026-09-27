// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. Field-screen state words
// and small entry points of the field HUD (the rest of the TU is unclaimed).

#include <rva.h>

#include <File/DataFile.h>
#include <Game/AreaMap.h>
#include <Game/Automap.h>
#include <Game/AutomapData.h>
#include <Game/BagItems.h>
#include <Game/Condition.h>
#include <Game/Field.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/InfoBar.h>
#include <Game/ItemUse.h>
#include <Game/PartyPick.h>
#include <Game/StateStack.h>
#include <Game/TreasureBox.h>
#include <Game/WorldMap.h>
#include <Gfx/Background.h>
#include <Gfx/Blit.h>
#include <Gfx/Scene.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/VideoState.h>
#include <Gfx/Vram.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Sound/Sound.h>
#include <Text/Font.h>
#include <Text/TextBand.h>
#include <Ui/FieldMenus.h>
#include <Ui/Hotspot.h>
#include <Ui/Message.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <string.h>

// The field panel: nine command rows (their ids pick the command; flag
// PANEL_INPUT_DISABLED, set by SetFieldMenuMode, disables a row). Its picture is set by
// SetFieldPanelImage.
DATA(0x000687e8)
static struct {
    Panel panel;
    PanelRow more[8];
} s_fieldPanel = {
    {0, 0, 0, 9, 0, 0, 0, {0}, {{0, 5, 0, ReorderRowHandler}}},
    {
        {0, 8, 0, StatusRowHandler},
        {0, 3, 0, SkillRowHandler},
        {0, 2, 0, ItemRowHandler},
        {0, 4, 0, DdsRowHandler},
        {0, 0, 0, FightRowHandler},
        {0, 1, 0, TalkRowHandler},
        {0, 7, 0, MappingRowHandler},
        {0, 6, 0, MenuRowHandler},
    },
};

// @identity-TODO: a hold word the field view update (0x414770) reads.
// The field command panel: row 0 (flag PANEL_INPUT_DISABLED, id 2) has no handler; row 1
// (id 1) carries the field status bits 0 and 11.
DATA(0x00068858)
static struct {
    Panel panel;
    PanelRow more[1];
} s_commandPanel = {
    {0, 0, 0, 2, 0, 0, 0, {0}, {{PANEL_INPUT_DISABLED, 2, 0, NULL}}},
    {{0, 1, 0, CommandRowHandler}},
};

DATA(0x0007b7f0)
static i16 s_viewHold;

DATA(0x00091244)
i16 g_viewX;

DATA(0x00091246)
i16 g_viewY;

DATA(0x00091290)
i16 g_viewReset;

// Set when an encounter was requested (sound 1), cleared once handled.
DATA(0x0007b7e0)
static i16 s_encounterPending;

// The size of the automap block of the current level (0x421720), which
// bounds the reveal.
DATA(0x0007b7dc)
static MapCoord s_roomSize;

// The world cell the cursor box was last drawn on.
DATA(0x00068690)
static i16 s_cursorCellX = -1;

DATA(0x00068694)
static i16 s_cursorCellY = -1;

// @identity-TODO: two words reset to -1 together.
DATA(0x000686a0)
static i16 s_cursorA = -1;

DATA(0x000686a2)
static i16 s_cursorB = -1;

// Set while the area palette is switched on (a dark cell).
DATA(0x0007b7ec)
static i16 s_areaPaletteOn;

// Cached effect-frame image key (0x1400 + the frame-set id; -1: none).
DATA(0x00068698)
static i16 s_effectFramesKey = -1;

// @identity-TODO: an image handle only the dead FreeFieldImage touches.
DATA(0x0007b7f4)
static u32 s_fieldImage;

DATA(0x0007b7fc)
static u32 s_effectFrames;

// The effect backdrop: its key (0x1300 + id; -1 for none), image and block.
// @identity-TODO: what the block holds is unrecovered.
DATA(0x0006869c)
static i16 s_effectBackdropKey = -1;

DATA(0x0007b7f8)
static u32 s_backdropImage;

DATA(0x0007b808)
static void* s_backdropBlock;

// While set, field objects are drawn hidden.
DATA(0x0007b7e8)
static i16 s_objectsHidden;

DATA(0x0007b80c)
static i32 s_fieldMessages;

RVA(0x00014730, 0x12)
i16 ExchangeViewHold(i16 hold) {
    i16 old = s_viewHold;
    s_viewHold = hold;
    return old;
}

RVA(0x00014750, 0xe)
void RequestFieldRefresh(void) {
    g_fieldRedrawRequest = 1;
    RedrawFieldView();
}

RVA(0x00014760, 0xa)
void RefreshFieldScene(void) {
    RequestFieldRefresh();
    RebuildViewScene();
}

// Takes a pending field redraw (or a forced one): resets the mask and either
// rebuilds the view or, while the view is held, keeps the current position
// as the drawn one. Without either, only the info bar is refreshed.
RVA(0x00014770, 0x82)
i16 PrepareFieldRedraw(i16 force) {
    if (!g_fieldRedrawRequest && !force) {
        UpdateInfoBar();
        return 0;
    }
    g_fieldRedrawRequest = 1;
    ClearMaskView();
    ResetMask(1);
    if (!s_viewHold) {
        RebuildFieldView();
        return 1;
    }
    FlushPlaneUpdates();
    g_viewX = g_field.pos.x;
    g_viewY = g_field.pos.y;
    g_viewFacing = g_field.pos.direction;
    g_viewReset = 0;
    return 1;
}

RVA(0x00014800, 0x37)
i16 UpdateFieldScreen(i16 force) {
    if (PrepareFieldRedraw(force)) {
        SetInfoBarLayout(0);
        RefreshStatusPanel(1);
        return RedrawScreen(1, 0);
    }
    return 0;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00014840, 0x10)
void ResetWorldCursorCell(void) {
    s_cursorCellX = s_cursorCellY = -1;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00014850, 0x14)
void FreeFieldImage(void) {
    s_fieldImage = FreeImageHandle(s_fieldImage);
}

// Tracks the world map cursor while the button is held: when the mouse moves
// onto another world cell inside the map, redraws the cursor box there.
RVA(0x00014870, 0x8d)
void TrackWorldMapCursor(i16 layer) {
    MapCoord cell;
    if (!(g_mousePosition.buttons & MOUSE_RIGHT_DOWN)) {
        return;
    }
    cell = GetMouseWorldCell();
    if (!IsWorldCellInMap(cell.x, cell.y)) {
        return;
    }
    if (s_cursorCellX == cell.x && s_cursorCellY == cell.y) {
        return;
    }
    g_mouseRightClick = 0;
    if (g_mousePosition.x >= 0 && g_mousePosition.x < 0x280 && g_mousePosition.y >= 0
        && g_mousePosition.y < 0x148) {
        DrawWorldMapCursor(g_mousePosition.x, g_mousePosition.y, layer);
        s_cursorCellX = cell.x;
        s_cursorCellY = cell.y;
    }
}

// The automap cell under the mouse, offset by the world cell of the cursor
// box (PickWorldMapDestination's travel target).
// @identity-TODO: why the cursor cell is added is unrecovered.
RVA(0x00014900, 0x54)
MapCoord GetMouseTravelCell(void) {
    i16 x = g_mousePosition.x;
    i16 y = g_mousePosition.y;
    MapCoord cell;

    ScreenToAutomapCell(&x, &y);
    cell.x = x + s_cursorCellX;
    cell.y = y + s_cursorCellY;
    return cell;
}

// Whether a party member who can act (the leader or member 10/11, without a
// blocking condition) is present on a cell that allows it.
RVA(0x00014960, 0x93)
i16 CanOpenAutomap(void) {
    Character* character;
    if (IsCellCommandBlocked(g_field.pos.x, g_field.pos.y) == 1) {
        return 0;
    }
    character = GetCharacterById(0);
    if (character != NULL && !GetPickBlockingCondition(GetCharacterConditions(character))) {
        return 1;
    }
    character = GetCharacterById(10);
    if (character != NULL && !GetPickBlockingCondition(GetCharacterConditions(character))) {
        return 1;
    }
    character = GetCharacterById(11);
    if (character != NULL && !GetPickBlockingCondition(GetCharacterConditions(character))) {
        return 1;
    }
    return 0;
}

// Opens the party-order selection state.
RVA(0x00014a00, 0x36)
i16 ReorderRowHandler(PanelRow* row, i16 value, i16 op) {
    if (ApplyRowCheck(row, value, op)) {
        PlaySoundEffect(1);
        PushGameState(0x24);
    }
    return value;
}

// Row 1 (id 8): the status screen (game state 0x19).
RVA(0x00014a40, 0x36)
i16 StatusRowHandler(PanelRow* row, i16 value, i16 op) {
    if (ApplyRowCheck(row, value, op)) {
        PlaySoundEffect(1);
        PushGameState(0x19);
    }
    return value;
}

// Row 2 (id 3): the field skill screen (game state 0x1a).
RVA(0x00014a80, 0x36)
i16 SkillRowHandler(PanelRow* row, i16 value, i16 op) {
    if (ApplyRowCheck(row, value, op)) {
        PlaySoundEffect(1);
        PushGameState(0x1a);
    }
    return value;
}

// Row 3 (id 2): the item screen (game state 0xe), when a human member can act
// and the bag is not empty.
RVA(0x00014ac0, 0x8a)
i16 ItemRowHandler(PanelRow* row, i16 value, i16 op) {
    if (ApplyRowCheck(row, value, op)) {
        if (CanHumanMemberAct()) {
            if (CountBagEntries()) {
                PlaySoundEffect(1);
                PushGameState(0xe);
            } else {
                ClearPanelRowCheck(row);
                // "[ITEM] アイテムが有りません"
                ShowMessage(
                    "[ITEM] \203A\203C\203e\203\200\202\252\227L\202\350\202\334\202\271\202\361",
                    -1
                );
            }
        } else {
            ClearPanelRowCheck(row);
            // "[ITEM] 使用できる人が居ません"
            ShowMessage(
                "[ITEM] "
                "\216g\227p\202\305\202\253\202\351\220l\202\252\213\217\202\334\202\271\202\361",
                -1
            );
        }
    }
    return value;
}

// Whether one of the six party members is human (id below 0x20) and free of
// blocking conditions.
RVA(0x00014b50, 0x3b)
i16 CanHumanMemberAct(void) {
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character != NULL && IsHumanCharacter(character)
            && !GetPickBlockingCondition(GetCharacterConditions(character))) {
            return 1;
        }
    }
    return 0;
}

// Row 4 (id 4): the DDS screen (game state 0x1e), once the DDS is carried.
RVA(0x00014b90, 0x81)
i16 DdsRowHandler(PanelRow* row, i16 value, i16 op) {
    if (ApplyRowCheck(row, value, op)) {
        if (CanOpenAutomap()) {
            if (IsEventFlagSet(2, 6)) {
                ClearPanelRowCheck(row);
                // "[DDS] DDSを所持していません"
                ShowMessage(
                    "[DDS] "
                    "DDS\202\360\217\212\216\235\202\265\202\304\202\242\202\334\202\271\202\361",
                    -1
                );
            } else {
                PlaySoundEffect(1);
                PushGameState(0x1e);
            }
        } else {
            ClearPanelRowCheck(row);
        }
    }
    return value;
}

// Row 5 (id 0): fight the field actors in sight.
RVA(0x00014c20, 0x83)
i16 FightRowHandler(PanelRow* row, i16 value, i16 op) {
    if (ApplyRowCheck(row, value, op)) {
        if (!CountHotspotsOfKind(2, 0)) {
            ClearPanelRowCheck(row);
            // "[FIGHT] 戦う相手が居ません"
            ShowMessage(
                "[FIGHT] \220\355\202\244\221\212\216\350\202\252\213\217\202\334\202\271\202\361",
                -1
            );
            return value;
        }
        if (!g_fieldBattleActive) {
            PlaySoundEffect(1);
            PlayMusic(0xd, 1);
        }
        g_fieldBattleActive = 1;
        ResetPartyTurnState();
    }
    return value;
}

// Row 6 (id 1): talk to a field actor in sight, once the DCS is carried.
RVA(0x00014cb0, 0xd1)
i16 TalkRowHandler(PanelRow* row, i16 value, i16 op) {
    if (ApplyRowCheck(row, value, op)) {
        if (CanOpenAutomap()) {
            if (!CountHotspotsOfKind(2, 0)) {
                ClearPanelRowCheck(row);
                // "[TALK] 会話相手が居ません"
                ShowMessage(
                    "[TALK] \211\357\230b\221\212\216\350\202\252\213\217\202\334\202\271\202\361",
                    -1
                );
            } else if (IsEventFlagSet(2, 7) && IsEventFlagSet(2, 8)) {
                ClearPanelRowCheck(row);
                // "[TALK] DCSを所持していません"
                ShowMessage(
                    "[TALK] "
                    "DCS\202\360\217\212\216\235\202\265\202\304\202\242\202\334\202\271\202\361",
                    -1
                );
            } else {
                PlaySoundEffect(1);
                ResetPartyTurnState();
                CloseFieldWindows();
                RequestTalk();
            }
        } else {
            ClearPanelRowCheck(row);
        }
    }
    return value;
}

// Row 7 (id 7): the automap (game state 0x20), once the AMS is carried.
RVA(0x00014d90, 0x81)
i16 MappingRowHandler(PanelRow* row, i16 value, i16 op) {
    if (ApplyRowCheck(row, value, op)) {
        if (CanOpenAutomap()) {
            if (IsEventFlagSet(2, 0x39)) {
                ClearPanelRowCheck(row);
                // "[MAPPING] AMSを所持していません"
                ShowMessage(
                    "[MAPPING] "
                    "AMS\202\360\217\212\216\235\202\265\202\304\202\242\202\334\202\271\202\361",
                    -1
                );
            } else {
                PlaySoundEffect(1);
                PushGameState(0x20);
            }
        } else {
            ClearPanelRowCheck(row);
        }
    }
    return value;
}

// Row 8 (id 6): the field menu (game state 0xf).
RVA(0x00014e20, 0x36)
i16 MenuRowHandler(PanelRow* row, i16 value, i16 op) {
    if (ApplyRowCheck(row, value, op)) {
        PlaySoundEffect(1);
        PushGameState(0xf);
    }
    return value;
}

RVA(0x00014e60, 0xa)
void SetFieldPanelImage(u32 image) {
    s_fieldPanel.panel.image = image;
}

// Runs field panel row `row` (checking it for op 1), then clears `clear` and
// sets `set` in its flags; returns whether it was checked before.
RVA(0x00014e70, 0x5e)
i32 RunFieldPanelRow(i16 row, i32 op, u16 clear, u16 set) {
    i32 checked = TestFlagBits(&GetPanelRow(&s_fieldPanel.panel, row)->flags, PANEL_ROW_CHECKED);
    RunPanelRow(&s_fieldPanel.panel, row, op == 1);
    ClearFlagBits(&GetPanelRow(&s_fieldPanel.panel, row)->flags, clear);
    SetFlagBits(&GetPanelRow(&s_fieldPanel.panel, row)->flags, set);
    return checked;
}

// Sets or clears field panel row `row`'s check; returns whether it was set.
RVA(0x00014ed0, 0x30)
i32 SetFieldPanelRowChecked(i16 row, i16 on) {
    i32 checked = IsPanelRowChecked(&s_fieldPanel.panel, row);
    SetPanelRowFlags(&s_fieldPanel.panel, row, PANEL_ROW_CHECKED, on);
    return checked;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00014f00, 0x1b)
i32 IsFieldPanelRowChecked(i16 row) {
    return TestFlagBits(&GetPanelRow(&s_fieldPanel.panel, row)->flags, PANEL_ROW_CHECKED);
}

// Sets which field panel rows menu mode `mode` disables (flag PANEL_INPUT_DISABLED): none
// in mode 0; rows 2, 3 and 5 in mode 1; 5, 6 and 7 in mode 2; 2, 3, 5 and 7
// in mode 3. Rows 0, 1, 4 and 8 are always enabled.
RVA(0x00014f20, 0x1f0)
void SetFieldMenuMode(i16 mode) {
    SetPanelRowFlags(&s_fieldPanel.panel, 0, PANEL_INPUT_DISABLED, 0);
    SetPanelRowFlags(&s_fieldPanel.panel, 1, PANEL_INPUT_DISABLED, 0);
    SetPanelRowFlags(&s_fieldPanel.panel, 8, PANEL_INPUT_DISABLED, 0);
    SetPanelRowFlags(&s_fieldPanel.panel, 4, PANEL_INPUT_DISABLED, 0);
    switch (mode) {
        case 0:
            SetPanelRowFlags(&s_fieldPanel.panel, 2, PANEL_INPUT_DISABLED, 0);
            SetPanelRowFlags(&s_fieldPanel.panel, 3, PANEL_INPUT_DISABLED, 0);
            SetPanelRowFlags(&s_fieldPanel.panel, 5, PANEL_INPUT_DISABLED, 0);
            SetPanelRowFlags(&s_fieldPanel.panel, 6, PANEL_INPUT_DISABLED, 0);
            SetPanelRowFlags(&s_fieldPanel.panel, 7, PANEL_INPUT_DISABLED, 0);
            break;
        case 1:
            SetPanelRowFlags(&s_fieldPanel.panel, 2, PANEL_INPUT_DISABLED, 1);
            SetPanelRowFlags(&s_fieldPanel.panel, 3, PANEL_INPUT_DISABLED, 1);
            SetPanelRowFlags(&s_fieldPanel.panel, 5, PANEL_INPUT_DISABLED, 1);
            SetPanelRowFlags(&s_fieldPanel.panel, 6, PANEL_INPUT_DISABLED, 0);
            SetPanelRowFlags(&s_fieldPanel.panel, 7, PANEL_INPUT_DISABLED, 0);
            break;
        case 2:
            SetPanelRowFlags(&s_fieldPanel.panel, 2, PANEL_INPUT_DISABLED, 0);
            SetPanelRowFlags(&s_fieldPanel.panel, 3, PANEL_INPUT_DISABLED, 0);
            SetPanelRowFlags(&s_fieldPanel.panel, 5, PANEL_INPUT_DISABLED, 1);
            SetPanelRowFlags(&s_fieldPanel.panel, 6, PANEL_INPUT_DISABLED, 1);
            SetPanelRowFlags(&s_fieldPanel.panel, 7, PANEL_INPUT_DISABLED, 1);
            break;
        case 3:
            SetPanelRowFlags(&s_fieldPanel.panel, 2, PANEL_INPUT_DISABLED, 1);
            SetPanelRowFlags(&s_fieldPanel.panel, 3, PANEL_INPUT_DISABLED, 1);
            SetPanelRowFlags(&s_fieldPanel.panel, 5, PANEL_INPUT_DISABLED, 1);
            SetPanelRowFlags(&s_fieldPanel.panel, 6, PANEL_INPUT_DISABLED, 0);
            SetPanelRowFlags(&s_fieldPanel.panel, 7, PANEL_INPUT_DISABLED, 1);
            break;
    }
}

// The field commands as direct actions (the keyboard shortcuts); `id` is the
// acting member.
RVA(0x00015110, 0x53)
void TalkCommand(void) {
    if (!CanOpenAutomap()) {
        return;
    }
    if (IsEventFlagSet(2, 7) && IsEventFlagSet(2, 8)) {
        // "[TALK] DCSを所持していません"
        ShowMessage(
            "[TALK] DCS\202\360\217\212\216\235\202\265\202\304\202\242\202\334\202\271\202\361",
            -1
        );
        return;
    }
    PlaySoundEffect(1);
    ResetPartyTurnState();
    CloseFieldWindows();
    RequestTalk();
}

RVA(0x00015170, 0x58)
void FightCommand(i16 id) {
    if (!CountFieldObjects()) {
        // "[FIGHT] 戦う相手が居ません"
        ShowMessage(
            "[FIGHT] \220\355\202\244\221\212\216\350\202\252\213\217\202\334\202\271\202\361",
            -1
        );
        return;
    }
    if (!g_fieldBattleActive) {
        PlaySoundEffect(1);
        PlayMusic(0xd, 1);
        ResetPartyTurnState();
    }
    g_fieldBattleActive = 1;
    SetMemberPickRole(id, 1);
}

RVA(0x000151d0, 0x58)
void GunCommand(i16 id) {
    if (!CountFieldObjects()) {
        // "[GUN] 戦う相手が居ません"
        ShowMessage(
            "[GUN] \220\355\202\244\221\212\216\350\202\252\213\217\202\334\202\271\202\361",
            -1
        );
        return;
    }
    if (!g_fieldBattleActive) {
        PlaySoundEffect(1);
        PlayMusic(0xd, 1);
        ResetPartyTurnState();
    }
    g_fieldBattleActive = 1;
    SetMemberPickRole(id, 2);
}

// A skill: picked in an encounter, else from the field skill screen.
RVA(0x00015230, 0x6f)
void SkillCommand(i16 id) {
    if (CountFieldObjects()) {
        if (!g_fieldBattleActive) {
            PlaySoundEffect(1);
            PlayMusic(0xd, 1);
        }
        g_fieldBattleActive = 1;
        PlaySoundEffect(1);
        SetMemberPickRole(id, 4);
        return;
    }
    PlaySoundEffect(1);
    PushGameState(0x1a);
    SetFieldSkillUser(id);
}

// An item: picked in an encounter, else from the item screen when member
// `id` can act and the bag is not empty.
RVA(0x000152a0, 0xac)
void ItemCommand(i16 id) {
    if (CountFieldObjects()) {
        if (!g_fieldBattleActive) {
            PlaySoundEffect(1);
            PlayMusic(0xd, 1);
        }
        g_fieldBattleActive = 1;
        PlaySoundEffect(1);
        SetMemberPickRole(id, 5);
        return;
    }
    if (!CanMemberAct(id)) {
        // "[ITEM] アイテムを使用できません"
        ShowMessage(
            "[ITEM] "
            "\203A\203C\203e\203\200\202\360\216g\227p\202\305\202\253\202\334\202\271\202\361",
            -1
        );
        return;
    }
    if (CountBagEntries()) {
        PlaySoundEffect(1);
        PushGameState(0xe);
        SetUseMemberId(id);
        return;
    }
    // "[ITEM] アイテムが有りません"
    ShowMessage("[ITEM] \203A\203C\203e\203\200\202\252\227L\202\350\202\334\202\271\202\361", -1);
}

// Whether party member `id` is present, human (id below 0x20) and free of
// blocking conditions.
RVA(0x00015350, 0x51)
i16 CanMemberAct(i16 id) {
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character != NULL && character->id == id) {
            if (!IsHumanCharacter(character)) {
                return 0;
            }
            if (!GetPickBlockingCondition(GetCharacterConditions(character))) {
                return 1;
            }
        }
    }
    return 0;
}

RVA(0x000153b0, 0x58)
void DefenceCommand(i16 id) {
    if (!CountFieldObjects()) {
        // "[DEFENCE] 戦う相手が居ません"
        ShowMessage(
            "[DEFENCE] \220\355\202\244\221\212\216\350\202\252\213\217\202\334\202\271\202\361",
            -1
        );
        return;
    }
    if (!g_fieldBattleActive) {
        PlaySoundEffect(1);
        PlayMusic(0xd, 1);
        ResetPartyTurnState();
    }
    g_fieldBattleActive = 1;
    SetMemberPickRole(id, 8);
}

// Returns the selected companion from the active party.
RVA(0x00015410, 0x1a)
void ReturnCommand(i16 id) {
    PlaySoundEffect(1);
    SetMemberPickRole(id, 7);
}

RVA(0x00015430, 0x3e)
void DdsCommand(void) {
    if (!CanOpenAutomap()) {
        return;
    }
    if (IsEventFlagSet(2, 6)) {
        // "[DDS] DDSを所持していません"
        ShowMessage(
            "[DDS] DDS\202\360\217\212\216\235\202\265\202\304\202\242\202\334\202\271\202\361",
            -1
        );
        return;
    }
    PlaySoundEffect(1);
    PushGameState(0x1e);
}

RVA(0x00015470, 0x14)
void StatusCommand(void) {
    PlaySoundEffect(1);
    PushGameState(0x19);
}

RVA(0x00015490, 0x14)
void SetEncounterPending(void) {
    PlaySoundEffect(1);
    s_encounterPending = 1;
}

RVA(0x000154b0, 0xa)
void ClearEncounterPending(void) {
    s_encounterPending = 0;
}

RVA(0x000154c0, 0x7)
i16 GetEncounterPending(void) {
    return s_encounterPending;
}

// Opens the field menu (game state 15).
RVA(0x000154d0, 0x14)
void OpenFieldMenu(void) {
    PlaySoundEffect(1);
    PushGameState(0xf);
}

// Opens the automap (game state 0x20) unless the MAPPING program is missing.
RVA(0x000154f0, 0x3e)
void OpenAutomap(void) {
    if (!CanOpenAutomap()) {
        return;
    }
    if (IsEventFlagSet(2, 0x39)) {
        // "[MAPPING] AMSを所持していません"
        ShowMessage(
            "[MAPPING] AMS\202\360\217\212\216\235\202\265\202\304\202\242\202\334\202\271\202\361",
            -1
        );
        return;
    }
    PlaySoundEffect(1);
    PushGameState(0x20);
}

// Whether this character can use the automap command on the current cell.
RVA(0x00015530, 0x56)
i16 CanCharacterOpenAutomap(Character* character) {
    if (IsCellCommandBlocked(g_field.pos.x, g_field.pos.y) != 1 && character != NULL
        && (character->id == 0 || character->id == 10 || character->id == 11)
        && !GetPickBlockingCondition(GetCharacterConditions(character))) {
        return 1;
    }
    return 0;
}

RVA(0x00015590, 0x7)
i16 GetFieldBattleActive(void) {
    return g_fieldBattleActive;
}

RVA(0x000155a0, 0x29)
void RevealAutomapRoom(i16 x, i16 y) {
    s_roomSize = GetAreaSize(g_field.pos.level);
    RevealAutomapCells(x, y);
}

static i16 MarkAutomapRowSpan(i16* x, i16 y);
static i16 CanRevealAutomapSouth(i16 x, i16 y);
static i16 CanRevealAutomapNorth(i16 x, i16 y);

RVA(0x000155d0, 0x6a)
void RevealAutomapCells(i16 x, i16 y) {
    i16 left = x;
    i16 right = MarkAutomapRowSpan(&left, y);
    i16 cell;
    for (cell = left; cell <= right; ++cell) {
        if (CanRevealAutomapSouth(cell, y)) {
            RevealAutomapCells(cell, y + 1);
        }
        if (CanRevealAutomapNorth(cell, y)) {
            RevealAutomapCells(cell, y - 1);
        }
    }
}

RVA(0x00015640, 0xe2)
static i16 MarkAutomapRowSpan(i16* x, i16 y) {
    i16 right = *x;
    i16 next;
    i16 previous;
    i16 wall;
    if (right < s_roomSize.x) {
        next = right + 1;
        while (right < s_roomSize.x) {
            MarkAutomapCell(g_field.pos.area, g_field.pos.level, right, y);
            wall = GetMapWallKind(right, y, 1);
            if (WallStops(wall, WALL_STOP_MOVEMENT) || next >= s_roomSize.x
                || !IsRoomCell(next, y)) {
                break;
            }
            ++right;
            ++next;
        }
    }
    while (*x >= 0) {
        MarkAutomapCell(g_field.pos.area, g_field.pos.level, *x, y);
        wall = GetMapWallKind(*x, y, 3);
        if (WallStops(wall, WALL_STOP_MOVEMENT)) {
            break;
        }
        previous = *x;
        --previous;
        if (previous < 0 || !IsRoomCell(previous, y)) {
            break;
        }
        --*x;
    }
    return right;
}

RVA(0x00015730, 0x73)
static i16 CanRevealAutomapSouth(i16 x, i16 y) {
    i16 wall = GetMapWallKind(x, y, 2);
    if (WallStops(wall, WALL_STOP_MOVEMENT)) {
        return 0;
    }
    ++y;
    if (y >= s_roomSize.y) {
        return 0;
    }
    if (!IsRoomCell(x, y)) {
        return 0;
    }
    return IsAutomapCellHidden(x, y, g_field.pos.area, g_field.pos.level) != 0;
}

RVA(0x000157b0, 0x6f)
static i16 CanRevealAutomapNorth(i16 x, i16 y) {
    i16 wall = GetMapWallKind(x, y, 0);
    if (WallStops(wall, WALL_STOP_MOVEMENT)) {
        return 0;
    }
    --y;
    if (y < 0) {
        return 0;
    }
    if (!IsRoomCell(x, y)) {
        return 0;
    }
    return IsAutomapCellHidden(x, y, g_field.pos.area, g_field.pos.level) != 0;
}

RVA(0x00015820, 0x10)
void ResetFieldCursors(void) {
    s_cursorA = s_cursorB = -1;
}

// @identity-TODO: an empty hook.
RVA(0x00015830, 0x1)
void FieldScreenNop(i16 unused) {}

// @identity-TODO: a stub image loader (returns no handle).
RVA(0x00015840, 0x10)
void LoadCommandMenuImage(void) {
    s_commandPanel.panel.image = LoadMenuImage(0x11);
}

RVA(0x00015850, 0x14)
void FreeCommandMenuImage(void) {
    s_commandPanel.panel.image = FreeImageHandle(s_commandPanel.panel.image);
}

RVA(0x00015870, 0x35)
i16 SetFieldStatusBit11(i16 on) {
    i16 old = TestPanelRowFlags(&s_commandPanel.panel, 1, 0x800);
    SetPanelRowFlags(&s_commandPanel.panel, 1, 0x800, on);
    return old;
}

RVA(0x000158b0, 0x2f)
i16 SetFieldStatusBit0(i16 on) {
    i16 old = TestPanelRowFlags(&s_commandPanel.panel, 1, PANEL_ROW_CHECKED);
    SetPanelRowFlags(&s_commandPanel.panel, 1, PANEL_ROW_CHECKED, on);
    return old;
}

// The command panel's row handler.
RVA(0x000158e0, 0x1d)
i16 CommandRowHandler(PanelRow* row, i16 value, i16 op) {
    ApplyRowCheck(row, value, op);
    return value;
}

RVA(0x00015900, 0x3)
u32 LoadMenuImage(i16 id) {
    return 0;
}

RVA(0x00015910, 0x9)
i16 GetPanelRowId(PanelRow* row) {
    return row->id;
}

RVA(0x00015920, 0x13)
MapCoord GetPanelSize(Panel* panel) {
    MapCoord coord;
    coord.x = 0;
    coord.y = 0;
    return coord;
}

RVA(0x00015940, 0x5f)
void DrawPanel(Panel* panel, i16* cell, i16 mode) {
    i16 row;
    if (mode < 0 || (panel->flags & PANEL_HIDDEN)) {
        return;
    }
    if (cell != NULL) {
        *cell = GetPanelTextCell(panel);
    }
    for (row = 0; row < GetPanelRowCount(panel); ++row) {
        DrawPanelRow(panel, panel->x, panel->y, row, mode);
    }
}

// Draws a visible panel row through the hotspot highlighter.
RVA(0x000159a0, 0x33)
void DrawPanelRow(Panel* panel, i16 x, i16 y, i16 row, i16 mode) {
    i16 id;
    if (!IsPanelRowHidden(panel, row)) {
        id = GetPanelRowId(GetPanelRow(panel, row));
        HighlightHotspot(mode, id, 0);
    }
}

// Hides every row of a panel; `cell`, when given, receives the panel's text
// cell index.
RVA(0x000159e0, 0x53)
void ClearPanel(Panel* panel, i16* cell) {
    i16 i;
    if (panel->flags & PANEL_HIDDEN) {
        return;
    }
    if (cell != NULL) {
        *cell = GetPanelTextCell(panel);
    }
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        ClearPanelRow(panel, panel->x, panel->y, i);
    }
}

RVA(0x00015a40, 0x30)
void ClearPanelRow(Panel* panel, i16 x, i16 y, i16 row) {
    i16 id;
    if (!IsPanelRowHidden(panel, row)) {
        id = GetPanelRowId(GetPanelRow(panel, row));
        HighlightHotspot(0, id, 0);
    }
}

// Whether x/y hits the hotspot of a row (`flags` bit 0: strict).
RVA(0x00015a70, 0x20)
i16 HitTestPanelRow(Panel* panel, i16 id, i16 x, i16 y, u8 flags) {
    return HitTestHotspot(id, x, y, flags & 1);
}

RVA(0x00015a90, 0x12)
i16 ExchangeObjectsHidden(i16 hidden) {
    i16 old = s_objectsHidden;
    s_objectsHidden = hidden;
    return old;
}

RVA(0x00015ab0, 0x7)
i16 GetObjectsHidden(void) {
    return s_objectsHidden;
}

// Switches the area palette on while the party stands on a dark cell.
RVA(0x00015ac0, 0x4e)
void UpdateAreaPalette(void) {
    if (IsDarkCell(g_field.pos.x, g_field.pos.y)) {
        SetAreaPaletteMode(1);
        s_areaPaletteOn = 1;
        return;
    }
    if (s_areaPaletteOn) {
        SetAreaPaletteMode(0);
        s_areaPaletteOn = 0;
    }
}

// Sets the view palette by whether the cell ahead of the party is dark.
RVA(0x00015b10, 0x79)
void UpdateViewPalette(void) {
    i16 y;
    i16 x;
    i16 direction;
    x = g_field.pos.x;
    y = g_field.pos.y;
    direction = TurnDirection(g_field.pos.direction, g_field.moveCommand);
    StepMapCoordBy(&x, &y, direction, 0, -1);
    if (IsDarkCell(x, y)) {
        SetViewPaletteMode(1);
        return;
    }
    SetViewPaletteMode(0);
}

void RedrawFieldViewAt(VideoPlane* header, i16 unused);

// Rebuilds the field view at the party's position.
RVA(0x00015b90, 0x44)
i16 RebuildFieldView(void) {
    RevealAreaMapAt(g_field.pos.x, g_field.pos.y);
    ClearDrawTable();
    FlushPlaneUpdates();
    RedrawFieldViewAt(GetPlaneHeader(0), 0);
    s_objectsHidden = 0;
    return 1;
}

// @identity-TODO: both arguments are unused.
RVA(0x00015be0, 0x20)
void RedrawFieldViewAt(VideoPlane* header, i16 unused) {
    RedrawFieldAt(g_field.pos.x, g_field.pos.y, g_field.pos.direction);
}

RVA(0x00015c00, 0x1d)
void FreeEffectFrames(void) {
    s_effectFramesKey = -1;
    s_effectFrames = FreeImageHandle(s_effectFrames);
}

RVA(0x00015c20, 0x31)
void FreeEffectBackdrop(void) {
    s_effectBackdropKey = -1;
    s_backdropBlock = FreeBlock(s_backdropBlock);
    s_backdropImage = FreeImageHandle(s_backdropImage);
}

// Selects the effect frames (key 0x1400 + `frames`) and, when they change,
// the backdrop (key 0x1300 + `backdrop`), dropping the cached images.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00015c60, 0x41)
void SelectEffectImages(i16 frames, i16 backdrop) {
    frames += 0x1400;
    if (s_effectFramesKey != frames) {
        FreeEffectFrames();
        s_effectFramesKey = frames;
        backdrop += 0x1300;
        if (s_effectBackdropKey != backdrop) {
            FreeEffectBackdrop();
            s_effectBackdropKey = backdrop;
        }
    }
}

RVA(0x00015cb0, 0x5)
i16 AreObjectsHidden(i16 view, i16 across, i16 along, i16 side) {
    return GetObjectsHidden();
}

// Closes the party picker and flashes the effect frames 0, 2 .. 8 over plane
// 0 with sound 6 (the party walks into a damaging wall).
RVA(0x00015cc0, 0x48)
void PlayWallEffect(void) {
    i16 i;
    RunPartyPicker(-1);
    PlaySoundEffect(6);
    for (i = 0; i < 10; i += 2) {
        DrawImageFrame(GetPlaneData(0), s_effectFrames, i, 0, 0, 0, 0, 0, 1);
    }
}

RVA(0x00015d10, 0x2d)
void LoadFieldMessages(void) {
    FILE* fp = OpenDataFile(0x101, 12, 0);
    s_fieldMessages = ReadCryptHandle(fp);
    CloseDataFile(fp);
}

RVA(0x00015d40, 0x378)
void UpdateFieldHud(i16 x, i16 y, i16 direction) {
    i16 left;
    i16 right;
    i16 center;
    i16 leftBlocked;
    i16 rightBlocked;
    i16 along;
    i16 scan;
    i16 code;
    MapCoord origin;
    MapCoord cell;
    center = left = right = leftBlocked = rightBlocked = 0;
    if (IsDarkCell(g_field.pos.x, g_field.pos.y)) {
        return;
    }
    origin.x = x;
    origin.y = y;
    for (along = 0; along >= -4; along--) {
        if (left == 0) {
            if (along > -4 && g_leftSideWalls[-along][0] == 1) {
                cell = OffsetCoordClamped(origin, direction, -1, along);
                code = GetEventCellCode(cell.x, cell.y);
                if (code && (center == 0 || along == 0)) {
                    left = DrawFieldMessage(code, TEXT_BAND_LEFT, 1);
                }
            } else if (leftBlocked == 0) {
                if (g_leftFrontWalls[-along][0] == 1) {
                    cell = OffsetCoordClamped(origin, direction, -1, along - 1);
                    code = GetEventCellCode(cell.x, cell.y);
                    if (code) {
                        for (scan = 0; scan > along; scan--) {
                            if (g_centerFrontWalls[-scan]) {
                                left = 2;
                                break;
                            }
                        }
                        if (left == 0) {
                            for (scan = 0; scan >= along; scan--) {
                                if (g_leftSideWalls[-scan][0]) {
                                    left = 2;
                                    break;
                                }
                            }
                            if (left == 0) {
                                left = DrawFieldMessage(code, TEXT_BAND_LEFT, 0);
                            }
                        }
                    }
                } else if (g_leftFrontWalls[-along][0]) {
                    leftBlocked = 1;
                }
            }
        }
        if (right == 0) {
            if (along > -4 && g_rightSideWalls[-along][0] == 1) {
                cell = OffsetCoordClamped(origin, direction, 1, along);
                code = GetEventCellCode(cell.x, cell.y);
                if (code && (center == 0 || along == 0)) {
                    right = DrawFieldMessage(code, TEXT_BAND_RIGHT, 1);
                }
            } else if (rightBlocked == 0) {
                if (g_rightFrontWalls[-along][0] == 1) {
                    cell = OffsetCoordClamped(origin, direction, 1, along - 1);
                    code = GetEventCellCode(cell.x, cell.y);
                    if (code) {
                        for (scan = 0; scan > along; scan--) {
                            if (g_centerFrontWalls[-scan]) {
                                right = 2;
                                break;
                            }
                        }
                        if (right == 0) {
                            for (scan = 0; scan >= along; scan--) {
                                if (g_rightSideWalls[-scan][0]) {
                                    right = 2;
                                    break;
                                }
                            }
                            if (right == 0) {
                                right = DrawFieldMessage(code, TEXT_BAND_RIGHT, 0);
                            }
                        }
                    }
                } else if (g_rightFrontWalls[-along][0]) {
                    rightBlocked = 1;
                }
            }
        }
        if (center == 0) {
            if (g_centerFrontWalls[-along] == 1) {
                cell = OffsetCoordClamped(origin, direction, 0, along - 1);
                code = GetEventCellCode(cell.x, cell.y);
                if (code) {
                    for (scan = 0; scan < along; scan--) {
                        if (g_centerFrontWalls[-scan]) {
                            center = 2;
                            break;
                        }
                    }
                    if (center == 0) {
                        center = DrawFieldMessage(code, TEXT_BAND_CENTER, 0);
                    }
                }
            }
            if (g_centerFrontWalls[-along]) {
                center = 2;
            }
        }
    }
}

RVA(0x000160c0, 0x163)
i16 DrawFieldMessage(i16 code, i16 band, i16 marked) {
    FieldMessage* message = GetFieldMessage(code);
    char* text = message->text;
    i16 x;
    i16 y;
    if (strlen(text) == 0) {
        return 0;
    }
    x = 0;
    y = 0;
    g_scratchBuffer[0] = 0;
    switch (band) {
        case TEXT_BAND_RIGHT:
            strcpy(g_scratchBuffer, text);
            if (marked) {
                strcat(g_scratchBuffer, "\201\243");
            }
            x = (78 - strlen(g_scratchBuffer)) * 8;
            y = 62;
            break;
        case TEXT_BAND_CENTER:
            strcpy(g_scratchBuffer, text);
            x = (40 - strlen(g_scratchBuffer) / 2) * 8;
            y = 40;
            break;
        case TEXT_BAND_LEFT:
            if (marked) {
                strcpy(g_scratchBuffer, "\201\243");
            }
            strcat(g_scratchBuffer, text);
            x = 16;
            y = 62;
            break;
    }
    DrawBandText(x, y, g_scratchBuffer, 0x2650, band);
    return 1;
}

RVA(0x00016230, 0x3c)
FieldMessage* GetFieldMessage(i16 code) {
    FieldMessageOffsets* messages;
    if (code < 0x40 || code > 0x9f) {
        code = 0x9f;
    }
    messages = HandleReadPtr(s_fieldMessages);
    code -= 0x40;
    return OffsetBy(messages, messages->offsets[code]);
}
