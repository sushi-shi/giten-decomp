// @identity-TODO: the original TU name is unproven. One object: its .data
// holds the status panel's and the equipment page's variables ahead of both
// parts' literals, and the alignment-chart helper's constants follow the
// equipment page's in .rdata.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/BagItems.h>
#include <Game/CharInfo.h>
#include <Game/Character.h>
#include <Game/ClickWait.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/EquipRequirements.h>
#include <Game/EquipScreen.h>
#include <Game/EquipSlotIndex.h>
#include <Game/FieldSight.h>
#include <Game/GameState.h>
#include <Game/GemItems.h>
#include <Game/HumanId.h>
#include <Game/ItemBag.h>
#include <Game/ItemBonus.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/Skill.h>
#include <Game/StatBarRows.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Game/StatusScreen.h>
#include <Game/WaitState.h>
#include <Gfx/Background.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/Vram.h>
#include <Input/Mouse.h>
#include <Ints.h>
#include <Mem/Alloc.h>
#include <Script/EventFlags.h>
#include <Script/ScenarioFlag.h>
#include <Script/ScriptOps.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/Menu.h>
#include <Ui/MenuBox.h>
#include <Ui/MenuStep.h>
#include <Ui/Panel.h>
#include <Util/Range.h>
#include <Util/Scratch.h>
#include <Util/WordList.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x0006a0f8)
static i16 s_statusCommandHotspots[11] = {-1, -1, -1, 55, 56, 57, -1, -1, 60, 61, 62};

DATA(0x0006499c)
static const i8 s_battleStatIcons[4] = {0, 1, 8, 9};

// A status sub-page's state: its text plane and the sub-state to resume (then
// the command picked). One record: the two fields sit two bytes apart, where
// separate variables take four-byte slots.
typedef struct StatusPage {
    i16 plane;
    i16 resume;
} StatusPage;

DATA(0x0006a110)
static StatusPage s_statPage = {-1, -1};

DATA(0x0006a118)
char* g_statusStatNames[12] = {
    "\222\274  \212\264",
    "\220\270\220_\227\315",
    "\226\202  \227\315",
    "\222m  \227\315",
    "\211\301  \214\354",
    "\213\255  \202\263",
    "\221\314  \227\315",
    "\225q\217\267\220\253",
    "\212\355\227p\202\263",
    "\226\243  \227\315",
    "\226\275  \211^",
    NULL,
};

DATA(0x0006a148)
char* g_statusBattleLabels[7] = {
    "\213\132\224\134",
    "\226\275\222\206",
    "\215\125\214\202",
    "\211\361\224\360",
    "\226\150\214\344",
    "\222\145\220\224",
    NULL,
};

DATA(0x0006a164)
static StatusPage s_alignmentPage = {-1, -1};

DATA(0x0006a168)
char* g_statusNumberLabels[40] = {
    "\202O",      "\202P",      "\202Q",      "\202R",      "\202S",      "\202T",
    "\202U",      "\202V",      "\202W",      "\202X",      "\202P\202O", "\202P\202P",
    "\202P\202Q", "\202P\202R", "\202P\202S", "\202P\202T", "\202P\202U", "\202P\202V",
    "\202P\202W", "\202P\202X", "\202Q\202O", "\202Q\202P", "\202Q\202Q", "\202Q\202R",
    "\202Q\202S", "\202Q\202T", "\202Q\202U", "\202Q\202V", "\202Q\202W", "\202Q\202X",
    "\202R\202O", "\202R\202P", "\202R\202Q", "\202R\202R", "\202R\202S", "\202R\202T",
    "\202R\202U", "\202R\202V", "\202R\202W", "\202R\202X",
};

