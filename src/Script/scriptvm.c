// @identity-TODO: the owning TU is unproven; this unit holds the dispatcher's
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/GameState.h>
#include <Game/ItemPool.h>
#include <Game/StateStack.h>
#include <Gfx/Render.h>
#include <Script/BranchMode.h>
#include <Script/EventFlags.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptSprite.h>
#include <Script/ScriptText.h>
#include <Script/ScriptVars.h>
#include <Script/ScriptVm.h>
#include <Script/TextState.h>
#include <Sound/Sound.h>
#include <Text/TextWindow.h>

DATA(0x00081228)
GZ_ENUM_STORAGE(ScriptOpcode, u16) g_scriptOpcode = 0;

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
i16 ExecScriptOpcode(i16 window, GZ_ENUM_PARAM(ScriptOpcode, u16) op) {
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
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                entry = ReadScriptByte();
                CallScript(op + 0x7eff, entry);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_TEST_RESULT_CLEAR:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TEST, 0);
                return 0;
            case SCRIPT_OP_ADVANCE_WINDOW_LINE:
                AdvanceWindowLine(window);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_TEST_RESULT_SET:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TEST, 1);
                return 0;
            case SCRIPT_OP_GOTO_SCRIPT:
                OpJumpScript(SCRIPT_BRANCH_JUMP);
                return 0;
            case SCRIPT_OP_CALL_SCRIPT:
                OpJumpScript(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_SWITCH_ON_RANDOM_JUMP:
                OpSwitchOnRandom(SCRIPT_BRANCH_JUMP);
                return 0;
            case SCRIPT_OP_SWITCH_ON_SELECTION_JUMP:
                OpSwitchOnSelection(SCRIPT_BRANCH_JUMP);
                return 0;
            case 16:
                OpJumpUnlessStatContest(0, SCRIPT_TEST_NORMAL, 0);
                return 0;
            case 17:
                OpJumpUnlessStatContest(0, SCRIPT_TEST_INVERTED, 0);
                return 0;
            case 18:
                OpJumpUnlessStatContest(1, SCRIPT_TEST_NORMAL, 0);
                return 0;
            case 19:
                OpJumpUnlessStatContest(1, SCRIPT_TEST_INVERTED, 0);
                return 0;
            case 20:
                OpJumpUnlessStatContest(2, SCRIPT_TEST_NORMAL, 0);
                return 0;
            case 21:
                OpJumpUnlessStatContest(2, SCRIPT_TEST_INVERTED, 0);
                return 0;
            case 22:
                OpJumpUnlessStatContest(3, SCRIPT_TEST_NORMAL, 0);
                return 0;
            case 23:
                OpJumpUnlessStatContest(3, SCRIPT_TEST_INVERTED, 0);
                return 0;
            case SCRIPT_OP_JUMP:
                OpJump();
                return 0;
            case SCRIPT_OP_SET_EVENT_FLAG:
                OpApplyEventFlag(SCRIPT_FLAG_SET, 1);
                return 0;
            case SCRIPT_OP_CLEAR_EVENT_FLAG:
                OpApplyEventFlag(SCRIPT_FLAG_CLEAR, 0);
                return 0;
            case SCRIPT_OP_BEGIN_TEXT_CAPTURE:
                SetTextCapture(1);
                return 0;
            case SCRIPT_OP_END_TEXT_CAPTURE:
                SetTextCapture(0);
                return 0;
            case SCRIPT_OP_PRINT_OPERAND_TEXT:
                OpPrintOperandText();
                return 0;
            case SCRIPT_OP_PRINT_NUMBER:
                OpPrintNumber();
                return 0;
            case SCRIPT_OP_IF_ANY_FLAGS:
                OpIfFlags(0);
                return 0;
            case SCRIPT_OP_IF_ALL_FLAGS:
                OpIfFlags(1);
                return 0;
            case 261:
                OpJumpUnlessStatContest(0, 0, 1);
                return 0;
            case 262:
                OpJumpUnlessStatContest(0, 1, 1);
                return 0;
            case 263:
                OpJumpUnlessStatContest(1, 0, 1);
                return 0;
            case 264:
                OpJumpUnlessStatContest(1, 1, 1);
                return 0;
            case 265:
                OpJumpUnlessStatContest(2, 0, 1);
                return 0;
            case 266:
                OpJumpUnlessStatContest(2, 1, 1);
                return 0;
            case 267:
                OpJumpUnlessStatContest(3, 0, 1);
                return 0;
            case 268:
                OpJumpUnlessStatContest(3, 1, 1);
                return 0;
            case SCRIPT_OP_ALERT_ACTOR:
                return OpSetActorAlert(0);
            case SCRIPT_OP_ALERT_ACTOR_IMMEDIATELY:
                return OpSetActorAlert(1);
            case SCRIPT_OP_DELAY_ACTOR:
                return OpSetActorAlert(2);
            case SCRIPT_OP_SWITCH_ON_RANDOM_CALL:
                OpSwitchOnRandom(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_SWITCH_ON_SELECTION_CALL:
                OpSwitchOnSelection(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_SWITCH_ON_ALIGNMENT_A_JUMP:
                OpSwitchOnAlignmentA(SCRIPT_BRANCH_JUMP);
                return 0;
            case SCRIPT_OP_SWITCH_ON_ALIGNMENT_A_CALL:
                OpSwitchOnAlignmentA(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_SWITCH_ON_ALIGNMENT_B_JUMP:
                OpSwitchOnAlignmentB(SCRIPT_BRANCH_JUMP);
                return 0;
            case SCRIPT_OP_SWITCH_ON_ALIGNMENT_B_CALL:
                OpSwitchOnAlignmentB(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_SWITCH_ON_MOON_PHASE_JUMP:
                OpSwitchOnMoonPhase(SCRIPT_BRANCH_JUMP);
                return 0;
            case SCRIPT_OP_SWITCH_ON_MOON_PHASE_CALL:
                OpSwitchOnMoonPhase(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_SWITCH_ON_RANGE_JUMP:
                OpSwitchOnRange(SCRIPT_BRANCH_JUMP);
                return 0;
            case SCRIPT_OP_SWITCH_ON_RANGE_CALL:
                OpSwitchOnRange(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_SWITCH_ON_ACTOR_ATTR_A_JUMP:
                OpSwitchOnActorAttrA(SCRIPT_BRANCH_JUMP);
                return 0;
            case SCRIPT_OP_SWITCH_ON_ACTOR_ATTR_A_CALL:
                OpSwitchOnActorAttrA(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_SWITCH_ON_ACTOR_ATTR_B_JUMP:
                OpSwitchOnActorAttrB(SCRIPT_BRANCH_JUMP);
                return 0;
            case SCRIPT_OP_SWITCH_ON_ACTOR_ATTR_B_CALL:
                OpSwitchOnActorAttrB(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_SET_ACTOR_ATTITUDE:
                OpSetActorAttitude();
                return 0;
            case SCRIPT_OP_SET_ACTOR_FIELD_STATE:
                OpSetActorFieldState();
                return 0;
            case 289:
                return SetActorMode(1);
            case 290:
                return SetActorMode(2);
            case 291:
                return SetActorMode(3);
            case 292:
                return SetActorMode(4);
            case 293:
                return SetActorMode(5);
            case 294:
                return SetActorMode(6);
            case 295:
                return SetActorMode(7);
            case 296:
                return SetActorMode(8);
            case 297:
                return SetActorMode(9);
            case 298:
                return SetActorMode(10);
            case 299:
                return SetActorMode(11);
            case SCRIPT_OP_RESET_AND_REVERSE_WINDOW_ATTR:
                SetWindowReverse(window, 1);
                return 0;
            case SCRIPT_OP_RESET_WINDOW_ATTR:
                SetWindowReverse(window, 0);
                return 0;
            case SCRIPT_OP_REVERSE_WINDOW_ATTR:
                SetWindowReverse(window, -1);
                return 0;
            case SCRIPT_OP_SET_WINDOW_GLYPH_COLOR:
                OpSetWindowColor(window, 0);
                return 0;
            case SCRIPT_OP_SET_WINDOW_BACKGROUND_COLOR:
                OpSetWindowColor(window, 2);
                return 0;
            case SCRIPT_OP_SET_WINDOW_DIM_COLOR:
                OpSetWindowColor(window, 1);
                return 0;
            case SCRIPT_OP_ENABLE_WINDOW_OPAQUE_BACKGROUND:
                SetWindowOpaqueBg(window, 1);
                return 0;
            case SCRIPT_OP_DISABLE_WINDOW_OPAQUE_BACKGROUND:
                SetWindowOpaqueBg(window, 0);
                return 0;
            case 308:
                SetWindowAttrFlag1(window, 1);
                return 0;
            case 309:
                SetWindowAttrFlag1(window, 0);
                return 0;
            case SCRIPT_OP_DISABLE_WINDOW_HALF_WIDTH:
                SetWindowHalfWidth(window, 0);
                return 0;
            case SCRIPT_OP_ENABLE_WINDOW_HALF_WIDTH:
                SetWindowHalfWidth(window, 1);
                return 0;
            case 312:
                SetWindowAttrFlag2(window, 0);
                return 0;
            case 313:
                SetWindowAttrFlag2(window, 1);
                return 0;
            case SCRIPT_OP_SAVE_WINDOW_COLOR:
                StashWindowColor(window, 1);
                return 0;
            case SCRIPT_OP_RESTORE_WINDOW_COLOR:
                StashWindowColor(window, 0);
                return 0;
            case SCRIPT_OP_SET_WINDOW_ALT_COLOR:
                OpSetWindowAltColor(window);
                return 0;
            case SCRIPT_OP_SET_WINDOW_INSTANT_COLOR:
                OpSetWindowInstantColor(window);
                return 0;
            case SCRIPT_OP_ADD_TO_ROSTER:
                return OpAddToRoster();
            case SCRIPT_OP_REMOVE_FROM_ROSTER:
                OpRemoveFromRoster();
                return 0;
            case SCRIPT_OP_JOIN_ACTIVE_PARTY:
                OpJoinActiveParty();
                return 0;
            case SCRIPT_OP_LEAVE_ACTIVE_PARTY:
                OpLeaveActiveParty();
                return 0;
            case SCRIPT_OP_MUL_LONG_VAR_EXPLICIT:
                OpMulLongVar(0);
                return 0;
            case SCRIPT_OP_DIV_LONG_VAR_EXPLICIT:
                OpDivLongVar(0);
                return 0;
            case SCRIPT_OP_ADD_LONG_VAR_EXPLICIT:
                OpAddLongVar(0);
                return 0;
            case SCRIPT_OP_SUB_LONG_VAR_EXPLICIT:
                OpSubLongVar(0);
                return 0;
            case SCRIPT_OP_AND_LONG_VAR_EXPLICIT:
                OpAndLongVar(0);
                return 0;
            case SCRIPT_OP_OR_LONG_VAR_EXPLICIT:
                OpOrLongVar(0);
                return 0;
            case SCRIPT_OP_XOR_LONG_VAR_EXPLICIT:
                OpXorLongVar(0);
                return 0;
            case SCRIPT_OP_SHL_LONG_VAR_EXPLICIT:
                OpShlLongVar(0);
                return 0;
            case SCRIPT_OP_SAR_LONG_VAR_EXPLICIT:
                OpSarLongVar(0);
                return 0;
            case SCRIPT_OP_PERCENT_LONG_VAR_EXPLICIT:
                OpPercentLongVar(0);
                return 0;
            case SCRIPT_OP_MUL_LONG_VAR_IN_PLACE:
                OpMulLongVar(1);
                return 0;
            case SCRIPT_OP_DIV_LONG_VAR_IN_PLACE:
                OpDivLongVar(1);
                return 0;
            case SCRIPT_OP_ADD_LONG_VAR_IN_PLACE:
                OpAddLongVar(1);
                return 0;
            case SCRIPT_OP_SUB_LONG_VAR_IN_PLACE:
                OpSubLongVar(1);
                return 0;
            case SCRIPT_OP_AND_LONG_VAR_IN_PLACE:
                OpAndLongVar(1);
                return 0;
            case SCRIPT_OP_OR_LONG_VAR_IN_PLACE:
                OpOrLongVar(1);
                return 0;
            case SCRIPT_OP_XOR_LONG_VAR_IN_PLACE:
                OpXorLongVar(1);
                return 0;
            case SCRIPT_OP_SHL_LONG_VAR_IN_PLACE:
                OpShlLongVar(1);
                return 0;
            case SCRIPT_OP_SAR_LONG_VAR_IN_PLACE:
                OpSarLongVar(1);
                return 0;
            case SCRIPT_OP_PERCENT_LONG_VAR_IN_PLACE:
                OpPercentLongVar(1);
                return 0;
            case SCRIPT_OP_TOGGLE_EVENT_FLAG:
                OpApplyEventFlag(SCRIPT_FLAG_TOGGLE, 1);
                return 0;
            case SCRIPT_OP_PUSH_RETURN_TARGET:
                OpPushReturnTarget();
                return 0;
            case SCRIPT_OP_DROP_CALL_FRAME:
                DropCallFrame();
                return 0;
            case SCRIPT_OP_SWAP_CALL_FRAMES:
                SwapCallFrames();
                return 0;
            case SCRIPT_OP_CLEAR_CALL_STACK:
                ClearCallStack();
                return 0;
            case SCRIPT_OP_ADD_MACCA:
                OpAddMacca(1);
                return 0;
            case SCRIPT_OP_SUBTRACT_MACCA:
                OpAddMacca(-1);
                return 0;
            case SCRIPT_OP_ADD_MAGNETITE:
                OpAddMagnetite(1);
                return 0;
            case SCRIPT_OP_SUBTRACT_MAGNETITE:
                OpAddMagnetite(-1);
                return 0;
            case SCRIPT_OP_GIVE_ITEM:
                OpGiveItem();
                return 0;
            case SCRIPT_OP_TAKE_ITEM:
                OpTakeItem();
                return 0;
            case 355:
            case 356:
                GrantActorReward(3);
                return 0;
            case SCRIPT_OP_GRANT_ACTOR_MACCA:
                GrantActorSpoil(0);
                return 0;
            case SCRIPT_OP_GRANT_ACTOR_MAGNETITE:
                GrantActorSpoil(1);
                return 0;
            case 359:
                GrantActorReward(1);
                return 0;
            case 360:
                GrantActorReward(0);
                return 0;
            case SCRIPT_OP_GRANT_ACTOR_GEM:
                GrantActorReward(2);
                return 0;
            case SCRIPT_OP_GRANT_ACTOR_EXPERIENCE:
                GrantActorSpoil(2);
                return 0;
            case SCRIPT_OP_GRANT_RANDOM_ACTOR_REWARD:
                GrantActorReward(7);
                return 0;
            case 364:
                GrantActorReward(8);
                return 0;
            case SCRIPT_OP_ROLL_ACTOR_MAGNETITE:
                OpRollActorMagnetite();
                return 0;
            case SCRIPT_OP_ROLL_ACTOR_MACCA:
                OpRollActorMacca();
                return 0;
            case SCRIPT_OP_LOAD_SPRITE:
                OpLoadSprite();
                return 0;
            case SCRIPT_OP_PLACE_SPRITE:
                OpPlaceSprite(0);
                return 0;
            case SCRIPT_OP_HIDE_SPRITE:
                OpHideSprite();
                return 0;
            case SCRIPT_OP_PLACE_SPRITE_ALIAS:
                OpPlaceSprite(1);
                return 0;
            case SCRIPT_OP_FADE_IN:
                OpFadeIn();
                return 0;
            case SCRIPT_OP_FADE_OUT:
                OpFadeOut();
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_SET_RESULT_CLEAR:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_SET, 0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_SET_RESULT_SET:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_SET, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_CLEAR_RESULT_CLEAR:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_CLEAR, 0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_CLEAR_RESULT_SET:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_CLEAR, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_TOGGLE_RESULT_CLEAR:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TOGGLE, 0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_TOGGLE_RESULT_SET:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TOGGLE, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_NOT_EQUAL_ZERO:
                OpJumpUnlessCompare(COMPARE_NOT_EQUAL, 0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_EQUAL_ZERO:
                OpJumpUnlessCompare(COMPARE_EQUAL, 0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_GREATER_EQUAL_ZERO:
                OpJumpUnlessCompare(COMPARE_GREATER_EQUAL, 0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_LESS_ZERO:
                OpJumpUnlessCompare(COMPARE_LESS, 0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_GREATER_ZERO:
                OpJumpUnlessCompare(COMPARE_GREATER, 0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_LESS_EQUAL_ZERO:
                OpJumpUnlessCompare(COMPARE_LESS_EQUAL, 0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_NOT_EQUAL_OPERAND:
                OpJumpUnlessCompare(COMPARE_NOT_EQUAL, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_EQUAL_OPERAND:
                OpJumpUnlessCompare(COMPARE_EQUAL, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_LESS_EQUAL_OPERAND:
                OpJumpUnlessCompare(COMPARE_LESS_EQUAL, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_GREATER_EQUAL_OPERAND:
                OpJumpUnlessCompare(COMPARE_GREATER_EQUAL, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_LESS_OPERAND:
                OpJumpUnlessCompare(COMPARE_LESS, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_GREATER_OPERAND:
                OpJumpUnlessCompare(COMPARE_GREATER, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_IN_VIEW_NORMAL:
                OpJumpUnlessPlayerInView(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_IN_VIEW_INVERTED:
                OpJumpUnlessPlayerInView(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_HP_PERCENT_ROLL_LESS_EQUAL:
                OpJumpUnlessHpPercentRoll(COMPARE_LESS_EQUAL);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_HP_PERCENT_ROLL_GREATER:
                OpJumpUnlessHpPercentRoll(COMPARE_GREATER);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_HP_QUARTER_ROLL_GREATER_EQUAL:
                OpJumpUnlessHpQuarterRoll(COMPARE_GREATER_EQUAL);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_HP_QUARTER_ROLL_LESS:
                OpJumpUnlessHpQuarterRoll(COMPARE_LESS);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_ACTOR_VISIBLE_NORMAL:
                OpJumpUnlessActorVisible(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_ACTOR_VISIBLE_INVERTED:
                OpJumpUnlessActorVisible(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_NEAR_FRONT_NORMAL:
                OpJumpUnlessPlayerNearFront(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_NEAR_FRONT_INVERTED:
                OpJumpUnlessPlayerNearFront(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_AT_RANGE_NORMAL:
                OpJumpUnlessPlayerAtRange(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_AT_RANGE_INVERTED:
                OpJumpUnlessPlayerAtRange(1);
                return 0;
            case 407:
                OpJumpUnlessActorCanStep(0, 2);
                return 0;
            case 408:
                OpJumpUnlessActorCanStep(1, 2);
                return 0;
            case 409:
                OpIfBlockedToward(0, 2);
                return 0;
            case 410:
                OpIfBlockedToward(1, 2);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_IN_ROSTER_NORMAL:
                OpJumpUnlessInRoster(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_IN_ROSTER_INVERTED:
                OpJumpUnlessInRoster(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_ROSTER_FULL_NORMAL:
                OpJumpUnlessRosterFull(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_ROSTER_FULL_INVERTED:
                OpJumpUnlessRosterFull(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_ALIGNMENT_MATCH_NORMAL:
                OpJumpUnlessAlignmentMatch(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_ALIGNMENT_MATCH_INVERTED:
                OpJumpUnlessAlignmentMatch(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_CAN_AFFORD_NORMAL:
                OpJumpUnlessCanAfford(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_CAN_AFFORD_INVERTED:
                OpJumpUnlessCanAfford(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_IN_PARTY_NORMAL:
                OpJumpUnlessInParty(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_IN_PARTY_INVERTED:
                OpJumpUnlessInParty(1);
                return 0;
            case SCRIPT_OP_IF_HAS_ITEM_NORMAL:
                OpIfHasItem(0);
                return 0;
            case SCRIPT_OP_IF_HAS_ITEM_INVERTED:
                OpIfHasItem(1);
                return 0;
            case SCRIPT_OP_IF_HAS_ALL_ITEMS_NORMAL:
                OpIfHasAllItems(0);
                return 0;
            case SCRIPT_OP_IF_HAS_ALL_ITEMS_INVERTED:
                OpIfHasAllItems(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_HEALTHY_NORMAL:
                OpJumpUnlessHealthy(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_HEALTHY_INVERTED:
                OpJumpUnlessHealthy(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_COMPANION_HEALTHY_NORMAL:
                OpJumpUnlessCompanionHealthy(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_COMPANION_HEALTHY_INVERTED:
                OpJumpUnlessCompanionHealthy(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_HERO_EQUIPPED_NORMAL:
                OpJumpUnlessHeroEquipped(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_HERO_EQUIPPED_INVERTED:
                OpJumpUnlessHeroEquipped(1);
                return 0;
            case SCRIPT_OP_IF_NO_ACTOR_NORMAL:
                OpIfNoActor(0);
                return 0;
            case SCRIPT_OP_IF_NO_ACTOR_INVERTED:
                OpIfNoActor(1);
                return 0;
            case SCRIPT_OP_BEGIN_CHOICES:
                OpBeginChoices(window);
                ReloadTextPeriod();
                return 0;
            case SCRIPT_OP_NEXT_CHOICE:
                OpNextChoice(window);
                return 0;
            case SCRIPT_OP_END_CHOICES:
                OpEndChoices(window);
                ReloadTextPeriod();
                return 0;
            case SCRIPT_OP_CLEAR_MESSAGE_WINDOW:
                ClearMessageWindow(window);
                return 0;
            case SCRIPT_OP_SCROLL_WINDOW:
                OpScrollWindow(window);
                return 0;
            case SCRIPT_OP_GET_WINDOW_SCROLL_TOP:
                OpGetWindowScrollTop(window);
                return 0;
            case SCRIPT_OP_SET_WINDOW_SCROLL_TOP:
                OpSetWindowScrollTop(window);
                return 0;
            case SCRIPT_OP_GET_WINDOW_INDENT:
                OpGetWindowIndent(window);
                return 0;
            case SCRIPT_OP_SET_WINDOW_INDENT:
                OpSetWindowIndent(window);
                return 0;
            case SCRIPT_OP_GET_WINDOW_SCROLL_STEP:
                OpGetWindowScrollStep(window);
                return 0;
            case SCRIPT_OP_SET_WINDOW_SCROLL_STEP:
                OpSetWindowScrollStep(window);
                return 0;
            case SCRIPT_OP_GET_WINDOW_CURSOR:
                OpGetWindowCursor(window);
                return 0;
            case SCRIPT_OP_SET_WINDOW_CURSOR:
                OpSetWindowCursor(window);
                return 0;
            case SCRIPT_OP_PLAY_SOUND_EFFECT:
                PlaySoundEffect(MapSoundEffectId(ReadScriptValue()));
                return 0;
            case SCRIPT_OP_PLAY_MUSIC:
                PlayMusic(ReadScriptValue(), 1);
                return 0;
            case 455:
            case 576:
                OpChangeHp(1);
                return 0;
            case SCRIPT_OP_DECREASE_HP:
                OpChangeHp(-1);
                return 0;
            case 457:
            case 577:
                OpChangeMp(1);
                return 0;
            case SCRIPT_OP_DECREASE_MP:
                OpChangeMp(-1);
                return 0;
            case SCRIPT_OP_APPLY_OBJECT_CONDITION:
                OpApplyObjectCondition();
                return 0;
            case 460:
            case 578:
                OpClearObjectCondition();
                return 0;
            case SCRIPT_OP_SHIFT_PLAYER_ALIGNMENT_B:
                OpShiftPlayerAlignmentB();
                return 0;
            case SCRIPT_OP_SHIFT_PLAYER_ALIGNMENT_A:
                OpShiftPlayerAlignmentA();
                return 0;
            case SCRIPT_OP_SAVE_OBJECT_CONDITIONS:
                OpSaveObjectConditions();
                return 0;
            case SCRIPT_OP_BEGIN_WINDOW_ALT_TEXT:
                BeginWindowAltText(window);
                return 0;
            case SCRIPT_OP_END_WINDOW_ALT_TEXT:
                EndWindowAltText(window);
                return 0;
            case SCRIPT_OP_BEGIN_WINDOW_INSTANT_TEXT:
                BeginWindowInstantText(window);
                return 0;
            case SCRIPT_OP_END_WINDOW_INSTANT_TEXT:
                EndWindowInstantText(window);
                return 0;
            case SCRIPT_OP_SET_OBJECT_FAMILIARITY:
                OpSetObjectFamiliarity(0);
                return 0;
            case SCRIPT_OP_SET_OBJECT_FAMILIARITY_NEGATED:
                OpSetObjectFamiliarity(1);
                return 0;
            case SCRIPT_OP_ADD_ACTOR_FAMILIARITY:
                OpAddActorFamiliarity(0);
                return 0;
            case SCRIPT_OP_SUBTRACT_ACTOR_FAMILIARITY:
                OpAddActorFamiliarity(1);
                return 0;
            case SCRIPT_OP_ADD_FAMILIARITY_COUNT:
                OpAddFamiliarityCount(0);
                return 0;
            case SCRIPT_OP_SUBTRACT_FAMILIARITY_COUNT:
                OpAddFamiliarityCount(1);
                return 0;
            case SCRIPT_OP_ADD_ACTOR_LEVEL_GAP:
                OpAddActorLevelGap(0);
                return 0;
            case SCRIPT_OP_SUBTRACT_ACTOR_LEVEL_GAP:
                OpAddActorLevelGap(1);
                return 0;
            case SCRIPT_OP_SET_ACTOR_FAMILIARITY:
                OpSetActorFamiliarity();
                return 0;
            case SCRIPT_OP_SET_ACTOR_LEVEL_GAP:
                OpSetActorLevelGap();
                return 0;
            case SCRIPT_OP_RESTART_AND_DESPAWN:
                RestartScript(0xdf, 1);
                DespawnScriptActor();
                return -3;
            case SCRIPT_OP_RESTART_AND_STEP_ACTOR_BACK:
                RestartScript(0xdf, 0);
                StepScriptActor(2);
                return -3;
            case SCRIPT_OP_STEP_ACTOR_FORWARD:
                return StepScriptActor(0);
            case SCRIPT_OP_STEP_ACTOR_BACK:
                return StepScriptActor(2);
            case SCRIPT_OP_TRANSITION_FORWARD:
                return PlayScreenTransition(0);
            case SCRIPT_OP_TRANSITION_BACK:
                return PlayScreenTransition(2);
            case SCRIPT_OP_UNEQUIP_LEADER_GUN_AND_AMMO:
                UnequipPart(0, EQUIP_PART_GUN);
                UnequipPart(0, EQUIP_PART_AMMO);
                RecalcCharacterStats(GetRosterCharacter(0));
                return 0;
            case SCRIPT_OP_SET_LONG_VAR:
                OpSetLongVar();
                return 0;
            case SCRIPT_OP_SWAP_LONG_VARS:
                OpSwapLongVars();
                return 0;
            case SCRIPT_OP_COPY_LONG_VAR:
                OpCopyLongVar();
                return 0;
            case SCRIPT_OP_UNSET_LONG_VAR:
                OpUnsetLongVar();
                return 0;
            case SCRIPT_OP_ZERO_LONG_VAR:
                OpZeroLongVar();
                return 0;
            case SCRIPT_OP_NEG_LONG_VAR:
                OpNegLongVar();
                return 0;
            case SCRIPT_OP_NOT_LONG_VAR:
                OpNotLongVar();
                return 0;
            case SCRIPT_OP_INC_LONG_VAR:
                OpIncLongVar();
                return 0;
            case SCRIPT_OP_DEC_LONG_VAR:
                OpDecLongVar();
                return 0;
            case SCRIPT_OP_INC_LONG_VAR_BELOW:
                OpIncLongVarBelow();
                return 0;
            case SCRIPT_OP_DEC_LONG_VAR_ABOVE:
                OpDecLongVarAbove();
                return 0;
            case SCRIPT_OP_CLAMP_LONG_VAR:
                OpClampLongVar();
                return 0;
            case SCRIPT_OP_ROLL_LONG_VAR:
                OpRollLongVar();
                return 0;
            case SCRIPT_OP_RAND_LONG_VAR:
                OpRandLongVar();
                return 0;
            case SCRIPT_OP_STORE_FRAME_LOCALS:
                StoreFrameLocals();
                return 0;
            case SCRIPT_OP_LOAD_FRAME_LOCALS:
                LoadFrameLocals();
                return 0;
            case SCRIPT_OP_SWAP_FRAME_LOCALS:
                SwapFrameLocals();
                return 0;
            case SCRIPT_OP_SET_TEXT_CHAR_DELAY:
                OpSetTextCharDelay();
                return 0;
            case SCRIPT_OP_ENABLE_TEXT_DELAY:
                EnableTextDelay();
                return 0;
            case SCRIPT_OP_DISABLE_TEXT_DELAY:
                DisableTextDelay();
                return 0;
            case SCRIPT_OP_REPLACE_TEXT_CHAR_DELAY:
                OpReplaceTextCharDelay();
                return 0;
            case SCRIPT_OP_DISABLE_TEXT_DELAY_SKIP:
                DisableTextDelaySkip();
                return 0;
            case SCRIPT_OP_ENABLE_TEXT_DELAY_SKIP:
                EnableTextDelaySkip();
                return 0;
            case SCRIPT_OP_SET_TEXT_WAIT_FRAMES:
                OpSetTextWaitFrames();
                return 0;
            case SCRIPT_OP_DISABLE_TEXT_TIMED_WAIT:
                SetTextTimedWait(0);
                return 0;
            case SCRIPT_OP_ENABLE_TEXT_TIMED_WAIT:
                SetTextTimedWait(1);
                return 0;
            case SCRIPT_OP_ENABLE_TEXT_SCROLL:
                SetTextScrollMode(1);
                SetTextPeriod(window);
                ClearTextPeriod();
                return 0;
            case SCRIPT_OP_DISABLE_TEXT_SCROLL:
                SetTextScrollMode(0);
                return 0;
            case SCRIPT_OP_CHANGE_MAP:
                OpChangeMap();
                return 0;
            case SCRIPT_OP_SET_WORLD_MAP_SPOT:
                OpSetWorldMapSpot();
                return 0;
            case 519:
            case 521:
                OpPushScriptWindow();
                return -3;
            case 520:
            case 522:
                PopScriptWindow();
                return -3;
            case SCRIPT_OP_START_TICK_COUNTER:
                OpStartTickCounter();
                return 0;
            case SCRIPT_OP_PAUSE_TICK_COUNTER:
                OpPauseTickCounter();
                return 0;
            case SCRIPT_OP_STOP_TICK_COUNTER:
                OpStopTickCounter();
                return 0;
            case SCRIPT_OP_GET_TICK_COUNTER:
                OpGetTickCounter();
                return 0;
            case SCRIPT_OP_SET_TICK_COUNTER:
                OpSetTickCounter();
                return 0;
            case SCRIPT_OP_WAIT_MESSAGE:
                return OpWaitMessage(window);
            case SCRIPT_OP_RUN_CHOICE_MENU:
                OpRunChoiceMenu(window);
                return -3;
            case SCRIPT_OP_IF_FACING_NORMAL:
                OpIfFacing(0);
                return 0;
            case SCRIPT_OP_IF_FACING_INVERTED:
                OpIfFacing(1);
                return 0;
            case SCRIPT_OP_IF_RETURN_FACING_NORMAL:
                OpIfReturnFacing(0);
                return 0;
            case SCRIPT_OP_IF_RETURN_FACING_INVERTED:
                OpIfReturnFacing(1);
                return 0;
            case SCRIPT_OP_IF_OBJECT_HAS_CONDITION_NORMAL:
                OpIfObjectHasCondition(0);
                return 0;
            case SCRIPT_OP_IF_OBJECT_HAS_CONDITION_INVERTED:
                OpIfObjectHasCondition(1);
                return 0;
            case 536:
                OpIfBlockedToward(0, 0);
                return 0;
            case 537:
                OpIfBlockedToward(1, 0);
                return 0;
            case 538:
                OpJumpUnlessActorCanStep(0, 0);
                return 0;
            case 539:
                OpJumpUnlessActorCanStep(1, 0);
                return 0;
            case SCRIPT_OP_BRANCH_ON_ITEMS_FIT_NORMAL:
                OpBranchOnItemsFit(0);
                return 0;
            case SCRIPT_OP_BRANCH_ON_ITEMS_FIT_INVERTED:
                OpBranchOnItemsFit(1);
                return 0;
            case SCRIPT_OP_QUEUE_AUTO_MOVES:
                OpQueueAutoMoves();
                return 0;
            case SCRIPT_OP_ADD_PENDING_ITEM:
                OpAddPendingItem();
                return 0;
            case SCRIPT_OP_REMOVE_PENDING_ITEM:
                OpRemovePendingItem();
                return 0;
            case SCRIPT_OP_GIVE_POOLED_ITEMS:
                GivePooledItems();
                return 0;
            case SCRIPT_OP_TAKE_POOLED_ITEMS:
                TakePooledItems();
                return 0;
            case SCRIPT_OP_FIND_FIRST_MEMBER_BY_HP_STATE:
                OpFindMemberByPoolState(0, POOL_MASK_HP);
                return 0;
            case SCRIPT_OP_MASK_MEMBERS_BY_HP_STATE:
                OpFindMemberByPoolState(1, POOL_MASK_HP);
                return 0;
            case SCRIPT_OP_FIND_FIRST_MEMBER_BY_MP_STATE:
                OpFindMemberByPoolState(0, POOL_MASK_MP);
                return 0;
            case SCRIPT_OP_MASK_MEMBERS_BY_MP_STATE:
                OpFindMemberByPoolState(1, POOL_MASK_MP);
                return 0;
            case SCRIPT_OP_FIND_FIRST_MEMBER_WITH_CONDITION:
                OpFindMemberWithCondition(0);
                return 0;
            case SCRIPT_OP_MASK_MEMBERS_WITH_CONDITION:
                OpFindMemberWithCondition(1);
                return 0;
            case SCRIPT_OP_FIND_FIRST_MEMBER_BY_ALIGNMENT_A:
                OpFindMemberByAlignmentA(0);
                return 0;
            case SCRIPT_OP_MASK_MEMBERS_BY_ALIGNMENT_A:
                OpFindMemberByAlignmentA(1);
                return 0;
            case SCRIPT_OP_FIND_FIRST_MEMBER_BY_ALIGNMENT_B:
                OpFindMemberByAlignmentB(0);
                return 0;
            case SCRIPT_OP_MASK_MEMBERS_BY_ALIGNMENT_B:
                OpFindMemberByAlignmentB(1);
                return 0;
            case SCRIPT_OP_COUNT_ITEM_OWNED:
                OpCountItemOwned();
                return 0;
            case SCRIPT_OP_ENABLE_CHOICE_CANCEL:
                SetWindowOption(1);
                return 0;
            case SCRIPT_OP_DISABLE_CHOICE_CANCEL:
                SetWindowOption(0);
                return 0;
            case SCRIPT_OP_CALL_SUB_SCENE:
                return OpCallSubScene();
            case SCRIPT_OP_GET_ACTOR_MOON_VALUE:
                OpGetActorMoonValue();
                return 0;
            case SCRIPT_OP_STORE_ACTOR_DISTANCE:
                OpStoreActorDistance();
                return 0;
            case SCRIPT_OP_PLACE_SCRIPT_ACTOR:
                PlaceScriptActor();
                return -1;
            case SCRIPT_OP_RETIRE_SCRIPT_ACTOR:
                RetireScriptActor();
                return -1;
            case 566:
                ReadScriptWord();
                ReadScriptWord();
                return 0;
            case SCRIPT_OP_SET_OBJECT_FIELD:
                OpSetObjectField();
                return 0;
            case 571:
                ReadScriptWord();
                ReadScriptByte();
                ReadScriptWord();
                return 0;
            case SCRIPT_OP_SQRT_LONG_VAR_EXPLICIT:
                OpSqrtLongVar(0);
                return 0;
            case SCRIPT_OP_SQRT_LONG_VAR_IN_PLACE:
                OpSqrtLongVar(1);
                return 0;
            case SCRIPT_OP_PRINT_ROSTER_NAME:
                OpPrintRosterName();
                return 0;
            case SCRIPT_OP_GET_SELECTED_OBJECT_ID:
                OpGetSelectedObjectId();
                return 0;
            case 580:
            case 581:
            case 582:
            case 583:
            case 584:
            case 585:
            case 586:
            case 587:
            case 588:
            case 589:
            case 590:
            case 591:
                target = ReadBranchTarget();
                ScriptJumpUnless(target, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_ROSTER_HAS_NO_DEMONS_NORMAL:
                OpJumpUnlessRosterHasNoDemons(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_ROSTER_HAS_NO_DEMONS_INVERTED:
                OpJumpUnlessRosterHasNoDemons(1);
                return 0;
            case SCRIPT_OP_IF_OBJECT_IS_ALLY_NORMAL:
                OpIfObjectIsAlly(0);
                return 0;
            case SCRIPT_OP_IF_OBJECT_IS_ALLY_INVERTED:
                OpIfObjectIsAlly(1);
                return 0;
            case SCRIPT_OP_IF_STATUS_POSITIVE_NORMAL:
                OpIfStatusPositive(0);
                return 0;
            case SCRIPT_OP_IF_STATUS_POSITIVE_INVERTED:
                OpIfStatusPositive(1);
                return 0;
            case SCRIPT_OP_RECOVER_ROSTER_HP:
                OpRecoverRosterPool(1);
                return 0;
            case SCRIPT_OP_RECOVER_ROSTER_MP:
                OpRecoverRosterPool(2);
                return 0;
            case SCRIPT_OP_CURE_ROSTER_CONDITION:
                OpCureRosterCondition();
                return 0;
            case SCRIPT_OP_MASK_ROSTER_BY_KIND:
                OpMaskRosterByKind();
                return 0;
            case SCRIPT_OP_SHOW_BACKGROUND:
                OpShowBackground();
                return 0;
            case SCRIPT_OP_RESTORE_BACKGROUND:
                OpRestoreBackground();
                return 0;
            case SCRIPT_OP_OPEN_SCRIPT_PANEL:
                OpOpenScriptPanel();
                return 0;
            case SCRIPT_OP_CLOSE_LAST_SCRIPT_PANEL:
                CloseLastScriptPanel();
                return 0;
            case SCRIPT_OP_CLOSE_ALL_SCRIPT_PANELS:
                CloseAllScriptPanels();
                return 0;
            case SCRIPT_OP_SET_PANEL_ENTRY_JUMP:
                OpSetPanelEntryJump();
                return 0;
            case SCRIPT_OP_DRAW_SCRIPT_PANELS:
                DrawScriptPanels();
                return 0;
            case SCRIPT_OP_SKIP_PANEL_OPERANDS:
                OpSkipPanelOperands();
                return 0;
            case SCRIPT_OP_SET_PANEL_ENTRY_VALUE:
                OpSetPanelEntryValue();
                return 0;
            case SCRIPT_OP_PUSH_GAME_STATE:
                OpPushGameState();
                return -3;
            case SCRIPT_OP_LOAD_RECORD:
                OpLoadRecord();
                return 0;
            case SCRIPT_OP_OPEN_ITEM_LIST_WINDOW:
                OpOpenItemListWindow();
                return 0;
            case SCRIPT_OP_CLOSE_ITEM_LIST_WINDOW:
                OpCloseItemListWindow();
                return 0;
            case SCRIPT_OP_CLOSE_SCRIPT_PANEL:
                OpCloseScriptPanel();
                return 0;
            case SCRIPT_OP_REDRAW_ITEM_LIST_TOTAL:
                OpRedrawItemListTotal();
                return 0;
            case SCRIPT_OP_IF_POOL_HAS_ITEMS_NORMAL:
                OpIfPoolHasItems(0);
                return 0;
            case SCRIPT_OP_IF_POOL_HAS_ITEMS_INVERTED:
                OpIfPoolHasItems(1);
                return 0;
            case SCRIPT_OP_IF_BAG_HAS_ENTRIES_NORMAL:
                OpIfBagHasEntries(0);
                return 0;
            case SCRIPT_OP_IF_BAG_HAS_ENTRIES_INVERTED:
                OpIfBagHasEntries(1);
                return 0;
            case SCRIPT_OP_FUSION_MENU_PAIR_FIRST:
                OpOpenFusionScreen(0);
                return -3;
            case SCRIPT_OP_FUSION_MENU_PAIR_SECOND:
                OpOpenFusionScreen(1);
                return -3;
            case SCRIPT_OP_FUSION_MENU_TRIPLE_THIRD:
                OpOpenFusionScreen(0x10);
                return -3;
            case SCRIPT_OP_FUSION_MENU_TRIPLE_FIRST:
                OpOpenFusionScreen(0x11);
                return -3;
            case SCRIPT_OP_FUSION_MENU_TRIPLE_SECOND:
                OpOpenFusionScreen(0x12);
                return -3;
            case SCRIPT_OP_RUN_FUSION_PAIR:
                OpRunFusion(0);
                return 0;
            case SCRIPT_OP_RUN_FUSION_TRIPLE:
                OpRunFusion(1);
                return 0;
            case SCRIPT_OP_END_FUSION:
                OpEndFusion();
                return 0;
            case SCRIPT_OP_FUSION_MENU_PAIR_COMMIT:
                OpOpenFusionScreen(3);
                return -3;
            case SCRIPT_OP_FUSION_MENU_TRIPLE_COMMIT:
                OpOpenFusionScreen(0x14);
                return -3;
            case SCRIPT_OP_OPEN_STATUS:
                PushGameState(0x19);
                return -3;
            case SCRIPT_OP_SET_SCENE_RENDER_MODE:
                SetSceneRenderMode();
                return 0;
            case 645:
            case 646:
            case 647:
                ReadScriptValue();
                return 0;
            case 650:
            case 652:
                SetBlankRenderMode();
                return 0;
            case SCRIPT_OP_JUMP_IF:
                OpJumpIf(1);
                return 0;
            case SCRIPT_OP_SKIP_JUMP_TARGET:
                OpSkipJumpTarget(1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_AT_MOST_ONE_DEMON:
                OpIfDemonCount(1, 1);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_AT_MOST_TWO_DEMONS:
                OpIfDemonCount(1, 2);
                return 0;
            case SCRIPT_OP_CREATE_SCRIPT_MENU:
                OpCreateScriptMenu();
                return 0;
            case SCRIPT_OP_RUN_SCRIPT_MENU:
                OpRunScriptMenu();
                return 0;
            case SCRIPT_OP_DESTROY_SCRIPT_MENU:
                OpDestroyScriptMenu();
                return 0;
            case SCRIPT_OP_ADD_MENU_LINE:
                OpAddMenuLine();
                return 0;
            case SCRIPT_OP_CLEAR_TEXT_BUFFERS:
                ClearTextBuffers();
                return 0;
            case SCRIPT_OP_FORMAT_NUMBER:
                OpFormatNumber();
                return 0;
            case SCRIPT_OP_FORMAT_CAPTURED_TEXT:
                OpFormatCapturedText();
                return 0;
            case SCRIPT_OP_RETURN_FROM_CALL:
                ReturnFromCall();
            case SCRIPT_OP_END:
            case 272:
            case 485:
                return -1;
            case SCRIPT_OP_GET_MENU_TAG:
                OpGetMenuTag();
                return 0;
            case SCRIPT_OP_ALLOC_LONG_ARRAY:
                OpAllocLongArray();
                return 0;
            case SCRIPT_OP_FREE_LONG_ARRAY:
                OpFreeLongArray();
                return 0;
            case SCRIPT_OP_GET_LONG_ARRAY_ITEM:
                OpGetLongArrayItem();
                return 0;
            case SCRIPT_OP_SET_LONG_ARRAY_ITEM:
                OpSetLongArrayItem();
                return 0;
            case SCRIPT_OP_CONVERT_CHARACTER_REF:
                OpConvertCharacterRef();
                return 0;
            case SCRIPT_OP_SELECT_PARTY_SLOT:
                OpSelectPartySlot();
                return 0;
            case SCRIPT_OP_END_PARTY_SLOT_SELECT:
                OpEndPartySlotSelect();
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_FLAG_SET:
                OpJumpUnlessFlagSet();
                return 0;
            case SCRIPT_OP_LOAD_DATA_FILE:
                OpLoadDataFile();
                return 0;
            case SCRIPT_OP_FREE_DATA_FILE:
                OpFreeDataFile();
                return 0;
            case SCRIPT_OP_READ_RECORD_INT:
                OpReadRecordInt();
                return 0;
            case SCRIPT_OP_READ_DATA_INT:
                OpReadDataInt();
                return 0;
            case SCRIPT_OP_SAVE_SCENE_CELL:
                OpSaveSceneCell();
                return 0;
            case SCRIPT_OP_MODIFY_EVENT_FLAG_BY_VALUE:
                OpModifyEventFlagByValue();
                return 0;
            case SCRIPT_OP_TEST_EVENT_FLAG_BY_VALUE:
                OpTestEventFlagByValue();
                return 0;
            case SCRIPT_OP_SWITCH_ON_VALUE_JUMP:
                OpSwitchOnValue(SCRIPT_BRANCH_JUMP);
                return 0;
            case SCRIPT_OP_SWITCH_ON_VALUE_CALL:
                OpSwitchOnValue(SCRIPT_BRANCH_CALL);
                return 0;
            case SCRIPT_OP_GET_PLAYER_LOCATION:
                OpGetPlayerLocation();
                return 0;
            case SCRIPT_OP_SET_PLAYER_POSITION:
                OpSetPlayerPosition();
                return 0;
            case SCRIPT_OP_GET_HOVERED_OBJECT_ID:
                OpGetHoveredObjectId();
                return 0;
            case SCRIPT_OP_SET_MENU_LINE_COLOR:
                OpSetMenuLineColor();
                return 0;
            case SCRIPT_OP_CLEAR_MENU_HIGHLIGHT:
                OpClearMenuHighlight();
                return 0;
            case SCRIPT_OP_GET_MENU_CURSOR:
                OpGetMenuCursor();
                return 0;
            case SCRIPT_OP_IF_MEMBER_HAS_CONDITION:
                OpIfMemberHasCondition();
                return 0;
            case SCRIPT_OP_REFRESH_FIELD_SCREEN:
                OpRefreshFieldScreen();
                return -3;
            case SCRIPT_OP_FADE_OUT_AND_CLEAR:
                OpFadeOutAndClear();
                return 0;
            case SCRIPT_OP_RESET_MASK:
                OpResetMask();
                return 0;
            case SCRIPT_OP_DRAW_IMAGE:
                OpDrawImage();
                return 0;
            case SCRIPT_OP_ENTER_FIELD_MAP:
                OpEnterFieldMap();
                return -3;
            case SCRIPT_OP_ADVANCE_CLOCK:
                OpAdvanceClock();
                return 0;
            case SCRIPT_OP_GET_TICKS_UNTIL_MOON_PHASE:
                OpGetTicksUntilMoonPhase();
                return 0;
            case SCRIPT_OP_GET_DAY_COUNT:
                OpGetDayCount();
                return 0;
            case SCRIPT_OP_GET_TIME_OF_DAY:
                OpGetTimeOfDay();
                return 0;
            case SCRIPT_OP_MOD_LONG_VAR_EXPLICIT:
                OpModLongVar(0);
                return 0;
            case SCRIPT_OP_MOD_LONG_VAR_IN_PLACE:
                OpModLongVar(1);
                return 0;
            case SCRIPT_OP_SET_OBJECT_PRESENCE:
                OpSetObjectPresence();
                return 0;
            case SCRIPT_OP_SAVE_DATA_COMMAND:
                OpSaveDataCommand();
                return 0;
            case SCRIPT_OP_FILL_SCREEN_CELLS:
                OpFillScreenCells();
                return 0;
            case SCRIPT_OP_MASK_SCREEN_CELLS:
                OpMaskScreenCells();
                return 0;
            case SCRIPT_OP_ENABLE_BACKGROUND:
                OpEnableBackground();
                return 0;
            case SCRIPT_OP_SET_MESSAGE_HOOK:
                OpSetMessageHook();
                return 0;
            case SCRIPT_OP_SKIP_VALUE_AND_VAR:
                OpSkipValueAndVar();
                return 0;
            case SCRIPT_OP_GET_ITEM_PRICE:
                OpGetItemPrice();
                return 0;
            case SCRIPT_OP_STEP_LIST_MENU:
                OpStepListMenu();
                return 0;
            case SCRIPT_OP_IF_BATTLE_RESULT:
                OpIfBattleResult();
                return 0;
            case SCRIPT_OP_IF_EVENT_OBJECT_IS:
                OpIfEventObjectIs();
                return 0;
            case SCRIPT_OP_COUNT_OBJECTS_AT:
                OpCountObjectsAt();
                return 0;
            case SCRIPT_OP_BOOST_POOL:
                OpBoostPool();
                return 0;
            case SCRIPT_OP_DISMISS_TALK_TARGET:
                DismissTalkTarget();
                return -3;
            case SCRIPT_OP_GET_COMBATANT_ID:
                OpGetCombatantId();
                return 0;
            case SCRIPT_OP_CAPTURE_DATA_STRING:
                OpCaptureDataString();
                return 0;
            case SCRIPT_OP_CAPTURE_RECORD_STRING:
                OpCaptureRecordString();
                return 0;
            case SCRIPT_OP_REQUEST_QUIT:
                RequestQuit();
                return -3;
            case SCRIPT_OP_COUNT_ACTIVE_PARTY:
                OpCountActiveParty();
                return 0;
            case SCRIPT_OP_PEEK_POKE_SCRATCH:
                OpPeekPokeScratch();
                return 0;
            case SCRIPT_OP_POLL_MOUSE_CLICK:
                OpPollMouseClick();
                return 0;
            case SCRIPT_OP_REDRAW_SCRIPT_MENU:
                OpRedrawScriptMenu();
                return 0;
            case SCRIPT_OP_SHOW_PICTURE:
                OpShowPicture();
                return 0;
            case SCRIPT_OP_SAVE_RESTORE_SCREEN:
                OpSaveRestoreScreen();
                return 0;
            case SCRIPT_OP_REBALANCE_MEMBER_STATS:
                OpRebalanceMemberStats();
                return 0;
            case SCRIPT_OP_STORE_SCRIPT_VAR:
                OpStoreScriptVar();
                return 0;
            case SCRIPT_OP_LOAD_SCRIPT_VAR:
                OpLoadScriptVar();
                return 0;
            case SCRIPT_OP_ADD_ROUTE_POINT:
                OpAddRoutePoint();
                return 0;
            case SCRIPT_OP_IF_IN_BATTLE:
                OpIfInBattle();
                return 0;
            case SCRIPT_OP_SET_LAST_PANEL_FLAG:
                OpSetLastPanelFlag();
                return 0;
            case SCRIPT_OP_GET_FUSION_RESULT:
                OpGetFusionResult();
                return 0;
            case SCRIPT_OP_CLEAR_CAPTURED_TEXT:
                ClearCapturedText();
                return 0;
            case SCRIPT_OP_LEVEL_UP_MEMBER:
                OpLevelUpMember();
                return 0;
            case SCRIPT_OP_STASH_ITEM_LISTS:
                OpStashItemLists();
                return 0;
            case SCRIPT_OP_GET_BATTLE_OUTCOME:
                OpGetBattleOutcome();
                return 0;
            case SCRIPT_OP_START_COUNTDOWN:
                OpStartCountdown();
                return 0;
            case SCRIPT_OP_CALL_TEXT_SCRIPT:
                OpCallTextScript();
                return 0;
            case SCRIPT_OP_SHOW_EVENT_PICTURE:
                OpShowEventPicture();
                return 0;
            case SCRIPT_OP_SET_FIELD_OPTION:
                OpSetFieldOption();
                return 0;
            case SCRIPT_OP_SET_FIELD_PARAMS:
                OpSetFieldParams();
                return 0;
            case SCRIPT_OP_SET_MENU_CHARACTER:
                OpSetMenuCharacter();
                return 0;
            case SCRIPT_OP_SET_MENU_SCROLL:
                OpSetMenuScroll();
                return 0;
            case SCRIPT_OP_PLAY_ANIMATION:
                OpPlayAnimation();
            case SCRIPT_OP_YIELD:
                return -3;
            case SCRIPT_OP_ADJUST_ITEM_COUNT:
                OpAdjustItemCount();
                return 0;
            case SCRIPT_OP_LIST_BAG_BY_CATEGORY:
                OpListBagByCategory();
                return 0;
            case SCRIPT_OP_GET_BAG_ENTRY:
                OpGetBagEntry();
                return 0;
            case SCRIPT_OP_CLEAR_BAG_ENTRY:
                OpClearBagEntry();
                return 0;
            case SCRIPT_OP_TAKE_DROP_SLOT:
                OpTakeDropSlot();
                return 0;
            case SCRIPT_OP_MODIFY_EVENT_FLAG:
                OpModifyEventFlag();
                return 0;
            case SCRIPT_OP_TEST_EVENT_FLAG:
                OpTestEventFlag();
                return 0;
            case SCRIPT_OP_STACK_MESSAGE_WINDOW:
                OpStackMessageWindow();
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_IN_LINE_NORMAL:
                OpJumpUnlessPlayerInLine(0);
                return 0;
            case SCRIPT_OP_JUMP_UNLESS_PLAYER_IN_LINE_INVERTED:
                OpJumpUnlessPlayerInLine(1);
                return 0;
            case SCRIPT_OP_SCREEN_TRANSITION:
                return OpScreenTransition();
            case SCRIPT_OP_SWAP_SCREEN_STATE:
                OpSwapScreenState();
                return 0;
            case SCRIPT_OP_COPY_ITEM_RECORD:
                OpCopyItemRecord();
                return 0;
            case SCRIPT_OP_ADD_MEMBER_SKILL:
                OpAddMemberSkill();
            default:
                return 0;
        }
    }
}
