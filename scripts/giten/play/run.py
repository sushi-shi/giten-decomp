"""giten play - build the bug-fixed game and start it under Wine.

    giten play [--disc DDSWIN.BIN] [--output WxH | --window] [--no-launch]

1. `ninja play`: the units that test GITEN_COMPAT or GITEN_BUGFIX recompiled
   with GITEN_BUGFIX, linked with the other units' matching objects and the
   retail resources into build/play/DDS.EXE.
2. The disc's DDSWIN/ game files are extracted into build/play/DDSWIN (the
   disc is not needed afterwards; --disc naming another image re-extracts).
   The built DDS.EXE is copied beside them.
3. build/play/prefix is created when missing and configured: the DevConfig
   device settings CONFIG.EXE would write, the X11 driver, and "Giten Gothic"
   standing in for MS Gothic. A changed configuration or font build re-applies.
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
from giten.play import font
from giten.tool import ToolError
from giten.tool.wine import boot_prefix, require, winepath

PLAY = REPO / graph.PLAY_DIR
GAME_DIR = PLAY / "DDSWIN"
PREFIX = PLAY / "prefix"
FONT = PLAY / "gitengothic.otf"
FONT_STAMP = PLAY / "gitengothic.stamp"
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


def extract(disc: Path | None) -> None:
    """Copy the disc's DDSWIN/ data files into GAME_DIR, once per disc image.

    With no --disc, files already extracted are kept whatever their source."""
    stamp = GAME_DIR / ".extracted"
    source = stamp.read_text().strip() if stamp.exists() else None
    if disc is None:
        if source is not None:
            return
        disc = Path(os.environ.get("GITEN_DISC", DEFAULT_DISC))
    elif source == str(disc.resolve()):
        return
    if not disc.is_file():
        raise SystemExit(f"[play] no disc at {disc}; "
                         "pass --disc <DDSWIN.BIN> (the raw MODE1/2352 image)")
    from giten.codecs.corpus import disc_files
    if GAME_DIR.exists():
        shutil.rmtree(GAME_DIR)
    GAME_DIR.mkdir(parents=True)
    count = 0
    try:
        for path, data in disc_files(disc):
            rel = path[len("DDSWIN/"):]
            if rel.upper() in ("DDS.EXE", "CONFIG.EXE"):
                continue
            dest = GAME_DIR / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
            count += 1
    except ValueError as e:
        raise SystemExit(f"[play] {disc}: {e}") from e
    stamp.write_text(f"{disc.resolve()}\n")
    print(f"[play] extracted {count} game file(s) from {disc} -> {GAME_DIR}")


def build_font() -> str:
    """Build FONT unless its stamp (font.VERSION and the source font) matches;
    returns the stamp."""
    source = Path(need_env("GITEN_PLAY_CJK_FONT"))
    stamp = f"{font.VERSION} {source}"
    if FONT.exists() and FONT_STAMP.exists() and FONT_STAMP.read_text() == stamp:
        return stamp
    print(f"[play] building {FONT.name} from {source.name}")
    em, glyphs, rows = font.build(source, FONT)
    FONT_STAMP.write_text(stamp)
    print(f"[play] built {FONT.name}: em {em} px, {glyphs} JIS glyph(s) "
          f"clipped by {rows} row(s) in RenderGlyph's window")
    return stamp


def run_checked(argv: list[str], env: dict[str, str], what: str, **kwargs) -> None:
    """Run a Wine tool; a failure is a ToolError naming `what`."""
    try:
        subprocess.run(argv, env=env, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        raise ToolError(f"{what} failed (rc={e.returncode}) in {PREFIX}") from e


def prepare_prefix() -> None:
    """Create PREFIX if missing and (re)apply the game's configuration and
    font whenever PREFIX_VERSION or the font stamp changes."""
    stamp = PREFIX / ".giten-play"
    env = wine_env()
    if not (PREFIX / "drive_c").is_dir():
        print(f"[play] creating Wine prefix {PREFIX}", flush=True)
    boot_prefix(env=env)
    config = f"{PREFIX_VERSION}\n{build_font()}\n"
    fonts = PREFIX / "drive_c/windows/Fonts"
    if stamp.exists() and stamp.read_text() == config and (fonts / FONT.name).exists():
        return
    fonts.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(FONT, fonts / FONT.name)
    reg = PLAY / "giten-play.reg"
    reg.write_bytes(b"\xff\xfe" + registry().encode("utf-16le"))
    run_checked([require("wine"), "reg", "import", winepath(reg, env)], env,
                "wine reg import", stdout=subprocess.DEVNULL)
    run_checked([require("wineserver"), "-w"], env, "wineserver -w")
    stamp.write_text(config)
    print("[play] prefix configured (DevConfig, X11 driver, Giten Gothic)")


def size_arg(spec: str) -> tuple[int, int]:
    """argparse type for WxH."""
    width, sep, height = spec.lower().partition("x")
    if not (sep and width.isdigit() and height.isdigit() and int(width) and int(height)):
        raise argparse.ArgumentTypeError(f"expected WxH, e.g. 2560x1440, not {spec!r}")
    return int(width), int(height)


def output_size() -> tuple[int, int] | None:
    """The physical size of niri's focused output, the default to scale to:
    its current mode, rotated as the output's transform rotates it."""
    if shutil.which("niri"):
        try:
            out = json.loads(subprocess.check_output(
                ["niri", "msg", "--json", "focused-output"], text=True,
                stderr=subprocess.DEVNULL))
            mode = out["modes"][out["current_mode"]]
            width, height = int(mode["width"]), int(mode["height"])
            if out["logical"]["transform"].endswith(("90", "270")):
                width, height = height, width
            return width, height
        except (subprocess.CalledProcessError, ValueError, KeyError, TypeError,
                IndexError, AttributeError):
            pass
    return None


def launch(size: tuple[int, int] | None, window: bool) -> int:
    env = wine_env()
    command = [require("wine"), "DDS.EXE"]
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
                    help="raw DDSWIN.BIN, extracted on first run or when it names "
                         "another image (default $GITEN_DISC or build/local/DDSWIN.BIN)")
    screen = ap.add_mutually_exclusive_group()
    screen.add_argument("--output", metavar="WxH", type=size_arg,
                        help="output size to scale to (default: the physical size "
                             "of niri's focused output)")
    screen.add_argument("--window", action="store_true",
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
    try:
        prepare_prefix()
        if args.no_launch:
            print(f"[play] ready: {target}")
            return 0
        return launch(args.output or output_size(), args.window)
    except ToolError as e:
        raise SystemExit(f"[play] {e}") from e


if __name__ == "__main__":
    sys.exit(main())
