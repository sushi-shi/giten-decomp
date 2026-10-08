"""Reconstruct observed import metadata in authentic SDK COFF archives.

The installed archives are immutable format/ABI witnesses. Generated archives
retain their complete real membership, member names, indexes, COFF/debug data,
relocations and symbols; only reviewed named-import hint fields are serialized
from the contract. No stub exports, synthetic members or image edits occur.
This reconstructs the observed import contract, not the missing original LIB.
"""
from __future__ import annotations

import csv
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path

from giten.core.paths import CONFIG

CONTRACT = CONFIG / "retail/imports.tsv"


@dataclass(frozen=True)
class Import:
    dll: str
    dll_order: int
    lookup_order: int
    kind: str
    lookup: str
    hint_or_ordinal: int
    caller: str
    member: str
    sdk_library: str
    sdk_sha256: str


def read_contract(path: Path = CONTRACT) -> list[Import]:
    with path.open() as handle:
        rows = list(csv.DictReader((line for line in handle if not line.startswith("#")), delimiter="\t"))
    imports = []
    seen = set()
    for row in rows:
        spec = Import(**{k: int(v) if k in ("dll_order", "lookup_order", "hint_or_ordinal") else v for k, v in row.items()})
        if spec.kind not in ("name", "ordinal") or not 0 <= spec.hint_or_ordinal <= 0xffff:
            raise ValueError("contract requires a proven import and WORD hint/ordinal")
        if spec.kind == "ordinal" and (spec.lookup or spec.hint_or_ordinal == 0):
            raise ValueError("ordinal imports must have no lookup name and a nonzero ordinal")
        if not spec.caller or len(spec.sdk_sha256) != 64:
            raise ValueError("missing caller ABI or SDK archive provenance")
        key = (spec.dll.lower(), spec.lookup)
        if key in seen:
            raise ValueError(f"duplicate import: {key}")
        seen.add(key)
        imports.append(spec)
    if not imports:
        raise ValueError("empty import contract")
    orders = sorted({s.dll_order for s in imports})
    if orders != list(range(len(orders))):
        raise ValueError("noncontiguous DLL order")
    for order in orders:
        group = [s for s in imports if s.dll_order == order]
        if len({(s.dll, s.sdk_library, s.sdk_sha256) for s in group}) != 1:
            raise ValueError("conflicting DLL/library provenance")
        if sorted(s.lookup_order for s in group) != list(range(len(group))):
            raise ValueError("noncontiguous lookup order")
    return imports


def symbols(body: bytes) -> dict[str, tuple[int, int, int]]:
    if len(body) < 20 or body[:2] != b"\x4c\x01":
        return {}
    pointer, count = struct.unpack_from("<II", body, 8)
    strings = pointer + count * 18
    if strings + 4 > len(body):
        raise ValueError("truncated COFF symbols")
    out = {}
    index = 0
    while index < count:
        at = pointer + index * 18
        raw = body[at:at + 8]
        if raw[:4] == bytes(4):
            start = strings + struct.unpack_from("<I", raw, 4)[0]
            name = body[start:body.index(0, start)].decode("latin1")
        else:
            name = raw.rstrip(b"\0").decode("latin1")
        value, section, _type, storage, aux = struct.unpack_from("<IhHBB", body, at + 8)
        out[name] = (value, section, storage)
        index += 1 + aux
    return out


def archive_members(data: bytes) -> list[dict]:
    if data[:8] != b"!<arch>\n":
        raise ValueError("not a Microsoft archive")
    members = []
    offset = 8
    longnames = b""
    while offset < len(data):
        header = data[offset:offset + 60]
        if len(header) != 60 or header[58:60] != b"`\n":
            raise ValueError("invalid archive member header")
        size = int(header[48:58])
        body = data[offset + 60:offset + 60 + size]
        if len(body) != size:
            raise ValueError("truncated archive member")
        name = header[:16].decode("ascii").strip()
        if name == "//":
            longnames = body
        if name.startswith("/") and name[1:].isdigit():
            start = int(name[1:])
            name = longnames[start:longnames.index(0, start)].decode("latin1")
        pad = data[offset + 60 + size:offset + 60 + size + (size & 1)]
        if len(pad) != (size & 1):
            raise ValueError("truncated archive alignment")
        members.append({"offset": offset, "header": header, "name": name, "body": body, "pad": pad})
        offset += 60 + size + (size & 1)
    validate_indexes(members)
    return members


