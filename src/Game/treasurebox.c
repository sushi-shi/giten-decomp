// @identity-TODO: the owning TU is unproven. One retail object: the .bss
// statics of the treasure box, party reorder, analyze window, world-map
// events, item-list menus, training, panel input, automap bitmaps and area NPC
// code interleave in one run, each read only by its own part's code; their
// initialized data, the info bar's included, forms one .data run out of .text
// order ahead of one run of literals; and the code, from the treasure box
// through the area NPCs, is contiguous in .text.

#include <rva.h>

#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <File/DataTableId.h>
#include <Game/ActorFlag.h>
#include <Game/Alignment.h>
#include <Game/Analyze.h>
#include <Game/AnalyzeData.h>
#include <Game/AreaLevel.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/Attitude.h>
#include <Game/Automap.h>
#include <Game/AutomapData.h>
#include <Game/BagItems.h>
#include <Game/BattleEffect.h>
#include <Game/CellTrap.h>
#include <Game/Character.h>
#include <Game/Clock.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/DoorRegion.h>
#include <Game/EquipRequirements.h>
#include <Game/Field.h>
#include <Game/FieldActor.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/HumanId.h>
#include <Game/InfoBar.h>
#include <Game/ItemMenu.h>
#include <Game/ItemRecord.h>
#include <Game/MapArea.h>
#include <Game/ModeFlags.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/PartyReorder.h>
#include <Game/SaveGame.h>
#include <Game/Scene.h>
#include <Game/SceneHotspot.h>
#include <Game/SkillList.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Game/StatusDraw.h>
#include <Game/StatusScreen.h>
#include <Game/TreasureBox.h>
#include <Game/WorldMap.h>
#include <Gfx/Background.h>
#include <Gfx/Render.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/VramAccess.h>
#include <Giten/Resource.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/OwnedFlag.h>
#include <Script/ScriptVars.h>
#include <Sound/Sound.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/MenuBox.h>
#include <Ui/Message.h>
#include <Ui/Panel.h>
#include <Ui/PartySlotSelection.h>
#include <Util/BitSet.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x00068b30)
static AutomapIcon s_mapIcons[] = {
    {0x40, 6, AUTOMAP_DETAIL_BASIC},
    {CELL_EXIT, 7, AUTOMAP_DETAIL_BASIC},
    {CELL_STAIRS_UP, 8, AUTOMAP_DETAIL_BASIC},
    {CELL_STAIRS_DOWN, 9, AUTOMAP_DETAIL_BASIC},
    {0x44, 10, AUTOMAP_DETAIL_BASIC},
    {0x45, 10, AUTOMAP_DETAIL_BASIC},
    {0x46, 10, AUTOMAP_DETAIL_BASIC},
    {0x48, 11, AUTOMAP_DETAIL_NPCS},
    {0x50, 12, AUTOMAP_DETAIL_NPCS},
    {0x51, 13, AUTOMAP_DETAIL_NPCS},
    {0x52, 14, AUTOMAP_DETAIL_NPCS},
    {0x53, 15, AUTOMAP_DETAIL_NPCS},
    {0x54, 16, AUTOMAP_DETAIL_NPCS},
    {0x55, 17, AUTOMAP_DETAIL_NPCS},
    {0x56, 18, AUTOMAP_DETAIL_NPCS},
    {0x57, 19, AUTOMAP_DETAIL_NPCS},
    {0x58, 20, AUTOMAP_DETAIL_NPCS},
    {0x59, 21, AUTOMAP_DETAIL_NPCS},
    {0x5b, 22, AUTOMAP_DETAIL_NPCS},
    {0x7b, 23, AUTOMAP_DETAIL_NPCS},
    {0x7d, 9, AUTOMAP_DETAIL_NPCS},
    {0x85, 18, AUTOMAP_DETAIL_NPCS},
    {0x86, 18, AUTOMAP_DETAIL_NPCS},
    {0x87, 18, AUTOMAP_DETAIL_NPCS},
    {CELL_STEPS_UP, 8, AUTOMAP_DETAIL_BASIC},
    {CELL_STEPS_DOWN, 9, AUTOMAP_DETAIL_BASIC},
    {0xbf, 24, AUTOMAP_DETAIL_BASIC},
    {CELL_LIST_END, 24, AUTOMAP_DETAIL_BASIC},
};

DATA(0x00068b88)
NpcTexture g_npcTextures[12] = {
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
    {0xffff, 0},
};

// @identity-TODO: no code in this image touches these eight words, which are
// one datum (cl keeps no all-zero initialized item in .data). The PC-98
// build keeps the same eight words; a reader there would name them.
DATA(0x00068be8)
static i16 s_unusedWordTable[8] = {0, 0, 2, 2, 0, 0, 0, 0};

// The five attitude names, indexed by Character.attitude.
DATA(0x00068bf8)
static char* s_attitudeNames[ATTITUDE_COUNT] = {
    "\210\243\212\350\223I",      // 哀願的
    "\227F\215D\223I",            // 友好的
    "\222\264\223G\221\316\223I", // 超敵対的
    "\223G\221\316\223I",         // 敵対的
    "\222\312\217\355",           // 通常
};

DATA(0x00068c0c)
static i16 s_markedLayer = -1;

DATA(0x00068c10)
static i16 s_markedX = -1;

DATA(0x00068c14)
static i16 s_markedY = -1;

// The window with the target's name (and the prompts).
DATA(0x00068c18)
static i16 s_namePlane = TEXT_PLANE_NONE;

// The window with the target's analyze data.
DATA(0x00068c1c)
static i16 s_dataPlane = TEXT_PLANE_NONE;

// The law/chaos and light/dark letters, indexed by AlignmentClass + 1.
DATA(0x00068c20)
static u8 s_lawChaosLetters[4] = "CNL";

DATA(0x00068c24)
static u8 s_lightDarkLetters[4] = "DNL";

DATA(0x00068c28)
static char* s_yesNo[2] = {"YES", "NO"};

DATA(0x00068c30)
static i32 s_shownMagnetite = -1;

DATA(0x00068c34)
static i32 s_shownMacca = -1;

DATA(0x00068c38)
static i16 s_shownMoonPhase = -1;

DATA(0x00068c3c)
static i16 s_nextLayout = 1;

// The area and level whose bitmap is unpacked (-1: none).
DATA(0x00068c40)
static i16 s_levelArea = -1;

DATA(0x00068c44)
static i16 s_levelIndex = -1;

DATA(0x00068c48)
static i16 s_itemMenuEquipGroup = -1;

DATA(0x00068c4c)
static i16 s_itemMenuMember = -1;

// The ammunition kind used by the open item menu.
DATA(0x00068c50)
i16 g_itemMenuAmmoType = -1;

// @identity-TODO: no code in this image touches this datum; the object's
// last initialized item is a 0xffff word in a four-byte slot, so its width
// and role are unproven. A reader in the PC-98 build would name it.
DATA(0x00068c54)
static i16 s_unusedItemMenuValue = -1;

// The per-area level tables (256 handles).
DATA(0x0007bee0)
static i32 s_areaStore[MAP_AREA_COUNT] = {0};

DATA(0x0007c2e0)
static i16 s_mapPlane = 0;

DATA(0x0007c2e8)
static MapPosition s_mapPosition = {0};

// The unpacked bitmap of the current level.
DATA(0x0007c2f8)
static AutomapBitmap s_levelBuffer = {0};

// The current area's NPCs (s_npcCount of them placed).
DATA(0x0007d300)
static AreaNpc s_npcs[AREA_NPC_COUNT] = {0};

// The treasure box in view: its cell, and the party's map position with x/y
// set to the view's lateral and depth position. Nothing reads either back.
// @identity-TODO: retail keeps the two adjacent inside one record (cl gives
// no standalone datum their addresses), but nothing references its leading
// bytes or uses it whole, so the record's type is unrecovered.
DATA(0x0007d5b2)
static TreasureBoxCell s_boxCell = {0};

DATA(0x0007d5b4)
static MapPosition s_boxPosition = {0};

DATA(0x0007d5c0)
static ItemStackList* s_itemMenuLimits = NULL;

DATA(0x0007d5c8)
static i32 s_events = 0;

// The yes/no menu of the "analyze in detail?" prompt.
DATA(0x0007d5cc)
static MenuBox* s_menu = NULL;

DATA(0x0007d5d0)
static Character* s_target = NULL;

// The window's step; -1 closes it.
DATA(0x0007d5d4)
static GZ_ENUM_STORAGE(AnalyzeStep, i16) s_step = ANALYZE_STEP_SHOW_NAME;

// Roster entry 15 while the detailed analysis borrows it.
DATA(0x0007d5d8)
static Character* s_savedRosterEntry = NULL;

DATA(0x0007d5dc)
static i16 s_npcCount = 0;

DATA(0x0007d5e0)
static i32* s_areas = NULL;

DATA(0x0007d5e4)
static GZ_ENUM_STORAGE(ViewDirection, i16) s_mapDirection = VIEW_NORTH;

DATA(0x0007d5e8)
static i16 s_mapOriginX = 0;

DATA(0x0007d5ec)
static i16 s_mapOriginY = 0;

DATA(0x0007d5f0)
static i16 s_mapWidth = 0;

DATA(0x0007d5f4)
static i16 s_mapHeight = 0;

DATA(0x0007d5f8)
static i16 s_mapScreenX = 0;

DATA(0x0007d5fc)
static i16 s_mapScreenY = 0;

DATA(0x0007d600)
static b16 s_mapActive = false;

DATA(0x0007d604)
static GZ_ENUM_STORAGE(AutomapDetail, i16) s_mapDetail = AUTOMAP_DETAIL_NONE;

DATA(0x0007d608)
static Panel* s_mapPanel = NULL;

DATA(0x0007d60c)
static AutomapBitmap* s_levelBitmap = NULL;

// The panel being polled (NULL outside a poll).
DATA(0x0007d610)
static Panel* s_activePanel = NULL;

// While set, a row click plays no sound.
DATA(0x0007d614)
static i16 s_panelSilent = 0;

DATA(0x0007d618)
static i32 s_learnableSkillTable = 0;

DATA(0x0007d61c)
static i32 s_learnableSkillRequirements = 0;

DATA(0x0007d620)
static i32 s_itemMenuStock = 0;

DATA(0x0007d624)
static MenuBox* s_itemMenu = NULL;

DATA(0x0007d628)
static b16 s_hideItemMenuTotal = false;

// The room-region grids (a byte per cell of a 64x64 map in a memory handle):
// the current regions and the copy RoomRegionsChanged compares against.
DATA(0x0007d62c)
static i32 s_roomRegions = 0;

DATA(0x0007d630)
static i32 s_prevRegions = 0;

DATA(0x0007d634)
static i16 s_reorderFirst = 0;

DATA(0x0007d638)
static i16 s_reorderSecond = 0;

