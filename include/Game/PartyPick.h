#ifndef GITEN_GAME_PARTYPICK_H
#define GITEN_GAME_PARTYPICK_H

#include <EnumDomain.h>
#include <Game/PartyCommand.h>
#include <Ints.h>

// The party position saved when the command's target is confirmed; summoning
// exchanges the summoned demon into that position.
extern i16 g_commandPosition;

// @identity-TODO: party picking during field play. A pick mode (0/1 while a
// member is being picked), a done flag the menus set, and one temporary party
// swap (a position and the slot it held) that is undone afterwards.
void CloseFieldWindows(void);
i16 CheckPickTarget(i16 index);
void MarkPickDone(void);
GZ_ENUM_RETURN(PartyCommandPhase, i16) GetPickMode(void);
i16 RunMemberPickMenu(i16 id);
void SetMemberPickRole(i16 id, i8 role);
i16 GetMemberPickRange(i16 id);
GZ_ENUM_RETURN(PartyCommandPhase, i16) QueryPickMode(void);
i16 GetTickElapsed(void);
i16 SwapInForPick(i16 keep, i16 slot);
void RestoreSwappedMember(void);
i16 GetSwappedMember(i16 index);

#endif // GITEN_GAME_PARTYPICK_H