RVA(0x00041ad0, 0xc5)
static i16 DrawStatusExperience(i16 x, i16 y, Character* member) {
    i32 remaining;

    sprintf(g_scratchBuffer, "EXP %10ld", member->experience);
    y = DrawStatusLine(
        x,
        y,
        g_scratchBuffer,
        TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
    if (member->level >= 99) {
        strcpy(g_scratchBuffer, "\215\305\215\202\203\214\203\170\203\213\202\305\202\267");
    } else {
        remaining = ExperienceForLevel(member->level + 1, member->id) - member->experience;
        if (remaining < 0) {
            remaining = 0;
        }
        sprintf(g_scratchBuffer, "NEXT %9ld", remaining);
    }
    return DrawStatusLine(
        x,
        y,
        g_scratchBuffer,
        TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
}

RVA(0x00041ba0, 0x89)
static i16 DrawStatusPools(i16 x, i16 y, Character* member) {
    sprintf(g_scratchBuffer, "HP  %4d\201\136%4d", member->pools.hp.cur, member->pools.hp.max);
    y = DrawStatusLine(
        x,
        y,
        g_scratchBuffer,
        TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
    sprintf(g_scratchBuffer, "MP    %3d\201\136%3d", member->pools.mp.cur, member->pools.mp.max);
    return DrawStatusLine(
        x,
        y,
        g_scratchBuffer,
        TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
}

RVA(0x00041c30, 0xb3)
static i16 DrawStatusLevel(i16 x, i16 y, Character* member) {
    sprintf(g_scratchBuffer, "LEVEL %8d", member->level);
    y = DrawStatusLine(
        x,
        y,
        g_scratchBuffer,
        TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
    if (IsHumanCharacter(member)) {
        strcpy(g_scratchBuffer, GetHumanTitleName(member->title));
    } else {
        strcpy(g_scratchBuffer, GetDemonPantheonName(member->id));
    }
    return DrawStatusLabel(x, y, g_scratchBuffer);
}

RVA(0x00041cf0, 0x5b)
void DrawStatusVitals(i16 slot) {
    Character* member = GetRosterCharacter(slot);

    if (member) {
        DrawStatusLevel(3, 8, member);
        DrawStatusPools(3, 12, member);
        DrawStatusExperience(3, 18, member);
        DrawStatTotals(3, 25, member, NULL);
        DrawBattleStatsPanel(52, 36, member, 0);
    }
}

RVA(0x00041d50, 0x92)
static i16 DrawStatusConditions(i16 x, i16 y, Character* member) {
    i16 cursor = 0;
    i16 entry;
    const char* name;

    y = DrawStatusLabel(x + 2, y, "CONDITIONS");
    for (entry = 0; entry < 18; entry++) {
        name = NextConditionName(GetCharacterConditions(member), &cursor);
        if (cursor < 0) {
            break;
        }
        DrawStatusLabel(x + entry % 3 * 7, y + entry / 3 * 2, name);
        cursor++;
    }
    return y;
}

RVA(0x00041df0, 0xe1)
static void DrawStatusMemberPortrait(i16 x, i16 y, Character* member) {
    ObjectPicture picture;
    ImageRequest request;
    void* image;
    i32 size;
    i16 file;
    i16 mode;

    if (!member) {
        return;
    }
    picture = GetObjectRecordPicture(member->id);
    if (IsHumanCharacter(member)) {
        file = picture.index + 0x4000;
        mode = picture.variant;
        if (member->id == HUMAN_KATSURAGI) {
            if (!IsEventFlagSet(
                    EVENT_FLAG_BANK_SCENARIO_2,
                    SCENARIO_2_KATSURAGI_CIVILIAN_PORTRAIT
                )) {
                file = 0x4001;
                mode = 1;
            }
        } else if (member->id == HUMAN_TACHIBANA) {
            if (!IsEventFlagSet(
                    EVENT_FLAG_BANK_SCENARIO_2,
                    SCENARIO_2_TACHIBANA_CIVILIAN_PORTRAIT
                )) {
                file = 0x4002;
                mode = 1;
            }
        }
        request.file = file;
        request.variant = 0;
        request.flags = 0;
        image = LoadImageRequest(&request, mode);
    } else {
        request.file = picture.index * 16 + 0x2002;
        request.variant = picture.variant;
        request.flags = 0;
        image = LoadImageFile(&request, &size);
    }
    DrawStatusPortrait(image);
    FreeImageFile(image);
}

RVA(0x00041ee0, 0x6c)
static i16 DrawStatusCapacity(i16 x, i16 y, Character* member) {
    if (IsHumanCharacter(member)) {
        sprintf(
            g_scratchBuffer,
            "\222\207\226\202 %5d\201\136%2d",
            CountRosterEntries(false),
            GetRosterCapacity() - 6
        );
    } else {
        sprintf(g_scratchBuffer, "CP %11d", member->levelBonus);
    }
    return DrawStatusLabel(x, y, g_scratchBuffer);
}

RVA(0x00041f50, 0x131)
static i16 DrawStatusName(i16 x, i16 y, Character* member) {
    char name[36];

    sprintf(g_scratchBuffer, "%-16.16s", FormatFullName(name, member));
    y = DrawStatusLabel(x, y, g_scratchBuffer);
    strcpy(g_scratchBuffer, GetDemonRaceName(member->id));
    y += 2;
    if (IsHumanCharacter(member)) {
        return DrawStatusLabel(x, y, g_scratchBuffer);
    }
    name[0] = 0;
    strcat(name, GetDemonClassName(member->id));
    strcat(name, ":");
    strcat(name, g_scratchBuffer);
    return DrawStatusLabel(x, y, name);
}

RVA(0x00042090, 0x16d)
void DrawStatusScreen(i16 slot) {
    Character* member;

    if (g_statusMember < 0) {
        g_statusMember = 0;
    }
    if (!g_statusFixedMember && ListEquipCandidates(g_statusMember, 1)) {
        SetStatusMenuItemFlag(STATUS_STEP_EQUIPMENT, 0x800, false);
    } else {
        SetStatusMenuItemFlag(STATUS_STEP_EQUIPMENT, 0x800, true);
    }
    member = GetRosterCharacter(g_statusMember);
    if (!GetWordCount(GetCharacterSkills(member))) {
        SetStatusMenuItemFlag(STATUS_STEP_SKILLS, 0x800, true);
    } else {
        SetStatusMenuItemFlag(STATUS_STEP_SKILLS, 0x800, false);
    }
    if (!g_statusFixedMember && CountRosterEntries(true) >= 2) {
        SetStatusMenuItemFlag(STATUS_STEP_NEXT_MEMBER, 0x800, false);
    } else {
        SetStatusMenuItemFlag(STATUS_STEP_NEXT_MEMBER, 0x800, true);
    }
    if (g_statusFixedMember) {
        SetStatusMenuItemFlag(STATUS_STEP_EXIT, 0x800, true);
        SetStatusMenuItemFlag(STATUS_STEP_ATTACH, 0x800, true);
        SetStatusMenuItemFlag(STATUS_STEP_ITEMS, 0x800, true);
    } else {
        SetStatusMenuItemFlag(STATUS_STEP_EXIT, 0x800, false);
        SetStatusMenuItemFlag(STATUS_STEP_ATTACH, 0x800, false);
        SetStatusMenuItemFlag(STATUS_STEP_ITEMS, 0x800, false);
    }
    ClearStatusMenu();
    member = GetRosterCharacter(slot);
    if (member) {
        DrawStatusName(3, 2, member);
        DrawStatusCapacity(3, 16, member);
        DrawStatusConditions(26, 37, member);
        DrawEquipLines(member, 52, 2);
        DrawStatusVitals(slot);
        DrawStatusMemberPortrait(36, 222, member);
    }
}

static b16 ResumeStatusPage(i16 command);

RVA(0x00042200, 0x1d4)
i16 RunStatusCommands(void) {
    i16 command;

    if (GetGameStep() >= STATUS_STEP_POLL) {
        command = PollStatusMenu();
    } else {
        command = STATUS_COMMAND_NONE;
    }
    if (g_previousStatusStep != GetGameStep()) {
        if (g_previousStatusStep == STATUS_STEP_EQUIPMENT) {
            DrawStatTotals(3, 25, GetRosterCharacter(g_statusMember), NULL);
        }
        g_previousStatusStep = GetGameStep();
    }
    switch (GetGameStep()) {
        case STATUS_STEP_DRAW:
            NextGameStep();
            DrawStatusScreen(g_statusMember);
            break;
        case STATUS_STEP_POLL:
            if (command == STATUS_COMMAND_CANCEL) {
                return g_statusFixedMember ? STATUS_COMMAND_CANCEL_FIXED_MEMBER
                                           : STATUS_COMMAND_CANCEL;
            }
            if (command != STATUS_COMMAND_NONE) {
                SetGameStep(command);
            }
            break;
        case STATUS_STEP_CLOSE:
            return STATUS_COMMAND_CANCEL_FIXED_MEMBER;
        case STATUS_STEP_SKILLS:
            ResumeStatusPage(RunSkillPage(command));
            break;
        case STATUS_STEP_STATS:
            ResumeStatusPage(RunStatPage(command));
            break;
        case STATUS_STEP_NEXT_MEMBER:
            do {
                g_statusMember++;
                if (g_statusMember >= ROSTER_SIZE) {
                    g_statusMember = 0;
                }
            } while (!GetRosterEntry(g_statusMember));
            SetGameStep(STATUS_STEP_DRAW);
            ClearStatusPicture();
            break;
        case STATUS_STEP_EXIT:
            return STATUS_STEP_EXIT;
        case STATUS_STEP_EQUIPMENT:
            ResumeStatusPage(RunEquipScreen(command));
            break;
        case STATUS_STEP_ATTACH:
            ResumeStatusPage(RunAttachScreen(command));
            break;
        case STATUS_STEP_ALIGNMENT:
            ResumeStatusPage(RunAlignmentPage(command));
            break;
        case STATUS_STEP_ITEMS:
            if (!GetStatusAnalyzeMode()) {
                command = RunItemPage(command);
            }
            ResumeStatusPage(command);
            break;
    }
    return STATUS_COMMAND_NONE;
}

RVA(0x000423e0, 0x6c)
static b16 ResumeStatusPage(i16 command) {
    u16 step;

    if (command == STATUS_COMMAND_NONE) {
        return true;
    }
    if (command == STATUS_COMMAND_CANCEL) {
        step = SetGameStep(STATUS_STEP_POLL);
        HighlightHotspot(0, s_statusCommandHotspots[step], 0);
        return true;
    }
    HighlightHotspot(0, s_statusCommandHotspots[GetGameStep()], 0);
    SetGameStep(command);
    return false;
}

static i16
DrawStatBar(i16 x, i16 y, i16 base, i16 bonus, i16 equipment, GZ_ENUM_PARAM(StatBarRows, i16) band);
static i16 DrawStatBarSegment(
    i16 first,
    i16 last,
    i16 x,
    i16 y,
    i16 offset,
    GZ_ENUM_PARAM(StatBarMark, i16) mark,
    GZ_ENUM_PARAM(StatBarRows, i16) band
);

RVA(0x00042450, 0xc3)
static i16 DrawStatList(i16 plane, Character* member) {
    i16 stat;
    i16 y;

    if (!member) {
        return -1;
    }
    for (stat = 0; stat < 10; stat++) {
        SetTextPlaneCursorLine(plane, 0, stat);
        sprintf(g_scratchBuffer, "%-6.6s %3d", g_statusStatNames[stat], GetStatTotal(member, stat));
        PrintWindowText(
            plane,
            g_scratchBuffer,
            TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
            0,
            true
        );
    }
    RepaintTextPlane(plane, -2);
    s_statPage.plane = plane;
    y = 1;
    for (stat = 0; stat < 10; stat++) {
        DrawStatBar(
            12,
            y,
            GetBaseStat(member, stat),
            GetStatBonus(member, stat),
            GetStatEquipment(member, stat),
            STAT_BAR_ROWS_BOTH
        );
        y += 2;
    }
    return plane;
}

RVA(0x00042520, 0x9e)
static i16 DrawStatBar(
    i16 x,
    i16 y,
    i16 base,
    i16 bonus,
    i16 equipment,
    GZ_ENUM_PARAM(StatBarRows, i16) band
) {
    i32 total = ClampSum100(base, bonus, equipment);
    i16 offset;

    if (base < 50) {
        offset = DrawStatBarSegment(0, base, x, y, 0, STAT_BAR_MARK_BASE, band);
    } else {
        offset = DrawStatBarSegment(0, 50, x, y, 0, STAT_BAR_MARK_BASE, band);
        offset = DrawStatBarSegment(50, base, x, y, offset, STAT_BAR_MARK_BASE_OVER_50, band);
    }
    offset = DrawStatBarSegment(0, bonus, x, y, offset, STAT_BAR_MARK_BONUS, band);
    offset = DrawStatBarSegment(0, equipment, x, y, offset, STAT_BAR_MARK_EQUIPMENT, band);
    return DrawStatBarSegment(total, 50, x, y, offset, STAT_BAR_MARK_EMPTY, band);
}

RVA(0x000425c0, 0xaa)
static i16 DrawStatBarSegment(
    i16 first,
    i16 last,
    i16 x,
    i16 y,
    i16 offset,
    GZ_ENUM_PARAM(StatBarMark, i16) mark,
    GZ_ENUM_PARAM(StatBarRows, i16) band
) {
    i16 row;
    i16 column;

    for (; first < last; first++) {
        row = offset / 50;
        column = offset % 50;
        if (row < 2) {
            if (band == STAT_BAR_ROWS_BOTH || (band == STAT_BAR_ROWS_FIRST && row == 0)
                || (band == STAT_BAR_ROWS_SECOND && row == 1)) {
                DrawStatBarMark(x + column, y, mark, s_statPage.plane);
            }
        }
        offset++;
    }
    return offset;
}

RVA(0x00042670, 0x2e)
i16 OpenStatListWindow(Character* character) {
    i16 plane;

    if (!character) {
        return -1;
    }
    plane = CreateTextPlane(17, 0);
    DrawStatList(plane, character);
    return plane;
}

RVA(0x000426a0, 0xd4)
i16 RunStatPage(i16 command) {
    if (command != STATUS_COMMAND_NONE && command != STATUS_COMMAND_CANCEL) {
        SetGameSub(MENU_STEP_CLOSE);
        s_statPage.resume = STATUS_COMMAND_CANCEL;
        if (command != STATUS_STEP_STATS) {
            s_statPage.resume = command;
        }
    }
    switch (GetGameSub()) {
        case MENU_STEP_OPEN:
            SetGameSub(MENU_STEP_RUN);
            SetStatusMenuItemFlag(STATUS_STEP_STATS, PANEL_ROW_CHECKED, true);
            s_statPage.plane = OpenStatListWindow(GetRosterCharacter(g_statusMember));
            break;
        case MENU_STEP_CLOSE:
            s_statPage.plane = CloseTextWindow(s_statPage.plane);
            SetStatusMenuItemFlag(STATUS_STEP_STATS, PANEL_ROW_CHECKED, false);
            return s_statPage.resume;
        case MENU_STEP_RUN:
            if (TakeClickUnlessCancel(command)) {
                PrevGameSub();
                s_statPage.resume = STATUS_COMMAND_CANCEL;
            }
            break;
    }
    return STATUS_COMMAND_NONE;
}

RVA(0x00042780, 0xa5)
i16 DrawStatTotals(i16 x, i16 y, Character* member, Character* compare) {
    i16 stat;
    i32 attr;

    for (stat = 0; stat < 10; stat++) {
        sprintf(g_scratchBuffer, "%-6.6s %7d", g_statusStatNames[stat], GetStatTotal(member, stat));
        if (!compare) {
            y = DrawStatusLine(
                x,
                y,
                g_scratchBuffer,
                TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
            );
        } else {
            attr =
                TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
            if (GetStatTotal(member, stat) < GetStatTotal(compare, stat)) {
                attr = TEXT_ATTR_OPAQUE
                       | TEXT_ATTR(TEXT_COLOR_GREEN, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
            } else if (GetStatTotal(member, stat) > GetStatTotal(compare, stat)) {
                attr = TEXT_ATTR_OPAQUE
                       | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
            }
            y = DrawStatusLine(x, y, g_scratchBuffer, attr);
        }
    }
    return y;
}

RVA(0x00042830, 0xf0)
void DrawStatLine(Character* member, i16 stat, i16 highlight, i16 window) {
    if (stat >= 10) {
        return;
    }
    if (highlight) {
        if (GetStatTotal(member, stat) >= 50) {
            DrawStatBar(
                12,
                stat * 2 + 1,
                GetBaseStat(member, stat),
                GetStatBonus(member, stat),
                GetStatEquipment(member, stat),
                STAT_BAR_ROWS_FIRST
            );
        }
    } else {
        DrawStatBar(
            12,
            stat * 2 + 1,
            GetBaseStat(member, stat),
            GetStatBonus(member, stat),
            GetStatEquipment(member, stat),
            STAT_BAR_ROWS_BOTH
        );
        sprintf(g_scratchBuffer, "%-6.6s %3d", g_statusStatNames[stat], GetStatTotal(member, stat));
        SetTextPlaneCursorLine(window, 0, stat);
        PrintWindowText(
            window,
            g_scratchBuffer,
            TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
            0,
            true
        );
    }
}

static i16 DrawStatusNumber(i16 x, i16 y, i16 value);

RVA(0x00042920, 0x96)
static i16 DrawBattleStatColumn(i16 x, i16 y, i16* stats, i16 icon, i16 id) {
    if (icon >= 0) {
        DrawStatusImage(x, y, s_battleStatIcons[icon]);
    }
    y += 3;
    if (stats) {
        if (id >= 32) {
            y += 2;
        } else {
            y = DrawStatusNumber(x, y, stats[0]);
        }
        y = DrawStatusNumber(x, y, stats[2]);
        y = DrawStatusNumber(x, y, stats[3]);
        y = DrawStatusNumber(x, y, stats[4]);
        y = DrawStatusNumber(x, y, stats[5]);
    }
    return x + 5;
}

RVA(0x000429c0, 0x35)
static i16 DrawStatusNumber(i16 x, i16 y, i16 value) {
    sprintf(g_scratchBuffer, "%3d", value);
    return DrawStatusLine(
        x,
        y,
        g_scratchBuffer,
        TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
    );
}

RVA(0x00042a00, 0xf3)
void DrawBattleStatsPanel(i16 x, i16 y, Character* member, i16 hideIcons) {
    i16 row;
    i16 startY = y;
    i16 icon = -hideIcons * 4;
    i16 gunX;
    char* label;

    y += 3;
    for (row = 0; (label = g_statusBattleLabels[row]) != NULL; row++) {
        if (!row && !IsHumanCharacter(member)) {
            y = DrawStatusLabel(x, y, g_emptyBattleSkillLabel);
        } else {
            y = DrawStatusLabel(x, y, label);
        }
    }
    x = DrawBattleStatColumn(
        x + 6,
        startY,
        GetBattleStatGroup(member, BATTLE_GROUP_WEAPON),
        icon,
        member->id
    );
    gunX = x;
    x = DrawBattleStatColumn(
        x,
        startY,
        GetBattleStatGroup(member, BATTLE_GROUP_GUN),
        icon + 1,
        member->id
    );
    DrawStatusNumber(gunX, startY + 13, GetCharacterEquipment(member)[EQUIP_SLOT_AMMO].quantity);
    x = DrawBattleStatColumn(
        x,
        startY,
        GetBattleStatGroup(member, BATTLE_GROUP_MAGIC),
        icon + 2,
        member->id
    );
    if (icon >= 0 && IsHumanCharacter(member)) {
        DrawBattleStatColumn(
            x,
            startY,
            GetBattleStatGroup(member, BATTLE_GROUP_DEMON_INTERACTION),
            icon + 3,
            member->id
        );
    }
}

static void DrawAlignmentMarker(i16 slot, Character* member);

RVA(0x00042b00, 0x108)
i16 RunAlignmentPage(i16 command) {
    if (command != STATUS_COMMAND_NONE && command != STATUS_COMMAND_CANCEL) {
        SetGameSub(MENU_STEP_CLOSE);
        s_alignmentPage.resume = STATUS_COMMAND_CANCEL;
        if (command != STATUS_STEP_ALIGNMENT) {
            s_alignmentPage.resume = command;
        }
    }
    switch (GetGameSub()) {
        case MENU_STEP_OPEN:
            SetGameSub(MENU_STEP_RUN);
            SetStatusMenuItemFlag(STATUS_STEP_ALIGNMENT, PANEL_ROW_CHECKED, true);
            s_alignmentPage.plane = CreateTextPlane(6, 0);
            ResetTextPlaneLineStep(s_alignmentPage.plane, 1);
            DrawAlignmentMarker(g_statusMember, GetRosterCharacter(g_statusMember));
            RepaintTextPlane(s_alignmentPage.plane, -2);
            break;
        case MENU_STEP_CLOSE:
            s_alignmentPage.plane = CloseTextWindow(s_alignmentPage.plane);
            SetStatusMenuItemFlag(STATUS_STEP_ALIGNMENT, PANEL_ROW_CHECKED, false);
            return s_alignmentPage.resume;
        case MENU_STEP_RUN:
            if (TakeClickUnlessCancel(command)) {
                PrevGameSub();
                s_alignmentPage.resume = STATUS_COMMAND_CANCEL;
            }
            break;
    }
    return STATUS_COMMAND_NONE;
}

RVA(0x00042c10, 0x88)
static void DrawAlignmentMarker(i16 slot, Character* member) {
    i16 x;
    i16 y;

    if (!member) {
        return;
    }
    x = AlignmentChartCell(member->alignmentLevelB);
    y = AlignmentChartCell(member->alignmentLevelA);
    SetTextPlaneCursorLine(s_alignmentPage.plane, x, y);
    sprintf(g_scratchBuffer, "%s", g_statusNumberLabels[slot + 1]);
    DrawPlaneText(s_alignmentPage.plane, x * 8, y * 8, g_scratchBuffer, TEXT_ATTR_NORMAL);
}

// Maps a signed alignment byte (-128..127) onto the 24-cell alignment chart,
// counting from the far end.
RVA(0x00042ca0, 0x2f)
i16 AlignmentChartCell(i16 value) {
    return 23 - (i16)(((value - -128.0) / 256.0) * 24.0);
}

// The Character `ammoCounts` entry of equipment kinds 11 through 19 (-1 for
// ammunition). character has an identical copy of its own (s_itemCountSlots).
DATA(0x000649a0)
static const i16 s_equipCountSlots[9] = {0, 1, -1, 4, 3, 4, 5, 6, 7};

// The equipment page's state: the menu of bag items to equip, the equipment
// panel, the picked item's name and description, the picked bag entry or
// equipment part (-1 none, -2 cancelled), and whether the equipment changed,
// so the status screen is redrawn on leaving.
typedef struct EquipPage {
    MenuBox* menu;
    i16 panelPlane;
    i16 infoPlane;
    i16 pick;
    b16 changed;
} EquipPage;

// One object: the fields sit two bytes apart, where separate variables take
// four-byte slots.
DATA(0x0006a208)
static EquipPage s_equipPage = {NULL, -1, -1, -1, false};

// One object: retail reads `itemBase` and `item` with dword moves that run
// into the next field.
DATA(0x0006a218)
static AttachPage s_attach = {NULL, NULL, NULL, 0, -1, -1, 0, 0, 0, 0};

// The item page: its menu, its info window, the sub-state to resume (and then
// the item picked), and the copy of the bag it lists.
DATA(0x0006a238)
static EquipItemPage s_itemPage = {NULL, -1, -1, NULL};

// The skill page: its menu, its description window, and the sub-state to
// resume (and then the skill picked).
DATA(0x0006a248)
static EquipSkillPage s_skillPage = {NULL, -1, -1};

// The bag entries the equipment menu lists and the attach page lists.
DATA(0x00083b50)
static i16 s_equipEntries[48] = {0};

DATA(0x00083bb0)
static AttachEntry s_attachEntries[48] = {0};

DATA(0x00083c70)
i16 g_previousStatusStep = 0;

DATA(0x00083c74)
char g_emptyBattleSkillLabel[4] = {0};

// The equipment menu's two header lines.
DATA(0x00083c78)
static char s_equipHeaderA[4] = {0};

DATA(0x00083c7c)
static char s_equipHeaderB[4] = {0};

// The attach page's two header lines.
DATA(0x00083c80)
static char s_attachHeaderA[4] = {0};

DATA(0x00083c84)
static char s_attachHeaderB[4] = {0};

// The label of an empty equipment part.
DATA(0x00083c88)
static char s_emptyPartLabel[4] = {0};

// The skill page's second header line and the label of an empty skill.
DATA(0x00083c8c)
static char s_skillHeaderLine[4] = {0};

DATA(0x00083c90)
static char s_emptySkillLabel[4] = {0};

DATA(0x00083c94)
char g_emptyEquipPickLabel[4] = {0};

DATA(0x000649b8)
static const i16 s_equipPickCategories[8] = {
    EQUIP_PART_WEAPON,
    EQUIP_PART_GUN,
    EQUIP_PART_AMMO,
    EQUIP_PART_HEAD,
    EQUIP_PART_BODY,
    EQUIP_PART_ARMS,
    EQUIP_PART_LEGS,
    EQUIP_PART_ACCESSORY
};

DATA(0x0006a250)
static i16 s_equipPickPart = -1;

RVA(0x00042cd0, 0x182)
i16 ListEquipCandidates(i16 member, i16 anyEquipped) {
    Character* character = GetRosterCharacter(member);
    i16 count;
    i16 item;
    i16 kind;
    i16 i;

    if (character == NULL) {
        return 0;
    }
    CompactBag();
    count = 0;
    for (i = 0; i < BAG_ORDINARY_ENTRY_COUNT; i++) {
        item = GetBagItem(i);
        if (CanEquipItem(character, item) < 0) {
            continue;
        }
        if (GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item >= 1
            && GetItemKind(GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item)
                   == ITEM_KIND_FULL_BODY_ARMOR) {
            kind = GetItemKind(item);
            if (kind == ITEM_KIND_HEAD_ARMOR || kind == ITEM_KIND_ARM_ARMOR
                || kind == ITEM_KIND_LEG_ARMOR) {
                continue;
            }
        }
        s_equipEntries[count++] = i;
    }
    if (count == 0 && anyEquipped) {
        if (GetCharacterEquipment(character)[EQUIP_SLOT_HEAD].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[EQUIP_SLOT_ARMS].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[EQUIP_SLOT_LEGS].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[EQUIP_SLOT_ACCESSORY].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[EQUIP_SLOT_WEAPON].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[EQUIP_SLOT_GUN].item >= 1) {
            return 1;
        }
        return GetCharacterEquipment(character)[EQUIP_SLOT_AMMO].item >= 1;
    }
    return count;
}

static __inline void ClearEquipPreview(void) {
    s_equipPage.infoPlane = CloseTextWindow(s_equipPage.infoPlane);
    DrawEquipPanel(GetRosterCharacter(g_statusMember), NULL);
    DrawStatTotals(3, 0x19, GetRosterCharacter(g_statusMember), NULL);
}

static __inline i16 FinishEquipChange(void) {
    RecalcCharacterStats(GetRosterCharacter(g_statusMember));
    s_equipPage.changed = true;
    SetGameSub(1);
    s_equipPage.pick = -1;
    return -1;
}

// Runs the equipment page one step for input `key` (-2 cancels): step 0
// opens it, 1 closes it, 2 waits for a bag item (menu) or an equipped part
// (panel), 3/4 preview and equip a bag item, 5/6 preview and remove an
// equipped one. A key other than -1/-2 restarts it at step 1 with that pick.
// @early-stop register allocation: from the second magazine clamp on, cl
// rotates the scratch registers one place against retail (ax/cx for dx/ax),
// and the equip path joins the removal tail one instruction early; the
// permuter found one compiler island.
RVA(0x00042e60, 0x6d0)
i16 RunEquipScreen(i16 key) {
    ItemSlot slot;
    i16 count;
    i16 part;
    ItemSlot loaded;

    if (key != STATUS_COMMAND_NONE && key != STATUS_COMMAND_CANCEL) {
        SetGameSub(MENU_STEP_CLOSE);
        s_equipPage.pick = STATUS_COMMAND_CANCEL;
        if (key != STATUS_STEP_EQUIPMENT) {
            s_equipPage.pick = key;
        }
    }
    switch (GetGameSub()) {
        case MENU_STEP_OPEN:
            SetGameSub(MENU_STEP_RUN);
            SetStatusMenuItemFlag(STATUS_STEP_EQUIPMENT, PANEL_ROW_CHECKED, true);
            s_equipPage.menu = OpenEquipMenu(g_statusMember, s_equipPage.menu);
            s_equipPage.panelPlane = CreateTextPlane(0x12, 0);
            ResetTextPlaneLineStep(s_equipPage.panelPlane, 3);
            DrawEquipPanel(GetRosterCharacter(g_statusMember), NULL);
            PollEquipPart(g_statusMember, EQUIP_PICK_RESET);
            return STATUS_COMMAND_NONE;

        case MENU_STEP_CLOSE:
            s_equipPage.infoPlane = CloseTextWindow(s_equipPage.infoPlane);
            s_equipPage.panelPlane = CloseTextWindow(s_equipPage.panelPlane);
            s_equipPage.menu = DestroyMenuBox(s_equipPage.menu);
            if (s_equipPage.changed) {
                DrawStatusScreen(g_statusMember);
                s_equipPage.changed = false;
            }
            SetStatusMenuItemFlag(STATUS_STEP_EQUIPMENT, PANEL_ROW_CHECKED, false);
            PollEquipPart(g_statusMember, EQUIP_PICK_CLEAR);
            if (s_equipPage.pick != STATUS_COMMAND_NONE) {
                return s_equipPage.pick;
            }
            PrevGameSub();
            return STATUS_COMMAND_NONE;

        case MENU_STEP_RUN:
            if (key == STATUS_COMMAND_CANCEL) {
                PrevGameSub();
                s_equipPage.pick = key;
                return STATUS_COMMAND_NONE;
            }
            if (RunListMenu(s_equipPage.menu) == LIST_MENU_OPEN) {
                part = PollEquipPart(g_statusMember, EQUIP_PICK_PART);
                if (part == -2) {
                    PrevGameSub();
                    s_equipPage.pick = STATUS_COMMAND_CANCEL;
                    return STATUS_COMMAND_NONE;
                }
                if (part == -1) {
                    return STATUS_COMMAND_NONE;
                }
                s_equipPage.pick = part;
                SetGameSub(EQUIP_STEP_PREVIEW_REMOVE);
                return STATUS_COMMAND_NONE;
            }
            NextGameSub();
            s_equipPage.pick = g_selectedObjectId;
            return STATUS_COMMAND_NONE;

        case EQUIP_STEP_PREVIEW_EQUIP:
            NextGameSub();
            PreviewEquipChange(s_equipPage.pick, 0);
            s_equipPage.infoPlane = OpenItemInfoPlane(GetBagItem(s_equipPage.pick));
            return STATUS_COMMAND_NONE;

        case EQUIP_STEP_EQUIP:
            if (key == STATUS_COMMAND_CANCEL) {
                SetGameSub(MENU_STEP_RUN);
                ClearEquipPreview();
                return STATUS_COMMAND_NONE;
            }
            if (TakeClickUnlessCancel(key) <= 0) {
                return STATUS_COMMAND_NONE;
            }
            ReadBagEntry(s_equipPage.pick, &slot, &count);
            if (GetItemKind(slot.item) == ITEM_KIND_AMMO) {
                slot.quantity = GetGunMagazineSize(
                    GetLoadedRecord(GetRosterEquipSlot(g_statusMember, EQUIP_PART_GUN).item)
                );
                if (GetRosterEquipSlot(g_statusMember, EQUIP_PART_AMMO).item == slot.item) {
                    loaded = GetRosterEquipSlot(g_statusMember, EQUIP_PART_AMMO);
                    slot.quantity -= loaded.quantity;
                    LimitItemSlotToBag(&slot);
                    if (slot.quantity < 0) {
                        slot.quantity = 0;
                    }
                    GetCharacterEquipment(GetRosterCharacter(g_statusMember))[EQUIP_SLOT_AMMO]
                        .quantity += slot.quantity;
                    TakeBagItems(slot.item, slot.quantity);
                    return FinishEquipChange();
                }
                LimitItemSlotToBag(&slot);
            } else {
                slot.quantity = 1;
            }
            EquipItem(g_statusMember, slot, count, s_equipPage.pick);
            return FinishEquipChange();

        case EQUIP_STEP_PREVIEW_REMOVE:
            NextGameSub();
            PreviewEquipChange(s_equipPage.pick, 1);
            slot = GetRosterEquipSlot(g_statusMember, s_equipPage.pick);
            s_equipPage.infoPlane = OpenItemInfoPlane(slot.item);
            return STATUS_COMMAND_NONE;

        case EQUIP_STEP_REMOVE:
            if (key == STATUS_COMMAND_CANCEL) {
                SetGameSub(MENU_STEP_RUN);
                ClearEquipPreview();
                PollEquipPart(g_statusMember, EQUIP_PICK_CLEAR);
                return STATUS_COMMAND_NONE;
            }
            if (TakeClickUnlessCancel(key) <= 0) {
                return STATUS_COMMAND_NONE;
            }
            if (s_equipPage.pick != EQUIP_PART_AMMO) {
                slot = GetRosterEquipSlot(g_statusMember, s_equipPage.pick);
                if (slot.quantity < 1) {
                    slot.quantity = 1;
                }
                StoreBagItem(slot.item, slot.quantity, slot.attachment);
                ClearItemSlot(&slot);
                SetEquipSlot(g_statusMember, s_equipPage.pick, slot, 0);
                if (s_equipPage.pick == EQUIP_PART_GUN) {
                    s_equipPage.pick = 2;
                }
            }
            if (s_equipPage.pick == EQUIP_PART_AMMO) {
                slot = GetRosterEquipSlot(g_statusMember, EQUIP_PART_AMMO);
                if (slot.item >= 1) {
                    StoreBagItem(slot.item, slot.quantity, slot.attachment);
                    ClearItemSlot(&slot);
                    SetEquipSlot(g_statusMember, s_equipPage.pick, slot, 0);
                }
            }
            return FinishEquipChange();
    }
    return STATUS_COMMAND_NONE;
}

static void EquipMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

RVA(0x00043530, 0x6a)
MenuBox* OpenEquipMenu(i16 member, MenuBox* old) {
    i16 count = ListEquipCandidates(member, 0);
    MenuBox* menu = CreateMenuBox(old, 0x13, 2);

    SetMenuItems(menu, 10, s_equipEntries, count, EquipMenuHandler);
    MoveMenuBox(menu, 6, 0x18);
    ResetTextPlaneLineStep(menu->plane, 3);
    SetTextPlaneFlag8(menu->plane, 1);
    return menu;
}

RVA(0x000435a0, 0x278)
static void EquipMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    Character* member;
    ItemRecord* record;
    i16 item;
    i16 cursed;

    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            AddMenuLine(menu->plane, s_equipHeaderA, TEXT_ATTR_DEFAULT, 0, MENU_LINE_DISABLED);
            AddMenuLine(menu->plane, s_equipHeaderB, TEXT_ATTR_DEFAULT, 0, MENU_LINE_DISABLED);
            break;
        case MENU_EVENT_ADD_ROW:
            item = GetBagItem(s_equipEntries[index]);
            sprintf(
                g_scratchBuffer,
                "%c %-20.20s %2d",
                GetBagEntryAttachment(s_equipEntries[index]) != -1 ? '*' : ' ',
                GetLoadedRecordName(item),
                GetBagEntryCount(s_equipEntries[index])
            );
            member = GetRosterCharacter(g_statusMember);
            if (member != NULL) {
                GZ_ENUM_LOCAL(EquipPart, i16) category = GetItemCategory(item);
                if (IsEquipCurseActive(member, category)) {
                    AddMenuLine(
                        menu->plane,
                        g_scratchBuffer,
                        TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                        s_equipEntries[index],
                        MENU_LINE_DISABLED
                    );
                    return;
                }
                record = GetLoadedRecord(item);
                if (record->kind == ITEM_KIND_FULL_BODY_ARMOR) {
                    cursed = IsEquipCurseActive(member, EQUIP_PART_HEAD);
                    cursed |= IsEquipCurseActive(member, EQUIP_PART_BODY);
                    cursed |= IsEquipCurseActive(member, EQUIP_PART_ARMS);
                    cursed |= IsEquipCurseActive(member, EQUIP_PART_LEGS);
                    if (cursed) {
                        AddMenuLine(
                            menu->plane,
                            g_scratchBuffer,
                            TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                            s_equipEntries[index],
                            MENU_LINE_DISABLED
                        );
                        return;
                    }
                    record = GetLoadedRecord(item);
                }
                if (record->kind == ITEM_KIND_GUN && GetBattleStatShown(member, 6) > 0) {
                    if (LacksItemRequiredStats(member, record, GetBattleStatShown(member, 6))) {
                        AddMenuLine(
                            menu->plane,
                            g_scratchBuffer,
                            TEXT_ATTR(TEXT_COLOR_YELLOW, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                            s_equipEntries[index],
                            MENU_LINE_DISABLED
                        );
                        return;
                    }
                    AddMenuLine(
                        menu->plane,
                        g_scratchBuffer,
                        TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                        s_equipEntries[index],
                        MENU_LINE_NORMAL
                    );
                    return;
                }
                if (LacksItemRequiredStats(member, record, 0)) {
                    AddMenuLine(
                        menu->plane,
                        g_scratchBuffer,
                        TEXT_ATTR(TEXT_COLOR_YELLOW, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                        s_equipEntries[index],
                        MENU_LINE_DISABLED
                    );
                    return;
                }
            }
            AddMenuLine(
                menu->plane,
                g_scratchBuffer,
                TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                s_equipEntries[index],
                MENU_LINE_NORMAL
            );
            return;
    }
}

static i16 DrawStatColumn(i16 x, i16 y, i16* stats, i16* preview);

RVA(0x00043820, 0x10a)
void DrawEquipPanel(Character* member, Character* preview) {
    i16 x;
    i16 y;
    i16 i;

    y = 0x28;
    for (i = 0; i < 4; i++) {
        DrawPlaneText(s_equipPage.panelPlane, 8, y, g_statusBattleLabels[i + 1], TEXT_ATTR_DEFAULT);
        y += 0x18;
    }
    if (preview == NULL) {
        x = DrawStatColumn(5, 5, GetBattleStatGroup(member, BATTLE_GROUP_WEAPON), NULL);
        x = DrawStatColumn(x, 5, GetBattleStatGroup(member, BATTLE_GROUP_GUN), NULL);
        DrawStatColumn(x, 5, GetBattleStatGroup(member, BATTLE_GROUP_MAGIC), NULL);
    } else {
        x = DrawStatColumn(
            5,
            5,
            GetBattleStatGroup(member, BATTLE_GROUP_WEAPON),
            GetBattleStatGroup(preview, BATTLE_GROUP_WEAPON)
        );
        x = DrawStatColumn(
            x,
            5,
            GetBattleStatGroup(member, BATTLE_GROUP_GUN),
            GetBattleStatGroup(preview, BATTLE_GROUP_GUN)
        );
        DrawStatColumn(
            x,
            5,
            GetBattleStatGroup(member, BATTLE_GROUP_MAGIC),
            GetBattleStatGroup(preview, BATTLE_GROUP_MAGIC)
        );
    }
    DrawPlaneImage(s_equipPage.panelPlane, 7, 1, 0);
    DrawPlaneImage(s_equipPage.panelPlane, 0x10, 1, 1);
    DrawPlaneImage(s_equipPage.panelPlane, 0x19, 1, 8);
}

static u16 DrawStatCompare(i16 x, i16 y, i16 value, i16 newValue);

RVA(0x00043930, 0xe4)
static i16 DrawStatColumn(i16 x, i16 y, i16* stats, i16* preview) {
    if (preview == NULL) {
        DrawStatCompare(x, y, stats[2], -1);
        DrawStatCompare(x, y + 3, stats[3], -1);
        DrawStatCompare(x, y + 6, stats[4], -1);
        DrawStatCompare(x, y + 9, stats[5], -1);
    } else {
        DrawStatCompare(x, y, stats[2], preview[2]);
        DrawStatCompare(x, y + 3, stats[3], preview[3]);
        DrawStatCompare(x, y + 6, stats[4], preview[4]);
        DrawStatCompare(x, y + 9, stats[5], preview[5]);
    }
    return x + strlen(g_scratchBuffer);
}

RVA(0x00043a20, 0x93)
static u16 DrawStatCompare(i16 x, i16 y, i16 value, i16 newValue) {
    i32 attr = TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);

    if (newValue < 0) {
        sprintf(g_scratchBuffer, "  %3d    ", value);
    } else {
        sprintf(g_scratchBuffer, "  %3d>%3d", value, newValue);
        if (value < newValue) {
            attr = TEXT_ATTR(TEXT_COLOR_GREEN, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
        } else if (value > newValue) {
            attr = TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
        }
    }
    DrawPlaneText(s_equipPage.panelPlane, x * 8, y * 8, g_scratchBuffer, attr);
    return attr;
}

static MenuBox* CreateAttachItemMenu(MenuBox* old);
static MenuBox* CreateAttachEntryMenu(MenuBox* old);
static void AttachTextHook(i16 plane, i16 event, i16 value);

RVA(0x00043ac0, 0x24f)
void PreviewEquipChange(i16 index, i16 fromEquipped) {
    Character* member = GetRosterCharacter(g_statusMember);
    Character* saved;
    ItemSlot slot;
    i16 count;
    GZ_ENUM_STORAGE(EquipPart, i16) result;
    i16 kind;
    i16 gun;

    if (!member) {
        return;
    }
    saved = GetCharacter(14);
    memcpy(saved, member, offsetof(Character, alignmentA));
    if (!fromEquipped) {
        ReadBagEntry(index, &slot, &count);
        kind = GetItemKind(slot.item);
        if (kind == ITEM_KIND_AMMO) {
            slot.quantity = GetGunMagazineSize(
                GetLoadedRecord(GetCharacterEquipment(member)[EQUIP_SLOT_GUN].item)
            );
            LimitItemSlotToBag(&slot);
        } else if (kind == ITEM_KIND_FULL_BODY_ARMOR) {
            EmptyItemSlot(&GetCharacterEquipment(member)[EQUIP_SLOT_HEAD]);
            EmptyItemSlot(&GetCharacterEquipment(member)[EQUIP_SLOT_ARMS]);
            EmptyItemSlot(&GetCharacterEquipment(member)[EQUIP_SLOT_LEGS]);
        } else {
            // Kind 19 selects index 7, overwriting returnPosition.area in
            // the saved preview copy; retain this original store.
            saved->ammoCounts[s_equipCountSlots[kind - ITEM_KIND_WEAPON]] = count;
            slot.quantity = 1;
        }
        if (kind == ITEM_KIND_GUN) {
            gun = GetCharacterEquipment(member)[EQUIP_SLOT_GUN].item;
            GetCharacterEquipment(member)[EQUIP_SLOT_GUN].item = slot.item;
            if (CanEquipItem(member, GetCharacterEquipment(member)[EQUIP_SLOT_AMMO].item) < 1) {
                EmptyItemSlot(&GetCharacterEquipment(member)[EQUIP_SLOT_AMMO]);
            }
            GetCharacterEquipment(member)[EQUIP_SLOT_GUN].item = gun;
        }
        SwapEquipSlot(g_statusMember, slot, &result);
    } else {
        ClearItemSlot(&slot);
        SetEquipSlot(g_statusMember, index, slot, 0);
        if (index == EQUIP_PART_GUN) {
            SetEquipSlot(g_statusMember, EQUIP_PART_AMMO, slot, 0);
        }
    }
    RecalcCharacterStats(member);
    DrawEquipPanel(saved, member);
    DrawStatTotals(3, 0x19, saved, member);
    memcpy(member, saved, offsetof(Character, alignmentA));
    InitWordList(GetCharacterSkills(saved), 0);
}

RVA(0x00043d10, 0x5e0)
i16 RunAttachScreen(i16 sub) {
    if (sub != STATUS_COMMAND_NONE && sub != STATUS_COMMAND_CANCEL) {
        SetGameSub(MENU_STEP_CLOSE);
        s_attach.resume = STATUS_COMMAND_CANCEL;
        if (sub != STATUS_STEP_ATTACH) {
            s_attach.resume = sub;
        }
    }
    switch (GetGameSub()) {
        case MENU_STEP_OPEN:
            SetGameSub(MENU_STEP_RUN);
            s_attach.itemBase = GetGemItemBase();
            SetStatusMenuItemFlag(STATUS_STEP_ATTACH, PANEL_ROW_CHECKED, true);
            s_attach.itemMenu = CreateAttachItemMenu(s_attach.itemMenu);
            s_attach.plane = CreateTextPlane(0x12, 0);
            ResetTextPlaneLineStep(s_attach.plane, 3);
            s_attach.prevHook = SetTextPlaneHook(AttachTextHook);
            return STATUS_COMMAND_NONE;
        case MENU_STEP_CLOSE:
            SetTextPlaneHook(s_attach.prevHook);
            s_attach.prevHook = NULL;
            s_attach.plane = CloseTextWindow(s_attach.plane);
            s_attach.entryMenu = DestroyMenuBox(s_attach.entryMenu);
            s_attach.itemMenu = DestroyMenuBox(s_attach.itemMenu);
            PollEquipPart(g_statusMember, EQUIP_PICK_CLEAR);
            if (s_attach.redraw) {
                DrawStatusScreen(g_statusMember);
                s_attach.redraw = false;
            }
            SetStatusMenuItemFlag(STATUS_STEP_ATTACH, PANEL_ROW_CHECKED, false);
            if (s_attach.resume == STATUS_COMMAND_NONE) {
                PrevGameSub();
                return STATUS_COMMAND_NONE;
            }
            return s_attach.resume;
        case MENU_STEP_RUN:
            if (sub == STATUS_COMMAND_CANCEL) {
                PrevGameSub();
                s_attach.resume = STATUS_COMMAND_CANCEL;
                return STATUS_COMMAND_NONE;
            }
            if (RunListMenu(s_attach.itemMenu) == LIST_MENU_OPEN || g_selectedObjectId < 0) {
                break;
            }
            NextGameSub();
            SetTextPlaneHook(s_attach.prevHook);
            s_attach.prevHook = NULL;
            s_attach.plane = CloseTextWindow(s_attach.plane);
            s_attach.itemMenu = DestroyMenuBox(s_attach.itemMenu);
            s_attach.item = g_selectedObjectId;
            return STATUS_COMMAND_NONE;
        case ATTACH_STEP_OPEN_TARGET_LIST:
            NextGameSub();
            s_attach.entryMenu = CreateAttachEntryMenu(s_attach.entryMenu);
            PollEquipPart(g_statusMember, EQUIP_PICK_RESET);
            return STATUS_COMMAND_NONE;
        case ATTACH_STEP_PICK_TARGET:
            if (sub == STATUS_COMMAND_CANCEL) {
                SetGameSub(MENU_STEP_CLOSE);
                s_attach.resume = STATUS_COMMAND_CANCEL;
                return STATUS_COMMAND_NONE;
            }
            if (RunListMenu(s_attach.entryMenu) == LIST_MENU_OPEN) {
                sub = PollEquipPart(g_statusMember, EQUIP_PICK_ATTACH_TARGET);
                if (sub == STATUS_COMMAND_CANCEL) {
                    SetGameSub(MENU_STEP_CLOSE);
                    s_attach.resume = STATUS_COMMAND_CANCEL;
                    return STATUS_COMMAND_NONE;
                }
                if (sub == STATUS_COMMAND_NONE) {
                    break;
                }
                s_attach.target = sub;
                s_attach.entryMenu = DestroyMenuBox(s_attach.entryMenu);
                SetGameSub(ATTACH_STEP_APPLY_EQUIPPED_ITEM);
                return STATUS_COMMAND_NONE;
            }
            NextGameSub();
            s_attach.target = g_selectedObjectId;
            s_attach.entryMenu = DestroyMenuBox(s_attach.entryMenu);
            return STATUS_COMMAND_NONE;
        case ATTACH_STEP_APPLY_BAG_ITEM:
            NextGameSub();
            s_attach.plane = CreateTextPlane(0x12, 0);
            sprintf(g_scratchBuffer, "%s", GetLoadedRecordName(s_attach.item));
            PrintWindowText(s_attach.plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            TakeBagItems(s_attach.item, 1);
            s_attach.item = AttachBagEntryItem(s_attach.target, s_attach.item);
            if (s_attach.item < 0) {
                // "をはめ込んだ" (fitted in)
                sprintf(g_scratchBuffer, "\202\360\202\315\202\337\215\236\202\361\202\276");
            } else {
                StoreBagItem(s_attach.item, 1, -1);
                // "と%sを付け替えた" (swapped for %s)
                sprintf(
                    g_scratchBuffer,
                    "\202\306%s\202\360\225\164\202\257\221\326\202\246\202\275",
                    GetLoadedRecordName(s_attach.item)
                );
            }
            PrintWindowText(s_attach.plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            RepaintTextPlane(s_attach.plane, -2);
            PushWaitState(WAIT_INPUT, WAIT_ON_ANY_INPUT, 0xffff, 0);
            return STATUS_COMMAND_NONE;
        case ATTACH_STEP_FINISH_BAG_ITEM:
            SetGameSub(MENU_STEP_CLOSE);
            s_attach.plane = CloseTextWindow(s_attach.plane);
            s_attach.resume = STATUS_COMMAND_NONE;
            return STATUS_COMMAND_NONE;
        case ATTACH_STEP_APPLY_EQUIPPED_ITEM:
            NextGameSub();
            s_attach.plane = CreateTextPlane(0x12, 0);
            sprintf(g_scratchBuffer, "%s", GetLoadedRecordName(s_attach.item));
            PrintWindowText(s_attach.plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            TakeBagItems(s_attach.item, 1);
            s_attach.item =
                AttachEquipItem(g_statusMember, s_attach.target, s_attach.item - s_attach.itemBase);
            if (s_attach.item < 0) {
                // "をはめ込んだ" (fitted in)
                sprintf(g_scratchBuffer, "\202\360\202\315\202\337\215\236\202\361\202\276");
            } else {
                s_attach.item += s_attach.itemBase;
                StoreBagItem(s_attach.item, 1, -1);
                // "と%sを付け替えた" (swapped for %s)
                sprintf(
                    g_scratchBuffer,
                    "\202\306%s\202\360\225\164\202\257\221\326\202\246\202\275",
                    GetLoadedRecordName(s_attach.item)
                );
            }
            PrintWindowText(s_attach.plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, true);
            RepaintTextPlane(s_attach.plane, -2);
            PushWaitState(WAIT_INPUT, WAIT_ON_ANY_INPUT, 0xffff, 0);
            return STATUS_COMMAND_NONE;
        case ATTACH_STEP_FINISH_EQUIPPED_ITEM:
            SetGameSub(MENU_STEP_CLOSE);
            s_attach.plane = CloseTextWindow(s_attach.plane);
            RecalcCharacterStats(GetRosterCharacter(g_statusMember));
            s_attach.redraw = true;
            s_attach.resume = STATUS_COMMAND_NONE;
            return STATUS_COMMAND_NONE;
    }
    return STATUS_COMMAND_NONE;
}

static void AttachItemMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

RVA(0x000442f0, 0x49)
static MenuBox* CreateAttachItemMenu(MenuBox* old) {
    MenuBox* menu = CreateMenuBox(old, 0x15, 2);

    SetMenuItems(menu, 16, NULL, 16, AttachItemMenuHandler);
    MoveMenuBox(menu, 0x2a, 0x50);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, false);
    return menu;
}

RVA(0x00044340, 0x86)
static void AttachItemMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    i16 count;

    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_ADD_ROW:
            count = CountGemItemsAt(index);
            sprintf(
                g_scratchBuffer,
                "%-14.14s %2d",
                GetLoadedRecordName(index + s_attach.itemBase),
                count
            );
            if (count == 0) {
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                    index + s_attach.itemBase,
                    MENU_LINE_UNCHOOSABLE
                );
            } else {
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                    index + s_attach.itemBase,
                    MENU_LINE_NORMAL
                );
            }
            break;
    }
}

static i16 ListAttachEntries(void);
static void AttachEntryMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

RVA(0x000443d0, 0x69)
static MenuBox* CreateAttachEntryMenu(MenuBox* old) {
    MenuBox* menu;

    s_attach.entryCount = ListAttachEntries();
    menu = CreateMenuBox(old, 0x13, 2);
    SetMenuItems(menu, 10, s_attachEntries, s_attach.entryCount, AttachEntryMenuHandler);
    MoveMenuBox(menu, 6, 0x18);
    ResetTextPlaneLineStep(menu->plane, 3);
    SetTextPlaneFlag8(menu->plane, 1);
    return menu;
}

RVA(0x00044440, 0x66)
static i16 ListAttachEntries(void) {
    i16 count;
    i16 item;
    i16 i;

    CompactBag();
    count = 0;
    for (i = 0; i < BAG_ORDINARY_ENTRY_COUNT; i++) {
        item = GetBagItem(i);
        if (item >= 0 && GetItemStackLimit(item) == 1 && GetItemKind(item) != ITEM_KIND_GUN) {
            s_attachEntries[count].entry = i;
            s_attachEntries[count].count = GetBagEntryCount(i);
            count++;
        }
    }
    return count;
}

RVA(0x000444b0, 0xda)
static void AttachEntryMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            AddMenuLine(menu->plane, s_attachHeaderA, TEXT_ATTR_DEFAULT, 0, MENU_LINE_DISABLED);
            AddMenuLine(menu->plane, s_attachHeaderB, TEXT_ATTR_DEFAULT, 0, MENU_LINE_DISABLED);
            break;
        case MENU_EVENT_ADD_ROW:
            sprintf(
                g_scratchBuffer,
                "%c %-20.20s %2d",
                GetBagEntryAttachment(s_attachEntries[index].entry) != -1 ? '*' : ' ',
                GetLoadedRecordName(GetBagItem(s_attachEntries[index].entry)),
                s_attachEntries[index].count
            );
            AddMenuLine(
                menu->plane,
                g_scratchBuffer,
                TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                s_attachEntries[index].entry,
                MENU_LINE_NORMAL
            );
            break;
    }
}

RVA(0x00044590, 0xb8)
static void AttachTextHook(i16 plane, i16 event, i16 value) {
    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    switch (event) {
        case TEXT_EVENT_CANCEL:
        case TEXT_EVENT_CHOOSE:
        case TEXT_EVENT_CHOOSE_RIGHT:
            return;
        case TEXT_EVENT_UNHIGHLIGHT:
            ClearTextPlane(s_attach.plane);
            break;
        case TEXT_EVENT_HIGHLIGHT:
            strcpy(g_scratchBuffer, GetItemDescription(value + s_attach.itemBase));
            PrintWindowText(s_attach.plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, 0, false);
            break;
    }
    RepaintTextPlane(s_attach.plane, -2);
}

static void ItemListHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

RVA(0x00044650, 0x1e0)
i16 RunItemPage(i16 sub) {
    if (sub != STATUS_COMMAND_NONE && sub != STATUS_COMMAND_CANCEL) {
        SetGameSub(MENU_STEP_CLOSE);
        s_itemPage.pick = STATUS_COMMAND_CANCEL;
        if (sub != STATUS_STEP_ITEMS) {
            s_itemPage.pick = sub;
        }
    }
    switch (GetGameSub()) {
        case MENU_STEP_OPEN:
            SetGameSub(MENU_STEP_RUN);
            CompactBag();
            SetStatusMenuItemFlag(STATUS_STEP_ITEMS, PANEL_ROW_CHECKED, true);
            s_itemPage.menu = CreateMenuBox(s_itemPage.menu, 0x19, 2);
            MoveMenuBox(s_itemPage.menu, -8, -0x16);
            s_itemPage.list = CopyBagEntries(0, BAG_ENTRY_COUNT, NULL);
            SetMenuItems(
                s_itemPage.menu,
                8,
                s_itemPage.list,
                GetItemListCount(s_itemPage.list),
                ItemListHandler
            );
            SetTextPlaneFirstSelectableRow(s_itemPage.menu->plane, 1, true);
            return STATUS_COMMAND_NONE;
        case MENU_STEP_CLOSE:
            s_itemPage.plane = CloseTextWindow(s_itemPage.plane);
            s_itemPage.menu = CloseListMenu(s_itemPage.menu);
            SetStatusMenuItemFlag(STATUS_STEP_ITEMS, PANEL_ROW_CHECKED, false);
            return s_itemPage.pick;
        case MENU_STEP_RUN:
            if (sub == STATUS_COMMAND_CANCEL) {
                PrevGameSub();
                s_itemPage.pick = sub;
                return STATUS_COMMAND_NONE;
            }
            if (RunListMenu(s_itemPage.menu) == LIST_MENU_OPEN) {
                break;
            }
            NextGameSub();
            s_itemPage.pick = g_selectedObjectId;
            return STATUS_COMMAND_NONE;
        case ITEM_PAGE_SHOW_DESCRIPTION:
            if (sub == STATUS_COMMAND_CANCEL) {
                PrevGameSub();
                return STATUS_COMMAND_NONE;
            }
            NextGameSub();
            s_itemPage.plane = OpenItemInfoPlane(s_itemPage.pick);
            return STATUS_COMMAND_NONE;
        case ITEM_PAGE_WAIT_DESCRIPTION:
            if (sub != STATUS_COMMAND_CANCEL && !TakeMouseLeftClick()) {
                break;
            }
            s_itemPage.plane = CloseTextWindow(s_itemPage.plane);
            SetGameSub(MENU_STEP_RUN);
            break;
    }
    return STATUS_COMMAND_NONE;
}

RVA(0x00044830, 0x109)
static void ItemListHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    i16 item;

    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->items.table = NULL;
            menu->itemCount = 0;
            s_itemPage.list = FreeBlock(s_itemPage.list);
            break;
        case MENU_EVENT_BEGIN_PAGE:
            // "所持アイテム %1d/8" (items held, page %d of 8)
            sprintf(
                g_scratchBuffer,
                "\217\212\216\235\203\101\203\103\203\145\203\200 %1d/8",
                menu->cursor / 8 + 1
            );
            AddMenuLine(menu->plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, -1, MENU_LINE_DISABLED);
            break;
        case MENU_EVENT_ADD_ROW:
            item = GetItemStackItem(GetItemListEntry(s_itemPage.list, index));
            sprintf(
                g_scratchBuffer,
                "%c %-30.30s%2d",
                HasItemStackAttachment(GetItemListEntry(s_itemPage.list, index)) ? '*' : ' ',
                GetLoadedRecordName(item),
                GetItemStackCount(GetItemListEntry(s_itemPage.list, index))
            );
            AddMenuLine(
                menu->plane,
                g_scratchBuffer,
                TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                item,
                MENU_LINE_NORMAL
            );
            break;
    }
}

