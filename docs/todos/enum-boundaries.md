# Enum boundaries awaiting stronger evidence

The reviewed enum value ledger lives in `config/reviews/enum-reuse.tsv`. These
source values look enum-like but cannot be placed in one closed value domain
from the current producers and consumers. Keep their numeric storage or the
existing named boundary until the payload path is established.

| Candidate | Current evidence | Missing evidence |
| --- | --- | --- |
| Text-plane handles | `src/Ui/message.c` initializes `s_messageWindow` to `TEXT_PLANE_NONE`, then stores the result of `CreateTextPlane`. `src/Game/statestack.c`, `treasurebox.c`, and `fusion.c` do the same for other windows. | The nonnegative values are dynamically allocated plane indices, so a finite enum of window roles would describe neither storage nor the value returned by `CreateTextPlane`. |
| Party and roster indices | `src/Game/partyaction.c` initializes `g_guestIndex` with `PARTY_POSITION_NONE`, `s_swapSaved` with `PARTY_SLOT_EMPTY`, and `s_pickedIndex` with `CHARACTER_ID_NONE`. They later hold positions, roster indices, or character IDs used to index distinct collections. | The shared `-1` sentinel does not make the positive indices one domain. A type for each index space would need complete caller and storage propagation. |
| List-menu result | `src/Game/character.c` starts `RunStatusListPicker` with `LIST_MENU_CANCELLED`, but replaces a selected result with `g_selectedObjectId`. `RunListMenu` also returns `LIST_MENU_OPEN`. | The result is an open value set: menu sentinels plus arbitrary selected object IDs. `ListMenuResult` intentionally names constants without making the whole return value an enum. |
| Script-switch result | `src/Script/scriptswitch.c` starts `ReadScriptSwitch` with `SCRIPT_SWITCH_END`, then stores arbitrary script keys and may OR `SCRIPT_SWITCH_LOCAL_JUMP` into the result. | A key and an encoded jump flag share one word. The end marker alone cannot type that word as a closed enum. |
| Handle index | `src/Game/partyaction.c` initializes `s_gunDistribution` with `HANDLE_NONE`, then stores a numbered memory handle. | Handle numbers are allocated identifiers, not enum members. |
| Contest stat selector | `ReadContestValues` in `include/Script/ScriptOperand.h` accepts `ContestStat` values 11..14, while `src/Script/scriptactor.c` also passes `STAT_PROTECTION` from `CharacterStat` (value 4). | Its parameter is a union of base-stat indices and contest-only selectors. The right combined type or explicit conversion boundary requires auditing all callers and the callee's table access. |
| Scene hotspot kind | `SceneHotspotKind` names object=2 and box=3, while `AddSceneHotspot` in `src/Game/abortflag.c` also switches over 0, 1, 4, and 5. The drawing path that passes `kind` is currently dead code. | The four extra roles lack proven names and producers; naming them from their numbers would invent identities. |
| Wait input mask | `StepWaitState` in `src/Game/waitstate.c` consumes only bits 1 and 2 for left-button conditions and always accepts right down. Callers pass 0, 10, 0x3c, and 0xffff; `OpWaitMessage` also forwards an operand-derived mask. | The meaning of the extra bits and whether raw caller values are authored masks or inactive arguments remains unresolved. |
| Cell codes | `s_cellKinds` in `src/Game/clock.c` contains raw codes 0x8b, 0x8c, and 0x8f. `CellCode.h` records that their authored roles remain unknown. | A table dispatch for `CELL_EVENT_FLOOR_PROPERTY` proves their event family, not distinct semantic names. |
| Party panel layer slots | `s_layerOrder` in `src/Platform/winmain.cpp` contains `SCREEN_LAYER_FIRST_PANEL + 1` through `+ 5` as typed `ScreenLayerSlot` values. | The six panel slot positions are proven, but the individual panel identities are not; casts at this index construction keep that boundary visible. |
| Sound IDs | `MapEffectSoundId` and `MapSoundEffectId` in `src/Platform/d3dapp.cpp` compare fifteen numeric IDs each; ID 59 maps to different outputs in the two directions. | The numeric comparisons need resource or caller evidence before a named sound enum or shared conversion table is justified. |

Several strict C enum warnings are expected for bit sets: `PanelFlags`,
`PaletteUpdateFlags`, `PickFlags`, and `CellKindFlags` are combined or cleared
with integer bit operations. `CellCode` also has the three unnamed table codes
above. These warnings do not by themselves identify an enum reuse.
