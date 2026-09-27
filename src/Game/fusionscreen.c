// @identity-TODO: the owning TU is unproven; this unit holds the fusion
// screen until the surrounding handlers are recovered.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/DemonTable.h>
#include <Game/Fusion.h>
#include <Game/FusionCompare.h>
#include <Game/FusionScreen.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/LevelUp.h>
#include <Game/Party.h>
#include <Game/StatusScreen.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/Vram.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Text/Font.h>
#include <Text/TextWindow.h>
#include <Ui/Panel.h>
#include <Util/Level.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

DATA(0x00068f88)
static i16 s_fusionInfoPlane = -1;

DATA(0x00068f8c)
static i16 s_fusionPageRows = 1;

DATA(0x00068f90)
static i16 s_fusionColumnCount = 1;

DATA(0x00068f98)
// D / N / L alignment labels.
static char* s_fusionAlignmentALabels[3] = {"\202\143", "\202\155", "\202\153"};

DATA(0x00068fa8)
// C / N / L alignment labels.
static char* s_fusionAlignmentBLabels[3] = {"\202\142", "\202\155", "\202\153"};

DATA(0x00068fb4)
static i16 s_selectedFusionIndex = -1;

DATA(0x00068fb8)
i16 g_fusionFirstSlot = -1;

DATA(0x00068fbc)
i16 g_fusionSecondSlot = -1;

DATA(0x00068fc0)
i16 g_fusionThirdSlot = -1;

// @identity-TODO: the individual contents of these auxiliary planes are unproven.
DATA(0x00068fc4)
static i16 s_firstFusionDetailPlane = -1;

DATA(0x00068fc8)
static i16 s_secondFusionDetailPlane = -1;

DATA(0x00068fcc)
static i16 s_thirdFusionDetailPlane = -1;

DATA(0x00068fd0)
static i16 s_pendingFusionResultId = -1;

DATA(0x00068fd4)
static i16 s_fusionPreviewPlane = -1;

DATA(0x00068fe0)
static i16 s_fusionSummaryIcons[14] = {14, 13, 3, 4, 5, 6, 7, 8, 9, 11, 12, 0, 1, 2};

DATA(0x00080198)
static i16 s_fusionPageAction;

DATA(0x000801a0)
static FusionSummary s_fusionPairSummaries[32 * 32];

DATA(0x000809a0)
static FusionSummary s_fusionSummary;

DATA(0x000809a8)
static i16 s_fusionSlots[32];

DATA(0x000809e8)
static u32 s_fusionSelectionImage;

DATA(0x000809ec)
static MenuBox* s_fusionMenu;

DATA(0x000809f0)
static TextPlaneHook s_previousFusionTextHook;

DATA(0x000809f4)
static i16 s_fusionColumnOffset;

DATA(0x000809f8)
static Panel* s_fusionPager;

DATA(0x000809fc)
static FusionSummary s_cachedFusionSummary;

DATA(0x00080a00)
static PaletteState* s_fusionSelectionPaletteState;

DATA(0x00080a04)
static i16 s_fusionResultId;

DATA(0x00080a08)
static i16 s_fusionCandidateCount;

DATA(0x00080a0c)
static Character* s_savedFusionCharacter;

DATA(0x00080a20)
PaletteState* g_fusionPaletteState;

DATA(0x00080a24)
static FusionSummary* s_fusionSummaryTable;

// @dead-code
// Zero-ref: no retail call, jump, or relocated pointer reaches this helper.
DATA(0x00080a84)
static i16 s_fusionPageActionPending;

RVA(0x00028840, 0x16)
void AcquireFusionSelectionMode(void) {
    s_fusionSelectionPaletteState = SavePaletteState(s_fusionSelectionPaletteState, 1);
}

// @dead-code
// Zero-ref: no retail call, jump, or relocated pointer reaches this helper.
RVA(0x00028860, 0x2a)
void ReleaseFusionSelectionResources(void) {
    s_fusionSelectionImage = FreeImageHandle(s_fusionSelectionImage);
    s_fusionSelectionPaletteState = RestorePaletteState(s_fusionSelectionPaletteState, 1);
}

RVA(0x00028890, 0xa)
void ResetThirdFusionSlot(void) {
    g_fusionThirdSlot = -1;
}

RVA(0x000288a0, 0x7)
i16 GetFirstFusionSlot(void) {
    return g_fusionFirstSlot;
}

