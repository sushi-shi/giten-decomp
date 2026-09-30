#ifndef GITEN_GAME_STATUSSCREEN_H
#define GITEN_GAME_STATUSSCREEN_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/Character.h>
#include <Game/StateStack.h>
#include <Ints.h>
#include <Ui/MenuBox.h>

// The roster slot the status screen shows.
extern i16 g_statusMember;
extern b16 g_statusFixedMember;
b16 RunStatusScreen(void);
GZ_ENUM_BEGIN_SPLIT(DismissMenuPhase, i16)
    DISMISS_MENU_RESET_SELECTION = 0,
    DISMISS_MENU_WAIT_INPUT = 1,
    DISMISS_MENU_CLOSE = 2
GZ_ENUM_END_SPLIT(DismissMenuPhase)

b16 RunDismissMenuState(void);
// The status screen's steps (RunStatusCommands): the status menu returns the
// step of the command picked, so a command is a StatusStep too.
GZ_ENUM_BEGIN(StatusStep)
    STATUS_STEP_DRAW = 0,
    STATUS_STEP_POLL = 1,
    STATUS_STEP_CLOSE = 2,
    STATUS_STEP_ITEMS = 3,
    STATUS_STEP_SKILLS = 4,
    STATUS_STEP_STATS = 5,
    STATUS_STEP_NEXT_MEMBER = 6,
    STATUS_STEP_EXIT = 7,
    STATUS_STEP_EQUIPMENT = 8,
    STATUS_STEP_ATTACH = 9,
    STATUS_STEP_ALIGNMENT = 10
GZ_ENUM_END(StatusStep)

// What the status menu and pages return when no page command is due.
GZ_ENUM_CONST_BEGIN(StatusCommand)
    STATUS_COMMAND_NONE = -1,
    STATUS_COMMAND_CANCEL = -2,
    STATUS_COMMAND_CANCEL_FIXED_MEMBER = -3
GZ_ENUM_CONST_END(StatusCommand)

// RunStatusScreen's phases: open, close, pick the member, run the commands.
GZ_ENUM_BEGIN(StatusScreenPhase)
    STATUS_PHASE_OPEN = 0,
    STATUS_PHASE_CLOSE = 1,
    STATUS_PHASE_PICK_MEMBER = 2,
    STATUS_PHASE_COMMANDS = 3
GZ_ENUM_END(StatusScreenPhase)

// PickStatusMember's steps: show the list, finish with a member picked, poll,
// finish cancelled.
GZ_ENUM_CONST_BEGIN(PickMemberStep)
    PICK_MEMBER_STEP_OPEN = 0,
    PICK_MEMBER_STEP_PICKED = 1,
    PICK_MEMBER_STEP_POLL = 2,
    PICK_MEMBER_STEP_CANCELLED = 0xffff
GZ_ENUM_CONST_END(PickMemberStep)

i16 RunStatusCommands(void);
i16 GetStatusAnalyzeMode(void);

// The status screen's command menu (Game/statusmenu.c).
void ClearStatusMenu(void);
i16 PollStatusMenu(void);
// Sets or clears `flag` of item `item` of the status screen's command menu.
void SetStatusMenuItemFlag(i16 item, i16 flag, i16 on);

// Draws `text` at cell (x, y) of the status picture in attribute `attr`
// (DrawStatusText); returns the row two below.
i16 DrawStatusLine(i16 x, i16 y, const char* text, i32 attr);
i16 DrawStatusLabel(i16 x, i16 y, const char* text);
void ResetStatusMenu(void);
void CheckStatusMenuItem(i16 item);
void SetStatusMenuItemsHidden(i16 on);

// Eleven stat names followed by the null terminator.
extern char* g_statusStatNames[12];
extern char* g_statusBattleLabels[7];
// Full-width decimal labels, indexed by the displayed number.
extern char* g_statusNumberLabels[40];

// Draws the ten stat totals of `member` at x/y, coloured against `compare`
// when not NULL.
i16 DrawStatTotals(i16 x, i16 y, Character* member, Character* compare);

// Formats roster slot `slot` as status-list row `row` into the scratch
// buffer; returns the member's id, -1 when the row cannot be picked, -2 when
// its cost cannot be paid.
i16 FormatStatusLine(i16 slot, i16 row);
// The roster picker filter: all members, summonable reserve demons, reserve
// demons without flag 0x40, all demons without that flag, reserve demons, or
// any demon. Column 5 uses FormatStatusLine's default arm.
GZ_ENUM_BEGIN_SPLIT(StatusListColumn, i16)
    STATUS_LIST_ALL = 0,
    STATUS_LIST_SUMMONABLE = 1,
    STATUS_LIST_RESERVE_UNFLAGGED = 2,
    STATUS_LIST_UNFLAGGED = 3,
    STATUS_LIST_RESERVE = 4,
    STATUS_LIST_DEMONS = 5
GZ_ENUM_END_SPLIT(StatusListColumn)

GZ_ENUM_RETURN(StatusListColumn, i16) SetStatusColumn(GZ_ENUM_PARAM(StatusListColumn, i16) column);
i16 RunStatusListPicker(i16 close);
i16 PickStatusMember(void);
MenuBox* CreateStatusListMenu(MenuBox* parent);
void StatusListMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);
i16 BuildStatusSlots(void);

// Writes the letter of `value`'s alignment class on axis `axis` (0 CNL, 1 DNL)
// into `out`; returns `out`.
char* FormatAlignmentLetter(i16 axis, i16 value, char* out);

void SetStatusAnalyzeMode(b16 on);

// Pending roster insertion and the script state it resumes.
extern Character* g_rosterPendingMember;
extern i16 g_rosterReturnState;
extern u16 g_rosterReturnPhase;
extern u16 g_rosterReturnStep;

static __inline void SaveRosterReturnState(void) {
    g_rosterReturnState = GetGameState();
    g_rosterReturnPhase = GetGamePhase();
    g_rosterReturnStep = GetGameStep();
}

static __inline void RestoreRosterReturnState(void) {
    SetGameState(g_rosterReturnState);
    SetGamePhase(g_rosterReturnPhase);
    SetGameStep(g_rosterReturnStep);
}

b16 ReplaceRosterMember(void);

void EnterStatusScreen(i16 nested);

void LeaveStatusScreen(i16 nested);

void DrawStatusVitals(i16 slot);

// @identity-TODO: The roles of menu items 3/4/6/7/8/9 enabled via 0x419c0(item,0x800,on) are
// unrecovered.
void DrawStatusScreen(i16 slot);

i16 OpenStatListWindow(Character* character);
i16 RunStatPage(i16 command);
i16 RunAlignmentPage(i16 command);
void DrawBattleStatsPanel(i16 x, i16 y, Character* member, i16 hideIcons);

// Highlighting redraws the first half of the bar only when total >= 50;
// the normal path redraws the full bar and its text.
void DrawStatLine(Character* character, i16 stat, i16 highlight, i16 window);

#endif // GITEN_GAME_STATUSSCREEN_H
