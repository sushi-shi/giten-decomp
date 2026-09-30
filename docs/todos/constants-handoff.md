# Constants work list: handoff

Goal: drive the open count of `giten verify constants` to 0. Each open numeric
literal either becomes a name (an enum or define backed by evidence, `NULL`,
or `true`/`false`) or gets a row in `config/constants.tsv` saying why it stays
numeric. The floor in that file only goes down.

## State

The constants work list is complete: `giten verify constants --gate` reports
zero open literals and no stale review rows. The floor is zero. Values with
unproven identities remain numeric in narrowly matched `config/constants.tsv`
rows; the evidence needed to name them is listed below.

The enum-domain view covers the scanned units. Use the retail compile and
comparison to check source changes.

## Batch workflow

1. `export GITEN_DIR=$PWD PYTHONPATH=$PWD/scripts`
2. Edit, then run `giten build` twice.
3. `giten verify status` must show 0 REGRESS and 0 RESET. DIPs in unedited
   functions are fine.
4. `giten verify constants --gate` must show no stale rows and no un-rowed
   compiler-proven replacements.
5. Run clang-format on the changed lines. New files need a full clang-format.
6. Run `git checkout -- README.md docs/todos/syntactic-recovery.tsv` and
   commit.
7. Run `giten verify constants --update-floor`, commit as
   `constants: lower floor`, and push.
8. Rebank from time to time with `giten verify bank --baseline-only`, committed
   as `match: rebank …`. This keeps TU dips in edited functions reported as DIP.

Tools:

- `giten verify constants --list partyaction` lists the open items.
  - The list is also in `build/gen/constants_open.tsv`, with columns file,
    line, owner, spelling, group, detail.
- `-v` with `build/gen/bare_constants.tsv` (column 10) shows the
  compiler-proven replacements.
  - Apply them by hand: `--fix` also rewrites the ones kept on purpose.

## Pitfalls

- Rows are fnmatch globs and the first match wins.
  - Put specific rows before broad ones; otherwise the specific row goes stale
    and the gate fails.
  - A broad row can also hide literals that already have names. For example,
    the `OpenDataFile` row hid `DataTableId` literals.
- Including `Game/AreaNpc.h` or `Game/SceneHotspot.h` in the fieldobj or
  FieldHud TUs clashes on `GrbToRgb`. Use small split headers instead, such as
  `Game/RoomRegion.h` and `Game/SceneHotspotKind.h`.
- `i < sizeof(x)` makes the compare unsigned. Prefer an `int` count define.
- The include-sorting helper can reorder existing includes. Check with
  `git diff | grep '^[-+]#include'`.
- A game-state phase gets a per-state enum (`XxxPhase` with `XXX_PHASE_*`),
  like `WorldMapPhase` and `MenuStateStep`.
  - Sub-state step functions return `SUBSTATE_RUNNING` or `SUBSTATE_FINISHED`.

## Evidence sources

### The disc image

`giten.codecs.corpus.disc_files(<DDSWIN.BIN>)` reads the image. It was last at
`/tmp/giten-codec-parity/build/codecs/DDSWIN.BIN`.

Map area names:

- File: `M/Mxxxx.BIN`.
- Decode: `n = u16(data, 0)`, then `r = decrypt(data[2:2 + n])`.
- The name is at `r[u16(r, 2):]`, in cp932.
- Areas 0x09 and 0x85 both carry the name 新宿都庁.
- `MapAreaId` includes records with distinct names from these files. The
  0x3d and 0x43 records are 千代田線 and 日比谷線; `LoadWallTextures` tests these
  two areas for the alternate wall texture. Other duplicate or blank map
  names still need their separate identities established.

Object record names:

- File: `P/Pxxxx.BIN`, where `xxxx` is `0x2000 + record id`.
- Decode: `n = u16(data, 0)`, then `r = decrypt(data[2:2 + n])`.
- `ObjectRecord.name` starts at `r[54]` and is cp932. Records 0x22,
  0x36, 0xce and 0x117 name Marduk, Pyankara, Primrose and Doppelganger.
