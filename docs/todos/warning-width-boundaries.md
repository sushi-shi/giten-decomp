# C4761 width boundaries in Util, Gfx, and Script

This ledger covers the 28 configured C translation units under `src/Util`,
`src/Gfx`, and `src/Script`. A compile with each unit's `config/units.toml`
profile found 114 C4761 diagnostics in 13 units. Two source changes remove
five diagnostics; the remaining 109 sites are below. The other 15 units in
these directories had no C4761 diagnostic. The full 82-unit census remains
in [compiler-warnings.md](compiler-warnings.md).

`OpModifyEventFlagByValue` and `OpModifyEventFlag` now hold their operation in
the existing 16-bit `BitChangeMode` domain, matching the parameter of
`ChangeEventFlag` and the character-flag operation. `OpRollLongVar` now holds
its two bounds in `i16`, the input width of `RandomAverage`; the accumulator
remains `i32`. The two affected units each produce a COFF object identical to
its previous object after masking only the timestamp. Warning counts are
`eventflags.c` 12 to 9 and `scriptactor.c` 48 to 46, with no new warning class.

| Unit | Remaining C4761 | Unit | Remaining C4761 |
| --- | ---: | --- | ---: |
| `src/Util/range.c` | 3 | `src/Gfx/blit.c` | 4 |
| `src/Gfx/motion.c` | 1 | `src/Gfx/vramaccess.c` | 2 |
| `src/Script/eventflags.c` | 9 | `src/Script/scriptactor.c` | 46 |
| `src/Script/scriptctx.c` | 5 | `src/Script/scriptfield.c` | 3 |
| `src/Script/scriptswitch.c` | 10 | `src/Script/scripttext.c` | 2 |
| `src/Script/scriptvars.c` | 16 | `src/Script/scriptvm.c` | 3 |
| `src/Script/windowcolor.c` | 5 | | |

## Boundaries still requiring evidence

| Pattern | Concrete sites | What must be established before changing it |
| --- | --- | --- |
| A 32-bit script value enters a 16-bit gameplay API | `scripttext.c:170` (`CloseScriptPanelByImage`), `scriptfield.c:110` (`ExchangeFieldOption`), `scriptvm.c:677` (`PlayMusic`), `scriptactor.c:2337` (`SetReturnPoint`) | The opcode's operand range and the callee's ABI. A temporary `i16` created only to suppress C4761 has no domain value. |
| Object-reference arithmetic enters a 16-bit character API | `scriptctx.c:157,161,164,214,220`; `scriptactor.c:2491,2494,2526,2562,2974,2981` | Whether the encoded reference is narrowed before or after subtracting its range base; `ReadObjectRef` currently returns `i32` because it constructs base-offset references. |
| Small fields are promoted to `int` by arithmetic | `blit.c:48,86` (sprite offsets into 16-bit coordinates), `vramaccess.c:40` (region rows times eight), `scriptswitch.c:94,101` (alignment transformed into an 8-bit switch key), `scriptvars.c:259` (image position offsets), `scriptactor.c:2404` (wrapped direction) | The valid coordinate/key range and the exact narrowing point. Widening the callee would change its ABI. |
| Packed flag words are masked or shifted into 16-bit arguments | `eventflags.c:134,149,164,191,200`; `scriptvars.c:1312` (system-variable offset becomes a flag index) | The pack format establishes bank and index ranges, but the exact extraction boundary and retained sign/zero extension need COFF comparison. |
| Storage size is narrowed by an allocator or offset helper | `vramaccess.c:31` (`AllocCleared` takes a 16-bit size), `scriptvars.c:536,541,556` (`ShiftScriptEntries` and `OffsetBy` take 16-bit spans), `scriptactor.c:2164` (array-handle count) | Maximum encoded payload size. Silencing C4761 by widening the allocator or helper can change truncation and break the recorded layout. |
| A 32-bit getter feeds a 16-bit stored attribute | `windowcolor.c:71,84` (`GetTextPlaneNormalAttr`/`GetTextPlaneAccentAttr` to `SwitchWindowAttr`), `windowcolor.c:32,106,116` (script value to color attributes), `scriptvars.c:605,617,630,646` (text-plane settings) | The getter's full return contract and the high-word behavior of the attribute before changing its return type. |
| Random or byte-key result enters a narrower API | `range.c:106,115` (`RandomUpTo` and `RandomAverage`), `scriptswitch.c:82,87,106,121`, `scriptactor.c:830` (sample count) | Input range and call-time truncation. A trial moving `OpRollLongVar`'s sample narrowing into a new local changed its COFF object, so the direct call remains. |
| File offsets and dispatch values mix 16- and 32-bit expressions | `motion.c:508` (`OpenDataFile` offset), `scriptvm.c:80` (`CallScript` bank from opcode), `scriptactor.c:922` (`FindLayerScriptEntry`) | The file and opcode encoding bounds and the retail argument width. |

These are representative sites in each remaining group. The table above gives
the full per-unit count; use compiler output for the exact duplicate warning
sites on lines with multiple arguments.