RVA(0x000288b0, 0x7)
i16 GetSecondFusionSlot(void) {
    return g_fusionSecondSlot;
}

RVA(0x000288c0, 0x7)
i16 GetThirdFusionSlot(void) {
    return g_fusionThirdSlot;
}

RVA(0x000288d0, 0x75)
i16 GetFusionResultKind(void) {
    if (s_fusionSummary.fields.overLevel) {
        return -3;
    }
    switch (s_fusionSummary.fields.kind) {
        case 0:
        case 3:
        case 4:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            if (RosterContainsId(s_fusionResultId)) {
                return -2;
            }
            break;
        case 1:
        case 2:
            return s_fusionSummary.fields.kind;
        case 5:
        case 6:
        case 7:
            return s_fusionSummary.fields.kind;
    }
    return s_fusionSummary.fields.kind;
}

RVA(0x00028950, 0x28f)
Character* CreatePairFusionCharacter(i16 first, i16 second, i16 rankChanges) {
    Character* firstCharacter = GetRosterEntry(first);
    Character* secondCharacter = GetRosterEntry(second);
    Character* result;
    i16 pending;
    if (firstCharacter == NULL || secondCharacter == NULL) {
        return NULL;
    }
    pending = s_pendingFusionResultId;
    s_pendingFusionResultId = -1;
    if (pending > 1) {
        s_fusionResultId = pending;
        s_fusionSummary = s_cachedFusionSummary;
    } else {
        s_fusionResultId = ResolvePairFusion(first, second, rankChanges);
        s_fusionSummary = GetPairFusionSummary(first, second);
        if (s_fusionResultId < 1) {
            return NULL;
        }
    }
    if (s_fusionSummary.fields.kind < 0) {
        return NULL;
    }
    switch (s_fusionSummary.fields.kind) {
        case 1:
        case 2:
            MoveSpecialFusionCharacters(&firstCharacter, &secondCharacter, &first, &second);
            result = CopyCharacter(firstCharacter, NULL);
            GainLevels(result, s_fusionSummary.fields.kind);
            result->level = ClampLevel(result->level + GetFusionGrowthBonus(first, second));
            RaiseExperienceToLevel(result);
            FullyRestoreCharacter(result);
            return result;
        case 5:
        case 6:
        case 7:
            MoveSpecialFusionCharacters(&firstCharacter, &secondCharacter, &first, &second);
            result = CopyCharacter(firstCharacter, NULL);
            if (s_fusionSummary.fields.kind <= 6) {
                result->pools.hp.cur += secondCharacter->pools.hp.cur;
                result->pools.mp.cur += secondCharacter->pools.mp.cur;
                if (s_fusionSummary.fields.kind == 6) {
                    ClearAllConditions(GetCharacterConditions(result));
                }
            }
            break;
        case 4:
            goto createCharacter;
        case 9:
            goto createCharacter;
        case 11:
            goto createCharacter;
        default:
        createCharacter:
            result = LoadCharacterCore(s_fusionResultId, NULL);
            InheritFusionStats(firstCharacter, secondCharacter, result);
            result->level = ClampLevel(result->level + GetFusionGrowthBonus(first, second));
            RaiseExperienceToLevel(result);
            FullyRestoreCharacter(result);
            if (s_fusionSummary.fields.kind == 4) {
                GainLevels(result, 1);
            }
            break;
    }
    return result;
}

