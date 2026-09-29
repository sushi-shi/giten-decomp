"""Generate the `source` and `classic` exports and seed `port`.

The generator reads one committed revision (`git archive`), never the build
tree. It keeps the unit sources, the project headers, a standalone build and
the licence, and removes the matching scaffolding: comments, the include/rva.h
claims, and the enum-domain macros, which it expands to their MSVC 5.0
spelling. The matching-only headers stay as stand-ins, so every unit opens
the same files as in the matching build. `classic` also decides GITEN_BUGFIX and GITEN_COMPAT as undefined;
`port` is seeded with both defined.
"""

from __future__ import annotations

import io
import json
import re
import shutil
import subprocess
import tarfile
import tempfile
import tomllib
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path, PurePosixPath

from giten.branch.lexer import REMOVED, finish, resolve_conditionals, rewrite_macros, \
    strip_comments, words

MARKER = ".giten-branch-generated"
PROVENANCE = "Generated-By: giten branch"
TEMPLATE = "scripts/giten/branch/project/"

#: The play-build flags (docs/play.md) and each variant's decision about them:
#: absent means the conditionals stay in the export.
PLAY_FLAGS = ("GITEN_BUGFIX", "GITEN_COMPAT")
RESOLVED = {
    "source": {},
    "classic": {flag: False for flag in PLAY_FLAGS},
    "port": {flag: True for flag in PLAY_FLAGS},
}

#: Matching-only headers, exported as stand-ins that keep only their guard
#: and includes. Every unit still opens them: MSVC 5.0's register allocation
#: and temporary numbering follow the files a unit opens, so dropping an
#: #include changes objects whose tokens are unchanged.
_STAND_IN = """\
#ifndef {guard}
#define {guard}

// Kept so that the headers each unit opens match the original build: MSVC 5.0
// allocates registers differently when a unit opens another set of files.
{includes}
#endif
"""
SCAFFOLD_HEADERS = {
    "include/rva.h": _STAND_IN.format(guard="GITEN_RVA_H", includes="#include <Ints.h>\n"),
    "include/Enums.h": _STAND_IN.format(guard="GITEN_ENUMS_H", includes="#include <Ints.h>\n"),
    "include/EnumDomain.h": _STAND_IN.format(guard="GITEN_ENUMDOMAIN_H", includes=""),
}

#: include/rva.h claims, removed; DATA_COMPGEN keeps its value, as it expands.
CLAIMS = {
    "RVA": (2, lambda a: REMOVED),
    "RVA_DECL": (1, lambda a: REMOVED),
    "DATA": (1, lambda a: REMOVED),
    "DATA_MESSAGE_MAP": (2, lambda a: REMOVED),
    "RVA_COMPGEN": (3, lambda a: REMOVED),
    "RVA_DYNINIT": (3, lambda a: REMOVED),
    "DATA_COMPGEN": (2, lambda a: a[1]),
    # include/Enums.h and include/EnumDomain.h, as MSVC 5.0 expands them.
    "GZ_ENUM_BEGIN": (1, lambda a: f"typedef enum {a[0]} {{"),
    "GZ_ENUM_END": (1, lambda a: f"}} {a[0]};"),
    "GZ_ENUM_BEGIN_SPLIT": (2, lambda a: f"typedef enum {a[0]} {{"),
    "GZ_ENUM_END_SPLIT": (1, lambda a: f"}} {a[0]};"),
    "GZ_ENUM_STORAGE": (2, lambda a: a[1]),
}
OBJECT_MACROS = {"OVERRIDE": ""}
SCAFFOLD_WORDS = set(CLAIMS) | set(OBJECT_MACROS) | {"GITEN_EMIT_META", "GZ_STRICT_ENUMS"}

#: link.exe settings of the matching tree's candidate and play links
#: (giten.graph.link), which build.py repeats.
LINK_FLAGS = ["/NOLOGO", "/SUBSYSTEM:WINDOWS", "/BASE:0x400000", "/INCREMENTAL:NO",
              "/ENTRY:WinMainCRTStartup", "/FIXED", "/OPT:NOREF", "/OPT:NOICF"]

#: Nothing that could carry game content may enter a generated branch.
PAYLOAD_SUFFIXES = {".exe", ".dll", ".res", ".rsrc", ".bin", ".cue", ".iso", ".img",
                    ".bmp", ".dib", ".ico", ".cur", ".ani", ".png", ".jpg", ".gif",
                    ".wav", ".mid", ".midi", ".avi", ".dat", ".obj", ".lib", ".pdb"}