static void
DrawEquipLine(i16 part, i16 item, i16 attach, i16 x, i16 y, Character* character, i16 gunItem);

RVA(0x00044940, 0x18a)
void DrawEquipLines(Character* character, i16 x, i16 y) {
    DrawEquipLine(
        0,
        GetCharacterEquipment(character)[EQUIP_SLOT_WEAPON].item,
        GetCharacterEquipment(character)[EQUIP_SLOT_WEAPON].attachment,
        x,
        y,
        character,
        0
    );
    DrawEquipLine(
        1,
        GetCharacterEquipment(character)[EQUIP_SLOT_GUN].item,
        GetCharacterEquipment(character)[EQUIP_SLOT_GUN].attachment,
        x,
        y + 4,
        character,
        0
    );
    DrawEquipLine(
        2,
        GetCharacterEquipment(character)[EQUIP_SLOT_AMMO].item,
        GetCharacterEquipment(character)[EQUIP_SLOT_AMMO].attachment,
        x,
        y + 8,
        character,
        0
    );
    DrawEquipLine(
        3,
        GetCharacterEquipment(character)[EQUIP_SLOT_HEAD].item,
        GetCharacterEquipment(character)[EQUIP_SLOT_HEAD].attachment,
        x,
        y + 12,
        character,
        GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item
    );
    DrawEquipLine(
        4,
        GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item,
        GetCharacterEquipment(character)[EQUIP_SLOT_BODY].attachment,
        x,
        y + 16,
        character,
        0
    );
    DrawEquipLine(
        5,
        GetCharacterEquipment(character)[EQUIP_SLOT_ARMS].item,
        GetCharacterEquipment(character)[EQUIP_SLOT_ARMS].attachment,
        x,
        y + 20,
        character,
        GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item
    );
    DrawEquipLine(
        6,
        GetCharacterEquipment(character)[EQUIP_SLOT_LEGS].item,
        GetCharacterEquipment(character)[EQUIP_SLOT_LEGS].attachment,
        x,
        y + 24,
        character,
        GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item
    );
    DrawEquipLine(
        7,
        GetCharacterEquipment(character)[EQUIP_SLOT_ACCESSORY].item,
        GetCharacterEquipment(character)[EQUIP_SLOT_ACCESSORY].attachment,
        x,
        y + 28,
        character,
        0
    );
}

