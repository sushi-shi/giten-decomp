# Retail libraries and SDK helpers

DDS.EXE uses Microsoft's VC5 static C runtime and Windows/DirectX APIs.
There is no identified MFC, ATL, STL, zlib, or other bundled third-party
library contribution. Names such as `CMMIO` and `CMidiStream` in this tree
are reconstructed wrappers, not evidence of MFC.

| Provider | Evidence and scope |
| --- | --- |
| Visual C++ 5.0 SP3 CRT | `/ML`, `LIBC.LIB`, and the function identities in [`functions_static_libs.tsv`](../config/retail/functions_static_libs.tsv). The compiler also requests `OLDNAMES` for CRT compatibility names. |
| Win32 | Retail imports KERNEL32, USER32, GDI32, and ADVAPI32, in that order. |
| DirectX | Retail imports DDRAW, DSOUND, and DINPUT. Direct3D interfaces are obtained through DirectDraw COM calls. The reconstruction uses the DirectX 6 SDK headers. |
| WinMM | Retail imports WINMM for multimedia, joystick, MIDI stream, and MMIO APIs. |
| DirectX static data | `DXGUID.LIB` supplies interface/device GUIDs; `DINPUT.LIB` supplies the keyboard/mouse formats. Exact library-object evidence is recorded in [`data_static_libs.tsv`](../config/retail/data_static_libs.tsv). |

The explicit link inputs are in
[`LINK_LIBS`](../scripts/giten/graph/link.py); see also
[candidate linking](linker-flags.md). The toolchain contains more libraries
than this executable uses. Inherited MFC labeling support and the empty
[`functions_zlib.tsv`](../config/retail/functions_zlib.tsv) channel do not
establish dependencies.

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
| Direct3D | `d3dtypes.h`, `d3dvec.inl` | `D3DVAL`, `RGBA_MAKE`, `RGBA_SETALPHA`, `RGB_MAKE`, and the vector/matrix constructors and operators. `/Ob0` can leave the inline SDK bodies out of line. |
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
- A message-cracking macro that splits a window procedure into handlers, or
  an MFC rectangle accessor, needs evidence for that API and call boundary.
