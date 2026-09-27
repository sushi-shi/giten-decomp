#ifndef GITEN_UI_MENU_H
#define GITEN_UI_MENU_H

#include <rva.h>

#include <Ints.h>
#include <Ui/MenuBox.h>

// Runs the list menu one frame: > 0 the selected object id, -1 still open,
// -2 cancelled.
i16 RunListMenu(MenuBox* menu);

// Closes the list menu; returns the handle to store back (NULL).
MenuBox* CloseListMenu(MenuBox* menu);

// The menu pick (0x4537a0) records its result in the cursor's hovered and
// selected object ids (Input/Mouse.h): the item hit, and the value it
// selected (-1 when cancelled or unselectable).
void SetHoveredObject(i16 item);
void SetSelectedObject(i16 value);

#endif // GITEN_UI_MENU_H
