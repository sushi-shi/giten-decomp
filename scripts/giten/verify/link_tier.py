"""giten.verify.link_tier - the candidate-EXE audits (link tier, opt-in).

Needs `giten link`'s outputs (DDS.candidate.EXE + .map). Four checks
folded into one tier module:

  LINK DEFECTS   the link must be REAL: 0 unresolved externals (the linker's
                 own unresolved.txt), and a pre-link closure over the base
                 objs - every undefined external resolves from a base obj
                 definition/COMMON/weak-alias, a toolchain .LIB, or an
                 import. A symbol none of those supply is a guaranteed
                 `unresolved external symbol` (the declared-only family).
  SECTION CENSUS the candidate's section table vs retail's - per-section raw
                 sizes and deltas (the link-order study's byte budget); a
                 section retail has that the candidate lacks is FATAL.
  IMAGE DIFF     for every claimed function the compare report scores at
                 100%, the candidate's bytes at its LINK-ASSIGNED rva (the
                 .map) must equal retail's at the retail rva under reloc
                 masking (both sides' 4-byte fixup fields blanked via each
                 image's own .reloc table). The linked-image check is what
                 objdiff's per-obj scoring cannot see: final placement.
  RESOURCES      the candidate's .rsrc (compiled from src/Giten/Giten.rc)
                 against retail's: tree, payloads, code pages, layout and
                 section-relative bytes (giten.rsrc.check).

    giten verify link-tier             # the four checks, exit 1 on any
    giten verify link-tier --census    # the section table only
    giten verify link-tier --strict --report build/link-identity.json
                                      # original raw EXE; MAX partial bodies only
"""

from __future__ import annotations

import re
import struct
import sys
import hashlib
import json
from pathlib import Path

from giten.core.paths import BUILD

CAND = BUILD / "exe/DDS.candidate.EXE"
CMAP = BUILD / "exe/DDS.candidate.map"
UNRESOLVED = BUILD / "exe/DDS.candidate.unresolved.txt"

_MAP_ROW = re.compile(r"^ (\d{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})")


def strict_map(path: Path, image_base: int) -> dict[str, dict]:
    """Read both public and static rows; preserve conflicting addresses.

    Linker maps do not state function extents. A following map symbol is not
    an extent oracle (aliases, labels and contribution padding intervene).
    """
    text = path.read_text(errors="strict")
    base = re.search(r"Preferred load address is\s+([0-9a-fA-F]+)", text)
    if base is None or int(base[1], 16) != image_base:
        raise ValueError("map preferred base missing or differs from candidate PE")
    out = {}
    for line in text.splitlines():
        match = _MAP_ROW.match(line.lower())
        if not match:
            continue
        # Keep the original case of decorated symbols and object names.
        cells = line.split()
        name, va = cells[1], int(cells[2], 16)
        row = out.setdefault(name, {"rvas": [], "objects": []})
        rva = va - image_base
        if rva not in row["rvas"]:
            row["rvas"].append(rva)
        owner = cells[-1]
        if owner not in row["objects"]:
            row["objects"].append(owner)
    if not out:
        raise ValueError("map has no symbol rows")
    return out


def partial_exclusions(ledger: dict) -> list[dict]:
    """Exclude known partial MAX bodies, never unrelated current dips."""
    rows = []
    seen = set()
    for (unit, name), entry in sorted(ledger.items()):
        if entry["best"] >= 100.0:
            continue
        rva = entry["addr"]
        if rva is None or rva in seen or entry.get("state") == "absent":
            raise ValueError(f"invalid/duplicate partial identity: {unit}:{name}")
        seen.add(rva)
        rows.append({"unit": unit, "name": name, "retail_rva": rva,
                     "source_hash": entry["fp"], "max": entry["best"],
                     "cur": entry["cur"]})
    return rows


