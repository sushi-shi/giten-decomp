#ifndef GITEN_SCRIPT_SCRIPTOPS_H
#define GITEN_SCRIPT_SCRIPTOPS_H

#include <rva.h>

#include <Enums.h>
#include <Game/MoveCommand.h>
#include <Script/ActorSpoilKind.h>
#include <Script/BranchMode.h>
#include <Script/ScriptPanel.h>
#include <Script/ScriptStatus.h>
#include <Script/WindowReverseMode.h>
#include <Util/Compare.h>

GZ_ENUM_BEGIN(ScriptFlagAction)
    SCRIPT_FLAG_TEST = 0,
    SCRIPT_FLAG_SET = 1,
    SCRIPT_FLAG_CLEAR = 2,
    SCRIPT_FLAG_TOGGLE = 3,
GZ_ENUM_END(ScriptFlagAction)

// Script opcode handlers and the helpers they share. Their owning translation
// units are not recovered yet; each declaration moves to its owner's header
// when that unit is reconstructed.

void SetWindowReverse(i16 window, GZ_ENUM_PARAM(WindowReverseMode, i16) mode);

void OpSetWindowColor(i16 window, i16 part);

void StashWindowColor(i16 window, i32 save);

void BeginWindowAltText(i16 window);

void EndWindowAltText(i16 window);

void BeginWindowInstantText(i16 window);

void EndWindowInstantText(i16 window);

void OpSetWindowAltColor(i16 window);
void ResetWindowAltColor(i16 window);

void OpSetWindowInstantColor(i16 window);
void ResetWindowInstantColor(i16 window);

void OpCreateScriptMenu(void);

void OpRunScriptMenu(void);

void OpDestroyScriptMenu(void);

void OpAddMenuLine(void);

void OpGetMenuTag(void);

void OpSetMenuLineColor(void);

void OpClearMenuHighlight(void);

void OpGetMenuCursor(void);

void OpRedrawScriptMenu(void);

void OpSetMenuScroll(void);

void OpAllocLongArray(void);

void OpFreeLongArray(void);

void OpGetLongArrayItem(void);

void OpSetLongArrayItem(void);

void OpLoadDataFile(void);

void OpFreeDataFile(void);

void OpReadRecordInt(void);

void OpReadDataInt(void);

void OpSaveSceneCell(void);

void OpCaptureDataString(void);

void OpCaptureRecordString(void);

void OpCallTextScript(void);

b16 OpStepListMenu(void);
void OpSetMenuCharacter(void);

// The save-data operations OpSaveDataCommand reads: a slot's summary field,
// saving (at the return point when one is set), loading, and the summary field
// as captured text.
GZ_ENUM_BEGIN(SaveDataOperation)
    SAVE_DATA_SUMMARY = 0,
    SAVE_DATA_SAVE = 1,
    SAVE_DATA_LOAD = 2,
    SAVE_DATA_SUMMARY_TEXT = 3
GZ_ENUM_END(SaveDataOperation)

b16 OpSaveDataCommand(void);

void OpIfFlags(b16 all);

// Queries the last field-entry result, or enters a map with a view-hold
// setting and the two sides' count/rate pairs.
RVA_DECL(0x000324b0)
void OpEnterFieldMap(void);

// Tests whether the current action's actor and target are the same combatant.
RVA_DECL(0x00032560)
void OpIfEventObjectIs(void);

// Branches on whether the last action struck one target or multiple targets.
RVA_DECL(0x000325b0)
void OpIfBattleResult(void);

i16 OpCountObjectsAt(void);

void OpIfStatusPositive(i16 invert);

RVA_DECL(0x000326a0)
void OpIfInBattle(void);

// @identity-TODO: the meaning of the values 0/1 the battle-end code writes is unrecovered
RVA_DECL(0x000326e0)
void OpGetBattleOutcome(void);

// @identity-TODO: what the word 0x468424 controls inside game state 0xb is unrecovered
RVA_DECL(0x00032700)
void OpSetFieldOption(void);

// @identity-TODO: what the three words (0x468428, 0x46842c, 0x478508) control is unrecovered
RVA_DECL(0x00032710)
void OpSetFieldParams(void);

// Reads the script's case table and jumps (with `call`, calls) to the entry for
// `value`.
// Exact matching chooses the last equal case; otherwise the first upper
// bound at least as large as the value is selected.
void SwitchOnValue(u8 value, i16 call, i16 exactMatch);

