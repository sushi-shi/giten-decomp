# Giten Megami Tensei: Tokyo Mokushiroku — port

The home of the bug fixes and modernisation of the 1999 Windows release of
偽典・女神転生 東京黙示録 (ASCII / Yū-Kikaku), `DDS.EXE`. It started from the
`source` branch with every bug fix enabled and is maintained by hand; it is
never regenerated.

```text
             main
               |
     +---------+---------+
     |                   |
     v                   v
  source              classic
     |
     v
   port (you are here)
```

| Branch | Purpose |
| --- | --- |
| `main` | Reconstruction and matching |
| `source` | Readable source; retail build, or with the bug fixes |
| `classic` | Retail source without the bug fixes |
| `port` | Bug fixes and modernisation, maintained by hand |

## Build

On x86-64 Linux with Nix flakes enabled, supply:

- Microsoft Visual C++ 5.0 SP3 and the DirectX 6 SDK, laid out as
  `msvc/{bin,include,lib}` and `dx/{Include,Lib}` in one directory;
- your original `DDS.EXE`, whose resources (bitmaps, sounds, icons) are
  copied into the build. No game content is part of this repository.

```sh
nix develop
python3 build.py --toolchain /path/to/toolchain --exe /path/to/DDS.EXE
```

The game is written to `build/DDS.EXE`.

## Following main

Changes to the reconstruction arrive through `source`. After `source` is
regenerated, apply the difference between its previous and its new commit:

```sh
git diff <previous source commit> <new source commit> | git apply -3
```

A change inside a `GITEN_COMPAT` or `GITEN_BUGFIX` block of `source` applies
here to the fix, whose conditional is gone; resolve such hunks by hand.
