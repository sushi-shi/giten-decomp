// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/AreaMap.h>
#include <Game/BagItems.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Clock.h>
#include <Game/DebugMenu.h>
#include <Game/Familiarity.h>
#include <Game/FieldMain.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldView.h>
#include <Game/GameLoop.h>
#include <Game/GameState.h>
#include <Game/GemItems.h>
#include <Game/ModeFlags.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/PlayTime.h>
#include <Game/SaveGame.h>
#include <Game/StateStack.h>
#include <Game/WorldMap.h>
#include <Input/Mouse.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Text/TextAttr.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Ui/MenuBox.h>
#include <Ui/MenuStep.h>
#include <Util/Scratch.h>

#include <stdio.h>
#include <string.h>

// The system menu's rows; picking row n runs phase MENU_STEP_PICK_FIRST + n.
DATA(0x00068310)
static i32 s_systemEntryCount = 3;
DATA(0x00068318)
static SystemMenuEntry s_systemEntries[4] = {
    // "オートマッピング" (auto-mapping)
    {false, "\203\111\201\133\203\147\203\175\203\142\203\163\203\223\203\117"},
    // "オートナビゲーション" (auto-navigation)
    {false, "\203\111\201\133\203\147\203\151\203\162\203\121\201\133\203\126\203\207\203\223"},
    // "ゲーム中断" (quit the game)
    {false, "\203\121\201\133\203\200\222\206\222\146"},
    {false, NULL},
};

// The quit confirmation's rows.
DATA(0x00068330)
static SystemMenuEntry s_quitEntries[2] = {
    {false, "\222\206\222\146\202\267\202\351"},         // "中断する" (quit)
    {false, "\222\206\222\146\202\265\202\310\202\242"}, // "中断しない" (don't quit)
};

// The auto-mapping and auto-navigation display choices.
DATA(0x00068340)
static SystemMenuEntry s_displayEntries[2] = {
    {false, "\216\251\227\122\225\134\216\246"}, // "自由表示" (free display)
    {false, "\214\305\222\350\225\134\216\246"}, // "固定表示" (fixed display)
};

DATA(0x00076050)
b16 g_loadedBefore = false;

DATA(0x00076054)
static MenuBox* s_systemMenu = NULL;

static void SystemMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);
static b16 RunDisplayChoice(void);
static b16 RunQuitConfirm(void);

// Opens the system menu's box listing `count` rows of `entries`.
static __inline void OpenSystemMenu(SystemMenuEntry* entries, i32 count) {
    s_systemMenu = CreateMenuBox(s_systemMenu, 0x19, 2);
    MoveMenuBox(s_systemMenu, -8, -0x16);
    SetMenuItems(s_systemMenu, 9, entries, count, SystemMenuHandler);
}

// Saves slot `slot` (the party's mark recorded in the leader first): the
// header, then each writer in LoadGame's order; the OR of their error counts.
RVA(0x00003a00, 0xd6)
i16 SaveGame(i16 slot) {
    FILE* fp;
    i16 errors;
    RecordMarkInLeader();
    GetSavePath(g_scratchBuffer, slot);
    fp = fopen(g_scratchBuffer, "wb");
    errors = WriteSaveHeader(fp);
    errors |= WriteEventFlags(fp);
    errors |= SaveFamiliarityCounts(fp);
    errors |= WriteBag(fp);
    errors |= WriteGemItems(fp);
    errors |= SavePlayTime(fp);
    errors |= WriteCharacters(fp);
    errors |= WriteFieldState(fp);
    errors |= SaveClock(fp);
    errors |= SaveAnalyzed(fp);
    errors |= WriteAutomapAreas(fp);
    errors |= WriteScriptVars(fp);
    errors |= SaveFieldMemory(fp);
    errors |= SaveScreenLayers(fp);
    fclose(fp);
    return errors;
}

