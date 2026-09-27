#ifndef GITEN_GAME_DDSMENU_H
#define GITEN_GAME_DDSMENU_H

#include <Ints.h>
#include <Ui/MenuBox.h>

i16 RunDdsMenu(void);
i16 RunDdsSummon(void);
i16 PickDdsSummon(void);
i16 PickDdsRosterMember(i16 step);
void DdsMenuHandler(MenuBox* menu, i16 index, i16 event);
i16 ReturnDdsMember(void);
i16 PickDdsPurgeMember(void);

#endif // GITEN_GAME_DDSMENU_H
