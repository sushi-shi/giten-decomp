#ifndef GITEN_GAME_EQUIPSCREEN_H
#define GITEN_GAME_EQUIPSCREEN_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/Character.h>
#include <Ints.h>
#include <Enums.h>
#include <Text/TextPlane.h>
#include <Ui/MenuBox.h>

// clang-format off
GZ_ENUM_BEGIN(EquipPickMode)
    EQUIP_PICK_RESET = -1,
    EQUIP_PICK_PART = 0,
    EQUIP_PICK_ATTACH_TARGET = 1,
    EQUIP_PICK_CLEAR = 100
GZ_ENUM_END(EquipPickMode);
// clang-format on

// The status screen's equipment page.

// The equipment page's steps past MenuStateStep's open, close and run:
// preview and equip a bag item, preview and remove an equipped one.
GZ_ENUM_BEGIN(EquipScreenStep)
    EQUIP_STEP_PREVIEW_EQUIP = 3,
    EQUIP_STEP_EQUIP = 4,
    EQUIP_STEP_PREVIEW_REMOVE = 5,
    EQUIP_STEP_REMOVE = 6
GZ_ENUM_END(EquipScreenStep)

i16 RunEquipScreen(i16 key);

// Lists the bag entries (of the first 48) member `member` can equip; with none
// and `anyEquipped` set, returns whether anything is equipped instead.
i16 ListEquipCandidates(i16 member, i16 anyEquipped);

// Builds the menu of the bag items member `member` can equip.
MenuBox* OpenEquipMenu(i16 member, MenuBox* old);

// Draws the equipment panel of `member`, comparing with `preview` when not
// NULL.
void DrawEquipPanel(Character* member, Character* preview);

// Previews equipping bag entry `index` (or removing part `index` when
// `fromEquipped`), using character 14 to save and restore the member.
void PreviewEquipChange(i16 index, i16 fromEquipped);

// Opens a window with the name and description of `item`; returns it.
i16 OpenItemInfoPlane(i16 item);

i16 DrawEquipPickRow(i16 member, GZ_ENUM_PARAM(EquipPart, i16) part, i32 attr);

// The game step the status screen last ran; RunStatusCommands redraws the
// stat totals when it leaves step 8.
extern i16 g_previousStatusStep;
// The zero-initialized empty battle-skill label.
extern char g_emptyBattleSkillLabel[];
// The zero-initialized empty picker label.
extern char g_emptyEquipPickLabel[];

// The equipment part under the cursor, or -1 when none is picked. Reset
// forgets the selection; clear restores its normal appearance before forgetting it.
i16 PollEquipPart(i16 member, i16 mode);

// The status screen's attach page: fits a gem item into a bag entry or an
// equipped part (sub-state 0 opens it, 1 closes it; 2..8 pick the item, the
// target and report the result); returns the sub-state to resume or -1.
i16 RunAttachScreen(i16 sub);

// The attach page's state: the text hook it displaced, its two menus (the
// gem items, then the bag entries), the first gem id, its message
// window, the sub-state to resume, how many entries it lists, the item and
// target picked, and whether the status screen needs a redraw.
typedef struct AttachPage {
    TextPlaneHook prevHook;
    MenuBox* itemMenu;
    MenuBox* entryMenu;
    i16 itemBase;
    i16 plane;
    i16 resume;
    i16 entryCount;
    i16 item;
    i16 target;
    i16 redraw;
} AttachPage;

typedef struct EquipItemPage {
    MenuBox* menu;
    i16 plane;
    i16 pick;
    struct ItemStackList* list;
} EquipItemPage;

typedef struct EquipSkillPage {
    MenuBox* menu;
    i16 plane;
    i16 pick;
} EquipSkillPage;

// The status screen's item page: lists the bag (with `*` on entries holding a
// gem item) and opens the picked item's description (sub-state 0 opens
// it, 1 closes it, 2 picks, 3..4 show the description until a click); returns
// the sub-state to resume or -1.
i16 RunItemPage(i16 sub);

// Draws `character`'s eight equipped parts (the three hand parts first) as
// lines from (x, y) down, 4 apart.
void DrawEquipLines(Character* character, i16 x, i16 y);

// The status screen's skill page: lists the member's skills with their costs
// and opens the picked skill's description (sub-state 0 opens it, 1 closes it,
// 2 picks, 3..4 show the description until a click); returns the sub-state to
// resume or -1.
i16 RunSkillPage(i16 sub);

// A bag entry that can take a gem item, and how many it holds.
typedef struct AttachEntry {
    i16 entry;
    i16 count;
} AttachEntry;

// Attaches gem item index `index` to `member`'s equipped part `part`;
// returns the index it replaced, else -1.
i16 AttachEquipItem(i16 member, i16 part, i16 index);

#endif // GITEN_GAME_EQUIPSCREEN_H