RVA(0x00028be0, 0x290)
i16 RunFirstFusionPicker(i16 step, i16 triple) {
    i16 count;
    i16 result;
    i16 oldOffset;
    switch (step) {
        case 0:
            g_fusionFirstSlot = -1;
            g_fusionSecondSlot = -1;
            CountRosterEntries(1);
            s_fusionInfoPlane = CreateFusionInfoPlane(6);
            if (!triple) {
                g_fusionThirdSlot = -1;
                count = BuildPairFusionCandidates(0);
            } else {
                count = BuildTripleFusionSummaries(g_fusionThirdSlot);
            }
            s_fusionPageRows = -1;
            CreateFusionList(3, count);
            s_fusionColumnOffset = 0;
            s_fusionColumnCount = count;
            PaintMenuBox(s_fusionMenu);
            if (s_fusionPageRows < count) {
                s_fusionPager = CreateKindPanel(s_fusionPager, 284, 2, 1);
                PaintPanel(s_fusionPager, s_fusionMenu->plane);
            }
            ++step;
            break;
        case 1:
            result = RunMenu(s_fusionMenu);
            if (result != 0) {
                if (result > 0) {
                    g_fusionFirstSlot = g_selectedObjectId;
                    s_selectedFusionIndex = s_fusionMenu->cursor + g_hoveredObjectId;
                } else {
                    g_fusionFirstSlot = -1;
                }
                ++step;
            } else {
                result = RunPanelInput(s_fusionPager);
                if (s_fusionPageActionPending == 1) {
                    result = s_fusionPageAction;
                    s_fusionPageActionPending = 0;
                    s_fusionPageAction = -1;
                    oldOffset = s_fusionColumnOffset;
                    if (result == 0) {
                        s_fusionColumnOffset -= s_fusionPageRows;
                        if (s_fusionColumnOffset < 0) {
                            s_fusionColumnOffset = 0;
                        }
                    } else if (result == 1) {
                        if (s_fusionColumnOffset + s_fusionPageRows < s_fusionColumnCount) {
                            s_fusionColumnOffset += s_fusionPageRows;
                        }
                    }
                    if (s_fusionColumnOffset != oldOffset) {
                        DrawFusionSummaryGrid();
                    }
                    ClearPanelChecksAgain(s_fusionPager);
                    PaintPanel(s_fusionPager, s_fusionMenu->plane);
                } else {
                    if (result >= 0) {
                        s_fusionPageActionPending = 1;
                        s_fusionPageAction = result;
                    }
                    if (result == -1) {
                        s_fusionPreviewPlane = OpenFusionPreviewOnClick();
                        if (s_fusionPreviewPlane >= 0) {
                            step = 3;
                        }
                    }
                }
            }
            break;
        case 2:
            s_fusionPager = ReleasePanel(s_fusionPager, 1);
            step = CloseFusionPicker(g_fusionFirstSlot);
            break;
        case 3:
            s_fusionPreviewPlane = CloseFusionPreviewOnClick(s_fusionPreviewPlane);
            if (s_fusionPreviewPlane < 0) {
                step = 1;
            }
            break;
    }
    return step;
}

RVA(0x00028e70, 0x60)
i16 CreateFusionInfoPlane(i16 unused) {
    i16 plane = CreateTextPlane(1, 0);
    ResetTextPlaneLineStep(plane, 3);
    ClearTextPlane(plane);
    RepaintTextPlane(plane, -2);
    DrawPlaneImage(plane, 25, 1, 0);
    DrawPlaneImage(plane, 43, 1, 1);
    DrawPlaneImage(plane, 61, 1, 8);
    return plane;
}

RVA(0x00028ed0, 0x220)
void DrawFusionSummaryGrid(void) {
    i16 column;
    i16 row;
    i16 x;
    i16 y;
    i16 kind;
    i16 frame;
    i16 growth;
    i16 overLevel;
    FusionSummary* summary;
    ClearTextPlaneRight(s_fusionMenu->plane);
    for (column = s_fusionColumnOffset; column < s_fusionColumnOffset + s_fusionPageRows;
         ++column) {
        if (column < s_fusionColumnCount) {
            sprintf(g_scratchBuffer, "%2d", column + 1);
        } else {
            sprintf(g_scratchBuffer, "  ");
        }
        DrawPlaneText(
            s_fusionMenu->plane,
            272 + (column - s_fusionColumnOffset) * 24,
            7,
            g_scratchBuffer,
            0x1400
        );
        for (row = s_fusionMenu->cursor; row < s_fusionMenu->cursor + s_fusionMenu->pageRows;
             ++row) {
            x = (column - s_fusionColumnOffset) * 3 + 34;
            y = (row - s_fusionMenu->cursor) * 16 + 24;
            if (column < s_fusionColumnCount && row < s_fusionMenu->itemCount) {
                summary = GetFusionPairSummaryCell(s_fusionSlots[row], s_fusionSlots[column]);
                kind = summary->fields.kind;
                overLevel = summary->fields.overLevel;
                growth = summary->fields.lowFlag;
                frame = 1 - summary->fields.highFlag;
                growth = growth < 0 ? 1 : (growth > 0 ? 0 : -1);
                ++kind;
                if (kind >= 0) {
                    DrawPlaneIcon(s_fusionMenu->plane, x, y, s_fusionSummaryIcons[kind], frame);
                    if (overLevel) {
                        DrawPlaneIconKeyed(s_fusionMenu->plane, x, y, 10, frame);
                    }
                    if (growth >= 0) {
                        DrawPlaneIconKeyed(s_fusionMenu->plane, x, y, growth + 15, frame);
                    }
                }
            }
        }
    }
}

