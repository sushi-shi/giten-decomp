"""Run upstream decomp-permuter's weighted C mutations through the MSVC auditor.

Only the target and generated inline helpers are emitted back into disposable
copies of the real TU. Preprocessed declarations give upstream its type context;
they are never compiled in place of the project's headers. Retained candidates
are diagnostic input and require source/semantic review before adoption.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import importlib
import importlib.util
import json
import math
import os
from pathlib import Path
import random
import subprocess
import sys
import time
import tomllib

from giten.permute import batch_source_variants as batch
from giten.permute.generate_ast_variants import (
    ci, clang_args, configure_libclang, target_function,
)
from giten.permute.tu_state_noise import project_root

# These mutate declarations outside the emitted region or intentionally remove
# control-flow guards. ABI/global recovery belongs to the structural matcher.
DISABLED = {
    "perm_randomize_external_type", "perm_randomize_function_type", "perm_remove_ast",
}


def load_upstream(path: Path):
    if not (path / "src/randomizer.py").is_file():
        raise ValueError("upstream checkout missing; use --upstream or GITEN_DECOMP_PERMUTER")
    name = "_giten_decomp_permuter"
    if name in sys.modules:
        loaded = Path(sys.modules[name].__file__).resolve().parent.parent
        if loaded != path.resolve():
            raise ValueError("cannot load two upstream revisions in one process")
    else:
        # Upstream's parser is bundled alongside its package, not installed.
        sys.path.insert(0, str(path.resolve()))
        spec = importlib.util.spec_from_file_location(
            name, path / "src/__init__.py", submodule_search_locations=[str(path / "src")],
        )
        module = importlib.util.module_from_spec(spec)
        sys.modules[name] = module
        spec.loader.exec_module(module)
    ast_util = importlib.import_module(name + ".ast_util")
    randomizer = importlib.import_module(name + ".randomizer")
    weights = tomllib.loads((path / "default_weights.toml").read_text())["base"]
    weights = {key: float(value) if key not in DISABLED else 0.0
               for key, value in weights.items()}
    return ast_util, randomizer.Randomizer, weights


def source_context(root: Path, source: Path, rva: int, ast_util):
    if source.suffix.lower() != ".c":
        raise ValueError("upstream's parser accepts C; use variants for C++ translation units")
    blob = source.read_bytes()
    configure_libclang()
    args = clang_args(root, source)
    tu = ci.Index.create().parse(str(source), args=args)
    fn = target_function(tu, source, blob, rva)
    errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
    if errors:
        raise ValueError("cannot derive type context:\n" + "\n".join(errors[:10]))
    start, end = fn.extent.start.offset, fn.extent.end.offset
    original = blob[start:end].decode()
    parser_body = bytearray(blob[start:end])
    for token in fn.get_tokens():
        if token.kind == ci.TokenKind.COMMENT:
            left, right = token.extent.start.offset - start, token.extent.end.offset - start
            if 0 <= left <= right <= len(parser_body):
                parser_body[left:right] = bytes(10 if c == 10 else 32
                                               for c in parser_body[left:right])
    # Parser-only portability spelling. The MSVC compile sees original headers.
    parse_args = [a for a in args if not a.startswith("-DGITEN_EMIT_META")]
    parse_args += ["-D__int64=long long", "-D__cdecl=", "-D__stdcall=",
                   "-D__fastcall=", "-D_cdecl=", "-D_stdcall=", "-D_fastcall=",
                   "-D__declspec(x)=", "-D__inline=inline", "-D_inline=inline"]
    preprocessed = subprocess.run(
        [os.environ.get("GITEN_CLANG", "clang"), *parse_args, "-E", "-P", str(source)],
        check=True, capture_output=True, text=True, timeout=60,
    ).stdout
    # Only declarations are needed for upstream's type map. Discard function
    # bodies using clang ranges so SDK inline assembly never reaches pycparser.
    context_path = source.with_name(".permuter-context.c")
    context_tu = ci.Index.create().parse(
        str(context_path), args=parse_args,
        unsaved_files=[(str(context_path), preprocessed)],
    )
    context_errors = [str(d) for d in context_tu.diagnostics
                      if d.severity >= ci.Diagnostic.Error]
    if context_errors:
        raise ValueError("cannot parse declaration context:\n" + "\n".join(context_errors[:10]))
    bodies = []
    for cursor in context_tu.cursor.walk_preorder():
        if cursor.kind == ci.CursorKind.FUNCTION_DECL and cursor.is_definition():
            for child in cursor.get_children():
                if child.kind == ci.CursorKind.COMPOUND_STMT:
                    bodies.append((child.extent.start.offset, child.extent.end.offset))
    declarations = preprocessed.encode()
    for left, right in sorted(bodies, reverse=True):
        declarations = declarations[:left] + b";" + declarations[right:]
    ast = ast_util.parse_c(declarations.decode())
    # Keep the authored body, including its macro/accessor calls. Expanding it
    # would erase the very inline boundaries the campaign is trying to recover.
    prelude = ast_util.to_c_raw(ast)
    authored_ast = ast_util.parse_c(prelude + "\n" + parser_body.decode())
    authored_fn, _ = ast_util.extract_fn(authored_ast, fn.spelling)
    signature = ast_util.to_c_raw(authored_fn.decl)
    return blob, start, end, fn.spelling, original, prelude, signature, parser_body.decode()


def generate_options(ast_util, randomizer, weights, prelude, name, signature,
                     parents, count, seed, chain_depth, seen=None):
    chooser = random.Random(seed)
    options = [{"name": "baseline"}]
    seen = set() if seen is None else seen
    seen.update(parents)
    for index, parent in enumerate(parents[1:], 1):
        options.append({"name": f"retained-parent-{index}", "replace": parent})
    target_count = len(options) + count
    failures = []
    attempts = 0
    ast = None
    prelude_count = len(ast_util.parse_c(prelude).ext)
    parent_asts = [ast_util.parse_c(prelude + "\n" + parent) for parent in parents]
    while len(options) < target_count and attempts < count * 8:
        attempts += 1
        if ast is None or depth >= chain_depth:
            parent = chooser.randrange(len(parents))
            ast = copy.copy(parent_asts[parent])
            ast.ext = (list(parent_asts[parent].ext[:prelude_count])
                       + copy.deepcopy(parent_asts[parent].ext[prelude_count:]))
            for node in ast.ext[prelude_count:]:
                if hasattr(node, "decl"):
                    node.decl.funcspec = ["inline" if s == "__inline" else s
                                          for s in node.decl.funcspec]
            known = {id(node) for node in ast.ext[:prelude_count]}
            target, _ = ast_util.extract_fn(ast, name)
            ast_util.normalize_ast(target, ast)
            chain_seed = chooser.getrandbits(64)
            mutator = randomizer(weights, chain_seed)
            depth = 0
        # Upstream mutation passes may fail on an unsupported type. Restart
        # that chain; never keep a partially edited AST after an exception.
        try:
            mutator.randomize(ast, name)
            depth += 1
            target, _ = ast_util.extract_fn(ast, name)
            if ast_util.to_c_raw(target.decl) != signature:
                raise ValueError("mutation changed the target signature")
            emitted = []
            for node in ast.ext:
                if id(node) in known:
                    continue
                if node is not target:
                    # Upstream's inline pass emits C99 inline; VC5 uses __inline.
                    # Keep upstream's AST spelling: extract_fn recognizes inline
                    # helpers by that spelling and otherwise drops their bodies.
                    node = copy.deepcopy(node)
                    node.decl.funcspec = ["__inline" if s == "inline" else s
                                          for s in node.decl.funcspec]
                    if "static" not in node.decl.storage:
                        node.decl.storage.append("static")
                emitted.append(ast_util.to_c(node).strip())
            candidate = "\n\n".join(emitted)
        except Exception as exc:
            failures.append({"seed": chain_seed, "depth": depth,
                             "error": type(exc).__name__ + ": " + str(exc)})
            ast = None
            continue
        if candidate in seen:
            continue
        seen.add(candidate)
        options.append({"name": f"parent-{parent}-seed-{chain_seed:x}-depth-{depth}",
                        "replace": candidate})
    return options, attempts, failures


def frontier_parents(directory: Path, original: bytes, start: int, end: int):
    prefix, suffix = original[:start], original[end:]
    parents = [original[start:end].decode()]
    for row in json.loads((directory / "frontier/frontier.json").read_text()):
        candidate = (directory / "frontier" / row["source"]).read_bytes()
        if not candidate.startswith(prefix) or not candidate.endswith(suffix):
            raise ValueError("frontier candidate changed source outside target region")
        finish = len(candidate) - len(suffix) if suffix else len(candidate)
        body = candidate[start:finish].decode()
        if body not in parents:
            parents.append(body)
    return parents


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("rva", type=lambda value: int(value, 0))
    parser.add_argument("--upstream", type=Path, default=os.environ.get("GITEN_DECOMP_PERMUTER"))
    parser.add_argument("--rounds", type=int, default=8)
    parser.add_argument("--trials", type=int, default=256, help="unique mutations per round")
    parser.add_argument("--scored", type=int,
                        help="stop after this many unique scored mutations; rounds remains a ceiling")
    parser.add_argument("--chain-depth", type=int, default=8)
    parser.add_argument("--jobs", type=int, default=8)
    parser.add_argument("--frontier", type=int, default=4)
    parser.add_argument("--seed", type=lambda value: int(value, 0), default=0x475254)
    parser.add_argument("--weights", type=Path, help="TOML [weights] overrides for upstream passes")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)
    if not args.upstream:
        parser.error("--upstream or GITEN_DECOMP_PERMUTER is required (provided by nix develop)")
    if min(args.rounds, args.trials, args.chain_depth, args.jobs, args.frontier) < 1:
        parser.error("rounds, trials, chain depth, jobs and frontier must be positive")
    if args.scored is not None and args.scored < 1:
        parser.error("--scored must be positive")
    root = project_root()
    source = (root / args.source).resolve()
    try:
        source.relative_to(root)
        upstream = args.upstream.resolve()
        ast_util, randomizer, weights = load_upstream(upstream)
        if args.weights:
            overrides = tomllib.loads(args.weights.read_text())["weights"]
            if any(k not in weights or k in DISABLED or not isinstance(v, (int, float))
                   or not math.isfinite(v) or v < 0 for k, v in overrides.items()):
                raise ValueError("weights must name enabled upstream passes with nonnegative values")
            weights.update(overrides)
        if not any(weights.values()):
            raise ValueError("at least one mutation weight must be positive")
        context = source_context(root, source, args.rva, ast_util)
        blob, start, end, name, original, prelude, signature, parser_body = context
        output = (root / args.output).resolve()
        output.mkdir(parents=True, exist_ok=False)
        revision = subprocess.run(["git", "-C", str(upstream), "rev-parse", "HEAD"],
                                  capture_output=True, text=True).stdout.strip()
        document = {"source": str(args.source), "rva": hex(args.rva), "seed": args.seed,
                    "upstream": str(upstream), "revision": revision,
                    "source_sha256": hashlib.sha256(blob).hexdigest(),
                    "weights": weights, "scored_target": args.scored, "rounds": []}
        parents = [parser_body]
        seen = set()
        scored_sources = set()
        target_states = set()
        campaign_started = time.perf_counter()
        for number in range(args.rounds):
            if source.read_bytes() != blob:
                raise ValueError("authored source changed during campaign")
            print(f"[random] round {number + 1}/{args.rounds}: "
                  f"{len(parents)} parents, {args.trials} mutations, {args.jobs} compilers",
                  flush=True)
            generation_started = time.perf_counter()
            options, attempts, failures = generate_options(
                ast_util, randomizer, weights, prelude, name, signature, parents,
                args.trials, args.seed + number, args.chain_depth, seen,
            )
            generation_seconds = time.perf_counter() - generation_started
            manifest = output / f"round-{number + 1:03d}.json"
            manifest.write_text(json.dumps({
                "schema": 1, "source": str(source.relative_to(root)), "rva": hex(args.rva),
                "axes": [{"name": "upstream", "find": original, "options": options}],
                "generation_attempts": attempts, "generation_failures": failures,
            }, indent=2) + "\n")
            results = output / f"round-{number + 1:03d}.results"
            code = batch.main([str(manifest), "--jobs", str(args.jobs),
                               "--limit", str(len(options)), "--frontier", str(args.frontier),
                               "--continue-after-exact", "--top", "4", "--output", str(results)])
            summary = json.loads((results / "results.json").read_text())
            document["rounds"].append({
                "round": number + 1, "generation_attempts": attempts,
                "generation_failures": len(failures), "generation_seconds": generation_seconds,
                "results": str(results),
                **{key: summary.get(key) for key in (
                    "attempted_variant_count", "executed_variant_count", "compile_failed_count",
                    "state_count", "elapsed_seconds", "best", "source_restored",
                )},
            })
            for row in summary["results"]:
                if row.get("score") is not None:
                    scored_sources.add(row["source_sha256"])
                    target_states.add(row["state_id"])
            document["totals"] = {
                "attempted": sum(r["attempted_variant_count"] for r in document["rounds"]),
                "scored": sum(r["executed_variant_count"] for r in document["rounds"]),
                "unique_scored_sources": len(scored_sources),
                "unique_scored_mutations": len(scored_sources - {document["source_sha256"]}),
                "distinct_target_states": len(target_states),
                "elapsed_seconds": time.perf_counter() - campaign_started,
            }
            (output / "campaign.json").write_text(json.dumps(document, indent=2) + "\n")
            if code:
                return code
            if (args.scored is not None
                    and document["totals"]["unique_scored_mutations"] >= args.scored):
                return 0
            parents = frontier_parents(results, blob, start, end)
            parents[0] = parser_body
        if args.scored is not None:
            print(f"[random] round ceiling reached before {args.scored} unique scored mutations",
                  file=sys.stderr)
            return 1
        return 0
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    raise SystemExit(main())