DATA(0x0007d63c)
static char s_emptyItemLine[1] = "";

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x0001aaa0, 0xa0)
void PrepareViewedTreasureBox(void) {
    TreasureBox* box = FindTreasureBoxAt(g_viewCellX, g_viewCellY, 0);
    if (box) {
        IsTreasureBoxOpen(box);
        GetApproachOffset(g_viewLateral, g_viewDepth);
        s_boxPosition = g_party.field.pos;
        s_boxPosition.x = g_viewLateral;
        s_boxPosition.y = g_viewDepth;
        memcpy(&s_boxCell, &box->head, sizeof(s_boxCell));
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
// @identity-TODO: PC-98 draws the hotspot's closed treasure-box frame after
// computing it here; the Windows build keeps only the frame arithmetic, and
// retail retains its first store.
RVA(0x0001ab40, 0x46)
void IsHotspotTreasureOpen(i32 index) {
    SceneSprite* sprite = GetHotspotSprite(index);
    TreasureBox* box = FindTreasureBoxAt(sprite->cellX, sprite->cellY, 0);
    i16 frame;
    if (box == NULL) {
        return;
    }
    frame = IsTreasureBoxOpen(box);
    if (frame == 1) {
        return;
    }
    frame++;
    switch (box->head.code) {
        case 0x4f:
            frame += 2;
        case TREASURE_BOX_LOWER:
            frame += 2;
        case 0x89:
            frame += 2;
    }
}

RVA(0x0001ab90, 0x168)
b16 RunPartyReorder(void) {
    i16 slot;
    switch (GetGamePhase()) {
        case REORDER_PHASE_OPEN:
            NextGamePhase();
            NextGamePhase();
            break;
        case REORDER_PHASE_CLOSE:
            ReturnFromGameState();
            FlushStatusRedraw(true);
            break;
        case REORDER_PHASE_PICK_FIRST:
            s_reorderFirst = PickReorderSlot();
            if (s_reorderFirst == REORDER_PICK_PENDING) {
                break;
            }
            if (s_reorderFirst < 0) {
                ClearPartySlotSelection();
                PrevGamePhase();
            } else {
                NextGamePhase();
            }
            break;
        case REORDER_PHASE_PICK_SECOND:
            s_reorderSecond = PickReorderSlot();
            if (s_reorderSecond == REORDER_PICK_PENDING) {
                break;
            }
            ClearPartySlotSelection();
            if (s_reorderSecond < 0) {
                PrevGamePhase();
                FlushStatusRedraw(true);
                break;
            }
            ExchangePartySlot(
                s_reorderFirst,
                ExchangePartySlot(s_reorderSecond, GetPartySlot(s_reorderFirst))
            );
            MarkPickDone();
            SetGamePhase(REORDER_PHASE_CLOSE);
            for (s_reorderFirst = 0; s_reorderFirst < PARTY_ROW_SIZE; s_reorderFirst++) {
                if (GetPartySlot(s_reorderFirst) >= 0) {
                    return false;
                }
            }
            for (slot = PARTY_ROW_SIZE; slot < PARTY_SIZE; slot++) {
                s_reorderFirst = GetPartySlot(slot);
                if (s_reorderFirst >= 0) {
                    s_reorderFirst = ExchangePartySlot(slot - PARTY_ROW_SIZE, s_reorderFirst);
                    ExchangePartySlot(slot, s_reorderFirst);
                }
            }
            break;
    }
    return false;
}

RVA(0x0001ad00, 0x39)
i16 PickReorderSlot(void) {
    if (!PollPartySlotSelection(PARTY_SLOT_ANY)) {
        return REORDER_PICK_PENDING;
    }
    if (g_selectedObjectId < 0) {
        return REORDER_PICK_CANCELLED;
    }
    ResetTextPlaneHighlight(g_infoPlane);
    return g_selectedObjectId;
}

static void AnalyzeMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

RVA(0x0001ad40, 0x10)
void SetAnalyzeTarget(Character* target) {
    s_target = target;
}

// Shows the target's name and, when the DAS has its data, its alignment,
// level, HP/MP, attitude and condition; then offers the detailed analysis,
// which shows a copy of the target through roster entry 15 on the status
// screen. Returns -1 once the window is closed, else 0.
RVA(0x0001ad50, 0x5f0)
i16 RunAnalyzeWindow(void) {
    Character* target = s_target;
    GZ_ENUM_LOCAL(TextEvent, i16) choice;

    switch (s_step) {
        case ANALYZE_STEP_CLOSE:
            if (s_menu != NULL) {
                s_menu = DestroyMenuBox(s_menu);
            }
            if (s_dataPlane != TEXT_PLANE_NONE) {
                s_dataPlane = CloseTextWindow(s_dataPlane);
            }
            if (s_namePlane != TEXT_PLANE_NONE) {
                s_namePlane = CloseTextWindow(s_namePlane);
            }
            s_step++;
            return SUBSTATE_FINISHED;

        case ANALYZE_STEP_SHOW_NAME:
            if (target == NULL) {
                return SUBSTATE_FINISHED;
            }
            if (IsEventFlagSet(EVENT_FLAG_BANK_OWNED, OWNED_DAS_V1_0)
                && IsEventFlagSet(EVENT_FLAG_BANK_OWNED, OWNED_DAS_V1_1)
                && IsEventFlagSet(EVENT_FLAG_BANK_OWNED, OWNED_DAS_V2_0)) {
                ShowMessage(
                    "\202c\202`\202r\202\252\203C\203\223\203X\203g\203D\201["
                    "\203\213\202\263\202\352\202\304\202\242\202\334\202\271\202\361",
                    -1
                ); // ＤＡＳがインストゥールされていません
                s_step = ANALYZE_STEP_CLOSE;
                return SUBSTATE_RUNNING;
            }
            s_namePlane = CreateTextPlane(15, 0);
            EraseTextPlaneText(s_namePlane);
            SetTextPlaneCursor(s_namePlane, 0, 0);
            sprintf(
                g_scratchBuffer,
                "%s\201@%s\n", // %s　%s
                GetDemonRaceName(target->id),
                target->namePrefix
            );
            PrintWindowText(s_namePlane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            RepaintTextPlane(s_namePlane, -2);
            s_step++;
            return SUBSTATE_RUNNING;

        case ANALYZE_STEP_SHOW_DATA:
            if (IsEventFlagSet(EVENT_FLAG_BANK_OWNED, OWNED_DAS_V1_1)
                && IsEventFlagSet(EVENT_FLAG_BANK_OWNED, OWNED_DAS_V2_0)) {
                s_step++;
                return SUBSTATE_RUNNING;
            }
            if (!HasAnalyzeData(target->id)) {
                PrintWindowText(
                    s_namePlane,
                    "\203A\203i\203\211\203C\203Y\203f\201[\203^"
                    "\202\252\202\240\202\350\202\334\202\271\202\361\n",
                    TEXT_ATTR_DEFAULT,
                    1,
                    true
                ); // アナライズデータがありません
                s_step++;
                return SUBSTATE_RUNNING;
            }
            s_step++;
            s_dataPlane = CreateTextPlane(16, 0);
            EraseTextPlaneText(s_dataPlane);
            SetTextPlaneCursor(s_dataPlane, 0, 0);
            sprintf(
                g_scratchBuffer,
                "\221\256\220\253\201@ %c/%c\n", // 属性　 %c/%c
                s_lightDarkLetters[GetAlignmentClassA(target) + 1],
                s_lawChaosLetters[GetAlignmentClassB(target) + 1]
            );
            PrintWindowText(s_dataPlane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            sprintf(g_scratchBuffer, "\203\214\203x\203\213 L%2d\n", target->level); // レベル L%2d
            PrintWindowText(s_dataPlane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            sprintf(
                g_scratchBuffer,
                "\202g\202o   %d/%d\n",
                target->pools.hp.cur,
                target->pools.hp.max
            ); // ＨＰ   %d/%d
            PrintWindowText(s_dataPlane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            sprintf(
                g_scratchBuffer,
                "\202l\202o   %d/%d\n",
                target->pools.mp.cur,
                target->pools.mp.max
            ); // ＭＰ   %d/%d
            PrintWindowText(s_dataPlane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            sprintf(
                g_scratchBuffer,
                "\221\324\223x   %s\n",
                s_attitudeNames[target->attitude]
            ); // 態度   %s
            PrintWindowText(s_dataPlane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            sprintf(
                g_scratchBuffer,
                "\217\363\221\324   %s\n",
                GetFirstConditionName(GetCharacterConditions(target))
            ); // 状態   %s
            PrintWindowText(s_dataPlane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            RepaintTextPlane(s_dataPlane, -2);
            return SUBSTATE_RUNNING;

        case ANALYZE_STEP_WAIT:
            if (TakeMouseLeftClick()) {
                s_step++;
                return SUBSTATE_RUNNING;
            }
            if (TakeMouseCancelSound()) {
                s_step = ANALYZE_STEP_CLOSE;
                return SUBSTATE_RUNNING;
            }
            break;

        case ANALYZE_STEP_OFFER_DETAIL:
            if (IsEventFlagSet(EVENT_FLAG_BANK_OWNED, OWNED_DAS_V2_0)
                || !HasAnalyzeData(target->id)) {
                s_step = ANALYZE_STEP_CLOSE;
                return SUBSTATE_RUNNING;
            }
            PrintWindowText(
                s_namePlane,
                "\217\332\215\327\203A\203i\203\211\203C\203Y\202\265\202\334\202\267\202\251\201H"
                "\n",
                TEXT_ATTR_DEFAULT,
                1,
                true
            ); // 詳細アナライズしますか？
            s_menu = CreateMenuBox(s_menu, 16, 2);
            SetMenuItems(s_menu, 5, s_yesNo, 2, AnalyzeMenuHandler);
            SetTextPlaneFirstSelectableRow(s_menu->plane, 0, false);
            SetTextPlaneHighlightMode(s_menu->plane, TEXT_HIGHLIGHT_OUTER);
            s_step++;
            return SUBSTATE_RUNNING;

        case ANALYZE_STEP_RUN_MENU:
            choice = RunMenu(s_menu);
            if (choice == TEXT_EVENT_NONE) {
                break;
            }
            s_menu = DestroyMenuBox(s_menu);
            s_step++;
            if (choice >= TEXT_EVENT_NONE && g_selectedObjectId >= 0) {
                break;
            }
            s_step = ANALYZE_STEP_CLOSE;
            return SUBSTATE_RUNNING;

        case ANALYZE_STEP_SHOW_DETAIL: {
            Character* copy;

            s_step++;
            copy = GetCharacter(ANALYZE_ROSTER_ENTRY);
            // The copy stops short of alignmentA and what follows it.
            memcpy(copy, target, offsetof(Character, alignmentA));
            s_savedRosterEntry = GetRosterEntry(ANALYZE_ROSTER_ENTRY);
            SetRosterEntry(ANALYZE_ROSTER_ENTRY, copy);
            SetStatusAnalyzeMode(true);
            PushGameState(GAME_STATE_STATUS);
            s_dataPlane = CloseTextWindow(s_dataPlane);
            s_namePlane = CloseTextWindow(s_namePlane);
            return SUBSTATE_RUNNING;
        }

        case ANALYZE_STEP_END_DETAIL: {
            Character* copy;

            s_step = ANALYZE_STEP_CLOSE;
            SetStatusAnalyzeMode(false);
            SetRosterEntry(ANALYZE_ROSTER_ENTRY, s_savedRosterEntry);
            s_savedRosterEntry = NULL;
            copy = GetCharacter(ANALYZE_ROSTER_ENTRY);
            InitWordList(GetCharacterSkills(copy), 0);
            break;
        }
    }
    return SUBSTATE_RUNNING;
}

// Lists the yes/no items, and forgets them when the menu is torn down; an
// item's object id is minus its index.
RVA(0x0001b340, 0x50)
static void AnalyzeMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    char** items = menu->items.text;

    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->items.text = NULL;
            menu->itemCount = 0;
            break;
        case MENU_EVENT_ADD_ROW:
            AddMenuLine(
                menu->plane,
                items[index],
                TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_YELLOW, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
                -index,
                MENU_LINE_NORMAL
            );
            break;
    }
}

RVA(0x0001b390, 0x14)
void FreeWorldMapEvents(void) {
    s_events = FreeHandle(s_events);
}

RVA(0x0001b3b0, 0x2f)
void LoadWorldMapEvents(void) {
    FILE* fp;
    FreeWorldMapEvents();
    fp = OpenDataFile(DATA_TABLE_WORLD_EVENTS, DATA_FILE_TABLE, 0);
    s_events = ReadRawHandle(fp);
    CloseDataFile(fp);
}

#define SetMarkedWorldMapEvent(layerValue, xValue, yValue)                                         \
    do {                                                                                           \
        s_markedLayer = (layerValue);                                                              \
        s_markedX = (xValue);                                                                      \
        s_markedY = (yValue);                                                                      \
    } while (0)

RVA(0x0001b3e0, 0x4f)
void MarkWorldMapEventSpot(i16 x, i16 y) {
    i16 block = GetWorldMapBlock(x, y);
    x = GetWorldBlockX(x);
    y = GetWorldBlockY(y);
    SetMarkedWorldMapEvent(block * 2 + IsOddMapLayer(), x, y);
}

RVA(0x0001b430, 0x189)
b16 CheckWorldMapEvent(i16 x, i16 y) {
    i16 block = GetWorldMapBlock(x, y);
    i16 left;
    i16 top;
    i16 marked;
    i16 index;
    WorldMapEventTable* table;
    WorldMapEvent* events;
    x = GetWorldBlockX(x);
    y = GetWorldBlockY(y);
    left = x - 2;
    x += 2;
    top = y - 2;
    y += 2;
    block = block * 2 + IsOddMapLayer();
    table = HandleReadPtr(s_events);
    events = OffsetBy(table, table->offsets[block]);
    marked = false;
    for (index = 0; events[index].x != -1; index++) {
        if (events[index].x >= left && events[index].x <= x && events[index].y >= top
            && events[index].y <= y) {
            if ((events[index].flagBank || events[index].flagIndex)
                && IsEventFlagSet(events[index].flagBank, events[index].flagIndex)) {
                continue;
            }
            if (events[index].x == s_markedX && events[index].y == s_markedY
                && block == s_markedLayer) {
                marked = true;
            } else {
                SetMarkedWorldMapEvent(block, events[index].x, events[index].y);
                SetSceneCell(&events[index]);
                SetSceneScriptByIndex(7, 8);
                return true;
            }
        }
    }
    if (!marked) {
        SetMarkedWorldMapEvent(-1, -1, -1);
    }
    return false;
}

#define InitItemMenuContext(menu, divisor, modeValue, totalVariable)                               \
    do {                                                                                           \
        (menu)->context.item.priceDivisor = (divisor);                                             \
        (menu)->context.item.mode = (modeValue);                                                   \
        (menu)->context.item.totalVar = (totalVariable);                                           \
    } while (0)

RVA(0x0001b5c0, 0x93)
void SetItemMenuCharacter(i16 member) {
    i16 ammo;
    i16 group;
    if (member == CHARACTER_ID_NONE) {
        if (s_itemMenuEquipGroup != -1 && s_itemMenu) {
            RequestMenuRedraw(s_itemMenu);
        }
        s_itemMenuEquipGroup = -1;
        s_itemMenuMember = CHARACTER_ID_NONE;
        g_itemMenuAmmoType = -1;
        return;
    }
    ammo = GetGunAmmoType(GetCharacterById(member));
    s_itemMenuMember = member;
    group = ReadObjectRecordField(member, offsetof(ObjectRecord, equipGroup), sizeof(i16));
    if (s_itemMenu && (s_itemMenuEquipGroup != group || g_itemMenuAmmoType != ammo)) {
        RequestMenuRedraw(s_itemMenu);
    }
    g_itemMenuAmmoType = ammo;
    s_itemMenuEquipGroup = group;
}

RVA(0x0001b660, 0xea)
i16 StepItemBuyMenu(i16* step) {
    switch (*step) {
        case ITEM_MENU_STEP_OPEN: {
            i16 count;
            i16* items = AllocItemMenuStock(GetSceneCellKind(), &count);
            ItemStackList* list = CreateItemMenuEntries(items, count);
            FreeBlock(items);
            s_itemMenu = CreateItemMenu(s_itemMenu, list, count);
            InitItemMenuContext(
                s_itemMenu,
                ITEM_PRICE_DIVISOR_BUY,
                ITEM_MENU_MODE_SHOP,
                ITEM_MENU_TOTAL_VAR
            );
            (*step)++;
            return SUBSTATE_RUNNING;
        }
        case ITEM_MENU_STEP_RUN: {
            i16 result = RunMenu(s_itemMenu);
            if (result == TEXT_EVENT_CANCEL || result == TEXT_EVENT_NONE) {
                return SUBSTATE_RUNNING;
            }
            if (result == TEXT_EVENT_CHOOSE_RIGHT) {
                result = -1;
            }
            AdjustItemMenuCount(s_itemMenu, g_hoveredObjectId, result, ITEM_STACK_MAX);
            return SUBSTATE_RUNNING;
        }
        case ITEM_MENU_STEP_CLOSE:
            s_itemMenu = DestroyMenuBox(s_itemMenu);
            return SUBSTATE_FINISHED;
    }
}

RVA(0x0001b750, 0x6d)
MenuBox* CreateItemMenu(MenuBox* old, ItemStackList* entries, i16 count) {
    MenuBox* menu;
    SetItemMenuCharacter(CHARACTER_ID_NONE);
    menu = CreateMenuBox(old, 0x19, 2);
    MoveMenuBox(menu, -8, -22);
    SetMenuItems(menu, ITEM_MENU_ROWS, entries, count, ItemMenuHandler);
    SetTextPlaneCancelEnabled(menu->plane, 0);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, true);
    menu->list->flags |= 2;
    return menu;
}

RVA(0x0001b7c0, 0x196)
void ItemMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    ItemStackList* list = menu->items.itemList;
    i16 i;
    switch (event) {
        case MENU_EVENT_DESTROY:
            if (menu->context.item.mode != ITEM_MENU_MODE_SCRIPT) {
                ClearPool();
                for (i = 0; i < menu->itemCount; i++) {
                    if (GetItemStackCount(GetItemListEntry(list, i))) {
                        AddToPool(
                            GetItemStackItem(GetItemListEntry(list, i)),
                            GetItemStackCount(GetItemListEntry(list, i))
                        );
                    }
                }
                SetScriptLongVar(
                    ITEM_MENU_TOTAL_VAR,
                    GetItemMenuTotal(list, 1, menu->context.item.priceDivisor)
                );
            }
            s_itemMenuLimits = FreeBlock(s_itemMenuLimits);
            menu->items.itemList = FreeBlock(list);
            menu->itemCount = 0;
            break;
        case MENU_EVENT_ADD_ROW: {
            ItemStack* entry = GetItemListEntry(list, index);
            i32 color = TEXT_ATTR_FLAG1 | TEXT_ATTR_OPAQUE
                        | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_BLACK);
            i32 price = FormatItemMenuEntry(*entry, 1, menu->context.item.priceDivisor);
            ItemRecord* record = GetLoadedRecord(GetItemStackItem(entry));
            if (!GetItemRecordPrice(record)) {
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    TEXT_ATTR_OPAQUE | TEXT_ATTR_FLAG1
                        | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
                    GetItemStackItem(entry),
                    MENU_LINE_DISABLED
                );
            } else {
                if (menu->context.item.mode == ITEM_MENU_MODE_SHOP
                    && menu->context.item.priceDivisor == ITEM_PRICE_DIVISOR_BUY) {
                    if (CompareMacca(-1, price) < 0) {
                        color = TEXT_ATTR_FLAG1 | TEXT_ATTR_OPAQUE
                                | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
                    }
                }
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    color,
                    GetItemStackItem(entry),
                    menu->context.item.mode == ITEM_MENU_MODE_SCRIPT
                );
            }
            break;
        }
        case MENU_EVENT_END_PAGE: {
            i32 total;
            if (menu->context.item.mode == ITEM_MENU_MODE_SCRIPT) {
                total = GetScriptLongVar(menu->context.item.totalVar);
            } else {
                total = GetItemMenuTotal(list, 1, menu->context.item.priceDivisor);
            }
            DrawItemMenuTotal(menu->plane, total, false, index - menu->cursor);
            break;
        }
    }
}

RVA(0x0001b960, 0x1d5)
i32 FormatItemMenuEntry(ItemStack entry, i32 numerator, i32 denominator) {
    char marker = ' ';
    ItemRecord* record = GetLoadedRecord(GetItemStackItem(&entry));
    i16 equipGroup = GetItemEquipCode(record);
    i32 price;
    if (EquipPartOfItem(record) >= 0 && s_itemMenuEquipGroup != -1) {
        if (GetItemCategory(GetItemStackItem(&entry)) == EQUIP_PART_ACCESSORY) {
            Character* member = GetCharacterById(s_itemMenuMember);
            if (member && CanEquipItem(member, GetItemStackItem(&entry)) > 0) {
                marker = 'E';
            }
        } else if (CanGroupEquip(s_itemMenuEquipGroup, equipGroup)) {
            Character* member;
            marker = 'E';
            member = GetCharacterById(s_itemMenuMember);
            record = GetLoadedRecord(GetItemStackItem(&entry));
            if (record->kind == ITEM_KIND_GUN
                && GetBattleStatShown(member, BATTLE_STAT_GUN_LEVEL) > 0) {
                if (LacksItemRequiredStats(
                        member,
                        record,
                        GetBattleStatShown(member, BATTLE_STAT_GUN_LEVEL)
                    )) {
                    marker = 'e';
                }
            } else if (LacksItemRequiredStats(member, record, 0)) {
                marker = 'e';
            }
        }
    }
    record = GetLoadedRecord(GetItemStackItem(&entry));
    price = ScaleItemPrice(GetItemRecordPrice(record), numerator, denominator, 1);
    if (GetItemRecordPrice(record)) {
        if (!GetItemStackCount(&entry)) {
            sprintf(
                g_scratchBuffer,
                "%c %-22.22s %6ld   ",
                marker,
                GetItemRecordName(record),
                price
            );
        } else {
            sprintf(
                g_scratchBuffer,
                "%c %-22.22s %6ldx%2d",
                marker,
                GetItemRecordName(record),
                price,
                GetItemStackCount(&entry)
            );
        }
    } else {
        if (!GetItemStackCount(&entry)) {
            sprintf(g_scratchBuffer, "%c %-22.22s          ", marker, GetItemRecordName(record));
        } else {
            sprintf(g_scratchBuffer, "%c %-22.22s          ", marker, GetItemRecordName(record));
        }
    }
    return price;
}

RVA(0x0001bb40, 0x17)
i32 ScaleItemPrice(i32 price, i32 numerator, i32 denominator, i16 count) {
    return numerator * price / denominator * count;
}

RVA(0x0001bb60, 0x5a)
i32 GetItemMenuTotal(ItemStackList* list, i32 numerator, i32 denominator) {
    i16 i;
    i32 total = 0;
    for (i = 0; i < GetItemListCount(list); i++) {
        ItemRecord* record = GetLoadedRecord(GetItemStackItem(GetItemListEntry(list, i)));
        total += ScaleItemPrice(
            GetItemRecordPrice(record),
            numerator,
            denominator,
            GetItemStackCount(GetItemListEntry(list, i))
        );
    }
    return total;
}

RVA(0x0001bbc0, 0xda)
void DrawItemMenuTotal(i16 plane, i32 total, b16 redraw, i16 line) {
    if (!redraw) {
        if (!s_hideItemMenuTotal) {
            sprintf(g_scratchBuffer, "                \215\207\214\166 %10ld   ", total);
        } else {
            sprintf(g_scratchBuffer, "                \215\207\214\166 ");
            s_hideItemMenuTotal = false;
        }
        for (; line < ITEM_MENU_ROWS; line++) {
            AddMenuLine(
                plane,
                s_emptyItemLine,
                TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
                -1,
                MENU_LINE_DISABLED
            );
        }
        AddMenuLine(
            plane,
            g_scratchBuffer,
            TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
            -1,
            MENU_LINE_DISABLED
        );
    } else {
        sprintf(g_scratchBuffer, "\215\207\214\166 %10ld   ", total);
        SetTextPlaneCursorLine(plane, 16, ITEM_MENU_ROWS);
        PrintWindowText(
            plane,
            g_scratchBuffer,
            TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
            1,
            true
        );
    }
}

RVA(0x0001bca0, 0x10b)
void AdjustItemMenuCount(MenuBox* menu, i16 row, i16 delta, i16 limit) {
    ItemStackList* list = menu->items.itemList;
    i16 index = menu->cursor + row;
    i16 previous = GetItemStackCount(GetItemListEntry(list, index));
    i16 count;
    i16 attr;
    i32 total;
    if (GetItemKind(GetItemStackItem(GetItemListEntry(list, index))) == ITEM_KIND_AMMO) {
        delta *= 10;
    }
    count = AddClampShort(GetItemStackCount(GetItemListEntry(list, index)), delta, 0, limit);
    if (count != previous) {
        SetItemStackCount(GetItemListEntry(list, index), count);
        FormatItemMenuEntry(*GetItemListEntry(list, index), 1, menu->context.item.priceDivisor);
        attr = GetMenuLineAttr(menu->plane, row);
        ResetTextPlaneHighlight(menu->plane);
        SetTextPlaneCursorLine(menu->plane, 0, row);
        SetMenuLineText(menu->plane, row, g_scratchBuffer);
        PrintWindowText(menu->plane, g_scratchBuffer, attr, 1, true);
        total = GetItemMenuTotal(list, 1, menu->context.item.priceDivisor);
        DrawItemMenuTotal(menu->plane, total, true, ITEM_MENU_ROWS);
    }
}

RVA(0x0001bdb0, 0x59)
ItemStackList* CreateItemMenuEntries(i16* items, i16 count) {
    ItemStackList* list =
        AllocCleared(1, offsetof(ItemStackList, entries) + count * sizeof(ItemStack));
    i16 i;
    list->count = count;
    for (i = 0; i < count; i++) {
        GetItemListEntry(list, i)->item = items[i];
        GetItemListEntry(list, i)->count = 0;
    }
    return list;
}

RVA(0x0001be10, 0x60)
b16 RunItemBuyMenu(void) {
    i16 step;
    i16 result;
    switch (GetGamePhase()) {
        case ITEM_MENU_PHASE_ENTER:
            NextGamePhase();
            break;
        case ITEM_MENU_PHASE_RUN:
            step = GetGameStep();
            result = StepItemBuyMenu(&step);
            SetGameStep(step);
            if (result == SUBSTATE_FINISHED) {
                NextGamePhase();
            }
            break;
        case ITEM_MENU_PHASE_RETURN:
            ReturnFromGameState();
            break;
    }
    return false;
}

RVA(0x0001be70, 0x110)
i16 StepItemSellMenu(i16* step) {
    ItemStackList* list;
    switch (*step) {
        case ITEM_MENU_STEP_OPEN: {
            i16 i;
            s_itemMenuLimits = CopyBagEntries(0, BAG_ORDINARY_ENTRY_COUNT, NULL);
            list = CopyBagEntries(0, BAG_ORDINARY_ENTRY_COUNT, NULL);
            for (i = 0; i < GetItemListCount(list); i++) {
                GetItemListEntry(list, i)->count = 0;
            }
            s_itemMenu = CreateItemMenu(s_itemMenu, list, GetItemListCount(list));
            InitItemMenuContext(
                s_itemMenu,
                ITEM_PRICE_DIVISOR_SELL,
                ITEM_MENU_MODE_SHOP,
                ITEM_MENU_TOTAL_VAR
            );
            (*step)++;
            return SUBSTATE_RUNNING;
        }
        case ITEM_MENU_STEP_RUN: {
            i16 result;
            result = RunMenu(s_itemMenu);
            if (result == TEXT_EVENT_CANCEL || result == TEXT_EVENT_NONE) {
                return SUBSTATE_RUNNING;
            }
            if (result == TEXT_EVENT_CHOOSE_RIGHT) {
                result = -1;
            }
            AdjustItemMenuCount(
                s_itemMenu,
                g_hoveredObjectId,
                result,
                GetItemStackCount(
                    GetItemListEntry(s_itemMenuLimits, s_itemMenu->cursor + g_hoveredObjectId)
                )
            );
            return SUBSTATE_RUNNING;
        }
        case ITEM_MENU_STEP_CLOSE:
            s_itemMenu = DestroyMenuBox(s_itemMenu);
            return SUBSTATE_FINISHED;
    }
}

RVA(0x0001bf80, 0x54)
b16 RunItemSellMenu(void) {
    i16 step;
    i16 result;
    switch (GetGamePhase()) {
        case ITEM_MENU_PHASE_ENTER:
            NextGamePhase();
            break;
        case ITEM_MENU_PHASE_RUN:
            step = GetGameStep();
            result = StepItemSellMenu(&step);
            SetGameStep(step);
            if (result == SUBSTATE_FINISHED) {
                NextGamePhase();
            }
            break;
        case ITEM_MENU_PHASE_RETURN:
            ReturnFromGameState();
            break;
    }
    return false;
}

RVA(0x0001bfe0, 0x2a)
void RefreshScriptItemMenuTotal(void) {
    DrawItemMenuTotal(
        s_itemMenu->plane,
        GetScriptLongVar(s_itemMenu->context.item.totalVar),
        true,
        ITEM_MENU_ROWS
    );
}

RVA(0x0001c010, 0x8b)
void OpenScriptItemMenu(i16 totalVar, i16 selling) {
    ItemStack* entries;
    i16 count;
    ItemStackList* list;
    SetItemMenuCharacter(CHARACTER_ID_NONE);
    entries = GetPoolEntries();
    count = CountPoolEntries();
    list = CopyItemMenuEntries(entries, count);
    s_itemMenu = CreateItemMenu(s_itemMenu, list, count);
    InitItemMenuContext(
        s_itemMenu,
        selling ? ITEM_PRICE_DIVISOR_SELL : ITEM_PRICE_DIVISOR_BUY,
        ITEM_MENU_MODE_SCRIPT,
        totalVar
    );
    s_hideItemMenuTotal = true;
    RunMenu(s_itemMenu);
    s_hideItemMenuTotal = false;
    RefreshScriptItemMenuTotal();
}

RVA(0x0001c0a0, 0x3a)
ItemStackList* CopyItemMenuEntries(ItemStack* entries, i16 count) {
    ItemStackList* list =
        AllocCleared(1, offsetof(ItemStackList, entries) + count * sizeof(ItemStack));
    i16 i;
    list->count = count;
    for (i = 0; i < count; i++) {
        *GetItemListEntry(list, i) = entries[i];
    }
    return list;
}

RVA(0x0001c0e0, 0x19)
void PollScriptItemMenu(void) {
    if (s_itemMenu && s_itemMenu->context.item.mode == ITEM_MENU_MODE_SCRIPT) {
        RunMenu(s_itemMenu);
    }
}

RVA(0x0001c100, 0x14)
void CloseItemMenu(void) {
    s_itemMenu = DestroyMenuBox(s_itemMenu);
}

RVA(0x0001c120, 0x34)
i16* GetItemMenuStock(i16 index) {
    ItemMenuStockTable* table = HandleReadPtr(s_itemMenuStock);
    if (index < 0 || index >= table->count) {
        index = 16;
    }
    return OffsetBy(table, table->offsets[index]);
}

RVA(0x0001c160, 0x33)
void LoadItemMenuStock(void) {
    if (s_itemMenuStock == HANDLE_NONE) {
        FILE* fp = OpenDataFile(DATA_TABLE_ITEM_MENU_STOCK, DATA_FILE_TABLE, 0);
        s_itemMenuStock = ReadRawHandle(fp);
        CloseDataFile(fp);
    }
}

RVA(0x0001c1a0, 0x2a)
i16 CountItemMenuStock(i16 index) {
    i16 count = 0;
    i16* items;
    LoadItemMenuStock();
    items = GetItemMenuStock(index);
    while (*items != ITEM_STOCK_END) {
        items++;
        count++;
    }
    return count;
}

RVA(0x0001c1d0, 0x3d)
i16 CopyItemMenuStock(i16 index, i16* items) {
    i16 count = 0;
    i16* stock;
    LoadItemMenuStock();
    stock = GetItemMenuStock(index);
    while (*stock != ITEM_STOCK_END) {
        items[count++] = *stock++;
    }
    return count;
}

RVA(0x0001c210, 0x32)
i16* AllocItemMenuStock(i16 index, i16* count) {
    i16* items;
    *count = CountItemMenuStock(index);
    items = AllocCleared(*count, sizeof(i16));
    CopyItemMenuStock(index, items);
    return items;
}

RVA(0x0001c250, 0x51)
void LoadLearnableSkillTables(void) {
    FILE* fp = OpenDataFile(DATA_TABLE_LEARNABLE_SKILLS, DATA_FILE_TABLE, 0);
    s_learnableSkillTable = ReadRawHandle(fp);
    CloseDataFile(fp);
    fp = OpenDataFile(DATA_TABLE_SKILL_LEARNING_REQUIREMENTS, DATA_FILE_TABLE, 0);
    s_learnableSkillRequirements = ReadRawHandle(fp);
    CloseDataFile(fp);
}

RVA(0x0001c2b0, 0x7f)
i16* GetLearnableSkillList(i16 id, i16 source) {
    i16 key;
    i16 i;
    LearnableSkillTable* table;
    if (id == HUMAN_KATSURAGI) {
        key = GetCharacterAffiliation(GetRosterCharacter(ROSTER_LEADER), source);
        key = -1 - key;
    } else {
        key = FindCharacter(id);
        if (key < 0) {
            return NULL;
        }
    }
    table = HandleReadPtr(s_learnableSkillTable);
    for (i = 0; i < table->count; i++) {
        if (table->entries[i].character == key) {
            return OffsetBy(table, table->entries[i].offset);
        }
    }
    return NULL;
}

RVA(0x0001c330, 0x127)
i16 TakeLearnableSkill(Character* character, i16* skills) {
    i16 id = character->id;
    i16 i;
    i16 skill;
    i16 j;
    for (i = 0; skills[i] != SKILL_LIST_END; i++) {
        if (ContainsWord(GetCharacterSkills(character), skills[i])) {
            continue;
        }
        if (id == HUMAN_KATSURAGI) {
            i16 found = -1;
            LearnableSkillRequirement* requirements = HandleReadPtr(s_learnableSkillRequirements);
            for (j = 0; requirements[j].skill != SKILL_LIST_END; j++) {
                if (requirements[j].skill == skills[i]) {
                    found = j;
                    break;
                }
            }
            if (found != -1 && !MatchFlagWord(&requirements[found].condition)) {
                continue;
            }
        }
        if (RollSkillLearning(character, skills[i])) {
            break;
        }
    }
    skill = skills[i];
    if (skill == SKILL_LIST_END) {
        skills[0] = SKILL_LIST_END;
        return SKILL_LIST_END;
    }
    i++;
    for (j = 0; skills[i + j] != SKILL_LIST_END; j++) {
        skills[j] = skills[i + j];
    }
    skills[j] = SKILL_LIST_END;
    return skill;
}

RVA(0x0001c460, 0x173)
i16 PickGrowthStats(Character* character, i16* picks, i16 turn) {
    memset(picks, -1, AFFILIATION_COUNT * sizeof(i16));
    if (GetCharacterAffiliation(character, 2) >= 0) {
        picks[0] =
            GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), RandomAverage(0, 1, 0));
        picks[1] =
            GetAffiliationGrowthStat(GetCharacterAffiliation(character, 1), RandomAverage(0, 1, 0));
        picks[2] =
            GetAffiliationGrowthStat(GetCharacterAffiliation(character, 2), RandomAverage(0, 1, 0));
        return 3;
    }
    if (GetCharacterAffiliation(character, 1) < 0) {
        if (GetCharacterAffiliation(character, 0) >= 0) {
            picks[0] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), 0);
            picks[1] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), 1);
            return 1;
        }
        return 0;
    }
    if (!(turn & 1)) {
        picks[0] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), 0);
        picks[1] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), 1);
        picks[2] =
            GetAffiliationGrowthStat(GetCharacterAffiliation(character, 1), RandomAverage(0, 1, 0));
        return 2;
    }
    picks[0] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 1), 0);
    picks[1] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 1), 1);
    picks[2] =
        GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), RandomAverage(0, 1, 0));
    return 2;
}

