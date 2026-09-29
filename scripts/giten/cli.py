"""giten - the umbrella CLI.

    giten tool <name> [args...]     drive one external tool (giten/tool/)
    giten labels [--all|--unit U]   source labels -> claim fragments (+ the
                                     tree-wide completeness sweep)
    giten model                     resolve claims x censuses -> bindings
    giten delink                    model -> synth pdb -> retail target objs
    giten compare                   base vs target -> objdiff report + summary
    giten build                     configure-if-needed + ninja (the loop:
                                     cl -> labels -> model -> delink -> compare)
    giten link                      the opt-in candidate link (EXE + .map)
    giten match                     build, then the compare summary for the
                                     units whose objs changed
    giten configure                 re-emit build/build.ninja
    giten sema <sub>                read-only investigation views (disasm,
                                     xref, rva, vtable, classof, strings, ...)
    giten walls <sub>               the wall campaign: inventory, diagnose,
                                     inline-model
    giten permute <verb>            classified state/variant search or island campaign
    giten ghidra <sub>              one-way viewer export: the retail image
                                     as a labelled Ghidra project (build,
                                     update, verify, status, export)
    giten verify <sub>              status / check (the MAX gate) / bank
                                     (baseline + README, manual) /
                                     fingerprints
    giten lsp <verb>                clangd-backed refs / hover / rename (the
                                     type-aware bulk member renamer)
    giten codecs --disc <DDSWIN.BIN> retail/candidate/Rust resource execution
    giten play [--disc DDSWIN.BIN]   build the bug-fixed image (GITEN_BUGFIX)
                                     and start the game under Wine/gamescope
    giten branch <source|classic|port>  generate the readable-source branches
                                     (docs/branches.md)
    giten init                      local setup (the build wine prefix; the
                                     dev-shell hook runs this at entry)

Subcommands grow with the rebuild; `tool` forwards to the named module's own
main(), so `giten tool cl ...` and `python3 -m giten.tool.cl ...` (the form
ninja rule lines use) are the same entry.
"""

from __future__ import annotations

import sys

TOOLS = ("wine", "cl", "link", "rc", "delinker", "pdbutil", "objdiff",
         "objdump", "ghidra", "clangd")


#: Query families where rc=1 answers "different", not "failed".
QUERY_COMMANDS = {"sema", "walls"}


def main(argv: list[str] | None = None) -> int:
    """Run one command and record it in build/giten_usage.{log,jsonl}."""
    from giten.core.paths import BUILD
    from giten.core.usage import run_logged
    argv = list(sys.argv[1:] if argv is None else argv)
    failure_rc = 2 if argv and argv[0] in QUERY_COMMANDS else 1
    return run_logged(_dispatch, argv, BUILD / "giten_usage.log",
                      failure_rc=failure_rc)


