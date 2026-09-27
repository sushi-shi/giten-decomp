#ifndef GITEN_GAME_PLAYTIME_H
#define GITEN_GAME_PLAYTIME_H

#include <Ints.h>

#include <stdio.h>

void ResetPlayTime(void);
void AdvancePlayTime(i16 ticks);
i16 SavePlayTime(FILE* fp);
i16 LoadPlayTime(FILE* fp);

#endif // GITEN_GAME_PLAYTIME_H
