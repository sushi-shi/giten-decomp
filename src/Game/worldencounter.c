// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Actor.h>
#include <Game/Clock.h>
#include <Game/FieldActor.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldView.h>
#include <Game/Party.h>
#include <Game/PartyCommand.h>
#include <Game/WorldMap.h>
#include <Input/Mouse.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

DATA(0x0007b0c0)
static i32 s_encounterChoices;

DATA(0x0007b0c4)
static i32 s_encounterWeights;

DATA(0x0007b0c8)
static i32 s_encounterBlock;

DATA(0x0007b0cc)
static i32 s_fieldTable;

DATA(0x0007b0bc)
static i16 s_encounterChanceBonus;

DATA(0x0007b0e8)
static u32 s_lastEncounterMinute;

DATA(0x000788b8)
static i16 s_encounterGroups[2];

DATA(0x000788c0)
u8 g_worldEncounterGroupSlots[16] = {0};

DATA(0x0007b0a8)
static i16 s_encounterCount;

DATA(0x0007b0ac)
static i16 s_fieldTableIndex;

DATA(0x00068610)
static i16 s_encounterSpread[4] = {0, 0, 1, 2};

RVA(0x00010f70, 0x4f)
void FreeEncounterTables(void) {
    s_fieldTable = FreeHandle(s_fieldTable);
    s_encounterBlock = FreeHandle(s_encounterBlock);
    s_encounterWeights = FreeHandle(s_encounterWeights);
    s_encounterChoices = FreeHandle(s_encounterChoices);
}

RVA(0x00010fc0, 0x67)
void LoadEncounterTables(void) {
    FILE* fp;
    FreeEncounterTables();
    fp = OpenDataFile(0x21, 12, 0);
    s_encounterChoices = ReadRawHandle(fp);
    s_encounterWeights = ReadRawHandle(fp);
    CloseDataFile(fp);
    fp = OpenDataFile(0x10ff, 12, 0);
    s_fieldTable = ReadRawHandle(fp);
    CloseDataFile(fp);
}

RVA(0x00011030, 0xfe)
i16 RollWorldMapEncounter(i16 x, i16 y) {
    i16 cell;
    i16 variant;
    WorldEncounterCell* cells;
    Character* leader = GetRosterCharacter(0);
    if (TestCharacterFlag(leader, 0x22) == 1) {
        return -1;
    }
    if (CheckWorldEncounterInterval() < 1) {
        return -1;
    }
    variant = 0;
    cell = LoadWorldEncounterBlock(x, y);
    cells = HandleReadPtr(s_encounterBlock);
    if (!CheckFlagWord(&GetWorldEncounterVariant(&cells[cell], variant)->condition)) {
        variant = 1;
        if (!CheckFlagWord(&GetWorldEncounterVariant(&cells[cell], variant)->condition)) {
            return 0;
        }
    }
    s_fieldTableIndex = cells[cell].fieldTable;
    if (!TestWorldEncounterChance(GetWorldEncounterVariant(&cells[cell], variant)->chance)) {
        return 0;
    }
    s_encounterCount = PrepareWorldEncounter(
        GetWorldEncounterVariant(&cells[cell], variant)->weights,
        GetWorldEncounterVariant(&cells[cell], variant)->choices,
        GetWorldEncounterVariant(&cells[cell], variant)->maximum
    );
    return s_encounterCount;
}

RVA(0x00011130, 0x38)
i16 CheckWorldEncounterInterval(void) {
    u32 next;
    u32 now;
    if (!(g_mousePosition.buttons & MOUSE_RIGHT_DOWN)) {
        next = s_lastEncounterMinute + 120;
    } else {
        next = GetClockMinutes() + 120;
    }
    now = GetClockMinutes();
    if (now < next) {
        return -1;
    }
    s_lastEncounterMinute = now;
    return 1;
}

RVA(0x00011170, 0xab)
i16 LoadWorldEncounterBlock(i16 x, i16 y) {
    i16 block = GetWorldMapBlock(x, y);
    FILE* fp;
    s_encounterBlock = FreeHandle(s_encounterBlock);
    fp = OpenDataFile(block + 0x1000, 12, 0);
    s_encounterBlock = ReadRawHandle(fp);
    CloseDataFile(fp);
    if (IsOddMapLayer()) {
        return 45;
    }
    x = GetWorldBlockX(x);
    y = GetWorldBlockY(y);
    x /= 32;
    y /= 40;
    x += y * 9;
    return x;
}

RVA(0x00011220, 0x53)
b16 TestWorldEncounterChance(i16 chance) {
    i16 totalChance = chance + s_encounterChanceBonus;
    if (totalChance < RandomAverage(1, 100, 2)) {
        s_encounterChanceBonus++;
        if (RosterContainsId(4)) {
            s_encounterChanceBonus++;
        }
        return false;
    }
    s_encounterChanceBonus = 0;
    return true;
}

