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
#include <Game/FieldSight.h>
#include <Game/GameState.h>
#include <Game/GemItems.h>
#include <Game/ItemBag.h>
#include <Game/ItemBonus.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/Skill.h>
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
#include <Script/ScriptOps.h>
#include <Script/TextToken.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/Menu.h>
#include <Ui/MenuBox.h>
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

// The item kind of ammunition, loaded into the gun's magazine.

DATA(0x000649a0)
const i16 g_equipCountSlots[9] = {0, 1, -1, 4, 3, 4, 5, 6, 7};

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

// The bag entries the attach page lists, and its two header lines.
// The bag entries the equipment menu lists, and its two header lines.
DATA(0x00083b50)
static i16 s_equipEntries[48];

DATA(0x00083c78)
static char s_equipHeaderA[4];

DATA(0x00083c7c)
static char s_equipHeaderB[4];

DATA(0x00083bb0)
static AttachEntry s_attachEntries[48];

DATA(0x00083c80)
static char s_attachHeaderA[4];

DATA(0x00083c84)
static char s_attachHeaderB[4];

// The label of an empty equipment part.
DATA(0x00083c88)
static char s_emptyPartLabel[4];

// The skill page's second header line and the label of an empty skill.
DATA(0x00083c8c)
static char s_skillHeaderLine[4];

DATA(0x00083c90)
static char s_emptySkillLabel[4];

DATA(0x00083c70)
i16 g_previousStatusStep = 0;

DATA(0x00083c74)
char g_emptyBattleSkillLabel[4] = {0};

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

static b16 ResumeStatusPage(i16 command);

