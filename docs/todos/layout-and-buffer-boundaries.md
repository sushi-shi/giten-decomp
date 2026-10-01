# Layout and buffer-boundary review

The pinned Gruntz [padding audit](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/padding-audit.md)
removed fields only after deletion preserved the complete owner layout and all
uses. Its [lifetime review](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/constructor-lifetime-cleanup.md)
kept even empty special members when they emitted required calls or vptr stores.
These are the current Giten candidates inspected under those rules. All Giten
units use `/Zp1`, so no named byte field below is automatic alignment inserted
by the compiler. A name containing `pad` or `reserved` does not prove that its
bytes are semantically empty.

## Constructed objects and aggregate copies

| Candidate | Evidence and disposition |
| --- | --- |
| `CMMChunk::CMMChunk()` in `Sound/Mmio.h` | The only empty in-class constructor found in the project headers. It has a distinct three-byte retail function at `0x0568c0`, exact in the current source, and `CMMIdChunk`/`CMMTypeChunk` construct the base under the platform `/Ob0` profile. An implicit constructor would remove that body and change the derived call set. Retain it. `CIme` and `CMidiStream` have nonempty lifetime bodies. |
| `Character` core copies in `CopyCharacterCore` (`character.c`), `PreviewEquipChange` (`statuspanel.c`), and the box copy in `treasurebox.c` | Each copies only the prefix ending at `offsetof(Character, alignmentA)`, then handles skill storage or recalculation separately. A whole `Character` assignment would change the copy extent and pointer ownership. Recover a typed prefix only after all whole-object uses establish the same boundary. |
| `TreasureBoxCell` and `MapPosition` in `PrepareViewedTreasureBox` (`treasurebox.c`) | The function copies two bytes from a box cell and a ten-byte `MapPosition` by value into adjacent statics. The source already notes that retail placement may be one record, but nothing reads the leading bytes or uses such a record whole. Adjacency alone does not establish an aggregate type or ownership. |
| `s_shotMotion = *motions` in `StartShot` (`shot.c`) | This is already a typed whole-`Body` copy. It does not justify replacing the distinct partial copies above. |

## Named layout spans

The search covers all 45 non-comment field declarations whose names match
`pad*`, `reserved*`, `m_reserved*`, or `unknownAfterTraining` under `include/`.
Every candidate is listed here; the group evidence explains why deletion has
not been established. Check the retail record stride, all offsets and whole
object operations together before replacing a span with typed members.

| Owner | Candidate fields | Current boundary and next evidence |
| --- | --- | --- |
| `Game/AreaMap.h` | `CellKind.pad03`; `BattleCell.pad09`; `LinkCell.pad09`; `TreasureBox.pad03`, `pad08`, `pad0d`; `ScriptCell.pad08`; `DoorCell.pad05` | Map-cell lists are file records walked by kind and terminator. The bytes sit inside fixed record strides, including unexplained destination and flag positions. A reader/writer or file-format witness is needed to name them. |
| `Game/AutomapData.h` | `AutomapBitmapHeader.pad04`; `AutomapLevelHeader.pad02` | Bitmap sizes and level-table handles are copied or allocated as complete stored records. Deletion changes the `size` and `levels` offsets under `/Zp1`. |
| `Game/ObjectRecord.h` | `ObjectRecord.pad69`, `pad6e`, `pad75` | `ReadObjectRecord` and `InitObjectFromRecord` use a fixed 0x7a-byte file record. The intervening bytes need field identity from complete record decoding. |
| `Game/GameState.h` | `MapPosition.pad06`, `pad09` | `MapPosition` is ten bytes and is passed by value and saved as part of `FieldState`. Removing either byte changes the ABI or saved extent. |
| `Game/AreaNpc.h` | `AreaNpc.pad00`, `pad1a`, `pad26` | The 42-byte NPC record is loaded and its leading 0x14 bytes are also passed whole to the draw path. Some bytes remain unnamed; removing them changes later script and texture offsets. |
| `Game/FieldObject.h` | `FieldObject.pad000`, `pad02d`, `unknownAfterTraining`, `pad07f`, `pad1e1`, `pad211`, `pad219`, `pad21c` | The character-like middle and the late script/event region have typed readers on either side. The three bytes after `trainingPoints` are currently unreferenced, but removing them moves `dropChance` and later fields. Identify them from record copies and retail displacements before naming or shrinking. |
| `Game/Character.h` | `Character.pad037`, `pad058`, `pad065`, `pad07c`, `pad07e`, `pad1c7` | The character prefix is copied to `alignmentA`; field offsets and this copy extent are live. An absence of direct named reads does not make an interior byte removable. |
| `Game/WorldMap.h` | `WorldMapBlock.reserved`; `WorldMapEvent.pad04` | The block word is cleared and swapped, so it is written storage. The event byte precedes flag and script fields in a map record. |
| `Platform/GameCalls.h` | `MarkerColor.reserved` | The marker palette is four-byte records with a 16-bit colour in each. The second word's role is unknown; removing it would change the array stride. |
| `Platform/DeviceSettings.h` | `D3DCaps.reserved` | `DeviceSettings` is a binary registry record. The byte separates three device-mode byte arrays from `driverCaps` under `/Zp1`; its record role is unrecovered. |
| `Platform/D3DApp.h` | `D3DDeviceInfo.reserved1`, `reserved2` | These are unrecovered prefix and tail spans around several DirectDraw and Direct3D descriptors; they are too large to be alignment. Establish the complete queried-device record before replacing them. |
| `Sound/MidiStream.h` | `CMidiStream.m_reserved1`, `m_reserved2`, `m_reserved3` | Both constructors explicitly initialize `m_reserved1` and `m_reserved3`; removing any member shifts `m_volume` and later fields. The middle array has no named source reader. Recover identities from member displacements and complete constructor/destructor use. |
| `Gfx/Motion.h` | `EffectRecord.pad00`, `pad0a`, `pad0e` | Effect records are read as binary blocks, then indexed by fixed palette, script, shot and motion offsets. The unnamed spans remain part of that record. |
| `Ui/Panel.h` | `Panel.pad10` | The variable `rows` follow this four-byte span. Removing it moves the row array and changes panel indexing. |

