#!/usr/bin/env python3
"""Use upstream decomp-permuter AST mutations with Giten's x86 COFF scorer."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
import tomllib
from pathlib import Path


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", required=True, type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--rva", required=True, type=lambda value: int(value, 0))
    parser.add_argument("--function", required=True)
    parser.add_argument("--signature", required=True)
    parser.add_argument("--end-marker", default="\nRVA(")
    parser.add_argument("--preamble", required=True, type=Path)
    parser.add_argument("--parser-replace", action="append", default=[],
                        help="OLD=NEW, only for a proven macro expansion in the parser surrogate")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--trials", type=int, default=64)
    parser.add_argument("--seed", type=int, default=83000)
    return parser.parse_args()


def main() -> int:
    args = arguments()
    if args.trials < 1:
        raise SystemExit("--trials must be positive")
    root = Path.cwd()
    source = root / args.source
    original = source.read_text()
    if original.count(args.signature) != 1:
        raise SystemExit("--signature must occur exactly once")
    start = original.index(args.signature)
    end = len(original) if args.end_marker == "EOF" else original.find(args.end_marker, start)
    if end < 0:
        raise SystemExit("--end-marker was not found after the function")
    body = original[start:end]

    sys.path.insert(0, str(args.upstream.resolve()))
    from src import ast_util
    from src.randomizer import Randomizer
    from giten.compare.normalize import relax_data_relocations
    from giten.core import data_matching
    from giten.permute.tu_state_noise import (
        canonicalize_disposable_object,
        compile_object,
        objdiff_scores,
        resolve_target,
    )
    from giten.walls.diagnose import NORM

    parser_body = body
    for replacement in args.parser_replace:
        if "=" not in replacement:
            raise SystemExit("--parser-replace requires OLD=NEW")
        old, new = replacement.split("=", 1)
        if not old or old not in parser_body:
            raise SystemExit(f"parser token {old!r} does not occur in the function")
        parser_body = parser_body.replace(old, new)
    parser_body = re.sub(r"^[ \t]*//[^\n]*$", "", parser_body, flags=re.MULTILINE)
    parsed = ast_util.parse_c(args.preamble.read_text() + "\n" + parser_body)
    function, _ = ast_util.extract_fn(parsed, args.function)
    ast_util.normalize_ast(function, parsed)
    weights = tomllib.loads((args.upstream / "default_weights.toml").read_text())["base"]
    # Restrict to C source-shape transforms; generated candidates are still
    # disposable and must be screened for semantic preservation.
    allowed = {
        "perm_temp_for_expr", "perm_expand_expr", "perm_reorder_stmts",
        "perm_reorder_decls", "perm_cast_simple", "perm_split_assignment",
        "perm_ins_block", "perm_struct_ref", "perm_condition",
        "perm_commutative", "perm_add_sub", "perm_inequalities",
        "perm_compound_assignment", "perm_duplicate_assignment",
        "perm_chain_assignment", "perm_inline", "perm_remove_var",
    }
    weights = {name: (weight if name in allowed else 0) for name, weight in weights.items()}

    target, flags = resolve_target(root, args.source, args.rva)
    if target.symbol not in ("_" + args.function, args.function):
        raise SystemExit(f"target symbol {target.symbol} does not match {args.function}")
    target_obj = NORM / "target" / f"{target.unit}.c.obj"
    baseline_obj = NORM / "base" / f"{target.unit}.obj"
    if not target_obj.is_file() or not baseline_obj.is_file():
        raise SystemExit("run giten build before the campaign")
    expected = objdiff_scores(target_obj, baseline_obj, target.symbol)[0].get(target.symbol)
    if expected is None:
        raise SystemExit("authoritative baseline did not score")

    out = args.output
    out.mkdir(parents=True, exist_ok=True)
    seen: set[str] = set()
    rows: list[dict] = []
    best = 0.0
    for trial in range(args.trials + 1):
        if trial == 0:
            candidate = body
        else:
            island = (trial - 1) // 4
            depth = (trial - 1) % 4 + 1
            tree = copy.deepcopy(parsed)
            randomizer = Randomizer(weights, args.seed + island)
            for _ in range(depth):
                randomizer.randomize(tree, args.function)
            candidate = ast_util.to_c(ast_util.extract_fn(tree, args.function)[0])
        digest = hashlib.sha256(candidate.encode()).hexdigest()
        if digest in seen:
            continue
        seen.add(digest)
        tag = f"{trial:04d}"
        trial_source = out / f"{tag}{source.suffix}"
        trial_obj = out / f"{tag}.obj"
        trial_source.write_text(original[:start] + candidate + original[end:])
        (out / f"{tag}.function.c").write_text(candidate)
        compiled, log, timed_out = compile_object(root, trial_source, trial_obj, flags, 60)
        (out / f"{tag}.compile.log").write_text(log)
        if not compiled:
            rows.append({"trial": trial, "failed": True, "timeout": timed_out})
            continue
        normalized = canonicalize_disposable_object(trial_obj, out / f"{tag}.normalized.obj")
        if not data_matching.enabled():
            relaxed, _ = relax_data_relocations(normalized.read_bytes())
            normalized = out / f"{tag}.relaxed.obj"
            normalized.write_bytes(relaxed)
        scores, sizes, _, _ = objdiff_scores(target_obj, normalized, target.symbol)
        score = scores.get(target.symbol)
        if trial == 0 and (score is None or abs(score - expected) > 0.0001):
            raise SystemExit(f"disposable baseline {score} differs from authoritative {expected}")
        row = {"trial": trial, "score": score, "size": sizes.get(target.symbol)}
        rows.append(row)
        (out / "summary.json").write_text(json.dumps(rows, indent=2) + "\n")
        if score is not None and score > best:
            best = score
            print(f"trial {trial}: {score:.6f}% (best)", flush=True)
    if source.read_text() != original:
        raise SystemExit("authored source changed during campaign")
    print(f"{len(rows)} distinct sources; baseline {expected:.6f}%; best {best:.6f}%")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