def git(repo: Path, *arguments: str) -> str:
    return subprocess.check_output(["git", "-C", str(repo), *arguments], text=True).strip()


def snapshot(repo: Path, revision: str, *, working: bool = False) -> tuple[str, dict[str, bytes]]:
    """The tracked files of `revision`, or with `working` of the work tree."""
    commit = git(repo, "rev-parse", "--verify", f"{revision}^{{commit}}")
    files = {}
    if working:
        for name in git(repo, "ls-files", "-z", "--cached").split("\0"):
            path = repo / name
            if name and path.is_file() and not path.is_symlink():
                files[name] = path.read_bytes()
        return commit, files
    archive = subprocess.check_output(["git", "-C", str(repo), "archive", commit])
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            if member.isfile():
                files[member.name] = stream.extractfile(member).read()
    return commit, files


def clean_source(text: str, resolved: dict[str, bool]) -> str:
    """One C/C++ file without comments, claims or enum-domain macros."""
    text = strip_comments(text)
    if resolved:
        text = resolve_conditionals(text, resolved)
    text = rewrite_macros(text, CLAIMS, OBJECT_MACROS)
    residue = words(text) & (SCAFFOLD_WORDS | (set(resolved) if resolved else set()))
    if residue:
        raise ValueError(f"unremoved scaffolding: {sorted(residue)}")
    return finish(text)


def clang_format_style(text: str) -> str:
    """The tree's style without comments or its rva.h macro lists."""
    text = text.split("# --- project macros", 1)[0]
    return "".join(line for line in text.splitlines(keepends=True)
                   if not line.lstrip().startswith("#")).strip() + "\n"


def clang_format(files: dict[str, bytes], style: str) -> dict[str, bytes]:
    formatter = shutil.which("clang-format")
    if formatter is None:
        raise ValueError("clang-format not found on PATH; run inside `nix develop`")
    with tempfile.TemporaryDirectory(prefix="giten-branch-format-") as directory:
        config = Path(directory) / ".clang-format"
        config.write_text(style)

        def run(name: str) -> tuple[str, bytes]:
            result = subprocess.run(
                [formatter, f"--style=file:{config}", f"--assume-filename={name}"],
                input=files[name], capture_output=True, check=True)
            return name, result.stdout

        with ThreadPoolExecutor() as pool:
            return dict(pool.map(run, sorted(files)))


def build_manifest(manifest: dict, fixes: bool) -> bytes:
    units = []
    for unit in sorted(manifest["unit"], key=lambda u: u["unit"]):
        flags = manifest["flags"][unit["flags"]]
        if any(re.match(r"/[DUIF]", flag) for flag in flags):
            raise ValueError(f"{unit['unit']}: unsupported flag in profile {unit['flags']}")
        units.append({"name": unit["unit"], "source": unit["source"], "flags": flags})
    from giten.graph import PLAY_DEFINES
    from giten.graph.link import LINK_LIBS
    data = {"include": ["include"], "units": units,
            "link": {"flags": LINK_FLAGS, "libraries": LINK_LIBS}}
    if fixes:
        data["fixes"] = PLAY_DEFINES
    return (json.dumps(data, indent=2) + "\n").encode()


