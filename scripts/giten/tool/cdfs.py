#!/usr/bin/env python3
"""giten.tool.cdfs - read an ISO9660 filesystem from a raw (MODE1/2352) or cooked (2048) CD image.

The image may be a local path or an HTTP(S) URL; URLs are read with range
requests, so only directory sectors and the requested files are transferred.

usage:
  cdfs.py SOURCE {2352|2048} list                 # TSV: lba, size, path
  cdfs.py SOURCE {2352|2048} get OUTDIR PATH...   # extract files, keeping disc paths
  cdfs.py SOURCE {2352|2048} pvd                  # volume id and declared block count
"""
import os
import struct
import sys
import urllib.request


class Image:
    def __init__(self, source, sector):
        self.source, self.sector = source, sector
        self.hdr = 16 if sector == 2352 else 0
        self.remote = source.startswith(("http://", "https://"))
        self.fh = None if self.remote else open(source, "rb")

    def read(self, lba, count):
        start, length = lba * self.sector, count * self.sector
        if self.remote:
            req = urllib.request.Request(self.source, headers={"Range": f"bytes={start}-{start + length - 1}"})
            raw = urllib.request.urlopen(req).read()
        else:
            self.fh.seek(start)
            raw = self.fh.read(length)
        s, h = self.sector, self.hdr
        return b"".join(raw[i * s + h:i * s + h + 2048] for i in range(count))


def records(img, lba, size):
    data = img.read(lba, (size + 2047) // 2048)
    for blk in range(0, len(data), 2048):
        off = blk
        while off < blk + 2048 and data[off]:
            rec = data[off:off + data[off]]
            off += len(rec)
            name = rec[33:33 + rec[32]]
            if name in (b"\x00", b"\x01"):
                continue
            ext = struct.unpack_from("<I", rec, 2)[0]
            sz = struct.unpack_from("<I", rec, 10)[0]
            yield name.decode("latin1").split(";")[0], ext, sz, bool(rec[25] & 2)


def walk(img, lba, size, prefix=""):
    for name, ext, sz, isdir in records(img, lba, size):
        path = prefix + name
        if isdir:
            yield path + "/", ext, sz, True
            yield from walk(img, ext, sz, path + "/")
        else:
            yield path, ext, sz, False


def main():
    img = Image(sys.argv[1], int(sys.argv[2]))
    pvd = img.read(16, 1)
    if pvd[1:6] != b"CD001":
        sys.exit("no ISO9660 primary volume descriptor at LBA 16")
    root = pvd[156:156 + 34]
    rlba, rsz = struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0]
    cmd = sys.argv[3]

    if cmd == "pvd":
        print(f"volume_id\t{pvd[40:72].decode('latin1').strip()}")
        print(f"volume_space_blocks\t{struct.unpack_from('<I', pvd, 80)[0]}")
    elif cmd == "list":
        print("lba\tsize\tpath")
        for path, ext, sz, _ in walk(img, rlba, rsz):
            print(f"{ext}\t{sz}\t{path}")
    elif cmd == "get":
        outdir, wanted = sys.argv[4], {p.upper(): p for p in sys.argv[5:]}
        for path, ext, sz, isdir in walk(img, rlba, rsz):
            if isdir or path.upper() not in wanted:
                continue
            dest = os.path.join(outdir, path)
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            with open(dest, "wb") as f:
                f.write(img.read(ext, (sz + 2047) // 2048)[:sz])
            print(f"{sz}\t{path}", file=sys.stderr)
            del wanted[path.upper()]
        if wanted:
            sys.exit(f"not found: {', '.join(wanted.values())}")
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