// Writes the save-file header ReadSaveHeader skips: the leader's full name
// and level, the format version (4), the area name and the displayed floor.
RVA(0x00003ae0, 0x10d)
i16 WriteSaveHeader(FILE* fp) {
    Character* leader = GetCharacter(0);
    u8 value;
    i16 floor;
    i16 failed;
    memset(g_scratchBuffer, 0, SAVE_TEXT_SIZE);
    FormatFullName(g_scratchBuffer, leader);
    failed = SAVE_TEXT_SIZE - fwrite(g_scratchBuffer, 1, SAVE_TEXT_SIZE, fp);
    value = leader->level;
    failed |= 1 - fwrite(&value, 1, 1, fp);
    value = SAVE_FORMAT_VERSION;
    failed |= 1 - fwrite(&value, 1, 1, fp);
    memset(g_scratchBuffer, 0, SAVE_TEXT_SIZE);
    strcpy(g_scratchBuffer, GetAreaName());
    failed |= SAVE_TEXT_SIZE - fwrite(g_scratchBuffer, 1, SAVE_TEXT_SIZE, fp);
    floor = GetLevelFloor();
    failed |= 1 - fwrite(&floor, 2, 1, fp);
    return failed;
}

// Marks the party's position (area, level, x, y, direction) in the roster
// leader (see Character.markPosition).
RVA(0x00003bf0, 0x38)
void RecordMarkInLeader(void) {
    Character* leader = GetRosterCharacter(ROSTER_LEADER);
    SetSavedMapPosition(
        &leader->markPosition,
        g_party.field.pos.area,
        g_party.field.pos.level,
        g_party.field.pos.x,
        g_party.field.pos.y,
        g_party.field.pos.direction
    );
}

// Loads save slot `slot` (without `keepField`, resetting the field objects
// and scene first); returns the OR of the loaders' error counts, or 0 when
// the file cannot be opened. After the first load the party faces back and
// steps one cell; every roster member's equipment group is re-read and its
// slots normalised, and the return point is set to the party's cell.
RVA(0x00003c30, 0x16d)
i16 LoadGame(i16 slot, b16 keepField) {
    FILE* fp;
    i16 errors;
    i16 i;
    if (!keepField) {
        ResetFieldObjects();
        ResetFieldScene();
    }
    GetSavePath(g_scratchBuffer, slot);
    fp = fopen(g_scratchBuffer, "rb");
    if (!fp) {
        return 0;
    }
    errors = ReadSaveHeader(fp);
    errors |= ReadEventFlags(fp);
    errors |= LoadFamiliarityCounts(fp);
    errors |= ReadBag(fp);
    errors |= ReadGemItems(fp);
    errors |= LoadPlayTime(fp);
    errors |= LoadCharacters(fp);
    errors |= LoadFieldState(fp);
    errors |= LoadClock(fp);
    errors |= LoadAnalyzed(fp);
    errors |= LoadAutomapAreas(fp);
    errors |= ReadScriptVars(fp);
    errors |= LoadFieldMemory(fp);
    errors |= LoadScreenLayers(fp);
    fclose(fp);
    if (!g_loadedBefore) {
        g_party.field.pos.direction += 2;
        g_party.field.pos.direction &= 3;
        OffsetMapCoordFacing(
            &g_party.field.pos.x,
            &g_party.field.pos.y,
            g_party.field.pos.direction,
            0,
            -1
        );
    }
    CompactBag();
    for (i = 0; i < ROSTER_SIZE; i++) {
        Character* character = GetRosterCharacter(i);
        if (character) {
            character->equipGroup = ReadObjectRecordField(character->id, 0x20, 2);
            NormalizeEquipSlots(character);
        }
    }
    g_loadedBefore = false;
    ReturnToCurrentCell();
    return errors;
}

// Skips the save-file header (two 32-byte text fields and three small
// fields). Returns -1 on a short read, else 4 minus the second header byte.
// @identity-TODO: what the header fields mean is not recovered.
RVA(0x00003da0, 0xae)
i16 ReadSaveHeader(FILE* fp) {
    u8 value;
    u16 word;
    i16 failed;
    failed = SAVE_TEXT_SIZE - fread(g_scratchBuffer, 1, SAVE_TEXT_SIZE, fp);
    failed |= 1 - fread(&value, 1, 1, fp);
    failed |= 1 - fread(&value, 1, 1, fp);
    failed |= SAVE_TEXT_SIZE - fread(g_scratchBuffer, 1, SAVE_TEXT_SIZE, fp);
    failed |= 1 - fread(&word, 2, 1, fp);
    if (failed) {
        return -1;
    }
    return SAVE_FORMAT_VERSION - value;
}