RVA(0x0001c5e0, 0x61)
void DropTopStatPicks(Character* character, i16* picks) {
    i16 i;
    for (i = 0; i < AFFILIATION_COUNT; i++) {
        if (picks[i] >= 0) {
            i16 j;
            for (j = 0; j < STAT_FORTUNE; j++) {
                if (GetBaseStat(character, j) > GetBaseStat(character, picks[i])) {
                    break;
                }
            }
            if (j >= STAT_FORTUNE && RandomAverage(0, 3, 0) == 0) {
                picks[i] = -1;
            }
        }
    }
}

// The training points a level needs: 3 (level - 1)^2 + 7, levels clamped to
// 0..99 (level 0 counting as 1).
RVA(0x0001c650, 0x33)
u32 TrainingThreshold(i16 level) {
    i32 n;
    if (level < 0) {
        n = 0;
    } else if (level > 99) {
        n = 99;
    } else {
        n = level - 1;
    }
    return n * n * 3 + 7;
}

// Adds `amount` to training counter `kind`, capped at level 99's threshold;
// returns the new count.
RVA(0x0001c690, 0x2c)
u32 AddTrainingPointsRaw(
    Character* character,
    GZ_ENUM_PARAM(BattleStatGroup, i16) kind,
    u32 amount
) {
    u32* points = &character->trainingPoints[kind];
    u32 limit;
    amount += *points;
    limit = TrainingThreshold(99);
    amount = min(limit, amount);
    *points = amount;
    return amount;
}

