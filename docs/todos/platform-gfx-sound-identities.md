# Platform and graphics identities

These are live function-level `@identity-TODO` boundaries in the Windows and
legacy graphics units. A call or an equal numeric value narrows a role; it does
not prove the original API name, an unused operation, or a record layout.

| Site | Current evidence | Evidence needed to close it |
| --- | --- | --- |
| `InvalidateSelectedHotspot` in `winmain.cpp` | Retail calls this copy from `StepObjectTowardParty` and `StepScriptActor`; it calls the separate `ClearSelectedHotspot` copy from `RunFieldEncounter`, `StepObjectTowardParty` and `RunWorldMap`. The bodies have the same effect but different addresses. | Recover the declaration or linkage boundary that produced two copies; the shared caller alone does not prove that one copy can be removed. |
| `TickFrameCount` in `winmain.cpp` | `RenderEventMode` increments the count up to 60 for selected scene pictures; the message handler tests it against `INPUT_DELAY_FRAMES`, and `AllowImmediateInput` bypasses the delay. | Recover the original timer/input API naming or another owner witness. The behavior alone does not identify the authored names. |
| `EndSaveRenderMode` in `winmain.cpp` | It changes mode 15 to scene mode; `OpSaveDataCommand` calls it before reading a save summary. | Find a second producer or renderer for mode 15, or a source/resource witness that identifies it as the save screen. |
| `AnimateDoor` in `winmain.cpp` | Its mesh frame occupies vertices 0–23 and moving leaves occupy 24–47; the function writes mirrored leaf coordinates. | Recover the mesh creation and vertex submission as a whole before naming individual vertex roles or splitting the mesh type. |
| `RenderEnemy` in `winmain.cpp` | Three calls pass `(true, false, true)` twice and `(true, false, false)` once. The last flag's distance-lighting role has both caller forms; the shading and view-filter flags have only one observed caller value. The lit-frame and 2D fallback paths remain partly decoded. | Trace those branches to image records and texture payloads; find an independent producer for the two one-valued flags before treating their names as original. |
| `RenderFieldView` in `winmain.cpp` | Retail initializes and tints a backdrop quad, yet the reconstructed live path does not submit that quad. | Find a retail draw reference or an independently recovered renderer branch before deleting or repurposing the quad. |
| `WinMain` in `winmain.cpp` | Two error captions are zeroed four-byte BSS arrays passed to failure boxes. | Recover a declaration or use that distinguishes their intended storage form; nearby zero bytes do not prove a larger object. |
| `ReadWorldMapTileCode` in `worldmap.cpp` | Four marker colors map to tile codes 15, 4, 5 and 2. `g_worldTravelTerrainFlags` marks all four as passable, so its consumer does not distinguish them. | Find map data or rendering behavior that distinguishes the marker kinds and identifies the colors independently. |
| `ClearMaskView` in `vram.c` | The empty Windows hook runs before `ResetMask(true)` on map or scene entry. | Recover an original API declaration or platform implementation; a no-op body cannot show what view state the legacy call once cleared. |
| `FreeImageHandle` in `vram.c` | The body returns zero without freeing its argument. Callers assign the result back to image slots, including the field image cache and menu/effect images. | Recover the original ownership contract or platform counterpart before renaming it to imply an actual free or removing the call boundary. |
| `AllocScreenSaveHandle` in `vramaccess.c` | It allocates header + four bytes per column/scanline + 128 bytes; known Windows stubs do not consume the 128-byte tail. | Find a reader of the tail or a screen-save record format before assigning it a field or enlarging the declared header. |
| `FillCell` in `vramcell.c` | A script cell operation passes a four-bit color, but the Windows body is empty. | Recover a platform implementation, record format or caller that observes filled cell planes before modeling its side effect. |

The D3D device query's MMX and RAMP line-cap fallback reads HAL flags in retail.
That behavior is retained as a source comment; it does not need an identity TODO.

Two initialized negative values have no existing same-domain constant:
`s_musicTrack` starts at `-1` for no track, while `s_joystickCount` starts at
`-1` until `InitJoystick` runs. `EFFECT_ID_NONE`, `FIELD_OUT_X` and the
hotspot sentinel each belong to different domains. The joystick axis values
and signed enemy-image entries use negative arithmetic or encoding, rather
than an absent-object sentinel. Keep these meanings separate unless a shared
producer/consumer protocol establishes one domain.
