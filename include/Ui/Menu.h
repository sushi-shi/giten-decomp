#ifndef GITEN_UI_MENU_H
#define GITEN_UI_MENU_H

#include <rva.h>

#include <EnumDomain.h>
#include <Ints.h>
#include <Ui/MenuBox.h>

// What RunListMenu returns while the menu stays open and once it is
// cancelled; otherwise the selected object id.
GZ_ENUM_CONST_BEGIN(ListMenuResult)
    LIST_MENU_OPEN = -1,
    LIST_MENU_CANCELLED = -2
GZ_ENUM_CONST_END(ListMenuResult)

// Runs the list menu one frame.
i16 RunListMenu(MenuBox* menu);

// Closes the list menu; returns the handle to store back (NULL).
MenuBox* CloseListMenu(MenuBox* menu);

// The menu pick (0x4537a0) records its result in the cursor's hovered and
// selected object ids (Input/Mouse.h): the item hit, and the value it
// selected (-1 when cancelled or unselectable).
void SetHoveredObject(i16 item);
void SetSelectedObject(i16 value);

#endif // GITEN_UI_MENU_H
