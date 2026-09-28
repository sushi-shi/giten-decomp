// @identity-TODO: the owning TU is unproven. One retail object: the .bss
// statics of the treasure box, party reorder, analyze window, world-map
// events, item-list menus, training, panel input, automap bitmaps and area
// NPC code interleave in one run (0x47bee0..0x47d63f), each read only by its
// own part's code; their initialized data, the info bar's included, forms
// one .data run out of .text order (0x468b30..0x468c51) ahead of one run of
// literals; and the code, from the treasure box through the area NPCs, is
// contiguous in .text.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Alignment.h>
#include <Game/Analyze.h>
#include <Game/AnalyzeData.h>
#include <Game/AreaLevel.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
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
#include <Game/InfoBar.h>
#include <Game/ItemMenu.h>
#include <Game/ItemRecord.h>
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
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
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

DATA(0x00064618)
const i16 g_affiliationGrowthStats[4][2] = {{5, 7}, {8, 0}, {2, 1}, {3, 9}};

DATA(0x00068b30)
static AutomapIcon s_mapIcons[] = {
    {0x40, 6, AUTOMAP_DETAIL_BASIC},  {0x41, 7, AUTOMAP_DETAIL_BASIC},
    {0x42, 8, AUTOMAP_DETAIL_BASIC},  {0x43, 9, AUTOMAP_DETAIL_BASIC},
    {0x44, 10, AUTOMAP_DETAIL_BASIC}, {0x45, 10, AUTOMAP_DETAIL_BASIC},
    {0x46, 10, AUTOMAP_DETAIL_BASIC}, {0x48, 11, AUTOMAP_DETAIL_NPCS},
    {0x50, 12, AUTOMAP_DETAIL_NPCS},  {0x51, 13, AUTOMAP_DETAIL_NPCS},
    {0x52, 14, AUTOMAP_DETAIL_NPCS},  {0x53, 15, AUTOMAP_DETAIL_NPCS},
    {0x54, 16, AUTOMAP_DETAIL_NPCS},  {0x55, 17, AUTOMAP_DETAIL_NPCS},
    {0x56, 18, AUTOMAP_DETAIL_NPCS},  {0x57, 19, AUTOMAP_DETAIL_NPCS},
    {0x58, 20, AUTOMAP_DETAIL_NPCS},  {0x59, 21, AUTOMAP_DETAIL_NPCS},
    {0x5b, 22, AUTOMAP_DETAIL_NPCS},  {0x7b, 23, AUTOMAP_DETAIL_NPCS},
    {0x7d, 9, AUTOMAP_DETAIL_NPCS},   {0x85, 18, AUTOMAP_DETAIL_NPCS},
    {0x86, 18, AUTOMAP_DETAIL_NPCS},  {0x87, 18, AUTOMAP_DETAIL_NPCS},
    {0x90, 8, AUTOMAP_DETAIL_BASIC},  {0x91, 9, AUTOMAP_DETAIL_BASIC},
    {0xbf, 24, AUTOMAP_DETAIL_BASIC}, {0xff, 24, AUTOMAP_DETAIL_BASIC},
};

DATA(0x00068b8c)
NpcTexture g_npcTextures[6] = {
    {0, 0xffff},
    {0, 0xffff},
    {0, 0xffff},
    {0, 0xffff},
    {0, 0xffff},
    {0, 0xffff},
};

// The five attitude names, indexed by Character.attitude.
DATA(0x00068bf8)
static char* s_attitudeNames[5] = {
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
static i16 s_namePlane = -1;

// The window with the target's analyze data.
DATA(0x00068c1c)
static i16 s_dataPlane = -1;

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

// The per-area level tables (256 handles).
DATA(0x0007bee0)
static i32 s_areaStore[256] = {0};

DATA(0x0007c2e0)
static i16 s_mapPlane = 0;

DATA(0x0007c2e8)
static MapPosition s_mapPosition = {0};

// The unpacked bitmap of the current level.
DATA(0x0007c2f8)
static AutomapBitmap s_levelBuffer = {0};

// The current area's NPCs (s_npcCount of them placed).
DATA(0x0007d300)
static AreaNpc s_npcs[16] = {0};

DATA(0x0007d5b2)
static TreasureBoxCell s_boxCell = {0};

DATA(0x0007d5b4)
static MapPosition s_boxPosition = {0};

DATA(0x0007d5c0)
static ItemStackList* s_itemMenuLimits = 0;

DATA(0x0007d5c8)
static i32 s_events = 0;

// The yes/no menu of the "analyze in detail?" prompt.
DATA(0x0007d5cc)
static MenuBox* s_menu = 0;

DATA(0x0007d5d0)
static Character* s_target = 0;

// The window's step; -1 closes it.
DATA(0x0007d5d4)
static i16 s_step = 0;

// Roster entry 15 while the detailed analysis borrows it.
DATA(0x0007d5d8)
static Character* s_savedRosterEntry = 0;

DATA(0x0007d5dc)
static i16 s_npcCount = 0;

DATA(0x0007d5e0)
static i32* s_areas = 0;

DATA(0x0007d5e4)
static i16 s_mapDirection = 0;

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
static GZ_ENUM_STORAGE(AutomapDetail, i16) s_mapDetail = 0;

DATA(0x0007d608)
static Panel* s_mapPanel = 0;

DATA(0x0007d60c)
static AutomapBitmap* s_levelBitmap = 0;

// The panel being polled (NULL outside a poll).
DATA(0x0007d610)
static Panel* s_activePanel = 0;

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
static MenuBox* s_itemMenu = 0;

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
        s_boxPosition = g_field.pos;
        s_boxPosition.x = g_viewLateral;
        s_boxPosition.y = g_viewDepth;
        memcpy(&s_boxCell, &box->head, sizeof(s_boxCell));
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x0001ab40, 0x46)
b32 IsHotspotTreasureOpen(i32 index) {
    SceneSprite* sprite = GetHotspotSprite(index);
    TreasureBox* box = FindTreasureBoxAt(sprite->cellX, sprite->cellY, 0);
    if (box) {
        return IsTreasureBoxOpen(box);
    }
    return false;
}

RVA(0x0001ab90, 0x168)
b16 RunPartyReorder(void) {
    i16 slot;
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            NextGamePhase();
            break;
        case 1:
            ReturnFromGameState();
            FlushStatusRedraw(1);
            break;
        case 2:
            s_reorderFirst = PickReorderSlot();
            if (s_reorderFirst == -1) {
                break;
            }
            if (s_reorderFirst < 0) {
                ClearPartySlotSelection();
                PrevGamePhase();
            } else {
                NextGamePhase();
            }
            break;
        case 3:
            s_reorderSecond = PickReorderSlot();
            if (s_reorderSecond == -1) {
                break;
            }
            ClearPartySlotSelection();
            if (s_reorderSecond < 0) {
                PrevGamePhase();
                FlushStatusRedraw(1);
                break;
            }
            ExchangePartySlot(
                s_reorderFirst,
                ExchangePartySlot(s_reorderSecond, GetPartySlot(s_reorderFirst))
            );
            MarkPickDone();
            SetGamePhase(1);
            for (s_reorderFirst = 0; s_reorderFirst < 3; s_reorderFirst++) {
                if (GetPartySlot(s_reorderFirst) >= 0) {
                    return false;
                }
            }
            for (slot = 3; slot < 6; slot++) {
                s_reorderFirst = GetPartySlot(slot);
                if (s_reorderFirst >= 0) {
                    s_reorderFirst = ExchangePartySlot(slot - 3, s_reorderFirst);
                    ExchangePartySlot(slot, s_reorderFirst);
                }
            }
            break;
    }
    return false;
}

