#ifndef GITEN_GAME_GAMELOOP_H
#define GITEN_GAME_GAMELOOP_H

#include <rva.h>

#include <Ints.h>

// @identity-TODO: a mask the start-up path sets to 0xc000; tested with 1 by
// one mode handler (TestFeatureMask). What its bits enable is not recovered.
extern i16 g_featureMask;

b16 InitGameData(void);
void ResetGameSession(void);
b16 StartGame(void);

void ClearScriptVars(void);
i16 SetLongFrame(i16 longFrame);

// Nonzero once the game should end; StepGame returns it. The state
// dispatcher's -1 and the system menu's quit confirmation set it.
extern i16 g_quitRequest;

b16 TickGameTasks(void);
i16 StepGame(void);

#endif // GITEN_GAME_GAMELOOP_H