def validate_indexes(members: list[dict]) -> None:
    """Check both real archive indexes against member-defined publics."""
    indexes = [m for m in members if m["name"] == "/"]
    if len(indexes) != 2:
        raise ValueError("expected two Microsoft archive indexes")
    actual = {m["offset"]: symbols(m["body"]) for m in members if m["name"] not in ("/", "//")}
    def check(offset, name):
        value, section, storage = actual.get(offset, {}).get(name, (0, 0, 0))
        if storage != 2 or not (section > 0 or section == -1 or section == 0 and value > 0):
            raise ValueError(f"archive index does not reach definition: {name}")
    first = indexes[0]["body"]
    count = struct.unpack_from(">I", first)[0]
    offsets = struct.unpack_from(f">{count}I", first, 4)
    names = first[4 + 4 * count:].split(b"\0")
    if len(names) < count + 1:
        raise ValueError("truncated first archive index")
    for offset, name in zip(offsets, names[:count]):
        check(offset, name.decode("latin1"))
    second = indexes[1]["body"]
    nmember = struct.unpack_from("<I", second)[0]
    offsets = struct.unpack_from(f"<{nmember}I", second, 4)
    if any(offset not in actual for offset in offsets):
        raise ValueError("second archive member index points outside members")
    at = 4 + 4 * nmember
    count = struct.unpack_from("<I", second, at)[0]
    ranks = struct.unpack_from(f"<{count}H", second, at + 4)
    names = second[at + 4 + 2 * count:].split(b"\0")
    if len(names) < count + 1 or any(not 1 <= rank <= nmember for rank in ranks):
        raise ValueError("invalid second archive symbol index")
    for rank, name in zip(ranks, names[:count]):
        check(offsets[rank - 1], name.decode("latin1"))


def lookup_field(body: bytes, caller: str) -> tuple[str, int, int] | None:
    publics = symbols(body)
    if caller not in publics or "__imp_" + caller not in publics:
        return None
    for name in (caller, "__imp_" + caller):
        if publics[name][1] <= 0 or publics[name][2] != 2:
            return None
    nsection, optional = struct.unpack_from("<H", body, 2)[0], struct.unpack_from("<H", body, 16)[0]
    found = []
    for index in range(nsection):
        at = 20 + optional + index * 40
        if body[at:at + 8] != b".idata$6":
            continue
        size, pointer = struct.unpack_from("<II", body, at + 16)
        if pointer + size > len(body) or size < 3:
            raise ValueError("truncated hint/name record")
        end = body.index(0, pointer + 2, pointer + size)
        found.append((body[pointer + 2:end].decode("ascii"), struct.unpack_from("<H", body, pointer)[0], pointer))
    if len(found) != 1:
        raise ValueError(f"expected one named lookup for {caller}")
    return found[0]


def reconstruct(source: Path, specs: list[Import]) -> tuple[bytes, dict]:
    original = source.read_bytes()
    digest = hashlib.sha256(original).hexdigest()
    if {s.sdk_sha256 for s in specs} != {digest}:
        raise ValueError(f"SDK archive hash mismatch: {source}")
    members = archive_members(original)
    observations = []
    for spec in specs:
        if spec.kind == "ordinal":
            from giten.delink.implib import _coff_import_ordinal_and_imp
            candidates = [m for m in members if _coff_import_ordinal_and_imp(m["body"]) == (spec.hint_or_ordinal, "__imp_" + spec.caller)]
            if len(candidates) != 1 or candidates[0]["name"] != spec.member:
                raise ValueError(f"missing/ambiguous SDK ordinal binding: {spec.caller}")
            observations.append({"caller": spec.caller, "member": spec.member,
                                 "ordinal": spec.hint_or_ordinal, "sdk_hint": spec.hint_or_ordinal,
                                 "hint": spec.hint_or_ordinal})
            continue
        candidates = []
        for member in members:
            field = lookup_field(member["body"], spec.caller)
            if field is not None:
                candidates.append((member, field))
        if len(candidates) != 1:
            raise ValueError(f"missing/ambiguous SDK caller: {spec.caller}")
        member, (lookup, hint, pointer) = candidates[0]
        if lookup != spec.lookup or member["name"] != spec.member:
            raise ValueError(f"SDK lookup/member differs: {spec.caller}")
        # Serialize the single observed WORD metadata field into a fresh
        # member. Its complete authentic COFF structure stays intact.
        body = member["body"]
        member["body"] = body[:pointer] + struct.pack("<H", spec.hint_or_ordinal) + body[pointer + 2:]
        observations.append({"lookup": lookup, "caller": spec.caller, "member": member["name"],
                             "sdk_hint": hint, "hint": spec.hint_or_ordinal,
                             "archive_field_offset": member["offset"] + 60 + pointer})
    generated = b"!<arch>\n" + b"".join(m["header"] + m["body"] + m["pad"] for m in members)
    rebuilt = archive_members(generated)
    if len(rebuilt) != len(members):
        raise ValueError("archive member count changed")
    allowed = {o["archive_field_offset"] + byte for o in observations if "archive_field_offset" in o for byte in (0, 1)}
    changes = [i for i, (a, b) in enumerate(zip(original, generated)) if a != b]
    if len(generated) != len(original) or any(i not in allowed for i in changes):
        raise ValueError("unexpected archive content change")
    return generated, {"sdk_path": str(source), "sdk_sha256": digest,
                       "archive_sha256": hashlib.sha256(generated).hexdigest(),
                       "members": len(members), "changed_bytes": changes, "imports": observations}


def cache_key(contract: Path, source: Path) -> dict:
    return {"contract": hashlib.sha256(contract.read_bytes()).hexdigest(),
            "generator": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "sdk": hashlib.sha256(source.read_bytes()).hexdigest()}


def generate(contract: Path, source: Path, specs: list[Import], output: Path) -> dict:
    payload, proof = reconstruct(source, specs)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(payload)
    proof["cache_key"] = cache_key(contract, source)
    output.with_suffix(".json").write_text(json.dumps(proof, indent=2) + "\n")
    return proof
