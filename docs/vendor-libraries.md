# Retail libraries and SDK helpers

DDS.EXE uses Microsoft's VC5 static C runtime and Windows/DirectX APIs.
There is no identified MFC, ATL, STL, zlib, or other bundled third-party
library contribution. Names such as `CMMIO` and `CMidiStream` in this tree
are reconstructed wrappers, not evidence of MFC.

| Provider | Evidence and scope |
| --- | --- |
| Visual C++ 5.0 SP3 CRT | `/ML`, `LIBC.LIB`, and the function identities in [`functions_static_libs.tsv`](../config/retail/functions_static_libs.tsv). The compiler also requests `OLDNAMES` for CRT compatibility names. |
| Win32 | Retail imports KERNEL32, USER32, GDI32, and ADVAPI32, in that order. |
| DirectX | Retail imports DDRAW, DSOUND, and DINPUT. Direct3D interfaces are obtained through DirectDraw COM calls. The reconstruction uses the pinned package's DirectX headers; its label is not proof of the retail SDK revision. |
| WinMM | Retail imports WINMM for multimedia, joystick, MIDI stream, and MMIO APIs. |
| DirectX static data | `DXGUID.LIB` supplies interface/device GUIDs; `DINPUT.LIB` supplies the keyboard/mouse formats. Exact library-object evidence is recorded in [`data_static_libs.tsv`](../config/retail/data_static_libs.tsv). |
| COM static data | `UUID.LIB` supplies `GUID_NULL` (`cguid_i_guid0.obj`), the default DirectDraw driver GUID, placed directly after `DXGUID.LIB`'s `.rdata`. |

The explicit link inputs are in
[`LINK_LIBS`](../scripts/giten/graph/link.py); see also
[candidate linking](linker-flags.md). The toolchain contains more libraries
than this executable uses. Inherited MFC labeling support and the empty
[`functions_zlib.tsv`](../config/retail/functions_zlib.tsv) channel do not
establish dependencies.

The Windows layer uses `IDirectDraw2`, `IDirectDrawSurface`,
`IDirect3D2`, `IDirect3DDevice2`, `IDirect3DViewport2`,
`IDirect3DMaterial2`, `IDirect3DTexture2`, DirectSound buffers, and
DirectInput keyboard/mouse devices. C callers already use the SDK's COM
dispatch macros. C++ callers use interface member calls. GDI supplies fonts
and glyph rasterization; USER32 supplies the window/message loop; ADVAPI32
supplies registry settings. Music uses WinMM `midiStream*`, with `mmio*` for
multimedia files. The dynamic import in `Platform/Ime.h` resolves
`WINNLSEnableIME` from USER32; it does not introduce another DLL dependency.

Compiler-generated exception handling, allocation, deleting destructors,
and Direct3D constructors are not MFC evidence. In particular, a tiny
deleting destructor that also matches a `LIBCP.LIB` member does not establish
use of iostreams or the C++ standard library. No MFC headers, message maps,
runtime-class machinery, containers, or MFC library bodies are identified
in the reconstructed game. The investigation attributes the separate
Windows `CONFIG.EXE` utility to Borland C++Builder/VCL; that is outside this
repository's `DDS.EXE` target.

## SDK revision and unused GUIDs

GUID storage and runtime calls are different evidence. Retail contains
DirectDraw4 and Direct3D3 GUIDs even though the recovered renderer uses the
older interfaces. It also contains DirectMusic GUIDs, including
`IID_IDirectMusicBuffer` at RVA `0x065d78` and
`IID_IDirectMusicObject` at `0x066248`, whose complete 16 bytes agree with
the pinned `dmusicc.h` / `dmusici.h` definitions. No DirectMusic interface
call is identified in the game.

The pinned definitions of `IID_IDirectMusic`
(`d2ac2876-b39b-11d1-8704-00600893b1bd`) and
`IID_IDirectMusicPerformance` (`de5e3a33-d31b-11d1-bc8b-00a0c922e6eb`)
are **absent** from the pinned retail image. Both occur in the pinned
`DXGUID.LIB`. Thus the investigation's assertion that those two retail GUIDs
prove an `INITGUID` TU or a particular SDK release is not supported by this
image/package pair.

Microsoft's [DirectX 6.1 announcement](https://news.microsoft.com/source/1999/02/03/microsoft-ships-directx-6-1/)
dates DirectMusic's first public release to February 1999 and also describes
an earlier developer release. The surviving GUID subset supports DirectMusic
SDK lineage, but does not distinguish a prerelease from a final SDK.
Exact SDK revision and provenance of unused GUID blocks remain
[matching work](todos/vendor-macros.md).

