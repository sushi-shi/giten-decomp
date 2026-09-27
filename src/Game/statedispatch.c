// @identity-TODO: the owning TU is unproven; this unit holds the main
// state dispatcher until link-order evidence names it.

#include <rva.h>

#include <Game/Automap.h>
#include <Game/DdsMenu.h>
#include <Game/Field.h>
#include <Game/FieldMain.h>
#include <Game/FusionMenu.h>
#include <Game/GemItems.h>
#include <Game/ItemMenu.h>
#include <Game/LevelUp.h>
#include <Game/PartyReorder.h>
#include <Game/SaveGame.h>
#include <Game/Scene.h>
#include <Game/ScreenEffect.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Game/StatusScreen.h>
#include <Game/WaitLoop.h>
#include <Game/WorldMap.h>
#include <Gfx/Shot.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptVars.h>
#include <Ui/FieldMenus.h>
#include <Ui/Message.h>

RVA(0x00017160, 0x1d0)
i16 DispatchGameState(void) {
    i16 result;
    u16 state = GetGameState();
    switch (state) {
        case 0:
            ReturnFromGameState();
            result = 0;
            break;
        case 1:
            result = StepWaitState();
            break;
        case 5:
            RunScriptScene();
            result = 0;
            break;
        case 6:
            RunScriptChoiceState();
            result = 0;
            break;
        case 7:
            RunDismissMenuState();
            result = 0;
            break;
        case 8:
            RunTextWindowState();
            result = 0;
            break;
        case 9:
            result = RunShotState();
            break;
        case 10:
            result = RunActorScene();
            break;
        case 11:
            result = RunFieldEncounter();
            break;
        case 12:
            result = RunClosingEffectState();
            break;
        case 13:
            result = RunScreenFadeState();
            break;
        case 14:
            result = RunItemUse();
            break;
        case 15:
            result = RunSystemMenu();
            break;
        case 16:
            result = RunFieldExploration();
            break;
        case 18:
            result = RunCellScene();
            break;
        case 19:
            result = RunItemBuyMenu();
            break;
        case 21:
            result = RunFieldTextScene();
            break;
        case 22:
            result = RunWorldMap();
            break;
        case 23:
            result = RunBackgroundScene();
            break;
        case 24:
            result = RunBattleAction();
            break;
        case 25:
            result = RunStatusScreen();
            break;
        case 26:
            result = RunFieldSkillUse();
            break;
        case 27:
            result = RunLevelUp();
            break;
        case 28:
            result = RunItemSellMenu();
            break;
        case 29:
            result = RunFrozenFieldScene();
            break;
        case 30:
            result = RunDdsMenu();
            break;
        case 31:
            result = RunFusionMenuState();
            break;
        case 32:
            result = RunAutomapState();
            break;
        case 33:
            result = RunMessageBoxState();
            break;
        case 34:
            result = RunFieldState();
            break;
        case 35:
            result = FinishMessageScene();
            break;
        case 36:
            result = RunPartyReorder();
            break;
        case 37:
            result = RunScriptAnimationState();
            break;
        case 38:
            result = RunGemItemGift();
            break;
        case 39:
            result = RunPictureTransition();
            break;
        case 40:
            result = ReplaceRosterMember();
            break;
    }
    // Retail leaves the result uninitialized for unhandled state numbers.
    return result;
}
