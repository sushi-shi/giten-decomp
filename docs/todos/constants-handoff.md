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
- M0000 is named 初台ｼｪﾙﾀｰ (Hatsudai Shelter), distinct from M0082's 初台;
  a warp in M0080 targets area 0, level 7.
- Areas 0x09 and 0x85 both carry the name 新宿都庁.
- Area 0x85 is a separate ten-level Tocho map with different wall layouts;
  the Bael Castle map (0x35) warps into its level 7. Its enum name marks
  it as the alternate Tocho map without assigning a story phase.
- M0028 and M0030 both display 渋谷 and have the same five wall layouts, but
  their decoded records differ in events and other level data. The code tests
  0x30 as Shibuya, so 0x28 is named its alternate map without a story-phase
  claim.
- M0029 and M002B both display 品川ホテル and have separate five-level wall
  layouts. A retail debug trace calls 0x29 品川ホテル（幻）, so 0x29 keeps the
  illusion qualifier and 0x2b uses the displayed hotel name.
- M0012 and M0013 both display シャンシャンシティ. M0012 has nine levels and
  M0013 ten, with different wall layouts; 0x13 is the area the source tests,
  so 0x13 keeps the unqualified name. MS000B entries 5 and 6 use bank-0 flag
  0x47 (ET0018: after Ikebukuro cleared): when it is clear they travel to
  M0013, otherwise to M0012. M0012 is the before-clear map.
- M0016 displays 御茶ﾉ水ｼｪﾙﾀｰ (Ochanomizu Shelter), distinct from M008A's
  御茶ﾉ水 (Ochanomizu). Its map has fifteen levels.
- M0011 and M0018 both display ミレニアム　総本山 and share seven wall layouts,
  but their decoded records differ. Source area checks use 0x18 as Millennium
  Headquarters. MS000B entry 9 uses bank-0 flag 0x9b (ET0018: after
  Belphegor defeated): when it is clear the entry travels to M0011,
  otherwise to M0018. M0011 is the after-defeat map.
- M0043 and M0044 both display 日比谷線 (Hibiya Line), with six and three
  distinct levels respectively. `LoadWallTextures` tests 0x43 as Hibiya Line;
  0x44 is its alternate map.
- M0023 and M0024 both display 御花屋敷 (Ohanayashiki), but their one-level
  wall layouts differ. `OpLoadSprite` checks 0x24 as Ohanayashiki; 0x23 is
  selected by MS000B entries 10 and 11 when bank-0 flag 0xa7 (ET0018:
  after Asakusa cleared) is clear. M0023 is the after-clear map.
- M0007 and M0008 both display 新宿地下道 (Shinjuku Underground Passage).
  Their warps connect to M0006 (新宿地下街, Shinjuku Underground Mall) level 1
  at y=4/6 and y=25/26 respectively. Since map north is decreasing y, these
  are the north and south passage maps.
- M000E and M000F both display オリンピックプール (Olympic Pool). M000E contains
  one level at floor 1; M000F has that floor plus three basement levels
  displayed as floors -1, -1 and -2. Their pool-floor wall layouts differ.
  The enum names describe the decoded layout without assigning a story phase.
- M0041 and M0050 both display 銀座線 (Ginza Line). M0041 has two levels with
  1-by-10 and 1-by-8 wall grids; M0050 has three levels with 5-by-1,
  1-by-3 and 6-by-1 grids. Their enum names use the level counts because
  neither map has a source check that identifies a primary version.
- M0060..M0068 are one-level, 1-by-3 transition maps displaying the names
  of larger areas. Their code-0x79 entrance links call MS000B entries 5..11;
  each entry branches to one of two destination maps for that location.
  M0060 and M0061 use entries 5 and 6, whose destinations are Shanshan City
  level 2 (floor 1) and level 5 (floor 10). M0064 and M0065 use entries 10
  and 11, whose Ohanayashiki destinations are (8, 10) and (10, 4); map north
  is decreasing y, so these are the south and north entrances. M0068 uses
  entry 9, reaching Millennium Headquarters level 6 (floor 7). M0062/63
  (Shinagawa Hotel) and M0066/67 (Ochanomizu Shelter) each share one MS000B
  destination entry. Their second cell distinguishes them: M0062 and M0066
  use code 0x7b to run MS0043 entry 0, which offers ARM terminal connection;
  M0063 and M0067 use code 0x40 to run MS003C entries 8 and 13, which load
  a keeper record and open entry 16's service-terminal menu.