RVA(0x00044ad0, 0xd4)
static void
DrawEquipLine(i16 part, i16 item, i16 attach, i16 x, i16 y, Character* character, i16 gunItem) {
    char mark;

    if (attach >= 0 && attach <= 15) {
        mark = '*';
    } else {
        mark = ' ';
    }
    DrawStatusImage(x, y, part);
    if (item >= 1) {
        sprintf(g_scratchBuffer, "%c%-20.20s", mark, GetLoadedRecordName(item));
    } else if (gunItem >= 1 && GetItemKind(gunItem) == ITEM_KIND_FULL_BODY_ARMOR) {
        sprintf(g_scratchBuffer, "%c%-20.20s", ' ', "--------------------");
    } else {
        sprintf(g_scratchBuffer, "%c%-20.20s", ' ', s_emptyPartLabel);
    }
    if (item >= 1 && IsEquipCurseActive(character, GetItemCategory(item))) {
        DrawStatusLine(
            x + 3,
            y + 1,
            g_scratchBuffer,
            TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
        );
    } else {
        DrawStatusLine(
            x + 3,
            y + 1,
            g_scratchBuffer,
            TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
        );
    }
}

static MenuBox* CreateSkillMenu(i16 member, MenuBox* old);

RVA(0x00044bb0, 0x7c)
i16 OpenItemInfoPlane(i16 item) {
    i16 plane = CreateTextPlane(0x20, 0);
    ItemRecord* record;

    ClearTextPlane(plane);
    record = GetLoadedRecord(item);
    PrintWindowText(plane, GetItemRecordName(record), TEXT_ATTR_DEFAULT, 0, true);
    PrintWindowText(plane, "\n", TEXT_ATTR_DEFAULT, 0, true);
    PrintWindowText(plane, record->description, TEXT_ATTR_DEFAULT, 0, true);
    RepaintTextPlane(plane, -2);
    return plane;
}