RVA(0x0001c6c0, 0x24)
u32 AddTrainingPoints(Character* character, GZ_ENUM_PARAM(BattleStatGroup, i16) kind, i16 amount) {
    if (kind >= 0 && kind < BATTLE_GROUP_COUNT) {
        return AddTrainingPointsRaw(character, kind, amount);
    }
}

#define RaiseTrainedLevel(level, points, raised)                                                   \
    do {                                                                                           \
        while (TrainingThreshold((level) + 1) <= (points)) {                                       \
            (level)++;                                                                             \
            (raised)++;                                                                            \
        }                                                                                          \
    } while (0)

// Raises training level `kind` (the battle-stat words 0, 6, 12 and 18) while
// its counter covers the next level's threshold; returns the levels gained.
// @identity-TODO: what the four training kinds measure is unrecovered.
RVA(0x0001c6f0, 0x140)
i16 ApplyTraining(Character* character, GZ_ENUM_PARAM(BattleStatGroup, i16) kind) {
    i16 raised = 0;
    switch (kind) {
        case BATTLE_GROUP_WEAPON:
            RaiseTrainedLevel(
                GetBattleStatBase(character, BATTLE_STAT_WEAPON_LEVEL),
                GetTrainingPoints(character, BATTLE_GROUP_WEAPON),
                raised
            );
            break;
        case BATTLE_GROUP_GUN:
            RaiseTrainedLevel(
                GetBattleStatBase(character, BATTLE_STAT_GUN_LEVEL),
                GetTrainingPoints(character, BATTLE_GROUP_GUN),
                raised
            );
            break;
        case BATTLE_GROUP_MAGIC:
            RaiseTrainedLevel(
                GetBattleStatBase(character, BATTLE_STAT_MAGIC_LEVEL),
                GetTrainingPoints(character, BATTLE_GROUP_MAGIC),
                raised
            );
            break;
        case 3:
            RaiseTrainedLevel(
                GetBattleStatBase(character, 18),
                GetTrainingPoints(character, 3),
                raised
            );
            break;
    }
    return raised;
}

// Clamps the three affiliations to 0..3 (-1 otherwise), drops repeats and
// packs the remaining ones to the front.
// @early-stop: retail addresses the affiliation bytes as [character + index];
// the spellings tried give [index + character].
RVA(0x0001c830, 0x9b)
void NormalizeAffiliations(Character* character) {
    i16 i;
    i16 j;
    for (i = 0; i < AFFILIATION_COUNT; i++) {
        if (character->affiliation[i] > BATTLE_GROUP_COUNT - 1 || character->affiliation[i] < 0) {
            SetCharacterAffiliation(character, i, AFFILIATION_NONE);
        }
    }
    for (i = AFFILIATION_COUNT - 1; i > 0; i--) {
        if (character->affiliation[i] != AFFILIATION_NONE) {
            for (j = i - 1; j >= 0; j--) {
                if (character->affiliation[i] == character->affiliation[j]) {
                    SetCharacterAffiliation(character, i, AFFILIATION_NONE);
                }
            }
        }
    }
    for (i = 0; i < AFFILIATION_COUNT - 1; i++) {
        if (character->affiliation[i] < 0) {
            for (j = 0; i + j + 1 < AFFILIATION_COUNT; j++) {
                SetCharacterAffiliation(character, i + j, character->affiliation[i + j + 1]);
            }
            SetCharacterAffiliation(character, i + j, AFFILIATION_NONE);
        }
    }
}

// Applies the training of each of the character's affiliations.
RVA(0x0001c8d0, 0x46)
void RaiseAffiliationLevels(Character* character) {
    if (GetCharacterAffiliation(character, 0) >= 0) {
        ApplyTraining(character, GetCharacterAffiliation(character, 0));
    }
    if (GetCharacterAffiliation(character, 1) >= 0) {
        ApplyTraining(character, GetCharacterAffiliation(character, 1));
    }
    if (GetCharacterAffiliation(character, 2) >= 0) {
        ApplyTraining(character, GetCharacterAffiliation(character, 2));
    }
}

// Codegen constraint: the default joins the signed division with damage
// still zero; an early zero return changes the shared return path.
RVA(0x0001c920, 0xcc)
i32 GetCellTrapDamage(ExitCell* cell, i16 maxHp) {
    i32 damage = 0;
    i16 percent;
    switch (cell->head.code) {
        case CELL_CHUTE:
            SetPendingSound(0x6b);
            if (cell->secondaryDamagePercent < 1) {
                return 0;
            }
            percent = RandomAverage(1, cell->secondaryDamagePercent, 1);
            break;
        case CELL_DAMAGE_TRAP:
            if (cell->trap.damagePercent < 1) {
                return 0;
            }
            percent = RandomAverage(1, cell->trap.damagePercent, 1);
            break;
        case CELL_ALIGNMENT_TRAP_FIRST:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case CELL_ALIGNMENT_TRAP_LAST:
            // Alignment traps reuse the flag-index byte as the damage percentage.
            if (cell->disableFlag[1] < 1) {
                return 0;
            }
            percent = RandomAverage(1, cell->disableFlag[1], 1);
            break;
        default:
            goto done;
    }
    damage = maxHp * percent;
done:
    return damage / 100;
}

RVA(0x0001c9f0, 0xee)
void RunCellTrap(i16 mode, i16 x, i16 y) {
    MapCell cell;
    Character* member;
    i32 damage;
    i16 hp;
    u8 alignmentMask;
    if (mode && CopyExitAt(x, y, &cell.exit)) {
        for (mode = 0; mode < PARTY_SIZE; mode++) {
            member = GetPartyCharacter(mode);
            if (member) {
                damage = GetCellTrapDamage(&cell.exit, member->pools.hp.max);
                hp = member->pools.hp.cur;
                if (cell.exit.head.code >= CELL_ALIGNMENT_TRAP_FIRST
                    && cell.exit.head.code <= CELL_ALIGNMENT_TRAP_LAST) {
                    alignmentMask = 4;
                    alignmentMask >>= GetAlignmentClassB(member) + 1;
                    if (!(cell.exit.trap.alignmentMask & alignmentMask)) {
                        continue;
                    }
                }
                ChangePool(&member->pools.hp, -damage);
                if (hp && !member->pools.hp.cur) {
                    ApplyEmptyPools(member);
                }
            }
        }
        if (cell.exit.head.code == CELL_CHUTE) {
            PlaySoundEffect(0x57);
        } else {
            PlaySoundEffect(0x60);
        }
        FlushStatusRedraw(true);
    }
}

DATA(0x000919f4)
i16 g_panelClickY;

DATA(0x000919f8)
i16 g_panelClickX;

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x0001cae0, 0xc)
void SetPanelSilent(i16 silent) {
    s_panelSilent = silent;
}

RVA(0x0001caf0, 0xe)
b16 IsPanelActive(i16 value) {
    return s_activePanel != NULL;
}

RVA(0x0001cb00, 0x10)
Panel* ExchangeActivePanel(Panel* panel) {
    Panel* old = s_activePanel;
    s_activePanel = panel;
    return old;
}

// Latches a left click (or, when held-button input is enabled, the held button) and its
// position; clears the right-click mark.
RVA(0x0001cb10, 0x65)
b16 CheckPanelLeftClick(Panel* panel) {
    panel->input.rightClick = false;
    g_panelClickX = g_mouseLeftClickX;
    g_panelClickY = g_mouseLeftClickY;
    if (!g_mouseLeftClick) {
        if (!(panel->flags & PANEL_HELD_BUTTON_INPUT)) {
            return false;
        }
        if (!(g_mousePosition.buttons & MOUSE_LEFT_DOWN)) {
            return false;
        }
        g_panelClickX = g_mousePosition.x;
        g_panelClickY = g_mousePosition.y;
    }
    return true;
}

// The same for the right button on panels that allow it;
// marks the panel (bit 0).
RVA(0x0001cb80, 0x7a)
b16 CheckPanelRightClick(Panel* panel) {
    if (!(panel->flags & PANEL_ALLOW_RIGHT_CLICK)) {
        return false;
    }
    if (panel->flags & PANEL_IGNORE_RIGHT_CLICK) {
        return false;
    }
    g_panelClickX = g_mouseRightClickX;
    g_panelClickY = g_mouseRightClickY;
    if (!g_mouseRightClick) {
        if (!(panel->flags & PANEL_HELD_BUTTON_INPUT)) {
            return false;
        }
        if (!(g_mousePosition.buttons & MOUSE_RIGHT_DOWN)) {
            return false;
        }
        g_panelClickX = g_mousePosition.x;
        g_panelClickY = g_mousePosition.y;
    }
    panel->input.rightClick = true;
    return true;
}

// The row whose hotspot x/y hits (skipping hidden or disabled rows, and on a
// right-click the rows that refuse it), or -1.
RVA(0x0001cc00, 0x78)
i16 FindPanelRowAt(Panel* panel, i16 x, i16 y) {
    i16 i;
    u16 flags;
    if (panel->flags & (PANEL_HIDDEN | PANEL_INPUT_DISABLED | 0x0800)) {
        return -1;
    }
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        flags = GetPanelRow(panel, i)->flags;
        if (flags & (PANEL_HIDDEN | PANEL_INPUT_DISABLED | 0x0800)) {
            continue;
        }
        if (WasPanelRightClicked(panel) && (flags & PANEL_IGNORE_RIGHT_CLICK)) {
            continue;
        }
        if (HitTestPanelRow(panel, GetPanelRowId(GetPanelRow(panel, i)), x, y, flags) > 0) {
            return i;
        }
    }
    return -1;
}

// Calls row `row`'s handler with `op` unless the panel or the row is locked
// (PANEL_HANDLER_LOCKED); returns the handler's result, else `row`.
RVA(0x0001cc80, 0x37)
i16 RunPanelRow(Panel* panel, i16 row, i16 op) {
    if (!(panel->flags & PANEL_HANDLER_LOCKED)
        && !(GetPanelRow(panel, row)->flags & PANEL_HANDLER_LOCKED)) {
        row = GetPanelRow(panel, row)->handler(GetPanelRow(panel, row), row, op);
    }
    return row;
}

// Runs the row under the latched click (with the click sound); -1 when none.
RVA(0x0001ccc0, 0x69)
i16 ClickPanel(Panel* panel) {
    i16 row;
    s_activePanel = panel;
    row = FindPanelRowAt(panel, g_panelClickX, g_panelClickY);
    if (row == -1) {
        s_activePanel = NULL;
        return row;
    }
    if (!s_panelSilent) {
        PlaySoundEffect(1);
    }
    row = RunPanelRow(panel, row, -1);
    s_activePanel = NULL;
    return row;
}

RVA(0x0001cd30, 0x42)
i16 PollPanel(Panel* panel) {
    s_activePanel = panel;
    if (!CheckPanelLeftClick(panel) && !CheckPanelRightClick(panel)) {
        s_activePanel = NULL;
        return -1;
    }
    return ClickPanel(panel);
}

RVA(0x0001cd80, 0x5f)
i16 ApplyRowCheck(PanelRow* row, i16 value, GZ_ENUM_PARAM(BitChangeMode, i16) op) {
    i16 result = 0;
    switch (op) {
        case BIT_CHANGE_TOGGLE:
            g_mouseLeftClick = MOUSE_CLICK_NONE;
            result = ToggleFlagBits(&row->flags, PANEL_ROW_CHECKED);
            break;
        case BIT_CHANGE_CLEAR:
            ClearPanelRowCheck(row);
            break;
        case BIT_CHANGE_SET:
            SetFlagBits(&row->flags, PANEL_ROW_CHECKED);
            result = 1;
            break;
    }
    return result;
}

static __inline i32 GetAutomapAreaHandle(i16 area) {
    return s_areas[area];
}

static __inline void EnsureAutomapStore(void) {
    if (s_areas == NULL) {
        memset(s_areaStore, 0, sizeof(s_areaStore));
        s_areas = s_areaStore;
    }
}

RVA(0x0001cde0, 0x2e)
void InitAutomap(void) {
    s_levelBitmap = &s_levelBuffer;
    EnsureAutomapStore();
}

// Makes sure the current area has a bitmap for each of its levels.
RVA(0x0001ce10, 0x12d)
void AllocAutomapLevels(void) {
    i16 area;
    i16 count;
    i32* slot;
    i32 levels;
    i16 level;
    MapCoord size;
    i16 bytes;
    i32 bitmap;
    AutomapBitmap* data;
    EnsureAutomapStore();
    area = GetCurrentArea();
    count = GetAreaLevelCount();
    slot = &s_areas[area];
    levels = *slot;
    if (levels == HANDLE_NONE) {
        levels = CreateArrayHandle(GetAutomapLevelTableSize(count), 1);
        *slot = levels;
        ((AutomapLevels*)HandleWritePtr(levels))->header.count = count;
    }
    for (level = 0; level < count; level++) {
        if (GetAutomapLevelHandle(HandleReadPtr(levels), level) == HANDLE_NONE) {
            size = GetAreaSize(level);
            bytes = ((i16)(size.x * size.y) + 7) / 8;
            bitmap = CreateArrayHandle(bytes + sizeof(AutomapBitmapHeader), 1);
            SetAutomapLevelHandle(HandleWritePtr(levels), level, bitmap);
            data = HandleWritePtr(bitmap);
            data->header.width = size.x;
            data->header.size = bytes;
            data->header.height = size.y;
        }
    }
}

