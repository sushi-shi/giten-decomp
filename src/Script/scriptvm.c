// @identity-TODO: the owning TU is unproven; this unit holds the dispatcher's
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/GameState.h>
#include <Game/ItemPool.h>
#include <Game/StateStack.h>
#include <Gfx/Render.h>
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
u16 g_scriptOpcode = 0;

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x0002ff40, 0x7)
u16 GetScriptOpcode(void) {
    return g_scriptOpcode;
}

// Executes one script opcode for the text window `window`. Opcodes 29..31
// prefix a second byte naming an extended opcode (0x300/0x200/0x100 + byte),
// which is dispatched in turn. Returns 0 to continue, -1 to end the script
// and -3 to yield until the next frame; some handlers return their own status.
RVA(0x0002ff50, 0x2560)
i16 ExecScriptOpcode(i16 window, u16 op) {
    u16 entry;
    i16 target;

    for (;;) {
        g_scriptOpcode = op;
        switch (op) {
            case 29:
                op = ReadScriptByte() + 0x300;
                continue;
            case 30:
                op = ReadScriptByte() + 0x200;
                continue;
            case 31:
                op = ReadScriptByte() + 0x100;
                continue;
            case 354:
                op = 0x1de;
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
            case 9:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TEST, 0);
                return 0;
            case 10:
                AdvanceWindowLine(window);
                return 0;
            case 11:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TEST, 1);
                return 0;
            case 12:
                OpJumpScript(0);
                return 0;
            case 13:
                OpJumpScript(1);
                return 0;
            case 14:
                OpSwitchOnRandom(0);
                return 0;
            case 15:
                OpSwitchOnSelection(0);
                return 0;
            case 16:
                OpJumpUnlessStatContest(0, 0, 0);
                return 0;
            case 17:
                OpJumpUnlessStatContest(0, 1, 0);
                return 0;
            case 18:
                OpJumpUnlessStatContest(1, 0, 0);
                return 0;
            case 19:
                OpJumpUnlessStatContest(1, 1, 0);
                return 0;
            case 20:
                OpJumpUnlessStatContest(2, 0, 0);
                return 0;
            case 21:
                OpJumpUnlessStatContest(2, 1, 0);
                return 0;
            case 22:
                OpJumpUnlessStatContest(3, 0, 0);
                return 0;
            case 23:
                OpJumpUnlessStatContest(3, 1, 0);
                return 0;
            case 24:
                OpJump();
                return 0;
            case 25:
                OpApplyEventFlag(SCRIPT_FLAG_SET, 1);
                return 0;
            case 26:
                OpApplyEventFlag(SCRIPT_FLAG_CLEAR, 0);
                return 0;
            case 27:
                SetTextCapture(1);
                return 0;
            case 28:
                SetTextCapture(0);
                return 0;
            case 257:
                OpPrintOperandText();
                return 0;
            case 258:
                OpPrintNumber();
                return 0;
            case 259:
                OpIfFlags(0);
                return 0;
            case 260:
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
            case 269:
                return OpSetActorAlert(0);
            case 270:
                return OpSetActorAlert(1);
            case 271:
                return OpSetActorAlert(2);
            case 273:
                OpSwitchOnRandom(1);
                return 0;
            case 274:
                OpSwitchOnSelection(1);
                return 0;
            case 275:
                OpSwitchOnAlignmentA(0);
                return 0;
            case 276:
                OpSwitchOnAlignmentA(1);
                return 0;
            case 277:
                OpSwitchOnAlignmentB(0);
                return 0;
            case 278:
                OpSwitchOnAlignmentB(1);
                return 0;
            case 279:
                OpSwitchOnMoonPhase(0);
                return 0;
            case 280:
                OpSwitchOnMoonPhase(1);
                return 0;
            case 281:
                OpSwitchOnRange(0);
                return 0;
            case 282:
                OpSwitchOnRange(1);
                return 0;
            case 283:
                OpSwitchOnActorAttrA(0);
                return 0;
            case 284:
                OpSwitchOnActorAttrA(1);
                return 0;
            case 285:
                OpSwitchOnActorAttrB(0);
                return 0;
            case 286:
                OpSwitchOnActorAttrB(1);
                return 0;
            case 287:
                OpSetActorAttitude();
                return 0;
            case 288:
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
            case 300:
                SetWindowReverse(window, 1);
                return 0;
            case 301:
                SetWindowReverse(window, 0);
                return 0;
            case 302:
                SetWindowReverse(window, -1);
                return 0;
            case 303:
                OpSetWindowColor(window, 0);
                return 0;
            case 304:
                OpSetWindowColor(window, 2);
                return 0;
            case 305:
                OpSetWindowColor(window, 1);
                return 0;
            case 306:
                SetWindowOpaqueBg(window, 1);
                return 0;
            case 307:
                SetWindowOpaqueBg(window, 0);
                return 0;
            case 308:
                SetWindowAttrFlag1(window, 1);
                return 0;
            case 309:
                SetWindowAttrFlag1(window, 0);
                return 0;
            case 310:
                SetWindowHalfWidth(window, 0);
                return 0;
            case 311:
                SetWindowHalfWidth(window, 1);
                return 0;
            case 312:
                SetWindowAttrFlag2(window, 0);
                return 0;
            case 313:
                SetWindowAttrFlag2(window, 1);
                return 0;
            case 314:
                StashWindowColor(window, 1);
                return 0;
            case 315:
                StashWindowColor(window, 0);
                return 0;
            case 316:
                OpSetWindowAltColor(window);
                return 0;
            case 317:
                OpSetWindowInstantColor(window);
                return 0;
            case 318:
                return OpAddToRoster();
            case 319:
                OpRemoveFromRoster();
                return 0;
            case 320:
                OpJoinActiveParty();
                return 0;
            case 321:
                OpLeaveActiveParty();
                return 0;
            case 322:
                OpMulLongVar(0);
                return 0;
            case 323:
                OpDivLongVar(0);
                return 0;
            case 324:
                OpAddLongVar(0);
                return 0;
            case 325:
                OpSubLongVar(0);
                return 0;
            case 326:
                OpAndLongVar(0);
                return 0;
            case 327:
                OpOrLongVar(0);
                return 0;
            case 328:
                OpXorLongVar(0);
                return 0;
            case 329:
                OpShlLongVar(0);
                return 0;
            case 330:
                OpSarLongVar(0);
                return 0;
            case 331:
                OpPercentLongVar(0);
                return 0;
            case 332:
                OpMulLongVar(1);
                return 0;
            case 333:
                OpDivLongVar(1);
                return 0;
            case 334:
                OpAddLongVar(1);
                return 0;
            case 335:
                OpSubLongVar(1);
                return 0;
            case 336:
                OpAndLongVar(1);
                return 0;
            case 337:
                OpOrLongVar(1);
                return 0;
            case 338:
                OpXorLongVar(1);
                return 0;
            case 339:
                OpShlLongVar(1);
                return 0;
            case 340:
                OpSarLongVar(1);
                return 0;
            case 341:
                OpPercentLongVar(1);
                return 0;
            case 342:
                OpApplyEventFlag(SCRIPT_FLAG_TOGGLE, 1);
                return 0;
            case 344:
                OpPushReturnTarget();
                return 0;
            case 345:
                DropCallFrame();
                return 0;
            case 346:
                SwapCallFrames();
                return 0;
            case 347:
                ClearCallStack();
                return 0;
            case 348:
                OpAddMacca(1);
                return 0;
            case 349:
                OpAddMacca(-1);
                return 0;
            case 350:
                OpAddMagnetite(1);
                return 0;
            case 351:
                OpAddMagnetite(-1);
                return 0;
            case 352:
                OpGiveItem();
                return 0;
            case 353:
                OpTakeItem();
                return 0;
            case 355:
            case 356:
                GrantActorReward(3);
                return 0;
            case 357:
                GrantActorSpoil(0);
                return 0;
            case 358:
                GrantActorSpoil(1);
                return 0;
            case 359:
                GrantActorReward(1);
                return 0;
            case 360:
                GrantActorReward(0);
                return 0;
            case 361:
                GrantActorReward(2);
                return 0;
            case 362:
                GrantActorSpoil(2);
                return 0;
            case 363:
                GrantActorReward(7);
                return 0;
            case 364:
                GrantActorReward(8);
                return 0;
            case 365:
                OpRollActorMagnetite();
                return 0;
            case 366:
                OpRollActorMacca();
                return 0;
            case 368:
                OpLoadSprite();
                return 0;
            case 369:
                OpPlaceSprite(0);
                return 0;
            case 370:
                OpHideSprite();
                return 0;
            case 371:
                OpPlaceSprite(1);
                return 0;
            case 375:
                OpFadeIn();
                return 0;
            case 376:
                OpFadeOut();
                return 0;
            case 377:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_SET, 0);
                return 0;
            case 378:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_SET, 1);
                return 0;
            case 379:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_CLEAR, 0);
                return 0;
            case 380:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_CLEAR, 1);
                return 0;
            case 381:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TOGGLE, 0);
                return 0;
            case 382:
                OpJumpUnlessEventFlag(SCRIPT_FLAG_TOGGLE, 1);
                return 0;
            case 383:
                OpJumpUnlessCompare(COMPARE_NOT_EQUAL, 0);
                return 0;
            case 384:
                OpJumpUnlessCompare(COMPARE_EQUAL, 0);
                return 0;
            case 385:
                OpJumpUnlessCompare(COMPARE_GREATER_EQUAL, 0);
                return 0;
            case 386:
                OpJumpUnlessCompare(COMPARE_LESS, 0);
                return 0;
            case 387:
                OpJumpUnlessCompare(COMPARE_GREATER, 0);
                return 0;
            case 388:
                OpJumpUnlessCompare(COMPARE_LESS_EQUAL, 0);
                return 0;
            case 389:
                OpJumpUnlessCompare(COMPARE_NOT_EQUAL, 1);
                return 0;
            case 390:
                OpJumpUnlessCompare(COMPARE_EQUAL, 1);
                return 0;
            case 391:
                OpJumpUnlessCompare(COMPARE_LESS_EQUAL, 1);
                return 0;
            case 392:
                OpJumpUnlessCompare(COMPARE_GREATER_EQUAL, 1);
                return 0;
            case 393:
                OpJumpUnlessCompare(COMPARE_LESS, 1);
                return 0;
            case 394:
                OpJumpUnlessCompare(COMPARE_GREATER, 1);
                return 0;
            case 395:
                OpJumpUnlessPlayerInView(0);
                return 0;
            case 396:
                OpJumpUnlessPlayerInView(1);
                return 0;
            case 397:
                OpJumpUnlessHpPercentRoll(COMPARE_LESS_EQUAL);
                return 0;
            case 398:
                OpJumpUnlessHpPercentRoll(COMPARE_GREATER);
                return 0;
            case 399:
                OpJumpUnlessHpQuarterRoll(COMPARE_GREATER_EQUAL);
                return 0;
            case 400:
                OpJumpUnlessHpQuarterRoll(COMPARE_LESS);
                return 0;
            case 401:
                OpJumpUnlessActorVisible(0);
                return 0;
            case 402:
                OpJumpUnlessActorVisible(1);
                return 0;
            case 403:
                OpJumpUnlessPlayerNearFront(0);
                return 0;
            case 404:
                OpJumpUnlessPlayerNearFront(1);
                return 0;
            case 405:
                OpJumpUnlessPlayerAtRange(0);
                return 0;
            case 406:
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
            case 411:
                OpJumpUnlessInRoster(0);
                return 0;
            case 412:
                OpJumpUnlessInRoster(1);
                return 0;
            case 413:
                OpJumpUnlessRosterFull(0);
                return 0;
            case 414:
                OpJumpUnlessRosterFull(1);
                return 0;
            case 415:
                OpJumpUnlessAlignmentMatch(0);
                return 0;
            case 416:
                OpJumpUnlessAlignmentMatch(1);
                return 0;
            case 417:
                OpJumpUnlessCanAfford(0);
                return 0;
            case 418:
                OpJumpUnlessCanAfford(1);
                return 0;
            case 419:
                OpJumpUnlessInParty(0);
                return 0;
            case 420:
                OpJumpUnlessInParty(1);
                return 0;
            case 421:
                OpIfHasItem(0);
                return 0;
            case 422:
                OpIfHasItem(1);
                return 0;
            case 423:
                OpIfHasAllItems(0);
                return 0;
            case 424:
                OpIfHasAllItems(1);
                return 0;
            case 425:
                OpJumpUnlessHealthy(0);
                return 0;
            case 426:
                OpJumpUnlessHealthy(1);
                return 0;
            case 427:
                OpJumpUnlessCompanionHealthy(0);
                return 0;
            case 428:
                OpJumpUnlessCompanionHealthy(1);
                return 0;
            case 429:
                OpJumpUnlessHeroEquipped(0);
                return 0;
            case 430:
                OpJumpUnlessHeroEquipped(1);
                return 0;
            case 431:
                OpIfNoActor(0);
                return 0;
            case 432:
                OpIfNoActor(1);
                return 0;
            case 433:
                OpBeginChoices(window);
                ReloadTextPeriod();
                return 0;
            case 434:
                OpNextChoice(window);
                return 0;
            case 439:
                OpEndChoices(window);
                ReloadTextPeriod();
                return 0;
            case 442:
                ClearMessageWindow(window);
                return 0;
            case 443:
                OpScrollWindow(window);
                return 0;
            case 444:
                OpGetWindowScrollTop(window);
                return 0;
            case 445:
                OpSetWindowScrollTop(window);
                return 0;
            case 446:
                OpGetWindowIndent(window);
                return 0;
            case 447:
                OpSetWindowIndent(window);
                return 0;
            case 448:
                OpGetWindowScrollStep(window);
                return 0;
            case 449:
                OpSetWindowScrollStep(window);
                return 0;
            case 450:
                OpGetWindowCursor(window);
                return 0;
            case 451:
                OpSetWindowCursor(window);
                return 0;
            case 452:
                PlaySoundEffect(MapSoundEffectId(ReadScriptValue()));
                return 0;
            case 453:
                PlayMusic(ReadScriptValue(), 1);
                return 0;
            case 455:
            case 576:
                OpChangeHp(1);
                return 0;
            case 456:
                OpChangeHp(-1);
                return 0;
            case 457:
            case 577:
                OpChangeMp(1);
                return 0;
            case 458:
                OpChangeMp(-1);
                return 0;
            case 459:
                OpApplyObjectCondition();
                return 0;
            case 460:
            case 578:
                OpClearObjectCondition();
                return 0;
            case 461:
                OpShiftPlayerAlignmentB();
                return 0;
            case 462:
                OpShiftPlayerAlignmentA();
                return 0;
            case 463:
                OpSaveObjectConditions();
                return 0;
            case 464:
                BeginWindowAltText(window);
                return 0;
            case 465:
                EndWindowAltText(window);
                return 0;
            case 466:
                BeginWindowInstantText(window);
                return 0;
            case 467:
                EndWindowInstantText(window);
                return 0;
            case 468:
                OpSetObjectFamiliarity(0);
                return 0;
            case 469:
                OpSetObjectFamiliarity(1);
                return 0;
            case 470:
                OpAddActorFamiliarity(0);
                return 0;
            case 471:
                OpAddActorFamiliarity(1);
                return 0;
            case 472:
                OpAddFamiliarityCount(0);
                return 0;
            case 473:
                OpAddFamiliarityCount(1);
                return 0;
            case 474:
                OpAddActorLevelGap(0);
                return 0;
            case 475:
                OpAddActorLevelGap(1);
                return 0;
            case 476:
                OpSetActorFamiliarity();
                return 0;
            case 477:
                OpSetActorLevelGap();
                return 0;
            case 478:
                RestartScript(0xdf, 1);
                DespawnScriptActor();
                return -3;
            case 479:
                RestartScript(0xdf, 0);
                StepScriptActor(2);
                return -3;
            case 481:
                return StepScriptActor(0);
            case 482:
                return StepScriptActor(2);
            case 483:
                return PlayScreenTransition(0);
            case 484:
                return PlayScreenTransition(2);
            case 487:
                UnequipPart(0, EQUIP_PART_GUN);
                UnequipPart(0, EQUIP_PART_AMMO);
                RecalcCharacterStats(GetRosterCharacter(0));
                return 0;
            case 488:
                OpSetLongVar();
                return 0;
            case 489:
                OpSwapLongVars();
                return 0;
            case 490:
                OpCopyLongVar();
                return 0;
            case 491:
                OpUnsetLongVar();
                return 0;
            case 492:
                OpZeroLongVar();
                return 0;
            case 493:
                OpNegLongVar();
                return 0;
            case 494:
                OpNotLongVar();
                return 0;
            case 495:
                OpIncLongVar();
                return 0;
            case 496:
                OpDecLongVar();
                return 0;
            case 497:
                OpIncLongVarBelow();
                return 0;
            case 498:
                OpDecLongVarAbove();
                return 0;
            case 499:
                OpClampLongVar();
                return 0;
            case 500:
                OpRollLongVar();
                return 0;
            case 501:
                OpRandLongVar();
                return 0;
            case 502:
                StoreFrameLocals();
                return 0;
            case 503:
                LoadFrameLocals();
                return 0;
            case 504:
                SwapFrameLocals();
                return 0;
            case 505:
                OpSetTextCharDelay();
                return 0;
            case 506:
                EnableTextDelay();
                return 0;
            case 507:
                DisableTextDelay();
                return 0;
            case 508:
                OpReplaceTextCharDelay();
                return 0;
            case 509:
                DisableTextDelaySkip();
                return 0;
            case 510:
                EnableTextDelaySkip();
                return 0;
            case 511:
                OpSetTextWaitFrames();
                return 0;
            case 512:
                SetTextTimedWait(0);
                return 0;
            case 513:
                SetTextTimedWait(1);
                return 0;
            case 514:
                SetTextScrollMode(1);
                SetTextPeriod(window);
                ClearTextPeriod();
                return 0;
            case 515:
                SetTextScrollMode(0);
                return 0;
            case 516:
                OpChangeMap();
                return 0;
            case 517:
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
            case 523:
                OpStartTickCounter();
                return 0;
            case 524:
                OpPauseTickCounter();
                return 0;
            case 525:
                OpStopTickCounter();
                return 0;
            case 526:
                OpGetTickCounter();
                return 0;
            case 527:
                OpSetTickCounter();
                return 0;
            case 528:
                return OpWaitMessage(window);
            case 529:
                OpRunChoiceMenu(window);
                return -3;
            case 530:
                OpIfFacing(0);
                return 0;
            case 531:
                OpIfFacing(1);
                return 0;
            case 532:
                OpIfReturnFacing(0);
                return 0;
            case 533:
                OpIfReturnFacing(1);
                return 0;
            case 534:
                OpIfObjectHasCondition(0);
                return 0;
            case 535:
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
            case 540:
                OpBranchOnItemsFit(0);
                return 0;
            case 541:
                OpBranchOnItemsFit(1);
                return 0;
            case 542:
                OpQueueAutoMoves();
                return 0;
            case 543:
                OpAddPendingItem();
                return 0;
            case 544:
                OpRemovePendingItem();
                return 0;
            case 545:
                GivePooledItems();
                return 0;
            case 546:
                TakePooledItems();
                return 0;
            case 547:
                OpFindMemberByPoolState(0, POOL_MASK_HP);
                return 0;
            case 548:
                OpFindMemberByPoolState(1, POOL_MASK_HP);
                return 0;
            case 549:
                OpFindMemberByPoolState(0, POOL_MASK_MP);
                return 0;
            case 550:
                OpFindMemberByPoolState(1, POOL_MASK_MP);
                return 0;
            case 551:
                OpFindMemberWithCondition(0);
                return 0;
            case 552:
                OpFindMemberWithCondition(1);
                return 0;
            case 553:
                OpFindMemberByAlignmentA(0);
                return 0;
            case 554:
                OpFindMemberByAlignmentA(1);
                return 0;
            case 555:
                OpFindMemberByAlignmentB(0);
                return 0;
            case 556:
                OpFindMemberByAlignmentB(1);
                return 0;
            case 557:
                OpCountItemOwned();
                return 0;
            case 558:
                SetWindowOption(1);
                return 0;
            case 559:
                SetWindowOption(0);
                return 0;
            case 560:
                return OpCallSubScene();
            case 561:
                OpGetActorMoonValue();
                return 0;
            case 562:
                OpStoreActorDistance();
                return 0;
            case 563:
                PlaceScriptActor();
                return -1;
            case 564:
                RetireScriptActor();
                return -1;
            case 566:
                ReadScriptWord();
                ReadScriptWord();
                return 0;
            case 570:
                OpSetObjectField();
                return 0;
            case 571:
                ReadScriptWord();
                ReadScriptByte();
                ReadScriptWord();
                return 0;
            case 572:
                OpSqrtLongVar(0);
                return 0;
            case 573:
                OpSqrtLongVar(1);
                return 0;
            case 574:
                OpPrintRosterName();
                return 0;
            case 579:
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
            case 592:
                OpJumpUnlessRosterHasNoDemons(0);
                return 0;
            case 593:
                OpJumpUnlessRosterHasNoDemons(1);
                return 0;
            case 604:
                OpIfObjectIsAlly(0);
                return 0;
            case 605:
                OpIfObjectIsAlly(1);
                return 0;
            case 606:
                OpIfStatusPositive(0);
                return 0;
            case 607:
                OpIfStatusPositive(1);
                return 0;
            case 610:
                OpRecoverRosterPool(1);
                return 0;
            case 611:
                OpRecoverRosterPool(2);
                return 0;
            case 612:
                OpCureRosterCondition();
                return 0;
            case 613:
                OpMaskRosterByKind();
                return 0;
            case 614:
                OpShowBackground();
                return 0;
            case 615:
                OpRestoreBackground();
                return 0;
            case 616:
                OpOpenScriptPanel();
                return 0;
            case 617:
                CloseLastScriptPanel();
                return 0;
            case 618:
                CloseAllScriptPanels();
                return 0;
            case 619:
                OpSetPanelEntryJump();
                return 0;
            case 620:
                DrawScriptPanels();
                return 0;
            case 621:
                OpSkipPanelOperands();
                return 0;
            case 622:
                OpSetPanelEntryValue();
                return 0;
            case 623:
                OpPushGameState();
                return -3;
            case 624:
                OpLoadRecord();
                return 0;
            case 625:
                OpOpenItemListWindow();
                return 0;
            case 626:
                OpCloseItemListWindow();
                return 0;
            case 627:
                OpCloseScriptPanel();
                return 0;
            case 628:
                OpRedrawItemListTotal();
                return 0;
            case 629:
                OpIfPoolHasItems(0);
                return 0;
            case 630:
                OpIfPoolHasItems(1);
                return 0;
            case 631:
                OpIfBagHasEntries(0);
                return 0;
            case 632:
                OpIfBagHasEntries(1);
                return 0;
            case 633:
                OpOpenFusionScreen(0);
                return -3;
            case 634:
                OpOpenFusionScreen(1);
                return -3;
            case 635:
                OpOpenFusionScreen(0x10);
                return -3;
            case 636:
                OpOpenFusionScreen(0x11);
                return -3;
            case 637:
                OpOpenFusionScreen(0x12);
                return -3;
            case 638:
                OpRunFusion(0);
                return 0;
            case 639:
                OpRunFusion(1);
                return 0;
            case 640:
                OpEndFusion();
                return 0;
            case 641:
                OpOpenFusionScreen(3);
                return -3;
            case 642:
                OpOpenFusionScreen(0x14);
                return -3;
            case 643:
                PushGameState(0x19);
                return -3;
            case 649:
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
            case 660:
                OpJumpIf(1);
                return 0;
            case 661:
                OpSkipJumpTarget(1);
                return 0;
            case 662:
                OpIfDemonCount(1, 1);
                return 0;
            case 663:
                OpIfDemonCount(1, 2);
                return 0;
            case 664:
                OpCreateScriptMenu();
                return 0;
            case 665:
                OpRunScriptMenu();
                return 0;
            case 666:
                OpDestroyScriptMenu();
                return 0;
            case 667:
                OpAddMenuLine();
                return 0;
            case 668:
                ClearTextBuffers();
                return 0;
            case 669:
                OpFormatNumber();
                return 0;
            case 670:
                OpFormatCapturedText();
                return 0;
            case 671:
                ReturnFromCall();
            case 0:
            case 272:
            case 485:
                return -1;
            case 672:
                OpGetMenuTag();
                return 0;
            case 673:
                OpAllocLongArray();
                return 0;
            case 674:
                OpFreeLongArray();
                return 0;
            case 675:
                OpGetLongArrayItem();
                return 0;
            case 676:
                OpSetLongArrayItem();
                return 0;
            case 677:
                OpConvertCharacterRef();
                return 0;
            case 678:
                OpSelectPartySlot();
                return 0;
            case 679:
                OpEndPartySlotSelect();
                return 0;
            case 680:
                OpJumpUnlessFlagSet();
                return 0;
            case 681:
                OpLoadDataFile();
                return 0;
            case 682:
                OpFreeDataFile();
                return 0;
            case 683:
                OpReadRecordInt();
                return 0;
            case 684:
                OpReadDataInt();
                return 0;
            case 685:
                OpSaveSceneCell();
                return 0;
            case 686:
                OpModifyEventFlagByValue();
                return 0;
            case 687:
                OpTestEventFlagByValue();
                return 0;
            case 688:
                OpSwitchOnValue(0);
                return 0;
            case 689:
                OpSwitchOnValue(1);
                return 0;
            case 690:
                OpGetPlayerLocation();
                return 0;
            case 691:
                OpSetPlayerPosition();
                return 0;
            case 692:
                OpGetHoveredObjectId();
                return 0;
            case 694:
                OpSetMenuLineColor();
                return 0;
            case 695:
                OpClearMenuHighlight();
                return 0;
            case 696:
                OpGetMenuCursor();
                return 0;
            case 697:
                OpIfMemberHasCondition();
                return 0;
            case 698:
                OpRefreshFieldScreen();
                return -3;
            case 699:
                OpFadeOutAndClear();
                return 0;
            case 700:
                OpResetMask();
                return 0;
            case 701:
                OpDrawImage();
                return 0;
            case 702:
                OpEnterFieldMap();
                return -3;
            case 703:
                OpAdvanceClock();
                return 0;
            case 704:
                OpGetTicksUntilMoonPhase();
                return 0;
            case 705:
                OpGetDayCount();
                return 0;
            case 706:
                OpGetTimeOfDay();
                return 0;
            case 707:
                OpModLongVar(0);
                return 0;
            case 708:
                OpModLongVar(1);
                return 0;
            case 709:
                OpSetObjectPresence();
                return 0;
            case 710:
                OpSaveDataCommand();
                return 0;
            case 711:
                OpFillScreenCells();
                return 0;
            case 712:
                OpMaskScreenCells();
                return 0;
            case 713:
                OpEnableBackground();
                return 0;
            case 715:
                OpSetMessageHook();
                return 0;
            case 714:
                OpSkipValueAndVar();
                return 0;
            case 716:
                OpGetItemPrice();
                return 0;
            case 717:
                OpStepListMenu();
                return 0;
            case 718:
                OpIfBattleResult();
                return 0;
            case 719:
                OpIfEventObjectIs();
                return 0;
            case 720:
                OpCountObjectsAt();
                return 0;
            case 721:
                OpBoostPool();
                return 0;
            case 722:
                DismissTalkTarget();
                return -3;
            case 723:
                OpGetCombatantId();
                return 0;
            case 724:
                OpCaptureDataString();
                return 0;
            case 725:
                OpCaptureRecordString();
                return 0;
            case 726:
                RequestQuit();
                return -3;
            case 727:
                OpCountActiveParty();
                return 0;
            case 728:
                OpPeekPokeScratch();
                return 0;
            case 729:
                OpPollMouseClick();
                return 0;
            case 730:
                OpRedrawScriptMenu();
                return 0;
            case 731:
                OpShowPicture();
                return 0;
            case 732:
                OpSaveRestoreScreen();
                return 0;
            case 733:
                OpRebalanceMemberStats();
                return 0;
            case 734:
                OpStoreScriptVar();
                return 0;
            case 735:
                OpLoadScriptVar();
                return 0;
            case 736:
                OpAddRoutePoint();
                return 0;
            case 737:
                OpIfInBattle();
                return 0;
            case 738:
                OpSetLastPanelFlag();
                return 0;
            case 739:
                OpGetFusionResult();
                return 0;
            case 740:
                ClearCapturedText();
                return 0;
            case 741:
                OpLevelUpMember();
                return 0;
            case 742:
                OpStashItemLists();
                return 0;
            case 743:
                OpGetBattleOutcome();
                return 0;
            case 744:
                OpStartCountdown();
                return 0;
            case 745:
                OpCallTextScript();
                return 0;
            case 746:
                OpShowEventPicture();
                return 0;
            case 747:
                OpSetFieldOption();
                return 0;
            case 748:
                OpSetFieldParams();
                return 0;
            case 749:
                OpSetMenuCharacter();
                return 0;
            case 750:
                OpSetMenuScroll();
                return 0;
            case 751:
                OpPlayAnimation();
            case 343:
                return -3;
            case 752:
                OpAdjustItemCount();
                return 0;
            case 753:
                OpListBagByCategory();
                return 0;
            case 754:
                OpGetBagEntry();
                return 0;
            case 755:
                OpClearBagEntry();
                return 0;
            case 756:
                OpTakeDropSlot();
                return 0;
            case 757:
                OpModifyEventFlag();
                return 0;
            case 758:
                OpTestEventFlag();
                return 0;
            case 759:
                OpStackMessageWindow();
                return 0;
            case 760:
                OpJumpUnlessPlayerInLine(0);
                return 0;
            case 761:
                OpJumpUnlessPlayerInLine(1);
                return 0;
            case 762:
                return OpScreenTransition();
            case 763:
                OpSwapScreenState();
                return 0;
            case 764:
                OpCopyItemRecord();
                return 0;
            case 765:
                OpAddMemberSkill();
            default:
                return 0;
        }
    }
}