RVA(0x00042200, 0x1d4)
i16 RunStatusCommands(void) {
    i16 command;

    if (GetGameStep() >= 1) {
        command = PollStatusMenu();
    } else {
        command = -1;
    }
    if (g_previousStatusStep != GetGameStep()) {
        if (g_previousStatusStep == 8) {
            DrawStatTotals(3, 25, GetRosterCharacter(g_statusMember), NULL);
        }
        g_previousStatusStep = GetGameStep();
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
static b16 ResumeStatusPage(i16 command) {
    u16 step;

    if (command == -1) {
        return true;
    }
    if (command == -2) {
        step = SetGameStep(1);
        HighlightHotspot(0, s_statusCommandHotspots[step], 0);
        return true;
    }
    HighlightHotspot(0, s_statusCommandHotspots[GetGameStep()], 0);
    SetGameStep(command);
    return false;
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

// Maps a signed alignment byte (-128..127) onto the 24-cell alignment chart,
// counting from the far end.
RVA(0x00042ca0, 0x2f)
i16 AlignmentChartCell(i16 value) {
    return 23 - (i16)(((value - -128.0) / 256.0) * 24.0);
}

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
    for (i = 0; i < 48; i++) {
        item = GetBagItem(i);
        if (CanEquipItem(character, item) < 0) {
            continue;
        }
        if (GetCharacterEquipment(character)[1].item >= 1
            && GetItemKind(GetCharacterEquipment(character)[1].item) == ITEM_KIND_FULL_BODY_ARMOR) {
            kind = GetItemKind(item);
            if (kind == ITEM_KIND_HEAD_ARMOR || kind == ITEM_KIND_ARM_ARMOR
                || kind == ITEM_KIND_LEG_ARMOR) {
                continue;
            }
        }
        s_equipEntries[count++] = i;
    }
    if (count == 0 && anyEquipped) {
        if (GetCharacterEquipment(character)[0].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[1].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[2].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[3].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[4].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[5].item >= 1) {
            return 1;
        }
        if (GetCharacterEquipment(character)[6].item >= 1) {
            return 1;
        }
        return GetCharacterEquipment(character)[7].item >= 1;
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

    if (key != -1 && key != -2) {
        SetGameSub(1);
        s_equipPage.pick = -2;
        if (key != 8) {
            s_equipPage.pick = key;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            SetStatusMenuItemFlag(8, PANEL_ROW_CHECKED, 1);
            s_equipPage.menu = OpenEquipMenu(g_statusMember, s_equipPage.menu);
            s_equipPage.panelPlane = CreateTextPlane(0x12, 0);
            ResetTextPlaneLineStep(s_equipPage.panelPlane, 3);
            DrawEquipPanel(GetRosterCharacter(g_statusMember), NULL);
            PollEquipPart(g_statusMember, EQUIP_PICK_RESET);
            return -1;

        case 1:
            s_equipPage.infoPlane = CloseTextWindow(s_equipPage.infoPlane);
            s_equipPage.panelPlane = CloseTextWindow(s_equipPage.panelPlane);
            s_equipPage.menu = DestroyMenuBox(s_equipPage.menu);
            if (s_equipPage.changed) {
                DrawStatusScreen(g_statusMember);
                s_equipPage.changed = false;
            }
            SetStatusMenuItemFlag(8, PANEL_ROW_CHECKED, 0);
            PollEquipPart(g_statusMember, EQUIP_PICK_CLEAR);
            if (s_equipPage.pick != -1) {
                return s_equipPage.pick;
            }
            PrevGameSub();
            return -1;

        case 2:
            if (key == -2) {
                PrevGameSub();
                s_equipPage.pick = key;
                return -1;
            }
            if (RunListMenu(s_equipPage.menu) == -1) {
                part = PollEquipPart(g_statusMember, EQUIP_PICK_PART);
                if (part == -2) {
                    PrevGameSub();
                    s_equipPage.pick = -2;
                    return -1;
                }
                if (part == -1) {
                    return -1;
                }
                s_equipPage.pick = part;
                SetGameSub(5);
                return -1;
            }
            NextGameSub();
            s_equipPage.pick = g_selectedObjectId;
            return -1;

        case 3:
            NextGameSub();
            PreviewEquipChange(s_equipPage.pick, 0);
            s_equipPage.infoPlane = OpenItemInfoPlane(GetBagItem(s_equipPage.pick));
            return -1;

        case 4:
            if (key == -2) {
                SetGameSub(2);
                ClearEquipPreview();
                return -1;
            }
            if (TakeClickUnlessCancel(key) <= 0) {
                return -1;
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
                    GetCharacterEquipment(GetRosterCharacter(g_statusMember))[7].quantity +=
                        slot.quantity;
                    TakeBagItems(slot.item, slot.quantity);
                    return FinishEquipChange();
                }
                LimitItemSlotToBag(&slot);
            } else {
                slot.quantity = 1;
            }
            EquipItem(g_statusMember, slot, count, s_equipPage.pick);
            return FinishEquipChange();

        case 5:
            NextGameSub();
            PreviewEquipChange(s_equipPage.pick, 1);
            slot = GetRosterEquipSlot(g_statusMember, s_equipPage.pick);
            s_equipPage.infoPlane = OpenItemInfoPlane(slot.item);
            return -1;

        case 6:
            if (key == -2) {
                SetGameSub(2);
                ClearEquipPreview();
                PollEquipPart(g_statusMember, EQUIP_PICK_CLEAR);
                return -1;
            }
            if (TakeClickUnlessCancel(key) <= 0) {
                return -1;
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
    return -1;
}

static void EquipMenuHandler(MenuBox* menu, i16 index, i16 event);

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
static void EquipMenuHandler(MenuBox* menu, i16 index, i16 event) {
    Character* member;
    ItemRecord* record;
    i16 item;
    i16 cursed;

    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            AddMenuLine(menu->plane, s_equipHeaderA, 0x400, 0, 1);
            AddMenuLine(menu->plane, s_equipHeaderB, 0x400, 0, 1);
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
                i16 category = GetItemCategory(item);
                if (IsEquipCurseActive(member, category)) {
                    AddMenuLine(menu->plane, g_scratchBuffer, 0x560, s_equipEntries[index], 1);
                    return;
                }
                record = GetLoadedRecord(item);
                if (record->kind == ITEM_KIND_FULL_BODY_ARMOR) {
                    cursed = IsEquipCurseActive(member, EQUIP_PART_HEAD);
                    cursed |= IsEquipCurseActive(member, EQUIP_PART_BODY);
                    cursed |= IsEquipCurseActive(member, EQUIP_PART_ARMS);
                    cursed |= IsEquipCurseActive(member, EQUIP_PART_LEGS);
                    if (cursed) {
                        AddMenuLine(menu->plane, g_scratchBuffer, 0x560, s_equipEntries[index], 1);
                        return;
                    }
                    record = GetLoadedRecord(item);
                }
                if (record->kind == ITEM_KIND_GUN && GetBattleStatShown(member, 6) > 0) {
                    if (LacksItemRequiredStats(member, record, GetBattleStatShown(member, 6))) {
                        AddMenuLine(menu->plane, g_scratchBuffer, 0x760, s_equipEntries[index], 1);
                        return;
                    }
                    AddMenuLine(menu->plane, g_scratchBuffer, 0x460, s_equipEntries[index], 0);
                    return;
                }
                if (LacksItemRequiredStats(member, record, 0)) {
                    AddMenuLine(menu->plane, g_scratchBuffer, 0x760, s_equipEntries[index], 1);
                    return;
                }
            }
            AddMenuLine(menu->plane, g_scratchBuffer, 0x460, s_equipEntries[index], 0);
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
        DrawPlaneText(s_equipPage.panelPlane, 8, y, g_statusBattleLabels[i + 1], 0x400);
        y += 0x18;
    }
    if (preview == NULL) {
        x = DrawStatColumn(5, 5, GetBattleStatGroup(member, 0), NULL);
        x = DrawStatColumn(x, 5, GetBattleStatGroup(member, 1), NULL);
        DrawStatColumn(x, 5, GetBattleStatGroup(member, 2), NULL);
    } else {
        x = DrawStatColumn(5, 5, GetBattleStatGroup(member, 0), GetBattleStatGroup(preview, 0));
        x = DrawStatColumn(x, 5, GetBattleStatGroup(member, 1), GetBattleStatGroup(preview, 1));
        DrawStatColumn(x, 5, GetBattleStatGroup(member, 2), GetBattleStatGroup(preview, 2));
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
    i32 attr = 0x400;

    if (newValue < 0) {
        sprintf(g_scratchBuffer, "  %3d    ", value);
    } else {
        sprintf(g_scratchBuffer, "  %3d>%3d", value, newValue);
        if (value < newValue) {
            attr = 0x600;
        } else if (value > newValue) {
            attr = 0x500;
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
    i16 result;
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
            slot.quantity =
                GetGunMagazineSize(GetLoadedRecord(GetCharacterEquipment(member)[6].item));
            LimitItemSlotToBag(&slot);
        } else if (kind == ITEM_KIND_FULL_BODY_ARMOR) {
            EmptyItemSlot(&GetCharacterEquipment(member)[0]);
            EmptyItemSlot(&GetCharacterEquipment(member)[2]);
            EmptyItemSlot(&GetCharacterEquipment(member)[3]);
        } else {
            // Kind 19 selects index 7, overwriting returnPosition.area in
            // the saved preview copy; retain this original store.
            saved->ammoCounts[g_equipCountSlots[kind - ITEM_KIND_WEAPON]] = count;
            slot.quantity = 1;
        }
        if (kind == ITEM_KIND_GUN) {
            gun = GetCharacterEquipment(member)[6].item;
            GetCharacterEquipment(member)[6].item = slot.item;
            if (CanEquipItem(member, GetCharacterEquipment(member)[7].item) < 1) {
                EmptyItemSlot(&GetCharacterEquipment(member)[7]);
            }
            GetCharacterEquipment(member)[6].item = gun;
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
    if (sub != -1 && sub != -2) {
        SetGameSub(1);
        s_attach.resume = -2;
        if (sub != 9) {
            s_attach.resume = sub;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            s_attach.itemBase = GetGemItemBase();
            SetStatusMenuItemFlag(9, PANEL_ROW_CHECKED, 1);
            s_attach.itemMenu = CreateAttachItemMenu(s_attach.itemMenu);
            s_attach.plane = CreateTextPlane(0x12, 0);
            ResetTextPlaneLineStep(s_attach.plane, 3);
            s_attach.prevHook = SetTextPlaneHook(AttachTextHook);
            return -1;
        case 1:
            SetTextPlaneHook(s_attach.prevHook);
            s_attach.prevHook = NULL;
            s_attach.plane = CloseTextWindow(s_attach.plane);
            s_attach.entryMenu = DestroyMenuBox(s_attach.entryMenu);
            s_attach.itemMenu = DestroyMenuBox(s_attach.itemMenu);
            PollEquipPart(g_statusMember, EQUIP_PICK_CLEAR);
            if (s_attach.redraw) {
                DrawStatusScreen(g_statusMember);
                s_attach.redraw = 0;
            }
            SetStatusMenuItemFlag(9, PANEL_ROW_CHECKED, 0);
            if (s_attach.resume == -1) {
                PrevGameSub();
                return -1;
            }
            return s_attach.resume;
        case 2:
            if (sub == -2) {
                PrevGameSub();
                s_attach.resume = -2;
                return -1;
            }
            if (RunListMenu(s_attach.itemMenu) == -1 || g_selectedObjectId < 0) {
                break;
            }
            NextGameSub();
            SetTextPlaneHook(s_attach.prevHook);
            s_attach.prevHook = NULL;
            s_attach.plane = CloseTextWindow(s_attach.plane);
            s_attach.itemMenu = DestroyMenuBox(s_attach.itemMenu);
            s_attach.item = g_selectedObjectId;
            return -1;
        case 3:
            NextGameSub();
            s_attach.entryMenu = CreateAttachEntryMenu(s_attach.entryMenu);
            PollEquipPart(g_statusMember, EQUIP_PICK_RESET);
            return -1;
        case 4:
            if (sub == -2) {
                SetGameSub(1);
                s_attach.resume = -2;
                return -1;
            }
            if (RunListMenu(s_attach.entryMenu) == -1) {
                sub = PollEquipPart(g_statusMember, EQUIP_PICK_ATTACH_TARGET);
                if (sub == -2) {
                    SetGameSub(1);
                    s_attach.resume = -2;
                    return -1;
                }
                if (sub == -1) {
                    break;
                }
                s_attach.target = sub;
                s_attach.entryMenu = DestroyMenuBox(s_attach.entryMenu);
                SetGameSub(7);
                return -1;
            }
            NextGameSub();
            s_attach.target = g_selectedObjectId;
            s_attach.entryMenu = DestroyMenuBox(s_attach.entryMenu);
            return -1;
        case 5:
            NextGameSub();
            s_attach.plane = CreateTextPlane(0x12, 0);
            sprintf(g_scratchBuffer, "%s", GetLoadedRecordName(s_attach.item));
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 1);
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
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 1);
            RepaintTextPlane(s_attach.plane, -2);
            PushWaitState(WAIT_INPUT, 0xffff, 0xffff, 0);
            return -1;
        case 6:
            SetGameSub(1);
            s_attach.plane = CloseTextWindow(s_attach.plane);
            s_attach.resume = -1;
            return -1;
        case 7:
            NextGameSub();
            s_attach.plane = CreateTextPlane(0x12, 0);
            sprintf(g_scratchBuffer, "%s", GetLoadedRecordName(s_attach.item));
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 1);
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
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 1);
            RepaintTextPlane(s_attach.plane, -2);
            PushWaitState(WAIT_INPUT, 0xffff, 0xffff, 0);
            return -1;
        case 8:
            SetGameSub(1);
            s_attach.plane = CloseTextWindow(s_attach.plane);
            RecalcCharacterStats(GetRosterCharacter(g_statusMember));
            s_attach.redraw = 1;
            s_attach.resume = -1;
            return -1;
    }
    return -1;
}

static void AttachItemMenuHandler(MenuBox* menu, i16 index, i16 event);

RVA(0x000442f0, 0x49)
static MenuBox* CreateAttachItemMenu(MenuBox* old) {
    MenuBox* menu = CreateMenuBox(old, 0x15, 2);

    SetMenuItems(menu, 16, NULL, 16, AttachItemMenuHandler);
    MoveMenuBox(menu, 0x2a, 0x50);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, 0);
    return menu;
}

RVA(0x00044340, 0x86)
static void AttachItemMenuHandler(MenuBox* menu, i16 index, i16 event) {
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
                AddMenuLine(menu->plane, g_scratchBuffer, 0x560, index + s_attach.itemBase, 2);
            } else {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x460, index + s_attach.itemBase, 0);
            }
            break;
    }
}

