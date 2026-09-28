// @identity-TODO: the owning TU is unproven; this unit holds the debug menu
// at the start of the image until link-order evidence names it.

#include <rva.h>

#include <Game/DebugMenu.h>
#include <Game/FieldObject.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/ModeFlags.h>
#include <Game/Skill.h>
#include <Game/StateStack.h>
#include <Game/WorldMap.h>
#include <Gfx/Shot.h>
#include <Input/Mouse.h>
#include <Platform/PlatformApi.h>
#include <Script/ScriptVars.h>
#include <Text/TextWindow.h>
#include <Ui/MenuBox.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>

// The open menu, the skill the magic test launches, and how many skills
// there are.
DATA(0x00071170)
static MenuBox* s_debugMenu = 0;

DATA(0x00071174)
static i16 s_testSkill = 0;

DATA(0x00071178)
static i16 s_skillCount = 0;

// The label of the debug menu's unnamed rows.
DATA(0x0007117c)
static char s_emptyLabels[4][4] = {0};

// The debug menu: each row starts scene 0xaf (0xd1 for the last) with its
// value, except the magic test (row 2) and destroying every demon (row 4).
DATA(0x000641f0)
static const MenuEntry s_debugEntries[15] = {
    {0, 0, "\202\141\202\146\202\154"},         // "ＢＧＭ"
    {0, 1, "\202\162\202\144"},                 // "ＳＥ"
    {1, 3, "\226\202\226\100\214\370\211\312"}, // "魔法効果" (magic effects)
    {1, 4, s_emptyLabels[0]},
    {1, 6, "\210\253\226\202\221\123\226\305"}, // "悪魔全滅" (all demons destroyed)
    {1, 7, s_emptyLabels[1]},
    {0, 8, s_emptyLabels[2]},
    {0, 10, "\203\101\203\103\203\145\203\200\216\346\223\276"}, // "アイテム取得" (get items)
    {0, 12, "\224\134\227\315\222\154\225\317\211\273"},         // "能力値変化" (change stats)
    {0, 29, "\202\122\202\143\210\332\223\256"},                 // "３Ｄ移動" (3D move)
    {0, 32, "\203\164\203\211\203\117\225\317\215\130"},         // "フラグ変更" (change flags)
    {1, 36, "\203\132\201\133\203\165"},                         // "セーブ" (save)
    {1, 38, "\203\215\201\133\203\150"},                         // "ロード" (load)
    {0, 6, "\203\146\201\133\203\136\212\155\224\106"},          // "データ確認" (check data)
    {0, 0, s_emptyLabels[3]},
};

// The magic test's rows: step the skill id, set the shot's rise, launch it.
DATA(0x00064250)
static const MenuEntry s_magicEntries[8] = {
    {0, 1, "\201\173\202\120"},                    // "＋１"
    {0, -1, "\201\174\202\120"},                   // "－１"
    {0, 10, "\201\173\202\120\202\117"},           // "＋１０"
    {0, -10, "\201\174\202\120\202\117"},          // "－１０"
    {0, 100, "\201\173\202\120\202\117\202\117"},  // "＋１００"
    {0, -100, "\201\174\202\120\202\117\202\117"}, // "－１００"
    {0, 0, "\213\227\227\243"},                    // "距離" (distance)
    {0, 0, "\216\300\215\163"},                    // "実行" (run)
};

// The magic test's shot rise (0..3, the distance row cycles it).
DATA(0x00068050)
static i16 s_shotRise = 3;

static void DebugMenuHandler(MenuBox* menu, i16 index, i16 event);
static void MagicMenuHandler(MenuBox* menu, i16 index, i16 event);

