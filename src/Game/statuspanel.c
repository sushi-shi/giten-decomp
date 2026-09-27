// @identity-TODO: the owning TU is unproven; this unit holds the status
// drawing span until link-order evidence names it.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/CharInfo.h>
#include <Game/ClickWait.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/EquipScreen.h>
#include <Game/FieldSight.h>
#include <Game/GameState.h>
#include <Game/LevelUp.h>
#include <Game/ObjectRecord.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Game/StatusScreen.h>
#include <Gfx/Background.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/Vram.h>
#include <Script/EventFlags.h>
#include <Script/ScriptOps.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/Panel.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

DATA(0x0006a0f8)
static i16 s_statusCommandHotspots[11] = {-1, -1, -1, 55, 56, 57, -1, -1, 60, 61, 62};

DATA(0x00083c70)
static i16 s_previousStatusStep;

DATA(0x0006499c)
static const i8 s_battleStatIcons[4] = {0, 1, 8, 9};

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
static i16 s_alignmentPlane = -1;

DATA(0x0006a166)
static i16 s_alignmentResume = -1;

DATA(0x0006a110)
static i16 s_statPlane = -1;

DATA(0x0006a112)
static i16 s_statResume = -1;

static i16 DrawStatBar(i16 x, i16 y, i16 base, i16 bonus, i16 equipment, i16 band);
static i16 DrawStatBarSegment(i16 first, i16 last, i16 x, i16 y, i16 offset, i16 mark, i16 band);

RVA(0x00041ad0, 0xc5)
static i16 DrawStatusExperience(i16 x, i16 y, Character* member) {
    i32 remaining;

    sprintf(g_scratchBuffer, "EXP %10ld", member->experience);
    y = DrawStatusLine(x, y, g_scratchBuffer, 0x1400);
    if (member->level >= 99) {
        strcpy(g_scratchBuffer, "\215\305\215\202\203\214\203\170\203\213\202\305\202\267");
    } else {
        remaining = ExperienceForLevel(member->level + 1, member->id) - member->experience;
        if (remaining < 0) {
            remaining = 0;
        }
        sprintf(g_scratchBuffer, "NEXT %9ld", remaining);
    }
    return DrawStatusLine(x, y, g_scratchBuffer, 0x1400);
}

RVA(0x00041ba0, 0x89)
static i16 DrawStatusPools(i16 x, i16 y, Character* member) {
    sprintf(g_scratchBuffer, "HP  %4d\201\136%4d", member->pools.hp.cur, member->pools.hp.max);
    y = DrawStatusLine(x, y, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "MP    %3d\201\136%3d", member->pools.mp.cur, member->pools.mp.max);
    return DrawStatusLine(x, y, g_scratchBuffer, 0x1400);
}