## Fixed buffers and read lengths

`ReadRawBlock` (`datafile.c`) reads a file-supplied 16-bit length and then
copies that many bytes to a `void*`; the API receives no destination capacity.
All six reconstructed callers below pass fixed storage without a length check.
The loader body and the shot/motion loops have banked exact matches, so adding
clamps would change their observed behavior. Recover the original file limits
or a caller-side precondition before changing this shared API.

| Caller | Destination capacity | Remaining bound question |
| --- | ---: | --- |
| `LoadMoonTable` (`statestack.c`) | `s_moonTableBuffer`: 0x280 bytes | Is the moon table resource always at most 0x280 bytes? |
| `LoadLayerScriptSet` (`fieldobj.c`) | `s_scriptSetBuffer`: 0x50 bytes | Does the table file length stay within 0x50? |
| `LoadEncounterWeights` (`fieldobj.c`) | `s_encounterBuffer`: 0x100 bytes | Does the encounter table length stay within 0x100? |
| `LoadMotionTable` (`motion.c`) | `g_motionFile`: 16 bytes | Does the block count fit the seven `MotionTable.paths` pointers, and do all offsets point inside the loaded bytes? The loop uses the file count unchecked. |
| `LoadEffectPalettes` (`motion.c`) | `s_paletteData`: 64 four-byte records (0x100 bytes) | Does the file length stay within the array, and is its first record present for `ApplyEffectPalette`? |
| `LoadShotTable` (`shot.c`) | `s_shotData`: 0x100 bytes | Does the file count fit the 63 `ShotTable.kinds` pointers, and do all offsets point inside the block? The loop uses the file count unchecked. |

Other buffer and initialization operations need the same source-faithful treatment:

| Operation | Concrete issue and next step |
| --- | --- |
| `UpdateFieldHud` (`fieldscreen.c`) | Four source checks of `g_leftFrontWalls[-along][0]` and `g_rightFrontWalls[-along][0]` can reach row 4 when `along == -4`, although both arrays have only four three-byte rows. Retail uses one byte load per side (`0x015e1c` and `0x015f4d`), then compares or tests the loaded byte for the two checks on that side. At row 4, the left load addresses `0x09121c`, in the unclaimed four bytes before `g_centerFrontWalls` at `0x091220`; the right load addresses `0x0912fc`, the first byte of `g_destinationY` immediately after `g_rightFrontWalls`. The arrays' admitted extents are `0x091210..0x09121b` and `0x0912f0..0x0912fb`. Trace the retail branch preconditions and the left gap's storage owner before choosing a source model. A bound guard or larger array would change the observed read or invent storage; preserve the exact function until that evidence is available. |
| `PlaySoundEffect` (`d3dapp.cpp`) | `buffer->Lock` returns `data1` with `size1`, yet the exact retail-matching body clears `capacity` bytes through `data1` before copying `size1` sample bytes. The lock could return a shorter first segment. Do not replace the count without comparing the actual retail instructions and the DirectSound buffer/lock preconditions. |
| `StoreAutomapLevel` / `LoadAutomapLevel` (`treasurebox.c`) | Both copy `header.size + sizeof(AutomapBitmapHeader)` bytes between a handle and the fixed `AutomapBitmap.bits[0x1000]` buffer; the size comes from the stored bitmap and the load truncates the result to `u16`. Establish the file/handle extent and the header size range before adding a bound. |
| `InitDirect3D` (`d3dapp.cpp`) | Four `memset(&desc, sizeof(desc), 0)` calls pass zero as the count. The source comment records the retail no-op setup. Correcting the argument order would add writes absent from the retail behavior; inspect that function's remaining 99.5451% wall separately. |