// The debug menu's game state (step 0 opens it, 1 closes it, 2 runs it,
// 3.. run the picked row; row 2's magic test runs by sub-state).
RVA(0x00001000, 0x410)
i16 RunDebugMenu(void) {
    i16 pick;
    i16 row;
    MapCoord from;
    MapCoord to;

    switch (GetGameStep()) {
        case 0:
            s_testSkill = 0x10;
            s_shotRise = 3;
            NextGameStep();
            NextGameStep();
            s_debugMenu = CreateMenuBox(s_debugMenu, 0x19, 2);
            MoveMenuBox(s_debugMenu, -8, -0x16);
            SetMenuItems(s_debugMenu, 8, s_debugEntries, 15, DebugMenuHandler);
            return 0;
        case 1:
            s_debugMenu = DestroyMenuBox(s_debugMenu);
            HideTextPlane(0);
            return -1;
        case 2:
            pick = RunMenu(s_debugMenu);
            if (pick == -1) {
                PrevGameStep();
            }
            if (pick > 0) {
                SetGameStep(g_selectedObjectId + 3);
                s_debugMenu = DestroyMenuBox(s_debugMenu);
                return 0;
            }
            break;
        case 7:
            SetGameStep(1);
            ResetObjectAnims();
            return 0;
        case 9:
        case 12:
        case 15:
            row = GetGameStep() - 3;
            SetGameStep(1);
            StartDebugScene(0xaf, s_debugEntries[row].value, g_infoPlane);
            return 0;
        case 16:
            row = GetGameStep() - 3;
            SetGameStep(0);
            StartDebugScene(0xd1, s_debugEntries[row].value, g_infoPlane);
            return 0;
        case 3:
        case 4:
        case 6:
        case 8:
        case 10:
        case 11:
        case 13:
        case 14:
            row = GetGameStep() - 3;
            SetGameStep(0);
            StartDebugScene(0xaf, s_debugEntries[row].value, g_infoPlane);
            return 0;
        case 17:
            SetGameStep(0);
            return 0;
        case 5:
            switch (GetGameSub()) {
                case 0:
                    s_skillCount = GetSkillCount();
                    NextGameSub();
                    NextGameSub();
                    s_debugMenu = CreateMenuBox(s_debugMenu, 0x19, 2);
                    MoveMenuBox(s_debugMenu, -8, -0x16);
                    SetMenuItems(s_debugMenu, 8, s_magicEntries, 8, MagicMenuHandler);
                    return 0;
                case 1:
                    s_debugMenu = DestroyMenuBox(s_debugMenu);
                    SetGameStep(0);
                    return 0;
                case 2:
                    pick = RunMenu(s_debugMenu);
                    if (pick == -1) {
                        PrevGameSub();
                    }
                    if (pick > 0) {
                        SetGameSub(g_selectedObjectId + 3);
                        return 0;
                    }
                    break;
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                    row = GetGameSub() - 3;
                    s_testSkill += s_magicEntries[row].value;
                    SetGameSub(2);
                    s_debugMenu->flags |= 1;
                    return 0;
                case 9:
                    SetGameSub(2);
                    s_debugMenu->flags |= 1;
                    if (++s_shotRise > 3) {
                        s_shotRise = 0;
                        return 0;
                    }
                    break;
                case 10:
                    s_debugMenu = DestroyMenuBox(s_debugMenu);
                    SetGameSub(0);
                    from = GetMapCoord();
                    to = MoveMapCoord(from, g_field.pos.direction, 0, -s_shotRise);
                    PushGameState(0xc);
                    LaunchShot(
                        GetSkillShotId(s_testSkill),
                        0,
                        s_shotRise,
                        from.x,
                        from.y,
                        to.x,
                        to.y
                    );
                    break;
            }
            break;
    }
    return 0;
}

RVA(0x00001410, 0xc0)
static void DebugMenuHandler(MenuBox* menu, i16 index, i16 event) {
    MenuEntry* entries = menu->items.table;
    MenuEntry* entry;

    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->items.table = NULL;
            menu->itemCount = 0;
            break;
        case MENU_EVENT_BEGIN_PAGE:
            // "<デバッグメニュー>" (debug menu)
            sprintf(
                g_scratchBuffer,
                "<\203\146\203\157\203\142\203\117\203\201\203\152\203\205\201\133>"
            );
            AddMenuLine(menu->plane, g_scratchBuffer, 0x400, -1, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            entry = &entries[index];
            if (entry->restricted && TestModeFlags(MODE_WORLD_MAP)) {
                AddMenuLine(menu->plane, entry->label, 0x500, index, 1);
            } else {
                AddMenuLine(menu->plane, entry->label, 0x2450, index, 0);
            }
            break;
    }
}

RVA(0x000014d0, 0x150)
static void MagicMenuHandler(MenuBox* menu, i16 index, i16 event) {
    MenuEntry* entries = menu->items.table;
    i16 skill;

    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->items.table = NULL;
            menu->itemCount = 0;
            return;
        case MENU_EVENT_BEGIN_PAGE:
            // "<魔法デバッグ>" (magic debug)
            sprintf(g_scratchBuffer, "<\226\202\226\100\203\146\203\157\203\142\203\117>");
            AddMenuLine(menu->plane, g_scratchBuffer, 0x400, -1, 1);
            return;
        case MENU_EVENT_ADD_ROW:
            if (index == 6) {
                sprintf(g_scratchBuffer, "%s(%.1d) +1", entries[6].label, s_shotRise);
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2450, 6, 0);
                return;
            }
            if (index <= 5) {
                skill = entries[index].value + s_testSkill;
                if (skill < 0 || skill >= s_skillCount) {
                    AddMenuLine(menu->plane, entries[index].label, 0x2560, index, 1);
                    return;
                }
            }
            AddMenuLine(menu->plane, entries[index].label, 0x2450, index, 0);
            return;
        case MENU_EVENT_END_PAGE:
            sprintf(g_scratchBuffer, "%3d %s", s_testSkill, GetSkillName(s_testSkill));
            AddMenuLine(menu->plane, g_scratchBuffer, 0x400, index, 1);
            return;
    }
}