RVA(0x00041c30, 0xb3)
static i16 DrawStatusLevel(i16 x, i16 y, Character* member) {
    sprintf(g_scratchBuffer, "LEVEL %8d", member->level);
    y = DrawStatusLine(x, y, g_scratchBuffer, 0x1400);
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
        if (member->id == 0) {
            if (!IsEventFlagSet(1, 0x5e)) {
                file = 0x4001;
                mode = 1;
            }
        } else if (member->id == 2) {
            if (!IsEventFlagSet(1, 0x75)) {
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
            CountRosterEntries(0),
            GetFieldCount() - 6
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
        SetStatusMenuItemFlag(8, 0x800, 0);
    } else {
        SetStatusMenuItemFlag(8, 0x800, 1);
    }
    member = GetRosterCharacter(g_statusMember);
    if (!GetWordCount(GetCharacterSkills(member))) {
        SetStatusMenuItemFlag(4, 0x800, 1);
    } else {
        SetStatusMenuItemFlag(4, 0x800, 0);
    }
    if (!g_statusFixedMember && CountRosterEntries(1) >= 2) {
        SetStatusMenuItemFlag(6, 0x800, 0);
    } else {
        SetStatusMenuItemFlag(6, 0x800, 1);
    }
    if (g_statusFixedMember) {
        SetStatusMenuItemFlag(7, 0x800, 1);
        SetStatusMenuItemFlag(9, 0x800, 1);
        SetStatusMenuItemFlag(3, 0x800, 1);
    } else {
        SetStatusMenuItemFlag(7, 0x800, 0);
        SetStatusMenuItemFlag(9, 0x800, 0);
        SetStatusMenuItemFlag(3, 0x800, 0);
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

static i16 ResumeStatusPage(i16 command);

RVA(0x00042200, 0x1d4)
i16 RunStatusCommands(void) {
    i16 command;

    if (GetGameStep() >= 1) {
        command = PollStatusMenu();
    } else {
        command = -1;
    }
    if (s_previousStatusStep != GetGameStep()) {
        if (s_previousStatusStep == 8) {
            DrawStatTotals(3, 25, GetRosterCharacter(g_statusMember), NULL);
        }
        s_previousStatusStep = GetGameStep();
    }
    switch (GetGameStep()) {
        case 0:
            NextGameStep();
            DrawStatusScreen(g_statusMember);
            break;
        case 1:
            if (command == -2) {
                return g_statusFixedMember ? -3 : -2;
            }
            if (command != -1) {
                SetGameStep(command);
            }
            break;
        case 2:
            return -3;
        case 4:
            ResumeStatusPage(RunSkillPage(command));
            break;
        case 5:
            ResumeStatusPage(RunStatPage(command));
            break;
        case 6:
            do {
                g_statusMember++;
                if (g_statusMember >= 32) {
                    g_statusMember = 0;
                }
            } while (!GetRosterEntry(g_statusMember));
            SetGameStep(0);
            ClearStatusPicture();
            break;
        case 7:
            return 7;
        case 8:
            ResumeStatusPage(RunEquipScreen(command));
            break;
        case 9:
            ResumeStatusPage(RunAttachScreen(command));
            break;
        case 10:
            ResumeStatusPage(RunAlignmentPage(command));
            break;
        case 3:
            if (!GetStatusAnalyzeMode()) {
                command = RunItemPage(command);
            }
            ResumeStatusPage(command);
            break;
    }
    return -1;
}

RVA(0x000423e0, 0x6c)
static i16 ResumeStatusPage(i16 command) {
    u16 step;

    if (command == -1) {
        return 1;
    }
    if (command == -2) {
        step = SetGameStep(1);
        HighlightHotspot(0, s_statusCommandHotspots[step], 0);
        return 1;
    }
    HighlightHotspot(0, s_statusCommandHotspots[GetGameStep()], 0);
    SetGameStep(command);
    return 0;
}

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
        PrintWindowText(plane, g_scratchBuffer, 0x1400, 0, 1);
    }
    RepaintTextPlane(plane, -2);
    s_statPlane = plane;
    y = 1;
    for (stat = 0; stat < 10; stat++) {
        DrawStatBar(
            12,
            y,
            GetBaseStat(member, stat),
            GetStatBonus(member, stat),
            GetStatEquipment(member, stat),
            0
        );
        y += 2;
    }
    return plane;
}

RVA(0x00042520, 0x9e)
static i16 DrawStatBar(i16 x, i16 y, i16 base, i16 bonus, i16 equipment, i16 band) {
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
static i16 DrawStatBarSegment(i16 first, i16 last, i16 x, i16 y, i16 offset, i16 mark, i16 band) {
    i16 row;
    i16 column;

    for (; first < last; first++) {
        row = offset / 50;
        column = offset % 50;
        if (row < 2) {
            if (band == 0 || (band == -1 && row == 0) || (band == 1 && row == 1)) {
                DrawStatBarMark(x + column, y, mark, s_statPlane);
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
    if (command != -1 && command != -2) {
        SetGameSub(1);
        s_statResume = -2;
        if (command != 5) {
            s_statResume = command;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            SetStatusMenuItemFlag(5, PANEL_ROW_CHECKED, 1);
            s_statPlane = OpenStatListWindow(GetRosterCharacter(g_statusMember));
            break;
        case 1:
            s_statPlane = CloseTextWindow(s_statPlane);
            SetStatusMenuItemFlag(5, PANEL_ROW_CHECKED, 0);
            return s_statResume;
        case 2:
            if (TakeClickUnlessCancel(command)) {
                PrevGameSub();
                s_statResume = -2;
            }
            break;
    }
    return -1;
}

RVA(0x00042780, 0xa5)
i16 DrawStatTotals(i16 x, i16 y, Character* member, Character* compare) {
    i16 stat;
    i32 attr;

    for (stat = 0; stat < 10; stat++) {
        sprintf(g_scratchBuffer, "%-6.6s %7d", g_statusStatNames[stat], GetStatTotal(member, stat));
        if (!compare) {
            y = DrawStatusLine(x, y, g_scratchBuffer, 0x1400);
        } else {
            attr = 0x1400;
            if (GetStatTotal(member, stat) < GetStatTotal(compare, stat)) {
                attr = 0x1600;
            } else if (GetStatTotal(member, stat) > GetStatTotal(compare, stat)) {
                attr = 0x1500;
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
                -1
            );
        }
    } else {
        DrawStatBar(
            12,
            stat * 2 + 1,
            GetBaseStat(member, stat),
            GetStatBonus(member, stat),
            GetStatEquipment(member, stat),
            0
        );
        sprintf(g_scratchBuffer, "%-6.6s %3d", g_statusStatNames[stat], GetStatTotal(member, stat));
        SetTextPlaneCursorLine(window, 0, stat);
        PrintWindowText(window, g_scratchBuffer, 0x1400, 0, 1);
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
    return DrawStatusLine(x, y, g_scratchBuffer, 0x1400);
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
    x = DrawBattleStatColumn(x + 6, startY, GetBattleStatGroup(member, 0), icon, member->id);
    gunX = x;
    x = DrawBattleStatColumn(x, startY, GetBattleStatGroup(member, 1), icon + 1, member->id);
    DrawStatusNumber(gunX, startY + 13, GetCharacterEquipment(member)[7].quantity);
    x = DrawBattleStatColumn(x, startY, GetBattleStatGroup(member, 2), icon + 2, member->id);
    if (icon >= 0 && IsHumanCharacter(member)) {
        DrawBattleStatColumn(x, startY, GetBattleStatGroup(member, 3), icon + 3, member->id);
    }
}

static void DrawAlignmentMarker(i16 slot, Character* member);

RVA(0x00042b00, 0x108)
i16 RunAlignmentPage(i16 command) {
    if (command != -1 && command != -2) {
        SetGameSub(1);
        s_alignmentResume = -2;
        if (command != 10) {
            s_alignmentResume = command;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            SetStatusMenuItemFlag(10, PANEL_ROW_CHECKED, 1);
            s_alignmentPlane = CreateTextPlane(6, 0);
            ResetTextPlaneLineStep(s_alignmentPlane, 1);
            DrawAlignmentMarker(g_statusMember, GetRosterCharacter(g_statusMember));
            RepaintTextPlane(s_alignmentPlane, -2);
            break;
        case 1:
            s_alignmentPlane = CloseTextWindow(s_alignmentPlane);
            SetStatusMenuItemFlag(10, PANEL_ROW_CHECKED, 0);
            return s_alignmentResume;
        case 2:
            if (TakeClickUnlessCancel(command)) {
                PrevGameSub();
                s_alignmentResume = -2;
            }
            break;
    }
    return -1;
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
    SetTextPlaneCursorLine(s_alignmentPlane, x, y);
    sprintf(g_scratchBuffer, "%s", g_statusNumberLabels[slot + 1]);
    DrawPlaneText(s_alignmentPlane, x * 8, y * 8, g_scratchBuffer, 0x700);
}
