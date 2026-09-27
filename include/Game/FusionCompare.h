#ifndef GITEN_GAME_FUSIONCOMPARE_H
#define GITEN_GAME_FUSIONCOMPARE_H

#include <Game/Character.h>
#include <Ints.h>

static __inline void SwapFusionSlotValues(i16* first, i16* second) {
    i16 temporary = *first;
    *first = *second;
    *second = temporary;
}

i16 SortFusionSlots(i16* first, i16* second);
i16 MoveSpecialFusionSlot(i16* first, i16* second);
void MoveSpecialFusionCharacters(
    Character** firstCharacter,
    Character** secondCharacter,
    i16* first,
    i16* second
);
i16 MoveSpecialRaceFusionSlot(i16* first, i16* second);
i16 MoveUnrankedFusionSlot(i16* first, i16* second);
i16 CompareFusionValues(i16 first, i16 second);
i16 CompareFusionCharacters(Character* first, Character* second);
i16 CompareRosterFusionClasses(i16 first, i16 second);

#endif // GITEN_GAME_FUSIONCOMPARE_H
