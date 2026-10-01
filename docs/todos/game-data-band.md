# Game data band

The HoMM3 data workflow separates retail byte coverage, source declarations,
comparison enrollment, and reference identity. Giten now has a lighter complete
partition of the retail initialized `.data` band (`0x068000..0x071200`) and its
zero-filled tail (`0x071200..0x092b54`):

```sh
giten verify data-coverage --all-bytes
```

The generated `build/gen/data_coverage_image.tsv` accounts for every byte in
those bands as `model`, `section-only`, or `unknown`, and separately says whether
the claim was enrolled in delink comparison. A `model` row means a typed
source/model claim covers the address; it is not independent proof of the
original declaration's extent. An `unknown` row preserves the bytes even when
they are zero or sit between two named objects. The separate
`data_coverage_gaps.tsv` joins interior unknown runs to retail payload,
relocations, and decoded accesses. Dynamic indexed accesses expose their base
address, not every possible register-derived address.

| Retail band | Modelled bytes | Unknown bytes |
| --- | ---: | ---: |
| Initialized `.data` | 24,129 | 13,247 |
| Zero-filled `.bss` tail | 131,368 | 6,188 |

These are accounting counts, not a percentage of correctly reconstructed game
data. Both bands also contain CRT/library objects. In particular, the large
unclaimed initialized spans at `0x06e994`, `0x070be8`, and `0x070c70` lie among
library claims; they are not evidence of missing game declarations.

Of the unknown runs adjacent to Game claims, 66 have a decoded retail touch.
All are zero-filled, have no relocation into them, and are at most six bytes
long. Most are two-byte runs touched by a wider load from the preceding
declared word. That observation does not by itself establish a new object or
prove the run is padding.

An access-map join found no direct game-code target inside an unknown run. Two
indexed targets land two bytes before the next declared table:
`CanFloodViewCell` uses `s_viewFloodMask[row - 1][col]` at `0x0684e6`, and
`PlaySoundEffect` uses `s_soundResources[sound - 1]` at `0x06a6c6`.
The large referenced unknown runs from `0x06e300` onward and the BSS tail
`0x0919fe..0x092b54` have only CRT/library readers in the current map. Their
bytes remain unknown; this pass found no supported missing game definition.

## Game-adjacent gaps and boundary reads

| RVA and length | Retail evidence | Current disposition |
| --- | --- | --- |
| `0x06814b+37` | Nonzero bytes between the debug-menu candidate data section and `s_quitArea`; no retail relocation, direct decoded touch, or reference into the run. | Owner and object boundaries unknown. |
| `0x0682fb+5` | Contains `00 3c 00 00 00` between data-file and message claims; no reference into the run. | Owner unknown. |
| `0x068302+14` | Contains two `0x40` bytes after the message candidate section and before save-game data; no reference into the run. | Owner unknown. |
| `0x09121c+4` | Zero bytes after the 12-byte `g_leftFrontWalls` claim; the HUD's indexed read may reach this address at row four. | Keep separate from the array until retail control flow and storage ownership prove its meaning. |
| `0x0912fc+2` | `g_destinationY` starts immediately after the 12-byte `g_rightFrontWalls` claim; the same HUD index can read its first byte. | Real adjacent global, not wall-array storage. |

The three nonzero initialized gaps have no evidence-backed source owner, so a
`DATA` declaration would manufacture identity. The zero-filled game-adjacent
gaps likewise do not become padding or array tails merely because they are
zero. Continue from retail reference sites, complete producer/consumer use,
and the candidate object's emitted section before adding or enlarging a
declaration. The [layout review](layout-and-buffer-boundaries.md) records the
wall access boundary in more detail.

This complete partition is limited to `.data` and the BSS tail. HoMM3 also
accounts for `.rdata` and distinguishes independently proven retail extents
from source claims; those two broader checks remain open in Giten.
