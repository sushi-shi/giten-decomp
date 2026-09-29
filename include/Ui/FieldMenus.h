#ifndef GITEN_UI_FIELDMENUS_H
#define GITEN_UI_FIELDMENUS_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/PartyAction.h>
#include <Ints.h>
#include <Ui/Menu.h>
#include <Enums.h>

// The open party picker's text plane, or -1 when closed.
i16 GetPickerSelection(void);

i16 RunPartyPicker(i16 command);
void SetPartyPickerMode(i16 mode);
struct PartyMemberList;
MenuBox* OpenPartyPicker(struct PartyMemberList* entries);
void PartyPickerHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

i16 CancelItemTargetMenu(i16 command);

// The field item-use flow (game state) and its user lookup.
// The phases of the field item-use state (RunItemUse): open the item list,
// close it, pick the item and its target, destroy the menu, prompt the action
// and finish.
GZ_ENUM_BEGIN(ItemUsePhase)
    ITEM_USE_PHASE_OPEN = 0,
    ITEM_USE_PHASE_CLOSE = 1,
    ITEM_USE_PHASE_PICK_ITEM = 2,
    ITEM_USE_PHASE_PICK_TARGET = 3,
    ITEM_USE_PHASE_DESTROY_MENU = 4,
    ITEM_USE_PHASE_PROMPT_ACTION = 5,
    ITEM_USE_PHASE_FINISH = 6
GZ_ENUM_END(ItemUsePhase)

b16 RunItemUse(void);
i16 FindFirstAbleMemberPosition(void);

i16 CancelFieldTargetMenu(i16 command);

// The field skill-use flow (game state), its pick and its preset member.
// The phases of the field skill-use state (RunFieldSkillUse): start, close the
// member picker, pick the member, open and close the skill list, pick the
// skill and its target, prompt the action, and end.
GZ_ENUM_BEGIN(FieldSkillUsePhase)
    SKILL_USE_PHASE_START = 0,
    SKILL_USE_PHASE_CLOSE_MEMBER_PICKER = 1,
    SKILL_USE_PHASE_PICK_MEMBER = 2,
    SKILL_USE_PHASE_OPEN_SKILL_LIST = 3,
    SKILL_USE_PHASE_CLOSE_SKILL_LIST = 4,
    SKILL_USE_PHASE_PICK_SKILL = 5,
    SKILL_USE_PHASE_PICK_TARGET = 6,
    SKILL_USE_PHASE_PROMPT_ACTION = 7,
    SKILL_USE_PHASE_END = 8
GZ_ENUM_END(FieldSkillUsePhase)

b16 RunFieldSkillUse(void);
void SetSkillPick(i16 position);
void SetFieldSkillUser(i16 id);

// Runs the member picker's menu one frame: the chosen object id, -1 while
// open, -2 cancelled.
i16 RunPickerMenu(MenuBox* menu);

// Releases the member picker's menu; returns the handle to store back.
MenuBox* ClosePickerMenu(MenuBox* menu);

// @identity-TODO: list menus a picked member acts through: the member's own
// skills (by member id) and an item list (0x423aa0 with 0x40 entries).
MenuBox* OpenItemListMenu(void);
void ItemListMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

MenuBox* OpenMemberSkillMenu(i16 id);
void MemberSkillMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

#endif // GITEN_UI_FIELDMENUS_H