RVA(0x00044c30, 0x1c0)
i16 RunSkillPage(i16 sub) {
    if (sub != STATUS_COMMAND_NONE && sub != STATUS_COMMAND_CANCEL) {
        SetGameSub(MENU_STEP_CLOSE);
        s_skillPage.pick = STATUS_COMMAND_CANCEL;
        if (sub != STATUS_STEP_SKILLS) {
            s_skillPage.pick = sub;
        }
    }
    switch (GetGameSub()) {
        case MENU_STEP_OPEN:
            SetGameSub(MENU_STEP_RUN);
            SetStatusMenuItemFlag(STATUS_STEP_SKILLS, PANEL_ROW_CHECKED, true);
            s_skillPage.menu = CreateSkillMenu(g_statusMember, s_skillPage.menu);
            return STATUS_COMMAND_NONE;
        case MENU_STEP_CLOSE:
            s_skillPage.plane = CloseTextWindow(s_skillPage.plane);
            s_skillPage.menu = DestroyMenuBox(s_skillPage.menu);
            SetStatusMenuItemFlag(STATUS_STEP_SKILLS, PANEL_ROW_CHECKED, false);
            return s_skillPage.pick;
        case MENU_STEP_RUN:
            if (sub == STATUS_COMMAND_CANCEL) {
                PrevGameSub();
                s_skillPage.pick = sub;
                return STATUS_COMMAND_NONE;
            }
            if (RunListMenu(s_skillPage.menu) == LIST_MENU_OPEN) {
                break;
            }
            NextGameSub();
            s_skillPage.pick = g_selectedObjectId;
            return STATUS_COMMAND_NONE;
        case SKILL_PAGE_SHOW_DESCRIPTION:
            NextGameSub();
            s_skillPage.plane = CreateTextPlane(0x20, 0);
            ClearTextPlane(s_skillPage.plane);
            PrintWindowText(
                s_skillPage.plane,
                FilterTextMarks(GetSkillDescription(s_skillPage.pick), true),
                TEXT_ATTR_DEFAULT,
                0,
                true
            );
            RepaintTextPlane(s_skillPage.plane, -2);
            return STATUS_COMMAND_NONE;
        case SKILL_PAGE_WAIT_DESCRIPTION:
            if (!TakeClickUnlessCancel(sub)) {
                break;
            }
            s_skillPage.plane = CloseTextWindow(s_skillPage.plane);
            SetGameSub(MENU_STEP_RUN);
            break;
    }
    return STATUS_COMMAND_NONE;
}

