#ifndef GITEN_UI_FIELDMENUS_H
#define GITEN_UI_FIELDMENUS_H

#include <rva.h>

#include <Game/PartyAction.h>
#include <Ints.h>
#include <Ui/Menu.h>

// The open party picker's text plane, or -1 when closed.
i16 GetPickerSelection(void);

i16 RunPartyPicker(i16 command);
void SetPartyPickerMode(i16 mode);
struct PartyMemberList;
MenuBox* OpenPartyPicker(struct PartyMemberList* entries);
void PartyPickerHandler(MenuBox* menu, i16 index, i16 event);

i16 CancelItemTargetMenu(i16 command);

// The field item-use flow (game state) and its user lookup.
i16 RunItemUse(void);
i16 FindFirstAbleMemberPosition(void);

i16 CancelFieldTargetMenu(i16 command);

// The field skill-use flow (game state), its pick and its preset member.
i16 RunFieldSkillUse(void);
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
void ItemListMenuHandler(MenuBox* menu, i16 index, i16 event);

MenuBox* OpenMemberSkillMenu(i16 id);
void MemberSkillMenuHandler(MenuBox* menu, i16 index, i16 event);

#endif // GITEN_UI_FIELDMENUS_H
