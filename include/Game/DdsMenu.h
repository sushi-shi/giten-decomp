#ifndef GITEN_GAME_DDSMENU_H
#define GITEN_GAME_DDSMENU_H

#include <EnumDomain.h>
#include <Ints.h>
#include <Ui/MenuBox.h>

b16 RunDdsMenu(void);
b16 RunDdsSummon(void);
i16 PickDdsSummon(void);
i16 PickDdsRosterMember(i16 step);
void DdsMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);
i16 ReturnDdsMember(void);
i16 PickDdsPurgeMember(void);

#endif // GITEN_GAME_DDSMENU_H
