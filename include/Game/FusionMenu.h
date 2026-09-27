#ifndef GITEN_GAME_FUSIONMENU_H
#define GITEN_GAME_FUSIONMENU_H

#include <Ints.h>

i16 RunFusionMenuState(void);
void PushFusionMenu(i16 kind, i16 resultVariable);
void FinishFusionMenuSelection(i16 status, i16 selection);

#endif // GITEN_GAME_FUSIONMENU_H
