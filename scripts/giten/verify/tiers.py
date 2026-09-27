"""giten.verify.tiers - the tier registry `giten verify check --tier` runs.

  fast    text/ledger checks (board, bans, casts, enum domains, label style,
          include order) - seconds, no build artifacts.
  normal  model/layout joins (unique names, library overlap, tu order, data
          tu order, undefined closure, data identity, the wall-review count
          certifications) - needs bindings + base/target objs.
  full    binary-evidence audits (vtable tier, alloc size, assert relocs,
          data relocs, caller-callee, the retail data-access map + the
          claim-side coverage census) - compiles nothing but reads many
          objs / the retail image / clang IR + libclang record layouts.
  link    candidate-EXE audits (sections, image diff, link defects) - needs
          `giten link`'s candidate image.

Default = fast+normal (what the graph's check edge runs); full/link opt in.
Every gate returns FINDINGS (a list of strings); a non-empty list fails the
gate, and the runner never writes anything - blessing a baseline is each
module's own manual verb. A gate may instead return a `Verdict`, whose
`advisory` findings are printed with their count but do not fail it: that is
how `data_matching = false` (config/compare.toml) turns the data-placement
gates into worklists (`DATA_MATCHING_GATES`).
"""

from __future__ import annotations

import time
from typing import NamedTuple


class Verdict(NamedTuple):
    """A gate result with a failing half and an advisory (listed) half."""
    findings: list
    advisory: list


def _data_matching() -> bool:
    from giten.core import data_matching
    return data_matching.enabled()


def _placement(findings: list) -> list | Verdict:
    """A data identity/placement gate's findings: failing with data matching
    on, advisory with it off."""
    return findings if _data_matching() else Verdict([], findings)


def _board():
    from giten.verify import board
    return board.gate()


def _bans():
    from giten.verify import bans
    return [f"[{label}] {p}:{ln}: {tok}"
            for label, p, ln, tok in bans.scan()]


def _casts():
    from giten.verify import casts
    return casts.gate_findings()


def _compiler_artifacts():
    from giten.verify import compiler_artifacts
    return compiler_artifacts.gate_findings()


def _enum_domains():
    from giten.verify import enum_domains
    fatal, _warn, _decl = enum_domains.audit()
    return fatal


def _label_style():
    from giten.verify import label_style
    return label_style.violations()


def _source_encoding():
    from giten.verify import source_encoding
    return source_encoding.gate_findings()


def _claim_size():
    from giten.verify import claim_size
    return claim_size.gate_findings()


def _include_order():
    from giten.verify import include_order
    dupes, preludes, unordered, _manual, _changed = include_order.audit()
    out = []
    for rel, d in sorted(dupes.items()):
        out.append(f"duplicate include(s) in {rel}: {', '.join(d)}")
    for rel, w in sorted(preludes.items()):
        out.append(f"header missing prelude {rel}: {', '.join(w)}")
    for rel in unordered:
        out.append(f"include block out of canonical order: {rel}")
    for edge in include_order.layering_violations():
        out.append(f"library header includes a consumer: {edge}")
    return out


def _unique_names():
    from giten.verify import unique_names
    bad, _n = unique_names.findings()
    return bad


def _library_overlap():
    from giten.verify import library_overlap
    bad, _n = library_overlap.findings()
    return bad


def _tu_order():
    from giten.verify import tu_order
    findings, _summary = tu_order.gate_findings()
    return findings


def _data_tu_order():
    from giten.verify import data_tu_order
    return _placement(data_tu_order.gate_findings())


def _undefined_closure():
    # The placeholder-extern debt is advisory while data matching is off and
    # failing while it is on; the module decides the split.
    from giten.verify import undefined_closure
    return Verdict(*undefined_closure.gate_verdict())


def _data_identity():
    # Identity conflicts fail in BOTH modes: the relaxation hides them from
    # objdiff, it does not make them debt.
    from giten.verify import data_identity
    return data_identity.gate_findings()


def _dead_code():
    from giten.verify import dead_code
    return dead_code.gate_findings()


def _review_claims():
    from giten.walls import recheck
    return recheck.gate_findings()


def _vtables():
    from giten.verify import vtables
    return vtables.gate_findings()


def _alloc_size():
    from giten.verify import alloc_size
    return alloc_size.gate_findings()


def _assert_relocs():
    from giten.verify import assert_relocs
    return assert_relocs.gate_findings()


def _data_relocs():
    # The referent rows compare data-section pointers against retail's:
    # data identity. The unscored-unit / orphan-payload rows are scoring
    # integrity and stay failing in both modes.
    from giten.verify import data_relocs
    referents, integrity = data_relocs.gate_parts()
    if _data_matching():
        return referents + integrity
    return Verdict(integrity, referents)


def _caller_callee():
    from giten.verify import caller_callee
    return caller_callee.gate_findings()


def _data_access():
    from giten.verify import data_access
    return data_access.gate_findings()


def _data_coverage():
    from giten.verify import data_coverage
    return _placement(data_coverage.gate_findings())


def _link_tier():
    from giten.verify import link_tier
    return link_tier.gate_findings()