- M0002 and M0086 both display 亜空間 (Subspace). MS0064 entry 2 opens a
  shrine's Subspace entrance and travels to M0002; M0002 has a warp to
  Kishimojin (M0089) and a link to MS0064 entry 3's Hariti encounter.
  MS005F entry 5 enters M0086 while the protagonist sleeps and dreams;
  MS006C entry 7 also enters it after Adonis administers an initiation
  drink and the protagonist's consciousness fades. M0086 links to MS006D
  entries 38 and 39's vision dialogue. The distinct route and vision roles
  identify the two Subspace maps.
- M0005's displayed map name is blank. MS0035 entry 2 prints 明治神宮入口
  (Meiji Jingu entrance) immediately before changing to area 5. MS0061
  entry 6 also enters area 5 during the gate-to-Yomi scene, consistent with
  the shrine entrance's story role.
- M0057's displayed map name is blank. In MS0021 entry 5, Yoshino Hime
  tells the protagonist to enter the darkness of their own heart and sorrow;
  their consciousness falls away immediately before the script changes to
  area 0x57. The four-level area's links run MS0014 entries that stage
  memories and visions, including the protagonist's mother in entry 9.
- The disc contains 109 map files, and `MapAreaId` names each one, including
  duplicate and blank displayed names. No M0054 file exists. The 0x3d and 0x43
  records are 千代田線 and 日比谷線; `LoadWallTextures` tests these two areas
  for the alternate wall texture.

Automap marks:

- Marks 0..3 are white party arrows pointing up, right, down and left.
  `DrawAutomapRegion` and `DrawMapOverlay` pass the party's direction relative
  to the displayed map direction, in the same order.
- The automap icon table uses image marks 7, 8 and 9 for exits, stairs
  up and stairs down. Its mark byte now has an `AutomapMark` domain, shared
  with live NPC (4) and field-object (5) marks.
- Retail bitmap resources selected by `g_mapMarkImages` show kanji signs for
  marks 12..22: 電, 武, 転, 薬, 邪, 病, 泉, 回, 防, 酒 and 道. The
  automap icon table maps cell codes 0x50..0x59 and 0x5b to those marks in
  order; codes 0x85..0x87 also use the spring mark. MS0038's transfer
  device, MS003E's fusion hall, MS003A's spring spirit, MS003F's incense
  and healing room, and MS0040's bartender establish codes 0x52, 0x54,
  0x56, 0x57 and 0x59. Code 0x5b calls MS0039's item shop; its 道 sign
  abbreviates 道具 (items), not a dojo. Code 0x50 runs MS0042's software
  price list, 0x55 runs MS0033's treatment menu, and codes 0x51, 0x53 and
  0x58 run MS0039's merchant menus. The 武 and 防 signs distinguish weapon
  and armor shops even where those codes call the same merchant entry.
- Mark 10 is the white E bitmap. Cell codes 0x44..0x46 use it; all are map
  links. In working shafts, 0x44 marks the lowest stop, 0x45 the highest,
  and 0x46 the intervening stops. M000A's six stops (floors 1, 4..8),
  M0014's three stops (floors -5..-3), and M001B's two separate shafts
  show the pattern. MS003B's menus omit the current floor from their
  destinations. M0028 uses 0x45 at three floors for a disabled elevator;
  its MS0006 entry 38 says the elevator is destroyed.
- Codes 0x85..0x87 share the spring icon with 0x56. Their map links call
  MS003A: 0x85 uses entries 2..4, 0x87 uses 6..7, and 0x86 uses 8..13.
  Each entry sets script long variable 0 before entering the shared entry
  27; entries 8..13 also set other variables. The script branches by those
  values, so the icon alone does not establish separate names for the codes.
- Cell code 0x7d uses the stairs-down mark and occurs in 25 station links,
  all calling MS0044 entries 3..28. That script describes stairs leading
  down to a subway platform and offers the platform route.
- Mark 11 is a red stick person. The icon table gives it to cell code 0x48,
  whose records occur in the area object lists; `SpawnLevelObjects` creates
  area NPCs from those records. The accepted cell-code range is 0x48..0x4e;
  the individual codes select NPC pictures whose identities remain open.
- Mark 24 selects retail bitmap 272, a 16-by-16 image whose pixels all use
  black palette entry 1. The icon table uses it for code 0xbf and its final
  sentinel row, so it is a blank mark.
