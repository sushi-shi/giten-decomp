#ifndef GITEN_GAME_EQUIPMAGICDEFENSE_H
#define GITEN_GAME_EQUIPMAGICDEFENSE_H

#include <Game/Character.h>
#include <Ints.h>

// itemrecord's sum of GetItemMagicDefenseBonus over a character's slots (bit 0 of `groups`
// takes slots 0..4, bit 1 slots 5..7), as charpool's RecalcDerivedStats sees it.
// Codegen constraint: declared returning i32 although itemrecord.c defines it
// returning i16 (see docs/todos/rule-exceptions.tsv): RecalcDerivedStats
// passes the result to a 32-bit parameter with no sign extension.
// Declared apart from <Game/Stats.h>: there it shifts charpool's
// CalcMagicAccuracyStat.
i32 SumEquippedMagicDefenseBonus(CharacterCore* character, u8 groups);

#endif // GITEN_GAME_EQUIPMAGICDEFENSE_H
