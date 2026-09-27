#ifndef GITEN_GAME_ANALYZEDATA_H
#define GITEN_GAME_ANALYZEDATA_H

#include <rva.h>

#include <Ints.h>

// The saved per-id analyze records, which live with the data-file readers
// rather than with the analyze window.
// @identity-TODO: what sets an id's bit (0x478878) is unrecovered.

i16 HasAnalyzeData(i16 id);

// @identity-TODO: the saved per-id byte count (0x47add8, set clamped to
// 0..255 by 0x410ba0) whose eighth becomes Character.familiarity; what it
// counts is unrecovered.
i16 GetFamiliarityCount(i16 id);

#endif // GITEN_GAME_ANALYZEDATA_H
