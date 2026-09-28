#ifndef GITEN_GAME_STATUSSCREEN_H
#define GITEN_GAME_STATUSSCREEN_H

#include <rva.h>

#include <Game/Character.h>
#include <Ints.h>
#include <Ui/MenuBox.h>

// The roster slot the status screen shows.
extern i16 g_statusMember;
extern i16 g_statusFixedMember;
b16 RunStatusScreen(void);
b16 RunDismissMenuState(void);
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
// @identity-TODO: label storage extents are unproven.
extern char g_emptyBattleSkillLabel[];
// Full-width decimal labels, indexed by the displayed number.
extern char* g_statusNumberLabels[40];

// Draws the ten stat totals of `member` at x/y, coloured against `compare`
// when not NULL.
i16 DrawStatTotals(i16 x, i16 y, Character* member, Character* compare);

// Formats roster slot `slot` as status-list row `row` into the scratch
// buffer; returns the member's id, -1 when the row cannot be picked, -2 when
// its cost cannot be paid.
i16 FormatStatusLine(i16 slot, i16 row);
i16 SetStatusColumn(i16 column);
i16 RunStatusListPicker(i16 close);
i16 PickStatusMember(void);
MenuBox* CreateStatusListMenu(MenuBox* parent);
void StatusListMenuHandler(MenuBox* menu, i16 index, i16 event);
i16 BuildStatusSlots(void);

// Writes the letter of `value`'s alignment class on axis `axis` (0 CNL, 1 DNL)
// into `out`; returns `out`.
char* FormatAlignmentLetter(i16 axis, i16 value, char* out);

void SetStatusAnalyzeMode(i16 on);

// Pending roster insertion and the script state it resumes.
extern Character* g_rosterPendingMember;
extern i16 g_rosterReturnState;
extern u16 g_rosterReturnPhase;
extern u16 g_rosterReturnStep;
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
