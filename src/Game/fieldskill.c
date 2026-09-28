// @identity-TODO: the owning TU is unproven; this unit holds the field
// skill-use flow's span until link-order evidence names it.

#include <rva.h>

#include <Game/CombatantId.h>

#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/Skill.h>
#include <Game/TargetFlags.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Input/Mouse.h>
#include <Math/Vec3.h>
#include <Ui/FieldMenus.h>
#include <Ui/Menu.h>

#include <stddef.h>

// @identity-TODO: PrepareSkillAction only clears this word; no reader survives.
DATA(0x00091980)
static i16 s_skillActionResetValue;

// The member (by id) using a skill on the field; -1 asks the picker.
DATA(0x00080a90)
static i16 s_skillUser;

// The skill picked from the member's list.
DATA(0x00080a94)
static i16 s_skillPicked;

// The selected skill's attack range, passed to the target window.
DATA(0x00080cec)
static u8 s_pickRange;

// The user's party position.
DATA(0x00080cf0)
static i16 s_userPosition;

// The picker or skill list menu.
DATA(0x00080d30)
static MenuBox* s_fieldMenu;

RVA(0x0002d7a0, 0x20)
i16 CancelFieldTargetMenu(i16 command) {
    if (command == -1) {
        s_fieldMenu = CloseListMenu(s_fieldMenu);
    }
    return -1;
}

static __inline void LocateFieldSkillUser(void) {
    s_userPosition = FindRosterSlotIn(s_skillUser, 1);
    s_userPosition = FindPartySlot(s_userPosition);
}

// Runs the field skill-use flow one phase: pick the member (unless one is
// set), pick a skill from its list, pick the skill's target, then hand the
// member's pick to the action prompt. Returns 0.
RVA(0x0002d7c0, 0x370)
i16 RunFieldSkillUse(void) {
    i16 flags;
    i16 picked;

    switch (GetGamePhase()) {
        case 0:
            HideScreenLayer(1);
            if (s_skillUser < 0) {
                SetGamePhase(8);
                return 0;
            }
            LocateFieldSkillUser();
            SetGamePhase(3);
            return 0;

        case 1:
            s_fieldMenu = ClosePickerMenu(s_fieldMenu);
            SetGamePhase(8);
            return 0;

        case 2:
            s_skillUser = RunPickerMenu(s_fieldMenu);
            if (s_skillUser == -2) {
                PrevGamePhase();
            }
            if (s_skillUser < 0) {
                break;
            }
            LocateFieldSkillUser();
            NextGamePhase();
            s_fieldMenu = ClosePickerMenu(s_fieldMenu);
            return 0;

        case 3:
            NextGamePhase();
            NextGamePhase();
            s_fieldMenu = OpenMemberSkillMenu(s_skillUser);
            return 0;

        case 4:
            SetGamePhase(8);
            s_fieldMenu = CloseListMenu(s_fieldMenu);
            return 0;

        case 5:
            s_skillPicked = RunListMenu(s_fieldMenu);
            if (s_skillPicked == -2) {
                PrevGamePhase();
            }
            if (s_skillPicked < 0) {
                break;
            }
            NextGamePhase();
            s_fieldMenu = CloseListMenu(s_fieldMenu);
            g_actionId = s_skillPicked;
            return 0;

        case 6:
            flags = GetSkillTargetFlags(s_skillPicked);
            if (TargetFlagsSelectSelf(flags)) {
                g_targetId = PartyCombatantId(s_userPosition);
                NextGamePhase();
                return 0;
            }
            if (TargetFlagsSelectActorGroup(flags)) {
                g_targetId = PartyCombatantId(s_userPosition);
                NextGamePhase();
                return 0;
            }
            s_pickRange = GetSkillAttackRange(g_actionId);
            if (flags == 0x10) {
                picked = RunPickTargetWindow(0, s_pickRange, 5, 0);
            } else if (flags == 0x11) {
                picked = RunPickTargetWindow(0, s_pickRange, 4, 0);
            } else if (flags == 0x30) {
                picked = RunPickTargetWindow(0, s_pickRange, 6, 0);
            } else {
                flags = 0;
                picked = RunPickTargetWindow(0, s_pickRange, 3, 0);
            }
            if (picked == -1) {
                SetGamePhase(3);
            }
            if (picked <= 0) {
                break;
            }
            if (flags) {
                g_selectedObjectId = SwapInForPick(s_userPosition, g_selectedObjectId);
            }
            g_targetId = g_selectedObjectId;
            NextGamePhase();
            return 0;

        case 7:
            NextGamePhase();
            g_actorId = PartyCombatantId(s_userPosition);
            SetSkillPick(s_userPosition);
            PrepareSkillAction();
            PushFieldUsePrompt();
            return 0;

        case 8:
            RestoreSwappedMember();
            ReturnFromGameState();
            break;
    }
    return 0;
}

// Makes party member `position`'s pick the chosen skill on the current target.
// @early-stop operand order: retail loads g_targetId into a register and xors the
// old pick word into it; cl here xors g_targetId from memory into the old word
// (same bits); the permuter found one compiler island.
RVA(0x0002db30, 0x60)
void SetSkillPick(i16 position) {
    Character* user = GetPartyCharacter(position);

    if (user != NULL) {
        user->pickObject = g_targetId;
        user->pickRole = 4;
        g_actionId = s_skillPicked;
        user->pickTarget = s_skillPicked;
    }
}

RVA(0x0002db90, 0x4e)
void PrepareSkillAction(void) {
    SkillHeader* skill;
    s_skillActionResetValue = 0;
    skill = GetCachedSkill(g_actionId);
    g_statusCondition = GetSkillInflictedCondition(skill);
    g_actionResult = GetSkillEffectCode(skill);
    g_drainAmount = 0;
    g_hpChange = GetSkillValueB(skill);
    g_mpChange = 0;
}

RVA(0x0002dbe0, 0x10)
void SetFieldSkillUser(i16 id) {
    s_skillUser = id;
}