RVA(0x000290f0, 0x2b)
FusionSummary* GetFusionPairSummaryCell(i16 first, i16 second) {
    if (s_fusionSummaryTable == NULL) {
        s_fusionSummaryTable = s_fusionPairSummaries;
    }
    first = first * 32 + second;
    return &s_fusionPairSummaries[first];
}

RVA(0x00029120, 0x8d)
i16 CreateFusionList(i16 window, i16 count) {
    SetPanelImage(0x11c);
    s_fusionMenu = CreateMenuBox(s_fusionMenu, window, 2);
    SetMenuItems(s_fusionMenu, 15, NULL, count, FusionListMenuHandler);
    SetTextPlaneFirstSelectableRow(s_fusionMenu->plane, 1, 0);
    s_previousFusionTextHook = SetTextPlaneHook(FusionSelectionTextHook);
    if (s_fusionPageRows == -1) {
        s_fusionPageRows = 15;
    }
    BuildMenuPage(s_fusionMenu);
    return 15;
}

RVA(0x000291b0, 0x198)
void FusionListMenuHandler(MenuBox* menu, i16 index, i16 event) {
    Character* character;
    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->itemCount = 0;
            break;
        case MENU_EVENT_BEGIN_PAGE:
            SetTextPlaneMenuOrigin(menu->plane, 0, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            g_scratchBuffer[0] = 0;
            character = GetRosterCharacter(s_fusionSlots[index]);
            if (character != NULL) {
                FormatFullName(g_fusionNameBuffer, character);
                sprintf(
                    g_scratchBuffer,
                    "%3d  %-10.10s %-16.16s",
                    index + 1,
                    GetDemonRaceName(character->id),
                    g_fusionNameBuffer
                );
            } else {
                sprintf(
                    g_scratchBuffer,
                    "%3d  %-9.9s %-16.16s",
                    index + 1,
                    g_fusionMissingRace,
                    g_fusionMissingName
                );
            }
            if (character != NULL
                && (g_fusionFirstSlot == -1 || g_fusionFirstSlot != s_fusionSlots[index])
                && (g_fusionThirdSlot == -1 || g_fusionThirdSlot != s_fusionSlots[index])
                && !IsFusionDemonRestricted(character->id)
                && (s_fusionPageRows != 1
                    || GetFusionPairSummaryCell(s_fusionSlots[index], g_fusionFirstSlot)
                               ->fields.kind
                           != -1)) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2450, s_fusionSlots[index], 0);
            } else {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2500, s_fusionSlots[index], 1);
            }
            break;
        case MENU_EVENT_BEFORE_PANEL:
            DrawFusionSummaryGrid();
            if (s_fusionPager != NULL) {
                PaintPanel(s_fusionPager, s_fusionMenu->plane);
            }
            break;
    }
}

RVA(0x00029350, 0x94)
void FusionSelectionTextHook(i16 plane, i16 event, i16 value) {
    Character* character;
    i16 index;
    if (plane == -1) {
        return;
    }
    switch (event) {
        case -1:
            break;
        case 3:
            ClearTextPlane(s_fusionInfoPlane);
            RepaintTextPlane(s_fusionInfoPlane, -2);
            break;
        case 4:
            g_scratchBuffer[0] = 0;
            index = s_fusionMenu->cursor + value - 1;
            character = GetRosterCharacter(s_fusionSlots[index]);
            if (character != NULL) {
                DrawFusionCharacterDetails(s_fusionInfoPlane, character);
                RepaintTextPlane(s_fusionInfoPlane, -2);
            }
            break;
    }
}

