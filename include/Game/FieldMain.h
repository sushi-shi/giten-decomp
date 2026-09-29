#ifndef GITEN_GAME_FIELDMAIN_H
#define GITEN_GAME_FIELDMAIN_H

#include <rva.h>

#include <Game/GameState.h>
#include <Ints.h>

#include <stdio.h>
#include <Enums.h>

// The macca a battle awards (GrantBattleRewards pays it out).
extern i32 g_rewardMacca;

// The phases of the field exploration state (RunFieldExploration): load the
// area, enter the party's cell, explore, run a cell event, resume exploring
// or run the analyze window, end an event, fade out and return to the return
// point, or fade out and leave for the world map.
GZ_ENUM_BEGIN(FieldPhase)
    FIELD_PHASE_LOAD_AREA = 0,
    FIELD_PHASE_ENTER_CELL = 1,
    FIELD_PHASE_EXPLORE = 2,
    FIELD_PHASE_CELL_EVENT = 3,
    FIELD_PHASE_RESUME = 4,
    FIELD_PHASE_RESUME_ALIAS = 5,
    FIELD_PHASE_ANALYZE = 6,
    FIELD_PHASE_END_EVENT = 7,
    FIELD_PHASE_FADE_TO_RETURN_POINT = 8,
    FIELD_PHASE_RETURN_TO_RETURN_POINT = 9,
    FIELD_PHASE_FADE_TO_WORLD_MAP = 10,
    FIELD_PHASE_CLOSE = 11
GZ_ENUM_END(FieldPhase)

b16 RunFieldExploration(void);

// Sets the return point the field leaves to (and resets the field objects
// and the selected hotspot).
void SetReturnPoint(i16 area, i16 level, i16 x, i16 y, i16 direction);

// Moves the party to x/y facing `direction` and rebuilds the view.
void MovePartyTo(i16 x, i16 y, i16 direction);

// scenecell's scene hold, for the auto-move op. Codegen constraint: declared
// here; in <Game/Scene.h> it perturbs scripttext (TU state).
i16 ExchangeSceneHold(i16 hold);
void MarkSceneDirty(void);

i16 AdvancePartyMove(i16 command);
void ResetLevelEvents(void);
b32 TestLevelEvent(i16 level);
b16 RaiseObjectEvent(i16 event, i16 queued);
i16 SaveFieldMemory(FILE* fp);
void LoadFieldEventTable(void);

void MergeViewOcclusionMask(u8** table, i16 index, void* destination);
// @identity-TODO: the fourth argument is passed as zero and unused here.
void BuildViewOcclusion(i16 x, i16 y, i16 direction, i16 mode);
void MarkVisibleFieldCells(i16 unused, i16 x, i16 y, i16 direction);
void MarkFieldViewCells(i16 unused, i16 x, i16 y, i16 direction);
i16 GetViewVisibility(i16 across, i16 along, i16 side);
// Sampling fills four rows. The HUD's final scan can read one row past these
// allocations; that row is not part of either array.
extern u8 g_leftFrontWalls[4][3];
extern u8 g_rightFrontWalls[4][3];
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
b16 HasAutoMoves(void);
u8 PopAutoMove(void);
i16 NextAutoMove(void);
i16 GetFieldExplorationActive(void);
i16 SetPendingSound(i16 sound);
void SetRebuildRoom(i16 rebuild);
i16 GetReturnPoint(i16* out);
b16 TickStepDamage(void);
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
b16 DrainUpkeep(Character* hero, Character* member, i16 cost, i16 position);

#endif // GITEN_GAME_FIELDMAIN_H
