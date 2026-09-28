# Playing

`giten play` builds a playable `DDS.EXE` from the reconstructed source and
starts it under Wine:

```sh
giten play --disc /path/to/DDSWIN.BIN   # first run; the disc is read once
giten play                              # later runs
```

It builds the `play` graph target and extracts the disc's `DDSWIN/` files
(not its executables) into `build/play/DDSWIN`. It then creates
`build/play/prefix` if it does not exist and starts the game from
`build/play/DDSWIN` inside gamescope. The raw MODE1/2352 disc image defaults
to `$GITEN_DISC` or `build/local/DDSWIN.BIN`. Options are in
`giten play --help`. Saves are written to the prefix's `C:\windows`.

## Build

The `play` target compiles every unit again with its own flag profile plus
`/DGITEN_BUGFIX` into `build/play/obj`. It links those objects with the
`.res` built from the local retail executable into `build/play/DDS.EXE`. The
matching objects in `build/objdiff/base` never see the define, so scores are
unaffected.

A fix for a defect in the original behavior goes under `#ifdef GITEN_BUGFIX`
beside the retail spelling, which stays in the `#else` branch. Its comment
states the retail defect and the condition that triggers it. Only fixes go
there, not reconstruction guesses or matching experiments.

## Runtime

The prefix is configured the way the original setup would configure Windows:

- `HKCU\Software\ASCII\GITEN_DDS\DevConfig` holds the 218-byte
  `DeviceSettings` record that `CONFIG.EXE` writes. Without it `WinMain`
  exits before creating a window. The record selects the HAL device because
  Wine has no ramp or MMX device.
- The game runs under `ja_JP.UTF-8` because Wine derives the ANSI code page
  (932) from the Unix locale. The text APIs are ANSI and the strings are
  Shift-JIS.
- The glyph font is created with no face name, which Japanese Windows
  resolves to MS Gothic. `Giten Gothic` replaces it. It is cut from Noto Sans
  Mono CJK JP with Windows metrics rescaled so that every JIS X 0208 glyph
  fits the game's 16-row cell.
- gamescope gives the game a real 640x480 screen and scales it by an integer
  factor. The game hides the system cursor, confines it to 640x480 and draws
  its own sprite at `GetCursorPos`, so a Wine-emulated mode change leaves the
  sprite and the pointer disagreeing.

The dev shell provides gamescope, the locale archive and the source font
through `GITEN_PLAY_*` variables.
