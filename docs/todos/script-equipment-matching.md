# Script and equipment matching boundaries

These functions have reconstructed behavior but still need a source shape that
reproduces the retail instructions. Use `giten walls diagnose <rva> --asm` and
the matching loop for live scores; the observations below are retail control
flow and caller contracts, not score checkpoints.

| Function | Retail evidence | Remaining source question |
| --- | --- | --- |
| [OpNextChoice](../../src/Script/scriptvars.c) (`0x03a870`) | The choice cursor is saved before `GetTextPlaneSize`. The choice walk advances that one cursor to null; later scrolls reload the exhausted cursor. Both signed divisions in each coordinate calculation use one column count. | The source now preserves the cursor and column-count behavior. Retail keeps separate entry and back-edge jumps around the choice walk and spills the cursor; the C loop still merges those edges. Recover the loop boundary without restarting the cursor on each scroll. |
| [PollEquipPart](../../src/Game/statuspanel.c) (`0x044fb0`) | A negative mode stores `-1` and returns directly at entry. Mode 100 enters the final unhighlight/reset path. Invalid coordinates, an excluded part, and an active curse converge on that path. Selecting the already highlighted part returns without resetting it. | The source has these paths, but MSVC merges its negative-mode reset with the final reset. Find a source-level return boundary that preserves the dedicated retail path for every negative mode. Callers pass `EQUIP_PICK_RESET`, `EQUIP_PICK_PART`, `EQUIP_PICK_ATTACH_TARGET`, or `EQUIP_PICK_CLEAR`; the retail signed comparison also defines behavior for other negative values. |
| [ItemListMenuHandler](../../src/Game/statestack.c) (`0x01a240`) | The callback accepts `MENU_EVENT_DESTROY`, `MENU_EVENT_BEGIN_PAGE`, and `MENU_EVENT_ADD_ROW` from `SetMenuItems`. Branches, returns, ordered referents, loads, stores, and immediate values align. Retail loads `menu` into `ebx` before saving `ebp`; the current object keeps `menu` in `ebp`. The retail row arm rereads the entry item after calls, matching repeated use of the existing item-stack accessor. | No missing event arm is indicated. A justified local or helper boundary would need to explain the register lifetime; declaration and case-scope changes alone did not. Avoid caching the item across calls without evidence that those calls cannot change it. |
| [RunEquipScreen](../../src/Game/statuspanel.c) (`0x042e60`) | Both objects have the same call, branch, return, and ordered-referent sequences. The opening key handling agrees. The second ammo magazine clamp differs in packed `ItemSlot` temporary scheduling, with extra moves in the current object. | Trace the aggregate copy and quantity clamp from the bag/equipment producers before changing the source order or storage model. |

The `PollEquipPart` caller modes are defined in
[EquipScreen.h](../../include/Game/EquipScreen.h), and the menu callback events
are defined in [MenuBox.h](../../include/Ui/MenuBox.h).