// The system menu's game state: phase 0 opens it, 1 closes it and returns, 2
// runs it; 3 and 4 pick the auto-mapping and auto-navigation display, 5
// confirms quitting and 6 runs the debug menu.
RVA(0x00003e50, 0x120)
b16 RunSystemMenu(void) {
    i16 pick;

    switch (GetGamePhase()) {
        case MENU_STEP_OPEN:
            NextGamePhase();
            NextGamePhase();
            OpenSystemMenu(s_systemEntries, s_systemEntryCount);
            return false;
        case MENU_STEP_CLOSE:
            ReturnFromGameState();
            s_systemMenu = DestroyMenuBox(s_systemMenu);
            return false;
        case MENU_STEP_RUN:
            pick = RunMenu(s_systemMenu);
            if (pick == TEXT_EVENT_CANCEL) {
                PrevGamePhase();
            }
            if (pick > 0) {
                SetGamePhase(g_selectedObjectId + MENU_STEP_PICK_FIRST);
                s_systemMenu = DestroyMenuBox(s_systemMenu);
                return false;
            }
            break;
        case MENU_STEP_PICK_FIRST + SYSTEM_ROW_AUTO_MAPPING:
        case MENU_STEP_PICK_FIRST + SYSTEM_ROW_AUTO_NAVIGATION:
            return RunDisplayChoice();
        case MENU_STEP_PICK_FIRST + SYSTEM_ROW_QUIT:
            return RunQuitConfirm();
        case MENU_STEP_PICK_FIRST + SYSTEM_ROW_DEBUG:
            if (RunDebugMenu() < SUBSTATE_RUNNING) {
                SetGamePhase(MENU_STEP_CLOSE);
            }
            break;
    }
    return false;
}

// Heads the menu with "<SYSTEM>" and the picked row's label, and lists the
// rows of the table the menu shows.
RVA(0x00003f70, 0x100)
static void SystemMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    i16 phase = GetGamePhase();
    SystemMenuEntry* entries = menu->items.systemTable;

    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->items.systemTable = NULL;
            menu->itemCount = 0;
            break;
        case MENU_EVENT_BEGIN_PAGE:
            if (phase >= MENU_STEP_PICK_FIRST) {
                sprintf(
                    g_scratchBuffer,
                    "<SYSTEM> %s",
                    s_systemEntries[phase - MENU_STEP_PICK_FIRST].label
                );
            } else {
                sprintf(g_scratchBuffer, "<SYSTEM>");
            }
            AddMenuLine(menu->plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, -1, MENU_LINE_DISABLED);
            break;
        case MENU_EVENT_ADD_ROW:
            if (TestModeFlags(MODE_WORLD_MAP) && entries[index].restricted) {
                AddMenuLine(
                    menu->plane,
                    entries[index].label,
                    TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
                    index,
                    MENU_LINE_DISABLED
                );
            } else {
                AddMenuLine(
                    menu->plane,
                    entries[index].label,
                    TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_BLACK),
                    index,
                    MENU_LINE_NORMAL
                );
            }
            break;
    }
}

