#ifndef GITEN_GAME_FIELDMAIN_H
#define GITEN_GAME_FIELDMAIN_H

#include <rva.h>

#include <Game/GameState.h>
#include <Ints.h>

#include <stdio.h>

// @identity-TODO: the macca a battle awards (GrantBattleRewards pays it out),
// cleared when the field is entered; its owner TU is unclaimed.
extern i32 g_rewardMacca;

i16 RunFieldExploration(void);

// Sets the return point the field leaves to (and resets the field objects
// and the selected hotspot).
void SetReturnPoint(i16 area, i16 level, i16 x, i16 y, i16 direction);

// Moves the party to x/y facing `direction` and rebuilds the view.
void MovePartyTo(i16 x, i16 y, i16 direction);

// scenecell's scene hold, for the auto-move op. Codegen constraint: declared
// here; in <Game/Scene.h> it perturbs scripttext (TU state).
i16 ExchangeSceneHold(i16 hold);
void MarkSceneDirty(void);

void GrowRoute(i16 more);
void FreeRoute(void);
void PushRoutePoint(MapCoord point);
MapCoord PopRoutePoint(void);
i16 IsRouteActive(void);
i16 AdvancePartyMove(i16 command);
void ResetLevelEvents(void);
i32 TestLevelEvent(i16 level);
i16 RaiseObjectEvent(i16 event, i16 queued);
i16 SaveFieldMemory(FILE* fp);
void LoadFieldEventTable(void);

void MergeViewOcclusionMask(u8** table, i16 index, void* destination);
// @identity-TODO: the fourth argument is passed as zero and unused here.
void BuildViewOcclusion(i16 x, i16 y, i16 direction, i16 mode);
void MarkVisibleFieldCells(i16 unused, i16 x, i16 y, i16 direction);
void MarkFieldViewCells(i16 unused, i16 x, i16 y, i16 direction);
i16 GetViewVisibility(i16 across, i16 along, i16 side);
// @identity-TODO: sampling clears four rows, but the HUD may read a fifth.
// Recover the allocation boundaries before defining these arrays.
extern u8 g_leftFrontWalls[][3];
extern u8 g_rightFrontWalls[][3];
extern u8 g_centerFrontWalls[5];
extern u8 g_leftSideWalls[5][3];
extern u8 g_rightSideWalls[5][3];

void SampleViewWalls(i16 x, i16 y, i16 direction);
void SelectFieldImageCache(i16 id, i16 variant);
void LoadFieldImage(i16 image, i16 variant, i16 mode);
void VisitVisibleCellWalls(i16 view, i16 across, i16 along, i16 direction, u16 cell);
void ResetFieldMemory(void);
void ReturnToCurrentCell(void);
void SetAreaFlagPreservation(i16 area, i16 level);
struct TreasureBox;
struct AreaNpc;
void StartBoxScene(struct TreasureBox* box);
void StartNpcScene(struct AreaNpc* npc);

// Callees of the talk and analyze picks, declared here for fieldmain:
// abortflag's exchange, analyze's target, fieldobj's talk scene start, and
// the training-point add (0x41c6c0: kind 0..3 of Character.trainingPoints,
// capped). Codegen constraint: SetAnalyzeTarget in <Game/Analyze.h> would
// reach field.c.
i16 ExchangeAbortPending(i16 pending);
void SetAnalyzeTarget(Character* target);
void StartActorScene(i16 scene, i16 entry, i16 index, Character* actor);

// @identity-TODO: label-only; the cap (0x41c650(99)) is unrecovered.
u32 AddTrainingPoints(Character* character, i16 kind, i16 amount);

// The object index under the selected hotspot, -1 for none.
// @identity-TODO: label-only; the hotspot table 0x48802c is Ui/Hotspot's.
RVA_DECL(0x000585c0)
i16 GetSelectedHotspotObject(void);

// fieldscreen's wall-damage flash. Codegen constraint: declared here; in
// <Game/FieldScreen.h> it perturbs fieldobj (TU state).
void PlayWallEffect(void);
void SetSavedPoint(i16 x, i16 y, i16 direction);
void RestoreSavedPoint(void);
void FreeAutoMoves(void);
void ResetAutoMoves(void);
void GrowAutoMoves(i16 more);
void PushAutoMove(u8 move);
i16 HasAutoMoves(void);
u8 PopAutoMove(void);
i16 NextAutoMove(void);
i16 GetFieldExplorationActive(void);
i16 SetPendingSound(i16 sound);
void SetRebuildRoom(i16 rebuild);
i16 GetReturnPoint(i16* out);
i16 TickStepDamage(void);
i16 TickFieldSteps(void);

// The party's per-step effects, run on every third step.
// @identity-TODO: label-only; what the effects are (0x4254b0 per member) is
// unrecovered.
RVA_DECL(0x0003fd70)
i16 TickPartySteps(void);

// The per-step party upkeep (charsave) and the magnetite/MP/HP drain it
// pays through.
i16 PayStepUpkeep(void);
i16 AddHundredths(Character* character, i16 amount);
i16 DrainUpkeep(Character* hero, Character* member, i16 cost, i16 position);

#endif // GITEN_GAME_FIELDMAIN_H