def generate(files: dict[str, bytes], variant: str) -> dict[str, bytes]:
    """The generated tree for `variant` from a snapshot of the matching tree."""
    resolved = RESOLVED[variant]
    manifest = tomllib.loads(files["config/units.toml"].decode())
    sources = {unit["source"] for unit in manifest["unit"]}
    headers = {name for name in files if name.startswith("include/") and name.endswith(".h")
               and name not in SCAFFOLD_HEADERS}
    if any(name.startswith("vendor/") and PurePosixPath(name).name != ".clang-format"
           for name in files):
        raise ValueError("vendor/ has content; the export does not handle vendored code")
    unsupported = [name for name in files if name.startswith(("src/", "include/"))
                   and name not in sources and name not in headers
                   and name not in SCAFFOLD_HEADERS]
    if unsupported:
        raise ValueError(f"source files the export does not handle: {sorted(unsupported)}")
    cleaned = {}
    for name in sorted(sources | headers):
        try:
            cleaned[name] = clean_source(files[name].decode("utf-8"), resolved).encode()
        except (ValueError, KeyError, UnicodeDecodeError) as error:
            raise ValueError(f"{name}: {error}") from error
    style = clang_format_style(files[".clang-format"].decode())
    for name, text in SCAFFOLD_HEADERS.items():
        if name not in files:
            raise ValueError(f"{name}: missing from the matching tree")
        cleaned[name] = text.encode()
    output = clang_format(cleaned, style)
    output[".clang-format"] = style.encode()
    for name, data in files.items():
        if name.startswith(TEMPLATE):
            relative = name.removeprefix(TEMPLATE)
            if relative.startswith("README."):
                if relative == f"README.{variant}.md":
                    output["README.md"] = data
            elif relative == "gitignore":
                output[".gitignore"] = data
            elif relative == "flake.nix.in":
                pin = re.search(r'nixpkgs\.url\s*=\s*"([^"]+)"', files["flake.nix"].decode())
                output["flake.nix"] = data.replace(b"@nixpkgs@", pin.group(1).encode())
            else:
                output[relative] = data
    output["LICENSE"] = files["LICENSE"]
    output["build.json"] = build_manifest(manifest, fixes=variant == "source")
    lock = json.loads(files["flake.lock"])
    nixpkgs = lock["nodes"][lock["nodes"]["root"]["inputs"]["nixpkgs"]]
    output["flake.lock"] = (json.dumps({
        "nodes": {"nixpkgs": nixpkgs, "root": {"inputs": {"nixpkgs": "nixpkgs"}}},
        "root": "root", "version": lock["version"]}, indent=2) + "\n").encode()
    check(output, variant)
    return output


def check(output: dict[str, bytes], variant: str) -> None:
    """Refuse anything that is not text source or build support."""
    for name, data in output.items():
        path = PurePosixPath(name)
        if any(part in ("tests", "__pycache__", ".git", "build", "config", "docs")
               for part in path.parts[:-1]):
            raise ValueError(f"forbidden generated path: {name}")
        if path.suffix.lower() in PAYLOAD_SUFFIXES or name.startswith("scripts/"):
            raise ValueError(f"forbidden generated file: {name}")
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError as error:
            raise ValueError(f"{name}: not UTF-8 text") from error
        if "\0" in text or REMOVED in text:
            raise ValueError(f"{name}: binary content")
        if variant == "classic" and name.startswith(("src/", "include/")) \
                and re.search(r"\bGITEN_(?:BUGFIX|COMPAT)\b", text):
            raise ValueError(f"{name}: a fix flag survives in classic")
    if "README.md" not in output:
        raise ValueError(f"no README template for {variant}")


def validate_output(repo: Path, requested: Path) -> Path:
    """An output below build/, empty or holding a previous export."""
    path = requested.absolute()
    if path.is_symlink() or any(parent.is_symlink() for parent in path.parents):
        raise ValueError("output must not traverse symlinks")
    path, repo = path.resolve(), repo.resolve()
    if not path.is_relative_to(repo / "build") or path == repo / "build":
        raise ValueError("output must be a directory below build/")
    if path.exists() and (not (path / MARKER).is_file() or (path / ".git").exists()):
        raise ValueError(f"{path}: not a generated output directory")
    return path


def write_output(repo: Path, requested: Path, files: dict[str, bytes], commit: str,
                 variant: str, working: bool) -> Path:
    output = validate_output(repo, requested)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".giten-branch-", dir=output.parent) as directory:
        staging = Path(directory) / "project"
        for name, data in sorted(files.items()):
            path = staging / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        (staging / MARKER).write_text(json.dumps(
            {"variant": variant, "commit": commit, "working": working}) + "\n")
        if output.exists():
            shutil.rmtree(output)
        staging.rename(output)
    return output


def _worktree(repo: Path, requested: Path, ref: str) -> Path | None:
    """The worktree path; None when it does not exist yet."""
    worktree = requested.absolute()
    if worktree.is_symlink() or any(parent.is_symlink() for parent in worktree.parents):
        raise ValueError("worktree must not traverse symlinks")
    worktree = worktree.resolve()
    if worktree == repo.resolve() or worktree in repo.resolve().parents:
        raise ValueError("worktree must not contain the matching checkout")
    for entry in git(repo, "worktree", "list", "--porcelain").split("\n\n"):
        lines = entry.splitlines()
        if f"branch {ref}" in lines:
            existing = Path(lines[0].removeprefix("worktree ")).resolve()
            if existing != worktree:
                raise ValueError(f"{ref}: already checked out at {existing}")
    if worktree.exists():
        if not (worktree / ".git").is_file() or git(worktree, "symbolic-ref", "HEAD") != ref:
            raise ValueError(f"{worktree}: not a worktree of {ref}")
        if git(worktree, "status", "--porcelain", "--untracked-files=all"):
            raise ValueError(f"{worktree}: the worktree has local changes")
    return worktree


