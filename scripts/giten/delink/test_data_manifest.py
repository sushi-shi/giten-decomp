"""C string literals and band gaps in ordinary COFF data sections."""

from __future__ import annotations

import unittest
from unittest import mock

from giten.delink import data_manifest


class _Candidate:
    section_table = [{"name": ".data"}]

    def __init__(self, names, offsets=None):
        self.names = names
        self.offsets = list(range(len(names))) if offsets is None else offsets

    def iter_symbols(self):
        return ((i, off, 1) for i, off in enumerate(self.offsets))

    def sym_name(self, i):
        return self.names[i][0]

    def cstring(self, _section, i):
        return self.names[self.offsets.index(i)][1]


class _Retail:
    def __init__(self, payloads):
        self.payloads = payloads

    def cstring(self, rva):
        return self.payloads.get(rva)


class CStringOracleTest(unittest.TestCase):
    def test_unique_payload_is_enrolled_and_duplicate_payload_is_withheld(self):
        objects = [
            ("one", _Candidate([("$SG1", b"unique"), ("$SG2", b"same")])),
            ("two", _Candidate([("$SG3", b"same")])),
        ]
        payloads = {0x1000: b"unique", 0x1010: b"same", 0x1020: b"same"}
        with mock.patch.object(data_manifest.coffx, "objects", return_value=objects), \
             mock.patch.object(data_manifest, "retail", return_value=_Retail(payloads)), \
             mock.patch.object(data_manifest, "_reloc_data_rvas",
                               return_value=payloads), \
             mock.patch.object(data_manifest, "_literal_site_rvas",
                               return_value={}), \
             mock.patch.object(data_manifest, "_classify",
                               return_value="data-initialized"):
            rows, withheld = data_manifest.c_string_rows()
        self.assertEqual([(r["name"], r["rva"], r["object"])
                          for r in rows], [("$SG1", 0x1000, "one.c")])
        self.assertEqual({rva for rva, _name, _why in withheld},
                         {0x1010, 0x1020})

    def test_exact_code_sites_disambiguate_repeated_text(self):
        objects = [("one", _Candidate([("$SG1", b"same")])),
                   ("two", _Candidate([("$SG2", b"same")]))]
        payloads = {0x1010: b"same", 0x1020: b"same"}
        anchors = {("one", "$SG1"): {0x1020},
                   ("two", "$SG2"): {0x1010}}
        with mock.patch.object(data_manifest.coffx, "objects", return_value=objects), \
             mock.patch.object(data_manifest, "retail", return_value=_Retail(payloads)), \
             mock.patch.object(data_manifest, "_reloc_data_rvas",
                               return_value=payloads), \
             mock.patch.object(data_manifest, "_literal_site_rvas",
                               return_value=anchors), \
             mock.patch.object(data_manifest, "_classify",
                               return_value="data-initialized"):
            rows, withheld = data_manifest.c_string_rows()
        self.assertEqual({r["name"]: r["rva"] for r in rows},
                         {"$SG1": 0x1020, "$SG2": 0x1010})
        self.assertEqual(withheld, [])

    def test_equal_spaced_copies_in_one_section_follow_retail_order(self):
        objects = [("one", _Candidate([("$SG0", b"before"),
                                       ("$SG1", b"same"),
                                       ("$SG2", b"same"),
                                       ("$SG3", b"after")],
                                      [0x20, 0x38, 0x50, 0x68]))]
        payloads = {0x1000: b"before", 0x1018: b"same",
                    0x1030: b"same", 0x1048: b"after"}
        anchors = {("one", "$SG1"): {0x1018}}
        with mock.patch.object(data_manifest.coffx, "objects", return_value=objects), \
             mock.patch.object(data_manifest, "retail", return_value=_Retail(payloads)), \
             mock.patch.object(data_manifest, "_reloc_data_rvas",
                               return_value=payloads), \
             mock.patch.object(data_manifest, "_literal_site_rvas",
                               return_value=anchors), \
             mock.patch.object(data_manifest, "_classify",
                               return_value="data-initialized"):
            rows, withheld = data_manifest.c_string_rows()
        self.assertEqual({r["name"]: r["rva"] for r in rows},
                         {"$SG0": 0x1000, "$SG1": 0x1018,
                          "$SG2": 0x1030, "$SG3": 0x1048})
        self.assertEqual(withheld, [])

    def test_equal_spacing_without_bracketing_literals_is_withheld(self):
        objects = [("one", _Candidate([("$SG1", b"same"),
                                       ("$SG2", b"same")], [0x38, 0x50]))]
        payloads = {0x1018: b"same", 0x1030: b"same"}
        with mock.patch.object(data_manifest.coffx, "objects", return_value=objects), \
             mock.patch.object(data_manifest, "retail", return_value=_Retail(payloads)), \
             mock.patch.object(data_manifest, "_reloc_data_rvas",
                               return_value=payloads), \
             mock.patch.object(data_manifest, "_literal_site_rvas",
                               return_value={}), \
             mock.patch.object(data_manifest, "_classify",
                               return_value="data-initialized"):
            rows, withheld = data_manifest.c_string_rows()
        self.assertEqual(rows, [])
        self.assertEqual({rva for rva, _name, _why in withheld},
                         {0x1018, 0x1030})

    def test_tu_local_ordinals_get_distinct_manifest_names(self):
        objects = [("one", _Candidate([("$SG17", b"first")])),
                   ("two", _Candidate([("$SG17", b"second")]))]
        payloads = {0x1010: b"first", 0x1020: b"second"}
        with mock.patch.object(data_manifest.coffx, "objects", return_value=objects), \
             mock.patch.object(data_manifest, "retail", return_value=_Retail(payloads)), \
             mock.patch.object(data_manifest, "_reloc_data_rvas",
                               return_value=payloads), \
             mock.patch.object(data_manifest, "_literal_site_rvas",
                               return_value={}), \
             mock.patch.object(data_manifest, "_classify",
                               return_value="data-initialized"):
            rows, withheld = data_manifest.c_string_rows()
        self.assertEqual({r["name"]: (r["member"], r["object"])
                          for r in rows},
                         {"$SG4112": ("$SG17", "one.c"),
                          "$SG4128": ("$SG17", "two.c")})
        self.assertEqual(withheld, [])

    def test_last_repeated_copy_follows_proven_exact_site(self):
        objects = [("one", _Candidate([("$SG1", b"same")])),
                   ("two", _Candidate([("$SG2", b"same")]))]
        payloads = {0x1010: b"same", 0x1020: b"same"}
        anchors = {("one", "$SG1"): {0x1020}}
        with mock.patch.object(data_manifest.coffx, "objects", return_value=objects), \
             mock.patch.object(data_manifest, "retail", return_value=_Retail(payloads)), \
             mock.patch.object(data_manifest, "_reloc_data_rvas",
                               return_value=payloads), \
             mock.patch.object(data_manifest, "_literal_site_rvas",
                               return_value=anchors), \
             mock.patch.object(data_manifest, "_classify",
                               return_value="data-initialized"):
            rows, withheld = data_manifest.c_string_rows()
        self.assertEqual({r["object"]: r["rva"] for r in rows},
                         {"one.c": 0x1020, "two.c": 0x1010})
        self.assertEqual(withheld, [])