// Picks free or fixed display for auto-mapping or auto-navigation, then asks
// for a field redraw and closes the system menu.
RVA(0x00004070, 0x100)
static b16 RunDisplayChoice(void) {
    GZ_ENUM_LOCAL(TextEvent, i16) pick;

    switch (GetGameStep()) {
        case SYSTEM_CHOICE_OPEN:
            NextGameStep();
            OpenSystemMenu(s_displayEntries, 2);
            break;
        case SYSTEM_CHOICE_POLL:
            pick = RunMenu(s_systemMenu);
            if (pick == TEXT_EVENT_NONE) {
                break;
            }
            if (pick == TEXT_EVENT_CANCEL) {
                SetGamePhase(MENU_STEP_OPEN);
            } else {
                if (GetGamePhase() == MENU_STEP_PICK_FIRST + SYSTEM_ROW_AUTO_MAPPING) {
                    g_party.status.automapFixed = g_selectedObjectId;
                } else {
                    g_party.status.navigationFixed = g_selectedObjectId;
                }
                g_fieldRedrawRequest = true;
                SetGamePhase(MENU_STEP_CLOSE);
            }
            s_systemMenu = DestroyMenuBox(s_systemMenu);
            return false;
    }
    return false;
}

// Asks whether to quit; "quit" requests the game's end and a field redraw.
RVA(0x00004170, 0xc0)
static b16 RunQuitConfirm(void) {
    GZ_ENUM_LOCAL(TextEvent, i16) pick;

    switch (GetGameStep()) {
        case SYSTEM_CHOICE_OPEN:
            NextGameStep();
            OpenSystemMenu(s_quitEntries, 2);
            break;
        case SYSTEM_CHOICE_POLL:
            pick = RunMenu(s_systemMenu);
            if (pick == TEXT_EVENT_NONE) {
                break;
            }
            if (pick == TEXT_EVENT_CANCEL) {
                SetGamePhase(MENU_STEP_OPEN);
            } else {
                if (g_selectedObjectId == 0) {
                    g_quitRequest = 1;
                    g_fieldRedrawRequest = true;
                }
                SetGamePhase(MENU_STEP_CLOSE);
            }
            s_systemMenu = DestroyMenuBox(s_systemMenu);
            return false;
    }
    return false;
}

// Reads slot `slot`'s header up to `field` into g_scratchBuffer for the save
// menu: 0 the leader's name, 1 "LV nn", 2 the area name, 3 the level's floor
// ("Bn" below ground, "nF" above, empty at 0). Returns `slot`, or -1 when the
// file cannot be opened.
RVA(0x00004230, 0x13f)
i16 ReadSaveSummary(i16 slot, GZ_ENUM_PARAM(SaveSummaryField, i16) field) {
    FILE* fp;
    u8 value;
    i16 floor;
    GetSavePath(g_scratchBuffer, slot);
    fp = fopen(g_scratchBuffer, "rb");
    if (fp == NULL) {
        return -1;
    }
    fread(g_scratchBuffer, 1, SAVE_TEXT_SIZE, fp);
    if (field != SAVE_SUMMARY_NAME) {
        fread(&value, 1, 1, fp);
        sprintf(g_scratchBuffer, "LV %2d", value);
        if (field != SAVE_SUMMARY_LEVEL) {
            fread(&value, 1, 1, fp);
            fread(g_scratchBuffer, 1, SAVE_TEXT_SIZE, fp);
            if (field != SAVE_SUMMARY_AREA) {
                fread(&floor, 2, 1, fp);
                if (floor < 0) {
                    sprintf(g_scratchBuffer, "B%1d", -floor);
                } else if (floor > 0) {
                    sprintf(g_scratchBuffer, "%1dF", floor);
                } else {
                    g_scratchBuffer[0] = 0;
                }
            }
        }
    }
    fclose(fp);
    return slot;
}

// @identity-TODO: the startup flag 0x80 has no reader in the claimed code.
DATA(0x000683f0)
static i16 s_modeFlags = 0x80;

RVA(0x00004370, 0xe)
i16 TestFeatureMask(i16 bits) {
    return g_featureMask & bits;
}

RVA(0x00004380, 0xc)
i16 TestModeFlags(GZ_ENUM_PARAM(ModeFlags, i16) bits) {
    return bits & s_modeFlags;
}

RVA(0x00004390, 0x12)
i16 SetModeFlags(GZ_ENUM_PARAM(ModeFlags, i16) bits) {
    return s_modeFlags |= bits;
}

RVA(0x000043b0, 0x16)
i16 ClearModeFlags(GZ_ENUM_PARAM(ModeFlags, i16) bits) {
    return s_modeFlags &= ~bits;
}