static void SkillListHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

RVA(0x00044df0, 0x55)
static MenuBox* CreateSkillMenu(i16 member, MenuBox* old) {
    Character* character = GetRosterCharacter(member);
    MenuBox* menu = CreateMenuBox(old, 9, 2);

    SetMenuItems(
        menu,
        8,
        GetWordArray(GetCharacterSkills(character)),
        GetWordCount(GetCharacterSkills(character)),
        SkillListHandler
    );
    MoveMenuBox(menu, 7, 0xc);
    return menu;
}

RVA(0x00044e50, 0x156)
static void SkillListHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    i16* skills = menu->items.entries;
    SkillView* view;
    char* unit;
    i16 skill;

    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            // "%-16.16s  MP  効果" (effect), "魔法名称" (magic name)
            sprintf(
                g_scratchBuffer,
                "%-16.16s  MP  \214\370\211\312",
                "\226\202\226\100\226\274\217\314"
            );
            AddMenuLine(menu->plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, -1, MENU_LINE_DISABLED);
            AddMenuLine(menu->plane, s_skillHeaderLine, TEXT_ATTR_DEFAULT, -1, MENU_LINE_DISABLED);
            break;
        case MENU_EVENT_ADD_ROW:
            skill = skills[index];
            view = GetSkillView(skill);
            if (skill < 1) {
                AddMenuLine(
                    menu->plane,
                    s_emptySkillLabel,
                    TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                    skill,
                    MENU_LINE_DISABLED
                );
                return;
            }
            if (SkillCostsFullPool(&view->parameters)) {
                unit = GetSkillParameterCost(&view->parameters) < 0 ? "hp" : "mp";
                sprintf(
                    g_scratchBuffer,
                    "%-16.16sMAX%s %-26.26s",
                    view->name,
                    unit,
                    FilterTextMarks(view->description, false)
                );
            } else {
                unit = GetSkillParameterCost(&view->parameters) < 0 ? "hp" : "mp";
                sprintf(
                    g_scratchBuffer,
                    "%-16.16s%3d%s %-26.26s",
                    view->name,
                    abs(GetSkillParameterCost(&view->parameters)),
                    unit,
                    FilterTextMarks(view->description, false)
                );
            }
            AddMenuLine(
                menu->plane,
                g_scratchBuffer,
                TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                skills[index],
                MENU_LINE_NORMAL
            );
            break;
    }
}

