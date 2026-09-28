# giten-codec

Allocation-free Rust readers for the original Windows Giten resources. The
library is `#![no_std]`, forbids unsafe code, and has no dependencies. The
optional `oracle` binary uses `std` to drive differential execution jobs.

| Module | Interface |
|---|---|
| `crypt` | Ciphertext-feedback XOR records; incremental keys and inverse encoding |
| `bitmap` | Borrowed BMP/DIBs, concatenated BMP iteration, palettes, 8/16-bit blits |
| `riff` | Bounded RIFF chunks with odd-length padding |
| `mids` | Borrowed music buffers, stream-id insertion, partial-write errors |
| `wave` | PCM format fields and borrowed sample bytes |
| `area` | Area names/levels and pointer-free expansion of the Windows map layout |
| `item` | Kind-dependent parameters and borrowed item names/descriptions |
| `text` | Code-page-932 character tokens, including game escape bytes |

```rust
use giten_codec::crypt;

let encrypted = [3, 0, 3, 1, 2];
let mut plain = [0; 3];
let (length, rest) = crypt::decode_record(&encrypted, &mut plain)?;
assert_eq!(plain, [0, 2, 3]);
# Ok::<(), giten_codec::Error>(())
```

Strings remain encoded bytes. `text` returns the game's packed character codes;
it does not translate Shift-JIS to Unicode. The bitmap reader deliberately
retains the game's width-based row stepping, including on padded original DIBs.
The area expansion writes normalized offsets into a caller-provided buffer;
ordinary callers can use the borrowed view without constructing that layout.

Malformed or unsupported resource headers return errors. This does not promise
compatibility with retail's invalid-pointer or out-of-bounds behaviour. The
`mids::convert` API does preserve its bounded partial writes before errors.
`crypt::encode_record` is a convenient inverse; retail has no resource encoder
to compare against.

Build without the host driver:

```sh
CARGO_TARGET_DIR="$PWD/build/codecs/rust" cargo check --offline \
  --manifest-path tools/giten-codec/Cargo.toml --lib --no-default-features
```

Run the original-resource comparison from the repository root inside
`nix develop`, with `GITEN_DIR=$PWD`:

```sh
giten codecs --disc build/codecs/DDSWIN.BIN
```

See [the execution contract](../../docs/codecs.md) for coverage, environment
seams, provenance, and interpretation of results.
