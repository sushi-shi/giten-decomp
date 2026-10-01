"""giten.verify - the VERIFY/BANK slice: MAX-ledger banking + the regression
gate + the README score block.

    python3 -m giten.verify status        current summary + the regression
                                           report (rva-keyed), bankable
                                           improvements, renames, losses -
                                           exit 0 always (it reports)
    python3 -m giten.verify check         same computation; exit nonzero on a
                                           REAL regression (a fresh below-bank
                                           dip, an unbanked loss, or a hard
                                           report failure) - the MAX gate
    python3 -m giten.verify bank          preconditions (bankable tree), then
                                           update config/match_baseline.tsv
                                           under the src_hash rules and refresh
                                           the README score block. A MANUAL
                                           act: nothing regenerates the
                                           baseline automatically.
    python3 -m giten.verify fingerprints  refresh the per-function source
                                           fingerprint cache (clangd range
                                           hashes; bank runs this itself)
    python3 -m giten.verify <gate>        run one ported gate/audit module
                                           (see the list below); `check
                                           --tier fast|normal|full|link`
                                           runs them in tiers (default
                                           fast,normal - the graph's edge)

Ported doctrine (from the frozen giten-old/match/status.py, never imported):
the retail RVA is the BODY's identity (a vanished row whose rva is still
occupied is a rename/move, not a loss; the high-water travels by rva);
best_pct is gated by src_hash (same hash + a different % banks the high mark -
TU composition moved, not the source; a CHANGED hash resets best to cur - the
old peak belonged to source that no longer exists); hist_pct never resets
except when the rva moves under a name.

Input surface: build/objdiff/compare-new/report.json (falling back to
build/objdiff/report.json), config/match_baseline.tsv, the Model
(giten.model.resolve) for rva/unit joins, config/units.toml for the module
rollup, and git for the bankable-tree precondition only.

Writes: config/match_baseline.tsv from `bank` ONLY, README.md's marked
block from `readme` (a default build edge), `check`, and `bank`, and each gate's committed floor from its own `--update` bless ONLY -
never from a gate run. Gates otherwise write nothing but build/gen/ scratch
(the fingerprint cache, the .LIB symbol cache, the layout and data-access
maps), which is derived and regenerated.
"""

from __future__ import annotations

_SUBS = ("status", "check", "bank", "readme", "fingerprints")

#: the ported gate/audit modules, runnable as `giten verify <name>`. MOST are
#: also a tier member of `check --tier` (giten.verify.tiers); the ones in
#: _QUERY_ONLY below are read-only oracles no tier runs - they answer a
#: question, they do not return findings.
_GATES = {"board": "giten.verify.board", "bans": "giten.verify.bans",
          "casts": "giten.verify.casts",
          "c-casts": "giten.verify.c_casts",
          "compiler-artifacts": "giten.verify.compiler_artifacts",
          "constants": "giten.verify.constants",
          "enum-domains": "giten.verify.enum_domains",
          "enum-reuse": "giten.verify.enum_reuse",
          "label-style": "giten.verify.label_style",
          "source-encoding": "giten.verify.source_encoding",
          "claim-size": "giten.verify.claim_size",
          "include-order": "giten.verify.include_order",
          "unique-names": "giten.verify.unique_names",
          "library-overlap": "giten.verify.library_overlap",
          "tu-order": "giten.verify.tu_order",
          "data-tu-order": "giten.verify.data_tu_order",
          "dead-code": "giten.verify.dead_code",
          "undefined-closure": "giten.verify.undefined_closure",
          "data-identity": "giten.verify.data_identity",
          # the one gate whose module lives OUTSIDE giten.verify: the wall
          # ledger's count certifications are re-measured by the same
          # giten.walls.recheck the campaign runs by hand. Registering the
          # module rather than a forwarding shim keeps ONE implementation, and
          # keeps the tier label typeable (`giten verify review-claims`).
          "review-claims": "giten.walls.recheck",
          "vtables": "giten.verify.vtables",
          "vtable-scan": "giten.verify.vtable_scan",
          "alloc-size": "giten.verify.alloc_size",
          "assert-relocs": "giten.verify.assert_relocs",
          "data-relocs": "giten.verify.data_relocs",
          "caller-callee": "giten.verify.caller_callee",
          "data-access": "giten.verify.data_access",
          "data-coverage": "giten.verify.data_coverage",
          "library-data-refs": "giten.verify.library_data_refs",
          "layout": "giten.verify.layout",
          "link-tier": "giten.verify.link_tier"}

#: runnable as `giten verify <name>` but in NO tier: read-only oracles, not
#: gates. `vtable-scan` enumerates the image's vtables (verify.vtables is the
#: gate over it); `layout` is the field-offset oracle verify.data_access
#: consumes. Neither returns findings, so neither can fail a build.
_QUERY_ONLY = ("layout", "library-data-refs", "vtable-scan")

#: Audits that are deliberately explicit because they parse the whole source
#: tree and are not part of a normal build tier.
_STANDALONE = ("c-casts", "constants", "enum-reuse")

#: tier label -> verb, where the two spellings differ. giten.verify.tiers
#: labels the bans row `vtable-bans`, while the module and verb are `bans`.
#: Without this mapping the label names no runnable command.
_ALIASES = {"vtable-bans": "bans"}


def _usage(stream=None) -> None:
    import sys
    out = stream or sys.stdout
    print(__doc__.strip(), file=out)
    gates = sorted(g for g in _GATES
                   if g not in _QUERY_ONLY and g not in _STANDALONE)
    print("\ngates (each also run by `check --tier`): " + ", ".join(gates),
          file=out)
    print("standalone audits (no tier runs these): "
          + ", ".join(sorted(_STANDALONE)), file=out)
    print("read-only oracles (no tier runs these): "
          + ", ".join(sorted(_QUERY_ONLY)), file=out)


def main(argv=None) -> int:
    import sys
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] in _ALIASES:
        argv[0] = _ALIASES[argv[0]]
    known = _SUBS + tuple(_GATES)
    if not argv:
        _usage(sys.stderr)
        print("\ngiten verify: pick a verb or a gate from the lists above",
              file=sys.stderr)
        return 2
    if argv[0] in ("-h", "--help"):
        _usage()
        return 0
    if argv[0] not in known:
        _usage(sys.stderr)
        print(f"\ngiten verify: unknown verb/gate {argv[0]!r} - pick one of "
              f"the names listed above", file=sys.stderr)
        return 2
    sub, rest = argv[0], argv[1:]
    if sub == "fingerprints":
        from giten.verify.fingerprints import main as fp_main
        return fp_main(rest)
    if sub in _GATES:
        import importlib
        mod = importlib.import_module(_GATES[sub])
        sys.argv = [f"giten verify {sub}", *rest]
        return mod.main(rest)
    from giten.verify import verbs
    sys.argv = [f"giten verify {sub}", *rest]
    return {"status": verbs.cmd_status, "check": verbs.cmd_check,
            "bank": verbs.cmd_bank, "readme": verbs.cmd_readme}[sub](rest)
