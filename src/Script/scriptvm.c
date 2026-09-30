// @identity-TODO: the owning TU is unproven; this unit holds the dispatcher's
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/FusionMenuStep.h>
#include <Game/GameState.h>
#include <Game/ItemPool.h>
#include <Game/MoveCommand.h>
#include <Game/StateStack.h>
#include <Gfx/Render.h>
#include <Input/MouseCancelMode.h>
#include <Script/ActorAlertMode.h>
#include <Script/ActorSpoilKind.h>
#include <Script/BranchMode.h>
#include <Script/EventFlags.h>
#include <Script/LongOperandMode.h>
#include <Script/LongVar.h>
#include <Script/RosterQueryResult.h>
#include <Script/Script.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptComparisonRhs.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptSprite.h>
#include <Script/ScriptText.h>
#include <Script/ScriptValueSign.h>
#include <Script/ScriptVars.h>
#include <Script/ScriptVm.h>
#include <Script/TextState.h>
#include <Script/WindowColorStash.h>
#include <Script/WindowReverseMode.h>
#include <Sound/Sound.h>
#include <Text/Font.h>
#include <Text/TextWindow.h>

DATA(0x00081228)
GZ_ENUM_STORAGE(ScriptOpcode, u16) g_scriptOpcode = SCRIPT_OP_END;

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x0002ff40, 0x7)
GZ_ENUM_RETURN(ScriptOpcode, u16) GetScriptOpcode(void) {
    return g_scriptOpcode;
}

