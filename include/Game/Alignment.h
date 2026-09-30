#ifndef GITEN_GAME_ALIGNMENT_H
#define GITEN_GAME_ALIGNMENT_H

#include <rva.h>

#include <Game/AlignmentSide.h>
#include <Ints.h>

// A character alignment value (-127..127), the last eight sides (-1/0/1)
// that moved it, newest first, and a count of moves per side.
typedef struct AlignmentInfo {
    i16 value;
    i16 last0 : 2;
    i16 last1 : 2;
    i16 last2 : 2;
    i16 last3 : 2;
    i16 last4 : 2;
    i16 last5 : 2;
    i16 last6 : 2;
    i16 last7 : 2;
    i32 negativeMoves;
    i32 neutralMoves;
    i32 positiveMoves;
} AlignmentInfo;

#define GetAlignmentValue(info) ((info)->value)

#define InitAlignmentInfo(info)                                                                    \
    do {                                                                                           \
        (info)->value = 0;                                                                         \
        (info)->last0 = 0;                                                                         \
        (info)->last1 = 0;                                                                         \
        (info)->negativeMoves = 0;                                                                 \
        (info)->positiveMoves = 0;                                                                 \
    } while (0)

GZ_ENUM_RETURN(AlignmentSide, i16)
MoveAlignment(AlignmentInfo* info, i16 weight, GZ_ENUM_PARAM(AlignmentSide, i16) side);
i16 GetAlignmentAffinity(AlignmentInfo* info, GZ_ENUM_PARAM(AlignmentSide, i16) side);
GZ_ENUM_RETURN(AlignmentSide, i16) AlignmentClass(i16 value);
i16 AlignmentChartCell(i16 value);

struct Character;

// Moves alignment B by `amount` towards `step`'s side and copies its value
// to `alignmentLevelB`.
void ShiftAlignmentB(
    struct Character* character,
    i16 amount,
    GZ_ENUM_PARAM(AlignmentSide, i16) step
);

// The same for alignment A and `alignmentLevelA`.
void ShiftAlignmentA(
    struct Character* character,
    i16 amount,
    GZ_ENUM_PARAM(AlignmentSide, i16) step
);

// Nonzero (-1) when `character`'s alignment classes conflict with the roster
// leader's (opposite sides on A, or opposite nonzero classes on B); also for
// no character.
i16 AlignmentConflicts(struct Character* character);

#endif // GITEN_GAME_ALIGNMENT_H
