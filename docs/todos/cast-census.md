# C source cast census

`giten verify c-casts` parses every C translation unit in `config/units.toml`
with the generated target-C compile database. It lists explicit C-style casts
written in `src/` and project headers, deduplicated by Clang spelling location.
A macro cast expanded by several callers appears once with every observed
source type, target type and unit/function context.

```sh
python3 -m giten.graph.compdb
GITEN_DIR=$PWD giten verify c-casts
GITEN_DIR=$PWD giten verify c-casts --kind pointer --scope source --list
GITEN_DIR=$PWD giten verify c-casts --path src/Game/clock.c --json
```

The audit writes `build/gen/c_casts.tsv` and
`build/gen/c_casts_coverage.json`; `--no-report` skips those derived files.
`--kind` selects pointer, scalar or mixed target categories, `--scope` selects
C source or headers, and repeated `--path` options select source paths.
`--max N` fails when the selected written-site count exceeds N. The C++
`reinterpret_cast` ledger is separate.

The command fails if a C source is absent from the manifest or compile
database, a compile database entry is stale or duplicated, libclang reports
a target-C error, or project source changes during the scan. The coverage
report lists every project header visited by C units and those not visited.
Unvisited headers are outside this audit: some are C++ only, and inactive
preprocessor branches cannot be inferred from a target-C AST. A site count
must not be read as complete header coverage without reviewing that list.
For example, the current C units do not include `Platform/D3DApp.h`,
`Sound/MidiStream.h` or `Text/FontApi.h`; their C++ casts belong to the
separate `giten verify casts` review.

## Per-site verdicts still needed

The current generated census has 142 written C cast rows: 96 pointer casts and
45 scalar casts in sources, plus one cast in a header. It records locations,
types and expansion contexts, but has no individual retain/remove/model-debt
verdict or owning function's current match state. The family reviews below
explain many boundaries; they do not finish a site-by-site review. For each
row, inspect the enclosing expression and callee, storage extent and alignment
where relevant, and the retail instructions and referents before deciding
whether the cast is necessary. The 48 C++ `reinterpret_cast` sites have a
separate review in `typed-boundaries.md` and are outside this C count.

In particular, the four `ItemStack*` conversions of `g_scriptVars` in
`src/Script/scriptactor.c:2091-2097` need a verdict tied to the script-variable
storage extent and the item-list callees. The two `i16*`/`i32*` reads from the
record byte buffer in `src/Game/fieldobj.c:2305-2307` need their offset,
alignment and encoded-width contracts checked separately. `DecodeAreaMap` in
`src/Game/clock.c:549-571` rebases several serialized offsets into typed
records; each target array needs its own extent and alignment evidence. None
of these examples is a proven removable cast merely because its spelling is
present in the census.

## Source-model review still open

The C-source pointer audit started with 97 written sites across 68 C units.
`IsRegionFlagOn` now passes its byte list to `IsCellFlagSet`, which accepts a
generic record pointer and views its bytes inside the owner function. The
affected C objects remain byte-identical. The remaining 96 source sites are
grouped below; the command above supplies each current line, source type,
target type and expansion context.

| Source sites | Current boundary and evidence needed |
| --- | --- |
| `src/Game/character.c`, `fieldobj.c`, `skilluse.c`, `treasurebox.c`; `src/Script/scriptactor.c`, `scriptctx.c` | `FieldObject.kind` and the script actor pointer are viewed as `Character*` and `FieldActor*`. The latter has only a partial layout; e.g. `DistanceToParty((FieldActor*)g_curScript->actor)` and `GetFieldActor` use the same storage for different fields. Recover the shared object layout and all whole-object users before replacing these views. This family also includes two packed field-value reads and four script-var `ItemStack*` views, which need separate storage proofs. |
| `src/Game/clock.c` | `DecodeAreaMap` rebases offsets from a serialized `u8*` record into typed `AreaLevel` lists; `CopyExitAt` copies the complete packed `ExitCell` through byte views. A decoded in-memory type does not replace the file-format view. Prove each source record's extent and alignment before removing its conversion. |
| `src/Script/scripttext.c` | Script values and long vars carry menu pointers, handles, strings or numeric arrays in integer slots. For example, `OpAddMenuLine` converts `ReadScriptValue()` to `MenuBox*`, while `GetRecordDataOffset` views record bytes as 16-bit offsets. Recover the value tag and handle owner before claiming one pointer type. |
| `src/Game/fieldmain.c`, `worldtravel.c`, `treasurebox.c`; `include/Game/AutomapData.h` | Named accessors turn generic handle payloads into event-state bytes, `MapCoord` route points and automap tables. These are real allocation/read boundaries; check all writers and the allocated size before moving the cast or changing the payload type. |
| `src/Game/itemrecord.c`, `statestack.c`, `treasurebox.c`; `src/Gfx/displayconfig.c`, `motion.c`, `shot.c` | Byte or word views cross packed item text, scene cells, palette words, GUID registry bytes, effect-script offsets and the shot table. Examples: `strcpy(..., (char*)src)`, `StartEffectScript((u8*)record, record->script)`, and `(Body*)(s_shotData.bytes + offset)`. The source bytes or Win32 API type define the boundary; confirm format and alignment before replacing a cast. |