static __inline void UnhighlightEquipPart(i16 member) {
    if (s_equipPickPart >= 0) {
        DrawEquipPickRow(
            member,
            s_equipPickPart,
            TEXT_ATTR_OPAQUE | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
        );
    }
}

// @early-stop tail merge: retail keeps the negative-mode reset and return
// at entry; this build shares the final reset. Assignment-return and
// returned-state forms retain the merge, as does the unhighlight macro.
RVA(0x00044fb0, 0x17f)
i16 PollEquipPart(i16 member, i16 mode) {
    i16 x;
    i16 y;
    i16 part;

    if (mode < EQUIP_PICK_PART) {
        s_equipPickPart = -1;
        return -1;
    }
    if (mode < EQUIP_PICK_CLEAR) {
        if (s_equipPickPart >= 0 && g_mouseLeftClick) {
            return s_equipPickPart;
        }
        x = g_mousePosition.x / 8 - 0x37;
        if (x >= 0 && x < 0x15) {
            y = (g_mousePosition.y - 40) / 8 - 3;
            if (y >= 0 && y % 4 != 2 && y % 4 != 3) {
                part = y / 4;
                if (part < EQUIP_SLOT_COUNT) {
                    if (s_equipPickPart == part) {
                        return -1;
                    }
                    if (mode != EQUIP_PICK_ATTACH_TARGET
                        || (part != EQUIP_PART_GUN && part != EQUIP_PART_AMMO
                            && part != EQUIP_PART_ACCESSORY)) {
                        if (!IsEquipCurseActive(
                                GetRosterCharacter(member),
                                s_equipPickCategories[part]
                            )) {
                            UnhighlightEquipPart(member);
                            s_equipPickPart = part;
                            if (DrawEquipPickRow(
                                    member,
                                    part,
                                    TEXT_ATTR_OPAQUE
                                        | TEXT_ATTR(
                                            TEXT_COLOR_GREEN,
                                            TEXT_COLOR_BLACK,
                                            TEXT_COLOR_BLACK
                                        )
                                )
                                < 1) {
                                s_equipPickPart = -1;
                            }
                            return -1;
                        }
                    }
                }
            }
        }
    }
    UnhighlightEquipPart(member);
    s_equipPickPart = -1;
    return -1;
}

RVA(0x00045130, 0x9a)
i16 DrawEquipPickRow(i16 member, GZ_ENUM_PARAM(EquipPart, i16) part, i32 attr) {
    ItemSlot slot = GetRosterEquipSlot(member, part);
    char mark;

    if (slot.attachment == -1) {
        mark = ' ';
    } else {
        mark = '*';
    }

    if (slot.item >= 1) {
        sprintf(g_scratchBuffer, "%c%s", mark, GetLoadedRecordName(slot.item));
    } else {
        sprintf(g_scratchBuffer, "%c%s", 0, g_emptyEquipPickLabel);
    }
    DrawStatusLine(0x37, part * 4 + 3, g_scratchBuffer, attr);
    return slot.item >= 1 ? 1 : -1;
}
