"""giten play - build the bug-fixed game and start it under Wine.

    giten play [--disc DDSWIN.BIN] [--output WxH | --window] [--no-launch]

1. `ninja play`: every unit recompiled with GITEN_BUGFIX, linked with the
   retail resources into build/play/DDS.EXE.
2. The disc's DDSWIN/ game files are extracted once into build/play/DDSWIN
   (the disc is not needed afterwards). The built DDS.EXE is copied beside them.
3. build/play/prefix is created when missing and configured: the DevConfig
   device settings CONFIG.EXE would write, the X11 driver, and "Giten Gothic"
   standing in for MS Gothic.
4. DDS.EXE runs from DDSWIN/ under a Japanese locale (Wine takes the ANSI code
   page, 932, from it) inside gamescope, which gives the game a real 640x480
   screen and scales it to the output.

Saves go to the prefix's C:\\windows\\saveNNNN.dds.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import struct
import subprocess
import sys
from pathlib import Path

from giten import graph
from giten.core.paths import REPO

PLAY = REPO / graph.PLAY_DIR
GAME_DIR = PLAY / "DDSWIN"
PREFIX = PLAY / "prefix"
FONT = PLAY / "gitengothic.otf"
DEFAULT_DISC = REPO / "build/local/DDSWIN.BIN"
#: Bump when the prefix configuration below changes; stale prefixes re-apply it.
PREFIX_VERSION = "1"
GAME_SIZE = (640, 480)


def device_settings() -> bytes:
    """The DevConfig record (DeviceSettings, /Zp1): the HAL device with
    bilinear filtering, dithering and real alpha blending on every kind.
    Wine offers no ramp or MMX software device, so hardwareOnly must be TRUE."""
    record = bytes(16)                                   # prefix, unread
    record += struct.pack("<I", 1)                       # hardwareOnly
    record += struct.pack("<3I", 16, 16, 16)             # zBufferDepth
    record += bytes([1, 1, 1, 1, 1, 1, 2, 2, 2, 0])      # bilinear, dither, blend, reserved
    record += struct.pack("<2I", 0, 0) + bytes(3 * 56)   # driverCaps, surfaceCaps, primCaps
    assert len(record) == 218
    return record


def registry() -> str:
    hexdata = ",".join(f"{b:02x}" for b in device_settings())
    return "\r\n".join([
        "Windows Registry Editor Version 5.00", "",
        r"[HKEY_CURRENT_USER\Software\ASCII\GITEN_DDS]",
        f'"DevConfig"=hex:{hexdata}', "",
        r"[HKEY_CURRENT_USER\Software\Wine\Drivers]",
        '"Graphics"="x11"', "",
        r"[HKEY_CURRENT_USER\Software\Wine\Fonts\Replacements]",
        '"MS Gothic"="Giten Gothic"',
        '"ＭＳ ゴシック"="Giten Gothic"', "", ""])


def need_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise SystemExit(f"[play] ${name} is unset - run inside `nix develop`")
    return value


def wine_env() -> dict[str, str]:
    archive = need_env("GITEN_PLAY_LOCALE_ARCHIVE")
    return {**os.environ, "WINEPREFIX": str(PREFIX),
            "WINEDEBUG": os.environ.get("GITEN_PLAY_WINEDEBUG", "-all"),
            "LANG": "ja_JP.UTF-8", "LC_ALL": "ja_JP.UTF-8",
            "LOCALE_ARCHIVE": archive, "LOCALE_ARCHIVE_2_27": archive}


def extract(disc: Path) -> None:
    """Copy the disc's DDSWIN/ data files into GAME_DIR, once."""
    stamp = GAME_DIR / ".extracted"
    if stamp.exists():
        return
    if not disc.is_file():
        raise SystemExit(f"[play] no game files in {GAME_DIR} and no disc at {disc}; "
                         "pass --disc <DDSWIN.BIN> (the raw MODE1/2352 image)")
    from giten.tool.cdfs import Image, walk
    image = Image(str(disc), 2352)
    pvd = image.read(16, 1)
    if pvd[1:6] != b"CD001":
        raise SystemExit(f"[play] {disc} is not a raw ISO9660 image")
    root = pvd[156:190]
    lba, size = struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0]
    count = 0
    for path, extent, length, is_dir in walk(image, lba, size):
        if is_dir or not path.upper().startswith("DDSWIN/"):
            continue
        rel = path[len("DDSWIN/"):]
        if rel.upper() in ("DDS.EXE", "CONFIG.EXE"):
            continue
        dest = GAME_DIR / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(image.read(extent, (length + 2047) // 2048)[:length])
        count += 1
    stamp.write_text(f"{disc}\n")
    print(f"[play] extracted {count} game file(s) -> {GAME_DIR}")


def prepare_prefix() -> None:
    """Create PREFIX if missing and (re)apply the game's configuration."""
    stamp = PREFIX / ".giten-play"
    env = wine_env()
    if not (PREFIX / "system.reg").exists():
        print(f"[play] creating Wine prefix {PREFIX}")
        PREFIX.mkdir(parents=True, exist_ok=True)
        subprocess.run(["wineboot", "-i"], env=env, check=True)
    if stamp.exists() and stamp.read_text() == PREFIX_VERSION and \
            (PREFIX / "drive_c/windows/Fonts" / FONT.name).exists():
        return
    if not FONT.exists():
        from giten.play.font import build
        em = build(Path(need_env("GITEN_PLAY_CJK_FONT")), FONT)
        print(f"[play] built {FONT.name} (em {em} px)")
    fonts = PREFIX / "drive_c/windows/Fonts"
    fonts.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(FONT, fonts / FONT.name)
    reg = PLAY / "giten-play.reg"
    reg.write_bytes(b"\xff\xfe" + registry().encode("utf-16le"))
    subprocess.run(["wine", "reg", "import", subprocess.check_output(
        ["winepath", "-w", str(reg)], env=env, text=True).strip()],
        env=env, check=True, stdout=subprocess.DEVNULL)
    subprocess.run(["wineserver", "-w"], env=env, check=True)
    stamp.write_text(PREFIX_VERSION)
    print("[play] prefix configured (DevConfig, X11 driver, Giten Gothic)")


def output_size(spec: str | None) -> tuple[int, int] | None:
    """The output to scale to: --output, else niri's focused output."""
    if spec:
        width, height = spec.lower().split("x")
        return int(width), int(height)
    if shutil.which("niri"):
        try:
            out = json.loads(subprocess.check_output(
                ["niri", "msg", "--json", "focused-output"], text=True,
                stderr=subprocess.DEVNULL))
            return int(out["logical"]["width"]), int(out["logical"]["height"])
        except (subprocess.CalledProcessError, ValueError, KeyError, TypeError):
            pass
    return None


def launch(size: tuple[int, int] | None, window: bool) -> int:
    env = wine_env()
    command = ["wine", "DDS.EXE"]
    if not window:
        scope = [need_env("GITEN_PLAY_GAMESCOPE"), "-w", str(GAME_SIZE[0]), "-h", str(GAME_SIZE[1]),
                 "-S", "integer", "-F", "nearest"]
        if size:
            scope += ["-W", str(size[0]), "-H", str(size[1]), "-f"]
        else:
            scope += ["-W", str(GAME_SIZE[0] * 2), "-H", str(GAME_SIZE[1] * 2)]
        command = [*scope, "--", *command]
    print(f"[play] {' '.join(command)}  (cwd {GAME_DIR})")
    return subprocess.run(command, cwd=GAME_DIR, env=env).returncode


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="giten play", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--disc", type=Path,
                    default=Path(os.environ.get("GITEN_DISC", DEFAULT_DISC)),
                    help="raw DDSWIN.BIN, read on first run (default $GITEN_DISC "
                         "or build/local/DDSWIN.BIN)")
    ap.add_argument("--output", metavar="WxH",
                    help="output size to scale to (default: niri's focused output)")
    ap.add_argument("--window", action="store_true",
                    help="run under plain Wine instead of gamescope")
    ap.add_argument("--no-launch", action="store_true",
                    help="build and prepare, but do not start the game")
    args = ap.parse_args(argv)

    from giten.graph.verbs import configure_if_needed, ninja
    configure_if_needed()
    rc = ninja(["play"])
    if rc:
        return rc
    extract(args.disc)
    exe = REPO / graph.PLAY_EXE
    target = GAME_DIR / "DDS.EXE"
    if not target.exists() or target.read_bytes() != exe.read_bytes():
        shutil.copyfile(exe, target)
    prepare_prefix()
    if args.no_launch:
        print(f"[play] ready: {target}")
        return 0
    return launch(output_size(args.output), args.window)


if __name__ == "__main__":
    sys.exit(main())
