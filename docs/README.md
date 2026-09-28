# Documentation

[Quickstart](../README.md#quickstart) · [Contributor rules](../AGENTS.md)

- [Build and comparison](build-system.md): pipeline, banking, data-matching modes, gates.
- [Compiler profiles](compiler-flags.md) and [toolchain evidence](compiler-detection.md).
- [Relocations](relocations.md) and [data attribution](data-attribution.md).
- [Candidate linking](linker-flags.md), [runtime investigation (WIP)](runtime-validation.md), and [clangd](clangd.md).
- [Permutation experiments](permuter.md) and [compiler patterns](patterns/INDEX.md).
- [Source markers](comment-markers.md), [todo ledgers](todos/README.md), and [configuration](../config/README.md).

Keep command options in `--help`, schemas beside their implementation, inputs
in `config/`, and generated reports in `build/`. Git holds retired experiments.
Commit-pinned Gruntz links in the pattern references are historical compiler
evidence, not Giten source evidence or current instructions.
