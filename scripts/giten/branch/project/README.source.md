# Giten Megami Tensei: Tokyo Mokushiroku — source

Source for the 1999 Windows release of 偽典・女神転生 東京黙示録
(ASCII / Yū-Kikaku), `DDS.EXE`: the game logic in C and the Windows layer
in C++, built with Microsoft Visual C++ 5.0.

```text
             main
               |
     +---------+---------+
     |                   |
     v                   v
  source              classic
(you are here)
     |
     v
   port
```

| Branch | Purpose |
| --- | --- |
| `main` | Reconstruction and matching |
| `source` | Readable source; retail build, or with the bug fixes |
| `classic` | Retail source without the bug fixes |
| `port` | Bug fixes and modernisation, maintained by hand |

The bug fixes are kept beside the retail code under two flags. `GITEN_COMPAT`
covers crashes, undefined behaviour, frame pacing and differences between
Windows, drivers and Wine. `GITEN_BUGFIX` covers defects in the game logic,
such as dropped input and soft-locks, and implies `GITEN_COMPAT`. A plain build
defines neither and is the retail game.

## Build

On x86-64 Linux with Nix flakes enabled, supply:

- Microsoft Visual C++ 5.0 SP3 and the DirectX 6 SDK, laid out as
  `msvc/{bin,include,lib}` and `dx/{Include,Lib}` in one directory;
- your original `DDS.EXE`, whose resources (bitmaps, sounds, icons) are
  copied into the build. No game content is part of this repository.

```sh
nix develop
python3 build.py --toolchain /path/to/toolchain --exe /path/to/DDS.EXE
python3 build.py --toolchain /path/to/toolchain --exe /path/to/DDS.EXE --fixes
```

The first command writes the retail game to `build/DDS.EXE`, the second the
game with the bug fixes to `build/fixes/DDS.EXE`. `GITEN_TOOLCHAIN` and
`GITEN_RETAIL_EXE` can stand in for the two options.

## Run

Put the built `DDS.EXE` in an installed game directory, beside its data
files. The game reads its device settings from the `DevConfig` value under
`HKCU\Software\ASCII\GITEN_DDS`, which the original `CONFIG.EXE` writes;
without it the game exits before opening a window. Under Wine, run it with a
Japanese locale (for example `LANG=ja_JP.UTF-8`) so that its Shift-JIS text is
read as such.

## Regeneration

This branch is generated from `main`, where changes are made. Every
regeneration replaces the branch with a single commit that names its `main`
commit.
