# Remaining Game macro literals

`giten verify constants --macro-list` inventories numeric spellings in macro
bodies. The following families in `include/Game` and `src/Game` retain numbers
because a matching semantic name or safe ownership boundary is unproven.

| Family | Examples and reason to keep the current value |
| --- | --- |
| Packed layouts and record offsets | `GetMapSpawnX` masks `MapSpawn.xLayer` with `0x7f` and `GetMapSpawnLayer` shifts by 7; the byte packs a seven-bit x coordinate and a layer bit. `GetItemDamagePower`, `GetItemSkillId`, and `ReadItemTargeting` address distinct `ItemRecord.params` bytes. These are format offsets, not enum members. The record layouts still need field recovery before those accesses can become typed members. |
| Direction arithmetic | `OppositeDirection(direction)` subtracts 2 and masks with 3. Although `ViewDirection` has members numbered 2 and 3, these operands are a half-turn and a modulo-four mask, not the south and west directions. |
| Local sentinels and limits | `ResetActionWaitDelay` stores `0xff` in a wait countdown; `InitFieldSkillCandidate` uses index `-1` and value `0x7fff`; `SetItemSlotItem` stores attachment `-1`. The observed meanings are known locally, but no shared named domain for these distinct fields is proved. |
| Calculation and layout constants | `IsCharacterHpLow` multiplies current HP by 5 for its threshold; `ClearLocationCaption` draws at `(8, 8)`. Those literals are a coefficient and coordinates rather than enum values. |
| View bounds across the C/platform boundary | `IsPointInWorldView` compares against `0x280` by `0x148`, matching the 640 by 328 view used by `SCREEN_WIDTH` and `VIEW_HEIGHT` in `include/Platform/Scene3D.h`. The game C translation unit does not include that platform header; moving dimensions to a shared owner needs a separate header ownership review. |
| Structural zero and one | `ClearItemStack`, `ClearActionWait`, and `ResetBattleTally` zero fields; several `do { ... } while (0)` macros use zero for statement wrapping. `IsActionWaitPickable` compares countdowns with one. These are storage resets, control structure, and numeric thresholds; equal spellings alone do not make them one enum. |
| Attack calculations | `include/Game/Attack.h` combines condition adjustments, clamps, and facing multipliers (`4`, `2`, `-20`, `20`, `100`, `1.5`, `1.2`). The values are arithmetic policy inside attack helpers. No current enum owns those quantities. |

This inventory is separate from the reviewed enum value ledger. A numeric
match to an unrelated enum member does not justify a replacement.

The object-like macro report contains source-written arithmetic too.
`WORLD_TRAVEL_SPAN` in `src/Game/worldtravel.c` computes a square grid's
width from `WORLD_TRAVEL_RADIUS`; `SIGHT_WIDTH` in `src/Game/fieldobj.c`
computes a sight-grid width from `SIGHT_RADIUS`; and the gun-burst limit
macros in `src/Game/partyaction.c` define target-count packing. These
numbers describe geometry or encoding, not enum members. `ROSTER_SIZE` and
`HUMAN_ID_LIMIT` are both 32 but denote roster capacity and the human-ID
cutoff respectively. Their equal value does not give them one owner.
