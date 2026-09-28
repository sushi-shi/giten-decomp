# Runtime prerequisites and validation

Linking the candidate does not establish that the game runs correctly.
Runtime comparison needs a correctly configured original game as its
reference. Both executables can exhibit the same environment failure.

## Inputs and configuration

- Supply the original executable through `GITEN_RETAIL_EXE` before entering
  `nix develop`. `giten link` extracts its executable resources locally.
- Supply a complete installed game directory separately. The executable's
  `.rsrc` section does not contain the external `ET`, `FC`, `M`, `P`, `S`,
  and `W` directories. Launch with that directory as the working directory;
  startup reads relative paths, including `et/et0002.bin`.
- Complete the original installation and configuration process in the same
  Windows environment or Wine prefix used to run the game. The original
  `CONFIG.EXE` is an installation component to inspect when device setup is
  missing; its successful configuration under Wine is not yet established.
- The `DevConfig` value under `HKCU\Software\ASCII\GITEN_DDS` must represent
  valid device settings. `LoadDeviceSettings` accepts a successful registry
  query without checking the returned record length. A one-byte placeholder
  can bypass startup while leaving the rest of `DeviceSettings` unset; it
  is not a valid device configuration.

Game files, executable resources, registry exports, screenshots, and runtime
logs remain local. This repository does not distribute these inputs or
provide download locations for them.

## Display and text troubleshooting

If both executables fail to open a window, inspect the working directory,
external files, and device settings before attributing the failure to the
reconstruction. An absent `DevConfig` makes `WinMain` return before window
creation. An incomplete installation can instead fail while reading assets.

If both executables display blue or black panels or garbled text, their
visual agreement is not evidence of correct rendering. The panel failure
has no established cause. Device configuration and graphics compatibility
remain open areas to investigate.

The text path uses ANSI font APIs and depends on the runtime's code page and
selected font. Under Wine, verify the actual Windows ANSI code page; setting
a Japanese locale variable is insufficient if the host lacks that locale.
Japanese code-page selection alone is not a verified fix.

`RenderGlyph` calls `GetGlyphOutline` without checking its return value, then
uses `metrics.gmptGlyphOrigin.y` to index a 24-element row-offset table. The
Japanese-locale Wine failure reaches this lookup in the original executable.
Whether the cause is a failed glyph query, unexpected metrics, or another
condition still needs to be established. The font is selected through
`CreateFontA` with `DEFAULT_CHARSET` and no explicit face name. A font
substitution workaround has not been validated.

Relevant source: [device settings](../src/Platform/devicesettings.cpp),
[record layout](../include/Platform/DeviceSettings.h),
[startup](../src/Platform/winmain.cpp), and
[font selection and glyph rendering](../src/Text/font.cpp).

## Windows comparison

1. Install and configure the original game in the Windows VM, using the
   appropriate Japanese language environment and the original setup tools.
2. Establish that the original renders panels and Japanese text correctly
   and progresses beyond startup. Record OS, locale, graphics settings,
   and the setup steps needed to reach this state.
3. Run the candidate with the same installed files, working directory,
   settings, and input sequence. Preserve a copy of the original settings
   and saves before comparison.
4. Compare behavior and capture each remaining failure separately. For a
   candidate-only failure, trace the relevant code, data, and relocations;
   investigate failures shared by the original as environment or original
   compatibility issues until evidence distinguishes them.

Each demonstrated code defect should receive its own fix PR. A compatibility
change to original behavior must be identified as such and kept separate
from byte-matching reconstruction work.
