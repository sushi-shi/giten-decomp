#ifndef GITEN_GAME_DDSMENU_H
#define GITEN_GAME_DDSMENU_H

#include <EnumDomain.h>
#include <Ints.h>
#include <Ui/MenuBox.h>

GZ_ENUM_BEGIN_SPLIT(DdsSummonStep, i16)
    DDS_SUMMON_STEP_PREPARE = 0,
    DDS_SUMMON_STEP_PICK = 1,
    DDS_SUMMON_STEP_CLOSE = 2
GZ_ENUM_END_SPLIT(DdsSummonStep)

GZ_ENUM_BEGIN_SPLIT(DdsSummonCursorStep, i16)
    DDS_SUMMON_CURSOR_PICK_ROSTER = 0,
    DDS_SUMMON_CURSOR_PICK_PARTY_SLOT = 1,
    DDS_SUMMON_CURSOR_TRANSITION = 2,
    DDS_SUMMON_CURSOR_EXCHANGE = 3,
    DDS_SUMMON_CURSOR_FINISHED = 4
GZ_ENUM_END_SPLIT(DdsSummonCursorStep)

GZ_ENUM_BEGIN_SPLIT(DdsRosterPickStep, i16)
    DDS_ROSTER_PICK_CLOSED = -1,
    DDS_ROSTER_PICK_OPEN = 0,
    DDS_ROSTER_PICK_POLL = 1,
    DDS_ROSTER_PICK_CLOSE = 2
GZ_ENUM_END_SPLIT(DdsRosterPickStep)

GZ_ENUM_BEGIN_SPLIT(DdsPurgeSubstep, i16)
    DDS_PURGE_PREPARE = 0,
    DDS_PURGE_FINISH = 1,
    DDS_PURGE_PICK = 2
GZ_ENUM_END_SPLIT(DdsPurgeSubstep)

GZ_ENUM_BEGIN_SPLIT(DdsActionResult, i16)
    DDS_ACTION_CANCELLED = -1,
    DDS_ACTION_PENDING = 0,
    DDS_ACTION_COMPLETED = 1
GZ_ENUM_END_SPLIT(DdsActionResult)

b16 RunDdsMenu(void);
b16 RunDdsSummon(void);
GZ_ENUM_RETURN(DdsActionResult, i16) PickDdsSummon(void);
GZ_ENUM_RETURN(DdsRosterPickStep, i16) PickDdsRosterMember(GZ_ENUM_PARAM(DdsRosterPickStep, i16) step);
void DdsMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);
GZ_ENUM_RETURN(DdsActionResult, i16) ReturnDdsMember(void);
i16 PickDdsPurgeMember(void);

#endif // GITEN_GAME_DDSMENU_H
