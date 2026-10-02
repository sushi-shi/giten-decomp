# Resource codec execution

`giten codecs` follows Gruntz's `recomp/harness` approach: execute the retail
machine code and link the **actual candidate COFF objects**, then compare their
observable results. A third implementation lives in
[`tools/giten-codec`](../tools/giten-codec/README.md), a dependency-free,
allocation-free `no_std` Rust library. Agreement is an execution result on the
listed corpus, not a proof for all inputs or a replacement for byte matching.

## Run

Inside `nix develop`, set `export GITEN_DIR=$PWD` in the selected worktree:

```sh
giten codecs --disc /path/to/DDSWIN.BIN
```

Supply your own original Windows MODE1/2352 disc image. It is read in place;
no game resources are committed. Supply the retail executable and toolchain
using the repository [quickstart](../README.md#quickstart); the command reads
`GITEN_RETAIL_EXE` from the development shell. The executable on the disc must
match that supplied executable. All generated files live in `build/codecs/`,
with Rust build products in its `rust/` subdirectory.

The command builds only the required candidate objects with their manifest
flags, builds the VC5 runner, enumerates disc/PE resources, executes both native
implementations under Wine, runs Rust, and compares the result streams. It does
not bank scores or modify the game sources. Nonzero status means a mismatch,
missing family coverage, invalid/truncated result stream, or execution failure.

- `report.json`: per-family counts and resource-named counterexamples.
- `inventory.json`: every disc and PE resource, size, hash, classification,
  and explicit exclusions/substitutions.
- `cases.json`: stable job order, resource/frame identity, arguments and sizes.
- `provenance.json`: disc, retail, relocation image, source, candidate-object,
  harness, Rust executable, and job hashes, plus Git HEAD.
- `jobs.bin`, `retail.bin`, `rust.bin`: reproducible inputs and canonical outputs.
- `compile.log`, `link.log`, `retail.log`, `rust.log`: diagnostics.

The protocol reader rejects missing results, count mismatches and trailing
bytes. A failed rerun removes the old report first. Zero cases cannot pass.

## Entry points and observations

| Family | Retail entry RVA | What is compared |
|---|---|---|
| XOR records | `1b20`, `1b40`, `1b70`, `1ba0`, `1bf0` | Whole-record, byte, word and 17-byte chunk reads; return counts, stream position, payload and guards |
| Raw records | `1d90` | Payload, stream position and guards |
| Concatenated BMPs | `56b00` | Seek success, file position and reported size, including one past the chain |
| BMP surfaces | `56bd0`, `56de0` | 8-bit, RGB555 and RGB565 pixels; pitch padding, palettes, colour keys and COM call observations |
| DIB resources | `56f30`, `57120` | The same conversions with nonzero destination offsets |
| MIDS | `55c90`, `55d50`, `55e50` | Format, buffer count, initialized MIDIHDR fields and event bytes; compact conversion and bounded partial writes |
| Area maps | `21470` | Complete expanded destination with pointer fields normalized to offsets; untouched bytes retained |
| Item records | `22d40` and its helper closure | Every table entry's scalar fields, kind parameters and both encoded strings; pointer-return contracts |
| PCM WAVE | `488a0`, `489f0` | Buffer selection/capacity, resource-id lookup, sample bytes, zero-filled remainder, loop flags and lock/play arguments |
| Text tokens | `1aa0` | Packed character code and consumed byte count under code page 932 |

Original resource enumeration covers complete BMP chains, all embedded DIBs,
framed BIN records, the item table, area records, every MIDS file, and every
embedded WAVE payload. A length prefix does **not** prove encryption: framed
records are exercised through both raw and encrypted primitives, and the
inventory makes no encryption claim from framing alone.

Additional, separately named controls:

- MIDS event streams with stream-id words removed exercise the alternate
  compact grammar. Their deltas, event words and long payloads come from the
  original streams; their representation is transformed.
- Bounded MIDS truncation/capacity fixtures exercise failure returns and partial
  writes. These are not original resources.
- All two-byte combinations exercise text-token lead-byte and escape rules;
  original area names also exercise that reader.
- WAVE resources absent from the retail sound table are supplied at the
  resource API seam for sound 1, explicitly marked in the inventory.

Fixed script offset tables, legacy sound BINs, icons, menus, dialogs, executable
files, and other non-codec data remain classified in the inventory. This does
not execute the script VM, game startup, save-game state serialization, complete
sprite/scene loaders, physical graphics, or audio drivers. Allocation wrappers
and device-failure permutations are outside this corpus check. Most malformed
retail inputs would have undefined memory accesses and are not submitted.

## Runtime boundaries

The runner maps `build/exe/DDS.EXE`, the **existing delinking image** produced
from `config/retail/reloc_sites.tsv`, and applies its HIGHLOW relocation table.
The game entry point and static initializers are not run. Imports are bound to
Wine's APIs. No decoder entry point is replaced.

Explicit environment seams are shared between retail and candidate:

- Retail CRT `fread` and `fseek` forward to the runner's VC5 CRT, so FILE objects
  belong to one initialized runtime. Internal codec calls remain retail calls.
- `HandleReadPtr` supplies the caller-owned item table. The item lookup and
  record decoder themselves execute unchanged on both sides.
- DirectDraw SDK vtables supply deterministic memory surfaces and capture
  palette/colour-key operations.
- DirectSound creation and resource lookup/load/lock supply SDK memory buffers
  and the selected original WAVE bytes. The import binder handles both named
  `DirectSoundCreate` and DSOUND ordinal 1. Neither side plays physical audio.
- The CRT code page is set to 932 and its initialized `_mbctype` table is copied
  to retail, supplying the platform state needed by the inlined lead-byte test.

The runner links whole objects with `/FORCE:UNRESOLVED`, as Gruntz does. Unused
functions retain unresolved dependencies; reaching one faults and fails the
run. The linked codec closure uses real CRT/WinMM calls or the listed seams.
Compiler/linker logs retain unresolved-symbol diagnostics. This is deliberately
not a runnable whole-game candidate.

Both sides receive the same input and destination addresses. The harness checks
input immutability and initializes output buffers consistently. It compares
full active buffers, padding, return contracts and recorded API effects. Rust
uses the same canonical wire representation without host pointers. DIB row
padding is a known retail quirk: the custom converters advance by width, and
Rust preserves that behaviour rather than applying standard BMP stride rules. Some original WAVE headers
overstate the outer RIFF length; the game and the Rust WAVE reader use the
fixed PCM header and data-chunk length instead. The general RIFF iterator
continues to validate its declared bounds.

## Codec bounds check

```sh
CARGO_TARGET_DIR="$PWD/build/codecs/rust" cargo test --offline \
  --manifest-path tools/giten-codec/Cargo.toml
```

The retained check ensures truncated and oversized encoded records leave the
output buffer untouched. It does not assert that a reconstructed function's
machine-code bytes equal retail.