RVA(0x0001cf40, 0x7f)
void FreeAutomap(void) {
    i16 i;
    i32 levels;
    i16 count;
    i16 k;
    if (s_areas == NULL) {
        return;
    }
    for (i = 0; i < MAP_AREA_COUNT; i++) {
        levels = GetAutomapAreaHandle(i);
        if (levels != HANDLE_NONE) {
            count = GetAutomapLevelCount(HandleReadPtr(levels));
            for (k = 0; k < count; k++) {
                FreeHandle(GetAutomapLevelHandle(HandleReadPtr(levels), k));
            }
            FreeHandle(levels);
        }
    }
    s_areas = NULL;
}

// Writes the unpacked level bitmap back to its handle.
RVA(0x0001cfc0, 0x89)
void StoreAutomapLevel(void) {
    AutomapLevels* levels;
    AutomapBitmap* data;
    i32 bitmap;
    u16 size;
    if (s_levelArea >= 0 && s_levelIndex >= 0 && s_areas != NULL
        && GetAutomapAreaHandle(s_levelArea) != HANDLE_NONE) {
        levels = HandleReadPtr(GetAutomapAreaHandle(s_levelArea));
        if (GetAutomapLevelCount(levels) > s_levelIndex) {
            bitmap = GetAutomapLevelHandle(levels, s_levelIndex);
            if (bitmap != HANDLE_NONE) {
                size = GetAutomapBitmapSize(&s_levelBitmap->header);
                data = HandleWritePtr(bitmap);
                memmove(data, s_levelBitmap, size);
            }
        }
    }
    s_levelIndex = s_levelArea = -1;
}

RVA(0x0001d050, 0x88)
void LoadAutomapLevel(i16 area, i16 level) {
    AutomapLevels* levels;
    i32 bitmap;
    AutomapBitmap* data;
    if (s_levelArea == area && s_levelIndex == level) {
        return;
    }
    StoreAutomapLevel();
    if (s_areas == NULL || GetAutomapAreaHandle(area) == HANDLE_NONE) {
        return;
    }
    levels = HandleReadPtr(GetAutomapAreaHandle(area));
    if (GetAutomapLevelCount(levels) <= level) {
        return;
    }
    bitmap = GetAutomapLevelHandle(levels, level);
    if (bitmap == HANDLE_NONE) {
        return;
    }
    data = HandleReadPtr(bitmap);
    memmove(s_levelBitmap, data, (u16)(GetAutomapBitmapSize(&data->header)));
    s_levelArea = area;
    s_levelIndex = level;
}

RVA(0x0001d0e0, 0x3d)
void MarkAutomapCell(i16 area, i16 level, i16 x, i16 y) {
    if (s_levelArea == area && s_levelIndex == level) {
        SetBit(s_levelBitmap->bits, AutomapCellIndex(s_levelBitmap, x, y));
    }
}

// AUTOMAP_CELL_HIDDEN when x/y of `area`/`level` has not been explored (or has no bitmap);
// coordinates wrap at the level size.
RVA(0x0001d120, 0xe1)
i16 IsAutomapCellHidden(i16 x, i16 y, i16 area, i16 level) {
    AutomapLevels* levels;
    AutomapBitmap* data;
    i32 bitmap;
    u8* bits;
    i16 index;
    if (s_levelArea == area && s_levelIndex == level) {
        return TestBit(s_levelBitmap->bits, AutomapCellIndex(s_levelBitmap, x, y)) != true
                   ? AUTOMAP_CELL_HIDDEN
                   : 0;
    } else {
        if (s_areas == NULL) {
            return AUTOMAP_CELL_HIDDEN;
        }
        if (GetAutomapAreaHandle(area) == HANDLE_NONE) {
            return AUTOMAP_CELL_HIDDEN;
        }
        levels = HandleReadPtr(GetAutomapAreaHandle(area));
        if (GetAutomapLevelCount(levels) <= level) {
            return AUTOMAP_CELL_HIDDEN;
        }
        bitmap = GetAutomapLevelHandle(levels, level);
        if (bitmap == HANDLE_NONE) {
            return AUTOMAP_CELL_HIDDEN;
        }
        data = HandleReadPtr(bitmap);
        if (x >= data->header.width) {
            x %= data->header.width;
        }
        if (y >= data->header.height) {
            y %= data->header.height;
        }
        bits = data->bits;
        index = AutomapCellIndex(data, x, y);
    }
    return TestBit(bits, index) != true ? AUTOMAP_CELL_HIDDEN : 0;
}

RVA(0x0001d210, 0xd4)
void RotateAutomapRegion(
    i16 x,
    i16 y,
    GZ_ENUM_PARAM(ViewDirection, i16) direction,
    i16* left,
    i16* top,
    i16* width,
    i16* height
) {
    i16 oldWidth;
    switch (direction) {
        case VIEW_NORTH:
            *left = -x;
            *top = -y;
            break;
        case VIEW_EAST:
            *left = -y;
            *top = x - *width + 1;
            oldWidth = *width;
            *width = *height;
            *height = oldWidth;
            break;
        case VIEW_SOUTH:
            *left = x - *width + 1;
            *top = y - *height + 1;
            break;
        case VIEW_WEST:
            *left = y - *height + 1;
            *top = -x;
            oldWidth = *width;
            *width = *height;
            *height = oldWidth;
            break;
    }
}

RVA(0x0001d2f0, 0x8b)
void DrawAutomapMark(i16 mark, i16 x, i16 y) {
    if (s_mapActive && !IsLevelMapRevealed()) {
        if (IsAutomapCellHidden(x, y, s_mapPosition.area, s_mapPosition.level)) {
            return;
        }
    }
    TransformAutomapPoint(&x, &y);
    if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
        DrawPlaneMapMark(mark, x, y, s_mapPlane);
    }
}

RVA(0x0001d380, 0xb4)
void TransformAutomapPoint(i16* x, i16* y) {
    i16 oldX;
    switch (s_mapDirection) {
        case VIEW_NORTH:
            *x -= s_mapOriginX;
            *y -= s_mapOriginY;
            break;
        case VIEW_EAST:
            oldX = *x;
            *x = *y - s_mapOriginY;
            *y = s_mapOriginX - oldX;
            break;
        case VIEW_SOUTH:
            *x = s_mapOriginX - *x;
            *y = s_mapOriginY - *y;
            break;
        case VIEW_WEST:
            oldX = *x;
            *x = s_mapOriginY - *y;
            *y = oldX - s_mapOriginX;
            break;
    }
}

RVA(0x0001d440, 0xf0)
void MarkMapCell(i16 kind, i16 x, i16 y) {
    if (s_mapActive && !IsLevelMapRevealed()) {
        if (IsAutomapCellHidden(x, y, s_mapPosition.area, s_mapPosition.level)) {
            return;
        }
    }
    TransformAutomapPoint(&x, &y);
    if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
        if (g_party.field.pos.area == MAP_AREA_HATSUDAI && g_party.field.pos.level == 15) {
            if (!g_party.status.navigationFixed && (g_party.field.pos.direction & 1)) {
                x += 3;
                y += 2;
            } else {
                x += 2;
                y += 3;
            }
        }
        DrawMapMark(kind, x, y);
    }
}

RVA(0x0001d530, 0x360)
b16 RunAutomapState(void) {
    i16 savedState;
    i16 input;
    if (TestModeFlags(MODE_WORLD_MAP)) {
        ReturnFromGameState();
        return false;
    }
    SetLayersRenderMode();
    switch (GetGamePhase()) {
        case AUTOMAP_PHASE_OPEN:
            NextGamePhase();
            s_mapActive = true;
            RestoreDrawState(SaveDrawState());
            s_mapPosition = g_party.field.pos;
            s_mapPlane = CreateTextPlane(31, 0);
            s_mapPanel = CreateKindPanel(s_mapPanel, IDB_BITMAP61, 4, 31);
            if (g_party.status.automapFixed) {
                s_mapPosition.direction = VIEW_NORTH;
            }
            s_mapDetail = AUTOMAP_DETAIL_NONE;
            if (!IsEventFlagSet(2, 0x39)) {
                s_mapDetail = AUTOMAP_DETAIL_NPCS;
            }
            if (s_mapDetail < AUTOMAP_DETAIL_BASIC) {
                SetGamePhase(AUTOMAP_PHASE_CLOSE);
            }
            break;
        case AUTOMAP_PHASE_DRAW:
            NextGamePhase();
            savedState = SaveDrawState();
            DrawAutomapViewport(s_mapPosition);
            UpdateAutomapScrollPanel();
            RestoreDrawState(savedState);
            break;
        case AUTOMAP_PHASE_SCROLL:
            input = RunPanelInput(s_mapPanel);
            if (input == PANEL_INPUT_CANCELLED) {
                NextGamePhase();
            } else if (input != PANEL_INPUT_NONE) {
                savedState = SaveDrawState();
                switch (input) {
                    case AUTOMAP_SCROLL_UP:
                        ScrollPlaneMapDown(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, 0, -1);
                        DrawAutomapRegion(s_mapOriginX, s_mapOriginY, s_mapWidth, 1, 0, 0);
                        break;
                    case AUTOMAP_SCROLL_RIGHT:
                        ScrollPlaneMapLeft(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, 1, 0);
                        DrawAutomapRegion(
                            s_mapOriginX,
                            s_mapOriginY,
                            1,
                            s_mapHeight,
                            s_mapWidth - 1,
                            0
                        );
                        break;
                    case AUTOMAP_SCROLL_DOWN:
                        ScrollPlaneMapUp(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, 0, 1);
                        DrawAutomapRegion(
                            s_mapOriginX,
                            s_mapOriginY,
                            s_mapWidth,
                            1,
                            0,
                            s_mapHeight - 1
                        );
                        break;
                    case AUTOMAP_SCROLL_LEFT:
                        ScrollPlaneMapRight(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, -1, 0);
                        DrawAutomapRegion(s_mapOriginX, s_mapOriginY, 1, s_mapHeight, 0, 0);
                        break;
                }
                UpdateAutomapScrollPanel();
                RestoreDrawState(savedState);
            }
            break;
        case AUTOMAP_PHASE_CLOSE:
            CloseTextWindow(s_mapPlane);
            s_mapPanel = ReleasePanel(s_mapPanel, true);
            RequestFieldRefresh();
            RunFieldPanelRow(7, 0, 0, 0);
            ReturnFromGameState();
            s_mapActive = false;
            break;
    }
    return false;
}

RVA(0x0001d890, 0x1ec)
void UpdateAutomapScrollPanel(void) {
    i16 width;
    i16 height;
    GZ_ENUM_LOCAL(AutomapScrollBlock, i16) blocked;
    ClearPanelChecksAgain(s_mapPanel);
    GetMapSize(&width, &height);
    blocked = AUTOMAP_BLOCK_NONE;
    switch (s_mapDirection) {
        case VIEW_NORTH:
            if (s_mapOriginX == 0) {
                blocked |= AUTOMAP_BLOCK_LEFT;
            }
            if (s_mapOriginX + s_mapWidth >= width) {
                blocked |= AUTOMAP_BLOCK_RIGHT;
            }
            if (s_mapOriginY == 0) {
                blocked |= AUTOMAP_BLOCK_UP;
            }
            if (s_mapOriginY + s_mapHeight >= height) {
                blocked |= AUTOMAP_BLOCK_DOWN;
            }
            break;
        case VIEW_EAST:
            if (s_mapOriginY == 0) {
                blocked |= AUTOMAP_BLOCK_LEFT;
            }
            if (s_mapOriginY + s_mapWidth >= height) {
                blocked |= AUTOMAP_BLOCK_RIGHT;
            }
            if (s_mapOriginX == width - 1) {
                blocked |= AUTOMAP_BLOCK_UP;
            }
            if (s_mapOriginX - s_mapHeight < 0) {
                blocked |= AUTOMAP_BLOCK_DOWN;
            }
            break;
        case VIEW_SOUTH:
            if (s_mapOriginX == width - 1) {
                blocked |= AUTOMAP_BLOCK_LEFT;
            }
            if (s_mapOriginX - s_mapWidth < 0) {
                blocked |= AUTOMAP_BLOCK_RIGHT;
            }
            if (s_mapOriginY == height - 1) {
                blocked |= AUTOMAP_BLOCK_UP;
            }
            if (s_mapOriginY - s_mapHeight < 0) {
                blocked |= AUTOMAP_BLOCK_DOWN;
            }
            break;
        case VIEW_WEST:
            if (s_mapOriginY == height - 1) {
                blocked |= AUTOMAP_BLOCK_LEFT;
            }
            if (s_mapOriginY - s_mapWidth < 0) {
                blocked |= AUTOMAP_BLOCK_RIGHT;
            }
            if (s_mapOriginX == 0) {
                blocked |= AUTOMAP_BLOCK_UP;
            }
            if (s_mapOriginX + s_mapHeight == width) {
                blocked |= AUTOMAP_BLOCK_DOWN;
            }
            break;
    }
    SetPanelRowFlags(s_mapPanel, AUTOMAP_SCROLL_LEFT, PANEL_HIDDEN, blocked & AUTOMAP_BLOCK_LEFT);
    SetPanelRowFlags(s_mapPanel, AUTOMAP_SCROLL_RIGHT, PANEL_HIDDEN, blocked & AUTOMAP_BLOCK_RIGHT);
    SetPanelRowFlags(s_mapPanel, AUTOMAP_SCROLL_UP, PANEL_HIDDEN, blocked & AUTOMAP_BLOCK_UP);
    SetPanelRowFlags(s_mapPanel, AUTOMAP_SCROLL_DOWN, PANEL_HIDDEN, blocked & AUTOMAP_BLOCK_DOWN);
    PaintPanel(s_mapPanel, s_mapPlane);
}

RVA(0x0001da80, 0x110)
void DrawAutomapRegion(i16 x, i16 y, i16 width, i16 height, i16 across, i16 along) {
    i16 row;
    i16 column;
    i16 tile;
    MapCoord cell;
    OffsetMapCoord(&x, &y, s_mapDirection, across, along);
    for (row = 0; row < height; row++) {
        for (column = 0; column < width; column++) {
            cell.x = x;
            cell.y = y;
            cell = MoveMapCoord(cell, s_mapDirection, column, row);
            tile = GetRotatedWallAtOffset(cell.x, cell.y, s_mapDirection, 0, 0);
            tile = GetWallStopCode(tile, WALL_STOP_MOVEMENT);
            DrawAutomapTile(tile, cell.x, cell.y);
        }
    }
    IsCellBlocked(s_mapPosition.level, CELL_SCAN_DRAW_ICONS, 0, 0);
    if (s_mapPosition.level == g_party.field.pos.level) {
        DrawAutomapMark(
            TurnDirection(g_party.field.pos.direction, -s_mapDirection),
            g_party.field.pos.x,
            g_party.field.pos.y
        );
    }
}

RVA(0x0001db90, 0xc1)
void DrawAutomapTile(i16 tile, i16 x, i16 y) {
    i16 mapX = x;
    i16 mapY = y;
    if (s_mapActive) {
        TransformAutomapPoint(&x, &y);
        if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
            if (!IsAutomapCellHidden(mapX, mapY, s_mapPosition.area, s_mapPosition.level)) {
                DrawPlaneMapTileLit(tile, x, y, s_mapPlane);
            } else if (IsLevelMapRevealed()) {
                DrawPlaneMapTile(tile, x, y, s_mapPlane);
            }
        }
    }
}