- `ObjectRecord.id`, `FieldObject.kind` and `Character.id` share this
  record-ID domain and retain their retail signed-word storage.
- Records 0x26 and 0x18f name Ishtar and Hell Dog. `FindAbleHumanMember`
  accepts these two alongside the ordinary human IDs.

Item record names:

- Weapon passive codes 0x86 and 0x87 restore the attacker's HP and MP,
  respectively, by the hit damage. ET0001 swords 0xd5 and 0x144 carry
  0x86; no weapon record in this disc carries 0x87.
- `ET0001` is a decrypted count and offset table; item records 1, 0x21,
  0x24 and 0x71 name Wound Medicine, Kushinada's Jar, Soma Cup and Core
  Shield. Kind-1 names begin at record +21; kind-5 names begin at +17. In
  item-use code, these were previously mistaken for equal-valued skill ids.
- `ItemId` now includes the empty slot (-1) and no-item record (0);
  equipment, bag, drop and decoded-record storage retain their retail widths.
- Their bank-7 flags 0xff, 0xfe and 0xfd respectively mark Kushinada's Jar
  and Soma Cup used until the full moon, and Core Shield active until the
  next moon phase. The menu disables the first two while marked; field traps
  are ignored while the last is marked.
- Item IDs 0xad..0xb4 are the lover's right and left legs, right and left
  arms, chest, abdomen, head and heart. They are the eight timed scenario
  items whose expiry is tracked by `StampSpecialItem`.

Skill family names:

- Family 0 holds the built-in attacks and unrelated actions. `FindSkill`
  excludes it from family searches even when a caller asks for 0; it is
  `SKILL_FAMILY_NONE`.
- ET0004 skill record 1 names Sword Attack; the basic attack path selects
  that record when it has no equipped weapon item.
- Record 250 names Self Recovery. `ApplySkillEffect` changes its combined
  kind/mode byte to 2 before dispatching it as a restore skill.
- ET0004 kind 0 contains only the sixteen built-in attack records (including
  the no-attack entry); kind 1 contains the ordinary attack-skill records.
  Both use the default attack handler in `ApplySkillEffect`.
- In ET0004, family codes 1..5 each contain one consistent spell series:
  Agi, Zan, Dawm, Zio and Bufu. Codes 6, 7, 8, 11, 14 and 16 group
  expulsion, Megi, remedy/revival, Shibabu, Kaja/Kunda and Dia skills.
  Code 9's three records all use the mental attack attribute. Code 22's
  four records all use the fire attribute. Code 25's eight records all
  inflict conditions. Codes 23, 27, 29, 30 and 33 group ice gas/breath,
  songs, sword techniques, martial arts and arrows.
  `SkillParameters.family` retains its retail byte storage; mixed families
  still need individual identities.

Training group:

- `BattleStatGroup` index 3 is trained for the player by analyzing a field
  actor, talking to one, and summoning. `ApplyTraining` raises battle-stat
  word 18 from that counter, and the group's affiliation growth uses
  Intelligence and Charm. The other three groups train weapon, gun and magic.
  The status panel draws this fourth group only for human characters;
  `RecalcDerivedStats` preserves all four named level words.

Fusion summary kind:

- Kind 0 leaves the result kind unspecified. `SetFusionSummaryKind` derives
  higher, lower or equal from source and result levels when it receives 0;
  a zero-initialized summary also remains 0 before a result is available.
  Kinds 1..7 follow the ET000C special-fusion table and pair-result builder;
  8 is a direct table demon and 9 a fallback demon.

### Event-flag names (ET0018)

`ET0018` is the developers' event-flag name table.

Format, after `decrypt`:

- A 2-byte entry count.
- Then 29-byte entries: a big-endian `bank * 256 + bit`, a 26-byte cp932 name,
  and a zero byte.