RVA(0x0001ad00, 0x39)
i16 PickReorderSlot(void) {
    if (!PollPartySlotSelection(1)) {
        return -1;
    }
    if (g_selectedObjectId < 0) {
        return -2;
    }
    ResetTextPlaneHighlight(g_infoPlane);
    return g_selectedObjectId;
}

static void AnalyzeMenuHandler(MenuBox* menu, i16 index, i16 event);

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
    i16 choice;

    switch (s_step) {
        case -1:
            if (s_menu != NULL) {
                s_menu = DestroyMenuBox(s_menu);
            }
            if (s_dataPlane != -1) {
                s_dataPlane = CloseTextWindow(s_dataPlane);
            }
            if (s_namePlane != -1) {
                s_namePlane = CloseTextWindow(s_namePlane);
            }
            s_step++;
            return -1;

        case 0:
            if (target == NULL) {
                return -1;
            }
            if (IsEventFlagSet(2, 11) && IsEventFlagSet(2, 12) && IsEventFlagSet(2, 16)) {
                ShowMessage(
                    "\202c\202`\202r\202\252\203C\203\223\203X\203g\203D\201["
                    "\203\213\202\263\202\352\202\304\202\242\202\334\202\271\202\361",
                    -1
                ); // ＤＡＳがインストゥールされていません
                s_step = -1;
                return 0;
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
            PrintWindowText(s_namePlane, g_scratchBuffer, 0x400, 0, 1);
            RepaintTextPlane(s_namePlane, -2);
            s_step++;
            return 0;

        case 1:
            if (IsEventFlagSet(2, 12) && IsEventFlagSet(2, 16)) {
                s_step++;
                return 0;
            }
            if (!HasAnalyzeData(target->id)) {
                PrintWindowText(
                    s_namePlane,
                    "\203A\203i\203\211\203C\203Y\203f\201[\203^"
                    "\202\252\202\240\202\350\202\334\202\271\202\361\n",
                    0x400,
                    1,
                    1
                ); // アナライズデータがありません
                s_step++;
                return 0;
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
            PrintWindowText(s_dataPlane, g_scratchBuffer, 0x400, 0, 1);
            sprintf(g_scratchBuffer, "\203\214\203x\203\213 L%2d\n", target->level); // レベル L%2d
            PrintWindowText(s_dataPlane, g_scratchBuffer, 0x400, 0, 1);
            sprintf(
                g_scratchBuffer,
                "\202g\202o   %d/%d\n",
                target->pools.hp.cur,
                target->pools.hp.max
            ); // ＨＰ   %d/%d
            PrintWindowText(s_dataPlane, g_scratchBuffer, 0x400, 0, 1);
            sprintf(
                g_scratchBuffer,
                "\202l\202o   %d/%d\n",
                target->pools.mp.cur,
                target->pools.mp.max
            ); // ＭＰ   %d/%d
            PrintWindowText(s_dataPlane, g_scratchBuffer, 0x400, 0, 1);
            sprintf(
                g_scratchBuffer,
                "\221\324\223x   %s\n",
                s_attitudeNames[target->attitude]
            ); // 態度   %s
            PrintWindowText(s_dataPlane, g_scratchBuffer, 0x400, 0, 1);
            sprintf(
                g_scratchBuffer,
                "\217\363\221\324   %s\n",
                GetFirstConditionName(GetCharacterConditions(target))
            ); // 状態   %s
            PrintWindowText(s_dataPlane, g_scratchBuffer, 0x400, 0, 1);
            RepaintTextPlane(s_dataPlane, -2);
            return 0;

        case 2:
            if (TakeMouseLeftClick()) {
                s_step++;
                return 0;
            }
            if (TakeMouseCancelSound()) {
                s_step = -1;
                return 0;
            }
            break;

        case 3:
            if (IsEventFlagSet(2, 16) || !HasAnalyzeData(target->id)) {
                s_step = -1;
                return 0;
            }
            PrintWindowText(
                s_namePlane,
                "\217\332\215\327\203A\203i\203\211\203C\203Y\202\265\202\334\202\267\202\251\201H"
                "\n",
                0x400,
                1,
                1
            ); // 詳細アナライズしますか？
            s_menu = CreateMenuBox(s_menu, 16, 2);
            SetMenuItems(s_menu, 5, s_yesNo, 2, AnalyzeMenuHandler);
            SetTextPlaneFirstSelectableRow(s_menu->plane, 0, 0);
            SetTextPlaneHighlightMode(s_menu->plane, 1);
            s_step++;
            return 0;

        case 4:
            choice = RunMenu(s_menu);
            if (choice == 0) {
                break;
            }
            s_menu = DestroyMenuBox(s_menu);
            s_step++;
            if (choice >= 0 && g_selectedObjectId >= 0) {
                break;
            }
            s_step = -1;
            return 0;

        case 5: {
            Character* copy;

            s_step++;
            copy = GetCharacter(15);
            // The copy stops short of alignmentA and what follows it.
            memcpy(copy, target, offsetof(Character, alignmentA));
            s_savedRosterEntry = GetRosterEntry(15);
            SetRosterEntry(15, copy);
            SetStatusAnalyzeMode(1);
            PushGameState(0x19);
            s_dataPlane = CloseTextWindow(s_dataPlane);
            s_namePlane = CloseTextWindow(s_namePlane);
            return 0;
        }

        case 6: {
            Character* copy;

            s_step = -1;
            SetStatusAnalyzeMode(0);
            SetRosterEntry(15, s_savedRosterEntry);
            s_savedRosterEntry = NULL;
            copy = GetCharacter(15);
            InitWordList(GetCharacterSkills(copy), 0);
            break;
        }
    }
    return 0;
}

// Lists the yes/no items, and forgets them when the menu is torn down; an
// item's object id is minus its index.
RVA(0x0001b340, 0x50)
static void AnalyzeMenuHandler(MenuBox* menu, i16 index, i16 event) {
    char** items = menu->items.text;

    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->items.text = NULL;
            menu->itemCount = 0;
            break;
        case MENU_EVENT_ADD_ROW:
            AddMenuLine(menu->plane, items[index], 0x1700, -index, 0);
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
    fp = OpenDataFile(32, 12, 0);
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
    marked = 0;
    for (index = 0; events[index].x != -1; index++) {
        if (events[index].x >= left && events[index].x <= x && events[index].y >= top
            && events[index].y <= y) {
            if ((events[index].flagBank || events[index].flagIndex)
                && IsEventFlagSet(events[index].flagBank, events[index].flagIndex)) {
                continue;
            }
            if (events[index].x == s_markedX && events[index].y == s_markedY
                && block == s_markedLayer) {
                marked = 1;
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
    if (member == -1) {
        if (s_itemMenuEquipGroup != -1 && s_itemMenu) {
            RequestMenuRedraw(s_itemMenu);
        }
        s_itemMenuEquipGroup = -1;
        s_itemMenuMember = -1;
        g_itemMenuAmmoType = -1;
        return;
    }
    ammo = GetGunAmmoType(GetCharacterById(member));
    s_itemMenuMember = member;
    group = ReadObjectRecordField(member, 0x20, 2);
    if (s_itemMenu && (s_itemMenuEquipGroup != group || g_itemMenuAmmoType != ammo)) {
        RequestMenuRedraw(s_itemMenu);
    }
    g_itemMenuAmmoType = ammo;
    s_itemMenuEquipGroup = group;
}

RVA(0x0001b660, 0xea)
i16 StepItemBuyMenu(i16* step) {
    switch (*step) {
        case 0: {
            i16 count;
            i16* items = AllocItemMenuStock(GetSceneCellKind(), &count);
            ItemStackList* list = CreateItemMenuEntries(items, count);
            FreeBlock(items);
            s_itemMenu = CreateItemMenu(s_itemMenu, list, count);
            InitItemMenuContext(s_itemMenu, 1, 0, 0x11);
            (*step)++;
            return 0;
        }
        case 1: {
            i16 result = RunMenu(s_itemMenu);
            if (result == -1 || result == 0) {
                return 0;
            }
            if (result == 2) {
                result = -1;
            }
            AdjustItemMenuCount(s_itemMenu, g_hoveredObjectId, result, 99);
            return 0;
        }
        case 2:
            s_itemMenu = DestroyMenuBox(s_itemMenu);
            return -1;
    }
}

RVA(0x0001b750, 0x6d)
MenuBox* CreateItemMenu(MenuBox* old, ItemStackList* entries, i16 count) {
    MenuBox* menu;
    SetItemMenuCharacter(-1);
    menu = CreateMenuBox(old, 0x19, 2);
    MoveMenuBox(menu, -8, -22);
    SetMenuItems(menu, 9, entries, count, ItemMenuHandler);
    SetTextPlaneCancelEnabled(menu->plane, 0);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, 1);
    menu->list->flags |= 2;
    return menu;
}

RVA(0x0001b7c0, 0x196)
void ItemMenuHandler(MenuBox* menu, i16 index, i16 event) {
    ItemStackList* list = menu->items.itemList;
    i16 i;
    switch (event) {
        case MENU_EVENT_DESTROY:
            if (menu->context.item.mode != 2) {
                ClearPool();
                for (i = 0; i < menu->itemCount; i++) {
                    if (GetItemStackCount(GetItemListEntry(list, i))) {
                        AddToPool(
                            GetItemStackItem(GetItemListEntry(list, i)),
                            GetItemStackCount(GetItemListEntry(list, i))
                        );
                    }
                }
                SetScriptLongVar(0x11, GetItemMenuTotal(list, 1, menu->context.item.priceDivisor));
            }
            s_itemMenuLimits = FreeBlock(s_itemMenuLimits);
            menu->items.itemList = FreeBlock(list);
            menu->itemCount = 0;
            break;
        case MENU_EVENT_ADD_ROW: {
            ItemStack* entry = GetItemListEntry(list, index);
            i32 color = 0x3450;
            i32 price = FormatItemMenuEntry(*entry, 1, menu->context.item.priceDivisor);
            ItemRecord* record = GetLoadedRecord(GetItemStackItem(entry));
            if (!GetItemRecordPrice(record)) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x3500, GetItemStackItem(entry), 1);
            } else {
                if (menu->context.item.mode == 0 && menu->context.item.priceDivisor == 1) {
                    if (CompareMacca(-1, price) < 0) {
                        color = 0x3500;
                    }
                }
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    color,
                    GetItemStackItem(entry),
                    menu->context.item.mode == 2
                );
            }
            break;
        }
        case MENU_EVENT_END_PAGE: {
            i32 total;
            if (menu->context.item.mode == 2) {
                total = GetScriptLongVar(menu->context.item.totalVar);
            } else {
                total = GetItemMenuTotal(list, 1, menu->context.item.priceDivisor);
            }
            DrawItemMenuTotal(menu->plane, total, 0, index - menu->cursor);
            break;
        }
    }
}