static i16 ListAttachEntries(void);
static void AttachEntryMenuHandler(MenuBox* menu, i16 index, i16 event);

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
    for (i = 0; i < 48; i++) {
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
static void AttachEntryMenuHandler(MenuBox* menu, i16 index, i16 event) {
    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            AddMenuLine(menu->plane, s_attachHeaderA, 0x400, 0, 1);
            AddMenuLine(menu->plane, s_attachHeaderB, 0x400, 0, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            sprintf(
                g_scratchBuffer,
                "%c %-20.20s %2d",
                GetBagEntryAttachment(s_attachEntries[index].entry) != -1 ? '*' : ' ',
                GetLoadedRecordName(GetBagItem(s_attachEntries[index].entry)),
                s_attachEntries[index].count
            );
            AddMenuLine(menu->plane, g_scratchBuffer, 0x460, s_attachEntries[index].entry, 0);
            break;
    }
}

RVA(0x00044590, 0xb8)
static void AttachTextHook(i16 plane, i16 event, i16 value) {
    if (plane == -1) {
        return;
    }
    switch (event) {
        case -1:
        case 1:
        case 2:
            return;
        case 3:
            ClearTextPlane(s_attach.plane);
            break;
        case 4:
            strcpy(g_scratchBuffer, GetItemDescription(value + s_attach.itemBase));
            PrintWindowText(s_attach.plane, g_scratchBuffer, 0x400, 0, 0);
            break;
    }
    RepaintTextPlane(s_attach.plane, -2);
}

static void ItemListHandler(MenuBox* menu, i16 index, i16 event);

RVA(0x00044650, 0x1e0)
i16 RunItemPage(i16 sub) {
    if (sub != -1 && sub != -2) {
        SetGameSub(1);
        s_itemPage.pick = -2;
        if (sub != 3) {
            s_itemPage.pick = sub;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            CompactBag();
            SetStatusMenuItemFlag(3, PANEL_ROW_CHECKED, 1);
            s_itemPage.menu = CreateMenuBox(s_itemPage.menu, 0x19, 2);
            MoveMenuBox(s_itemPage.menu, -8, -0x16);
            s_itemPage.list = CopyBagEntries(0, 64, NULL);
            SetMenuItems(
                s_itemPage.menu,
                8,
                s_itemPage.list,
                GetItemListCount(s_itemPage.list),
                ItemListHandler
            );
            SetTextPlaneFirstSelectableRow(s_itemPage.menu->plane, 1, 1);
            return -1;
        case 1:
            s_itemPage.plane = CloseTextWindow(s_itemPage.plane);
            s_itemPage.menu = CloseListMenu(s_itemPage.menu);
            SetStatusMenuItemFlag(3, PANEL_ROW_CHECKED, 0);
            return s_itemPage.pick;
        case 2:
            if (sub == -2) {
                PrevGameSub();
                s_itemPage.pick = sub;
                return -1;
            }
            if (RunListMenu(s_itemPage.menu) == -1) {
                break;
            }
            NextGameSub();
            s_itemPage.pick = g_selectedObjectId;
            return -1;
        case 3:
            if (sub == -2) {
                PrevGameSub();
                return -1;
            }
            NextGameSub();
            s_itemPage.plane = OpenItemInfoPlane(s_itemPage.pick);
            return -1;
        case 4:
            if (sub != -2 && !TakeMouseLeftClick()) {
                break;
            }
            s_itemPage.plane = CloseTextWindow(s_itemPage.plane);
            SetGameSub(2);
            break;
    }
    return -1;
}

RVA(0x00044830, 0x109)
static void ItemListHandler(MenuBox* menu, i16 index, i16 event) {
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
            AddMenuLine(menu->plane, g_scratchBuffer, 0x400, -1, 1);
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
            AddMenuLine(menu->plane, g_scratchBuffer, 0x460, item, 0);
            break;
    }
}

static void
DrawEquipLine(i16 part, i16 item, i16 attach, i16 x, i16 y, Character* character, i16 gunItem);

RVA(0x00044940, 0x18a)
void DrawEquipLines(Character* character, i16 x, i16 y) {
    DrawEquipLine(
        0,
        GetCharacterEquipment(character)[5].item,
        GetCharacterEquipment(character)[5].attachment,
        x,
        y,
        character,
        0
    );
    DrawEquipLine(
        1,
        GetCharacterEquipment(character)[6].item,
        GetCharacterEquipment(character)[6].attachment,
        x,
        y + 4,
        character,
        0
    );
    DrawEquipLine(
        2,
        GetCharacterEquipment(character)[7].item,
        GetCharacterEquipment(character)[7].attachment,
        x,
        y + 8,
        character,
        0
    );
    DrawEquipLine(
        3,
        GetCharacterEquipment(character)[0].item,
        GetCharacterEquipment(character)[0].attachment,
        x,
        y + 12,
        character,
        GetCharacterEquipment(character)[1].item
    );
    DrawEquipLine(
        4,
        GetCharacterEquipment(character)[1].item,
        GetCharacterEquipment(character)[1].attachment,
        x,
        y + 16,
        character,
        0
    );
    DrawEquipLine(
        5,
        GetCharacterEquipment(character)[2].item,
        GetCharacterEquipment(character)[2].attachment,
        x,
        y + 20,
        character,
        GetCharacterEquipment(character)[1].item
    );
    DrawEquipLine(
        6,
        GetCharacterEquipment(character)[3].item,
        GetCharacterEquipment(character)[3].attachment,
        x,
        y + 24,
        character,
        GetCharacterEquipment(character)[1].item
    );
    DrawEquipLine(
        7,
        GetCharacterEquipment(character)[4].item,
        GetCharacterEquipment(character)[4].attachment,
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
        DrawStatusLine(x + 3, y + 1, g_scratchBuffer, 0x1500);
    } else {
        DrawStatusLine(x + 3, y + 1, g_scratchBuffer, 0x1400);
    }
}

static MenuBox* CreateSkillMenu(i16 member, MenuBox* old);

RVA(0x00044bb0, 0x7c)
i16 OpenItemInfoPlane(i16 item) {
    i16 plane = CreateTextPlane(0x20, 0);
    ItemRecord* record;

    ClearTextPlane(plane);
    record = GetLoadedRecord(item);
    PrintWindowText(plane, GetItemRecordName(record), 0x400, 0, 1);
    PrintWindowText(plane, "\n", 0x400, 0, 1);
    PrintWindowText(plane, record->description, 0x400, 0, 1);
    RepaintTextPlane(plane, -2);
    return plane;
}

RVA(0x00044c30, 0x1c0)
i16 RunSkillPage(i16 sub) {
    if (sub != -1 && sub != -2) {
        SetGameSub(1);
        s_skillPage.pick = -2;
        if (sub != 4) {
            s_skillPage.pick = sub;
        }
    }
    switch (GetGameSub()) {
        case 0:
            SetGameSub(2);
            SetStatusMenuItemFlag(4, PANEL_ROW_CHECKED, 1);
            s_skillPage.menu = CreateSkillMenu(g_statusMember, s_skillPage.menu);
            return -1;
        case 1:
            s_skillPage.plane = CloseTextWindow(s_skillPage.plane);
            s_skillPage.menu = DestroyMenuBox(s_skillPage.menu);
            SetStatusMenuItemFlag(4, PANEL_ROW_CHECKED, 0);
            return s_skillPage.pick;
        case 2:
            if (sub == -2) {
                PrevGameSub();
                s_skillPage.pick = sub;
                return -1;
            }
            if (RunListMenu(s_skillPage.menu) == -1) {
                break;
            }
            NextGameSub();
            s_skillPage.pick = g_selectedObjectId;
            return -1;
        case 3:
            NextGameSub();
            s_skillPage.plane = CreateTextPlane(0x20, 0);
            ClearTextPlane(s_skillPage.plane);
            PrintWindowText(
                s_skillPage.plane,
                FilterTextMarks(GetSkillDescription(s_skillPage.pick), 1),
                0x400,
                0,
                1
            );
            RepaintTextPlane(s_skillPage.plane, -2);
            return -1;
        case 4:
            if (!TakeClickUnlessCancel(sub)) {
                break;
            }
            s_skillPage.plane = CloseTextWindow(s_skillPage.plane);
            SetGameSub(2);
            break;
    }
    return -1;
}

static void SkillListHandler(MenuBox* menu, i16 index, i16 event);

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
static void SkillListHandler(MenuBox* menu, i16 index, i16 event) {
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
            AddMenuLine(menu->plane, g_scratchBuffer, 0x400, -1, 1);
            AddMenuLine(menu->plane, s_skillHeaderLine, 0x400, -1, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            skill = skills[index];
            view = GetSkillView(skill);
            if (skill < 1) {
                AddMenuLine(menu->plane, s_emptySkillLabel, 0x460, skill, 1);
                return;
            }
            if (SkillCostsFullPool(&view->parameters)) {
                unit = GetSkillParameterCost(&view->parameters) < 0 ? "hp" : "mp";
                sprintf(
                    g_scratchBuffer,
                    "%-16.16sMAX%s %-26.26s",
                    view->name,
                    unit,
                    FilterTextMarks(view->description, 0)
                );
            } else {
                unit = GetSkillParameterCost(&view->parameters) < 0 ? "hp" : "mp";
                sprintf(
                    g_scratchBuffer,
                    "%-16.16s%3d%s %-26.26s",
                    view->name,
                    abs(GetSkillParameterCost(&view->parameters)),
                    unit,
                    FilterTextMarks(view->description, 0)
                );
            }
            AddMenuLine(menu->plane, g_scratchBuffer, 0x460, skills[index], 0);
            break;
    }
}

static __inline void UnhighlightEquipPart(i16 member) {
    if (s_equipPickPart >= 0) {
        DrawEquipPickRow(member, s_equipPickPart, 0x1400);
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
                if (part < 8) {
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
                            if (DrawEquipPickRow(member, part, 0x1600) < 1) {
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
i16 DrawEquipPickRow(i16 member, i16 part, i32 attr) {
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
