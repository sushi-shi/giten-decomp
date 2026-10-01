# C4761 width boundaries in Util, Gfx, Script, and selected Game units

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

## Six Game units

A second focused compile audited `src/Game/worldtravel.c`, `fusion.c`,
`savegame.c`, `gameloop.c`, `equipeffect.c`, and `itemrecord.c` using their
configured MSVC profiles. Initially they had 39 C4761 diagnostics: 14, 8,
4, 1, 5, and 7 respectively. `OpenSystemMenu` in `savegame.c` now takes an
`i16 count`: it forwards that count only to `SetMenuItems(i16 itemCount)`.
The compiled COFF object is unchanged after timestamp masking, and its
warning count falls from 4 to 2 without a replacement warning. The six units
retain 37 diagnostics. These boundaries remain:

| Unit | Written warning sites and boundary evidence |
| --- | --- |
| `worldtravel.c` | `:184` subtracts historical coordinates from a current `i16` position (two arguments); `:250,253,256,259` combines `i16` cell coordinates with row/column offsets (two arguments on each line); `:284,287,297,300` passes `row ± 1` to the 16-bit recursive scanner. The travel-grid range is bounded, but the callee ABI and promotion point are established by the current call shape. |
| `fusion.c` | `:337,553,1461,1488` pass promoted level arithmetic to `ClampLevel(i16)`; `:681` passes promoted stat arithmetic to `ClampTo100(i16)`; `:1345` adds a resource base to the 16-bit `OpenDataFile` ID; `:1624` passes promoted display coordinates to `DrawPlaneText`; `:1643` adds an icon bias to the `DrawPlaneIconKeyed(i16)` icon argument. The 16-bit inputs are existing API boundaries, while `ClampLevel` itself returns `i32`. |
| `savegame.c` | `:239` passes the stored `i32 s_systemEntryCount` into the now 16-bit `OpenSystemMenu` count parameter; its storage width cannot be changed from this call alone. `:251` adds a menu-row offset to a phase value before `SetGamePhase`. |
| `gameloop.c` | `:182` chooses the literal 31 or 24 in an `int` conditional expression passed to `AdvancePlayTime(i16)`. The values fit; no typed domain is recovered from the choice alone. |
| `equipeffect.c` | `:48,57,66,75,84` halve a stat total using promoted arithmetic before `ClampTo100(i16)`. `GetStatTotal` reads the stored stat, and `SetStatTotal` stores the clamped result; an intermediate cast or local only to suppress C4761 would obscure the call-time width. |
| `itemrecord.c` | `:751` computes a variable allocation size for `AllocCleared(u16,u16)`; `:2093,2094` add a loop offset to special-item IDs before 16-bit calls (two diagnostics at `:2093`); `:2234,2235` add gift familiarity and item-base offsets before 16-bit calls; `:2268` adds a menu index to `s_giftItemBase` for `GetLoadedRecordName(i16)` inside `sprintf`. The allocation maximum and item-ID ranges need proof before changing these expressions. |

## Debug menu and area/clock owner

`src/Game/debugmenu.c` retains three diagnostics. At `:124` and `:180`, an
`i16` selected menu row plus `MENU_STEP_PICK_FIRST` is promoted to `int`
before `SetGameStep(u16)` or `SetGameSub(i16)`. At `:207`, unary minus promotes
the stored `i16 s_shotRise` (cycled through 0..3) before passing it as the
`i16` along offset of `MoveMapCoord`. The menu steps and shot distance are
bounded, but changing their storage types would change data layout; inserting
a temporary solely for warning suppression would add no recovered domain.

`src/Game/clock.c` was reviewed read-only because its area/clock owner is being
modeled in a separate lane. Its 13 C4761 diagnostics split as follows:

| Site | Retained boundary |
| --- | --- |
| `:172,177` | Moon-phase comparisons yield `int` 0/1, then enter the 16-bit `BitChangeMode` argument of `ModifyEventFlag`. The boolean meaning is clear; the exact operation width remains the callee ABI. |
| `:391,396` | `GetMapSpawnX` masks a packed byte using `& 0x7f`, yielding `int` for a 16-bit map coordinate. `:396` also passes the spawn index to `SpawnMapObject`'s 8-bit event argument. These are the packed spawn-record boundary. |
| `:591` | `i + 1` is promoted before entering the 16-bit level index of `GetAreaLevelOffset` in the area-record copy length. This lies inside the owner whose packed layout is under review. |
| `:911` | `g_areaLevel->width * y + x` is promoted before the 16-bit room-bit index of `TestBit`. The map dimensions and bitmap extent must be considered together. |
| `:1241` | A variable panel allocation size enters `AllocCleared`'s 16-bit size argument. This static inline helper is expanded at three callers, producing three diagnostics at one written site. The maximum row count is required before changing the allocation expression. |
| `:1266,1285,1297` | `first + i` or `image + i` is promoted before the 16-bit row ID of `InitPanelRow`. Each table's ID domain and the packed panel-row ABI need to remain consistent. |

## Input, Math, Mem, and Text gap

The four remaining non-Game units outside the first audit had 13 C4761
diagnostics. `src/Input/mouse.c` had two: its private inline
`LatchMouseButtonClick` received the `int` result of a button-mask `&` in
an `i16` parameter, then used that result only as a condition. It now takes
`b32 pressed`. The complete COFF object is identical after timestamp
masking; both C4761 diagnostics disappear with no replacement warning.
The other three units retain 11 diagnostics:

| Unit | Written warning sites and retained boundary |
| --- | --- |
| `src/Math/vec3.c` | `:31,36,41` shift each signed 8.8 fixed-point velocity component by eight. C promotes the shift result to `int` before `ClampDelta(i16 delta)`. The velocity storage is `i16`; its post-shift value fits the parameter, while widening `ClampDelta` would affect callers beyond this unit. |
| `src/Mem/handle.c` | `:50,97` pass `u32` requested sizes to `AllocCleared(u16 size)`; `:59,109` pass `u32` sizes to the `u16` stored size in `SetHandleEntry`/`SetHandlePtr`; `:108` passes a `u32` size to `ReallocBlock(u16 size)`. `HandleEntry.size` is a 16-bit field and the heap wrappers take 16-bit sizes, so silently widening one declaration would change the handle and allocation contracts. Prove maximum requested size and overflow behavior before changing them. |
| `src/Text/windowtext.c` | `:87` converts the text-column pixel expression `x * 8` to `DrawTextCell(i16 px)`; `:91` passes the high byte `ch >> 8` to `StoreTextCell(u8 byte)`; `:94` passes the whole `u16 ch` to that same byte writer for a single-byte character. The writer stores one byte and a 16-bit attribute in parallel rows; these conversions are part of the character-cell encoding. |

## Complete warning report reconciliation

The completed `giten verify compiler-warnings` report compiled all 82 configured
units. Its 392 rows are 382 C4761 argument-width warnings in 36 units, one
C4133 packed-pointer warning at `clock.c:569`, and nine retained C4805 boolean
comparisons. Every C4761 unit and count agrees with the table in
[compiler-warnings.md](compiler-warnings.md), and every C4761-bearing unit
appears either in this ledger or in
[argument-width-warnings.md](argument-width-warnings.md). The generated rows
for `vec3.c:31,36,41`, `handle.c:50,59,97,108,109`, and
`windowtext.c:87,91,94` agree with the retained sites above; `mouse.c` has
no remaining warning. The generated TSV is the precise current-site list
when source or signatures change.
