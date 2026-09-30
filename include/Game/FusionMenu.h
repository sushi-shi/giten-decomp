#ifndef GITEN_GAME_FUSIONMENU_H
#define GITEN_GAME_FUSIONMENU_H

#include <EnumDomain.h>
#include <Game/FusionMenuStep.h>
#include <Ints.h>

GZ_ENUM_BEGIN_SPLIT(FusionMenuPhase, i16)
    FUSION_MENU_PHASE_OPEN = 0,
    FUSION_MENU_PHASE_CLOSE = 1,
    FUSION_MENU_PHASE_SELECT = 2
GZ_ENUM_END_SPLIT(FusionMenuPhase)

GZ_ENUM_BEGIN_SPLIT(FusionMenuLifecycleStep, i16)
    FUSION_MENU_LIFECYCLE_ADVANCE = 0,
    FUSION_MENU_LIFECYCLE_APPLY = 1
GZ_ENUM_END_SPLIT(FusionMenuLifecycleStep)

b16 RunFusionMenuState(void);
void PushFusionMenu(GZ_ENUM_PARAM(FusionMenuStep, i16) kind, i16 resultVariable);
void FinishFusionMenuSelection(i16 status, i16 selection);

#endif // GITEN_GAME_FUSIONMENU_H