// Returns the selected case byte (255 when absent), with bit 8 set for a
// local jump. The outputs are a jump target or a script file/entry pair.
i16 ReadScriptSwitch(u8 value, i16* target, i16* entry, i16 exactMatch);

RVA_DECL(0x000328c0)
void OpSwitchOnRandom(i16 call);

void OpSwitchOnSelection(i16 call);

// @identity-TODO: which axis (law/chaos or light/dark) +0x7a is unproven
RVA_DECL(0x00032900)
void OpSwitchOnAlignmentA(i16 call);

// @identity-TODO: which axis +0x7b is unproven
RVA_DECL(0x00032930)
void OpSwitchOnAlignmentB(i16 call);

RVA_DECL(0x00032960)
void OpSwitchOnRange(i16 call);

// @identity-TODO: the meaning of actor byte +0x1c4 is unrecovered
RVA_DECL(0x00032980)
void OpSwitchOnActorAttrA(i16 call);

// @identity-TODO: the meaning of actor byte +0x1c5 is unrecovered
RVA_DECL(0x000329a0)
void OpSwitchOnActorAttrB(i16 call);

RVA_DECL(0x000329c0)
void OpSwitchOnValue(i16 call);

void DespawnScriptActor(void);

void RetireScriptActor(void);

RVA_DECL(0x00032d30)
GZ_ENUM_RETURN(ScriptStatus, i16) StepScriptActor(GZ_ENUM_PARAM(MoveCommand, i16) turn);

RVA_DECL(0x00032d80)
void OpStoreActorDistance(void);

// @identity-TODO: What 0xda40/0xdbf0 build (map sprite vs event object) is unproven; decode
// them to confirm "place on map".
RVA_DECL(0x00032dc0)
void PlaceScriptActor(void);

// What GrantActorReward gives the party from the script actor. The kind it
// reports to the reward script names what was given; a random-table roll
// reports the kind it resolved to, or ACTOR_REWARD_HEALED.
GZ_ENUM_BEGIN(ActorRewardKind)
    ACTOR_REWARD_FIRST_ITEM = 0,
    ACTOR_REWARD_SECOND_ITEM = 1,
    ACTOR_REWARD_GEM = 2,
    ACTOR_REWARD_PICK_ITEM = 3,
    ACTOR_REWARD_SPOIL_MACCA = 4,
    ACTOR_REWARD_SPOIL_MAGNETITE = 5,
    ACTOR_REWARD_SPOIL_EXPERIENCE = 6,
    ACTOR_REWARD_RANDOM = 7,
    ACTOR_REWARD_HEALED = 7,
    ACTOR_REWARD_RANDOM_B = 8
GZ_ENUM_END(ActorRewardKind)

void GrantActorReward(GZ_ENUM_PARAM(ActorRewardKind, i16) kind);

struct Character;
i16 PickEquipmentReward(struct Character* character);

RVA_DECL(0x00033210)
void GrantActorSpoil(GZ_ENUM_PARAM(ActorSpoilKind, i16) kind);

// @identity-TODO: The 0x78878 bitmap meaning (object gone/defeated) is inferred from 0x36080;
// confirm via its readers.
RVA_DECL(0x00033320)
void OpSetObjectPresence(void);

// @identity-TODO: why action 0x10e animates the target instead of removing it is unproven.
void DismissTalkTarget(void);

RVA_DECL(0x00033390)
void OpJumpUnlessActorCanStep(
    GZ_ENUM_PARAM(ScriptTestPolarity, i16) invert,
    GZ_ENUM_PARAM(MoveCommand, i16) turn
);

// @identity-TODO: Meaning of 0x2ac50 (actor+0x1c4=2, local flags 8/10, mode=6) and the word at
// actor+0x17f is unproven; decode other 0x2ac50 callers (0x7390, 0xf890).
RVA_DECL(0x00033490)
GZ_ENUM_RETURN(ScriptStatus, i16) OpSetActorAlert(i16 level);

RVA_DECL(0x00033510)
void OpJumpUnlessPlayerInLine(i16 invert);

b16 OpPushReturnTarget(void);

b16 DropCallFrame(void);

b16 SwapCallFrames(void);

b16 ClearCallStack(void);

i16 ReturnFromCall(void);

void RestartScript(i16 file, i16 entry);

