#ifndef GITEN_GAME_STATUSDRAW_H
#define GITEN_GAME_STATUSDRAW_H

#include <rva.h>

#include <Game/Character.h>
#include <Ints.h>

i16 LockStatusRedraw(i16 lock);
void RequestStatusRedraw(void);
void DrawPartyStatusSlot(i16 slot, CharacterCore* character);

void FlushStatusRedraw(i16 force);

#endif // GITEN_GAME_STATUSDRAW_H