RVA(0x0001dc60, 0x1e0)
b16 DrawAutomapViewport(MapPosition position) {
    i16 width;
    i16 height;
    i16 left;
    i16 top;
    i16 viewWidth;
    i16 viewHeight;
    i16 x;
    i16 y;
    GZ_ENUM_LOCAL(ViewDirection, i16) direction;
    GetMapSize(&width, &height);
    x = position.x;
    y = position.y;
    direction = position.direction;
    RotateAutomapRegion(x, y, direction, &left, &top, &width, &height);
    if (width <= AUTOMAP_VIEW_WIDTH) {
        viewWidth = width;
    } else {
        viewWidth = AUTOMAP_VIEW_WIDTH;
        if (-left * 2 > AUTOMAP_VIEW_WIDTH) {
            left = -(AUTOMAP_VIEW_WIDTH / 2);
            switch (direction) {
                case VIEW_NORTH:
                    if (x + AUTOMAP_VIEW_WIDTH / 2 > width) {
                        left = width - x - AUTOMAP_VIEW_WIDTH;
                    }
                    break;
                case VIEW_EAST:
                    if (y + AUTOMAP_VIEW_WIDTH / 2 > width) {
                        left = width - y - AUTOMAP_VIEW_WIDTH;
                    }
                    break;
                case VIEW_SOUTH:
                    if (x - (AUTOMAP_VIEW_WIDTH / 2 - 1) < 0) {
                        left = x - (AUTOMAP_VIEW_WIDTH - 1);
                    }
                    break;
                case VIEW_WEST:
                    if (y - (AUTOMAP_VIEW_WIDTH / 2 - 1) < 0) {
                        left = y - (AUTOMAP_VIEW_WIDTH - 1);
                    }
                    break;
            }
        }
    }
    if (height <= AUTOMAP_VIEW_HEIGHT) {
        viewHeight = height;
    } else {
        viewHeight = AUTOMAP_VIEW_HEIGHT;
        if (-top * 2 > AUTOMAP_VIEW_HEIGHT) {
            top = -(AUTOMAP_VIEW_HEIGHT / 2);
            switch (direction) {
                case VIEW_NORTH:
                    if (y + AUTOMAP_VIEW_HEIGHT / 2 > height) {
                        top = height - y - AUTOMAP_VIEW_HEIGHT;
                    }
                    break;
                case VIEW_EAST:
                    if (x - (AUTOMAP_VIEW_HEIGHT / 2 - 1) < 0) {
                        top = x - (AUTOMAP_VIEW_HEIGHT - 1);
                    }
                    break;
                case VIEW_SOUTH:
                    if (y - (AUTOMAP_VIEW_HEIGHT / 2 - 1) < 0) {
                        top = y - (AUTOMAP_VIEW_HEIGHT - 1);
                    }
                    break;
                case VIEW_WEST:
                    if (x + AUTOMAP_VIEW_HEIGHT / 2 > height) {
                        top = height - x - AUTOMAP_VIEW_HEIGHT;
                    }
                    break;
            }
        }
    }
    s_mapDirection = direction;
    s_mapWidth = viewWidth;
    s_mapHeight = viewHeight;
    s_mapOriginX = x;
    s_mapOriginY = y;
    OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, direction, left, top);
    DrawAutomapRegion(s_mapOriginX, s_mapOriginY, viewWidth, viewHeight, 0, 0);
    return false;
}

RVA(0x0001de40, 0x3d0)
void DrawMapOverlay(MapPosition position) {
    i16 width, height;
    i16 left, top;
    i16 viewWidth, viewHeight;
    i16 x, y, direction;
    i16 screenX;
    i16 screenY;
    i16 row, column;
    i16 tile;
    i16 edge;
    MapCoord cell;
    if (TestModeFlags(MODE_WORLD_MAP)) {
        return;
    }
    ClearLayerSurface(SCREEN_LAYER_AUTOMAP);
    if (g_party.status.navigationFixed) {
        position.direction = VIEW_NORTH;
    }
    s_mapDetail = AUTOMAP_DETAIL_NONE;
    if (!IsEventFlagSet(EVENT_FLAG_BANK_OWNED, OWNED_AMS_V1_0)) {
        s_mapDetail = AUTOMAP_DETAIL_BASIC;
    }
    if (!IsEventFlagSet(EVENT_FLAG_BANK_OWNED, OWNED_AMS_V2_0)) {
        s_mapDetail = AUTOMAP_DETAIL_NPCS;
    }
    if (!IsEventFlagSet(2, 0x38)) {
        s_mapDetail = AUTOMAP_DETAIL_OBJECTS;
    }
    if (s_mapDetail < AUTOMAP_DETAIL_BASIC) {
        return;
    }
    if (IsDarkCell(g_party.field.pos.x, g_party.field.pos.y)) {
        return;
    }
    GetMapSize(&width, &height);
    x = position.x;
    y = position.y;
    direction = position.direction;
    RotateAutomapRegion(x, y, direction, &left, &top, &width, &height);
    if (width <= MAP_OVERLAY_SIZE) {
        viewWidth = width;
        screenX = MAP_OVERLAY_X + MAP_OVERLAY_SIZE - width;
    } else {
        screenX = MAP_OVERLAY_X;
        viewWidth = MAP_OVERLAY_SIZE;
        if (-left * 2 > MAP_OVERLAY_SIZE) {
            left = -(MAP_OVERLAY_SIZE / 2);
            switch (position.direction) {
                case VIEW_NORTH:
                    if (position.x + MAP_OVERLAY_SIZE / 2 + 1 > width) {
                        left = width - x - MAP_OVERLAY_SIZE;
                    }
                    break;
                case VIEW_EAST:
                    if (position.y + MAP_OVERLAY_SIZE / 2 + 1 > width) {
                        left = width - y - MAP_OVERLAY_SIZE;
                    }
                    break;
                case VIEW_SOUTH:
                    edge = x - MAP_OVERLAY_SIZE / 2;
                    if (edge < 0) {
                        left = x - (MAP_OVERLAY_SIZE - 1);
                    }
                    break;
                case VIEW_WEST:
                    edge = y - MAP_OVERLAY_SIZE / 2;
                    if (edge < 0) {
                        left = y - (MAP_OVERLAY_SIZE - 1);
                    }
                    break;
            }
        }
    }
    if (height <= MAP_OVERLAY_SIZE) {
        viewHeight = height;
        screenY = MAP_OVERLAY_Y + MAP_OVERLAY_SIZE - height;
    } else {
        screenY = MAP_OVERLAY_Y;
        viewHeight = MAP_OVERLAY_SIZE;
        edge = -top * 2;
        if (edge > MAP_OVERLAY_SIZE) {
            top = -(MAP_OVERLAY_SIZE / 2);
            switch (position.direction) {
                case VIEW_NORTH:
                    if (position.y + MAP_OVERLAY_SIZE / 2 + 1 > height) {
                        top = height - y - MAP_OVERLAY_SIZE;
                    }
                    break;
                case VIEW_EAST:
                    edge = x - MAP_OVERLAY_SIZE / 2;
                    if (edge < 0) {
                        top = x - (MAP_OVERLAY_SIZE - 1);
                    }
                    break;
                case VIEW_SOUTH:
                    edge = y - MAP_OVERLAY_SIZE / 2;
                    if (edge < 0) {
                        top = y - (MAP_OVERLAY_SIZE - 1);
                    }
                    break;
                case VIEW_WEST:
                    if (position.x + MAP_OVERLAY_SIZE / 2 + 1 > height) {
                        top = height - x - MAP_OVERLAY_SIZE;
                    }
                    break;
            }
        }
    }
    s_mapDirection = position.direction;
    s_mapWidth = viewWidth;
    s_mapHeight = viewHeight;
    s_mapScreenX = screenX;
    s_mapScreenY = screenY * 8;
    s_mapOriginX = position.x;
    s_mapOriginY = position.y;
    OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, direction, left, top);
    for (row = 0; row < viewHeight; row++) {
        for (column = 0; column < viewWidth; column++) {
            cell.x = position.x;
            cell.y = position.y;
            cell = MoveMapCoord(cell, direction, column + left, row + top);
            tile =
                GetRotatedWallAtOffset(position.x, position.y, direction, column + left, row + top);
            tile = GetWallStopCode(tile, WALL_STOP_GEOMETRY);
            DrawMapOverlayTile(tile, cell.x, cell.y);
        }
    }
    IsCellBlocked(g_party.field.pos.level, CELL_SCAN_DRAW_ICONS, 0, 0);
    if (s_mapDetail >= AUTOMAP_DETAIL_NPCS) {
        MarkAreaNpcs();
    }
    if (s_mapDetail >= AUTOMAP_DETAIL_OBJECTS) {
        MarkObjectsOnMap();
    }
    MarkMapCell(
        TurnDirection(g_party.field.pos.direction, -direction),
        g_party.field.pos.x,
        g_party.field.pos.y
    );
}

RVA(0x0001e210, 0xf0)
void DrawMapOverlayTile(i16 tile, i16 x, i16 y) {
    if (s_mapActive && !IsLevelMapRevealed()) {
        if (IsAutomapCellHidden(x, y, s_mapPosition.area, s_mapPosition.level)) {
            return;
        }
    }
    TransformAutomapPoint(&x, &y);
    if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
        if (g_party.field.pos.area == MAP_AREA_HATSUDAI && g_party.field.pos.level == 15) {
            if (!g_party.status.navigationFixed && (g_party.field.pos.direction & 1)) {
                x += 3;
                y += 2;
            } else {
                x += 2;
                y += 3;
            }
        }
        DrawMapTile(tile, x, y);
    }
}

RVA(0x0001e300, 0x50)
b16 IsCellInView(i16 x, i16 y) {
    TransformAutomapPoint(&x, &y);
    if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
        return true;
    }
    return false;
}

RVA(0x0001e350, 0x70)
void DrawAutomapCellIcon(u8 code, i16 x, i16 y) {
    i16 index = 0;
    while (s_mapIcons[index].code < code) {
        index++;
    }
    if (s_mapIcons[index].code == code && s_mapDetail >= s_mapIcons[index].detail) {
        if (GetRenderMode() == RENDER_MODE_LAYERS) {
            DrawAutomapMark(s_mapIcons[index].mark, x, y);
        } else {
            MarkMapCell(s_mapIcons[index].mark, x, y);
        }
    }
}

RVA(0x0001e3c0, 0x160)
i16 WriteAutomapAreas(FILE* fp) {
    i16 errors = 0;
    i16 area;
    i16 level;
    i16 count;
    i16 bytes;
    i32 handle;
    i32 bitmap;
    AutomapLevels* levels;
    AutomapBitmap* data;
    memset(g_scratchBuffer, 0, 256);
    if (!s_areas) {
        for (area = 0; area < 4; area++) {
            errors += 256 - fwrite(g_scratchBuffer, 1, 256, fp);
        }
        return errors;
    }
    StoreAutomapLevel();
    errors = MAP_AREA_COUNT - fwrite(s_areas, 4, MAP_AREA_COUNT, fp);
    for (area = 0; area < MAP_AREA_COUNT; area++) {
        handle = GetAutomapAreaHandle(area);
        if (handle) {
            levels = HandleReadPtr(handle);
            count = GetAutomapLevelCount(levels);
            bytes = GetAutomapLevelTableSize(count);
            errors += bytes - fwrite(levels, 1, bytes, fp);
            for (level = 0; level < count; level++) {
                levels = HandleReadPtr(handle);
                bitmap = GetAutomapLevelHandle(levels, level);
                if (bitmap) {
                    data = HandleReadPtr(bitmap);
                    bytes = GetAutomapBitmapSize(&data->header);
                    errors += bytes - fwrite(data, 1, bytes, fp);
                }
            }
        }
    }
    LoadAutomapLevel(g_party.field.pos.area, g_party.field.pos.level);
    return errors;
}

RVA(0x0001e520, 0x1d0)
i16 LoadAutomapAreas(FILE* fp) {
    i16 errors;
    i16 area;
    i16 level;
    i16 count;
    i16 bytes;
    i32 handle;
    i32 bitmap;
    AutomapLevelHeader levelHeader;
    AutomapBitmapHeader bitmapHeader;
    AutomapLevels* levels;
    AutomapBitmap* data;
    StoreAutomapLevel();
    FreeAutomap();
    EnsureAutomapStore();
    errors = MAP_AREA_COUNT - fread(s_areas, 4, MAP_AREA_COUNT, fp);
    if (errors) {
        return errors;
    }
    for (area = 0; area < MAP_AREA_COUNT; area++) {
        if (GetAutomapAreaHandle(area)) {
            errors += 1 - fread(&levelHeader, 4, 1, fp);
            count = levelHeader.count;
            handle = CreateArrayHandle(GetAutomapLevelTableSize(count), 1);
            s_areas[area] = handle;
            levels = HandleWritePtr(handle);
            levels->header = levelHeader;
            errors += count - fread(levels->levels, 4, count, fp);
            for (level = 0; level < count; level++) {
                levels = HandleWritePtr(handle);
                if (GetAutomapLevelHandle(levels, level)) {
                    errors += 1 - fread(&bitmapHeader, 8, 1, fp);
                    bitmap = CreateArrayHandle(GetAutomapBitmapSize(&bitmapHeader), 1);
                    data = HandleWritePtr(bitmap);
                    data->header = bitmapHeader;
                    bytes = data->header.size;
                    errors += bytes - fread(data->bits, 1, bytes, fp);
                    levels = HandleWritePtr(handle);
                    SetAutomapLevelHandle(levels, level, bitmap);
                }
            }
        }
    }
    return errors;
}

RVA(0x0001e6f0, 0x2a)
void DrawFieldView(void) {
    DrawMapOverlay(g_party.field.pos);
}

RVA(0x0001e720, 0x12)
i16 SetInfoBarLayout(i16 layout) {
    i16 previous = s_nextLayout;
    s_nextLayout = layout;
    return previous;
}

RVA(0x0001e740, 0x4b)
void DrawMoneyCounters(i16 mode) {
    DrawMoneyCounter(mode, 8, RosterMemberAt(ROSTER_LEADER)->magnetite, 0);
    s_shownMagnetite = RosterMemberAt(ROSTER_LEADER)->magnetite;
    DrawMoneyCounter(mode, 11, RosterMemberAt(ROSTER_LEADER)->macca, 1);
    s_shownMacca = RosterMemberAt(ROSTER_LEADER)->macca;
}

RVA(0x0001e790, 0xcf)
void DrawMoneyCounter(i16 mode, i16 row, i32 value, i16 currency) {
    i32 attr = 0xffffb400;
    if (!currency) {
        DrawLayerText(
            SCREEN_LAYER_CURRENCY,
            8,
            8,
            "       ",
            TEXT_ATTR_OPAQUE | TEXT_ATTR_FLAG1 | TEXT_ATTR_HALF_WIDTH
                | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
        );
        if (value < 1) {
            attr = 0xffffb500;
        }
        sprintf(g_scratchBuffer, "%7ld", value);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 8, 8, g_scratchBuffer, attr);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 72, 8, "MAG", attr);
    } else {
        DrawLayerText(
            SCREEN_LAYER_CURRENCY,
            40,
            32,
            "       ",
            TEXT_ATTR_OPAQUE | TEXT_ATTR_FLAG1 | TEXT_ATTR_HALF_WIDTH
                | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
        );
        if (value < 1) {
            attr = 0xffffb500;
        }
        sprintf(g_scratchBuffer, "%7ld", value);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 8, 32, "\\", attr);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 40, 32, g_scratchBuffer, attr);
    }
}