// @identity-TODO: Content of data-file 9 records beyond the shop slot 0x7f is unproven; decode
// 0x3aa90 / the record users.
RVA_DECL(0x00034000)
void OpLoadRecord(void);

// Ends the game: sets g_quitRequest, which StepGame returns (the system
// menu's quit confirmation sets it too).
RVA_DECL(0x000345a0)
void RequestQuit(void);

RVA_DECL(0x000345b0)
void OpChangeHp(i16 sign);

RVA_DECL(0x000345e0)
void OpChangeMp(i16 sign);

// @identity-TODO: It always reads the MP pair (+0x84/+0x86) but writes hp.cur when which==0;
// confirm whether that is the intended source.
RVA_DECL(0x00034610)
void OpBoostPool(void);

RVA_DECL(0x00034680)
i16 ReadBranchTarget(void);

RVA_DECL(0x00034740)
void OpJumpUnlessCompare(ComparisonOperator op, i32 withRhs);

RVA_DECL(0x00034780)
b32 OpApplyEventFlag(ScriptFlagAction action, i32 expect);

RVA_DECL(0x00034850)
void OpJumpUnlessEventFlag(ScriptFlagAction action, i32 expect);

RVA_DECL(0x00034880)
void OpJumpUnlessFlagSet(void);

RVA_DECL(0x000348b0)
void OpJumpUnlessStatContest(i16 level, GZ_ENUM_PARAM(ScriptTestPolarity, i16) invert, b16 swap);

void OpJumpUnlessPlayerInView(i16 invert);

void OpJumpUnlessHpPercentRoll(ComparisonOperator op);

void OpJumpUnlessHpQuarterRoll(ComparisonOperator op);

void OpJumpUnlessPlayerNearFront(i16 invert);

// @identity-TODO: The byte at actor+0x6d as a trigger range and the 0x784fc override (0x7160)
// are unproven.
void OpJumpUnlessPlayerAtRange(i16 invert);

void OpJumpUnlessActorVisible(i16 invert);

void OpJumpUnlessInRoster(i16 invert);

void OpJumpUnlessRosterFull(i16 invert);

void OpJumpUnlessAlignmentMatch(i16 invert);

// gamestate's GetRosterCapacity and fieldobj's IsPartyInSight, for the
// roster and sight branches.
// Codegen constraint: declared here; in <Game/GameState.h> and
// <Game/FieldSight.h> they perturb party and fieldobj (TU state).
i16 GetRosterCapacity(void);
i16 IsPartyInSight(i16 x, i16 y);

// @identity-TODO: GetRankScore (rank*10) serving as the price is inferred.
void OpJumpUnlessCanAfford(i16 invert);

void OpJumpUnlessInParty(i16 invert);

// @identity-TODO: That roster ids >= 0x20 are demons is unproven; confirm against the
// character/demon id split.
void OpJumpUnlessRosterHasNoDemons(i16 invert);

// @identity-TODO: The five bytes at +0x156 as status ailments are inferred from
// 0x3edd0/0x3faa0.
void OpJumpUnlessHealthy(i16 invert);

// @identity-TODO: Which characters script ids -2/-3/-7 name is unproven.
void OpJumpUnlessCompanionHealthy(i16 invert);

// @identity-TODO: Which equipment item slot 6 (+0x1ba) is is unproven.
void OpJumpUnlessHeroEquipped(i16 invert);

void OpIfNoActor(i16 negate);

void OpIfFacing(i16 negate);

// @identity-TODO: 0x491090 is the direction saved with the return position (0x124c0 saves,
// 0x125d0 restores into 0x491080..84); confirm it is a "return point" by reading 0x12280/0x13640.
void OpIfReturnFacing(i16 negate);

void OpIfObjectHasCondition(i16 negate);

void OpIfHasItem(i16 negate);

void OpIfHasAllItems(i16 negate);

void OpGiveItem(void);

void OpTakeItem(void);

void OpOpenItemListWindow(void);

void OpCloseItemListWindow(void);

void OpRedrawItemListTotal(void);

// Branches on whether the item pool (<Game/ItemPool.h>) holds any entry.
void OpIfPoolHasItems(i16 negate);

// Branches on whether the bag (g_bagItems) holds any entry (CountBagEntries).
void OpIfBagHasEntries(i16 negate);

// @identity-TODO: the dword at +2 of the item record loaded by 0x22d40 into 0x4911c0 is assumed
// to be the price; confirm from the shop code.
void OpGetItemPrice(void);