def raw_difference(retail: bytes, candidate: bytes,
                   excluded: list[tuple[int, int]] = ()) -> dict:
    """Count every differing byte; bounded samples are explicitly labeled.

    Intervals address this region, not relocation fields. Bytes present on
    only one side always differ, including when that tail is excluded.
    """
    excluded = sorted(excluded)
    for i, (lo, hi) in enumerate(excluded):
        if not 0 <= lo < hi <= min(len(retail), len(candidate)):
            raise ValueError("exclusion outside common raw extent")
        if i and lo < excluded[i - 1][1]:
            raise ValueError("overlapping exclusions")
    count = 0
    samples = []
    interval = 0
    for offset in range(max(len(retail), len(candidate))):
        while interval < len(excluded) and offset >= excluded[interval][1]:
            interval += 1
        if interval < len(excluded) and excluded[interval][0] <= offset:
            continue
        a = retail[offset] if offset < len(retail) else None
        b = candidate[offset] if offset < len(candidate) else None
        if a != b:
            count += 1
            if len(samples) < 64:
                samples.append({"offset": offset, "retail": a, "candidate": b})
    return {"equal": count == 0, "different_bytes": count,
            "sample_limit": 64, "samples_truncated": count > len(samples),
            "samples": samples, "excluded_bytes": sum(hi - lo for lo, hi in excluded),
            "retail_size": len(retail), "candidate_size": len(candidate),
            "retail_sha256": hashlib.sha256(retail).hexdigest(),
            "candidate_sha256": hashlib.sha256(candidate).hexdigest()}


def _pe_headers(pe) -> tuple[dict, bytes, bytes]:
    nt = struct.unpack_from("<I", pe.data, 0x3c)[0]
    optional = nt + 24
    header_size = struct.unpack_from("<I", pe.data, optional + 60)[0]
    if header_size > len(pe.data):
        raise ValueError("truncated PE headers")
    end = max([header_size] + [s["rptr"] + s["rsize"] for s in pe.sections])
    if end > len(pe.data):
        raise ValueError("truncated PE section")
    fields = {"image_base": pe.image_base, "timestamp": struct.unpack_from("<I", pe.data, nt + 8)[0],
              "entry_point": struct.unpack_from("<I", pe.data, optional + 16)[0],
              "size_of_image": struct.unpack_from("<I", pe.data, optional + 56)[0],
              "size_of_headers": header_size, "directories": pe.directories}
    return fields, pe.data[:header_size], pe.data[end:]


