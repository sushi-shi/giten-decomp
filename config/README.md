# Configuration

Tracked inputs and reviewed retail evidence live here; generated state belongs
in ignored `build/`. Merge rows by identity and remeasure the merged tree rather
than accepting either side wholesale.

| Input | Owner / purpose |
| --- | --- |
| `units.toml` | `giten.manifest`: unit/source mapping and complete [compiler profiles](../docs/compiler-flags.md) |
| `compare.toml` | [Data-matching mode](../docs/build-system.md#data-matching) |
| `match_baseline.tsv` | CUR/MAX/HIST ledger; written by `giten verify bank` |
| `cleanliness/` | Audit floors and reviewed exceptions; use each owning gate's update command |
| `reviews/` | Reviewed findings consumed by the corresponding audit; recheck when source changes |

## Retail evidence

`retail/functions.tsv` and `retail/data.tsv` describe admitted starts and kinds.
Provider tables supply names and ownership; source macros supply reconstructed
claims. [The model](../scripts/giten/model.py) resolves precedence, extents and
violations. Table headers and their consuming modules define the schemas.

| Table in `retail/` | Purpose |
| --- | --- |
| `functions_static_libs.tsv` | Proven library function identities; low-confidence rows are leads |
| `data_static_libs.tsv` | Library data, SDK constants and GUIDs |
| `data_vtables.tsv` | Vtable identities |
| `data_compgen.tsv` | Header COMMONs and per-TU static copies; see [data attribution](../docs/data-attribution.md) |
| `functions_zlib.tsv`, `data_zlib.tsv` | Provider channels for pristine vendored source |
| `reloc_sites.tsv` | [Absolute relocation sites](../docs/relocations.md) absent from the retail PE |
| `reloc_referents.tsv` | Proven site-specific owner/addend expressions |
| `link_order.tsv`, `link_bands.tsv` | Reviewed contribution order and coarse image bands |

Correct a false or missing retail fact at its owning table; do not compensate
with fabricated source. A table row is not a source definition or independent
proof of a whole-object match.