RVA(0x00011280, 0x77)
i16 PrepareWorldEncounter(i16 weights, i16 choices, i16 maximum) {
    i16 count;
    i16 i;
    maximum = GetWorldEncounterMaximum(maximum);
    if (maximum == 0) {
        return 0;
    }
    count = RandomAverage(1, maximum, 1);
    s_encounterGroups[0] = s_encounterGroups[1] = PickWorldEncounterGroup(weights, choices);
    for (i = 0; i < 10 && s_encounterGroups[1] == s_encounterGroups[0]; i++) {
        s_encounterGroups[1] = PickWorldEncounterGroup(weights, choices);
    }
    AssignWorldEncounterGroups(count);
    return count;
}

RVA(0x00011300, 0x2f)
i16 GetWorldEncounterMaximum(i16 maximum) {
    if (RosterContainsId(4)) {
        maximum += 2;
    }
    maximum += GetPartyEncounterSizeBonus();
    if (maximum >= 16) {
        maximum = 16;
    }
    return maximum;
}

RVA(0x00011330, 0x6f)
i16 GetPartyEncounterSizeBonus(void) {
    i16 total = 0;
    i16 count = 0;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character != NULL && IsHumanCharacter(character)) {
            count++;
            total += GetStatTotal(character, STAT_FORTUNE);
        }
    }
    if (count < 1) {
        return 0;
    }
    total = 50 - total / count;
    total /= 10;
    if (total < 1) {
        total = 1;
    }
    return total;
}

RVA(0x000113a0, 0x9f)
i16 PickWorldEncounterGroup(i16 weights, i16 choices) {
    WorldEncounterWeights* table = HandleReadPtr(s_encounterWeights);
    WorldEncounterChoices* groups;
    i16 roll = RandomAverage(1, 100, 0);
    i16 total;
    i16 i;
    if (RosterContainsId(4)) {
        roll += 10;
    }
    total = 0;
    for (i = 0; i < 6; i++) {
        total += table[weights].weights[i];
        if (roll < total) {
            groups = HandleReadPtr(s_encounterChoices);
            return groups[choices].groups[i];
        }
    }
    groups = HandleReadPtr(s_encounterChoices);
    return groups[choices + 1].groups[0];
}

RVA(0x00011440, 0x37)
i16 AssignWorldEncounterGroups(i16 count) {
    i16 i;
    for (i = 0; i < count; i++) {
        if (RandomAverage(1, 100, 0) < 40) {
            g_worldEncounterGroupSlots[i] = 1;
        } else {
            g_worldEncounterGroupSlots[i] = 0;
        }
    }
    return count;
}

RVA(0x00011480, 0x89)
void LoadFieldTable(void) {
    FILE* fp = NULL;
    EncounterFieldImage* images;
    if (s_fieldTable == 0) {
        fp = OpenDataFile(0x10ff, 12, 0);
        s_fieldTable = ReadRawHandle(fp);
        CloseDataFile(fp);
    }
    images = HandleReadPtr(s_fieldTable);
    LoadFieldImage(
        images[s_fieldTableIndex].image + 0x5000,
        images[s_fieldTableIndex].variant,
        images[s_fieldTableIndex].option
    );
    if (fp != NULL) {
        s_fieldTable = FreeHandle(s_fieldTable);
    }
}

RVA(0x00011510, 0x102)
void PrepareFieldRandom(void) {
    i16 i;
    i16 x;
    i16 y;
    i16 along;
    i16 across;
    i16 spread;
    Character* actor;
    for (i = 0; i < s_encounterCount; i++) {
        x = g_field.pos.x;
        y = g_field.pos.y;
        along = RandomAverage(-3, 0, 0);
        spread = s_encounterSpread[-along];
        across = RandomAverage(-spread, spread, 0);
        OffsetMapCoord(&x, &y, g_field.pos.direction, across, along);
        SpawnFieldObject(
            g_worldEncounterGroupSlots[i],
            x,
            y,
            OppositeDirection(g_field.pos.direction),
            s_encounterGroups[g_worldEncounterGroupSlots[i]],
            0,
            -1,
            0
        );
        actor = GetFieldActor(i);
        AlertActor(actor, 2);
    }
    LoadEnemyGroupSlot(0, s_encounterGroups[0]);
    LoadEnemyGroupSlot(1, s_encounterGroups[1]);
}

RVA(0x00011620, 0x37)
b32 AnyObjectInReach(void) {
    b32 found = false;
    i16 i;
    i16 object;
    for (i = 0; i < 16; i++) {
        object = GetLiveObject(i);
        if (object >= 0 && HasObjectInReach(1, -1, object)) {
            found = true;
        }
    }
    return found;
}
