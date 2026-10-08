"""Validate genuine static-library inputs without rebuilding their contents."""
from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
import hashlib
from pathlib import Path
import re
import struct
from typing import Mapping

from giten.core.paths import BUILD, REPO, RETAIL
from giten.core.pe import Pe
from giten.graph.import_contract import archive_members, symbols

CONTRACT = RETAIL / "link_libraries.tsv"
ORIGINAL_RETAIL = BUILD / "local/DDS.EXE"


@dataclass(frozen=True)
class Library:
    library: str
    artifact: str
    archive_size: int
    archive_sha256: str
    member: str
    member_sha256: str
    section: str
    alignment: int
    guid_count: int
    retail_rva: int
    payload_size: int
    payload_sha256: str
    provenance: str
    package_url: str
    package_size: int
    package_sha256: str
    package_sha1: str
    package_md5: str


def read_contract(path: Path = CONTRACT) -> list[Library]:
    with Path(path).open() as handle:
        rows = csv.DictReader((line for line in handle if not line.startswith("#")), delimiter="\t")
        result = []
        seen = set()
        integers = {"archive_size", "alignment", "guid_count", "retail_rva", "payload_size", "package_size"}
        for row in rows:
            spec = Library(**{k: int(v, 0) if k in integers else v for k, v in row.items()})
            name = spec.library.lower()
            if name in seen or Path(name).name != name or not name.endswith(".lib"):
                raise ValueError(f"invalid or duplicate static library: {spec.library}")
            seen.add(name)
            for field, length in [("archive_sha256", 64), ("member_sha256", 64),
                                  ("payload_sha256", 64), ("package_sha256", 64),
                                  ("package_sha1", 40), ("package_md5", 32)]:
                if not re.fullmatch(rf"[0-9a-f]{{{length}}}", getattr(spec, field)):
                    raise ValueError(f"invalid {field}: {spec.library}")
            if (not spec.artifact or not spec.member or spec.section != ".rdata"
                    or spec.guid_count <= 0 or spec.payload_size != 16 * spec.guid_count
                    or spec.retail_rva < 0 or spec.archive_size <= 0 or spec.package_size <= 0
                    or spec.alignment <= 0 or spec.alignment & (spec.alignment - 1)
                    or not spec.provenance or not spec.package_url.startswith("https://")):
                raise ValueError(f"incomplete GUID-library contract: {spec.library}")
            result.append(spec)
    if not result:
        raise ValueError("empty static-library contract")
    return result


def validate(spec: Library, artifact: Path, retail: Pe) -> Path:
    """Check the archive, original member and complete unmodified retail span."""
    artifact = Path(artifact)
    if not artifact.is_file():
        raise ValueError(f"missing authentic {spec.library}: {artifact}; supply the SDK artifact "
                         f"identified by {CONTRACT}; the installed SDK is not a fallback")
    data = artifact.read_bytes()
    if len(data) != spec.archive_size or hashlib.sha256(data).hexdigest() != spec.archive_sha256:
        raise ValueError(f"static archive size/hash mismatch: {artifact}")
    try:
        members = archive_members(data)  # Also validates both Microsoft symbol indexes.
        definitions = [m for m in members if m["body"][:2] == b"\x4c\x01"]
        matching = [m for m in definitions if m["name"] == spec.member]
        if len(definitions) != 1 or len(matching) != 1:
            raise ValueError("expected the single original GUID member")
        body = matching[0]["body"]
        if hashlib.sha256(body).hexdigest() != spec.member_sha256:
            raise ValueError("original member hash mismatch")
        nsections, optional = struct.unpack_from("<H12xH", body, 2)
        sections = []
        for index in range(nsections):
            at = 20 + optional + index * 40
            name = body[at:at + 8].rstrip(b"\0").decode("ascii")
            size, raw = struct.unpack_from("<II", body, at + 16)
            relocations = struct.unpack_from("<H", body, at + 32)[0]
            characteristics = struct.unpack_from("<I", body, at + 36)[0]
            if name == spec.section:
                sections.append((index + 1, size, raw, relocations, characteristics))
            elif name != ".drectve":
                raise ValueError("unexpected original GUID member section")
        if len(sections) != 1:
            raise ValueError("missing or duplicate GUID payload section")
        number, size, raw, relocations, characteristics = sections[0]
        alignment_code = (characteristics >> 20) & 15
        alignment = 1 << (alignment_code - 1) if alignment_code else 1
        if (size != spec.payload_size or relocations or alignment != spec.alignment
                or characteristics & (0x80000000 | 0x1000 | 0x20)
                or characteristics & 0x40000040 != 0x40000040
                or raw < 20 + optional + nsections * 40 or raw + size > len(body)):
            raise ValueError("GUID payload is not the pinned ordinary readonly section")
        payload = body[raw:raw + size]
        if hashlib.sha256(payload).hexdigest() != spec.payload_sha256:
            raise ValueError("GUID payload hash mismatch")
        publics = [(value, name) for name, (value, section, storage) in symbols(body).items()
                   if section == number and storage == 2]
        if (len(publics) != spec.guid_count
                or sorted(value for value, _name in publics) != list(range(0, size, 16))):
            raise ValueError("GUID definitions/count/order differ from the contract")
        section = retail.section(spec.section)
        if not (section["va"] <= spec.retail_rva
                and spec.retail_rva + size <= section["va"] + min(section["vsize"], section["rsize"])):
            raise ValueError("retail GUID span is not entirely stored section data")
        if retail.read(spec.retail_rva, size) != payload:
            raise ValueError("GUID payload differs from the original executable")
    except (IndexError, KeyError, UnicodeError, struct.error) as error:
        raise ValueError(f"malformed GUID archive or retail image: {artifact}") from error
    return artifact.resolve()


def on_disk(contract: Path = CONTRACT, *, retail: Path = ORIGINAL_RETAIL,
            artifacts: Mapping[str, Path] | None = None) -> dict[str, Path]:
    """Return validated replacements keyed by logical filename, for in-place use."""
    specs = read_contract(contract)
    overrides = {name.lower(): Path(path) for name, path in (artifacts or {}).items()}
    if len(overrides) != len(artifacts or {}) or overrides.keys() - {s.library.lower() for s in specs}:
        raise ValueError("unknown or duplicate static-library artifact override")
    image = Pe(retail)  # ORIGINAL input, never the relocation-augmented oracle.
    return {s.library.lower(): validate(s, overrides.get(s.library.lower(), REPO / s.artifact), image)
            for s in specs}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--contract", type=Path, default=CONTRACT)
    parser.add_argument("--retail", type=Path, default=ORIGINAL_RETAIL)
    args = parser.parse_args()
    for name, path in on_disk(args.contract, retail=args.retail).items():
        print(f"{name}\t{path}")


if __name__ == "__main__":
    main()
