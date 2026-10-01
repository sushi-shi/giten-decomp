# Typed layout and API boundaries

The remaining casts and partial layouts below need evidence before they can be
replaced. The exact functions that require a source-rule exception are listed
in [rule-exceptions.tsv](rule-exceptions.tsv). A matching score alone does not
prove that a proposed aggregate or signature is the original one.

| Boundary | Current example | Evidence needed for cleanup |
| --- | --- | --- |
| Character versus field actor | `FieldActor` in `include/Game/FieldSight.h` keeps an opaque `character[0x1f9]` prefix before `pos` and `direction`. `GetFieldActor` in `src/Game/fieldobj.c` returns `Character*` at `FieldObject.kind`; `KnockBack` in `src/Game/treasurebox.c` then views the same pointer as `FieldActor*`. | Recover a shared character prefix from whole-object copies and callers. A complete `Character` cannot be embedded at that offset: its tail would overlap the field object's script pointer. Then replace the listed casts without changing the retail referents. |
| Platform calls into game C | `src/Platform/winmain.cpp` casts `RedrawFieldAt` to a three-int function at the `AnimateMove` call, passes unsigned map dimensions through `GetMapSize`'s signed-word pointers in `BuildRoomGeometry`, and casts `SetMouseState` to a byte/LONG signature in `PollInput`. | Recover the declarations visible to the original Windows translation unit. The current C declarations and the retail push/sign-extension widths differ; retyping a C++ declaration or function-pointer call requires an ABI and mangling check. |
| Texture source argument | `OpenTextureBitmap` and `LoadTexture` take `const char* name` plus `fromFile`; in `src/Gfx/bitmapio.cpp` the false branch views `name` as a borrowed `BmpFile*`. `src/Gfx/layertexture.cpp` and `src/Gfx/objecttexture.c` pass bitmap data through that parameter. | Recover a typed overload or original heterogeneous API boundary while preserving the exact C/C++ symbol and call sites. A path and a bitmap pointer cannot be represented by one truthful `const char*` type without the selector. |
| Serialized area and object records | `DecodeAreaMap` in `src/Game/clock.c` rebases `AreaLevelRecord` offsets into typed `AreaLevel` pointers; `ReadObjectRecordField` in `src/Game/fieldobj.c` reads a 16- or 32-bit value from a byte buffer. | Treat the casts as file-format boundaries until all offset and width interpretations are proved. Do not substitute an in-memory layout for packed data. |
| NPC palette input | `src/Game/treasurebox.c` passes a packed record's palette words to `LoadNpcPalette` through `u16*`; `src/Game/clock.c` passes a map record cell to `AddAreaNpc` through `u8*`. | Prove the source record format and alignment before replacing the byte or word views with a shared typed record. |
| Per-kind item parameters | `ItemRecord.params` in `include/Game/ItemRecord.h` remains a byte array. `DecodeItemRecord` writes different offsets for weapons, guns, gems and other kinds. Consumers with established meanings now use named accessors. | Recover each kind's complete variant layout and whole-object use before replacing the array with a union or separate record types. |
| Header ownership | `include/Platform/GameCalls.h` collects game declarations needed by the Windows layer, including `RedrawFieldAt`, texture helpers and `ClearHandleTable`; moving a declaration into an owner header changes MSVC 5 translation-unit state. `include/Platform/WinMain.h` still marks `g_windowRect` as a placeholder extern. | Move each declaration when the owner and original include boundary are evidenced, then compile every affected unit and compare functions and referents. |
| Incomplete device and NPC records | `D3DDeviceInfo` in `include/Platform/D3DApp.h` retains `reserved1[0x38]` and `reserved2[0x1b4]`; `AreaNpc` in `include/Game/AreaNpc.h` retains a first `pad00[0x14]` block passed whole to drawing code. | Name fields only after callers, storage, and whole-record access establish their roles. Size and offset alone do not establish identity. |

The BMP byte-walking macros in `include/Gfx/Bitmap.h` and MIDI event-buffer
walking in `include/Sound/MidiStream.h` are retained typed format boundaries:
their offsets come from serialized sizes. The remaining `reinterpret_cast`
sites in `src/Platform`, `src/Gfx`, and `src/Sound` generally cross Win32,
COM, WinMM, or serialized-byte APIs. Review their caller and callee types
before removing a cast; a textual cast count is not a cleanup criterion.