def strict_report(retail_path: Path, expected_partials: int | None = None) -> dict:
    """Raw binary identity audit. No score-selected bodies or fixup masks.

    Object extents/references are diagnostic provenance, not a substitute for
    linked bytes. Every section byte outside validated partial ranges is
    compared, including unclaimed code, imports, resources and alignment.
    """
    from giten.core.pe import Pe
    from giten.delink.coffx import Obj
    from giten.model import resolve
    from giten.verify.baseline import load
    from giten.walls.diagnose import _find_function

    report = {"schema": 1, "mode": "raw-identity-except-MAX-partial-bodies",
              "inputs": {}, "exclusions": [], "functions": [], "sections": [],
              "reference_coverage": "candidate owner COFF fields; retail field interpretation is diagnostic, not independently proven relocation-site identity",
              "findings": []}
    bad = report["findings"]
    try:
        retail, candidate = Pe(retail_path), Pe(CAND)
        symbols = strict_map(CMAP, candidate.image_base)
        map_stamp = re.search(r"Timestamp is\s+([0-9a-fA-F]+)", CMAP.read_text())
        nt = struct.unpack_from("<I", candidate.data, 0x3c)[0]
        if map_stamp is None or int(map_stamp[1], 16) != struct.unpack_from("<I", candidate.data, nt + 8)[0]:
            raise ValueError("map timestamp missing or differs from candidate PE")
        ledger = load()
        exclusions = partial_exclusions(ledger)
        report["exclusions"] = exclusions
        if expected_partials is not None and len(exclusions) != expected_partials:
            bad.append(f"expected {expected_partials} partial bodies, found {len(exclusions)}")
        for label, path in [("retail", retail_path), ("candidate", CAND), ("map", CMAP)]:
            report["inputs"][label] = {"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
        from giten.verify.baseline import BASELINE
        report["inputs"]["baseline"] = {"path": str(BASELINE), "sha256": hashlib.sha256(BASELINE.read_bytes()).hexdigest()}
        objects = {}
        excluded_by_rva = {row["retail_rva"]: row for row in exclusions}
        ranges = []
        visited = set()
        for binding in resolve().functions:
            if not binding.name:
                continue
            row = {"name": binding.name, "unit": binding.unit,
                   "retail_rva": binding.rva, "retail_size": binding.size,
                   "excluded_body": binding.rva in excluded_by_rva}
            entry = ledger.get((binding.unit, binding.name))
            if entry:
                row["ledger"] = {key: entry[key] for key in ("best", "cur", "fp", "state")}
            report["functions"].append(row)
            errors = row["findings"] = []
            located = symbols.get(binding.name)
            if located is None or len(located["rvas"]) != 1:
                errors.append("missing or ambiguous map symbol")
                continue
            crva = located["rvas"][0]
            row["candidate_rva"] = crva
            row["map_objects"] = located["objects"]
            if crva != binding.rva:
                errors.append("function RVA differs")
            map_owner = located["objects"][0].replace("\\", "/").split("/")[-1]
            mapped_path = BUILD / "objdiff/base" / map_owner
            obj_path = mapped_path if mapped_path.is_file() else BUILD / "objdiff/base" / (binding.unit + ".obj")
            row["reference_object"] = str(obj_path) if obj_path.is_file() else None
            row["reference_object_is_map_owner"] = obj_path == mapped_path
            # Only claimed authored owners have base objects. Library/unclaimed
            # model rows remain included by the complete section comparison.
            body, relocs, size = None, {}, None
            if binding.unit or mapped_path.is_file():
                if not obj_path.is_file():
                    errors.append("missing owner object")
                else:
                    if obj_path != mapped_path:
                        errors.append("selected map-owner object unavailable; references use claimed owner")
                    if obj_path not in objects:
                        objects[obj_path] = Obj(obj_path)
                        report["inputs"].setdefault("objects", {})[str(obj_path)] = hashlib.sha256(obj_path.read_bytes()).hexdigest()
                    body, relocs, size = _find_function(objects[obj_path], binding.name)
                    if body is None:
                        errors.append("function missing from owner object")
                    elif size != binding.size:
                        row.setdefault("diagnostics", []).append(
                            "trimmed owner-object extent differs from retail model extent; "
                            "heuristic extent is not an independent linked-body size oracle")
            row["candidate_object_size"] = size
            row["object_extent_method"] = "next-defined-member, trailing CC/NOP trimmed" if size is not None else None
            if obj_path.is_file() and obj_path.stat().st_mtime > CAND.stat().st_mtime:
                errors.append("candidate image older than owner object")
            rb = retail.read(binding.rva, binding.size)
            cb = candidate.read(crva, binding.size)
            if rb is None or cb is None:
                errors.append("unreadable or truncated linked function range")
                continue
            row["raw_bytes"] = raw_difference(rb, cb)
            if not row["excluded_body"] and rb != cb:
                errors.append("raw linked function bytes differ")
            references = row["references"] = []
            for offset, (target, kind) in sorted((relocs or {}).items()):
                ref = {"offset": offset, "coff_target": target, "coff_type": kind,
                       "candidate_site_rva": crva + offset, "retail_site_rva": binding.rva + offset}
                references.append(ref)
                if body is None or offset + 4 > len(body) or offset + 4 > len(rb):
                    ref["error"] = "relocation field outside compared body"
                    if not row["excluded_body"]:
                        errors.append("unreadable reference field")
                    continue
                ref["object_raw_addend"] = struct.unpack_from("<I", body, offset)[0]
                rv, cv = struct.unpack_from("<I", rb, offset)[0], struct.unpack_from("<I", cb, offset)[0]
                ref.update(retail_raw_value=rv, candidate_raw_value=cv, raw_equal=rv == cv)
                if kind == 0x14:  # IMAGE_REL_I386_REL32
                    ref["retail_field_interpreted_target_rva"] = (binding.rva + offset + 4 + rv) & 0xffffffff
                    ref["candidate_target_rva"] = (crva + offset + 4 + cv) & 0xffffffff
                elif kind == 6:  # IMAGE_REL_I386_DIR32
                    ref["retail_field_interpreted_target_rva"] = rv - retail.image_base
                    ref["candidate_target_rva"] = cv - candidate.image_base
                ref["site_correspondence"] = "same offset; not proven retail relocation site"
            if row["excluded_body"]:
                exclusion = excluded_by_rva[binding.rva]
                visited.add(binding.rva)
                if exclusion["name"] != binding.name or exclusion["unit"] != binding.unit:
                    errors.append("partial identity does not match model owner/name")
                elif crva == binding.rva:
                    ranges.append((binding.rva, binding.rva + binding.size))
                    exclusion["size"] = binding.size
                    exclusion["applied"] = True
        for row in exclusions:
            if row["retail_rva"] not in visited:
                bad.append(f"partial exclusion not located: {row['name']}")
        rh, rheader, roverlay = _pe_headers(retail)
        ch, cheader, coverlay = _pe_headers(candidate)
        report["headers"] = {"retail": rh, "candidate": ch,
                             "raw": raw_difference(rheader, cheader)}
        report["overlay"] = raw_difference(roverlay, coverlay)
        if rheader != cheader:
            bad.append("raw PE/DOS headers differ (includes historical timestamp)")
        if roverlay != coverlay:
            bad.append("file overlay differs")
        file_intervals = []
        for name in sorted({s["name"] for s in retail.sections + candidate.sections}):
            rs = [s for s in retail.sections if s["name"] == name]
            cs = [s for s in candidate.sections if s["name"] == name]
            section = {"name": name, "retail": rs, "candidate": cs}
            report["sections"].append(section)
            if len(rs) != 1 or len(cs) != 1:
                bad.append(f"section {name}: missing or ambiguous")
                continue
            rs, cs = rs[0], cs[0]
            # Include section characteristics omitted by Pe's convenience view.
            def characteristics(pe, sec):
                nt = struct.unpack_from("<I", pe.data, 0x3c)[0]
                opt = struct.unpack_from("<H", pe.data, nt + 20)[0]
                return struct.unpack_from("<I", pe.data, nt + 24 + opt + pe.sections.index(sec) * 40 + 36)[0]
            section["retail_characteristics"] = characteristics(retail, rs)
            section["candidate_characteristics"] = characteristics(candidate, cs)
            section["metadata_equal"] = rs == cs and section["retail_characteristics"] == section["candidate_characteristics"]
            rbytes = retail.data[rs["rptr"]:rs["rptr"] + rs["rsize"]]
            cbytes = candidate.data[cs["rptr"]:cs["rptr"] + cs["rsize"]]
            intervals = []
            if rs["va"] == cs["va"]:
                for lo, hi in ranges:
                    if rs["va"] <= lo and hi <= rs["va"] + min(rs["rsize"], cs["rsize"]):
                        intervals.append((lo - rs["va"], hi - rs["va"]))
                        if rs["rptr"] == cs["rptr"]:
                            file_intervals.append((rs["rptr"] + lo - rs["va"],
                                                   rs["rptr"] + hi - rs["va"]))
            section["excluded_intervals"] = intervals
            section["raw"] = raw_difference(rbytes, cbytes, intervals)
            if not section["metadata_equal"] or not section["raw"]["equal"]:
                bad.append(f"section {name}: metadata or unexcluded raw bytes differ")
        # Covers header/section gaps as well as overlays and raw padding. No
        # timestamp or DOS-stub exception is implied by the body exclusions.
        report["file"] = raw_difference(retail.data, candidate.data, file_intervals)
        if not report["file"]["equal"]:
            bad.append("complete file differs outside applied function-body exclusions")
        for row in report["functions"]:
            bad.extend(f"{row['name']}: {error}" for error in row["findings"])
    except (OSError, ValueError, KeyError, struct.error) as error:
        bad.append(f"strict audit input failure: {error}")
    report["summary"] = {"functions": len(report["functions"]),
                         "excluded_bodies": len(report["exclusions"]),
                         "applied_excluded_bodies": sum(row.get("applied", False) for row in report["exclusions"]),
                         "included_MAX_exact_CUR_dips": sum(row.get("ledger", {}).get("best") == 100.0 and row["ledger"]["cur"] < 100.0 for row in report["functions"]),
                         "findings": len(bad), "equal": not bad}
    return report


def parse_map() -> dict[str, int]:
    """{symbol: candidate rva} from the .map's Publics-by-Value table."""
    out: dict[str, int] = {}
    if not CMAP.is_file():
        return out
    base = None
    in_publics = False
    for ln in CMAP.read_text(errors="replace").splitlines():
        if "Publics by Value" in ln:
            in_publics = True
            continue
        if not in_publics:
            continue
        m = _MAP_ROW.match(ln)
        if not m:
            continue
        va = int(m.group(4), 16)
        if base is None:
            base = va - (int(m.group(2), 16) + 0x1000)  # 0001 section at 0x1000
        out.setdefault(m.group(3), va - 0x400000)
    return out


def _candidate_pe():
    from giten.core.pe import Pe
    return Pe(CAND)


def link_defect_findings() -> list[str]:
    out = []
    if not CAND.is_file():
        return [f"link-tier: no candidate EXE ({CAND}) - run `giten link` "
                f"first (the link tier is opt-in)"]
    if UNRESOLVED.is_file():
        text = UNRESOLVED.read_text(errors="replace").strip()
        if text:
            for ln in text.splitlines()[:20]:
                out.append(f"link-defects: unresolved external: {ln}")
    # pre-link closure over the base objs
    from giten.verify.undefined_closure import (_sym_sets, lib_symbols,
                                                 live_base_objs)
    bdef, bund = _sym_sets(live_base_objs())
    libs = lib_symbols()
    for s in sorted(bund - bdef):
        if s in libs or s.lstrip("_") in libs:
            continue
        if s.startswith("__imp_"):
            base = s[len("__imp_"):]
            if base in libs or base.lstrip("_") in libs:
                continue
        out.append(f"link-defects: {s} resolves from no base obj and from no "
                   f"archive under $MSVC_DIR/$DXSDK_DIR - either a declared-"
                   f"only symbol (fix the declaration/definition) or a "
                   f"library this closure does not scan")
    return out


def census() -> list[tuple[str, int, int]]:
    """[(section, retail_rsize, candidate_rsize)] (0 for a missing side)."""
    from giten.core.pe import image
    retail = image()
    cand = _candidate_pe()
    names = [s["name"] for s in retail.sections]
    names += [s["name"] for s in cand.sections if s["name"] not in names]
    rows = []
    for n in names:
        r = next((s["rsize"] for s in retail.sections if s["name"] == n), 0)
        c = next((s["rsize"] for s in cand.sections if s["name"] == n), 0)
        rows.append((n, r, c))
    return rows


def census_findings() -> list[str]:
    if not CAND.is_file():
        return []
    return [f"link-sections: retail section {n} ({r:#x} B) is ABSENT from "
            f"the candidate" for n, r, c in census() if r and not c
            and n != ".reloc"]


def _reloc_sites(pe) -> set[int]:
    sites = set()
    try:
        sec = pe.section(".reloc")
    except KeyError:
        return sites
    blob = pe.data[sec["rptr"]:sec["rptr"] + sec["rsize"]]
    p, end = 0, min(sec["rsize"], sec["vsize"])
    while p + 8 <= end:
        page, blk = struct.unpack_from("<II", blob, p)
        if blk < 8:
            break
        for i in range(8, min(blk, end - p), 2):
            ent = struct.unpack_from("<H", blob, p + i)[0]
            if ent >> 12 == 3:
                sites.add(page + (ent & 0xFFF))
        p += blk
    return sites


def _rel32_offsets(blob: bytes, vma: int) -> list[int]:
    """Window offsets of decoded E8/E9 (call/jmp rel32) instructions."""
    from giten.tool import objdump
    out = []
    for line in objdump.disassemble(blob, vma=vma).splitlines():
        if ":\t" not in line:
            continue
        addr, rest = line.split(":\t", 1)
        parts = rest.split("\t")
        if len(parts) < 2:
            continue
        raw = parts[0].strip().split()
        if raw and raw[0] in ("e8", "e9"):
            try:
                out.append(int(addr.strip(), 16) - vma)
            except ValueError:
                continue
    return out


def _byte_exact_symbols(pct: dict[tuple[str, str], float]) -> set[str]:
    """Symbols whose object score is literally 100%, not merely displayed
    as exact by the 99.995% navigation threshold.

    The linked-image audit isolates link-assigned placement and referents. A
    below-100 object body already contains an object-local byte difference, so
    admitting it here misreports that known compiler residue as a link defect.
    """
    return {sym for (_unit, sym), score in pct.items() if score == 100.0}


def image_diff_findings(limit: int = 25) -> list[str]:
    """Exact-scored functions whose LINKED candidate bytes diverge from
    retail under reloc masking."""
    if not CAND.is_file():
        return []                       # link_defect_findings already said so
    if not CMAP.is_file():
        # Without the map there is no rva for any candidate body, so the
        # image diff cannot run at all - and main()'s success line would
        # otherwise claim "every exact body byte-identical in the linked
        # image" on the strength of a check that never executed.
        return [f"image-diff: no candidate map ({CMAP}) - the linked-image "
                f"diff could not run; re-run `giten link` (it writes the "
                f".map beside the EXE)"]
    # a candidate older than the newest base obj was linked from OTHER
    # bytes: a divergence would be stale-image noise, not a link fact
    newest = max((p.stat().st_mtime
                  for p in (BUILD / "objdiff/base").glob("*.obj")),
                 default=0.0)
    if newest > CAND.stat().st_mtime:
        return ["image-diff: candidate EXE is STALE (a base obj is newer) - "
                "re-run `giten link` before reading the linked-image diff"]
    from giten.core.pe import image
    from giten.model import resolve
    from giten.walls.inventory import report_scores
    retail = image()
    cand = _candidate_pe()
    cand_rvas = parse_map()
    rsites = _reloc_sites(retail)
    csites = _reloc_sites(cand)
    _p, pct = report_scores()
    exact = _byte_exact_symbols(pct)
    out = []
    checked = 0
    for b in resolve().functions:
        if not b.name or b.name not in exact:
            continue
        crva = cand_rvas.get(b.name)
        if crva is None:
            continue
        rb = retail.read(b.rva, b.size)
        cb = cand.read(crva, b.size)
        if rb is None or cb is None:
            continue
        checked += 1
        rm, cm = bytearray(rb), bytearray(cb)
        for s in range(b.rva, b.rva + b.size):
            if s in rsites:
                for k in range(4):
                    if s - b.rva + k < len(rm):
                        rm[s - b.rva + k] = 0
        for s in range(crva, crva + b.size):
            if s in csites:
                for k in range(4):
                    if s - crva + k < len(cm):
                        cm[s - crva + k] = 0
        # REL32 call/jmp displacements re-resolve at link. Masked at DECODED
        # instruction boundaries (a byte scan desyncs on immediates that
        # contain 0xE8 - `mov [esi+0x1c],0x3e8` swallowed the next call's
        # opcode); the retail decode's offsets apply to both sides - if the
        # bodies structurally diverge the mask misapplies and the diff
        # fires, which is the correct verdict.
        for off in _rel32_offsets(rb, b.rva):
            if off + 5 <= len(rm):
                rm[off + 1:off + 5] = b"\0\0\0\0"
                cm[off + 1:off + 5] = b"\0\0\0\0"
        if bytes(rm) != bytes(cm):
            first = next((i for i, (x, y) in enumerate(zip(rm, cm))
                          if x != y), 0)
            out.append(f"image-diff: {b.name[:60]} retail 0x{b.rva:06x} != "
                       f"candidate 0x{crva:06x} at +{first:#x} (100%-scored "
                       f"body diverges in the LINKED image)")
            if len(out) >= limit:
                out.append("image-diff: ... (limit reached)")
                break
    if not checked and exact:
        out.append("image-diff: 0 exact functions located in the candidate "
                   "map - map/report join broken (never vacuous)")
    return out


def resource_findings() -> list[str]:
    if not CAND.is_file():
        return []                       # link_defect_findings already said so
    from giten.core.pe import image
    from giten.rsrc.check import findings
    from giten.rsrc.tree import read
    try:
        return [f"rsrc: {f}" for f in findings(read(image()), read(CAND))]
    except (OSError, ValueError, KeyError) as error:
        return [f"rsrc: {error}"]


def gate_findings() -> list[str]:
    return (link_defect_findings() + census_findings() + image_diff_findings()
            + resource_findings())


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="giten verify link-tier",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--census", action="store_true",
                    help="print the retail-vs-candidate section table and stop")
    ap.add_argument("--strict", action="store_true",
                    help="audit raw identity, excluding only ledger MAX<100 bodies")
    ap.add_argument("--retail", type=Path, default=BUILD / "local/DDS.EXE",
                    help="original retail EXE for --strict (not augmented delink oracle)")
    ap.add_argument("--report", type=Path,
                    help="write full strict JSON audit, including every function and section")
    ap.add_argument("--expected-partials", type=int,
                    help="assert exclusion count for this strict audit")
    a = ap.parse_args(argv)
    if a.strict:
        if a.census:
            ap.error("--strict and --census are separate audits")
        report = strict_report(a.retail, a.expected_partials)
        if a.report:
            a.report.parent.mkdir(parents=True, exist_ok=True)
            a.report.write_text(json.dumps(report, indent=2) + "\n")
        else:
            print(json.dumps(report, indent=2))
        summary = report["summary"]
        print(f"link-tier strict: {'OK' if summary['equal'] else 'DIFFERENT'} - "
              f"{summary['functions']} functions, {summary['excluded_bodies']} partial "
              f"bodies, {summary['findings']} findings", file=sys.stderr)
        return 0 if summary["equal"] else 1
    if a.report or a.expected_partials is not None:
        ap.error("--report and --expected-partials require --strict")
    if a.census:
        print(f"{'section':<10} {'retail':>10} {'candidate':>10} {'delta':>9}")
        for n, r, c in census():
            print(f"{n:<10} {r:>10,} {c:>10,} {c - r:>+9,}")
        return 0
    bad = gate_findings()
    for b in bad:
        print("  " + b, file=sys.stderr)
    if bad:
        print(f"link-tier: FATAL - {len(bad)} finding(s)", file=sys.stderr)
        return 1
    print("link-tier: OK - link closes, sections present, every exact body "
          "byte-identical in the linked image (reloc-masked), .rsrc identical "
          "to retail's (section-relative)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
