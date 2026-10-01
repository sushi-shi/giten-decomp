#ifndef GITEN_GAME_CLICKWAIT_H
#define GITEN_GAME_CLICKWAIT_H

#include <EnumDomain.h>
#include <Ints.h>

GZ_ENUM_CONST_BEGIN(ClickWaitResult)
    CLICK_WAIT_CANCELLED = -1
GZ_ENUM_CONST_END(ClickWaitResult)

i16 TakeClickUnlessCancel(i16 command);

#endif // GITEN_GAME_CLICKWAIT_H
