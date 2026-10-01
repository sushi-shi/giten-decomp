# Short initialized-data claims

`giten.verify.placement.short_data_claims()` compares the size of a source
`DATA` object with the span to the next start in `config/retail/data.tsv`.
That span is an upper bound from the census, not proof that one source object
owns every byte. Do not enlarge a declaration to absorb unrelated bytes.

## Unresolved rows

| Source claim and retail row | Evidence | Next step |
| --- | --- | --- |
| `g_worldTravelTerrainFlags` in `src/Game/worldtravel.c`, 16 bytes at `0x0644f8`; census row 72 bytes to `0x064540` | The first 16 retail bytes equal the initializer. The sole relocation into the row is `LoadWorldTravelCandidates` at `0x011acf`, to the base. Its index comes from `ReadWorldMapTileCode`, which returns only 0, 2, 4, 5 or 15. The tail starts `ff ff 00 00` and later contains nonzero 16-bit values including 6, 2, 4, 3, 5 and 1; no interior relocated target identifies an owner. | Identify the trailing bytes from an independent data or source witness before adding a census start or a new object. The 16-byte flag table is the supported extent. |
| `s_messageWindow` in `src/Ui/message.c`, a two-byte handle at `0x068300`; census row 16 bytes to `0x068310` | Retail starts `ff ff 00 00 40 00 00 00 40 00 00 00 00 00 00 00`. Seventeen relocated code sites in the message functions target the base, with none into the tail. The current `i16` model compiles to 15/15 exact message functions. The two 64-valued dwords have no established field identity or owner. | Recover the origin of the two dwords, or another independent boundary, before splitting the census row. Preserve the two-byte handle declaration. |

## Resolved boundary

`s_shotRise` at `0x068050` is a two-byte initialized counter (`3`), followed
by two bytes of alignment. The bytes at `0x068054` are the Shift-JIS label
“ＢＧＭ”, and the first debug-menu entry at `0x0641f2` has a relocation directly
to that string. `config/retail/data.tsv` now starts a `string` row at
`0x068054`, so the counter's census span ends at four bytes. The source
counter remains two bytes and the debug-menu functions remain exact.