The remaining source-site counts by unit are: `character` 1, `clock` 22,
`fieldmain` 1, `fieldobj` 10, `itemrecord` 2, `skilluse` 1, `statestack` 1,
`treasurebox` 5, `worldtravel` 2, `displayconfig` 1, `motion` 2, `shot` 1,
`scriptactor` 32, `scriptctx` 1 and `scripttext` 14. These total 96.

`IsCellFlagSet` now accepts a generic record pointer, but it cannot validate
the offset without a list extent. Cell callers use offsets 3, 6 or 7; `MarkRegionList`
passes offsets after a rectangle or marker and stops at the `ff ff` sentinel.
The allocation length and bounds for every serialized room list remain to be
recovered before adding a checked span or claiming the offset domain complete.
`SpawnLevelObjects` retains `(u8*)cell` when calling `AddAreaNpc`: that callee
walks all eight bytes of the `ObjectCell`. Passing `&cell->head.x` would start
from the one-byte member rather than convert the complete record to its byte
representation.

## Scalar conversion review

The same target-C scan found 49 written scalar casts before this pass. Two
`u16` action-wait casts in `src/Script/scriptactor.c` and two `u16` casts on
the already `u16` game phase and step getters in `src/Game/partyaction.c`
were redundant; their focused MSVC objects stayed byte-identical. The 45
retained source casts occur in these units:

| Units | Sites | Units | Sites | Units | Sites |
| --- | ---: | --- | ---: | --- | ---: |
| `character` | 5 | `fieldmain` | 3 | `fieldobj` | 7 |
| `fieldscreen` | 1 | `fieldview` | 3 | `itemrecord` | 1 |
| `statuspanel` | 1 | `treasurebox` | 3 | `motion` | 1 |
| `shot` | 1 | `vram` | 1 | `pool` | 1 |
| `recordcache` | 3 | `scriptactor` | 3 | `scriptctx` | 1 |
| `scriptop` | 1 | `scripttext` | 4 | `scriptvars` | 3 |
| `range` | 2 | | | | |

| Conversion family | Current example and reason to retain |
| --- | --- |
| Floating point to integral | `CalcMaxHp` and `CalcMaxMp` in `character.c`, `RoundToInt` in `range.c`, `OpSqrtLongVar` in `scriptactor.c`, and the shot/status drawing calculations select truncation at that expression. Remove only if the rounding and destination range are proved equivalent. |
| Narrow before later arithmetic | `recordcache.c` narrows a product to `i16` before dividing; `fieldobj.c` narrows a coordinate difference before dividing; `treasurebox.c` narrows map area before adding seven and dividing. Moving the cast to the final assignment changes overflow and division behavior. `scripttext.c` narrows a record offset before pointer addition. |
| Protocol bytes and encoded bits | `fieldmain.c` and `fieldobj.c` store directions and object ids in bytes; `fieldobj.c` casts a complemented flag bank to `u8` before shifting, so sign extension cannot leak into the result. `fieldview.c`, `scriptop.c`, `scriptvars.c` and `vram.c` similarly extract or pass encoded byte values. Check the full domain before replacing a cast with a wider temporary. |
| Signedness and comparison range | `character.c` converts an armor bonus and a pool cost to `u16`; `scriptctx.c` widens a code word to `u32` before shifting by 16. `DrawDownCountdown` in `scriptvars.c` narrows only after comparing with a `u16` amount. |
| Pointer values stored as integers | `fieldscreen.c` returns a `WordList**` as `u32`; `scripttext.c` stores menu pointers in script long vars; dead `AllocatePool` in `pool.c` treats a pool pointer as a handle. Recover the caller and storage ABI before replacing an integer slot with a pointer type. |
| Codegen-sensitive redundant casts | `ReadScriptBlock` in `scriptvars.c` reads one item, so `(u16)fread(...)` cannot change its value; `LoadNpcTexture` in `treasurebox.c` masks mode to `0` or `0x80`, so its `u32` cast cannot change the following shift. `UseObjectSkill` in `fieldobj.c` computes at most 49149 from a `u16` maximum, so its `u16` cast cannot change the value. Removing each changed a previously stronger retail match; retain their present spelling. |

Rerun the audit command above after source changes to obtain each current
scalar site with its original and target types.

A lower cast count is a navigation signal, not proof of a better source model.
Match the affected instructions, call signatures and referents before keeping
a typed replacement. The [KF1 target-C cast audit](https://github.com/sushi-shi/kings-field-decomp/blob/master/docs/cast-audit.md)
provides the historical method behind this worklist.