def _tip(repo: Path, ref: str) -> str | None:
    found = subprocess.run(["git", "-C", str(repo), "rev-parse", "--verify", "--quiet", ref],
                           capture_output=True, text=True)
    return found.stdout.strip() if found.returncode == 0 else None


def _replace_tree(worktree: Path, files: dict[str, bytes]) -> None:
    tracked = set(git(worktree, "ls-files", "-z").split("\0")) - {""}
    for name in files:
        if (worktree / name).exists() and name not in tracked:
            raise ValueError(f"{worktree / name}: would overwrite an untracked or ignored file")
    for name in sorted(tracked - set(files)):
        git(worktree, "rm", "--quiet", "--", name)
    for name, data in sorted(files.items()):
        target = worktree / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    git(worktree, "add", "--all", "--", *sorted(files))


def publish(repo: Path, files: dict[str, bytes], commit: str, branch: str,
            requested: Path) -> tuple[Path, bool]:
    """Replace `branch` with one root commit holding `files`; (worktree, changed).

    The branch is local; nothing is pushed. Its previous tip must be a
    generated commit, and a regeneration from the same commit with the same
    content changes nothing.
    """
    ref = f"refs/heads/{branch}"
    if git(repo, "branch", "--show-current") == branch:
        raise ValueError("cannot publish over the current branch")
    tip = _tip(repo, ref)
    if tip and PROVENANCE not in git(repo, "log", "-1", "--format=%B", tip).splitlines():
        raise ValueError(f"{branch}: the branch tip was not generated by giten branch")
    worktree = _worktree(repo, requested, ref)
    if not worktree.exists():
        worktree.parent.mkdir(parents=True, exist_ok=True)
        if tip:
            git(repo, "worktree", "add", str(worktree), branch)
        else:
            git(repo, "worktree", "add", "--orphan", "-b", branch, str(worktree))
    if tip:
        tracked = set(git(worktree, "ls-files", "-z").split("\0")) - {""}
        same = tracked == set(files) and all(
            (worktree / name).read_bytes() == data for name, data in files.items())
        root = len(git(repo, "rev-list", "--parents", "-1", tip).split()) == 1
        if same and root and f"Source-Commit: {commit}" in git(
                repo, "log", "-1", "--format=%B", tip).splitlines():
            return worktree, False
    _replace_tree(worktree, files)
    message = f"{branch}: regenerate from {commit[:12]}\n\n{PROVENANCE}\nSource-Commit: {commit}\n"
    tree = git(worktree, "write-tree")
    new_tip = git(repo, "commit-tree", tree, "-m", message)
    git(repo, "update-ref", ref, new_tip, tip or "")
    return worktree, True


def seed_port(repo: Path, files: dict[str, bytes], commit: str, requested: Path,
              source_branch: str = "source") -> Path:
    """Create `port` once: the port tree as a commit on top of the source tip."""
    ref = "refs/heads/port"
    if _tip(repo, ref):
        raise ValueError("port exists; it is maintained by hand and never regenerated")
    base = _tip(repo, f"refs/heads/{source_branch}")
    if base is None:
        raise ValueError(f"no {source_branch} branch; publish it first")
    if f"Source-Commit: {commit}" not in git(repo, "log", "-1", "--format=%B", base).splitlines():
        raise ValueError(f"{source_branch} was not generated from {commit[:12]}; "
                         "regenerate it first")
    worktree = requested.absolute().resolve()
    if worktree.exists():
        raise ValueError(f"{worktree} exists")
    git(repo, "worktree", "add", "-b", "port", str(worktree), base)
    _replace_tree(worktree, files)
    git(worktree, "commit", "--quiet", "--no-verify", "-m",
        f"port: seed from {source_branch} {base[:12]} with the fixes enabled\n\n"
        f"Source-Commit: {commit}\n")
    return worktree
