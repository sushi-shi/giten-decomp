// @identity-TODO: the owning TU is unproven; this unit holds the analyze
// window's span until link-order evidence names it.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/Analyze.h>
#include <Game/AnalyzeData.h>
#include <Game/Character.h>
#include <Game/Condition.h>
#include <Game/DemonTable.h>
#include <Game/GameState.h>
#include <Game/Party.h>
#include <Game/SkillList.h>
#include <Game/StateStack.h>
#include <Game/StatusScreen.h>
#include <Input/Mouse.h>
#include <Script/EventFlags.h>
#include <Script/TextToken.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/MenuBox.h>
#include <Ui/Message.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// The five attitude names, indexed by Character.attitude.
DATA(0x00068bf8)
static char* s_attitudeNames[5] = {
    "\210\243\212\350\223I",      // 哀願的
    "\227F\215D\223I",            // 友好的
    "\222\264\223G\221\316\223I", // 超敵対的
    "\223G\221\316\223I",         // 敵対的
    "\222\312\217\355",           // 通常
};

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

// The yes/no menu of the "analyze in detail?" prompt.
DATA(0x0007d5cc)
static MenuBox* s_menu;

DATA(0x0007d5d0)
static Character* s_target;

// The window's step; -1 closes it.
DATA(0x0007d5d4)
static i16 s_step;

// Roster entry 15 while the detailed analysis borrows it.
DATA(0x0007d5d8)
static Character* s_savedRosterEntry;

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