// @early-stop load width: retail extracts the by-value entry's item with
// a dword load and left shift followed by a word arithmetic right shift;
// this build uses word operations throughout. The item/attachment container
// remains a word, as the other packed-entry readers and writers require.
RVA(0x0001b960, 0x1d5)
i32 FormatItemMenuEntry(ItemStack entry, i32 numerator, i32 denominator) {
    i16 item = GetItemStackItem(&entry);
    char marker = ' ';
    ItemRecord* record = GetLoadedRecord(item);
    i16 equipGroup = GetItemEquipCode(record);
    i32 price;
    if (EquipPartOfItem(record) >= 0 && s_itemMenuEquipGroup != -1) {
        if (GetItemCategory(item) == EQUIP_PART_ACCESSORY) {
            Character* member = GetCharacterById(s_itemMenuMember);
            if (member && CanEquipItem(member, item) > 0) {
                marker = 'E';
            }
        } else if (CanGroupEquip(s_itemMenuEquipGroup, equipGroup)) {
            Character* member;
            marker = 'E';
            member = GetCharacterById(s_itemMenuMember);
            record = GetLoadedRecord(item);
            if (record->kind == ITEM_KIND_GUN && GetBattleStatShown(member, 6) > 0) {
                if (LacksItemRequiredStats(member, record, GetBattleStatShown(member, 6))) {
                    marker = 'e';
                }
            } else if (LacksItemRequiredStats(member, record, 0)) {
                marker = 'e';
            }
        }
    }
    record = GetLoadedRecord(item);
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
void DrawItemMenuTotal(i16 plane, i32 total, i16 redraw, i16 line) {
    if (!redraw) {
        if (!s_hideItemMenuTotal) {
            sprintf(g_scratchBuffer, "                \215\207\214\166 %10ld   ", total);
        } else {
            sprintf(g_scratchBuffer, "                \215\207\214\166 ");
            s_hideItemMenuTotal = false;
        }
        for (; line < 9; line++) {
            AddMenuLine(plane, s_emptyItemLine, 0x1400, -1, 1);
        }
        AddMenuLine(plane, g_scratchBuffer, 0x1400, -1, 1);
    } else {
        sprintf(g_scratchBuffer, "\215\207\214\166 %10ld   ", total);
        SetTextPlaneCursorLine(plane, 16, 9);
        PrintWindowText(plane, g_scratchBuffer, 0x1400, 1, 1);
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
        PrintWindowText(menu->plane, g_scratchBuffer, attr, 1, 1);
        total = GetItemMenuTotal(list, 1, menu->context.item.priceDivisor);
        DrawItemMenuTotal(menu->plane, total, 1, 9);
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
        case 0:
            NextGamePhase();
            break;
        case 1:
            step = GetGameStep();
            result = StepItemBuyMenu(&step);
            SetGameStep(step);
            if (result == -1) {
                NextGamePhase();
            }
            break;
        case 2:
            ReturnFromGameState();
            break;
    }
    return false;
}

RVA(0x0001be70, 0x110)
i16 StepItemSellMenu(i16* step) {
    ItemStackList* list;
    switch (*step) {
        case 0: {
            i16 i;
            s_itemMenuLimits = CopyBagEntries(0, 48, NULL);
            list = CopyBagEntries(0, 48, NULL);
            for (i = 0; i < GetItemListCount(list); i++) {
                GetItemListEntry(list, i)->count = 0;
            }
            s_itemMenu = CreateItemMenu(s_itemMenu, list, GetItemListCount(list));
            InitItemMenuContext(s_itemMenu, 4, 0, 0x11);
            (*step)++;
            return 0;
        }
        case 1: {
            i16 result;
            result = RunMenu(s_itemMenu);
            if (result == -1 || result == 0) {
                return 0;
            }
            if (result == 2) {
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
            return 0;
        }
        case 2:
            s_itemMenu = DestroyMenuBox(s_itemMenu);
            return -1;
    }
}

RVA(0x0001bf80, 0x54)
b16 RunItemSellMenu(void) {
    i16 step;
    i16 result;
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            break;
        case 1:
            step = GetGameStep();
            result = StepItemSellMenu(&step);
            SetGameStep(step);
            if (result == -1) {
                NextGamePhase();
            }
            break;
        case 2:
            ReturnFromGameState();
            break;
    }
    return false;
}

RVA(0x0001bfe0, 0x2a)
void RefreshScriptItemMenuTotal(void) {
    DrawItemMenuTotal(s_itemMenu->plane, GetScriptLongVar(s_itemMenu->context.item.totalVar), 1, 9);
}

RVA(0x0001c010, 0x8b)
void OpenScriptItemMenu(i16 totalVar, i16 selling) {
    ItemStack* entries;
    i16 count;
    ItemStackList* list;
    SetItemMenuCharacter(-1);
    entries = GetPoolEntries();
    count = CountPoolEntries();
    list = CopyItemMenuEntries(entries, count);
    s_itemMenu = CreateItemMenu(s_itemMenu, list, count);
    InitItemMenuContext(s_itemMenu, selling ? 4 : 1, 2, totalVar);
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
    if (s_itemMenu && s_itemMenu->context.item.mode == 2) {
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
    if (s_itemMenuStock == 0) {
        FILE* fp = OpenDataFile(8, 12, 0);
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
    while (*items != -1) {
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
    while (*stock != -1) {
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
    FILE* fp = OpenDataFile(48, 12, 0);
    s_learnableSkillTable = ReadRawHandle(fp);
    CloseDataFile(fp);
    fp = OpenDataFile(49, 12, 0);
    s_learnableSkillRequirements = ReadRawHandle(fp);
    CloseDataFile(fp);
}

RVA(0x0001c2b0, 0x7f)
i16* GetLearnableSkillList(i16 id, i16 source) {
    i16 key;
    i16 i;
    LearnableSkillTable* table;
    if (id == 0) {
        key = GetCharacterAffiliation(GetRosterCharacter(0), source);
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
    for (i = 0; skills[i] != -1; i++) {
        if (ContainsWord(GetCharacterSkills(character), skills[i])) {
            continue;
        }
        if (id == 0) {
            i16 found = -1;
            LearnableSkillRequirement* requirements = HandleReadPtr(s_learnableSkillRequirements);
            for (j = 0; requirements[j].skill != -1; j++) {
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
    if (skill == -1) {
        skills[0] = -1;
        return -1;
    }
    i++;
    for (j = 0; skills[i + j] != -1; j++) {
        skills[j] = skills[i + j];
    }
    skills[j] = -1;
    return skill;
}

RVA(0x0001c460, 0x173)
i16 PickGrowthStats(Character* character, i16* picks, i16 turn) {
    memset(picks, -1, 3 * sizeof(i16));
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
    for (i = 0; i < 3; i++) {
        if (picks[i] >= 0) {
            i16 j;
            for (j = 0; j < 10; j++) {
                if (GetBaseStat(character, j) > GetBaseStat(character, picks[i])) {
                    break;
                }
            }
            if (j >= 10 && RandomAverage(0, 3, 0) == 0) {
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
u32 AddTrainingPointsRaw(Character* character, i16 kind, u32 amount) {
    u32* points = &character->trainingPoints[kind];
    u32 limit;
    amount += *points;
    limit = TrainingThreshold(99);
    amount = min(limit, amount);
    *points = amount;
    return amount;
}

RVA(0x0001c6c0, 0x24)
u32 AddTrainingPoints(Character* character, i16 kind, i16 amount) {
    if (kind >= 0 && kind < 4) {
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
i16 ApplyTraining(Character* character, i16 kind) {
    i16 raised = 0;
    switch (kind) {
        case 0:
            RaiseTrainedLevel(
                GetBattleStatBase(character, 0),
                GetTrainingPoints(character, 0),
                raised
            );
            break;
        case 1:
            RaiseTrainedLevel(
                GetBattleStatBase(character, 6),
                GetTrainingPoints(character, 1),
                raised
            );
            break;
        case 2:
            RaiseTrainedLevel(
                GetBattleStatBase(character, 12),
                GetTrainingPoints(character, 2),
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
    for (i = 0; i < 3; i++) {
        if (character->affiliation[i] > 3 || character->affiliation[i] < 0) {
            SetCharacterAffiliation(character, i, -1);
        }
    }
    for (i = 2; i > 0; i--) {
        if (character->affiliation[i] != -1) {
            for (j = i - 1; j >= 0; j--) {
                if (character->affiliation[i] == character->affiliation[j]) {
                    SetCharacterAffiliation(character, i, -1);
                }
            }
        }
    }
    for (i = 0; i < 2; i++) {
        if (character->affiliation[i] < 0) {
            for (j = 0; i + j + 1 < 3; j++) {
                SetCharacterAffiliation(character, i + j, character->affiliation[i + j + 1]);
            }
            SetCharacterAffiliation(character, i + j, -1);
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
        case 0x60:
            if (cell->trap.damagePercent < 1) {
                return 0;
            }
            percent = RandomAverage(1, cell->trap.damagePercent, 1);
            break;
        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
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

// @early-stop stack layout: retail reserves sixteen bytes while the local
// ten-byte ExitCell needs twelve. All operations and register assignments
// agree; the record stride and CopyExitAt forbid padding the cell type.
RVA(0x0001c9f0, 0xee)
void RunCellTrap(i16 mode, i16 x, i16 y) {
    ExitCell cell;
    Character* member;
    i32 damage;
    i16 hp;
    u8 alignmentMask;
    if (mode && CopyExitAt(x, y, &cell)) {
        for (mode = 0; mode < 6; mode++) {
            member = GetPartyCharacter(mode);
            if (member) {
                damage = GetCellTrapDamage(&cell, member->pools.hp.max);
                hp = member->pools.hp.cur;
                if (cell.head.code >= 0x68 && cell.head.code <= 0x6e) {
                    alignmentMask = 4;
                    alignmentMask >>= GetAlignmentClassB(member) + 1;
                    if (!(cell.trap.alignmentMask & alignmentMask)) {
                        continue;
                    }
                }
                ChangePool(&member->pools.hp, -damage);
                if (hp && !member->pools.hp.cur) {
                    ApplyEmptyPools(member);
                }
            }
        }
        if (cell.head.code == CELL_CHUTE) {
            PlaySoundEffect(0x57);
        } else {
            PlaySoundEffect(0x60);
        }
        FlushStatusRedraw(1);
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
    panel->input.rightClick = 0;
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
    panel->input.rightClick = 1;
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
i16 ApplyRowCheck(PanelRow* row, i16 value, i16 op) {
    i16 result = 0;
    switch (op) {
        case -1:
            g_mouseLeftClick = 0;
            result = ToggleFlagBits(&row->flags, PANEL_ROW_CHECKED);
            break;
        case 0:
            ClearPanelRowCheck(row);
            break;
        case 1:
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
    if (levels == 0) {
        levels = CreateArrayHandle(GetAutomapLevelTableSize(count), 1);
        *slot = levels;
        ((AutomapLevels*)HandleWritePtr(levels))->header.count = count;
    }
    for (level = 0; level < count; level++) {
        if (GetAutomapLevelHandle(HandleReadPtr(levels), level) == 0) {
            size = GetAreaSize(level);
            bytes = ((i16)(size.x * size.y) + 7) / 8;
            bitmap = CreateArrayHandle(bytes + 8, 1);
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
    for (i = 0; i < 256; i++) {
        levels = GetAutomapAreaHandle(i);
        if (levels != 0) {
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
        && GetAutomapAreaHandle(s_levelArea) != 0) {
        levels = HandleReadPtr(GetAutomapAreaHandle(s_levelArea));
        if (GetAutomapLevelCount(levels) > s_levelIndex) {
            bitmap = GetAutomapLevelHandle(levels, s_levelIndex);
            if (bitmap != 0) {
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
    if (s_areas == NULL || GetAutomapAreaHandle(area) == 0) {
        return;
    }
    levels = HandleReadPtr(GetAutomapAreaHandle(area));
    if (GetAutomapLevelCount(levels) <= level) {
        return;
    }
    bitmap = GetAutomapLevelHandle(levels, level);
    if (bitmap == 0) {
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

// 0x100 when x/y of `area`/`level` has not been explored (or has no bitmap);
// coordinates wrap at the level size.
RVA(0x0001d120, 0xe1)
i16 IsAutomapCellHidden(i16 x, i16 y, i16 area, i16 level) {
    AutomapLevels* levels;
    AutomapBitmap* data;
    i32 bitmap;
    u8* bits;
    i16 index;
    if (s_levelArea == area && s_levelIndex == level) {
        return TestBit(s_levelBitmap->bits, AutomapCellIndex(s_levelBitmap, x, y)) != 1 ? 0x100 : 0;
    } else {
        if (s_areas == NULL) {
            return 0x100;
        }
        if (GetAutomapAreaHandle(area) == 0) {
            return 0x100;
        }
        levels = HandleReadPtr(GetAutomapAreaHandle(area));
        if (GetAutomapLevelCount(levels) <= level) {
            return 0x100;
        }
        bitmap = GetAutomapLevelHandle(levels, level);
        if (bitmap == 0) {
            return 0x100;
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
    return TestBit(bits, index) != 1 ? 0x100 : 0;
}

RVA(0x0001d210, 0xd4)
void RotateAutomapRegion(
    i16 x,
    i16 y,
    i16 direction,
    i16* left,
    i16* top,
    i16* width,
    i16* height
) {
    i16 oldWidth;
    switch (direction) {
        case 0:
            *left = -x;
            *top = -y;
            break;
        case 1:
            *left = -y;
            *top = x - *width + 1;
            oldWidth = *width;
            *width = *height;
            *height = oldWidth;
            break;
        case 2:
            *left = x - *width + 1;
            *top = y - *height + 1;
            break;
        case 3:
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
        case 0:
            *x -= s_mapOriginX;
            *y -= s_mapOriginY;
            break;
        case 1:
            oldX = *x;
            *x = *y - s_mapOriginY;
            *y = s_mapOriginX - oldX;
            break;
        case 2:
            *x = s_mapOriginX - *x;
            *y = s_mapOriginY - *y;
            break;
        case 3:
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
        if (g_field.pos.area == 0x82 && g_field.pos.level == 15) {
            if (!g_fieldStatus.navigationFixed && (g_field.pos.direction & 1)) {
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
        case 0:
            NextGamePhase();
            s_mapActive = true;
            RestoreDrawState(SaveDrawState());
            s_mapPosition = g_field.pos;
            s_mapPlane = CreateTextPlane(31, 0);
            s_mapPanel = CreateKindPanel(s_mapPanel, 0x11d, 4, 31);
            if (g_fieldStatus.automapFixed) {
                s_mapPosition.direction = 0;
            }
            s_mapDetail = AUTOMAP_DETAIL_NONE;
            if (!IsEventFlagSet(2, 0x39)) {
                s_mapDetail = AUTOMAP_DETAIL_NPCS;
            }
            if (s_mapDetail < AUTOMAP_DETAIL_BASIC) {
                SetGamePhase(3);
            }
            break;
        case 1:
            NextGamePhase();
            savedState = SaveDrawState();
            DrawAutomapViewport(s_mapPosition);
            UpdateAutomapScrollPanel();
            RestoreDrawState(savedState);
            break;
        case 2:
            input = RunPanelInput(s_mapPanel);
            if (input == -2) {
                NextGamePhase();
            } else if (input != -1) {
                savedState = SaveDrawState();
                switch (input) {
                    case 0:
                        ScrollPlaneMapDown(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, 0, -1);
                        DrawAutomapRegion(s_mapOriginX, s_mapOriginY, s_mapWidth, 1, 0, 0);
                        break;
                    case 1:
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
                    case 2:
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
                    case 3:
                        ScrollPlaneMapRight(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, -1, 0);
                        DrawAutomapRegion(s_mapOriginX, s_mapOriginY, 1, s_mapHeight, 0, 0);
                        break;
                }
                UpdateAutomapScrollPanel();
                RestoreDrawState(savedState);
            }
            break;
        case 3:
            CloseTextWindow(s_mapPlane);
            s_mapPanel = ReleasePanel(s_mapPanel, 1);
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
    i16 blocked;
    ClearPanelChecksAgain(s_mapPanel);
    GetMapSize(&width, &height);
    blocked = 0;
    switch (s_mapDirection) {
        case 0:
            if (s_mapOriginX == 0) {
                blocked |= 1;
            }
            if (s_mapOriginX + s_mapWidth >= width) {
                blocked |= 2;
            }
            if (s_mapOriginY == 0) {
                blocked |= 4;
            }
            if (s_mapOriginY + s_mapHeight >= height) {
                blocked |= 8;
            }
            break;
        case 1:
            if (s_mapOriginY == 0) {
                blocked |= 1;
            }
            if (s_mapOriginY + s_mapWidth >= height) {
                blocked |= 2;
            }
            if (s_mapOriginX == width - 1) {
                blocked |= 4;
            }
            if (s_mapOriginX - s_mapHeight < 0) {
                blocked |= 8;
            }
            break;
        case 2:
            if (s_mapOriginX == width - 1) {
                blocked |= 1;
            }
            if (s_mapOriginX - s_mapWidth < 0) {
                blocked |= 2;
            }
            if (s_mapOriginY == height - 1) {
                blocked |= 4;
            }
            if (s_mapOriginY - s_mapHeight < 0) {
                blocked |= 8;
            }
            break;
        case 3:
            if (s_mapOriginY == height - 1) {
                blocked |= 1;
            }
            if (s_mapOriginY - s_mapWidth < 0) {
                blocked |= 2;
            }
            if (s_mapOriginX == 0) {
                blocked |= 4;
            }
            if (s_mapOriginX + s_mapHeight == width) {
                blocked |= 8;
            }
            break;
    }
    SetPanelRowFlags(s_mapPanel, 3, PANEL_HIDDEN, blocked & 1);
    SetPanelRowFlags(s_mapPanel, 1, PANEL_HIDDEN, blocked & 2);
    SetPanelRowFlags(s_mapPanel, 0, PANEL_HIDDEN, blocked & 4);
    SetPanelRowFlags(s_mapPanel, 2, PANEL_HIDDEN, blocked & 8);
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
    IsCellBlocked(s_mapPosition.level, 0, 0, 0);
    if (s_mapPosition.level == g_field.pos.level) {
        DrawAutomapMark(
            TurnDirection(g_field.pos.direction, -s_mapDirection),
            g_field.pos.x,
            g_field.pos.y
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
    i16 direction;
    GetMapSize(&width, &height);
    x = position.x;
    y = position.y;
    direction = position.direction;
    RotateAutomapRegion(x, y, direction, &left, &top, &width, &height);
    if (width <= 38) {
        viewWidth = width;
    } else {
        viewWidth = 38;
        if (-left * 2 > 38) {
            left = -19;
            switch (direction) {
                case 0:
                    if (x + 19 > width) {
                        left = width - x - 38;
                    }
                    break;
                case 1:
                    if (y + 19 > width) {
                        left = width - y - 38;
                    }
                    break;
                case 2:
                    if (x - 18 < 0) {
                        left = x - 37;
                    }
                    break;
                case 3:
                    if (y - 18 < 0) {
                        left = y - 37;
                    }
                    break;
            }
        }
    }
    if (height <= 18) {
        viewHeight = height;
    } else {
        viewHeight = 18;
        if (-top * 2 > 18) {
            top = -9;
            switch (direction) {
                case 0:
                    if (y + 9 > height) {
                        top = height - y - 18;
                    }
                    break;
                case 1:
                    if (x - 8 < 0) {
                        top = x - 17;
                    }
                    break;
                case 2:
                    if (y - 8 < 0) {
                        top = y - 17;
                    }
                    break;
                case 3:
                    if (x + 9 > height) {
                        top = height - x - 18;
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
    ClearLayerSurface(6);
    if (g_fieldStatus.navigationFixed) {
        position.direction = 0;
    }
    s_mapDetail = AUTOMAP_DETAIL_NONE;
    if (!IsEventFlagSet(2, 9)) {
        s_mapDetail = AUTOMAP_DETAIL_BASIC;
    }
    if (!IsEventFlagSet(2, 15)) {
        s_mapDetail = AUTOMAP_DETAIL_NPCS;
    }
    if (!IsEventFlagSet(2, 0x38)) {
        s_mapDetail = AUTOMAP_DETAIL_OBJECTS;
    }
    if (s_mapDetail < AUTOMAP_DETAIL_BASIC) {
        return;
    }
    if (IsDarkCell(g_field.pos.x, g_field.pos.y)) {
        return;
    }
    GetMapSize(&width, &height);
    x = position.x;
    y = position.y;
    direction = position.direction;
    RotateAutomapRegion(x, y, direction, &left, &top, &width, &height);
    if (width <= 7) {
        viewWidth = width;
        screenX = 8 - width;
    } else {
        screenX = 1;
        viewWidth = 7;
        if (-left * 2 > 7) {
            left = -3;
            switch (position.direction) {
                case 0:
                    if (position.x + 4 > width) {
                        left = width - x - 7;
                    }
                    break;
                case 1:
                    if (position.y + 4 > width) {
                        left = width - y - 7;
                    }
                    break;
                case 2:
                    edge = x - 3;
                    if (edge < 0) {
                        left = x - 6;
                    }
                    break;
                case 3:
                    edge = y - 3;
                    if (edge < 0) {
                        left = y - 6;
                    }
                    break;
            }
        }
    }
    if (height <= 7) {
        viewHeight = height;
        screenY = 21 - height;
    } else {
        screenY = 14;
        viewHeight = 7;
        edge = -top * 2;
        if (edge > 7) {
            top = -3;
            switch (position.direction) {
                case 0:
                    if (position.y + 4 > height) {
                        top = height - y - 7;
                    }
                    break;
                case 1:
                    edge = x - 3;
                    if (edge < 0) {
                        top = x - 6;
                    }
                    break;
                case 2:
                    edge = y - 3;
                    if (edge < 0) {
                        top = y - 6;
                    }
                    break;
                case 3:
                    if (position.x + 4 > height) {
                        top = height - x - 7;
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
    IsCellBlocked(g_field.pos.level, 0, 0, 0);
    if (s_mapDetail >= AUTOMAP_DETAIL_NPCS) {
        MarkAreaNpcs();
    }
    if (s_mapDetail >= AUTOMAP_DETAIL_OBJECTS) {
        MarkObjectsOnMap();
    }
    MarkMapCell(TurnDirection(g_field.pos.direction, -direction), g_field.pos.x, g_field.pos.y);
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
        if (g_field.pos.area == 0x82 && g_field.pos.level == 15) {
            if (!g_fieldStatus.navigationFixed && (g_field.pos.direction & 1)) {
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
    errors = 256 - fwrite(s_areas, 4, 256, fp);
    for (area = 0; area < 256; area++) {
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
    LoadAutomapLevel(g_field.pos.area, g_field.pos.level);
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
    errors = 256 - fread(s_areas, 4, 256, fp);
    if (errors) {
        return errors;
    }
    for (area = 0; area < 256; area++) {
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
    DrawMapOverlay(g_field.pos);
}

RVA(0x0001e720, 0x12)
i16 SetInfoBarLayout(i16 layout) {
    i16 previous = s_nextLayout;
    s_nextLayout = layout;
    return previous;
}

RVA(0x0001e740, 0x4b)
void DrawMoneyCounters(i16 mode) {
    DrawMoneyCounter(mode, 8, RosterMemberAt(0)->magnetite, 0);
    s_shownMagnetite = RosterMemberAt(0)->magnetite;
    DrawMoneyCounter(mode, 11, RosterMemberAt(0)->macca, 1);
    s_shownMacca = RosterMemberAt(0)->macca;
}

RVA(0x0001e790, 0xcf)
void DrawMoneyCounter(i16 mode, i16 row, i32 value, i16 currency) {
    i32 attr = 0xffffb400;
    if (!currency) {
        DrawLayerText(SCREEN_LAYER_CURRENCY, 8, 8, "       ", 0xb400);
        if (value < 1) {
            attr = 0xffffb500;
        }
        sprintf(g_scratchBuffer, "%7ld", value);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 8, 8, g_scratchBuffer, attr);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 72, 8, "MAG", attr);
    } else {
        DrawLayerText(SCREEN_LAYER_CURRENCY, 40, 32, "       ", 0xb400);
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
    DrawLayerText(SCREEN_LAYER_MOON_PHASE, 8, 8, g_scratchBuffer, 0xb400);
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
    DrawLayerText(SCREEN_LAYER_LOCATION, 8, 8, g_scratchBuffer, 0x3400);
    floor = GetLevelFloor();
    DrawLayerText(SCREEN_LAYER_LOCATION, 176, 8, "    ", 0xb400);
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
        DrawLayerText(SCREEN_LAYER_LOCATION, x, 8, g_scratchBuffer, 0xb400);
    }
}

RVA(0x0001e9f0, 0x60)
b16 RefreshInfoBar(i16 force) {
    if (force) {
        DrawInfoBar(s_nextLayout, 1);
    } else if (s_shownMoonPhase != g_clock.moonPhase
               || s_shownMagnetite != RosterMemberAt(0)->magnetite
               || s_shownMacca != RosterMemberAt(0)->macca) {
        DrawInfoBar(s_nextLayout, 1);
    }
    s_nextLayout = 1;
    return false;
}

RVA(0x0001ea50, 0x34)
b16 UpdateInfoBar(void) {
    if (g_fieldRedrawRequest) {
        DrawInfoBar(0, 0);
        return false;
    }
    if (g_tickElapsed >= CLOCK_UPDATE_MOON) {
        DrawInfoBar(1, 0);
    }
    return false;
}

static __inline void EnsureGridByteStorage(i32* grid) {
    if (!*grid) {
        *grid = AllocHandle(0x1000);
    }
}

static __inline i16 GridByteIndex(i16 x, i16 y) {
    return y * 64 + x;
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

// Sets cell x/y of a 64x64 byte grid (allocated on first use).
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
            if (GetRoomRegion(x, y) == 0xff) {
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
    return (IsCellFlagSet((CellHead*)list, offset) != 0) ^ invert;
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

// Clears the region grid, then marks the level's rooms (0x1f-byte entries,
// regions 0..) and doors (13-byte entries, regions 0x80..).
RVA(0x0001ed20, 0x4b)
void MarkRoomRegions(u8* rooms, u8* doors, i16 width, i16 height) {
    FillRegionRect(0, 0, 0x3f, 0x3f, 0xff);
    MarkRegionList(rooms, 0x1f, 0, width, height);
    MarkRegionList(doors, 0xd, 0x80, width, height);
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
            i16 now = GetRoomRegion(x, y);
            changed |= now - GetPrevRegion(x, y);
            SetRoomRegion(x, y, GetPrevRegion(x, y));
        }
    }
    return changed;
}

// Whether re-marking the rooms would change any cell's region (the grid is
// left as it was).
// @early-stop register allocation: width and height swap ebx/ebp.
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
    return GetMapCellCode(g_field.pos.x, g_field.pos.y);
}

// Whether a region code is a door (0x80..).
RVA(0x0001eee0, 0xa)
i16 IsObjectCell(i16 code) {
    return code & 0x80;
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
    ResetFieldLayer(1);
    ResetFieldLayer(0);
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
    return FindRegionData(GetLevelList(0), 0x1f, code & 0x7f);
}

RVA(0x0001efb0, 0x1e)
DoorRegionData* GetCellObjectTable(i16 code) {
    void* data = FindRegionData(GetLevelList(1), 0xd, code & 0x7f);
    return data;
}

// Enters region `code`: a room loads its NPC images, a door its two enemy
// groups (object ids 0x20..0x201f).
RVA(0x0001efd0, 0x95)
void EnterRoom(i16 code) {
    DoorRegionData* table;
    i16 object;
    SetCurrentRoomCode(code);
    if (code == 0xff || code == -1) {
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
    if (object >= 0x20 && object <= 0x201f) {
        LoadEnemyGroupSlot(0, object);
    }
    object = LookupCellObject(table, 1);
    if (object >= 0x20 && object <= 0x201f) {
        LoadEnemyGroupSlot(1, object);
    }
}

// Re-enters the party's region when it changed (resetting the field and
// respawning), else re-picks the two special pictures of area 0x85 level 3.
RVA(0x0001f070, 0xb6)
void UpdateCurrentRoom(void) {
    i16 code;
    if (!CellCodeDiffers(GetCurrentRoomCode(), g_field.pos.x, g_field.pos.y)) {
        if (g_field.pos.area == 0x85 && g_field.pos.level == 3 && g_field.pos.x == 3) {
            if (g_field.pos.y == 4) {
                LoadNpcTexture(0, 0x53, 0);
            } else if (g_field.pos.y == 5) {
                LoadNpcTexture(0, 0x4c, 0);
            }
        }
        return;
    }
    RequestFieldRefresh();
    ResetFieldObjects();
    ResetFieldScene();
    code = GetMapCellCode(g_field.pos.x, g_field.pos.y);
    EnterRoom(code);
    SpawnMapObjects(code);
}

// After a step through a door: when the cell behind the party is in another
// region, resets the field and enters that region.
RVA(0x0001f130, 0x99)
void FinishDoorStep(void) {
    i16 x = g_field.pos.x;
    i16 y = g_field.pos.y;
    i16 direction = TurnDirection(g_field.pos.direction, g_field.moveCommand);
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

// The slot the next NPC takes, -1 when all 16 are placed.
RVA(0x0001f250, 0x11)
i16 NextNpcSlot(void) {
    if (s_npcCount >= 16) {
        return -1;
    }
    return s_npcCount;
}

RVA(0x0001f270, 0x12)
void CountPlacedNpc(void) {
    if (s_npcCount < 16) {
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
            MarkMapCell(4, s_npcs[i].x, s_npcs[i].y);
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
    if (code == 0x2b && mode == 0 && g_field.pos.area == 0x82 && g_field.pos.level == 8) {
        if ((g_field.pos.x == 0xb && g_field.pos.y == 7)
            || (g_field.pos.x == 0xc && g_field.pos.y == 6)) {
            code = 0x24;
        }
    } else if (code == 0x53 && mode == 0 && g_field.pos.area == 0x85 && g_field.pos.level == 3
               && g_field.pos.y > 4) {
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
    for (i = 0; i < 6; i++) {
        if (p[1] != 0xff) {
            LoadNpcTexture(i, p[0], p[1]);
        }
        p += 2;
    }
}

// The NPC texture in `slot` (0..5), else 0.
RVA(0x0001f6b0, 0x1e)
u32 GetNpcTexture(i16 slot) {
    if (slot >= 0 && slot < 6) {
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
// succeed. Returns the handler's result (1 done, 0 no effect, -1 failed).
// @identity-TODO: the handlers are named from their bodies only.
RVA(0x0001f700, 0xcc)
i16 RunFieldEffect(i16 effect) {
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
            return 1;
    }
    return 0;
}

// Pushes `who` (a field object, or the party for a negative id) one cell
// back (the party: behind itself; an object: away from the party), unless a
// wall, a map-cell change or a blocked cell stops it; -1 then.
RVA(0x0001f7d0, 0x152)
i16 KnockBack(i16 who) {
    i16* at;
    i16 direction;
    if (who < 0) {
        direction = g_field.pos.direction;
        at = &g_field.pos.x;
    } else {
        i16 object;
        i16 code;
        i16 x;
        i16 y;
        Character* actor;
        direction = OppositeDirection(g_field.pos.direction);
        object = GetLiveObject(who);
        if (object < 0) {
            return -1;
        }
        actor = GetFieldActor(object);
        at = &((FieldActor*)GetFieldActor(object))->pos.x;
        if (TestCharacterFlag(actor, 0x20)) {
            return -1;
        }
        code = GetMapCellCode(at[0], at[1]);
        x = at[0];
        y = at[1];
        StepMapCoord(&x, &y, direction, 2);
        WrapMapPosition(&x, &y);
        if (CellCodeDiffers(code, x, y)) {
            return -1;
        }
        if (IsCellBlocked(g_field.pos.level, 1, x, y)) {
            return -1;
        }
    }
    if (WallStopsToward(at[0], at[1], direction, 2)) {
        return -1;
    }
    StepMapCoord(&at[0], &at[1], direction, 2);
    RefreshFieldScene();
    return 1;
}

// Gives the target a shield of a tenth of its maximum HP (a field object:
// respawns it instead).
RVA(0x0001f930, 0x56)
i16 ShieldTarget(void) {
    Character* target;
    if (g_targetId < 0) {
        target = GetCombatant(g_targetId);
        if (!target) {
            return -1;
        }
        target->shield = target->pools.hp.max / 10;
        return 1;
    }
    return RespawnFieldObject(g_targetId, 0, -1, 1);
}

RVA(0x0001f990, 0x2d)
i16 SetTargetFlag21(void) {
    Character* target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    SetCharacterFlag(target, 0x21);
    return 1;
}

// Sets the leader's flag 0x22 and clears flag 10 of every live object out of
// reach.
RVA(0x0001f9c0, 0x67)
b16 ScatterObjects(void) {
    i16 i;
    u8* flags = GetCharacterFlags(GetRosterCharacter(0));
    SetBit(flags, 0x22);
    for (i = 0; i < 16; i++) {
        i16 object = GetLiveObject(i);
        if (object >= 0 && !HasObjectInReach(1, -1, object)) {
            flags = GetCharacterFlags(GetCombatant(object));
            ClearBit(flags, 10);
        }
    }
    return true;
}

// Sends the party to the return point recorded in the roster leader.
RVA(0x0001fa30, 0x35)
b16 ReturnToLeaderWarp(void) {
    Character* leader = GetRosterCharacter(0);
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
    Character* leader = GetRosterCharacter(0);
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
i16 KnockBackActor(void) {
    i16 result;
    if (g_actorId >= 0) {
        return -1;
    }
    result = KnockBack(g_actorId);
    if (result < 0) {
        return result;
    }
    if (HasObjectInReach(0, -1, 0)) {
        return 0;
    }
    ExchangeAbortPending(1);
    return 1;
}

// Spawns a second enemy group at the acting object's cell.
RVA(0x0001fb30, 0x42)
i16 SpawnActorGroup(void) {
    i16 object = GetLiveObject(g_actorId);
    MapCoord* pos;
    if (object < 0) {
        return -1;
    }
    pos = &((FieldActor*)GetFieldActor(object))->pos;
    SpawnSecondGroupActor(pos->x, pos->y, -1);
}

// Seals the target (flag 0x3f, result 3) unless it is human or of races
// 0x1a..0x22; while the field marker is set against an object the action
// fails with result 6.
RVA(0x0001fb80, 0xe4)
i16 SealTarget(void) {
    Character* actor = GetCombatant(g_actorId);
    Character* target;
    i16 race;
    if (g_targetId >= 0 && GetFieldMarker()) {
        SetActionResult(actor, 6);
        return 0;
    }
    target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    if (IsHumanCharacter(target)) {
        return 0;
    }
    race = GetDemonRace(target->id);
    if (race == 0x1a || race == 0x1b || race == 0x1c || race == 0x1d || race == 0x1e || race == 0x1f
        || race == 0x20 || race == 0x21 || race == 0x22) {
        return 0;
    }
    SetActionResult(actor, 3);
    SetCharacterFlag(target, 0x3f);
    return 1;
}

// Sets the target's flag 0x23 (not with 0x23 or 0x24 already set) and
// recalculates its stats.
RVA(0x0001fc70, 0x6d)
i16 RaiseTargetFlag23(void) {
    Character* target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    if (TestCharacterFlag(target, 0x23) == 1) {
        return -1;
    }
    if (TestCharacterFlag(target, 0x24) == 1) {
        return -1;
    }
    SetCharacterFlag(target, 0x23);
    RecalcCharacterStats(target);
    return 1;
}

// The same with flag 0x25, only while the moon is not new.
RVA(0x0001fce0, 0x67)
i16 RaiseTargetFlag25(void) {
    Character* target;
    if (!GetMoonPhase()) {
        return -1;
    }
    target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    if (TestCharacterFlag(target, 0x25) == 1) {
        return -1;
    }
    SetCharacterFlag(target, 0x25);
    RecalcCharacterStats(target);
    return 1;
}

RVA(0x0001fd50, 0x67)
i16 RaiseTargetFlag26(void) {
    Character* target;
    if (!GetMoonPhase()) {
        return -1;
    }
    target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    if (TestCharacterFlag(target, 0x26) == 1) {
        return -1;
    }
    SetCharacterFlag(target, 0x26);
    RecalcCharacterStats(target);
    return 1;
}
