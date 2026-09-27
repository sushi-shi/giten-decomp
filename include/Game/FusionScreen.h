#ifndef GITEN_GAME_FUSIONSCREEN_H
#define GITEN_GAME_FUSIONSCREEN_H

#include <rva.h>

#include <Game/Character.h>
#include <Game/Fusion.h>
#include <Gfx/Palette.h>
#include <Ui/MenuBox.h>

// @identity-TODO: the Windows screen-save stubs do not reveal its extent.
extern u8 g_fusionPreviewSave[];
// @identity-TODO: extents are unproven; only text producers/consumers reach these buffers.
extern char g_fusionNameBuffer[];
extern char g_fusionMissingRace[];
extern char g_fusionMissingName[];
extern PaletteState* g_fusionPaletteState;
extern i16 g_fusionFirstSlot;
extern i16 g_fusionSecondSlot;
extern i16 g_fusionThirdSlot;

void AcquireFusionSelectionMode(void);
void ReleaseFusionSelectionResources(void);
void ResetThirdFusionSlot(void);
i16 GetFirstFusionSlot(void);
i16 GetSecondFusionSlot(void);
i16 GetThirdFusionSlot(void);
i16 GetFusionResultKind(void);
i16 RunFirstFusionPicker(i16 step, i16 triple);
i16 RunSecondFusionPicker(i16 step);
i16 RunThirdFusionPicker(i16 step);
void DrawFusionSummaryGrid(void);
i16 BuildPairFusionCandidates(i16 skipCalculation);
i16 BuildTripleFusionSummaries(i16 third);
void StoreFusionPairSummary(i16 first, i16 second, const FusionSummary* summary);
FusionSummary* GetFusionPairSummaryCell(i16 first, i16 second);
i16 CreateFusionList(i16 window, i16 count);
void FusionListMenuHandler(MenuBox* menu, i16 index, i16 event);
void FusionSelectionTextHook(i16 plane, i16 event, i16 value);
i32 CloseFusionPicker(i16 selection);
i16 CreateFusionInfoPlane(i16 unused);
void DrawFusionCharacterDetails(i16 plane, Character* character);
void DrawFusionStatGroup(i16 plane, i16 x, i16* stats);
i16 CloseFusionPreviewOnClick(i16 plane);
i16 OpenFusionPreviewOnClick(void);
void DrawFusionPreviewCard(i16 plane, Character* character);
i16 CreateFusionPreviewCard(i16 window, i16 slot);
i16 PreviewFusionCharacter(Character* character);
i16 CloseFusionPreview(void);
void RunPairFusion(void);
void RunTripleFusion(void);
void EndFusion(void);
i16 CommitPairFusion(void);
i16 CommitTripleFusion(void);
Character* LoadFusionResultCharacter(Character* destination);
i16 StageFusionCharacter(i16 id);
i16 StagePairFusionCharacter(i16 first, i16 second, i16 rankChanges);
i32 RestoreFusionCharacter(void);

Character* CreatePairFusionCharacter(i16 first, i16 second, i16 rankChanges);

#endif // GITEN_GAME_FUSIONSCREEN_H
