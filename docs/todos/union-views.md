# Union view review

King's Field's [cast, union and goto review](https://github.com/sushi-shi/kings-field-decomp/blob/master/docs/patterns/cast-union-goto-review.md) checks every union member against its consumers before removing a view. This is the corresponding source review for Giten's `src/` and `include/` tree. A union with several used views is a model of one packed value or serialized payload; the review does not establish that the original source spelled a union.

There were 20 written union definitions at the start of this review. The unused `Character` field-position view was removed, leaving 19. Every remaining definition and its direct consumers are listed below.

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
| `ItemSlot` (`include/Game/Character.h`) | Packed `value` in `src/Script/scriptactor.c`; `item`, `attachment`, `quantity` in `src/Game/itemrecord.c` and other equipment callers. | Keep packed equipment value and fields. |
| `Character` alignment B (`include/Game/Character.h`) | `alignmentB` in `src/Game/character.c`; `fieldHidden` in `src/Script/scriptactor.c`. | Keep roster/actor overlap. |
| `TextAttr` (`include/Text/TextAttr.h`) | `value` in `src/Text/font.cpp`; `fg`, `bg`, `dim` in `src/Util/nibble.c`; `flags` in `include/Text/TextAttr.h`. | Keep packed text attribute and nibbles. |
| `ShotFile` (`include/Gfx/Shot.h`) | `table.offsets` and `bytes` in `src/Gfx/shot.c`. | Keep file header and byte-offset views. |
| `EffectCommand` (`include/Gfx/Motion.h`) | `opcode`, `frame`, `jump`, `parameter` in `src/Gfx/motion.c`. | Keep command variants. |
| `MotionFile` (`include/Gfx/Motion.h`) | `table.offsets` and `bytes` in `src/Gfx/motion.c`. | Keep file header and byte-offset views. |
| `ScriptScratchValue` (`include/Script/ScriptVars.h`) | `value`, `word`, `byte` in `src/Script/scriptvars.c`. | Keep VM operand-width views. |
| `ScriptPanelJump` (`include/Script/ScriptPanel.h`) | `value` and `parts.file`/`parts.entry` in `src/Script/scripttext.c`. | Keep packed jump and decoded parts. |
| `FlagBank` (`include/Script/EventFlags.h`) | `bits`, `words`, `sys.packed`, `sys.tag` in `src/Script/eventflags.c`. | Keep bank-wide and system views. |

## Remaining case: `MapCell`

`RunCellTrap` copies one `ExitCell` and reads only that view. The other seven `MapCell` members have no direct source consumer. A focused MSVC 5.0 control replacing the local `MapCell` with `ExitCell` changed its stack reservation from 16 to 12 bytes and moved every argument/local stack address in the exact retail body. The source was restored. A narrower union must preserve a supported 16-byte object without inventing trailing padding or using an unrelated member merely for size; neither is established yet. Resolve the local object's complete extent from retail stack writes and the `CopyExitAt` contract before changing this definition.

## Removed view: `Character` alignment A

The anonymous `Character` union had `AlignmentInfo alignmentA` and an alternate struct with `fieldPosition`, `facing` and `fieldStateReserved`. No source consumer read those alternate members. The field actor's canonical owner, `FieldObject`, already declares `pos` and `direction`. `Character` now has one `AlignmentInfo alignmentA` field at the same position and extent. The focused MSVC object compiles; the type-declaration change shifts some codegen in `character.c`, so affected units need a full matching pass when this branch is integrated. The source identity improvement stands independently of that compiler-state effect.