RVA(0x0001e860, 0x81)
b16 DrawInfoBar(i16 layout, i16 partial) {
    DrawIconLayerImage(g_clock.moonPhase);
    sprintf(g_scratchBuffer, "%2d", g_clock.moonPhase + 1);
    DrawLayerText(
        SCREEN_LAYER_MOON_PHASE,
        8,
        8,
        g_scratchBuffer,
        TEXT_ATTR_OPAQUE | TEXT_ATTR_FLAG1 | TEXT_ATTR_HALF_WIDTH
            | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
    s_shownMoonPhase = g_clock.moonPhase;
    DrawMoneyCounters(layout);
    if (!partial || TestModeFlags(MODE_WORLD_MAP)) {
        DrawAreaInfo();
    }
    return false;
}

RVA(0x0001e8f0, 0xf5)
void DrawAreaInfo(void) {
    i16 floor;
    i16 x;
    ClearLocationCaption();
    if (!TestModeFlags(MODE_WORLD_MAP)) {
        strcpy(g_scratchBuffer, GetAreaName());
    } else {
        FormatWorldMapLocation();
    }
    DrawLayerText(
        SCREEN_LAYER_LOCATION,
        8,
        8,
        g_scratchBuffer,
        TEXT_ATTR_OPAQUE | TEXT_ATTR_FLAG1
            | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
    floor = GetLevelFloor();
    DrawLayerText(
        SCREEN_LAYER_LOCATION,
        176,
        8,
        "    ",
        TEXT_ATTR_OPAQUE | TEXT_ATTR_FLAG1 | TEXT_ATTR_HALF_WIDTH
            | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
    if (floor) {
        x = 176;
        if (floor < 0) {
            sprintf(g_scratchBuffer, "B%1dF", -floor);
            if (floor > -10) {
                x = 184;
            }
        } else {
            sprintf(g_scratchBuffer, " %2dF", floor);
        }
        DrawLayerText(
            SCREEN_LAYER_LOCATION,
            x,
            8,
            g_scratchBuffer,
            TEXT_ATTR_OPAQUE | TEXT_ATTR_FLAG1 | TEXT_ATTR_HALF_WIDTH
                | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
        );
    }
}

RVA(0x0001e9f0, 0x60)
b16 RefreshInfoBar(i16 force) {
    if (force) {
        DrawInfoBar(s_nextLayout, true);
    } else if (s_shownMoonPhase != g_clock.moonPhase
               || s_shownMagnetite != RosterMemberAt(ROSTER_LEADER)->magnetite
               || s_shownMacca != RosterMemberAt(ROSTER_LEADER)->macca) {
        DrawInfoBar(s_nextLayout, true);
    }
    s_nextLayout = 1;
    return false;
}

RVA(0x0001ea50, 0x34)
b16 UpdateInfoBar(void) {
    if (g_fieldRedrawRequest) {
        DrawInfoBar(0, false);
        return false;
    }
    if (g_tickElapsed >= CLOCK_UPDATE_MOON) {
        DrawInfoBar(1, false);
    }
    return false;
}

static __inline void EnsureGridByteStorage(i32* grid) {
    if (!*grid) {
        *grid = AllocHandle(REGION_GRID_SIZE * REGION_GRID_SIZE);
    }
}

static __inline i16 GridByteIndex(i16 x, i16 y) {
    return y * REGION_GRID_SIZE + x;
}

// Sets region `value` on every cell of the rectangle x0..x1, y0..y1.
RVA(0x0001ea90, 0x3f)
void FillRegionRect(i16 x0, i16 y0, i16 x1, i16 y1, u8 value) {
    i16 x;
    i16 y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            SetRoomRegion(x, y, value);
        }
    }
}

RVA(0x0001ead0, 0x1d)
void SetRoomRegion(i16 x, i16 y, u8 value) {
    SetGridByte(&s_roomRegions, x, y, value);
}

// Sets cell x/y of a region grid (allocated on first use).
RVA(0x0001eaf0, 0x3d)
void SetGridByte(i32* grid, i16 x, i16 y, u8 value) {
    u8* bytes;
    EnsureGridByteStorage(grid);
    bytes = HandleWritePtr(*grid);
    bytes[GridByteIndex(x, y)] = value;
}

// Gives region `value` to every cell of the w x h map still without one.
RVA(0x0001eb30, 0x49)
void FillEmptyRegions(i16 width, i16 height, u8 value) {
    i16 x;
    i16 y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (GetRoomRegion(x, y) == REGION_NONE) {
                SetRoomRegion(x, y, value);
            }
        }
    }
}

RVA(0x0001eb80, 0x18)
u8 GetRoomRegion(i16 x, i16 y) {
    return GetGridByte(&s_roomRegions, x, y);
}

RVA(0x0001eba0, 0x39)
u8 GetGridByte(i32* grid, i16 x, i16 y) {
    u8* bytes;
    EnsureGridByteStorage(grid);
    bytes = HandleReadPtr(*grid);
    return bytes[GridByteIndex(x, y)];
}

// Whether the flag pair at `offset` of an entry is "on": its flag set, or
// clear for a pair whose bank byte (at offset + 2) is nonzero.
// @identity-TODO: the meaning of the nonzero byte is unrecovered.
RVA(0x0001ebe0, 0x2e)
b16 IsRegionFlagOn(u8* list, i16 offset) {
    b16 invert = list[offset + 2] != 0;
    return (IsCellFlagSet((CellHead*)list, offset) != false) ^ invert;
}

// Marks the regions of a level's room list: entries of `stride` bytes (a
// rectangle x0, y0, x1, y1 then a flag pair) fill their rectangle, marker
// entries (0xff, then a flag pair; stride - 3 bytes) fill the cells left; an
// entry whose flag is on is skipped. Regions are numbered from `code`; the
// list ends with 0xff 0xff.
RVA(0x0001ec10, 0xad)
void MarkRegionList(u8* list, i16 stride, u8 code, i16 width, i16 height) {
    i16 offset = 0;
    for (;; code++) {
        u8* entry = &list[offset];
        if (list[offset] == 0xff && entry[1] == 0xff) {
            return;
        }
        if (list[offset] != 0xff) {
            if (!IsRegionFlagOn(list, offset + 4)) {
                FillRegionRect(entry[0], entry[1], entry[2], entry[3], code);
            }
            offset += stride;
        } else {
            if (!IsRegionFlagOn(list, offset + 1)) {
                FillEmptyRegions(width, height, code);
            }
            offset += stride - 3;
        }
    }
}

// The data of entry `index` of a room list (after its rectangle or marker),
// NULL past the end.
// @early-stop: the returned address is formed as [offset + list] in retail
// and [list + offset] here.
RVA(0x0001ecc0, 0x60)
u8* FindRegionData(u8* list, i16 stride, i16 index) {
    i16 offset = 0;
    i16 i = 0;
    for (;;) {
        if (list[offset] == 0xff && list[offset + 1] == 0xff) {
            return NULL;
        }
        if (list[offset] != 0xff) {
            if (i == index) {
                return offset + list + 4;
            }
            offset += stride;
        } else {
            if (i == index) {
                return offset + list + 1;
            }
            offset += stride - 3;
        }
        i++;
    }
}

// Clears the region grid, then marks the level's rooms and doors.
RVA(0x0001ed20, 0x4b)
void MarkRoomRegions(u8* rooms, u8* doors, i16 width, i16 height) {
    FillRegionRect(0, 0, REGION_GRID_SIZE - 1, REGION_GRID_SIZE - 1, REGION_NONE);
    MarkRegionList(rooms, ROOM_ENTRY_SIZE, 0, width, height);
    MarkRegionList(doors, DOOR_ENTRY_SIZE, REGION_DOOR, width, height);
}

static __inline void SaveRoomRegions(i16 width, i16 height) {
    i16 x;
    i16 y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            SetPrevRegion(x, y, GetRoomRegion(x, y));
        }
    }
}

static __inline i16 RestoreRoomRegions(i16 width, i16 height) {
    i16 x;
    i16 y;
    i16 changed = 0;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            changed |= GetRoomRegion(x, y) - GetPrevRegion(x, y);
            SetRoomRegion(x, y, GetPrevRegion(x, y));
        }
    }
    return changed;
}

// Whether re-marking the rooms would change any cell's region (the grid is
// left as it was).
// @early-stop TU state: retail reads the room region before the previous one;
// cl 5.0 orders these two equal-cost calls by translation-unit symbol state.
RVA(0x0001ed70, 0xb9)
i16 RoomRegionsChanged(u8* rooms, u8* doors, i16 width, i16 height) {
    SaveRoomRegions(width, height);
    MarkRoomRegions(rooms, doors, width, height);
    return RestoreRoomRegions(width, height);
}

RVA(0x0001ee30, 0x1d)
void SetPrevRegion(i16 x, i16 y, u8 value) {
    SetGridByte(&s_prevRegions, x, y, value);
}

RVA(0x0001ee50, 0x18)
u8 GetPrevRegion(i16 x, i16 y) {
    return GetGridByte(&s_prevRegions, x, y);
}

// The region (room or door) of map cell x/y, wrapped into the map.
RVA(0x0001ee70, 0x4f)
i16 GetMapCellCode(i16 x, i16 y) {
    i16 width;
    i16 height;
    GetMapSize(&width, &height);
    x = WrapMapCoord(x, width);
    return GetRoomRegion(x, WrapMapCoord(y, height));
}

RVA(0x0001eec0, 0x18)
i16 GetPartyCellCode(void) {
    return GetMapCellCode(g_party.field.pos.x, g_party.field.pos.y);
}

// Whether a region code is a door.
RVA(0x0001eee0, 0xa)
i16 IsObjectCell(i16 code) {
    return code & REGION_DOOR;
}

RVA(0x0001eef0, 0x1a)
i16 GetCellSpawnRate(i16 code) {
    DoorRegionData* table = GetCellObjectTable(code);
    if (!table) {
        return 0;
    }
    return table->spawnInterval;
}

RVA(0x0001ef10, 0x19)
i16 CellCodeDiffers(i16 code, i16 x, i16 y) {
    return GetMapCellCode(x, y) - code;
}

RVA(0x0001ef30, 0x24)
void ResetFieldScene(void) {
    ReleaseNpcTextures();
    ResetFieldLayer(FIELD_LAYER_SECOND);
    ResetFieldLayer(FIELD_LAYER_FIRST);
    SetCurrentRoomCode(-1);
}

// The object of layer 0 or 1 a door's data names (0x20 for none).
RVA(0x0001ef60, 0x26)
i16 LookupCellObject(DoorRegionData* table, i16 layer) {
    if (table && layer >= 0 && layer <= 1) {
        return table->objects[layer];
    }
    return 0x20;
}

RVA(0x0001ef90, 0x1e)
u8* GetRoomData(i16 code) {
    return FindRegionData(
        GetLevelList(LEVEL_LIST_ROOMS),
        ROOM_ENTRY_SIZE,
        code & (REGION_DOOR - 1)
    );
}

RVA(0x0001efb0, 0x1e)
DoorRegionData* GetCellObjectTable(i16 code) {
    void* data =
        FindRegionData(GetLevelList(LEVEL_LIST_DOORS), DOOR_ENTRY_SIZE, code & (REGION_DOOR - 1));
    return data;
}

// Enters region `code`: a room loads its NPC images, a door its two enemy
// groups (object record kinds).
RVA(0x0001efd0, 0x95)
void EnterRoom(i16 code) {
    DoorRegionData* table;
    i16 object;
    SetCurrentRoomCode(code);
    if (code == REGION_NONE || code == -1) {
        return;
    }
    if (!IsObjectCell(code)) {
        u8* data = GetRoomData(code);
        if (data) {
            LoadAreaNpcImages(data);
        }
        return;
    }
    table = GetCellObjectTable(code);
    if (!table) {
        return;
    }
    object = LookupCellObject(table, 0);
    if (object >= HUMAN_ID_LIMIT && object <= OBJECT_KIND_END - 1) {
        LoadEnemyGroupSlot(FIELD_LAYER_FIRST, object);
    }
    object = LookupCellObject(table, 1);
    if (object >= HUMAN_ID_LIMIT && object <= OBJECT_KIND_END - 1) {
        LoadEnemyGroupSlot(FIELD_LAYER_SECOND, object);
    }
}

// Re-enters the party's region when it changed (resetting the field and
// respawning), else re-picks the two special pictures of area 0x85 level 3.
RVA(0x0001f070, 0xb6)
void UpdateCurrentRoom(void) {
    i16 code;
    if (!CellCodeDiffers(GetCurrentRoomCode(), g_party.field.pos.x, g_party.field.pos.y)) {
        if (g_party.field.pos.area == 0x85 && g_party.field.pos.level == 3
            && g_party.field.pos.x == 3) {
            if (g_party.field.pos.y == 4) {
                LoadNpcTexture(0, 0x53, 0);
            } else if (g_party.field.pos.y == 5) {
                LoadNpcTexture(0, 0x4c, 0);
            }
        }
        return;
    }
    RequestFieldRefresh();
    ResetFieldObjects();
    ResetFieldScene();
    code = GetMapCellCode(g_party.field.pos.x, g_party.field.pos.y);
    EnterRoom(code);
    SpawnMapObjects(code);
}

// After a step through a door: when the cell behind the party is in another
// region, resets the field and enters that region.
RVA(0x0001f130, 0x99)
void FinishDoorStep(void) {
    i16 x = g_party.field.pos.x;
    i16 y = g_party.field.pos.y;
    i16 direction = TurnDirection(g_party.field.pos.direction, g_party.field.moveCommand);
    StepMapCoordBy(&x, &y, direction, 0, -1);
    if (CellCodeDiffers(GetCurrentRoomCode(), x, y)) {
        i16 code;
        ResetFieldObjects();
        ResetFieldScene();
        code = GetMapCellCode(x, y);
        EnterRoom(code);
        SpawnMapObjects(code);
    }
}

// Which of the two objects of the cell at x/y is object `id`: 0 or 1, else -1
// (also when the cell holds no objects).
RVA(0x0001f1d0, 0x75)
i16 FindCellObject(i16 id, i16 x, i16 y) {
    i16 code = GetMapCellCode(x, y);
    DoorRegionData* table;
    if (!IsObjectCell(code)) {
        return -1;
    }
    table = GetCellObjectTable(code);
    if (!table) {
        return -1;
    }
    if (LookupCellObject(table, 0) == id) {
        return 0;
    }
    return LookupCellObject(table, 1) != id ? -1 : 1;
}

// The slot the next NPC takes, -1 when all AREA_NPC_COUNT are placed.
RVA(0x0001f250, 0x11)
i16 NextNpcSlot(void) {
    if (s_npcCount >= AREA_NPC_COUNT) {
        return -1;
    }
    return s_npcCount;
}

RVA(0x0001f270, 0x12)
void CountPlacedNpc(void) {
    if (s_npcCount < AREA_NPC_COUNT) {
        s_npcCount++;
    }
}

// The picture of NPC code `code`.
RVA(0x0001f290, 0xe)
i16 GetNpcImageOfCode(i16 code) {
    // Retail indexes the physical recovery list from its fifth entry.
    return g_physicalRecoveryConditions[code + 4];
}

RVA(0x0001f2a0, 0xa)
void ClearAreaNpcs(void) {
    s_npcCount = 0;
}

// Places an NPC from its map record: cell x/y, picture code, event flag
// (bank, index), scene script (file, entry) and texture slot.
RVA(0x0001f2b0, 0x93)
void AddAreaNpc(const u8* record) {
    i16 slot = NextNpcSlot();
    if (slot < 0) {
        return;
    }
    s_npcs[slot].x = *record++;
    s_npcs[slot].y = *record++;
    s_npcs[slot].image = GetNpcImageOfCode(*record++);
    s_npcs[slot].flagBank = *record++;
    s_npcs[slot].flagIndex = *record++;
    s_npcs[slot].script = *record++;
    s_npcs[slot].entry = *record;
    s_npcs[slot].textureSlot = record[1];
    CountPlacedNpc();
}

