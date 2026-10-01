#ifndef GITEN_SCRIPT_SCRIPTCMD_H
#define GITEN_SCRIPT_SCRIPTCMD_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/AlignmentSide.h>
#include <Game/CharacterPools.h>
#include <Game/CharacterStat.h>
#include <Ints.h>

GZ_ENUM_BEGIN_SPLIT(ItemCountScope, i16)
    ITEM_COUNT_BAG = 0,
    ITEM_COUNT_EQUIPMENT = 1,
    ITEM_COUNT_BAG_AND_EQUIPMENT = 2
GZ_ENUM_END_SPLIT(ItemCountScope)

struct ItemSlot;
i16 CountItemInSlots(i16 item, struct ItemSlot* slots);

// One selectable text run in a script window's choice list.
typedef struct ScriptChoice {
    struct ScriptChoice* next;
    i16 x;
    i16 y;
    i16 width;
    i16 value;
    u16 disabled : 1;
    u16 : 15;
} ScriptChoice;

ScriptChoice* ListTail(ScriptChoice* node);
ScriptChoice* AppendScriptChoice(ScriptChoice** head);
ScriptChoice* FreeScriptChoices(ScriptChoice* head);
ScriptChoice*
PrintScriptChoice(i16 window, ScriptChoice** head, const char* text, i16 value, i16 disabled);
ScriptChoice* PushScriptChoiceMenu(ScriptChoice* choices, i16 window, i16 keep, i16 cancelMode);
void InitScriptChoiceMenu(ScriptChoice* choices, i16 window, i16 keep, i16 cancelMode);
i16 FindScriptChoiceAtMouse(void);
i16 PollScriptChoiceMenu(void);
GZ_ENUM_BEGIN_SPLIT(ScriptChoiceSubstep, i16)
    SCRIPT_CHOICE_SUBSTEP_INITIALIZE = 0,
    SCRIPT_CHOICE_SUBSTEP_POLL = 1
GZ_ENUM_END_SPLIT(ScriptChoiceSubstep)

b16 RunScriptChoiceState(void);

void OpIfMemberHasCondition(void);
// @identity-TODO: why the conditions are copied into bank 15's tag is unrecovered.
void OpSaveObjectConditions(void);
void OpApplyObjectCondition(void);
void OpClearObjectCondition(void);
// An object's total of `stat`; 0 for no object.
i32 GetObjectStatTotal(i16 ref, GZ_ENUM_PARAM(CharacterStat, i16) stat);
i32 GetObjectLevel(i16 ref);
i32 GetObjectAlignmentLevelB(i16 ref);
i32 GetObjectAlignmentLevelA(i16 ref);
GZ_ENUM_BEGIN_SPLIT(AlignmentStepMode, i16)
    ALIGNMENT_STEP_TO_POSITIVE = 0,
    ALIGNMENT_STEP_TO_NEUTRAL = 1,
    ALIGNMENT_STEP_TO_NEGATIVE = 2
GZ_ENUM_END_SPLIT(AlignmentStepMode)

GZ_ENUM_RETURN(AlignmentSide, i16) StepForMode(GZ_ENUM_PARAM(AlignmentStepMode, i16) mode);
void OpShiftPlayerAlignmentB(void);
void OpShiftPlayerAlignmentA(void);
b16 OpLevelUpMember(void);
void OpAddFamiliarityCount(i16 negate);
void OpAddActorFamiliarity(i16 negate);
void OpAddActorLevelGap(i16 negate);
void OpSetActorFamiliarity(void);
void OpSetActorLevelGap(void);
void OpSetActorAttitude(void);
void OpSetActorFieldState(void);
void OpSetObjectFamiliarity(i16 negate);
// @identity-TODO: What the 64-entry item list at 0x7fea0 is (pending/received items) is
// unproven (see Game/ItemPool.h).
void OpBranchOnItemsFit(i16 invert);
void OpRemovePendingItem(void);
void OpAddPendingItem(void);
// @identity-TODO: The byte at +2 of the per-id record read by 0xffa0 (table handle 0x47b0d0) is
// assumed to be a kind/race; its other readers would name it.
void OpMaskRosterByKind(void);
void OpRecoverRosterPool(GZ_ENUM_PARAM(CharacterPoolMask, i16) pool);
void OpCureRosterCondition(void);

#endif // GITEN_SCRIPT_SCRIPTCMD_H
