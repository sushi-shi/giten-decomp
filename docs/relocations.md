# Relocations

DDS.EXE is a `/FIXED` image with `IMAGE_FILE_RELOCS_STRIPPED` and no `.reloc`.
[config/retail/reloc_sites.tsv](../config/retail/reloc_sites.tsv) supplies the
reviewed absolute address fields used by the delinker and analysis tools.

## Site table

Each `site`, `target`, `origin` row names a four-byte linked absolute address:

| Origin | Evidence |
| --- | --- |
| `code-disp` | Decoded disp32 referencing code or data |
| `code-imm` | Decoded imm32 referencing an instruction start or data |
| `swept-code` | Data operand found by a linear code sweep |
| `text-table` | Aligned pointer outside decoded instructions in `.text` |
| `rdata-ptr`, `data-ptr` | Pointer stored in the corresponding data section |

Relative branches, import-directory RVAs and IAT slots are excluded. Correct
false/missing sites in this table, not through source workarounds. Matched CRT
COFF fixups validated admission; newly matched game bodies provide further
site and referent evidence. Absolute-zero weak externals are not relocation sites.

## Build image and imports

`giten.delink.reloc_image` copies `$GITEN_RETAIL_EXE` to `build/exe/DDS.EXE`,
appends the synthesized `.reloc`, updates PE metadata and clears the stripped
flag. Existing sections, bytes and RVAs stay in place. `$GITEN_EXE` names this
analysis copy.

Retail imports live in `.rdata`, with no `.idata` section. `giten.core.pe` uses
the IAT data-directory span; the pinned delinker's `vostok-iat-in-rdata.patch`
and synthetic PDB supply matching import identities.

## Data referents

Constants in `.text` belong to the data census/provider. Code and data starts
bound one another; the PDB emits named constants as data records in that section.
Library constants need no fabricated game storage.

Unprovided data referenced by claimed game functions becomes an `UNPROVISIONED_`
fence that strict delinking refuses. Relaxed mode emits `DAT_` and records the
same debt in `build/gen/data_debt.tsv`; see [data matching](build-system.md#data-matching).
For proven owner/addend expressions outside an object's extent, use
`config/retail/reloc_referents.tsv`. Data ownership and source pins are described
in [data attribution](data-attribution.md).
