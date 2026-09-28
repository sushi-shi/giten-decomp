# Compiler patterns

[The index](INDEX.md) collects bounded observations for MSVC 5.0 SP3.
They help interpret output; matching bytes do not uniquely recover source.

- Prefer correcting an existing entry. New entries need a distinct mechanism,
  recognizable signature, reproducible evidence, and stated limits.
- Separate measurements, historical reports and source hypotheses. Failed
  searches do not prove impossibility; one success is not a universal rule.
- Keep scores, campaign logs, failed-spelling lists and per-function verdicts
  out of this reference. Use Git history for retired experiments.
- Link to tool/build contracts instead of copying them. Commit-pinned sibling
  project links are historical evidence, not current Giten instructions.
