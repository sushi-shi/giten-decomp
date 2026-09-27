# Relocations

DDS.EXE was linked `/FIXED`: `IMAGE_FILE_RELOCS_STRIPPED` is set and there is
no `.reloc` section. Everything that needs the image's absolute address operands
(the delinker, `giten sema`, the data and relocation gates) reads them from
[config/retail/reloc_sites.tsv](../config/retail/reloc_sites.tsv) instead,
through a build copy of the image.

## The table

`site	target	origin` - one row per 4-byte field that holds a linked absolute
address (what a `.reloc` HIGHLOW entry would name), with the target it stores
and how the site was found:

| origin | meaning |
|---|---|
| `code-disp` | a decoded instruction's disp32 naming `.text`, `.rdata` or `.data` (tables in `.text` included) |
| `code-imm` | a decoded instruction's imm32 naming an instruction start, `.rdata` or `.data` |
| `swept-code` | an operand in code the census never decoded (linear sweep), data targets only |
| `text-table` | an aligned dword outside decoded code in `.text` (jump/pointer tables) |
| `rdata-ptr`, `data-ptr` | an aligned pointer stored in `.rdata`/`.data` |

The IAT and the import directory are excluded (they hold RVAs, never
relocated). Relative branches never appear: the delinker decodes them itself.

The table is MANUALLY MANAGED after admission, like `functions.tsv`: a false
site (a constant that happens to fall in the image range) is removed here, a
missed one added here. Never work around a wrong site in source or in a tool.

## Admission and validation

The table was admitted by the investigation's
`tools/admit_config.py` from the Ghidra census of the image. Ground truth
exists for part of the image - the uniquely matched VS97 SP3 `LIBC.LIB` bodies
carry their own COFF fixups - and admission requires the synthesis to reproduce
every DIR32 fixup of those bodies with no extra site inside them (768 fixups,
0 missed, 0 extra at admission). DIR32 fixups against absolute-zero weak
externals store 0 in the image and get no base relocation; they are not sites.

As game units match, their base objects' own relocations are further evidence:
a function at 100% whose operands carry the same referents as retail confirms
every site inside it.

## The build copy

The `reloc_image` edge (`scripts/giten/delink/reloc_image.py`) writes
`build/exe/DDS.EXE` = the pristine image (`$GITEN_RETAIL_EXE`, from the nix
store) plus the sites as a genuine `.reloc`: one base-relocation block per page,
a section header appended after `.rsrc`, data directory 5 set, SizeOfImage grown
and RELOCS_STRIPPED cleared. No existing byte, section or RVA moves.
`$GITEN_EXE` names this copy, so every consumer reads relocations exactly as it
would from an image that kept them.

## Imports inside .rdata

The same link merged the import tables into `.rdata`; there is no `.idata`
section. `core/pe.py` takes the IAT span from the data directory
(IMAGE_DIRECTORY_ENTRY_IAT), and the pinned delinker carries
`nix/patches/vostok-iat-in-rdata.patch`: with no `.idata`, the IAT gets a
synthetic section id past the real ones, which the synthetic PDB mirrors for the
`__imp_` symbols, and an IAT target is classified before the section arms.

## Data identities

Proven constants in `.text` belong in the data census and their data provider,
not the function census. Code and data starts cap each other's extents in that
section. The synthetic PDB emits named constants as `S_LDATA32` records in the
executable section. The delinker resolves exact references to these records as
external data symbols before looking up a preceding function or import thunk;
it does not infer interior addends for untyped PDB data. Library-owned constants
need no reconstructed game storage.

The delinker refuses to emit an `UNPROVISIONED_` fence - an unnamed datum that
game code references. The fence is scoped to sites inside functions a unit
claims (`delink/pdb_synth.game_site_test`): unclaimed code is delinked into the
synthetic `FUN_` buckets objdiff never pairs, while every datum a RECONSTRUCTED
function references must be provided (a `DATA()` claim, a library label, or a
string the base object also emits). With `data_matching = false` the fence is
spelled `DAT_` instead and the target is listed in `build/gen/data_debt.tsv`
([build-system](build-system.md#data-matching)).