class _GapRetail:
    """A retail image whose bytes are all zero and carry no relocations."""

    def payload(self, _rva, n):
        return bytes(n)

    def relocs_in(self, _lo, _hi):
        return []


class BandGapTest(unittest.TestCase):
    SECTION = {"index": 3, "alignment": 8}

    def _gaps(self, rows):
        with mock.patch.object(data_manifest, "retail", return_value=_GapRetail()), \
             mock.patch.object(data_manifest, "_classify",
                               return_value="data-initialized"), \
             mock.patch.object(data_manifest, "declared_types", return_value={}):
            return data_manifest.gap_rows(rows, [])

    def _row(self, name, rva, size, offset=None):
        row = {"name": name, "object": "page.c", "rva": rva, "size": size,
               "storage": "data"}
        if offset is not None:
            row["section"] = self.SECTION
            row["section_offset"] = offset
        return row

    def test_padding_the_candidate_section_reproduces_is_not_a_datum(self):
        # A 12-byte record, then an 8-byte one on the section's latched
        # eight-byte boundary: cl's own section holds the same 4-byte hole.
        rows, withheld = self._gaps([self._row("_item", 0x6a238, 0xc, 0x140),
                                     self._row("_skill", 0x6a248, 0x8, 0x150)])
        self.assertEqual(rows, [])
        self.assertEqual([(rva, why.split("(")[0].strip())
                          for rva, _name, why in withheld],
                         [(0x6a244, "band gap reproduced by the candidate "
                                    "section's own layout")])

    def test_zero_hole_the_candidate_section_lacks_is_carved(self):
        rows, withheld = self._gaps([self._row("_item", 0x6a238, 0xc, 0x140),
                                     self._row("_skill", 0x6a248, 0x8, 0x14c)])
        self.assertEqual([(r["rva"], r["size"], r["provenance"]) for r in rows],
                         [(0x6a244, 4, "provisional-band-gap-zero")])
        self.assertEqual(withheld, [])


if __name__ == "__main__":
    unittest.main()
