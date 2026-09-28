#ifndef GITEN_GAME_FAMILIARITY_H
#define GITEN_GAME_FAMILIARITY_H

#include <Game/Character.h>
#include <Ints.h>

#include <stdio.h>

// The saved per-id counts that become a demon's familiarity, and the
// familiarity / level-gap bytes derived from them.

void AddFamiliarityCount(i16 id, i16 delta);
void SetLevelGap(Character* character, i16 gap);
void AddLevelGap(Character* character, i16 delta);
void SetFamiliarity(Character* character, i16 familiarity);
void AddFamiliarity(Character* character, i16 delta);
void SetAnalyzed(i16 id, i16 on);
i32 RollCharacterMagnetite(Character* character);
i32 RollCharacterMacca(Character* character);

// The save-file sections of the familiarity counts and the analyzed flags.
i16 LoadFamiliarityCounts(FILE* fp);
i16 SaveFamiliarityCounts(FILE* fp);
i16 LoadAnalyzed(FILE* fp);
i16 SaveAnalyzed(FILE* fp);

b16 ClearAnalyzed(void);

#endif // GITEN_GAME_FAMILIARITY_H
