"""giten branch - generate the `source` and `classic` branches, seed `port`.

    giten branch source  [--verify] [--publish] [--ref REV | --working-tree]
    giten branch classic [--verify] [--publish] [--ref REV | --working-tree]
    giten branch port    [--ref REV]

`source` is the readable source: comments, claims and enum-domain macros go,
the bug-fix conditionals stay. `classic` also removes the bug fixes, keeping
the retail branch of every GITEN_BUGFIX/GITEN_COMPAT conditional. Each is
written to build/branch/<name> and, with --publish, committed as the single
root commit of the local branch <name>, checked out at build/<name>.
`port` is created once, from the published `source`, with every fix enabled;
it is maintained by hand afterwards. Nothing is pushed. See docs/branches.md.
"""

from __future__ import annotations

import argparse
import hashlib
import subprocess
from pathlib import Path


def main(argv: list[str] | None = None) -> int:
    from giten.branch import export
    from giten.core.paths import REPO

    parser = argparse.ArgumentParser(prog="giten branch", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("variant", choices=("source", "classic", "port"))
    parser.add_argument("--ref", default="HEAD", help="the committed revision to export")
    parser.add_argument("--working-tree", action="store_true",
                        help="preview the tracked working files (cannot publish)")
    parser.add_argument("--out", type=Path, help="output directory (build/branch/<variant>)")
    parser.add_argument("--verify", action="store_true",
                        help="build the export and compare its objects with the matching build")
    parser.add_argument("--publish", action="store_true",
                        help="commit the export to the local branch <variant>")
    parser.add_argument("--worktree", type=Path, help="the branch's worktree (build/<variant>)")
    args = parser.parse_args(argv)
    try:
        if args.variant == "port" and (args.working_tree or args.verify or args.publish):
            raise ValueError("port takes only --ref, --out and --worktree")
        if args.publish and args.working_tree:
            raise ValueError("publication needs a committed revision, not --working-tree")
        out = args.out or REPO / "build/branch" / args.variant
        worktree = args.worktree or REPO / "build" / args.variant
        commit, files = export.snapshot(REPO, args.ref, working=args.working_tree)
        generated = export.generate(files, args.variant)
        if args.variant == "port":
            path = export.seed_port(REPO, generated, commit, worktree)
            print(f"port: seeded at {path}")
            return 0
        output = export.write_output(REPO, out, generated, commit, args.variant,
                                     args.working_tree)
        digest = hashlib.sha256(b"".join(name.encode() + b"\0" + data
                                         for name, data in sorted(generated.items())))
        print(f"{args.variant}: {len(generated)} files from {commit[:12]} at {output}; "
              f"SHA-256 {digest.hexdigest()}", flush=True)
        if args.verify:
            from giten.branch.verify import verify
            verify(output, REPO, commit, fixes=args.variant == "source")
        if args.publish:
            path, changed = export.publish(REPO, generated, commit, args.variant, worktree)
            print(f"{args.variant}: {'committed' if changed else 'unchanged'} at {path}")
        return 0
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.error(str(error))