- Mark 6's standing-figure bitmap belongs to cell code 0x40. Across the
  disc's maps, its scripted links load MS003C's service-terminal menu or
  enter a small map whose second cell loads that menu. Mark 23's red
  C-shaped bitmap belongs to code 0x7b. Its script cells run MS003D or
  MS0043's ARM-terminal menu or enter a small map whose second cell runs
  that menu. The two 0x7b links and one 0x40 link in M0000 have no direct
  script-file referent in their link records.

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
  inflict conditions. Codes 23, 27, 28, 29, 30 and 33 group ice gas/breath,
  songs, eye blinding and needle attacks, sword techniques, martial arts
  and arrows.
  Codes 19, 20 and 21 group beast-style attacks (growl, bite, scratch,
  body slam and howl), aerial attacks (shriek, claw, wingbeat and tornado),
  and body attacks (crush, rampage and tail). The groups describe the moves;
  the object records also assign some of them to human and mechanical actors.
  `SkillParameters.family` retains its retail byte storage; mixed families
  still need individual identities.

Battle tally names:

- ET0004 record 286 is the only kind-3 skill with effect code 4. Its name is
  不治呪縛 (Incurable Curse); `UseBattleTallySkill` writes that effect code to
  the same numbered battle-tally slot. The slot's later gameplay effect is
  not established by a direct reader in the reconstructed C.

Training group:

- `BattleStatGroup` index 3 is trained for the player by analyzing a field
  actor, talking to one, and summoning. `ApplyTraining` raises battle-stat
  word 18 from that counter, and the group's affiliation growth uses
  Intelligence and Charm. The other three groups train weapon, gun and magic.
  The status panel draws this fourth group only for human characters;
  `RecalcDerivedStats` preserves all four named level words.

Demon fusion flags:

- The demon table flag byte has low and high fusion-value bits (0 and 4)
  plus independent unavailable bits (2 and 6). The getters return -1 for
  an unavailable pair, otherwise its value 0 or 1. The game meaning of
  either value pair remains open.

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
- Bank 2 bit 0x39 gates the MAPPING command and NPC automap detail. Both
  MAPPING entry paths say the AMS is not owned while it is set, establishing
  `OWNED_MAPPING_AMS`. MS0025 entry 1 clears the bit as the characters are
  issued automapping, and MS0028 entry 0 clears it when they receive a
  modified arm terminal with automapping. Bit 0x38 enables
  `MarkObjectsOnMap` in `DrawMapOverlay`; `OWNED_AMS_OBJECT_MAPPING` names
  that capability without assigning it an AMS version. The direct MS-script
  references to 0x38 test or set it, but do not clear it.
- Bank 4 holds the boxes.
- Bank 8 bit 0 is "no enemies".
- Bank 9's names are only the Shinjuku base's.
- Bank 9 bit 0x7b selects a fixed background while clear. `OpShowBackground`
  replaces the requested picture with image 0x31, variant 4, and
  `RestoreBackground` makes the same substitution in its scene cell. The
  `AREA_FIXED_BACKGROUND` name records this shared picture gate without
  assigning it to a story event.
- Bank 1 bits 0x5e and 0x75 select FC4001 and FC4002 frame 1 for Katsuragi
  and Tachibana's status portraits while clear. The retail bitmap chains
  show civilian clothing in frame 1 and armored DB clothing in frame 0.
  MS001E entry 3 sets both bits after issuing DB equipment and uniforms;
  MS0055 entry 2 clears both before the entrance examination. The enum
  names describe the proven portrait selection, not an assumed story phase.
- Bank 1 bit 0x2b suppresses `TickStepDamage` while set, so its enum name
  describes that code-level effect. MS0056 entry 0 sets it amid a toxic-gas
  and protective-suit scene; MS0051 entry 5 sets it in a hospital scene.
  The specific source of step damage across the story remains unproven.
- Bank 1 bit 0x2d suppresses party combat effects while set:
  `CanAffectCombatant` skips skill costs for a party actor and resolved
  effects for a party target. MS0017 entry 2 clears it early in the
  qualification story. The enum describes the code gate, not a named item.
- Bank 1 bit 0x4f suppresses the fallen-human rescue scene while set.
  `fieldmain` starts MS001A entry 6 only when a human has fallen and the bit
  is clear; the entry immediately sets it, then describes carrying the
  fallen party member to a medical facility. Other story entries reset it,
  so the enum names its observed gate rather than a one-time event.