## Recovering expanded macros

Read the pinned era headers in `$MSVC_DIR/include` and `$DXSDK_DIR/Include`
inside `nix develop`. SDK versions matter: do not substitute a modern macro
definition or copy SDK declarations into a project header. Platform includes
enter through `Win32.h`; the Direct3D and WinMM wrappers add the corresponding
SDK facilities. The CRT's headers are included directly.

| Family | Era headers | Expansion to look for |
| --- | --- | --- |
| HRESULT | `WINERROR.H` | `FAILED` / `SUCCEEDED`: signed HRESULT tests against zero. |
| Min/max | `WINDEF.H`, `STDLIB.H` | Repeated-operand comparison/selection expressions. Preserve operand evaluation and promoted types. |
| Memory | `WINBASE.H`, `WINNT.H` | `ZeroMemory`, `CopyMemory`, `MoveMemory`, `FillMemory` map to CRT memory operations. SDK record initialization is a natural use; a generic CRT call alone does not prove its original spelling. |
| Handles/GDI | `WINDOWSX.H` | `GlobalAllocPtr`, `GlobalFreePtr`, `GetStockBrush`, `SelectFont`, `DeleteFont`; inspect the complete API call sequence. |
| Packed Windows values | `WINDEF.H`, `WINGDI.H`, `WINUSER.H` | `LOWORD`, `HIWORD`, `LOBYTE`, `HIBYTE`, `MAKEWORD`, `MAKELONG`, `RGB`, channel getters, `MAKEINTRESOURCE`. Width and signedness are part of the expansion. |
| Direct3D | `d3dtypes.h`, `d3dvec.inl` | `D3DVAL`, `D3DRGB`, `D3DRGBA`, `RGBA_MAKE`, `RGBA_SETALPHA`, `RGB_MAKE`, and the vector/matrix constructors and operators. `/Ob0` can leave the inline SDK bodies out of line. |
| COM in C | `ddraw.h`, `d3d.h`, `dinput.h`, `dsound.h` | `IDirect…_Method` macros, instead of hand-written vtable dispatch. C++ member calls already express the same interface. |
| Multimedia | `MMSYSTEM.H` | `mmioFOURCC`, `MEVT_EVENTPARM`, event flags and resource constants. |
| CRT | `STDIO.H`, `MBCTYPE.H` | `getc`, `putc`, stream predicates, `_ismbblead` and character-table predicates; do not access their backing tables directly. |

Identical output establishes compatibility with a macro, not proof of its
historical spelling. Check all supported sites, compare the affected units,
and preserve retail's behavior at the exceptions:

- `hr == DD_OK` is an exact-code test, whereas `SUCCEEDED(hr)` accepts every
  nonnegative HRESULT. WinMM result codes are not HRESULTs.
- `GlobalFreePtr(p)` expands to two `GlobalHandle` calls. A cached handle
  followed by `GlobalUnlock` and `GlobalFree` is a different call sequence.
- `memset(&desc, sizeof(desc), 0)` has zero length. Converting that retail
  behavior into a real `ZeroMemory` clear changes the program.
- Packed game/script words and bitmap pixel layouts are not automatically
  Windows message parameters or `COLORREF`s.
- `if (x < bound) x = bound` and `x = max(x, bound)` need not agree for
  floating-point NaNs. Pointer/global clamps also require checking the
  conditional store and operand evaluation, not just their final value.
- A message-cracking macro that splits a window procedure into handlers, or
  an MFC rectangle accessor, needs evidence for that API and call boundary.

Search both `src/` and `include/`: an expanded SDK operation may be inside a
project macro. Review preprocessed uses as well as raw source, tracing each
macro to the pinned definition. Inspect HRESULT tests, comparison/selection
expressions and branches, memory calls and record types, casts, bit packing,
GDI/global-memory call sequences, COM dispatch, multimedia event words,
CRT stream/character-table accesses, and Direct3D vector/color arithmetic.
Compare every affected TU with the actual MSVC profile; equivalent C/C++
expressions do not guarantee identical code generation.

Plausible recoveries that remain expanded, and cases requiring different
semantics or library evidence, are tracked in
[vendor macro recovery](todos/vendor-macros.md). Source-hash resets use the
existing [syntactic recovery ledger](todos/syntactic-recovery.tsv).
