// @identity-TODO: the original TU name is unproven; this unit holds the
// fusion menu state following the fusion comparison helper family.

#include <rva.h>

#include <Game/Fusion.h>
#include <Game/FusionMenu.h>
#include <Game/FusionScreen.h>
#include <Game/StateStack.h>
#include <Gfx/Render.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/Vram.h>
#include <Platform/PlatformApi.h>
#include <Script/ScriptVars.h>

DATA(0x00080a44)
static i16 s_initialFusionStep;

DATA(0x00080a48)
static i16 s_fusionResultVariable;

DATA(0x00080a4c)
static b16 s_restoreFusionRenderMode;

RVA(0x0002a760, 0x21)
void PushFusionMenu(i16 kind, i16 resultVariable) {
    s_initialFusionStep = kind;
    s_fusionResultVariable = resultVariable;
    PushGameState(31);
}

RVA(0x0002a790, 0x23d)
b16 RunFusionMenuState(void) {
    i16 result;
    switch (GetGamePhase()) {
        case 0:
            switch (GetGameStep()) {
                case 0:
                    NextGameStep();
                case 1:
                    NextGamePhase();
                    NextGamePhase();
                    g_fusionPaletteState = SavePaletteState(g_fusionPaletteState, 2);
                    LoadFusionTables();
                    SetGameStep(s_initialFusionStep);
                    break;
            }
            break;
        case 1:
            switch (GetGameStep()) {
                case 0:
                    NextGameStep();
                case 1:
                    ReturnFromGameState();
                    FreeFusionTables();
                    g_fusionPaletteState = RestorePaletteState(g_fusionPaletteState, 1);
                    ErasePictureSurface(54);
                    if (s_restoreFusionRenderMode) {
                        SetSceneRenderMode();
                        SetBlankStep(0);
                        s_restoreFusionRenderMode = false;
                    }
                    break;
            }
            break;
        case 2:
            switch (GetGameStep()) {
                case 0:
                    ClearStatusPicture();
                    ResetThirdFusionSlot();
                    result = RunFirstFusionPicker(GetGameSub(), 0);
                    SetGameSub(result);
                    if (result < 0) {
                        FinishFusionMenuSelection(result, GetFirstFusionSlot());
                    }
                    break;
                case 3:
                    result = CommitPairFusion();
                    if (s_fusionResultVariable >= 0) {
                        SetScriptLongVar(s_fusionResultVariable, result);
                    }
                    PrevGamePhase();
                    break;
                case 16:
                    ClearStatusPicture();
                    result = RunThirdFusionPicker(GetGameSub());
                    SetGameSub(result);
                    if (result < 0) {
                        FinishFusionMenuSelection(result, GetThirdFusionSlot());
                    }
                    break;
                case 17:
                    result = RunFirstFusionPicker(GetGameSub(), 1);
                    SetGameSub(result);
                    if (result < 0) {
                        FinishFusionMenuSelection(result, GetFirstFusionSlot());
                    }
                    break;
                case 1:
                case 18:
                    result = RunSecondFusionPicker(GetGameSub());
                    SetGameSub(result);
                    if (result < 0) {
                        FinishFusionMenuSelection(result, GetSecondFusionSlot());
                    }
                    break;
                case 20:
                    result = CommitTripleFusion();
                    if (s_fusionResultVariable >= 0) {
                        SetScriptLongVar(s_fusionResultVariable, result);
                    }
                    PrevGamePhase();
                    break;
            }
            break;
    }
    return false;
}

RVA(0x0002a9d0, 0x44)
void FinishFusionMenuSelection(i16 status, i16 selection) {
    if (status < 0) {
        if (s_fusionResultVariable >= 0) {
            SetScriptLongVar(s_fusionResultVariable, selection);
        }
        if (selection < 0 && s_fusionResultVariable == 1) {
            s_restoreFusionRenderMode = true;
        }
        PrevGamePhase();
    }
}
