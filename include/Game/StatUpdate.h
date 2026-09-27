#ifndef GITEN_GAME_STATUPDATE_H
#define GITEN_GAME_STATUPDATE_H

#include <Game/Character.h>

// Stat-block helpers the roster refresh calls. Codegen constraint: kept out
// of <Game/Stats.h> and <Game/CharInfo.h>, whose declaration counts charpool's
// StatAverage functions depend on.
i16 ClearStatModifiers(StatBlock* stats);
void UpdateStatTotals(StatBlock* stats);
void ApplyStatFlags(Character* character);
i32 CalcMaxHp(Character* character);
i32 CalcMaxMp(Character* character);

#endif // GITEN_GAME_STATUPDATE_H