RVA(0x0001f350, 0x18)
b16 IsReservedObjectCell(const CellHead* cell) {
    if (cell->code >= 0x48 && cell->code <= 0x4e) {
        return true;
    }
    return false;
}

// Draws the NPCs still present on the view cell being drawn (with the
// approach offset of the view position).
RVA(0x0001f370, 0xad)
void DrawAreaNpcs(void) {
    i16 i;
    MapCoord offset;
    for (i = 0; i < s_npcCount; i++) {
        if (IsEventFlagSet(s_npcs[i].flagBank, s_npcs[i].flagIndex)) {
            continue;
        }
        if (s_npcs[i].x != g_viewCellX || s_npcs[i].y != g_viewCellY) {
            continue;
        }
        offset = GetApproachOffset(g_viewLateral, g_viewDepth);
        DrawNpcAt(offset.x, offset.y, g_viewDepth, &s_npcs[i], i);
    }
}

RVA(0x0001f420, 0x7)
i16 GetAreaNpcCount(void) {
    return s_npcCount;
}

RVA(0x0001f430, 0x2a)
b32 IsAreaNpcGone(i16 npc) {
    return IsEventFlagSet(s_npcs[npc].flagBank, s_npcs[npc].flagIndex);
}

RVA(0x0001f460, 0x17)
i16* GetAreaNpcCell(i16 npc) {
    return &s_npcs[npc].x;
}

RVA(0x0001f480, 0x18)
i16 GetAreaNpcTextureSlot(i16 npc) {
    return s_npcs[npc].textureSlot;
}

RVA(0x0001f4a0, 0x17)
AreaNpc* GetAreaNpc(i16 npc) {
    return &s_npcs[npc];
}

// Marks every NPC still present on the automap (kind 4).
RVA(0x0001f4c0, 0x5f)
void MarkAreaNpcs(void) {
    i16 i;
    for (i = 0; i < s_npcCount; i++) {
        if (!IsEventFlagSet(s_npcs[i].flagBank, s_npcs[i].flagIndex)) {
            MarkMapCell(MAP_MARK_NPC, s_npcs[i].x, s_npcs[i].y);
        }
    }
}

RVA(0x0001f520, 0x1)
void NotifyEncounterStart(void) {}

RVA(0x0001f530, 0x1)
void NotifyEncounterEnd(void) {}

RVA(0x0001f540, 0x5)
void ReleaseNpcTextures(void) {
    ReleaseObjectTextures();
}

// Loads NPC picture `code` (image 0x4000 + code; `mode` bit 7 picks the
// variant) into object texture slot `slot`. Two spots swap the picture: code
// 0x2b at area 0x82 level 8 cell 11/7 or 12/6 shows 0x24, and code 0x53 at
// area 0x85 level 3 south of row 4 shows 0x4c.
// @identity-TODO: why those spots swap pictures is unrecovered.
RVA(0x0001f550, 0xd0)
void LoadNpcTexture(i16 slot, i16 code, i16 mode) {
    ImageRequest request;
    void* image;
    if (code == 0x2b && mode == 0 && g_party.field.pos.area == MAP_AREA_HATSUDAI
        && g_party.field.pos.level == 8) {
        if ((g_party.field.pos.x == 0xb && g_party.field.pos.y == 7)
            || (g_party.field.pos.x == 0xc && g_party.field.pos.y == 6)) {
            code = 0x24;
        }
    } else if (code == 0x53 && mode == 0 && g_party.field.pos.area == 0x85
               && g_party.field.pos.level == 3 && g_party.field.pos.y > 4) {
        code = 0x4c;
    }
    request.file = code + 0x4000;
    request.variant = 0;
    request.flags = ((u32)(mode & 0x80) >> 7) + 3;
    image = LoadImageRequest(&request, mode);
    LoadObjectTexture(image, slot);
    FreeImageFile(image);
}

// Sets palette colours 8..13 from six GRB words; returns the data after them.
RVA(0x0001f620, 0x32)
u16* LoadNpcPalette(u16* colors) {
    i16 i;
    for (i = 0; i < 6; i++) {
        SetPaletteColor(i + 8, GrbToRgb(*colors));
        colors++;
    }
    return colors;
}

// Loads the area's NPC palette and pictures from its record (the six
// (code, mode) pairs after the palette; mode 0xff leaves a slot empty).
RVA(0x0001f660, 0x41)
void LoadAreaNpcImages(u8* record) {
    u8* p;
    i16 i;
    if (!record) {
        return;
    }
    p = (u8*)LoadNpcPalette((u16*)(record + 3));
    for (i = 0; i < NPC_TEXTURE_SLOTS; i++) {
        if (p[1] != 0xff) {
            LoadNpcTexture(i, p[0], p[1]);
        }
        p += 2;
    }
}

// The NPC texture in `slot` (below NPC_TEXTURE_SLOTS), else 0.
RVA(0x0001f6b0, 0x1e)
u32 GetNpcTexture(i16 slot) {
    if (slot >= 0 && slot < NPC_TEXTURE_SLOTS) {
        return g_npcTextures[slot].texture;
    }
    return 0;
}

// The texture an NPC is drawn with at depth `depth` (only 0 and -1 draw it).
// @identity-TODO: at other depths nothing is returned (retail leaves eax as
// it was); x, y and `index` are unread.
RVA(0x0001f6d0, 0x22)
u32 DrawNpcAt(i16 x, i16 y, i16 depth, AreaNpc* npc, i16 index) {
    if (depth == 0 || depth == -1) {
        return GetNpcTexture(npc->textureSlot);
    }
}

// Runs the field effect of a skill or item: 1 knocks the target back, 3
// shields it, 0x11 and 0x14 set flags, 0x15/0x16 return to the leader's
// recorded point or mark, 0x17 knocks the actor back, 0x19/0x1a spawn a second
// group, 0x1b seals a demon, 0x20..0x22 set stat flags, 0x23 does nothing but
// succeed. Returns the handler's result.
// @identity-TODO: the handlers are named from their bodies only.
RVA(0x0001f700, 0xcc)
GZ_ENUM_RETURN(FieldEffectResult, i16) RunFieldEffect(i16 effect) {
    switch (effect) {
        case 0:
            break;
        case 1:
            return KnockBack(g_targetId);
        case 3:
            return ShieldTarget();
        case 0x11:
            return SetTargetFlag21();
        case 0x14:
            return ScatterObjects();
        case 0x15:
            return ReturnToLeaderWarp();
        case 0x16:
            return ReturnToLeaderMark();
        case 0x17:
            return KnockBackActor();
        case 0x19:
        case 0x1a:
            return SpawnActorGroup();
        case 0x1b:
            return SealTarget();
        case 0x20:
            return RaiseTargetFlag23();
        case 0x21:
            return RaiseTargetFlag25();
        case 0x22:
            return RaiseTargetFlag26();
        case 0x23:
            return FIELD_EFFECT_DONE;
    }
    return FIELD_EFFECT_NONE;
}

// Pushes `who` (a field object, or the party for a negative id) one cell
// back (the party: behind itself; an object: away from the party), unless a
// wall, a map-cell change or a blocked cell stops it; -1 then.
RVA(0x0001f7d0, 0x152)
GZ_ENUM_RETURN(FieldEffectResult, i16) KnockBack(i16 who) {
    i16* at;
    i16 direction;
    if (who < 0) {
        direction = g_party.field.pos.direction;
        at = &g_party.field.pos.x;
    } else {
        i16 object;
        i16 code;
        i16 x;
        i16 y;
        Character* actor;
        direction = OppositeDirection(g_party.field.pos.direction);
        object = GetLiveObject(who);
        if (object < 0) {
            return FIELD_EFFECT_FAILED;
        }
        actor = GetFieldActor(object);
        at = &((FieldActor*)GetFieldActor(object))->pos.x;
        if (TestCharacterFlag(actor, 0x20)) {
            return FIELD_EFFECT_FAILED;
        }
        code = GetMapCellCode(at[0], at[1]);
        x = at[0];
        y = at[1];
        StepMapCoord(&x, &y, direction, MOVE_BACK);
        WrapMapPosition(&x, &y);
        if (CellCodeDiffers(code, x, y)) {
            return FIELD_EFFECT_FAILED;
        }
        if (IsCellBlocked(g_party.field.pos.level, CELL_SCAN_TEST, x, y)) {
            return FIELD_EFFECT_FAILED;
        }
    }
    if (WallStopsToward(at[0], at[1], direction, MOVE_BACK)) {
        return FIELD_EFFECT_FAILED;
    }
    StepMapCoord(&at[0], &at[1], direction, MOVE_BACK);
    RefreshFieldScene();
    return FIELD_EFFECT_DONE;
}

// Gives the target a shield of a tenth of its maximum HP (a field object:
// respawns it instead).
RVA(0x0001f930, 0x56)
GZ_ENUM_RETURN(FieldEffectResult, i16) ShieldTarget(void) {
    Character* target;
    if (g_targetId < 0) {
        target = GetCombatant(g_targetId);
        if (!target) {
            return FIELD_EFFECT_FAILED;
        }
        target->shield = target->pools.hp.max / 10;
        return FIELD_EFFECT_DONE;
    }
    return RespawnFieldObject(g_targetId, false, FIELD_OBJECT_NO_EVENT, true);
}

RVA(0x0001f990, 0x2d)
GZ_ENUM_RETURN(FieldEffectResult, i16) SetTargetFlag21(void) {
    Character* target = GetCombatant(g_targetId);
    if (!target) {
        return FIELD_EFFECT_FAILED;
    }
    SetCharacterFlag(target, 0x21);
    return FIELD_EFFECT_DONE;
}

// Sets the leader's flag 0x22 and clears ACTOR_FLAG_NOTICED of every live
// object out of reach.
RVA(0x0001f9c0, 0x67)
b16 ScatterObjects(void) {
    i16 i;
    u8* flags = GetCharacterFlags(GetRosterCharacter(ROSTER_LEADER));
    SetBit(flags, 0x22);
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        i16 object = GetLiveObject(i);
        if (object >= 0 && !HasObjectInReach(1, -1, object)) {
            flags = GetCharacterFlags(GetCombatant(object));
            ClearBit(flags, ACTOR_FLAG_NOTICED);
        }
    }
    return true;
}

// Sends the party to the return point recorded in the roster leader.
RVA(0x0001fa30, 0x35)
b16 ReturnToLeaderWarp(void) {
    Character* leader = GetRosterCharacter(ROSTER_LEADER);
    SetReturnPoint(
        leader->returnPosition.area,
        leader->returnPosition.level,
        leader->returnPosition.x,
        leader->returnPosition.y,
        leader->returnPosition.direction
    );
    return true;
}

// Sends the party to the cell in front of the leader's marked position.
RVA(0x0001fa70, 0x6d)
b16 ReturnToLeaderMark(void) {
    Character* leader = GetRosterCharacter(ROSTER_LEADER);
    i16 area = leader->markPosition.area;
    i16 level = leader->markPosition.level;
    i16 x = leader->markPosition.x;
    i16 y = leader->markPosition.y;
    i16 direction = OppositeDirection(leader->markPosition.direction);
    OffsetMapCoordFacing(&x, &y, direction, 0, -1);
    SetReturnPoint(area, level, x, y, direction);
    return true;
}

// Knocks the acting object back; when nothing is left within reach, raises
// the pending abort.
RVA(0x0001fae0, 0x44)
GZ_ENUM_RETURN(FieldEffectResult, i16) KnockBackActor(void) {
    GZ_ENUM_LOCAL(FieldEffectResult, i16) result;
    if (g_actorId >= 0) {
        return FIELD_EFFECT_FAILED;
    }
    result = KnockBack(g_actorId);
    if (result < 0) {
        return result;
    }
    if (HasObjectInReach(0, -1, 0)) {
        return FIELD_EFFECT_NONE;
    }
    ExchangeAbortPending(true);
    return FIELD_EFFECT_DONE;
}

// Spawns a second enemy group at the acting object's cell.
RVA(0x0001fb30, 0x42)
GZ_ENUM_RETURN(FieldEffectResult, i16) SpawnActorGroup(void) {
    i16 object = GetLiveObject(g_actorId);
    MapCoord* pos;
    if (object < 0) {
        return FIELD_EFFECT_FAILED;
    }
    pos = &((FieldActor*)GetFieldActor(object))->pos;
    SpawnSecondGroupActor(pos->x, pos->y, -1);
}

// Seals the target (flag 0x3f, result 3) unless it is human or of a race from
// RACE_MAJIN to RACE_INU; while the field marker is set against an object the
// action fails with result 6.
RVA(0x0001fb80, 0xe4)
GZ_ENUM_RETURN(FieldEffectResult, i16) SealTarget(void) {
    Character* actor = GetCombatant(g_actorId);
    Character* target;
    GZ_ENUM_LOCAL(DemonRace, i16) race;
    if (g_targetId >= 0 && GetFieldMarker()) {
        SetActionResult(actor, 6);
        return FIELD_EFFECT_NONE;
    }
    target = GetCombatant(g_targetId);
    if (!target) {
        return FIELD_EFFECT_FAILED;
    }
    if (IsHumanCharacter(target)) {
        return FIELD_EFFECT_NONE;
    }
    race = GetDemonRace(target->id);
    if (race == RACE_MAJIN || race == RACE_DEMONOID || race == RACE_JUUJIN
        || race == RACE_ISHTAR_BELIEVER || race == RACE_BAEL_BELIEVER || race == RACE_KYOUJIN
        || race == RACE_HEISHI || race == RACE_HITO || race == RACE_INU) {
        return FIELD_EFFECT_NONE;
    }
    SetActionResult(actor, 3);
    SetCharacterFlag(target, 0x3f);
    return FIELD_EFFECT_DONE;
}

// Sets the target's flag 0x23 (not with 0x23 or 0x24 already set) and
// recalculates its stats.
RVA(0x0001fc70, 0x6d)
GZ_ENUM_RETURN(FieldEffectResult, i16) RaiseTargetFlag23(void) {
    Character* target = GetCombatant(g_targetId);
    if (!target) {
        return FIELD_EFFECT_FAILED;
    }
    if (TestCharacterFlag(target, 0x23) == true) {
        return FIELD_EFFECT_FAILED;
    }
    if (TestCharacterFlag(target, 0x24) == true) {
        return FIELD_EFFECT_FAILED;
    }
    SetCharacterFlag(target, 0x23);
    RecalcCharacterStats(target);
    return FIELD_EFFECT_DONE;
}

// The same with flag 0x25, only while the moon is not new.
RVA(0x0001fce0, 0x67)
GZ_ENUM_RETURN(FieldEffectResult, i16) RaiseTargetFlag25(void) {
    Character* target;
    if (!GetMoonPhase()) {
        return FIELD_EFFECT_FAILED;
    }
    target = GetCombatant(g_targetId);
    if (!target) {
        return FIELD_EFFECT_FAILED;
    }
    if (TestCharacterFlag(target, 0x25) == true) {
        return FIELD_EFFECT_FAILED;
    }
    SetCharacterFlag(target, 0x25);
    RecalcCharacterStats(target);
    return FIELD_EFFECT_DONE;
}

RVA(0x0001fd50, 0x67)
GZ_ENUM_RETURN(FieldEffectResult, i16) RaiseTargetFlag26(void) {
    Character* target;
    if (!GetMoonPhase()) {
        return FIELD_EFFECT_FAILED;
    }
    target = GetCombatant(g_targetId);
    if (!target) {
        return FIELD_EFFECT_FAILED;
    }
    if (TestCharacterFlag(target, 0x26) == true) {
        return FIELD_EFFECT_FAILED;
    }
    SetCharacterFlag(target, 0x26);
    RecalcCharacterStats(target);
    return FIELD_EFFECT_DONE;
}