// Saves or restores the 16-entry list 0x47fe60 (and related state) through
// consecutive g_scriptVars words starting at the operand (capped at 176).
// @identity-TODO: what the list (re-filled with consecutive ids by 0x246b0)
// holds is unrecovered.
void OpStashItemLists(void);

void OpAdjustItemCount(void);

// The bag-entry ops: list the bag's items by category, read an entry (the
// whole ItemStack into a long variable), empty an entry.
void OpListBagByCategory(void);

void OpGetBagEntry(void);

void OpClearBagEntry(void);

// Takes a drop slot (g_dropSlots): RemapItem replaces the item and RollItemAmount rolls the
// amount of a remapped one.
// @identity-TODO: that g_dropSlots is the battle drop table is assumed; confirm from the battle
// code that fills it.
void OpTakeDropSlot(void);

// @identity-TODO: which scene game mode 0x26 is (pushed with 0x16c00) is unrecovered; find the
// mode-0x26 handler.
GZ_ENUM_RETURN(ScriptStatus, i16) OpCallSubScene(void);

void OpCopyItemRecord(void);

void OpOpenFusionScreen(i16 kind);

void OpRunFusion(b16 triple);

void OpEndFusion(void);

void OpJumpIf(b16 cond);

// @identity-TODO: the dispatcher pushes 1 but the body never reads it; whether the original
// took a flag is unproven.
void OpSkipJumpTarget(i16 unused);

void OpIfDemonCount(GZ_ENUM_PARAM(ScriptTestPolarity, i16) mode, i16 limit);

void OpGetFusionResult(void);

RVA_DECL(0x00035c90)
void OpAddMagnetite(i16 sign);

RVA_DECL(0x00035cc0)
void OpAddMacca(i16 sign);

// @identity-TODO: that the 0xff-terminated byte list is a walk sequence (0x47be84 as "auto-move
// on") is inferred; confirm from the consumer of the 0x47b7cc buffer.
void OpQueueAutoMoves(void);

// @identity-TODO: operand order (map, floor, x, y, dir) is inferred from OpGetPlayerLocation's
// five outputs; confirm from 0x12d20's use of 0x47b760..0x47b770.
void OpChangeMap(void);

void OpSetWorldMapSpot(void);

void OpAddRoutePoint(void);

// @identity-TODO: which of bytes 0x491087/0x491088 is map vs floor is unproven.
void OpGetPlayerLocation(void);

void OpSetPlayerPosition(void);

void OpIfBlockedToward(
    GZ_ENUM_PARAM(ScriptTestPolarity, i16) negate,
    GZ_ENUM_PARAM(MoveCommand, i16) turn
);

// @identity-TODO: the effect table 0x46b9d8 (8 entries, called after a colour-fill Blt in
// 0x49f50) is unnamed.
GZ_ENUM_RETURN(ScriptStatus, i16) PlayScreenTransition(i16 effect);

GZ_ENUM_RETURN(ScriptStatus, i16) OpScreenTransition(void);

// Adds the character, entering the roster replacement state on failure.
void AddScriptCharacterToRoster(struct Character* character, i16 unused);

GZ_ENUM_RETURN(ScriptStatus, i16) OpAddToRoster(void);

RVA_DECL(0x000361b0)
i16 OpRemoveFromRoster(void);

RVA_DECL(0x00036250)
i16 OpJoinActiveParty(void);

RVA_DECL(0x00036330)
i16 OpLeaveActiveParty(void);

RVA_DECL(0x000363d0)
b16 OpSelectPartySlot(void);

RVA_DECL(0x00036400)
b16 OpEndPartySlotSelect(void);

RVA_DECL(0x00036410)
i16 OpCountActiveParty(void);

// Converts the action-actor or action-target reference to its combatant id;
// other references produce -1.
RVA_DECL(0x00036440)
i16 OpGetCombatantId(void);

RVA_DECL(0x000364c0)
void OpIfObjectIsAlly(i16 negate);

// @identity-TODO: the meaning of the four stat rows summed (+0x88/+0x9e/+0xca/+0xb4) and the
// target row +0xe0 is unrecovered.
RVA_DECL(0x00036560)
void OpRebalanceMemberStats(void);

// @identity-TODO: the list at Character+0x1f1 (count + word array; 0x2dce0 inserts if absent)
// is assumed to be skills.
RVA_DECL(0x000365e0)
void OpAddMemberSkill(void);

