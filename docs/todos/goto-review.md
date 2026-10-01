# Shared control-flow joins

There are 39 written `goto` statements in ten source functions. This is a
review inventory, not a ban: a join may express the source operation or the
MSVC 5 control-flow shape. Replace one only when the resulting source models
the same action and the focused retail comparison supports the change. A
change to an exact function needs instruction and ordered-relocation parity;
a partial function needs the first CFG and referent divergence inspected.

| Function | Gotos | Current join | Next review |
| --- | ---: | --- | --- |
| `src/Game/character.c` `SortRoster` | 1 | `next` skips to the next roster entry | Its source comment records that a `for` loop rotates the search and changes the layout; retain this edge unless another structured form preserves that evidence. |
| `src/Game/fieldobj.c` `RunObjectStep` | 3 | `attack` unifies action modes before attack execution | Trace each mode's state updates and the common attack entry. |
| `src/Game/fusion.c` `CreatePairFusionCharacter` | 3 | `createCharacter` is the shared constructor arm of three summary kinds and the default | Check whether grouped `case` labels keep the same fall-through and calls. |
| `src/Game/itemrecord.c` `DecodeItemRecord` | 1 | `readItemMessages` joins record-kind decoding | The source already records a retail-specific separation of final parameter reads; compare the two arms before changing this edge. |
| `src/Game/partyaction.c` `RunPartyCommandInput` | 4 | `target_selected` joins menu and command selection | Verify which paths have already changed the selected target and which still need setup. |
| `src/Game/skilluse.c` `RunBattleAction` | 6 | `nextTarget`, `complete` and `done` join nested target loops and battle cleanup | Keep target advancement, result reporting and final teardown separate in the retail CFG. |
| `src/Game/treasurebox.c` `GetCellTrapDamage` | 1 | `done` applies the common percentage scaling to known and default trap kinds | Its source comment records that an early zero return changes the shared signed division; keep that path in any rewrite. |
| `src/Gfx/shot.c` `StepShot` | 4 | `moving` and `arrived` select bounds checks versus a shared clamp | Its source comment records the forward-depth edge and two bounds exits as a codegen constraint; any rewrite needs that evidence checked again. |
| `src/Script/scriptactor.c` `ReadTextToken` | 12 | `readIndexedToken` and `readTokenValue` share operand reads among token kinds | Try grouped case labels only if both token families retain their distinct byte reads and call order. |
| `src/Ui/hotspotclick.cpp` `ClickHotspotAt` | 4 | `nearerGreater` and `nearerLess` share directional depth comparisons | Preserve selected/candidate coordinates and tie policy for all four directions. |

The written-site inventory can be refreshed with
`rg -n '^\s*goto\s+' src include --glob '*.{c,cpp,h}'`. This source search
found no header goto; it is not an AST proof that conditional preprocessing
makes every site active. The source and retail CFG decide each disposition.
