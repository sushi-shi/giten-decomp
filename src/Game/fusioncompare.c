// @identity-TODO: the owning TU is unproven; this unit holds the contiguous
// fusion comparison span until link-order evidence names its owner.

#include <rva.h>

#include <Game/DemonTable.h>
#include <Game/Fusion.h>
#include <Game/FusionCompare.h>
#include <Game/GameState.h>
#include <Game/Party.h>

RVA(0x0002a480, 0x130)
i16 CompareFusionCharacters(Character* first, Character* second) {
    i16 result;
    i16 firstStat;
    i16 secondStat;
    i16 index;
    result = CompareFusionValues(first->level, second->level);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(GetDemonLevel(first->id), GetDemonLevel(second->id));
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->pools.hp.max, second->pools.hp.max);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->pools.hp.max, second->pools.hp.max);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->pools.mp.max, second->pools.mp.max);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->pools.mp.max, second->pools.mp.max);
    if (result != -1) {
        return result;
    }
    for (index = 0; index < 11; index++) {
        firstStat = GetBaseStat(first, index);
        secondStat = GetBaseStat(second, index);
    }
    result = CompareFusionValues(firstStat, secondStat);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->id, second->id);
    if (result != -1) {
        return 1 - result;
    }
    return 0;
}

RVA(0x0002a5b0, 0x20)
i16 CompareFusionValues(i16 first, i16 second) {
    if (first == second) {
        return -1;
    }
    return first <= second;
}

RVA(0x0002a5d0, 0x44)
i16 SortFusionSlots(i16* first, i16* second) {
    Character* firstCharacter = GetRosterCharacter(*first);
    Character* secondCharacter = GetRosterCharacter(*second);
    i16 result = CompareFusionCharacters(firstCharacter, secondCharacter);
    if (result == 0) {
        SwapFusionSlotValues(first, second);
    }
    return result;
}

RVA(0x0002a620, 0x4b)
b16 MoveSpecialFusionSlot(i16* first, i16* second) {
    if (GetFusionSpecialRace(*first) || FindFusionFallbackIndex(GetRosterId(*first)) >= 0) {
        SwapFusionSlotValues(first, second);
        return true;
    }
    return false;
}

RVA(0x0002a670, 0x2a)
void MoveSpecialFusionCharacters(
    Character** firstCharacter,
    Character** secondCharacter,
    i16* first,
    i16* second
) {
    if (MoveSpecialFusionSlot(first, second)) {
        Character* temporary = *firstCharacter;
        *firstCharacter = *secondCharacter;
        *secondCharacter = temporary;
    }
}

RVA(0x0002a6a0, 0x31)
b16 MoveSpecialRaceFusionSlot(i16* first, i16* second) {
    if (GetFusionSpecialRace(*first)) {
        SwapFusionSlotValues(first, second);
        return true;
    }
    return false;
}

RVA(0x0002a6e0, 0x3b)
b16 MoveUnrankedFusionSlot(i16* first, i16* second) {
    if (GetDemonFlagLow(GetRosterId(*first)) == -1) {
        SwapFusionSlotValues(first, second);
        return true;
    }
    return false;
}

RVA(0x0002a720, 0x36)
i16 CompareRosterFusionClasses(i16 first, i16 second) {
    i16 cls = GetDemonClass(GetRosterId(first));
    cls -= GetDemonClass(GetRosterId(second));
    return cls;
}