TIERS: dict[str, list[tuple[str, object]]] = {
    "fast": [
        ("board", _board),
        ("vtable-bans", _bans),
        ("casts", _casts),
        ("compiler-artifacts", _compiler_artifacts),
        ("enum-domains", _enum_domains),
        ("label-style", _label_style),
        ("source-encoding", _source_encoding),
        ("claim-size", _claim_size),
        ("include-order", _include_order),
    ],
    "normal": [
        ("unique-names", _unique_names),
        ("library-overlap", _library_overlap),
        ("tu-order", _tu_order),
        ("data-tu-order", _data_tu_order),
        ("dead-code", _dead_code),
        ("undefined-closure", _undefined_closure),
        ("data-identity", _data_identity),
        ("review-claims", _review_claims),
        # The data-VALUE gates run by default: a wrong datum leaves the code
        # byte-identical, so nothing else on the default path can see it.
        # ~10s total; the slow full-tier gates stay opt-in.
        ("data-relocs", _data_relocs),
        ("data-access", _data_access),
        ("data-coverage", _data_coverage),
    ],
    "full": [
        ("vtables", _vtables),
        ("alloc-size", _alloc_size),
        ("assert-relocs", _assert_relocs),
        ("caller-callee", _caller_callee),
    ],
    "link": [
        ("link-tier", _link_tier),
    ],
}

DEFAULT = ("fast", "normal")

#: Gates whose verdict follows the switch. The first three exist to prove
#: data identity or placement and are advisory while data matching is off
#: (data-relocs keeps its scoring-integrity rows failing); undefined-closure
#: moves only its placeholder-extern rows. data-access is NOT here: its gated
#: categories (width, stride, undercount, shortfall, adjacent, import-slot)
#: are instruction-level facts about claims that DO exist, and need no
#: identity for the data a unit has not claimed yet.
DATA_MATCHING_GATES = ("data-tu-order", "data-coverage", "data-relocs",
                       "undefined-closure")


def parse_tiers(spec: str | None):
    if not spec:
        return list(DEFAULT)
    if spec == "none":
        return []
    names = [t.strip() for t in spec.split(",") if t.strip()]
    for t in names:
        if t not in TIERS:
            raise SystemExit(f"unknown tier {t!r} (pick from "
                             f"{', '.join(TIERS)}, or 'none')")
    return names


def _rerun_command(name: str) -> str:
    """`giten verify <gate>` - the spelling that actually reaches the module.

    Never derive it from the tier label: the `vtable-bans` row runs
    giten.verify.BANS, so `python3 -m giten.verify.vtable_bans` (the old
    mechanical name.replace) names a module that does not exist.
    """
    from giten.verify import _ALIASES, _GATES
    verb = _ALIASES.get(name, name)
    if verb in _GATES:
        return f"giten verify {verb}"
    for gate, module in _GATES.items():
        if module.rsplit(".", 1)[-1] == verb.replace("-", "_"):
            return f"giten verify {gate}"
    return f"giten verify check --tier {name}"


def run(tier_names, *, max_findings: int = 12) -> int:
    """Run every gate in the named tiers; print a verdict per gate. Returns
    the number of FAILING gates."""
    failed = 0
    for tier in tier_names:
        for name, fn in TIERS[tier]:
            if fn is None:
                print(f"[verify {tier}] {name}: DEFERRED (not ported - an "
                      f"honestly absent gate, never a fake green; see "
                      f"docs/giten-old-triage.md)")
                continue
            t0 = time.monotonic()
            try:
                findings = fn()
            # SystemExit is a BaseException: a gate that reports a missing
            # input by raising it (`no report.json - run giten compare`)
            # would otherwise abort the whole tier run, and every gate after
            # it would be silently skipped with no verdict at all.
            except SystemExit as exc:  # noqa: BLE001
                findings = [f"gate could not run: {exc} "
                            f"(re-run `{_rerun_command(name)}`)"]
            except Exception as exc:  # noqa: BLE001 - a broken gate is a failure
                findings = [f"gate crashed: {type(exc).__name__}: {exc} - a "
                            f"broken gate is a FAILURE, never a pass; re-run "
                            f"`{_rerun_command(name)}` for the traceback. A "
                            f"message naming a build/gen artifact means the "
                            f"tree is mid-build or that file is truncated: "
                            f"rebuild with `giten build`."]
            dt = time.monotonic() - t0
            advisory = []
            if isinstance(findings, Verdict):
                findings, advisory = findings
            if findings:
                failed += 1
                print(f"[verify {tier}] {name}: FAIL "
                      f"({len(findings)} finding(s), {dt:.1f}s)")
                for f in findings[:max_findings]:
                    print(f"    {f.splitlines()[0][:200]}")
                if len(findings) > max_findings:
                    print(f"    ... {len(findings) - max_findings} more "
                          f"({_rerun_command(name)})")
            elif advisory:
                print(f"[verify {tier}] {name}: OK, ADVISORY "
                      f"({len(advisory)} finding(s) listed, not failing: "
                      f"data_matching = false; {dt:.1f}s)")
            else:
                print(f"[verify {tier}] {name}: OK ({dt:.1f}s)")
            if advisory:
                if findings:
                    print(f"    advisory ({len(advisory)}, not failing):")
                for f in advisory[:max_findings]:
                    print(f"    {f.splitlines()[0][:200]}")
                if len(advisory) > max_findings:
                    print(f"    ... {len(advisory) - max_findings} more "
                          f"({_rerun_command(name)})")
    return failed