void OpSwitchOnMoonPhase(i16 call);

void OpGetActorMoonValue(void);

void OpAdvanceClock(void);

void OpGetTicksUntilMoonPhase(void);

void OpGetDayCount(void);

void OpGetTimeOfDay(void);

RVA_DECL(0x00036aa0)
void OpRollActorMagnetite(void);

RVA_DECL(0x00036ad0)
void OpRollActorMacca(void);

RVA_DECL(0x00037490)
i32 ReadScriptValue(void);

RVA_DECL(0x000375b0)
void OpSetObjectField(void);

RVA_DECL(0x00037950)
void OpFindMemberByPoolState(i16 all, i16 pools);

RVA_DECL(0x000379f0)
void OpFindMemberWithCondition(i16 all);

// @identity-TODO: which alignment axis byte +0x7a is (derived from +0x209 by 0x3e0a0) is
// unrecovered.
RVA_DECL(0x00037ad0)
void OpFindMemberByAlignmentA(i16 all);

// @identity-TODO: which alignment axis byte +0x7b is (derived from +0x1f9 by 0x3e0d0) is
// unrecovered.
RVA_DECL(0x00037bc0)
void OpFindMemberByAlignmentB(i16 all);

RVA_DECL(0x00037cb0)
void OpCountItemOwned(void);

// @identity-TODO: The dispatcher passes 0 (case 369) and 1 (case 371) but the body never reads
// the argument; what the two opcodes were meant to differ in is unrecovered.
RVA_DECL(0x0003a250)
b16 OpPlaceSprite(i16 variant);

RVA_DECL(0x0003a350)
b16 OpHideSprite(void);

// @identity-TODO: Fade direction of 0x49d60 modes 1/5 vs 2/6 is inferred only from
// OpFadeOutAndClear (mode 2 then colour-fill); the fade renderer would confirm.
RVA_DECL(0x0003a370)
void OpFadeIn(void);

// @identity-TODO: Fade direction of 0x49d60 modes 2/6 is inferred only from OpFadeOutAndClear;
// the fade renderer would confirm.
RVA_DECL(0x0003a3b0)
void OpFadeOut(void);

// @identity-TODO: The low three bits go to the empty Windows stub 0x15830 (a PC-98 display
// call); the exact screen part refreshed is inferred.
RVA_DECL(0x0003a3f0)
void OpRefreshFieldScreen(void);

RVA_DECL(0x0003a440)
void OpFadeOutAndClear(void);

RVA_DECL(0x0003a4a0)
void OpResetMask(void);

RVA_DECL(0x0003a4b0)
void OpDrawImage(void);

// @identity-TODO: PC-98 VRAM path whose Windows bodies (FillCell) are empty; the original
// effect is inferred.
RVA_DECL(0x0003a530)
void OpFillScreenCells(void);

// @identity-TODO: PC-98 VRAM path stubbed on Windows; the original effect is inferred.
RVA_DECL(0x0003a5b0)
void OpMaskScreenCells(void);

// @identity-TODO: The bits ORed into 0x78068 by 0x4920 are unrecovered.
RVA_DECL(0x0003a640)
void OpEnableBackground(void);

void OpSkipValueAndVar(void);

// @identity-TODO: 0x18740 never writes its two out-parameters, so the second and third
// variables receive uninitialised stack words; the PC-98 original would show their meaning.
void OpPollMouseClick(void);

// Plays a screen-effect animation at the script-supplied origin.
void OpPlayAnimation(void);

RVA_DECL(0x0003a710)
void OpSwapScreenState(void);

void OpPushGameState(void);

// @identity-TODO: How it differs in purpose from OpShowBackground (record flag word 1, 10-frame
// refresh) is unproven.
RVA_DECL(0x0003c460)
void OpShowPicture(void);

// @identity-TODO: Why pictures 0x5080..0x508f/0x5092..0x5098/0x509a..0x50c7 get an immediate
// render is unrecovered.
RVA_DECL(0x0003c4e0)
void OpShowEventPicture(void);

// @identity-TODO: The Windows screen-save bodies are empty stubs, so the saved area's use is
// inferred from the PC-98 shape.
RVA_DECL(0x0003c5a0)
void OpSaveRestoreScreen(void);

#endif // GITEN_SCRIPT_SCRIPTOPS_H
