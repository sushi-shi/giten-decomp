// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/GameState.h>
#include <Util/Range.h>

#include <stddef.h>
#include <stdlib.h>

DATA(0x00069ee0)
static i16 s_affinity[5][3] = {
    {3, 0, 0},
    {2, 1, 0},
    {1, 0, 1},
    {0, 1, 2},
    {0, 0, 3},
};

RVA(0x0003df20, 0x50)
i16 GetAlignmentAffinity(AlignmentInfo* info, i16 side) {
    i16 sum = info->last1 + info->last0;
    if (sum == 0 && info->last0 == 0 && side == 0) {
        return 2;
    }
    sum += 2;
    side++;
    return s_affinity[sum][side];
}

// Moves the alignment by the side's affinity times `weight` (a neutral move
// pulls it toward 0 by at most its size) and records the side; returns the
// new alignment class (or, for an unknown side, the unchanged one).
// @early-stop register residue: retail inserts `last0 = side` through the
// side's own register (xor bl,cl; and ebx,3) where this build uses eax;
// everything before the final insert matches.
RVA(0x0003df70, 0x110)
i16 MoveAlignment(AlignmentInfo* info, i16 weight, i16 side) {
    i16 delta = GetAlignmentAffinity(info, side) * weight;
    switch (side) {
        case -1:
            info->negativeMoves++;
            delta = -delta;
            break;
        case 0:
            info->neutralMoves++;
            if (delta > abs(info->value)) {
                delta = abs(info->value);
            }
            if (info->value > 0) {
                delta = -delta;
            }
            break;
        case 1:
            info->positiveMoves++;
            break;
        default:
            return AlignmentClass(info->value);
    }
    info->value += delta;
    info->value = ClampShort(info->value, -127, 127);
    info->last7 = info->last6;
    info->last6 = info->last5;
    info->last5 = info->last4;
    info->last4 = info->last3;
    info->last3 = info->last2;
    info->last2 = info->last1;
    info->last1 = info->last0;
    info->last0 = side;
    return AlignmentClass(info->value);
}

// -1 at or below -42, 1 at or above 42, 0 between.
RVA(0x0003e080, 0x1d)
i16 AlignmentClass(i16 value) {
    if (value <= -42) {
        return -1;
    }
    return value >= 42;
}

RVA(0x0003e0a0, 0x27)
void ShiftAlignmentB(Character* character, i16 amount, i16 step) {
    AlignmentInfo* alignment = &character->alignmentB;
    MoveAlignment(alignment, amount, step);
    character->alignmentLevelB = GetAlignmentValue(alignment);
}

RVA(0x0003e0d0, 0x27)
void ShiftAlignmentA(Character* character, i16 amount, i16 step) {
    AlignmentInfo* alignment = &character->alignmentA;
    MoveAlignment(alignment, amount, step);
    character->alignmentLevelA = GetAlignmentValue(alignment);
}

// The alignment class of the party's average alignment on one axis (0 reads
// `alignmentB`, any other `alignmentA`); 0 with nobody in the party.
// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x0003e100, 0x60)
i16 PartyAlignmentClass(i16 axis) {
    i32 sum = 0;
    i16 count = 0;
    i16 i;
    Character* member;
    for (i = 0; i < 6; i++) {
        member = GetPartyEntry(i);
        if (member != NULL) {
            if (axis == 0) {
                sum += GetAlignmentValue(&member->alignmentB);
            } else {
                sum += GetAlignmentValue(&member->alignmentA);
            }
            count++;
        }
    }
    if (count != 0) {
        return AlignmentClass(sum / count);
    }
    return 0;
}
