// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. Kept apart from charpool:
// in that unit these two change the codegen of its later functions.

#include <rva.h>

#include <Game/Character.h>
#include <Game/StatUpdate.h>
#include <Util/BitSet.h>

#include <math.h>

// Maximum HP: level * vitality / 2 + protection and fortune + 5, doubled by personal
// flag 37 and again by flag 38, at most 9999.
// @early-stop register residue: retail loads stat 10 into ecx and stat 4 into
// edx for the integer sum; cl swaps them. Operand orders are canonicalised
// and the permuter is flat.
RVA(0x0003cec0, 0xa3)
i32 CalcMaxHp(Character* character) {
    double value = character->level;
    value *= character->stats.total[6];
    value *= 0.5;
    value += character->stats.total[4] + character->stats.total[10] + 5;
    if (TestBit(character->personalFlags, 37) == 1) {
        value += value;
    }
    if (TestBit(character->personalFlags, 38) == 1) {
        value += value;
    }
    if (value > 9999.0) {
        value = 9999.0;
    }
    return (i32)value;
}

// Maximum MP: (charm and mental strength) / 2 + sqrt(level) * magic * 1.5, doubled by
// personal flag 38, at most 999.
// @early-stop register residue: the integer base sum loads stats 1 and 9
// into ecx/edx where retail loads 9 and 1. Staged integer sums are flat,
// and a 32-island campaign found one compiler state.
RVA(0x0003cf70, 0x8c)
i32 CalcMaxMp(Character* character) {
    double scaled = sqrt(character->level);
    double value = GetStatTotal(character, 9) + GetStatTotal(character, 1);
    value *= 0.5;
    scaled *= GetStatTotal(character, 2);
    scaled *= 1.5;
    value += scaled;
    if (TestCharacterFlag(character, 38) == 1) {
        value += value;
    }
    if (value > 999.0) {
        value = 999.0;
    }
    return (i32)value;
}
