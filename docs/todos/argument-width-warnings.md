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
