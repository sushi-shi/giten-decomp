# Argument-width warning review

An MSVC 5.0 SP3 `/W4` compile reports 107 C4761 argument-size diagnostics in
the four reviewed Game units. The production compile uses its pinned flags;
the warning-level compile here was only a source audit. C4761 marks an
arithmetic expression promoted to `int` before a narrower parameter. It
does not by itself prove that the callee type is wrong.

| Unit | C4761 sites | Current boundary and next evidence |
| --- | ---: | --- |
| `character.c` | 14 | `ClampTo999`/`ClampTo100`, `AlignmentClass`, and `DrainUpkeep` take `i16`; `SetConditionAge` takes `u8`; `AllocCleared` takes `u16` sizes. For example, `CalcMagicPowerStat` passes `stats[STAT_MAGIC] + amount` to `ClampTo999`; the compiled clamp reads a word argument. Keep the original call conversion until whole-range evidence supports a wider helper contract. |
| `fieldobj.c` | 15 | `LoadLayerScripts` and `OpenDataFile` take 16-bit resource IDs while callers add table bases; `TraceSight` and `StepObjectTowardParty` take 16-bit directions; `FindObjectAt` takes a 16-bit start index. The event-flag operation in `RemoveFieldObject` derives a clear/set bit from packed `flagBank`. Confirm encoded ranges and the retail word loads before changing these contracts. |
| `statuspanel.c` | 43 | Most sites compute text/menu pixel or row coordinates (`x + 2`, `x * 8`, `y + 3`) for drawing APIs with `i16` coordinates; others compute item/slot indices. `DrawStatCompare` already takes narrow coordinates, while `DrawPlaneText` uses a wider `i32` text attribute. Prove a drawing API's wider parameter from its body/callers before retyping it. |
| `treasurebox.c` | 35 | Menu/hotspot indices and map offsets feed 16-bit APIs; `GetAutomapLevelTableSize` and `GetAutomapBitmapSize` feed `CreateArrayHandle(u16 count, u16 size)`. The allocator's 16-bit count is a real limit, so widening it would change storage behavior. Range validation for malformed map data is separate work. |

The `character.c` trial replaced all 14 call warnings with typed local
initializations. It preserved the whole `.text` section, but the compiler
then emitted C4244 at those initializations. The trial was reverted because
it changed where a warning appears without improving the source model.
Generic casts would hide the same boundaries; callee widening would change
word-sized semantics. The current reviewed source keeps these conversions
visible at their calls.

The `fieldobj.c` flag-bank expression also contains a C-style cast. Its
low-byte complement is shifted to choose the `BitChangeMode` clear/set
value. That representation needs separate bit-level source recovery; replacing
it only to silence C4761 would obscure the packed flag rule.

## Further Game call sites

Four more Game units have 75 C4761 diagnostics under the same `/W4` compile.
Their reviewed arithmetic feeds narrow, established game interfaces; no
callee width change or warning-free, COFF-preserving source correction is
supported by the current evidence.

| Unit | C4761 sites | Current boundary and next evidence |
| --- | ---: | --- |
| `fieldview.c` | 25 | `FloodViewCells` and `ViewCellHasWall` take 16-bit view-grid columns and rows. Calls such as `ViewCellHasWall(x, y, dir, col + 1, row + 1, side, width, height)` promote the two coordinates to `int`; the grid itself is only seven by four cells. The facing `dir` and wall `side` remain 32-bit by the current contract. |
| `fieldscreen.c` | 14 | `RevealAutomapCells(cell, y + 1)` steps a 16-bit map coordinate; `LoadWorldMapBlockImage(block + 10, 5)` passes a neighboring 16-bit block index; `HitTestHotspot(id, x, y, flags & 1)` passes a one-bit strictness value to an `i16` parameter. `ReadWorldMapTileCode` similarly receives local coordinates after arithmetic. Prove a wider storage or API contract before changing them. |
| `statestack.c` | 27 | `NextGamePhase`/`PrevGamePhase` and step/sub-step wrappers pass arithmetic to 16-bit state setters. `AddItemUseMenuLine` builds a 16-bit packed text attribute for `AddMenuLine`; `AdvanceClock(steps * 5)` passes minutes to an `u16` interface. The state storage, text-cell attribute and clock amount are narrow domains. |
| `partyaction.c` | 9 | `FindPickablePartyMember(index + 1)`, `RandomAverage(0, count - 1, 0)`, and menu-row `index + 1` use narrow indices. `OppositeDirection` computes a value 0..3 for `SpawnFieldObject`'s 16-bit direction; `AccessScriptReg(1, 0, 1 - s_fieldLeftEarly)` writes a 16-bit script value. The expressions are bounded by their protocol, while the promotion is ordinary C arithmetic. |

Retyping these narrow APIs to `int` would change their source contracts;
inserting narrow temporaries only moves the diagnostics to assignments, as
the `character.c` trial showed. The reviewed source is unchanged.

## Final Game call sites

A final `/W4` pass found 27 more C4761 diagnostics in four Game units, plus
the 15 already counted for `fieldobj.c`. The source-supported width for each
callee remains narrow; no call rewrite satisfied both warning reduction and
byte-identical COFF. The two same-type `u16` casts around state getters in
`partyaction.c` were removed separately with identical COFF; they were not
C4761 sites.

| Unit | C4761 sites | Current boundary and rejected rewrite |
| --- | ---: | --- |
| `fieldmain.c` | 16 | `AdvanceClock(RandomUpTo(4) + 3)` supplies 3..7 minutes to an `u16` clock interface. Cell-event arithmetic supplies a `u16` game step, `OffsetMapCoord` receives a signed 16-bit lateral offset, and `RotateByDirection(wall, direction + 3)` receives a 16-bit direction. A new temporary would only move the conversion into an initializer. |
| `skilluse.c` | 6 | `LaunchShot` receives the short vertical difference `to.y - from.y`; `RandomUpTo(lastIndex)` receives an 8-bit RNG bound; `OffsetMapCoord` receives short offsets `across - 3` and `along - 3`. `ClampTo999(base * 4)` keeps the helper's 16-bit input conversion. Wider callee types would change these established interfaces. |
| `skillattack.c` | 3 | `WearSkillValue(skillValue + accuracy)` and the two other worn-power calls feed a 16-bit input. `WearSkillValue` in `src/Script/recordcache.c` reads that input as `i16` before its percentage and 1..30000 clamp. Widening the input would change the calculation; narrowing a new local would add C4244. |
| `itemattack.c` | 2 | Both `WearSkillValue` calls similarly pass computed item power to its 16-bit input. The same reason keeps the conversions at the calls. |
| `fieldobj.c` | 15, counted above | Rechecked resource IDs, map directions, object indices, RNG bounds and the packed flag-bank mode. The earlier table and flag-bank note give the specific sites; no new width evidence supports a change. |