// Executes one script opcode for the text window `window`. Opcodes 29..31
// prefix a second byte naming an extended opcode (0x300/0x200/0x100 + byte),
// which is dispatched in turn. Returns 0 to continue, -1 to end the script
// and -3 to yield until the next frame; some handlers return their own status.
RVA(0x0002ff50, 0x2560)
GZ_ENUM_RETURN(ScriptStatus, i16) ExecScriptOpcode(i16 window, GZ_ENUM_PARAM(ScriptOpcode, u16) op) {
    u16 entry;
    i16 target;

    for (;;) {
        g_scriptOpcode = op;
        switch (op) {
            case SCRIPT_OP_EXTEND_BANK_THREE:
                op = ReadScriptByte() + SCRIPT_OPCODE_BANK_THREE;
                continue;
            case SCRIPT_OP_EXTEND_BANK_TWO:
                op = ReadScriptByte() + SCRIPT_OPCODE_BANK_TWO;
                continue;
            case SCRIPT_OP_EXTEND_BANK_ONE:
                op = ReadScriptByte() + SCRIPT_OPCODE_BANK_ONE;
                continue;
            case SCRIPT_OP_RESTART_AND_DESPAWN_ALIAS:
                op = SCRIPT_OP_RESTART_AND_DESPAWN;
                continue;
            case SCRIPT_OP_CALL_BANK_SCRIPT_1:
            case SCRIPT_OP_CALL_BANK_SCRIPT_2:
            case SCRIPT_OP_CALL_BANK_SCRIPT_3:
            case SCRIPT_OP_CALL_BANK_SCRIPT_4:
            case SCRIPT_OP_CALL_BANK_SCRIPT_5:
            case SCRIPT_OP_CALL_BANK_SCRIPT_6:
            case SCRIPT_OP_CALL_BANK_SCRIPT_7:
            case SCRIPT_OP_CALL_BANK_SCRIPT_8:
                entry = ReadScriptByte();
                CallScript(op + 0x7eff, entry);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_TEST_RESULT_CLEAR:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TEST, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADVANCE_WINDOW_LINE:
                AdvanceWindowLine(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_TEST_RESULT_SET:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TEST, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GOTO_SCRIPT:
                OpJumpScript(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CALL_SCRIPT:
                OpJumpScript(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_RANDOM_JUMP:
                OpSwitchOnRandom(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_SELECTION_JUMP:
                OpSwitchOnSelection(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CONTEST_LEVEL_0_NORMAL:
                OpJumpUnlessStatContest(0, SCRIPT_TEST_NORMAL, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CONTEST_LEVEL_0_INVERTED:
                OpJumpUnlessStatContest(0, SCRIPT_TEST_INVERTED, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CONTEST_LEVEL_1_NORMAL:
                OpJumpUnlessStatContest(1, SCRIPT_TEST_NORMAL, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CONTEST_LEVEL_1_INVERTED:
                OpJumpUnlessStatContest(1, SCRIPT_TEST_INVERTED, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CONTEST_LEVEL_2_NORMAL:
                OpJumpUnlessStatContest(2, SCRIPT_TEST_NORMAL, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CONTEST_LEVEL_2_INVERTED:
                OpJumpUnlessStatContest(2, SCRIPT_TEST_INVERTED, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CONTEST_LEVEL_3_NORMAL:
                OpJumpUnlessStatContest(3, SCRIPT_TEST_NORMAL, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CONTEST_LEVEL_3_INVERTED:
                OpJumpUnlessStatContest(3, SCRIPT_TEST_INVERTED, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP:
                OpJump();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_EVENT_FLAG:
                OpApplyEventFlag(SCRIPT_FLAG_SET, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_EVENT_FLAG:
                OpApplyEventFlag(SCRIPT_FLAG_CLEAR, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_BEGIN_TEXT_CAPTURE:
                SetTextCapture(true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_END_TEXT_CAPTURE:
                SetTextCapture(false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PRINT_OPERAND_TEXT:
                OpPrintOperandText();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PRINT_NUMBER:
                OpPrintNumber();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_ANY_FLAGS:
                OpIfFlags(false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_ALL_FLAGS:
                OpIfFlags(true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_SWAPPED_CONTEST_LEVEL_0_NORMAL:
                OpJumpUnlessStatContest(0, SCRIPT_TEST_NORMAL, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_SWAPPED_CONTEST_LEVEL_0_INVERTED:
                OpJumpUnlessStatContest(0, SCRIPT_TEST_INVERTED, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_SWAPPED_CONTEST_LEVEL_1_NORMAL:
                OpJumpUnlessStatContest(1, SCRIPT_TEST_NORMAL, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_SWAPPED_CONTEST_LEVEL_1_INVERTED:
                OpJumpUnlessStatContest(1, SCRIPT_TEST_INVERTED, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_SWAPPED_CONTEST_LEVEL_2_NORMAL:
                OpJumpUnlessStatContest(2, SCRIPT_TEST_NORMAL, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_SWAPPED_CONTEST_LEVEL_2_INVERTED:
                OpJumpUnlessStatContest(2, SCRIPT_TEST_INVERTED, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_SWAPPED_CONTEST_LEVEL_3_NORMAL:
                OpJumpUnlessStatContest(3, SCRIPT_TEST_NORMAL, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_SWAPPED_CONTEST_LEVEL_3_INVERTED:
                OpJumpUnlessStatContest(3, SCRIPT_TEST_INVERTED, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ALERT_ACTOR:
                return OpSetActorAlert(ACTOR_ALERT_NORMAL);
            case SCRIPT_OP_ALERT_ACTOR_IMMEDIATELY:
                return OpSetActorAlert(ACTOR_ALERT_IMMEDIATE);
            case SCRIPT_OP_DELAY_ACTOR:
                return OpSetActorAlert(ACTOR_ALERT_DELAY);
            case SCRIPT_OP_SWITCH_ON_RANDOM_CALL:
                OpSwitchOnRandom(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_SELECTION_CALL:
                OpSwitchOnSelection(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_ALIGNMENT_A_JUMP:
                OpSwitchOnAlignmentA(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_ALIGNMENT_A_CALL:
                OpSwitchOnAlignmentA(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_ALIGNMENT_B_JUMP:
                OpSwitchOnAlignmentB(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_ALIGNMENT_B_CALL:
                OpSwitchOnAlignmentB(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_MOON_PHASE_JUMP:
                OpSwitchOnMoonPhase(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_MOON_PHASE_CALL:
                OpSwitchOnMoonPhase(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_RANGE_JUMP:
                OpSwitchOnRange(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_RANGE_CALL:
                OpSwitchOnRange(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_ACTOR_ATTR_A_JUMP:
                OpSwitchOnActorAttrA(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_ACTOR_ATTR_A_CALL:
                OpSwitchOnActorAttrA(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_ACTOR_ATTR_B_JUMP:
                OpSwitchOnActorAttrB(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_ACTOR_ATTR_B_CALL:
                OpSwitchOnActorAttrB(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_ACTOR_ATTITUDE:
                OpSetActorAttitude();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_ACTOR_FIELD_STATE:
                OpSetActorFieldState();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_ACTOR_MODE_ATTACK:
                return SetActorMode(ACTOR_MODE_ATTACK);
            case SCRIPT_OP_SET_ACTOR_MODE_FLEE:
                return SetActorMode(ACTOR_MODE_FLEE);
            case SCRIPT_OP_SET_ACTOR_MODE_DEFEND:
                return SetActorMode(ACTOR_MODE_DEFEND);
            case SCRIPT_OP_SET_ACTOR_MODE_APPROACH:
                return SetActorMode(ACTOR_MODE_APPROACH);
            case SCRIPT_OP_SET_ACTOR_MODE_STEP_INTO_RANGE:
                return SetActorMode(ACTOR_MODE_STEP_INTO_RANGE);
            case SCRIPT_OP_SET_ACTOR_MODE_STEP_CLOSER:
                return SetActorMode(ACTOR_MODE_STEP_CLOSER);
            case SCRIPT_OP_SET_ACTOR_MODE_CIRCLE_AROUND:
                return SetActorMode(ACTOR_MODE_CIRCLE_AROUND);
            case SCRIPT_OP_SET_ACTOR_MODE_RECOVER:
                return SetActorMode(ACTOR_MODE_RECOVER);
            case SCRIPT_OP_SET_ACTOR_MODE_WANDER:
                return SetActorMode(ACTOR_MODE_WANDER);
            case SCRIPT_OP_SET_ACTOR_MODE_IDLE:
                return SetActorMode(ACTOR_MODE_IDLE);
            case SCRIPT_OP_SET_ACTOR_MODE_TALK:
                return SetActorMode(ACTOR_MODE_TALK);
            case SCRIPT_OP_RESET_AND_REVERSE_WINDOW_ATTR:
                SetWindowReverse(window, WINDOW_ATTR_RESET_AND_REVERSE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RESET_WINDOW_ATTR:
                SetWindowReverse(window, WINDOW_ATTR_RESET);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_REVERSE_WINDOW_ATTR:
                SetWindowReverse(window, WINDOW_ATTR_REVERSE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_GLYPH_COLOR:
                OpSetWindowColor(window, TEXT_COLOR_GLYPH);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_BACKGROUND_COLOR:
                OpSetWindowColor(window, TEXT_COLOR_BG);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_DIM_COLOR:
                OpSetWindowColor(window, TEXT_COLOR_DIM);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ENABLE_WINDOW_OPAQUE_BACKGROUND:
                SetWindowOpaqueBg(window, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DISABLE_WINDOW_OPAQUE_BACKGROUND:
                SetWindowOpaqueBg(window, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_ATTR_FLAG1:
                SetWindowAttrFlag1(window, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_WINDOW_ATTR_FLAG1:
                SetWindowAttrFlag1(window, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DISABLE_WINDOW_HALF_WIDTH:
                SetWindowHalfWidth(window, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ENABLE_WINDOW_HALF_WIDTH:
                SetWindowHalfWidth(window, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_WINDOW_ATTR_FLAG2:
                SetWindowAttrFlag2(window, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_ATTR_FLAG2:
                SetWindowAttrFlag2(window, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SAVE_WINDOW_COLOR:
                StashWindowColor(window, WINDOW_COLOR_SAVE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RESTORE_WINDOW_COLOR:
                StashWindowColor(window, WINDOW_COLOR_RESTORE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_ALT_COLOR:
                OpSetWindowAltColor(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_INSTANT_COLOR:
                OpSetWindowInstantColor(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_TO_ROSTER:
                return OpAddToRoster();
            case SCRIPT_OP_REMOVE_FROM_ROSTER:
                OpRemoveFromRoster();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JOIN_ACTIVE_PARTY:
                OpJoinActiveParty();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_LEAVE_ACTIVE_PARTY:
                OpLeaveActiveParty();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MUL_LONG_VAR_EXPLICIT:
                OpMulLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DIV_LONG_VAR_EXPLICIT:
                OpDivLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_LONG_VAR_EXPLICIT:
                OpAddLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SUB_LONG_VAR_EXPLICIT:
                OpSubLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_AND_LONG_VAR_EXPLICIT:
                OpAndLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_OR_LONG_VAR_EXPLICIT:
                OpOrLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_XOR_LONG_VAR_EXPLICIT:
                OpXorLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SHL_LONG_VAR_EXPLICIT:
                OpShlLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SAR_LONG_VAR_EXPLICIT:
                OpSarLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PERCENT_LONG_VAR_EXPLICIT:
                OpPercentLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MUL_LONG_VAR_IN_PLACE:
                OpMulLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DIV_LONG_VAR_IN_PLACE:
                OpDivLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_LONG_VAR_IN_PLACE:
                OpAddLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SUB_LONG_VAR_IN_PLACE:
                OpSubLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_AND_LONG_VAR_IN_PLACE:
                OpAndLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_OR_LONG_VAR_IN_PLACE:
                OpOrLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_XOR_LONG_VAR_IN_PLACE:
                OpXorLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SHL_LONG_VAR_IN_PLACE:
                OpShlLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SAR_LONG_VAR_IN_PLACE:
                OpSarLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PERCENT_LONG_VAR_IN_PLACE:
                OpPercentLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_TOGGLE_EVENT_FLAG:
                OpApplyEventFlag(SCRIPT_FLAG_TOGGLE, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PUSH_RETURN_TARGET:
                OpPushReturnTarget();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DROP_CALL_FRAME:
                DropCallFrame();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWAP_CALL_FRAMES:
                SwapCallFrames();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_CALL_STACK:
                ClearCallStack();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_MACCA:
                OpAddMacca(1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SUBTRACT_MACCA:
                OpAddMacca(-1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_MAGNETITE:
                OpAddMagnetite(1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SUBTRACT_MAGNETITE:
                OpAddMagnetite(-1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GIVE_ITEM:
                OpGiveItem();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_TAKE_ITEM:
                OpTakeItem();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GRANT_ACTOR_PICK_ITEM:
            case SCRIPT_OP_GRANT_ACTOR_PICK_ITEM_ALIAS:
                GrantActorReward(ACTOR_REWARD_PICK_ITEM);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GRANT_ACTOR_MACCA:
                GrantActorSpoil(ACTOR_SPOIL_MACCA);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GRANT_ACTOR_MAGNETITE:
                GrantActorSpoil(ACTOR_SPOIL_MAGNETITE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GRANT_ACTOR_SECOND_ITEM:
                GrantActorReward(ACTOR_REWARD_SECOND_ITEM);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GRANT_ACTOR_FIRST_ITEM:
                GrantActorReward(ACTOR_REWARD_FIRST_ITEM);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GRANT_ACTOR_GEM:
                GrantActorReward(ACTOR_REWARD_GEM);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GRANT_ACTOR_EXPERIENCE:
                GrantActorSpoil(ACTOR_SPOIL_EXPERIENCE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GRANT_RANDOM_ACTOR_REWARD:
                GrantActorReward(ACTOR_REWARD_RANDOM);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GRANT_RANDOM_ACTOR_REWARD_B:
                GrantActorReward(ACTOR_REWARD_RANDOM_B);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ROLL_ACTOR_MAGNETITE:
                OpRollActorMagnetite();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ROLL_ACTOR_MACCA:
                OpRollActorMacca();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_LOAD_SPRITE:
                OpLoadSprite();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PLACE_SPRITE:
                OpPlaceSprite(0);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_HIDE_SPRITE:
                OpHideSprite();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PLACE_SPRITE_ALIAS:
                OpPlaceSprite(1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FADE_IN:
                OpFadeIn();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FADE_OUT:
                OpFadeOut();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_SET_RESULT_CLEAR:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_SET, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_SET_RESULT_SET:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_SET, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_CLEAR_RESULT_CLEAR:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_CLEAR, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_CLEAR_RESULT_SET:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_CLEAR, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_TOGGLE_RESULT_CLEAR:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TOGGLE, false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_TOGGLE_RESULT_SET:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TOGGLE, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_NOT_EQUAL_ZERO:
                OpJumpUnlessCompare(COMPARE_NOT_EQUAL, SCRIPT_COMPARE_WITH_ZERO);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_EQUAL_ZERO:
                OpJumpUnlessCompare(COMPARE_EQUAL, SCRIPT_COMPARE_WITH_ZERO);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_GREATER_EQUAL_ZERO:
                OpJumpUnlessCompare(COMPARE_GREATER_EQUAL, SCRIPT_COMPARE_WITH_ZERO);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_LESS_ZERO:
                OpJumpUnlessCompare(COMPARE_LESS, SCRIPT_COMPARE_WITH_ZERO);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_GREATER_ZERO:
                OpJumpUnlessCompare(COMPARE_GREATER, SCRIPT_COMPARE_WITH_ZERO);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_LESS_EQUAL_ZERO:
                OpJumpUnlessCompare(COMPARE_LESS_EQUAL, SCRIPT_COMPARE_WITH_ZERO);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_NOT_EQUAL_OPERAND:
                OpJumpUnlessCompare(COMPARE_NOT_EQUAL, SCRIPT_COMPARE_WITH_OPERAND);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_EQUAL_OPERAND:
                OpJumpUnlessCompare(COMPARE_EQUAL, SCRIPT_COMPARE_WITH_OPERAND);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_LESS_EQUAL_OPERAND:
                OpJumpUnlessCompare(COMPARE_LESS_EQUAL, SCRIPT_COMPARE_WITH_OPERAND);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_GREATER_EQUAL_OPERAND:
                OpJumpUnlessCompare(COMPARE_GREATER_EQUAL, SCRIPT_COMPARE_WITH_OPERAND);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_LESS_OPERAND:
                OpJumpUnlessCompare(COMPARE_LESS, SCRIPT_COMPARE_WITH_OPERAND);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_GREATER_OPERAND:
                OpJumpUnlessCompare(COMPARE_GREATER, SCRIPT_COMPARE_WITH_OPERAND);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_IN_VIEW_NORMAL:
                OpJumpUnlessPlayerInView(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_IN_VIEW_INVERTED:
                OpJumpUnlessPlayerInView(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_HP_PERCENT_ROLL_LESS_EQUAL:
                OpJumpUnlessHpPercentRoll(COMPARE_LESS_EQUAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_HP_PERCENT_ROLL_GREATER:
                OpJumpUnlessHpPercentRoll(COMPARE_GREATER);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_HP_QUARTER_ROLL_GREATER_EQUAL:
                OpJumpUnlessHpQuarterRoll(COMPARE_GREATER_EQUAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_HP_QUARTER_ROLL_LESS:
                OpJumpUnlessHpQuarterRoll(COMPARE_LESS);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ACTOR_VISIBLE_NORMAL:
                OpJumpUnlessActorVisible(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ACTOR_VISIBLE_INVERTED:
                OpJumpUnlessActorVisible(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_NEAR_FRONT_NORMAL:
                OpJumpUnlessPlayerNearFront(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_NEAR_FRONT_INVERTED:
                OpJumpUnlessPlayerNearFront(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_AT_RANGE_NORMAL:
                OpJumpUnlessPlayerAtRange(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_AT_RANGE_INVERTED:
                OpJumpUnlessPlayerAtRange(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ACTOR_CAN_STEP_AWAY_NORMAL:
                OpJumpUnlessActorCanStep(SCRIPT_TEST_NORMAL, MOVE_BACK);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ACTOR_CAN_STEP_AWAY_INVERTED:
                OpJumpUnlessActorCanStep(SCRIPT_TEST_INVERTED, MOVE_BACK);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_BLOCKED_BEHIND_NORMAL:
                OpIfBlockedToward(SCRIPT_TEST_NORMAL, MOVE_BACK);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_BLOCKED_BEHIND_INVERTED:
                OpIfBlockedToward(SCRIPT_TEST_INVERTED, MOVE_BACK);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_IN_ROSTER_NORMAL:
                OpJumpUnlessInRoster(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_IN_ROSTER_INVERTED:
                OpJumpUnlessInRoster(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ROSTER_FULL_NORMAL:
                OpJumpUnlessRosterFull(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ROSTER_FULL_INVERTED:
                OpJumpUnlessRosterFull(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ALIGNMENT_MATCH_NORMAL:
                OpJumpUnlessAlignmentMatch(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ALIGNMENT_MATCH_INVERTED:
                OpJumpUnlessAlignmentMatch(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CAN_AFFORD_NORMAL:
                OpJumpUnlessCanAfford(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_CAN_AFFORD_INVERTED:
                OpJumpUnlessCanAfford(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_IN_PARTY_NORMAL:
                OpJumpUnlessInParty(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_IN_PARTY_INVERTED:
                OpJumpUnlessInParty(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_HAS_ITEM_NORMAL:
                OpIfHasItem(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_HAS_ITEM_INVERTED:
                OpIfHasItem(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_HAS_ALL_ITEMS_NORMAL:
                OpIfHasAllItems(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_HAS_ALL_ITEMS_INVERTED:
                OpIfHasAllItems(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_HEALTHY_NORMAL:
                OpJumpUnlessHealthy(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_HEALTHY_INVERTED:
                OpJumpUnlessHealthy(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_COMPANION_HEALTHY_NORMAL:
                OpJumpUnlessCompanionHealthy(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_COMPANION_HEALTHY_INVERTED:
                OpJumpUnlessCompanionHealthy(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_HERO_EQUIPPED_NORMAL:
                OpJumpUnlessHeroEquipped(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_HERO_EQUIPPED_INVERTED:
                OpJumpUnlessHeroEquipped(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_NO_ACTOR_NORMAL:
                OpIfNoActor(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_NO_ACTOR_INVERTED:
                OpIfNoActor(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_BEGIN_CHOICES:
                OpBeginChoices(window);
                ReloadTextPeriod();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_NEXT_CHOICE:
                OpNextChoice(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_END_CHOICES:
                OpEndChoices(window);
                ReloadTextPeriod();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_MESSAGE_WINDOW:
                ClearMessageWindow(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SCROLL_WINDOW:
                OpScrollWindow(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_WINDOW_SCROLL_TOP:
                OpGetWindowScrollTop(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_SCROLL_TOP:
                OpSetWindowScrollTop(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_WINDOW_INDENT:
                OpGetWindowIndent(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_INDENT:
                OpSetWindowIndent(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_WINDOW_SCROLL_STEP:
                OpGetWindowScrollStep(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_SCROLL_STEP:
                OpSetWindowScrollStep(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_WINDOW_CURSOR:
                OpGetWindowCursor(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WINDOW_CURSOR:
                OpSetWindowCursor(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PLAY_SOUND_EFFECT:
                PlaySoundEffect(MapSoundEffectId(ReadScriptValue()));
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PLAY_MUSIC:
                PlayMusic(ReadScriptValue(), true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_INCREASE_HP:
            case SCRIPT_OP_INCREASE_HP_ALIAS:
                OpChangeHp(1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DECREASE_HP:
                OpChangeHp(-1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_INCREASE_MP:
            case SCRIPT_OP_INCREASE_MP_ALIAS:
                OpChangeMp(1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DECREASE_MP:
                OpChangeMp(-1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_APPLY_OBJECT_CONDITION:
                OpApplyObjectCondition();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_OBJECT_CONDITION:
            case SCRIPT_OP_CLEAR_OBJECT_CONDITION_ALIAS:
                OpClearObjectCondition();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SHIFT_PLAYER_ALIGNMENT_B:
                OpShiftPlayerAlignmentB();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SHIFT_PLAYER_ALIGNMENT_A:
                OpShiftPlayerAlignmentA();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SAVE_OBJECT_CONDITIONS:
                OpSaveObjectConditions();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_BEGIN_WINDOW_ALT_TEXT:
                BeginWindowAltText(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_END_WINDOW_ALT_TEXT:
                EndWindowAltText(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_BEGIN_WINDOW_INSTANT_TEXT:
                BeginWindowInstantText(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_END_WINDOW_INSTANT_TEXT:
                EndWindowInstantText(window);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_OBJECT_FAMILIARITY:
                OpSetObjectFamiliarity(SCRIPT_VALUE_AS_READ);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_OBJECT_FAMILIARITY_NEGATED:
                OpSetObjectFamiliarity(SCRIPT_VALUE_NEGATED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_ACTOR_FAMILIARITY:
                OpAddActorFamiliarity(SCRIPT_VALUE_AS_READ);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SUBTRACT_ACTOR_FAMILIARITY:
                OpAddActorFamiliarity(SCRIPT_VALUE_NEGATED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_FAMILIARITY_COUNT:
                OpAddFamiliarityCount(SCRIPT_VALUE_AS_READ);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SUBTRACT_FAMILIARITY_COUNT:
                OpAddFamiliarityCount(SCRIPT_VALUE_NEGATED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_ACTOR_LEVEL_GAP:
                OpAddActorLevelGap(SCRIPT_VALUE_AS_READ);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SUBTRACT_ACTOR_LEVEL_GAP:
                OpAddActorLevelGap(SCRIPT_VALUE_NEGATED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_ACTOR_FAMILIARITY:
                OpSetActorFamiliarity();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_ACTOR_LEVEL_GAP:
                OpSetActorLevelGap();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RESTART_AND_DESPAWN:
                RestartScript(0xdf, 1);
                DespawnScriptActor();
                return SCRIPT_YIELD;
            case SCRIPT_OP_RESTART_AND_STEP_ACTOR_BACK:
                RestartScript(0xdf, 0);
                StepScriptActor(MOVE_BACK);
                return SCRIPT_YIELD;
            case SCRIPT_OP_STEP_ACTOR_FORWARD:
                return StepScriptActor(MOVE_FORWARD);
            case SCRIPT_OP_STEP_ACTOR_BACK:
                return StepScriptActor(MOVE_BACK);
            case SCRIPT_OP_TRANSITION_FORWARD:
                return PlayScreenTransition(MOVE_FORWARD);
            case SCRIPT_OP_TRANSITION_BACK:
                return PlayScreenTransition(MOVE_BACK);
            case SCRIPT_OP_UNEQUIP_LEADER_GUN_AND_AMMO:
                UnequipPart(ROSTER_LEADER, EQUIP_PART_GUN);
                UnequipPart(ROSTER_LEADER, EQUIP_PART_AMMO);
                RecalcCharacterStats(GetRosterCharacter(ROSTER_LEADER));
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_LONG_VAR:
                OpSetLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWAP_LONG_VARS:
                OpSwapLongVars();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_COPY_LONG_VAR:
                OpCopyLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_UNSET_LONG_VAR:
                OpUnsetLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ZERO_LONG_VAR:
                OpZeroLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_NEG_LONG_VAR:
                OpNegLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_NOT_LONG_VAR:
                OpNotLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_INC_LONG_VAR:
                OpIncLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DEC_LONG_VAR:
                OpDecLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_INC_LONG_VAR_BELOW:
                OpIncLongVarBelow();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DEC_LONG_VAR_ABOVE:
                OpDecLongVarAbove();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLAMP_LONG_VAR:
                OpClampLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ROLL_LONG_VAR:
                OpRollLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RAND_LONG_VAR:
                OpRandLongVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_STORE_FRAME_LOCALS:
                StoreFrameLocals();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_LOAD_FRAME_LOCALS:
                LoadFrameLocals();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWAP_FRAME_LOCALS:
                SwapFrameLocals();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_TEXT_CHAR_DELAY:
                OpSetTextCharDelay();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ENABLE_TEXT_DELAY:
                EnableTextDelay();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DISABLE_TEXT_DELAY:
                DisableTextDelay();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_REPLACE_TEXT_CHAR_DELAY:
                OpReplaceTextCharDelay();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DISABLE_TEXT_DELAY_SKIP:
                DisableTextDelaySkip();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ENABLE_TEXT_DELAY_SKIP:
                EnableTextDelaySkip();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_TEXT_WAIT_FRAMES:
                OpSetTextWaitFrames();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DISABLE_TEXT_TIMED_WAIT:
                SetTextTimedWait(false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ENABLE_TEXT_TIMED_WAIT:
                SetTextTimedWait(true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ENABLE_TEXT_SCROLL:
                SetTextScrollMode(true);
                SetTextPeriod(window);
                ClearTextPeriod();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DISABLE_TEXT_SCROLL:
                SetTextScrollMode(false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CHANGE_MAP:
                OpChangeMap();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_WORLD_MAP_SPOT:
                OpSetWorldMapSpot();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PUSH_SCRIPT_WINDOW:
            case SCRIPT_OP_PUSH_SCRIPT_WINDOW_ALIAS:
                OpPushScriptWindow();
                return SCRIPT_YIELD;
            case SCRIPT_OP_POP_SCRIPT_WINDOW:
            case SCRIPT_OP_POP_SCRIPT_WINDOW_ALIAS:
                PopScriptWindow();
                return SCRIPT_YIELD;
            case SCRIPT_OP_START_TICK_COUNTER:
                OpStartTickCounter();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PAUSE_TICK_COUNTER:
                OpPauseTickCounter();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_STOP_TICK_COUNTER:
                OpStopTickCounter();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_TICK_COUNTER:
                OpGetTickCounter();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_TICK_COUNTER:
                OpSetTickCounter();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_WAIT_MESSAGE:
                return OpWaitMessage(window);
            case SCRIPT_OP_RUN_CHOICE_MENU:
                OpRunChoiceMenu(window);
                return SCRIPT_YIELD;
            case SCRIPT_OP_IF_FACING_NORMAL:
                OpIfFacing(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_FACING_INVERTED:
                OpIfFacing(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_RETURN_FACING_NORMAL:
                OpIfReturnFacing(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_RETURN_FACING_INVERTED:
                OpIfReturnFacing(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_OBJECT_HAS_CONDITION_NORMAL:
                OpIfObjectHasCondition(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_OBJECT_HAS_CONDITION_INVERTED:
                OpIfObjectHasCondition(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_BLOCKED_AHEAD_NORMAL:
                OpIfBlockedToward(SCRIPT_TEST_NORMAL, MOVE_FORWARD);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_BLOCKED_AHEAD_INVERTED:
                OpIfBlockedToward(SCRIPT_TEST_INVERTED, MOVE_FORWARD);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ACTOR_CAN_STEP_TOWARD_NORMAL:
                OpJumpUnlessActorCanStep(SCRIPT_TEST_NORMAL, MOVE_FORWARD);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ACTOR_CAN_STEP_TOWARD_INVERTED:
                OpJumpUnlessActorCanStep(SCRIPT_TEST_INVERTED, MOVE_FORWARD);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_BRANCH_ON_ITEMS_FIT_NORMAL:
                OpBranchOnItemsFit(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_BRANCH_ON_ITEMS_FIT_INVERTED:
                OpBranchOnItemsFit(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_QUEUE_AUTO_MOVES:
                OpQueueAutoMoves();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_PENDING_ITEM:
                OpAddPendingItem();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_REMOVE_PENDING_ITEM:
                OpRemovePendingItem();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GIVE_POOLED_ITEMS:
                GivePooledItems();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_TAKE_POOLED_ITEMS:
                TakePooledItems();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FIND_FIRST_MEMBER_BY_HP_STATE:
                OpFindMemberByPoolState(ROSTER_QUERY_FIRST_SLOT, POOL_MASK_HP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MASK_MEMBERS_BY_HP_STATE:
                OpFindMemberByPoolState(ROSTER_QUERY_SLOT_MASK, POOL_MASK_HP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FIND_FIRST_MEMBER_BY_MP_STATE:
                OpFindMemberByPoolState(ROSTER_QUERY_FIRST_SLOT, POOL_MASK_MP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MASK_MEMBERS_BY_MP_STATE:
                OpFindMemberByPoolState(ROSTER_QUERY_SLOT_MASK, POOL_MASK_MP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FIND_FIRST_MEMBER_WITH_CONDITION:
                OpFindMemberWithCondition(ROSTER_QUERY_FIRST_SLOT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MASK_MEMBERS_WITH_CONDITION:
                OpFindMemberWithCondition(ROSTER_QUERY_SLOT_MASK);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FIND_FIRST_MEMBER_BY_ALIGNMENT_A:
                OpFindMemberByAlignmentA(ROSTER_QUERY_FIRST_SLOT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MASK_MEMBERS_BY_ALIGNMENT_A:
                OpFindMemberByAlignmentA(ROSTER_QUERY_SLOT_MASK);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FIND_FIRST_MEMBER_BY_ALIGNMENT_B:
                OpFindMemberByAlignmentB(ROSTER_QUERY_FIRST_SLOT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MASK_MEMBERS_BY_ALIGNMENT_B:
                OpFindMemberByAlignmentB(ROSTER_QUERY_SLOT_MASK);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_COUNT_ITEM_OWNED:
                OpCountItemOwned();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ENABLE_CHOICE_CANCEL:
                SetWindowOption(MOUSE_CANCEL_ACCEPT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DISABLE_CHOICE_CANCEL:
                SetWindowOption(MOUSE_CANCEL_IGNORE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CALL_SUB_SCENE:
                return OpCallSubScene();
            case SCRIPT_OP_GET_ACTOR_MOON_VALUE:
                OpGetActorMoonValue();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_STORE_ACTOR_DISTANCE:
                OpStoreActorDistance();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PLACE_SCRIPT_ACTOR:
                PlaceScriptActor();
                return SCRIPT_END;
            case SCRIPT_OP_RETIRE_SCRIPT_ACTOR:
                RetireScriptActor();
                return SCRIPT_END;
            case SCRIPT_OP_IGNORE_TWO_WORDS:
                ReadScriptWord();
                ReadScriptWord();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_OBJECT_FIELD:
                OpSetObjectField();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IGNORE_WORD_BYTE_WORD:
                ReadScriptWord();
                ReadScriptByte();
                ReadScriptWord();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SQRT_LONG_VAR_EXPLICIT:
                OpSqrtLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SQRT_LONG_VAR_IN_PLACE:
                OpSqrtLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PRINT_ROSTER_NAME:
                OpPrintRosterName();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_SELECTED_OBJECT_ID:
                OpGetSelectedObjectId();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_NEVER_JUMP_0:
            case SCRIPT_OP_NEVER_JUMP_1:
            case SCRIPT_OP_NEVER_JUMP_2:
            case SCRIPT_OP_NEVER_JUMP_3:
            case SCRIPT_OP_NEVER_JUMP_4:
            case SCRIPT_OP_NEVER_JUMP_5:
            case SCRIPT_OP_NEVER_JUMP_6:
            case SCRIPT_OP_NEVER_JUMP_7:
            case SCRIPT_OP_NEVER_JUMP_8:
            case SCRIPT_OP_NEVER_JUMP_9:
            case SCRIPT_OP_NEVER_JUMP_10:
            case SCRIPT_OP_NEVER_JUMP_11:
                target = ReadBranchTarget();
                ScriptJumpUnless(target, true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ROSTER_HAS_NO_DEMONS_NORMAL:
                OpJumpUnlessRosterHasNoDemons(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_ROSTER_HAS_NO_DEMONS_INVERTED:
                OpJumpUnlessRosterHasNoDemons(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_OBJECT_IS_ALLY_NORMAL:
                OpIfObjectIsAlly(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_OBJECT_IS_ALLY_INVERTED:
                OpIfObjectIsAlly(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_STATUS_POSITIVE_NORMAL:
                OpIfStatusPositive(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_STATUS_POSITIVE_INVERTED:
                OpIfStatusPositive(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RECOVER_ROSTER_HP:
                OpRecoverRosterPool(POOL_MASK_HP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RECOVER_ROSTER_MP:
                OpRecoverRosterPool(POOL_MASK_MP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CURE_ROSTER_CONDITION:
                OpCureRosterCondition();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MASK_ROSTER_BY_KIND:
                OpMaskRosterByKind();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SHOW_BACKGROUND:
                OpShowBackground();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RESTORE_BACKGROUND:
                OpRestoreBackground();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_OPEN_SCRIPT_PANEL:
                OpOpenScriptPanel();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLOSE_LAST_SCRIPT_PANEL:
                CloseLastScriptPanel();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLOSE_ALL_SCRIPT_PANELS:
                CloseAllScriptPanels();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_PANEL_ENTRY_JUMP:
                OpSetPanelEntryJump();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DRAW_SCRIPT_PANELS:
                DrawScriptPanels();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SKIP_PANEL_OPERANDS:
                OpSkipPanelOperands();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_PANEL_ENTRY_VALUE:
                OpSetPanelEntryValue();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PUSH_GAME_STATE:
                OpPushGameState();
                return SCRIPT_YIELD;
            case SCRIPT_OP_LOAD_RECORD:
                OpLoadRecord();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_OPEN_ITEM_LIST_WINDOW:
                OpOpenItemListWindow();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLOSE_ITEM_LIST_WINDOW:
                OpCloseItemListWindow();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLOSE_SCRIPT_PANEL:
                OpCloseScriptPanel();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_REDRAW_ITEM_LIST_TOTAL:
                OpRedrawItemListTotal();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_POOL_HAS_ITEMS_NORMAL:
                OpIfPoolHasItems(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_POOL_HAS_ITEMS_INVERTED:
                OpIfPoolHasItems(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_BAG_HAS_ENTRIES_NORMAL:
                OpIfBagHasEntries(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_BAG_HAS_ENTRIES_INVERTED:
                OpIfBagHasEntries(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FUSION_MENU_PAIR_FIRST:
                OpOpenFusionScreen(FUSION_MENU_PAIR_FIRST);
                return SCRIPT_YIELD;
            case SCRIPT_OP_FUSION_MENU_PAIR_SECOND:
                OpOpenFusionScreen(FUSION_MENU_PAIR_SECOND);
                return SCRIPT_YIELD;
            case SCRIPT_OP_FUSION_MENU_TRIPLE_THIRD:
                OpOpenFusionScreen(FUSION_MENU_TRIPLE_THIRD);
                return SCRIPT_YIELD;
            case SCRIPT_OP_FUSION_MENU_TRIPLE_FIRST:
                OpOpenFusionScreen(FUSION_MENU_TRIPLE_FIRST);
                return SCRIPT_YIELD;
            case SCRIPT_OP_FUSION_MENU_TRIPLE_SECOND:
                OpOpenFusionScreen(FUSION_MENU_TRIPLE_SECOND);
                return SCRIPT_YIELD;
            case SCRIPT_OP_RUN_FUSION_PAIR:
                OpRunFusion(false);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RUN_FUSION_TRIPLE:
                OpRunFusion(true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_END_FUSION:
                OpEndFusion();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FUSION_MENU_PAIR_COMMIT:
                OpOpenFusionScreen(FUSION_MENU_PAIR_COMMIT);
                return SCRIPT_YIELD;
            case SCRIPT_OP_FUSION_MENU_TRIPLE_COMMIT:
                OpOpenFusionScreen(FUSION_MENU_TRIPLE_COMMIT);
                return SCRIPT_YIELD;
            case SCRIPT_OP_OPEN_STATUS:
                PushGameState(GAME_STATE_STATUS);
                return SCRIPT_YIELD;
            case SCRIPT_OP_SET_SCENE_RENDER_MODE:
                SetSceneRenderMode();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IGNORE_VALUE:
            case SCRIPT_OP_IGNORE_VALUE_ALIAS_1:
            case SCRIPT_OP_IGNORE_VALUE_ALIAS_2:
                ReadScriptValue();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_BLANK_RENDER_MODE:
            case SCRIPT_OP_SET_BLANK_RENDER_MODE_ALIAS:
                SetBlankRenderMode();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_IF:
                OpJumpIf(true);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SKIP_JUMP_TARGET:
                OpSkipJumpTarget(1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_AT_MOST_ONE_DEMON:
                OpIfDemonCount(SCRIPT_TEST_INVERTED, 1);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_AT_MOST_TWO_DEMONS:
                OpIfDemonCount(SCRIPT_TEST_INVERTED, 2);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CREATE_SCRIPT_MENU:
                OpCreateScriptMenu();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RUN_SCRIPT_MENU:
                OpRunScriptMenu();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DESTROY_SCRIPT_MENU:
                OpDestroyScriptMenu();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_MENU_LINE:
                OpAddMenuLine();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_TEXT_BUFFERS:
                ClearTextBuffers();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FORMAT_NUMBER:
                OpFormatNumber();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FORMAT_CAPTURED_TEXT:
                OpFormatCapturedText();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RETURN_FROM_CALL:
                ReturnFromCall();
            case SCRIPT_OP_END:
            case SCRIPT_OP_END_ALIAS_1:
            case SCRIPT_OP_END_ALIAS_2:
                return SCRIPT_END;
            case SCRIPT_OP_GET_MENU_TAG:
                OpGetMenuTag();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ALLOC_LONG_ARRAY:
                OpAllocLongArray();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FREE_LONG_ARRAY:
                OpFreeLongArray();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_LONG_ARRAY_ITEM:
                OpGetLongArrayItem();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_LONG_ARRAY_ITEM:
                OpSetLongArrayItem();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CONVERT_CHARACTER_REF:
                OpConvertCharacterRef();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SELECT_PARTY_SLOT:
                OpSelectPartySlot();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_END_PARTY_SLOT_SELECT:
                OpEndPartySlotSelect();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_SET:
                OpJumpUnlessFlagSet();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_LOAD_DATA_FILE:
                OpLoadDataFile();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FREE_DATA_FILE:
                OpFreeDataFile();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_READ_RECORD_INT:
                OpReadRecordInt();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_READ_DATA_INT:
                OpReadDataInt();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SAVE_SCENE_CELL:
                OpSaveSceneCell();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MODIFY_EVENT_FLAG_BY_VALUE:
                OpModifyEventFlagByValue();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_TEST_EVENT_FLAG_BY_VALUE:
                OpTestEventFlagByValue();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_VALUE_JUMP:
                OpSwitchOnValue(SCRIPT_BRANCH_JUMP);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SWITCH_ON_VALUE_CALL:
                OpSwitchOnValue(SCRIPT_BRANCH_CALL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_PLAYER_LOCATION:
                OpGetPlayerLocation();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_PLAYER_POSITION:
                OpSetPlayerPosition();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_HOVERED_OBJECT_ID:
                OpGetHoveredObjectId();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_MENU_LINE_COLOR:
                OpSetMenuLineColor();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_MENU_HIGHLIGHT:
                OpClearMenuHighlight();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_MENU_CURSOR:
                OpGetMenuCursor();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_MEMBER_HAS_CONDITION:
                OpIfMemberHasCondition();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_REFRESH_FIELD_SCREEN:
                OpRefreshFieldScreen();
                return SCRIPT_YIELD;
            case SCRIPT_OP_FADE_OUT_AND_CLEAR:
                OpFadeOutAndClear();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_RESET_MASK:
                OpResetMask();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DRAW_IMAGE:
                OpDrawImage();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ENTER_FIELD_MAP:
                OpEnterFieldMap();
                return SCRIPT_YIELD;
            case SCRIPT_OP_ADVANCE_CLOCK:
                OpAdvanceClock();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_TICKS_UNTIL_MOON_PHASE:
                OpGetTicksUntilMoonPhase();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_DAY_COUNT:
                OpGetDayCount();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_TIME_OF_DAY:
                OpGetTimeOfDay();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MOD_LONG_VAR_EXPLICIT:
                OpModLongVar(LONG_OPERAND_EXPLICIT);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MOD_LONG_VAR_IN_PLACE:
                OpModLongVar(LONG_OPERAND_IN_PLACE);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_OBJECT_PRESENCE:
                OpSetObjectPresence();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SAVE_DATA_COMMAND:
                OpSaveDataCommand();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_FILL_SCREEN_CELLS:
                OpFillScreenCells();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MASK_SCREEN_CELLS:
                OpMaskScreenCells();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ENABLE_BACKGROUND:
                OpEnableBackground();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_MESSAGE_HOOK:
                OpSetMessageHook();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SKIP_VALUE_AND_VAR:
                OpSkipValueAndVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_ITEM_PRICE:
                OpGetItemPrice();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_STEP_LIST_MENU:
                OpStepListMenu();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_BATTLE_RESULT:
                OpIfBattleResult();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_EVENT_OBJECT_IS:
                OpIfEventObjectIs();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_COUNT_OBJECTS_AT:
                OpCountObjectsAt();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_BOOST_POOL:
                OpBoostPool();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_DISMISS_TALK_TARGET:
                DismissTalkTarget();
                return SCRIPT_YIELD;
            case SCRIPT_OP_GET_COMBATANT_ID:
                OpGetCombatantId();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CAPTURE_DATA_STRING:
                OpCaptureDataString();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CAPTURE_RECORD_STRING:
                OpCaptureRecordString();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_REQUEST_QUIT:
                RequestQuit();
                return SCRIPT_YIELD;
            case SCRIPT_OP_COUNT_ACTIVE_PARTY:
                OpCountActiveParty();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PEEK_POKE_SCRATCH:
                OpPeekPokeScratch();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_POLL_MOUSE_CLICK:
                OpPollMouseClick();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_REDRAW_SCRIPT_MENU:
                OpRedrawScriptMenu();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SHOW_PICTURE:
                OpShowPicture();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SAVE_RESTORE_SCREEN:
                OpSaveRestoreScreen();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_REBALANCE_MEMBER_STATS:
                OpRebalanceMemberStats();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_STORE_SCRIPT_VAR:
                OpStoreScriptVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_LOAD_SCRIPT_VAR:
                OpLoadScriptVar();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_ROUTE_POINT:
                OpAddRoutePoint();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_IF_IN_BATTLE:
                OpIfInBattle();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_LAST_PANEL_FLAG:
                OpSetLastPanelFlag();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_FUSION_RESULT:
                OpGetFusionResult();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_CAPTURED_TEXT:
                ClearCapturedText();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_LEVEL_UP_MEMBER:
                OpLevelUpMember();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_STASH_ITEM_LISTS:
                OpStashItemLists();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_BATTLE_OUTCOME:
                OpGetBattleOutcome();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_START_COUNTDOWN:
                OpStartCountdown();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CALL_TEXT_SCRIPT:
                OpCallTextScript();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SHOW_EVENT_PICTURE:
                OpShowEventPicture();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_FIELD_OPTION:
                OpSetFieldOption();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_FIELD_PARAMS:
                OpSetFieldParams();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_MENU_CHARACTER:
                OpSetMenuCharacter();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SET_MENU_SCROLL:
                OpSetMenuScroll();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_PLAY_ANIMATION:
                OpPlayAnimation();
            case SCRIPT_OP_YIELD:
                return SCRIPT_YIELD;
            case SCRIPT_OP_ADJUST_ITEM_COUNT:
                OpAdjustItemCount();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_LIST_BAG_BY_CATEGORY:
                OpListBagByCategory();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_GET_BAG_ENTRY:
                OpGetBagEntry();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_CLEAR_BAG_ENTRY:
                OpClearBagEntry();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_TAKE_DROP_SLOT:
                OpTakeDropSlot();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_MODIFY_EVENT_FLAG:
                OpModifyEventFlag();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_TEST_EVENT_FLAG:
                OpTestEventFlag();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_STACK_MESSAGE_WINDOW:
                OpStackMessageWindow();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_IN_LINE_NORMAL:
                OpJumpUnlessPlayerInLine(SCRIPT_TEST_NORMAL);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_IN_LINE_INVERTED:
                OpJumpUnlessPlayerInLine(SCRIPT_TEST_INVERTED);
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_SCREEN_TRANSITION:
                return OpScreenTransition();
            case SCRIPT_OP_SWAP_SCREEN_STATE:
                OpSwapScreenState();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_COPY_ITEM_RECORD:
                OpCopyItemRecord();
                return SCRIPT_CONTINUE;
            case SCRIPT_OP_ADD_MEMBER_SKILL:
                OpAddMemberSkill();
            default:
                return SCRIPT_CONTINUE;
        }
    }
}