RVA(0x000293f0, 0x192)
void DrawFusionCharacterDetails(i16 plane, Character* character) {
    ClearTextPlane(plane);
    sprintf(g_scratchBuffer, "%4d  %4d", character->pools.hp.cur, character->pools.hp.max);
    DrawPlaneText(plane, 56, 8, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d  %3d", character->pools.mp.cur, character->pools.mp.max);
    DrawPlaneText(plane, 56, 32, g_scratchBuffer, 0x1400);
    sprintf(
        g_scratchBuffer,
        " %s   %s",
        s_fusionAlignmentALabels[GetAlignmentClassA(character) + 1],
        s_fusionAlignmentBLabels[GetAlignmentClassB(character) + 1]
    );
    DrawPlaneText(plane, 56, 56, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", character->levelBonus);
    DrawPlaneText(plane, 168, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", character->level);
    DrawPlaneText(plane, 168, 56, g_scratchBuffer, 0x1400);
    DrawFusionStatGroup(plane, 240, GetBattleStatGroup(character, 0));
    DrawFusionStatGroup(plane, 384, GetBattleStatGroup(character, 1));
    DrawFusionStatGroup(plane, 528, GetBattleStatGroup(character, 2));
}

RVA(0x00029590, 0xca)
void DrawFusionStatGroup(i16 plane, i16 x, i16* stats) {
    i16 column = x;
    sprintf(g_scratchBuffer, "%3d", stats[3]);
    DrawPlaneText(plane, column, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", stats[5]);
    DrawPlaneText(plane, column, 56, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", stats[2]);
    column += 72;
    DrawPlaneText(plane, column, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", stats[4]);
    DrawPlaneText(plane, column, 56, g_scratchBuffer, 0x1400);
}

RVA(0x00029660, 0x85)
i32 CloseFusionPicker(i16 selection) {
    if (s_secondFusionDetailPlane >= 0) {
        s_secondFusionDetailPlane = CloseTextWindow(s_secondFusionDetailPlane);
    }
    if (s_firstFusionDetailPlane >= 0) {
        s_firstFusionDetailPlane = CloseTextWindow(s_firstFusionDetailPlane);
    }
    SetTextPlaneHook(s_previousFusionTextHook);
    s_previousFusionTextHook = NULL;
    s_fusionMenu = DestroyMenuBox(s_fusionMenu);
    s_fusionInfoPlane = CloseTextWindow(s_fusionInfoPlane);
    return selection == -1 ? -2 : -1;
}

RVA(0x000296f0, 0x134)
i16 BuildPairFusionCandidates(i16 skipCalculation) {
    i16 first;
    i16 second;
    i16 demon;
    FusionSummary summary;
    s_fusionCandidateCount = 0;
    for (first = 0; first < 32; first++) {
        if (GetRosterId(first) >= 32) {
            s_fusionSlots[s_fusionCandidateCount++] = first;
        }
        for (second = 0; second < 32; second++) {
            s_pendingFusionResultId = -1;
            summary.value = -256;
            StoreFusionPairSummary(first, second, &summary);
            if (first == second) {
                summary.fields.kind = -1;
                StoreFusionPairSummary(first, second, &summary);
            } else if (GetRosterId(second) < 32) {
                summary.fields.kind = -1;
                StoreFusionPairSummary(first, second, &summary);
            } else if (!skipCalculation && CalculatePairFusion(first, second)) {
                summary = GetPairFusionSummary(first, second);
                StoreFusionPairSummary(first, second, &summary);
            }
        }
    }
    summary.value = -128;
    for (first = 0; first < 32; first++) {
        demon = GetRosterId(first);
        if (demon >= 32 && IsFusionDemonRestricted(demon)) {
            for (second = 0; second < 32; second++) {
                StoreFusionPairSummary(first, second, &summary);
                StoreFusionPairSummary(second, first, &summary);
            }
        }
    }
    return s_fusionCandidateCount;
}

RVA(0x00029830, 0x33)
void StoreFusionPairSummary(i16 first, i16 second, const FusionSummary* summary) {
    if (s_fusionSummaryTable == NULL) {
        s_fusionSummaryTable = s_fusionPairSummaries;
    }
    first = first * 32 + second;
    s_fusionPairSummaries[first] = *summary;
}

RVA(0x00029870, 0x12d)
i16 BuildTripleFusionSummaries(i16 third) {
    i16 first;
    i16 second;
    i16 demon;
    FusionSummary summary;
    summary.value = -256;
    for (first = 0; first < 32; first++) {
        for (second = 0; second < 32; second++) {
            StoreFusionPairSummary(first, second, &summary);
        }
    }
    for (first = 0; first < 32; first++) {
        if (GetRosterId(first) >= 32) {
            for (second = 0; second < 32; second++) {
                s_pendingFusionResultId = -1;
                if (GetRosterId(second) < 32) {
                    summary.fields.kind = -1;
                    StoreFusionPairSummary(first, second, &summary);
                } else if (first != second && first != g_fusionThirdSlot
                           && second != g_fusionThirdSlot
                           && CalculateTripleFusion(third, first, second) > 0) {
                    summary = GetTripleFusionSummary(third, first, second);
                    StoreFusionPairSummary(first, second, &summary);
                } else {
                    summary.value = -128;
                    StoreFusionPairSummary(first, second, &summary);
                }
            }
        }
    }
    summary.value = -128;
    for (first = 0; first < 32; first++) {
        demon = GetRosterId(first);
        if (demon >= 32 && IsFusionDemonRestricted(demon)) {
            for (second = 0; second < 32; second++) {
                StoreFusionPairSummary(first, second, &summary);
                StoreFusionPairSummary(second, first, &summary);
            }
        }
    }
    return s_fusionCandidateCount;
}

RVA(0x000299a0, 0x64)
i16 CloseFusionPreviewOnClick(i16 plane) {
    if (!g_mouseLeftClick) {
        return plane;
    }
    if (s_thirdFusionDetailPlane != -1) {
        s_thirdFusionDetailPlane = CloseTextWindow(s_thirdFusionDetailPlane);
    }
    s_secondFusionDetailPlane = CloseTextWindow(s_secondFusionDetailPlane);
    s_firstFusionDetailPlane = CloseTextWindow(s_firstFusionDetailPlane);
    return CloseTextWindow(plane);
}

RVA(0x00029a10, 0x1e0)
i16 OpenFusionPreviewOnClick(void) {
    i16 column;
    i16 row;
    i16 first;
    i16 second;
    i16 plane;
    Character* character;
    if (!g_mouseLeftClick) {
        return -1;
    }
    g_mouseLeftClick = 0;
    column = (g_mousePosition.x - 272) / 24;
    row = (g_mousePosition.y - 54) / 16;
    if (column < 0 || column >= s_fusionPageRows || row < 0 || row >= s_fusionMenu->pageRows) {
        return -1;
    }
    column += s_fusionColumnOffset;
    row += s_fusionMenu->cursor;
    if (column >= 32 || row >= 32) {
        return -1;
    }
    first = s_fusionSlots[column];
    second = s_fusionSlots[row];
    if (g_fusionThirdSlot == -1) {
        character = CreatePairFusionCharacter(first, second, 0);
        if (!character) {
            return -1;
        }
        plane = CreateTextPlane(11, 0);
        ResetTextPlaneLineStep(plane, 3);
        DrawFusionPreviewCard(plane, character);
        s_firstFusionDetailPlane = CreateFusionPreviewCard(7, second);
        s_secondFusionDetailPlane = CreateFusionPreviewCard(8, first);
        RepaintTextPlane(plane, -2);
        FreeWordList(GetCharacterSkills(character));
        FreeBlock(character);
        return plane;
    } else {
        if (!CalculateTripleFusion(g_fusionThirdSlot, first, second)) {
            return -1;
        }
        character = GetCharacter(13);
        plane = CreateTextPlane(12, 0);
        ResetTextPlaneLineStep(plane, 3);
        DrawFusionPreviewCard(plane, character);
        s_firstFusionDetailPlane = CreateFusionPreviewCard(7, g_fusionThirdSlot);
        s_secondFusionDetailPlane = CreateFusionPreviewCard(8, second);
        s_thirdFusionDetailPlane = CreateFusionPreviewCard(11, first);
        RepaintTextPlane(plane, -2);
        return plane;
    }
}

RVA(0x00029bf0, 0x19f)
void DrawFusionPreviewCard(i16 plane, Character* character) {
    ClearTextPlane(plane);
    FormatFullName(g_fusionNameBuffer, character);
    sprintf(
        g_scratchBuffer,
        "%-10.10s %-16.16s",
        GetDemonRaceName(character->id),
        g_fusionNameBuffer
    );
    DrawPlaneText(plane, 16, 8, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%4d  %4d", character->pools.hp.cur, character->pools.hp.max);
    DrawPlaneText(plane, 56, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d  %3d", character->pools.mp.cur, character->pools.mp.max);
    DrawPlaneText(plane, 56, 56, g_scratchBuffer, 0x1400);
    sprintf(
        g_scratchBuffer,
        " %s   %s",
        s_fusionAlignmentALabels[GetAlignmentClassA(character) + 1],
        s_fusionAlignmentBLabels[GetAlignmentClassB(character) + 1]
    );
    DrawPlaneText(plane, 184, 56, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", character->levelBonus);
    DrawPlaneText(plane, 256, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", character->level);
    DrawPlaneText(plane, 184, 32, g_scratchBuffer, 0x1400);
}

RVA(0x00029d90, 0x44)
i16 CreateFusionPreviewCard(i16 window, i16 slot) {
    i16 plane = CreateTextPlane(window, 0);
    ResetTextPlaneLineStep(plane, 3);
    DrawFusionPreviewCard(plane, GetRosterCharacter(slot));
    RepaintTextPlane(plane, -2);
    return plane;
}

RVA(0x00029de0, 0x11f)
i16 RunSecondFusionPicker(i16 step) {
    i16 count;
    i16 result;
    switch (step) {
        case 0:
            count = s_fusionCandidateCount;
            g_fusionSecondSlot = -1;
            s_fusionInfoPlane = CreateFusionInfoPlane(6);
            s_fusionPageRows = 1;
            CreateFusionList(4, count);
            s_fusionColumnCount = s_selectedFusionIndex + 1;
            s_fusionColumnOffset = s_selectedFusionIndex;
            PaintMenuBox(s_fusionMenu);
            if (g_fusionThirdSlot == -1) {
                s_firstFusionDetailPlane = CreateFusionPreviewCard(7, g_fusionFirstSlot);
            } else {
                s_firstFusionDetailPlane = CreateFusionPreviewCard(7, g_fusionThirdSlot);
                s_secondFusionDetailPlane = CreateFusionPreviewCard(8, g_fusionFirstSlot);
            }
            ++step;
            break;
        case 1:
            result = RunMenu(s_fusionMenu);
            if (result != 0) {
                if (result > 0) {
                    g_fusionSecondSlot = g_selectedObjectId;
                } else {
                    g_fusionSecondSlot = -1;
                }
                ++step;
            }
            break;
        case 2:
            step = CloseFusionPicker(g_fusionSecondSlot);
            break;
    }
    return step;
}

RVA(0x00029f00, 0xd4)
i16 RunThirdFusionPicker(i16 step) {
    i16 count;
    i16 result;
    switch (step) {
        case 0:
            g_fusionThirdSlot = -1;
            g_fusionFirstSlot = -1;
            g_fusionSecondSlot = -1;
            CountRosterEntries(1);
            s_fusionInfoPlane = CreateFusionInfoPlane(6);
            count = BuildPairFusionCandidates(1);
            s_fusionPageRows = 0;
            CreateFusionList(10, count);
            s_fusionColumnOffset = 0;
            PaintMenuBox(s_fusionMenu);
            ++step;
            break;
        case 1:
            result = RunMenu(s_fusionMenu);
            if (result != 0) {
                if (result > 0) {
                    g_fusionThirdSlot = g_selectedObjectId;
                } else {
                    g_fusionThirdSlot = -1;
                }
                ++step;
            }
            break;
        case 2:
            step = CloseFusionPicker(g_fusionThirdSlot);
            break;
    }
    return step;
}

RVA(0x00029fe0, 0x7e)
i16 PreviewFusionCharacter(Character* character) {
    Character* previous = SetRosterEntry(0, character);
    RaiseExperienceToLevel(character);
    AllocScreenSave(g_fusionPreviewSave);
    CaptureScreenSaveWithState(g_fusionPreviewSave);
    g_fusionPaletteState = SavePaletteState(g_fusionPaletteState, 2);
    EnterStatusScreen(1);
    DrawStatusScreen(0);
    SetRosterEntry(0, previous);
    return 0;
}

RVA(0x0002a060, 0x49)
i16 CloseFusionPreview(void) {
    LeaveStatusScreen(1);
    RestoreDrawState(SaveDrawState());
    g_fusionPaletteState = RestorePaletteState(g_fusionPaletteState, 1);
    RestoreScreenSave(g_fusionPreviewSave);
    FreeScreenSave(g_fusionPreviewSave);
    return 0;
}

RVA(0x0002a0b0, 0x16)
Character* LoadFusionResultCharacter(Character* destination) {
    return LoadCharacterCore(s_fusionResultId, destination);
}

RVA(0x0002a0d0, 0x78)
void RunPairFusion(void) {
    Character* result;
    LoadFusionTables();
    s_pendingFusionResultId = -1;
    result = CreatePairFusionCharacter(g_fusionFirstSlot, g_fusionSecondSlot, 0);
    if (result != NULL) {
        s_pendingFusionResultId = result->id;
        s_cachedFusionSummary = GetPairFusionSummary(g_fusionFirstSlot, g_fusionSecondSlot);
        PreviewFusionCharacter(result);
        FreeWordList(GetCharacterSkills(result));
        FreeBlock(result);
    }
}

RVA(0x0002a150, 0x81)
void RunTripleFusion(void) {
    Character* result;
    i16 id;
    LoadFusionTables();
    s_pendingFusionResultId = -1;
    CalculateTripleFusion(g_fusionThirdSlot, g_fusionFirstSlot, g_fusionSecondSlot);
    s_cachedFusionSummary =
        GetTripleFusionSummary(g_fusionThirdSlot, g_fusionFirstSlot, g_fusionSecondSlot);
    s_fusionSummary = s_cachedFusionSummary;
    result = GetCharacter(13);
    PreviewFusionCharacter(result);
    id = result->id;
    s_pendingFusionResultId = id;
    s_fusionResultId = id;
}

RVA(0x0002a1e0, 0xa)
void EndFusion(void) {
    CloseFusionPreview();
    FreeFusionTables();
}

RVA(0x0002a1f0, 0x52)
i16 CommitPairFusion(void) {
    Character* result = CreatePairFusionCharacter(g_fusionFirstSlot, g_fusionSecondSlot, 0);
    if (result == NULL) {
        return 0;
    }
    RemoveFromRoster(g_fusionFirstSlot);
    RemoveFromRoster(g_fusionSecondSlot);
    AddToRoster(result);
    return result->id;
}

RVA(0x0002a250, 0xd6)
i16 CommitTripleFusion(void) {
    Character* result;
    Character* temporary;
    i16 id = s_pendingFusionResultId;
    s_pendingFusionResultId = -1;
    if (id > 1) {
        s_fusionResultId = id;
        s_fusionSummary = s_cachedFusionSummary;
    } else {
        CalculateTripleFusion(g_fusionThirdSlot, g_fusionFirstSlot, g_fusionSecondSlot);
    }
    RemoveFromRoster(g_fusionThirdSlot);
    RemoveFromRoster(g_fusionFirstSlot);
    RemoveFromRoster(g_fusionSecondSlot);
    result = AllocCleared(1, sizeof(Character));
    memcpy(result, GetCharacter(13), sizeof(Character));
    id = result->id;
    s_fusionResultId = id;
    AddToRoster(result);
    temporary = GetCharacter(13);
    InitWordList(GetCharacterSkills(temporary), 0);
    return id;
}

RVA(0x0002a330, 0x50)
i16 StageFusionCharacter(i16 id) {
    Character* character = GetCharacter(13);
    FreeWordList(GetCharacterSkills(character));
    if (id >= 32) {
        character = LoadCharacterCore(id, character);
    }
    if (s_savedFusionCharacter == NULL) {
        s_savedFusionCharacter = SetRosterEntry(0, character);
    }
    return 0;
}

RVA(0x0002a380, 0xb3)
i16 StagePairFusionCharacter(i16 first, i16 second, i16 rankChanges) {
    i16 result = ResolvePairFusion(first, second, rankChanges);
    Character* temporary = GetCharacter(13);
    Character* fusion;
    FreeWordList(GetCharacterSkills(temporary));
    fusion = CreatePairFusionCharacter(first, second, rankChanges);
    if (fusion != NULL) {
        memcpy(temporary, fusion, sizeof(Character));
        InitWordList(GetCharacterSkills(fusion), 0);
        FreeBlock(fusion);
    }
    if (s_savedFusionCharacter == NULL) {
        s_savedFusionCharacter = SetRosterEntry(0, temporary);
        if (s_savedFusionCharacter != GetCharacter(0)) {
            return -1;
        }
    }
    return result;
}

RVA(0x0002a440, 0x35)
i32 RestoreFusionCharacter(void) {
    if (s_savedFusionCharacter != NULL) {
        if (s_savedFusionCharacter != GetCharacter(0)) {
            return 0;
        }
        SetRosterEntry(0, s_savedFusionCharacter);
    }
    s_savedFusionCharacter = NULL;
    return 0;
}
