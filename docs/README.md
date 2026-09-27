# Project documentation

Start with the root [README](../README.md) for setup and the build loop.

- [Build system](build-system.md) and [tooling map](tooling-map.md): commands and pipeline ownership.
- [Compiler profiles](compiler-flags.md), [linking](linker-flags.md), [toolchain setup](toolchain-vc50-sp3.md), [compiler identification](compiler-detection.md), and [relocations](relocations.md).
- [Match tracking](match-status.md), [permuter](permuter.md), and the small [compiler-pattern reference](patterns/INDEX.md).
- [Data attribution](data-attribution.md), [linked-image comparison](image-diff.md), [cleanliness](cleanliness-metrics.md), and [source markers](comment-markers.md).
- [clangd](clangd.md).

## Source of truth and storage

Tool inputs belong in `config/`; generated reports belong in ignored `build/`.
Documentation is neither a runtime input nor a maintained copy of generated state.
The retail labels in [config/retail](../config/retail/) were admitted once from
the investigation (`~/Projects/giten/investigation`: the Ghidra census, the
LIBC.LIB matches, the relocation synthesis) and are hand-managed since.

Generate the optional layout map with `python3 -m giten.sema.exe_map` inside
`nix develop`; output goes to `build/exe-map/`. It is a heuristic view of
current model attribution, not proof of original TU ownership.

## Provenance

The pipeline, gates, skills and most of these docs come from gruntz main at
`26bb3e9da` ([sushi-shi/gruntz-decomp](https://github.com/sushi-shi/gruntz-decomp)).
Links pinned to its commit `b27b05deb` are that project's historical evidence
for a mechanism (usually a VC5 codegen observation that holds here too), not
Giten evidence and not maintained instructions.
Keep new docs about ongoing usage and contracts; do not add another PR diary.