- Bank 14 holds the actor flags. Bits 11-16 contradict the code, so they are
  not used.
- Bank 0 bits 137..142 name the six withered lover's limbs and torso parts;
  `s_timedItemFlags` clears them when their corresponding items expire.
  The lover's head uses bank 1 bit 93, beyond this name table's coverage.
  Its identity follows from the seventh row's index relative to
  `ITEM_LOVER_RIGHT_LEG`; the heart has no expiry flag.

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

- Battle-tally slot 6, attack attributes 0/1, 9 and 10, and the shared
  result word's noncombat encodings. The ET0004 skill table links effect
  codes to protection slots: 0..3, 5 and 7..14 now have evidence-backed names.
  Attribute 1 is not simply the gun category: ET0001 gives it to 21 melee
  weapons as well as 21 ammunition records; attribute 0 occurs on 98 melee
  weapons and seven ammunition records.
  Slot 5 is set by Tetrakarn and Counterattack and reflects attacks with
  attribute 0. The barrier-cleared message in MS00DF entry 3 uses the generic
  barrier label from MS7F00 entry 52, so it does not distinguish slots 5 and 6.
  MS00DD entry 5 dispatches base action results 0..10 to entries 10..19 and
  97; the messages and combat resolver identify miss, no effect, graze,
  success, critical, lethal, immune, reflect, HP/MP absorb and protection.
  The resistance bytes 251..255 map to reflected or absorbed damage. The
  remaining tally effects still need their script and record relationships decoded.
  The combat resolver identifies result-word tags 0x50, 0x70 and 0x80
  for HP, MP and experience draining skills.
  Attribute 10 bypasses the resistance array with a fixed value of 50;
  the current ET0004 skills and ET0001 attack items do not use it.
- Skill kinds 9, 10 and 14 have no records in the current ET0004 skill table.
  Their distinct identities are still unknown; the combat code shares the
  first two's handler and the field-effect code shares kind 14's handler
  with kinds 12 and 13.
- Target-area codes 1, 3..5, 6..7 and 17. The collector proves selected-only
  (0), line (2), visible-grid (6..7) and weapon-hit (0xff) behavior. Codes
  6 and 7 have distinct record meanings still unproven. The gun
  path uses 7; ET0001 also gives 7 to items 110, 111, 116 (kind 5) and
  140 (software), so it is not gun-specific.
  In ET0004, code 3 has 34 mostly close-range attacks, code 5 has only two
  spear attacks, and code 17 has only six single-target remedy/revival skills.
  Their distinct selection meanings are not established by the collector.
- Field-effect code 0x1a shares Sabatoma's actor-group handler, but its
  distinct identity remains unknown. Code 0x23 returns `FIELD_EFFECT_DONE`
  directly without changing state, so its name describes that behavior.
  The dispatcher also proves
  no effect (0), knockback (1), raised accuracy/evasion (0x20), doubled
  maximum HP and raised weapon power/defense (0x21), and doubled maximum
  HP/MP followed by Ash at the next new moon (0x22). Actor flags 0x23..0x26
  carry these effects.
  ET0004 identifies
  illusion (3), invisibility (0x11), Estoma (0x14), Traesto (0x15),
  Traport (0x16), Trafuri (0x17), Sabatoma (0x19) and Desaman (0x1b).
- Cell code 0x8a selects the lower half of the treasure-box texture in
  `RenderTBox`. The dead `IsHotspotTreasureOpen` frame arithmetic also gives
  it the third open/closed frame pair, so it is a `CellCode` enumerator rather
  than a separate treasure-box constant.
- Field-object image codes -1 and 0..4 name the mirrored side, facing rows,
  acting row and reaction frame. The fifth frame of the disc's five-BMP actor
  images is a distinct reaction pose. `FlashHitObject` selects it after a pool
  change; the vanish animation and hidden state reuse it, with the lit bit
  selecting the animation lighting path.
- Wall kind 6. The wall-stop table gives it movement class 0 and geometry
  class 3; `GetWallAt` treats it as absent when flooding the visible cells,
  and `BuildRoomGeometry` draws no quad for it. Its geometry-class result
  still needs reconciling with that open behavior before naming the kind.
- Item kinds 5 and 6. ET0001 has fifteen kind-5 records spanning charms,
  incense, a shield, dummy items and apparent scenario items; its one kind-6
  record is the Necronomicon. Those records do not establish category names.