Coverage:

- Banks 0 and 1 hold the scenario flags. The names run on through bank 1 bits
  0-28.
- Bank 2 holds the owned maps and programs.
- Bank 4 holds the boxes.
- Bank 8 bit 0 is "no enemies".
- Bank 9's names are only the Shinjuku base's.
- Bank 14 holds the actor flags. Bits 11-16 contradict the code, so they are
  not used.
- Bank 0 bits 137..142 name the six withered lover's limbs and torso parts;
  `s_timedItemFlags` clears them when their corresponding items expire.
  The lover's head uses bank 1 bit 93, beyond this name table's coverage;
  the heart has no expiry flag.

Polarity:

- `ResetEventFlags` sets every bank except 4, 14 and 15. A new level sets
  bank 8.
- A scenario or owned flag means "happened" or "held" when it is **clear**.
- Check each use before writing a comment.

The actor flag at bit 0x20 is anchored: SpawnFieldObject sets it for
alternate-script objects, and pursuit and knockback refuse to move one.

`MS00DF` entry 2 dispatches the reward kind in script long variable 18. Its
branches for kinds 8 and 9 print a bomb throw and a punch, respectively, then
run the HP-change opcode using a value derived from the actor's level in long
variable 19. Kind 8 also selects the second random reward table when passed
into `GrantActorReward`, so the enum keeps both names for that value.

## Deferred identities

Kept as rows until there is evidence:

- Battle-tally slots 4..6, attack attributes 0/1, 9 and 10, and the shared
  result word's noncombat encodings. The ET0004 skill table links effect
  codes to protection slots: 0..3 and 7..14 now have evidence-backed names.
  MS00DD entry 5 dispatches base action results 0..10 to entries 10..19 and
  97; the messages and combat resolver identify miss, no effect, graze,
  success, critical, lethal, immune, reflect, HP/MP absorb and protection.
  The resistance bytes 251..255 map to reflected or absorbed damage. The
  tally effects still need their script and record relationships decoded.
  Attribute 10 bypasses the resistance array with a fixed value of 50;
  the current ET0004 skills and ET0001 attack items do not use it.
- Skill kinds 9, 10 and 14 have no records in the current ET0004 skill table.
  Their distinct identities are still unknown; the combat code shares the
  first two's handler and the field-effect code shares kind 14's handler
  with kinds 12 and 13.
- Target-area codes 1, 3..7 and 17. The collector proves selected-only
  (0), line (2) and weapon-hit (0xff) behavior. Codes 6 and 7 scan the same
  visible grid, but their distinct record meanings are unproven. The gun
  path uses 7; ET0001 also gives 7 to items 110, 111, 116 (kind 5) and
  140 (software), so it is not gun-specific.
- Field-effect codes 0x1a and 0x23. The dispatcher proves
  no effect (0), knockback (1), raised accuracy/evasion (0x20), doubled
  maximum HP and raised weapon power/defense (0x21), and doubled maximum
  HP/MP followed by Ash at the next new moon (0x22). Actor flags 0x23..0x26
  carry these effects.
  ET0004 identifies
  illusion (3), invisibility (0x11), Estoma (0x14), Traesto (0x15),
  Traport (0x16), Trafuri (0x17), Sabatoma (0x19) and Desaman (0x1b).
- Field-object image codes -1 and 0..3 now name the mirrored side,
  facing rows and acting row. Code 4 serves redraw, animation and hidden states;
  its distinct sprite-row identity remains unproven.
- Wall kind 6. The wall-stop table gives it movement class 0 and geometry
  class 3; `GetWallAt` treats it as absent when flooding the visible cells.
  The map record or renderer still needs to establish its in-game identity.
- The identity of area 0x85.
- Item kinds 5 and 6. ET0001 has fifteen kind-5 records spanning charms,
  incense, a shield, dummy items and apparent scenario items; its one kind-6
  record is the Necronomicon. Those records do not establish category names.
