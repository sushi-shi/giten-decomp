# Banked exact function differences

These functions reached an exact match in an earlier source or translation-unit
state, but their current object differs. Each offset is the first differing
byte in `giten walls diagnose <rva>`. The diagnosis groups instruction shapes;
it does not prove that branch destinations or effects agree. Compare the
banked source and current owner translation unit before changing a proven type
or helper solely to recover register allocation.

| Function (retail RVA) | Current evidence and next check |
| --- | --- |
| `LoadBitmapToSurface16` (`0x056bd0`) | First difference `+0xb8`; both sides have 174 instructions, three calls, ten branches, two returns and six relocations. Trace the bitmap scanline cursor and the owner TU changes after the exact bank. |
| `CalcMagicEvasionStat` (`0x03d510`) | First difference `+0x7` in an 11-instruction, one-call body with the same control-flow and referent counts. Compare the stat helper argument and return register schedule with its exact bank. |
| `CalcWeaponPowerStat` (`0x03d3f0`) | First difference `+0xf`; both sides have 36 instructions, two calls and one branch. Trace the weapon parameter lifetime before changing the calculation. |
| `MultiplyMatrix` (`0x046230`) | First difference `+0x13`; 82 instructions and nine calls agree. Compare matrix operand evaluation and the owner TU's floating-point helper state with the exact bank. |
| `PushAutoMove` (`0x012760`), `PopAutoMove` (`0x0127c0`) | First differences `+0x35` and `+0x3d`; each retains its banked call, branch, return and relocation counts. Review the shared auto-move stack access and later fieldmain TU changes together. |
| `GetRaceClass` (`0x010080`) | First difference `+0x17` near the end of a seven-instruction, one-call body. Check return-width handling against the exact bank and its caller contract. |
| `PickWorldEncounterGroup` (`0x0113a0`) | First difference `+0x42`; the current object has one extra instruction, while five calls, three branches, two returns and eight relocations agree. Check the group selection's local lifetime before changing its domain types. |
| `GetFusionRaceEntry` (`0x028380`) | First difference `+0x33`; the current object has one fewer instruction, with three calls, three branches, three returns and four relocations on both sides. Compare the race-entry lookup's final return path with the exact bank. |
| `GetTripleFusionSummary` (`0x027c30`) | First difference `+0x3a`; the current object has one extra instruction, with seven calls, ten branches, two returns and nine relocations preserved. Trace the summary accumulator and helper boundaries. |
| `InheritFusionStats` (`0x0270b0`) | First difference `+0x4`; both sides have 47 instructions, three calls and four branches. Compare source and destination stat pointer lifetimes with the exact bank. |
| `SelectRandomFusionDemon` (`0x026f90`) | First difference `+0x22`; both sides have 47 instructions, six calls and four branches. Check random-bound and selected-entry scheduling against the banked source. |
| `SetSceneFlags` (`0x004920`) | Retail loads the flag word, ORs in the 32-bit argument and stores its low half, using two references to the same global. Current MSVC emits one `or word ptr` and one reference. The exact bank used `volatile`, later removed because no asynchronous owner or observer was found. Restore a real observer only if retail evidence supports one. |
| `OffsetByWord` (`0x00b830`) | Both bodies have six instructions, no calls or branches, and the same pointer arithmetic. Retail zeroes EAX and reads the unsigned offset word through ECX before loading `base` into ECX; current MSVC loads `base` into EDX before reading the word. A named offset local, reversed pointer addition, and reordering the `Range.h` includes compiled to the same current bytes. The source expression itself was exact before the typed `ViewDirection` declarations entered this translation unit; retain those supported declarations and seek a genuine translation-unit boundary that restores the retail load order. |
| `TestModeFlags` (`0x004380`) | Retail loads the global into EAX before the argument into ECX; current MSVC loads the argument first, then the global into ECX. Both have four instructions and one global reference, while the current encoding is one byte longer. Reversing the `&` operands compiled byte-identically. Removing only the `WorldMapRequest` enum definition from the current `WorldMap.h` in a disposable replay restored the exact 12-byte function; the old whole header did too. The old `FieldMain.h`, `EventFlags.h`, and `PlatformApi.h` individually, moving only the `Enums.h` include, and moving the redraw-global declaration to its actual owner did not. The current typed enum and object were restored. Retain the supported domain despite this unrelated MSVC TU-state dip. |
| `PickDdsSummon` (`0x017520`) | First difference `+0x126`; both sides have 140 instructions, 21 calls, ten branches, eight returns and 41 relocations. Compare the shared summon-cost inline helper and the statestack TU state with the exact bank. |
| `DrawStatList` (`0x042450`) | First difference `+0x8a`; both sides have 70 instructions, five calls, three branches, two returns and ten relocations. Trace the stat-row cursor and panel helper expansion. |
| `FindRegionData` (`0x01ecc0`) | First difference `+0x4d`; both sides have 48 instructions and seven branches, with no calls or relocations. Compare region record pointer/index lifetimes. |
| `NormalizeAffiliations` (`0x01c830`) | First difference at entry; the current object has one fewer instruction, with 12 branches and one return on both sides. Compare initial register setup and table traversal against the exact bank. |
| `UpdateAutomapScrollPanel` (`0x01d890`) | First difference `+0x54`; both sides have 139 instructions, seven calls, 19 branches and 35 relocations. Trace the panel row and scroll position locals through the first call. |
| `CellToField` (`0x00d650`) | First difference `+0x6`; the current object has two fewer arithmetic instructions and no calls or branches. Compare coordinate expression grouping and conversion precision with the exact bank. |
| `UnprojectPoint` (`0x00d6b0`) | First difference `+0x6`; the current object has one extra arithmetic instruction and no calls or branches. Compare transform expression grouping and conversion precision with the exact bank. |
| `ShadeMesh` (`0x04ae30`) | First difference `+0x93`; both sides have 156 instructions, ten calls, ten branches, three returns and 26 relocations. Trace mesh lighting operand lifetimes and the WinMain TU's helper composition. |

The other current dips from exact banks have deeper notes in the Game, Script,
fieldview and non-Game matching ledgers linked from this directory's README.