def _dispatch(argv: list[str]) -> int:
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        print(f"\ntools: {', '.join(TOOLS)}")
        return 0 if argv else 2
    cmd, rest = argv[0], argv[1:]
    if cmd in ("labels", "model", "delink", "compare"):
        import importlib
        mod = importlib.import_module(
            {"labels": "giten.retail_labels.source", "model": "giten.model",
             "delink": "giten.delink.run", "compare": "giten.compare.run"}[cmd])
        sys.argv = [f"giten {cmd}", *rest]
        return mod.main()
    if cmd in ("sema", "walls", "ghidra", "verify", "lsp", "branch"):
        import importlib
        return importlib.import_module(f"giten.{cmd}").main(rest)
    if cmd == "permute":
        if not rest or rest[0] in ("-h", "--help"):
            print("giten permute candidates [options]\n"
                  "giten permute campaign [--rva <rva>] [options]\n"
                  "giten permute state --source <tu.cpp> --rva <rva> [options]\n"
                  "giten permute variants <tu.cpp> <rva> [options]\n"
                  "giten permute random <tu.c> <rva> --output <dir> [options]\n"
                  "giten permute mine --output <build/dir> [options]\n"
                  "  candidates: classify every live source-owned residual\n"
                  "  campaign: run N islands and retain M distinct best solutions\n"
                  "  state: classified, disposable compiler-state search\n"
                  "  variants: reviewed exact axes x AST shapes x TU state\n"
                  "  random: upstream weighted mutations with frontier feedback")
            return 0 if rest else 2
        if rest[0] in ("candidates", "campaign"):
            from giten.permute.campaign import main as campaign_main
            return campaign_main(rest)
        if rest[0] == "mine":
            from giten.permute.mine import main as mine_main
            return mine_main(rest[1:])
        if rest[0] not in ("state", "variants", "random"):
            print("giten permute: unknown verb " + repr(rest[0])
                  + " (have: candidates, campaign, state, variants, random)", file=sys.stderr)
            return 2
        verb, permute_args = rest[0], rest[1:]
        if any(value in ("-h", "--help") for value in permute_args):
            if verb == "state":
                from giten.permute.tu_state_noise import main as permute_main
            elif verb == "random":
                from giten.permute.upstream import main as permute_main
            else:
                from giten.permute.match_variants import main as permute_main
            return permute_main(permute_args)
        rva_arg = (
            next((
                permute_args[index + 1]
                for index, value in enumerate(permute_args[:-1])
                if value == "--rva"
            ), None)
            if verb == "state"
            else (permute_args[1] if len(permute_args) >= 2 else None)
        )
        if rva_arg is None:
            print(f"giten permute {verb}: an RVA is required", file=sys.stderr)
            return 2
        from contextlib import redirect_stdout
        from io import StringIO
        from giten.walls.diagnose import diagnose
        diagnosis = StringIO()
        with redirect_stdout(diagnosis):
            result = diagnose(rva_arg)
        report = diagnosis.getvalue()
        print(report, end="")
        if result or "class: REGALLOC/SCHEDULING" not in report:
            print(f"giten permute {verb}: refused - permutation requires a "
                  "REGALLOC/SCHEDULING diagnosis", file=sys.stderr)
            return 2
        from giten.model import resolve
        from giten.verify.baseline import load as load_baseline
        rva = int(rva_arg, 0)
        if rva >= 0x400000:
            rva -= 0x400000
        binding = next((row for row in resolve().functions if row.rva == rva), None)
        bank = load_baseline().get((binding.unit, binding.name)) if binding else None
        if bank and bank["hist"] >= 100.0:
            print(f"giten permute {verb}: refused - historical MAX is already "
                  "100%", file=sys.stderr)
            return 2
        if verb == "state":
            from giten.permute.tu_state_noise import main as permute_main
        elif verb == "random":
            from giten.permute.upstream import main as permute_main
        else:
            from giten.permute.match_variants import main as permute_main
        return permute_main(permute_args)
    if cmd in ("build", "link", "match"):
        from giten.graph.verbs import VERBS
        return VERBS[cmd](rest)
    if cmd == "configure":
        from giten.graph.emit import main as configure
        sys.argv = ["giten configure", *rest]
        return configure()
    if cmd == "play":
        from giten.play.run import main as play_main
        return play_main(rest)
    if cmd == "codecs":
        from giten.codecs.run import main as codecs_main
        return codecs_main(rest)
    if cmd == "init":
        from giten.tool import ToolError
        from giten.tool.wine import init_prefix, verify_prefix
        try:
            init_prefix()
            verify_prefix()
        except ToolError as e:
            print(f"[init] {e}", file=sys.stderr)
            return 1
        print("[init] build wine prefix OK (the graph/init steps grow with "
              "the rebuild)")
        return 0
    if cmd == "tool":
        if not rest or rest[0] not in TOOLS:
            print(f"giten tool: pick one of {', '.join(TOOLS)}", file=sys.stderr)
            return 2
        import importlib
        mod = importlib.import_module(f"giten.tool.{rest[0]}")
        sys.argv = [f"giten tool {rest[0]}", *rest[1:]]
        return mod.main()
    print(f"giten: unknown command {cmd!r} (the rebuild grows these "
          "step by step; see scripts/giten/__init__.py)", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
