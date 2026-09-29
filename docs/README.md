# Documentation

[Quickstart](../README.md#quickstart) · [Contributor rules](../AGENTS.md)

- [Build and comparison](build-system.md): pipeline, banking, data-matching modes, gates.
- [Compiler profiles](compiler-flags.md) and [toolchain evidence](compiler-detection.md).
- [Relocations](relocations.md) and [data attribution](data-attribution.md).
- [Retail libraries and SDK helpers](vendor-libraries.md).
- [Resource codec execution](codecs.md): retail/candidate/Rust comparisons on original resources.
- [Candidate linking](linker-flags.md) and [clangd](clangd.md).
- [Playing](play.md): the bug-fixed image and its Wine runtime; [bugs](bugs.md): the retail defects and their fixes.
- [Generated branches](branches.md): the `source`, `classic` and `port` branches.
- [Permutation experiments](permuter.md) and [compiler patterns](patterns/INDEX.md).
- [Enum domains](enum-domains.md): declaring and typing proven value domains.
- [Source markers](comment-markers.md), [todo ledgers](todos/README.md), and [configuration](../config/README.md).

Keep command options in `--help`, schemas beside their implementation, inputs
in `config/`, and generated reports in `build/`. Git holds retired experiments.
Commit-pinned Gruntz links in the pattern references are historical compiler
evidence, not Giten source evidence or current instructions.
