# Union view review

King's Field's [cast, union and goto review](https://github.com/sushi-shi/kings-field-decomp/blob/master/docs/patterns/cast-union-goto-review.md) checks every union member against its consumers before removing a view. This is the corresponding source review for Giten's `src/` and `include/` tree. A union with several used views is a model of one packed value or serialized payload; the review does not establish that the original source spelled a union.

There are 18 written union definitions. Every definition and its source consumers are listed below.
This is a view-level review, not a completed member-by-member census: it does
not count every member access across the compiled C and C++ variants or prove
which member determines each union's extent. For each of the 18 definitions,
resolve all member accesses, record unused members and compare the union size
with its largest used view before removing any alternative. Check the owning
functions' instructions and referents after a proposed change. A missing
direct spelling can reflect access through a macro or typed pointer, and an
unused member can still determine the size of a copied or stack object.

| Union | Used views and evidence | Decision |
| --- | --- | --- |
| `MapCell` (`include/Game/CellTrap.h`) | Only `exit` is read in `RunCellTrap` (`src/Game/treasurebox.c`). | Open: see below. |
| `SceneHotspot` object (`include/Game/SceneHotspot.h`) | `object`, `npc`, `fieldObject` in `src/Game/abortflag.c`. | Keep the typed scene-object variants. |
| `ExitCell.trap` (`include/Game/AreaMap.h`) | `damagePercent` and `alignmentMask` in `src/Game/treasurebox.c`. | Keep the two trap interpretations. |
| `Panel` flags (`include/Ui/Panel.h`) | `flags` in `src/Game/treasurebox.c`; `input.rightClick` there and in `include/Ui/Panel.h`. | Keep packed flag and bit view. |
| `SkillParameters` type (`include/Game/Skill.h`) | `type`, `kind`, `mode` in `src/Game/skilluse.c`; `kind` and `mode` also in `src/Script/recordcache.c`. | Keep packed byte and decoded bits. |
| `MenuContext` (`include/Ui/MenuBox.h`) | `value` in `src/Ui/menubox.c`; `script` in `src/Script/scripttext.c`; `item` in `src/Game/treasurebox.c`. | Keep distinct menu payloads. |
| `MenuBox.items` (`include/Ui/MenuBox.h`) | All eight views (`character`, `text`, `entries`, `table`, `systemTable`, `itemList`, `memberList`, `tag`) occur in `src/Game/`, `src/Script/scripttext.c` or `src/Ui/menubox.c`. | Keep the menu-specific payload types. |
| `MenuBox` flags (`include/Ui/MenuBox.h`) | `flags` in game/script callers; `flagBits.redraw` and `flagBits.repaintMode` in `src/Ui/menubox.c`. | Keep packed flag and bit view. |
| `FusionSummary` (`include/Game/Fusion.h`) | `value` and all four `fields` bits in `src/Game/fusion.c`. | Keep packed result and decoded bits. |
| `ItemStack` (`include/Game/ItemStack.h`) | Packed `value` in `src/Script/scriptactor.c`; all item/count/attachment fields in `src/Game/itemrecord.c`. | Keep packed script value and item fields. |
| `ItemSlot` (`include/Game/CharacterCore.h`) | Packed `value` in `src/Script/scriptactor.c`; `item`, `attachment`, `quantity` in `src/Game/itemrecord.c` and other equipment callers. | Keep packed equipment value and fields. |
| `TextAttr` (`include/Text/TextAttr.h`) | `value` in `src/Text/font.cpp`; `fg`, `bg`, `dim` in `src/Util/nibble.c`; `flags` in `include/Text/TextAttr.h`. | Keep packed text attribute and nibbles. |
| `ShotFile` (`include/Gfx/Shot.h`) | `table.offsets` and `bytes` in `src/Gfx/shot.c`. | Keep file header and byte-offset views. |
| `EffectCommand` (`include/Gfx/Motion.h`) | `opcode`, `frame`, `jump`, `parameter` in `src/Gfx/motion.c`. | Keep command variants. |
| `MotionFile` (`include/Gfx/Motion.h`) | `table.offsets` and `bytes` in `src/Gfx/motion.c`. | Keep file header and byte-offset views. |
| `ScriptScratchValue` (`include/Script/ScriptVars.h`) | `value`, `word`, `byte` in `src/Script/scriptvars.c`. | Keep VM operand-width views. |
| `ScriptPanelJump` (`include/Script/ScriptPanel.h`) | `value` and `parts.file`/`parts.entry` in `src/Script/scripttext.c`. | Keep packed jump and decoded parts. |
| `FlagBank` (`include/Script/EventFlags.h`) | `bits`, `words`, `sys.packed`, `sys.tag` in `src/Script/eventflags.c`. | Keep bank-wide and system views. |

## Remaining case: `MapCell`

`RunCellTrap` copies one `ExitCell` and reads only that view. The other seven `MapCell` members have no direct source consumer. A focused MSVC 5.0 control replacing the local `MapCell` with `ExitCell` changed its stack reservation from 16 to 12 bytes and moved every argument/local stack address in the exact retail body. The source was restored. A narrower union must preserve a supported 16-byte object without inventing trailing padding or using an unrelated member merely for size; neither is established yet. Resolve the local object's complete extent from retail stack writes and the `CopyExitAt` contract before changing this definition.

## Separate character and field-actor tails

`CharacterCore` contains the shared prefix. `Character` stores `alignmentA`
and `alignmentB` after that prefix; `FieldActor` instead stores map position,
direction and field state there. These are separate record types, rather than
alternative union members of `Character`.

`GetFieldActor` returns `FieldActor*`. Generic character operations borrow its
`core` member through `GetFieldActorCore`; they do not interpret field state as
roster alignment records. Whole-character allocation and serialization use
`Character`. The field actor's complete standalone extent and original type
name remain open, as recorded in `include/Game/FieldActor.h`.
