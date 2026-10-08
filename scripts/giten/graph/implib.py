"""Reconstruct Giten's reviewed import metadata without fictitious exports.

DDS.EXE imports eight DLLs. Authentic VC5/DX import archives cover every
caller ABI, including DSOUND's ordinal1 import. config/retail/imports.tsv
records the observed lookup names/ordinals/hints and SHA-pinned SDK members.
Only archives with differing hints need generated replacements. The link
places each replacement at its existing library-line position; no extra roots,
DLLs, filler exports, installed-library edits or EXE rewriting are involved.
"""
from __future__ import annotations

import hashlib
import json
import struct
from pathlib import Path

from giten.core.paths import BUILD, dxsdk_dir, msvc_dir
from giten.core.pe import Pe
from giten.delink.image import Image
from giten.graph.import_contract import CONTRACT, cache_key, generate, read_contract, reconstruct
from giten.tool import ToolError
from giten.tool.wine import find_ci

OUT_DIR = BUILD / "lib"


def import_table(pe: Pe) -> dict[str, dict[str, int]]:
    """Named import hint records only; ordinal identities are read separately."""
    image = Image(pe)
    out = {}
    rva = pe.directories[1][0]
    def raw(at):
        offset = image.off(at)
        if offset is None:
            raise ValueError("import address outside raw image")
        return offset
    descriptor = raw(rva)
    while True:
        lookup, stamp, forwarder, name, iat = struct.unpack_from("<IIIII", pe.data, descriptor)
        if not any((lookup, stamp, forwarder, name, iat)):
            break
        at = raw(name)
        dll = pe.data[at:pe.data.index(0, at)].decode("ascii")
        out[dll] = {}
        thunk = raw(lookup or iat)
        while True:
            target = struct.unpack_from("<I", pe.data, thunk)[0]
            if not target:
                break
            if not target & 0x80000000:
                at = raw(target)
                hint = struct.unpack_from("<H", pe.data, at)[0]
                lookup_name = pe.data[at + 2:pe.data.index(0, at + 2)].decode("ascii")
                out[dll][lookup_name] = hint
            thunk += 4
        descriptor += 20
    return out


def lib_dirs() -> list[Path]:
    return [dxsdk_dir() / "Lib", msvc_dir() / "lib"]


def toolchain_lib(stem: str) -> Path | None:
    return next((found for directory in lib_dirs() if (found := find_ci(directory, stem + ".lib"))), None)


def _groups(contract: Path):
    specs = read_contract(contract)
    original = Pe(BUILD / "local/DDS.EXE")
    named = import_table(original)
    actual = [(dll, "ordinal" if ordinal is not None else "name", name or "", ordinal if ordinal is not None else named[dll][name])
              for _slot, name, dll, ordinal in Image(original).import_slots()]
    expected = [(s.dll, s.kind, s.lookup, s.hint_or_ordinal) for s in sorted(specs, key=lambda s: (s.dll_order, s.lookup_order))]
    if actual != expected:
        raise ValueError("import contract differs from original DDS.EXE lookup identities/order")
    for order in sorted({s.dll_order for s in specs}):
        group = [s for s in specs if s.dll_order == order]
        source = toolchain_lib(Path(group[0].sdk_library).stem)
        if source is None:
            raise ValueError(f"missing authentic SDK archive: {group[0].sdk_library}")
        payload, proof = reconstruct(source, group)
        yield group, source, proof


def _key(contract: Path, source: Path):
    key = cache_key(contract, source)
    key["driver"] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    return key


def _valid(output: Path, key: dict) -> bool:
    try:
        proof = json.loads(output.with_suffix(".json").read_text())
        return proof["cache_key"] == key and proof["archive_sha256"] == hashlib.sha256(output.read_bytes()).hexdigest()
    except (OSError, ValueError, KeyError):
        return False


def on_disk(out_dir: Path = OUT_DIR, contract: Path = CONTRACT) -> list[Path]:
    """Only valid generated replacements, without creating build artifacts."""
    out = []
    for group, source, proof in _groups(contract):
        if not proof["changed_bytes"]:
            continue
        path = out_dir / (Path(group[0].dll).stem.lower() + ".lib")
        if _valid(path, _key(contract, source)):
            out.append(path)
    return out


def ensure_all(out_dir: Path = OUT_DIR, verbose: bool = True, contract: Path = CONTRACT) -> list[Path]:
    out = []
    for group, source, proof in _groups(contract):
        if not proof["changed_bytes"]:
            continue
        path = out_dir / (Path(group[0].dll).stem.lower() + ".lib")
        key = _key(contract, source)
        if not _valid(path, key):
            proof = generate(contract, source, group, path)
            proof["cache_key"] = key
            path.with_suffix(".json").write_text(json.dumps(proof, indent=2) + "\n")
        out.append(path)
        if verbose:
            print(f"[implib] {group[0].dll}: {len(group)} observed imports, authentic SDK members -> {path}")
    return out


def main() -> int:
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--out-dir", type=Path, default=OUT_DIR)
    parser.add_argument("--contract", type=Path, default=CONTRACT)
    args = parser.parse_args()
    try:
        if args.list:
            for group, source, proof in _groups(args.contract):
                changed = sum(row["sdk_hint"] != row["hint"] for row in proof["imports"])
                print(f"{group[0].dll:16s} {len(group):3d} imports {changed:2d} changed hints {source}")
        else:
            ensure_all(args.out_dir, contract=args.contract)
        return 0
    except (OSError, RuntimeError, ValueError, struct.error, ToolError) as error:
        print(f"[implib] {error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
