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
- `RunLevelUp` has distinct close, human-growth, demon-growth and redraw step
  domains within its named phases. The human and demon `GetGameSub` limits are
  iteration counts, not members of those step domains.
- `StepForMode`'s input values directly select positive, neutral and negative
  alignment steps and have their own `AlignmentStepMode` domain. The two
  script opcodes remap an operand of 1 to helper mode 2 before calling it;
  their operand domain remains separate.

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
  values. In entry 27, selectors 1, 3, 7, 8, 9 and 10 each take a separate
  branch to entry 37; selector 13 has another guarded branch to entry 38.
  Those special cases span all three cell codes, so the branches do not
  establish a single effect for any one code. Entry 13's branch can reach
  the Hagenti encounter asking for the Kusanagi sword, but the same code
  0x86 also uses entries 8..12. The icon
  and that encounter do not establish separate names for the codes.
  Across the current map records, code 0x85 occurs in four links, 0x86 in
  thirteen and 0x87 in five. M0081 (会話チェック, "conversation check") places
  the three codes in adjacent cells at (12,15), (13,15) and (14,15). This
  confirms that the map data distinguishes them even in one location, but
  does not explain what the three variants mean. The PC-98 disc's 22 links
  with these codes are byte-identical to Windows, as is decoded MS003A.
  Its executable's automap table gives all three the same mark byte 0x11.
- Cell code 0x7d uses the stairs-down mark and occurs in 25 station links,
  all calling MS0044 entries 3..28. That script describes stairs leading
  down to a subway platform and offers the platform route.
- Mark 11 is a red stick person. The icon table gives it to cell code 0x48,
  whose records occur in the area object lists; `SpawnLevelObjects` creates
  area NPCs from those records. The accepted cell-code range is 0x48..0x4e.
  `GetNpcImageOfCode` indexes from `g_physicalRecoveryConditions` into the
  adjacent `s_fourBitMasks` data at exactly these seven codes, returning
  15, 1, 2, 4, 8, 5 and 10. The single bits match north, east, south and west
  in `GetFacingBit`; the last two values pair opposite directions. These
  masks name all seven `CellCode` values. Windows uses the record's separate
  texture slot for drawing and does not read its legacy `directionMask` field.
- Mark 24 selects retail bitmap 272, a 16-by-16 image whose pixels all use
  black palette entry 1. The icon table uses it for code 0xbf and its final
  sentinel row, so it is a blank mark. Code 0xbf has no record in any of the
  eight cell lists across the 109 Windows area maps; its `CellCode` name
  describes only this table behavior.
  The final code row of the automap and cell-kind tables is
  `CELL_CODE_TABLE_END` (0xff); a cell-record list instead ends when its x
  byte is `CELL_LIST_X_END` (also 0xff). They are separate fields and domains.
- Mark 6's standing-figure bitmap belongs to cell code 0x40. Across the
  disc's maps, its scripted links load MS003C's service-terminal menu or
  enter a small map whose second cell loads that menu. Mark 23's red
  C-shaped bitmap belongs to code 0x7b. Its script cells run MS003D or
  MS0043's ARM-terminal menu or enter a small map whose second cell runs
  that menu. The two 0x7b links and one 0x40 link in M0000 have no direct
  script-file referent in their link records.

Alignment traps:

- `RunCellTrap` computes `4 >> (GetAlignmentClassB(member) + 1)` and applies
  damage only when that bit is in the exit record's alignment mask. The class
  labels `CNL` establish mask bits 4 = Chaos, 2 = Neutral and 1 = Law.
- All seven code-0x6c exit records in the Windows map have mask 5, all four
  code-0x6d records have mask 6, and all 145 code-0x6e records have mask 7.
  The PC-98 maps independently have the same seven and four records with
  masks 5 and 6, and 151 code-0x6e records with mask 7. These identify the
  Chaos/Law, Chaos/Neutral and all-alignment trap codes; the mask byte has a
  `CellTrapAlignmentMask` flag domain. Neither disc has an exit record for
  codes 0x68..0x6b, so those code identities remain unnamed apart from the
  first range bound.

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
- `CheckSkillArea` has a separate three-value result for Traesto, Traport and
  Trafuri: allowed (1), blocked by the current field marker (0), and forbidden
  by the area's skill flag (-1). Other skills always return allowed. Its
  caller `CanUseSkill` also returns 0 for an unpaid cost and -1 for a wrong
  game mode, so that broader result retains its own numeric type.
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
  Family 12's five skills create hallucinations, blindness, invisibility or
  decoys, or dispel illusions, so it is the illusion family. Family 13's
  eleven records include a dark barrier, HP conversion, fear, curses,
  petrification, a black hole and zombification; the name dark magic covers
  that set without assigning a narrower spell series. Family 17's seventeen
  records inflict conditions, possession or HP/MP/experience drain through
  gazes, touch, words and kisses. Family 31's eleven records describe melee
  techniques with elbows, knees, thrusts, swords or repeated blows.
  Windows family 18 has nine guard, counter, slash, berserk, shout and
  headbutt actions. The original PC-98 ET0004 also assigns its basic sword,
  unarmed and gun attacks (records 1..15) to family 18; Windows assigns those
  records to family 0 instead. Thus combat action covers the family in both
  versions without restricting it to special maneuvers. PC-98 also assigns
  Sabatoma (record 125) to family 10 where Windows assigns family 0. Family
  10 has sixteen Windows skills: eleven inflict or cure mental and other status
  conditions, two drain or transfer MP, two expel demons, and one
  zombifies allies. The composite name also admits PC-98's Sabatoma summoning
  skill; none of those effects alone identifies the whole family.
  Family 15 has six barrier, reflection or barrier-clearing skills and four
  travel, concealment or escape skills. Family 24 has one electric attack and
  four water skills, including a water barrier. Family 26 has four dances and
  one Fairy Bolt; its composite name keeps that outlier visible. Family 32
  has five elemental attacks (fire, lightning, ice, holy and dark) and three
  demon expulsion, charm or destruction powers. These composite names record
  their mixed members without treating any one subset as the whole family.
  `SkillParameters.family` retains its retail byte storage. All family values
  in the retail skill tables now have members; the composite names describe
  their records, while authored school labels remain unproven.

Battle tally names:

- ET0004 record 286 is the only kind-3 skill with effect code 4. Its name is
  不治呪縛 (Incurable Curse); `UseBattleTallySkill` writes that effect code to
  the same numbered battle-tally slot. The slot's later gameplay effect is
  not established by a direct reader in the reconstructed C.
- `ReportBattleTally` mode 1 remembers and reads an index, mode 0 tests the
  supplied index for clearing, and mode -1 tests the remembered index. Its
  remembered and message indices use -1 when absent.

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
  either value pair remains open. `FusionFlagValue` names unavailable, clear
  and set for both getters and the signed three-bit fusion summary fields.

Fusion summary kind:

- `GetFusionResultKind` sends `FUSION_RESULT_OVER_LEVEL` when the summary's
  over-level bit is set, or `FUSION_RESULT_ALREADY_IN_ROSTER` when the result
  demon is already present for a summary kind that checks roster membership.
  Otherwise it sends the `FusionSummaryKind` value. These two negative
  statuses are separate from the stored summary-kind domain.
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

## Sprite placement override

`g_shopKind` is a one-use sprite placement override, despite its old name.
`OpLoadSprite` sets it for particular images at shop locations, then
`OpPlaceSprite` consumes it: modes 1 and 2 remap image and slot, 3 raises a
matching sprite, and 4..9 force x=40 with distinct y positions. The same
modes occur at different shop categories, so their enumerators describe
placement behavior rather than a shop type.

## Script operand operations

`OpStackMessageWindow` reads a two-value operand: 0 stacks a message window,
while 1 removes the top stack node without closing its window.
`OpStepListMenu` uses a separate two-value operand: 0 advances the buy menu
and 1 advances the sell menu. Their switch branches establish separate enum
domains for the two opcodes.

The item menu context stores a different domain: shop mode 0 replaces the
temporary item pool with selected rows and writes the shop total, while script
mode 2 reads the script's total variable and leaves the pool to its caller.

The sixteen normal and swapped stat-contest opcodes pass levels 0..3 to
`OpJumpUnlessStatContest`. The same level selects different random ranges for
different stats, so `StatContestLevel` names the explicit opcode levels rather
than assigning one difficulty label across all stats.

## Actor modes

The PC-98 overlay function paired with Windows `RunObjectStep` references
eleven contiguous action labels. Their order agrees with the nine modes that
Windows names from behavior; labels three and eight are 防御行動 (defense action)
and 回復行動 (recovery action). They name `ACTOR_MODE_DEFEND` and
`ACTOR_MODE_RECOVER`. Windows `RunObjectStep` has no branch for either mode;
`MarkActorActionReady` gives both the defense pick role for party members.
The same label order identifies modes five through seven as 間合に一歩 (step into
range), 一歩近づく (step closer) and 周り込み (circle around). Their former
charge/pursue/sidestep names were less faithful to those labels.

## Windows view hotspots

The active Windows `Hotspot.kind` stores destination (1), NPC (2) or treasure
box (3). `RenderNPC` temporarily assigns 14 to an NPC on the party's cell
and 15 to one directly ahead to order nearby billboards, then changes both
to 2. These values form `UiHotspotKind` in the record's retail `u32` slot.
The older `SceneHotspotKind` also has 2 and 3, but its other values occur only
in the unused scene-hotspot path and are not identified by the Windows UI.

## Panel hit-test flag

`FindPanelRowAt` rejects a panel or row with flag bit `0x0800` before testing
its hotspot. The status screen sets this bit on unavailable commands and on
commands excluded in fixed-member mode; the field command panel sets it on
its status row. The observed behavior supports `PANEL_SKIP_HIT_TEST` in the
shared `PanelFlags` domain without assigning a visual style to the bit.

## Bag search groups

`FindBagItem` tests bit 0 of its group operand before scanning ordinary bag
slots 0..47 and bit 1 before scanning scenario slots 48..63. `CompactBagCore`
passes both bits. The operand is now `BagSearchGroup`, with the two storage
groups and their combined search named.

## Two-step menu and message states

`RunTextWindowState` repaints at sub-step 0 and closes at 1;
`RunMessageBoxState` repaints and waits for input at step 0, then closes at 1.
`RunDisplayChoice` and `RunQuitConfirm` each open their two-row menu at step
0 and poll it at step 1. Their independently stored game steps have separate
local enum domains.

## Script pool boost modes

`OpBoostPool` reads a mode after its pool selector and amount. Mode 0 keeps
the MP maximum used by this function, mode 1 doubles it, and mode 2 caps at
`0x7fff`. These form `ScriptPoolBoostMode`. `PoolFillMode` uses codes 1 and 2
for different limits. The pool selector
writes HP for 0 and MP for any nonzero value, so its nonzero authored range
stays open.

## Demon classes

ET0000 has six encrypted blocks. Its second block maps each of the 51 demon
races to a major class; the sixth block names all sixteen classes in order.
`GetRaceClass` reads the map, and `GetDemonClassName` indexes the name block
with the resulting class. `DemonClass` follows those names and types the class
accessors and class search while retaining their retail `i16` ABI. The observed
checks select 鬼族系 (8) for one inflicted condition, 邪霊系 (10) and 人系/魔人系
(12/13) for step upkeep, and 無生物 (14) for random fusion exclusion.
The fourth ET0000 block names 23 pantheons. Its indices occupy a byte in
each demon record; `GetDemonPantheon` supplies the value copied to field
objects, and `GetDemonPantheonName` uses it to read the same name table.
`DemonPantheon` types both storage sites and the accessor's return.
The fifth ET0000 block gives seven human titles (愚者, 異能者, 覚醒者, 超人,
導師, 神人, 神). `Character.title` selects that table for human status names;
field objects copy the title from their leader. `HumanTitle` types those
bytes and the name lookup's index.
The script's four blood-type labels are Ａ, Ｂ, ＡＢ and Ｏ in that order;
`Character.bloodType` indexes them and now has a `BloodType` domain. The
adjacent twelve sign-name buffers are empty and never written, so their
individual sign identities are not established by this table.

## Event-flag banks

The saved flag block has sixteen 256-bit banks. Source readers and resets
identify bank 0/1 as the two scenario banks, 2 as owned maps/programs, 4 as
opened boxes, 7 as timed item effects, 8/9 as level/area flags, 12/13 as
scratch banks, 14 as the current actor's flags, and 15 as system settings.
These named selectors now form `EventFlagBank`. `ResetEventFlags` also walks
unnamed banks, and a script operand can carry a negate bit with its bank, so
the dynamic bank parameters retain their numeric widths.
Both retail ET0018 tables contain 613 names, covering only banks 0, 1, 2,
4, 8, 9 and 14. They provide no names for banks 3, 5, 6, 10 or 11.

## Moon phases

`GameClock.moonPhase` is a saved byte cycling through 0..27. `AdvanceClock`
and `ApplyClockChanges` identify phase 0 as new moon and 14 as full moon; the
other phase numbers have no separate names in the current source. `MoonPhase`
types the saved byte while the 28-phase count and tick duration remain
numeric extents.

## Script scratch transfer

`OpPeekPokeScratch` reads a width operand and an operation operand. Operation
`-1` copies the scratch value to a long variable, masking it to one or two
bytes for widths 1 and 2. Operation `-2` copies the long variable into the
scratch byte, word or full value. Both widths 3 and 4 use the full 32-bit
value; width 3 is therefore named as a full-value alias. Nonnegative
operations select a script register instead of a scratch transfer, so the two
negative operations are named constants within a mixed operand rather than
declaring the whole operand to be a closed enum.

`OpSaveRestoreScreen` interprets its next operand as flags. Bit 0 allocates a
new screen save and bit 1 skips saving and restoring the draw state around
either operation. When bit 0 is clear, it restores from and frees the handle
in the script long variable. These are the local `ScriptScreenSaveFlags`.

## Script tick counter

The script tick counter has three states: -1 pauses it, 0 stops it, and 1
runs it. `TickCounter` increments only in the running state, and
`SetTickCountOn` resets the count only on the stopped-to-running transition.
Those values now form the local `TickCounterState` domain in `scriptvars.c`.

## World travel cell flags

`LoadWorldTravelCandidates` copies each terrain code's flags into a travel
score byte. Bit 0 marks a passable cell; `MarkReachableWorldTravelCells`
adds bit 1 after finding a route to it. The higher bits hold candidate
weights, so the score byte stays numeric while its two state bits form the
local `WorldTravelCellFlag` domain.

`HitTestWorldMap` returns zero when no marker colour is hit, bit 1 for a
marker colour, and bit 0 when the sampled surface belongs to the automap
preview layer. Its only nonzero results are 2 on the world map and 3 on the
preview. `PickWorldMapDestination` accepts a hit of 2 as the direct world
cell; a hit of 3 uses the preview's offset travel cell. These bits form
`WorldMapHitFlags`.

## Alignment classes

`AlignmentClass` returns the existing `AlignmentSide` values -1, 0 and 1;
`MoveAlignment` returns that class and `GetAlignmentAffinity` accepts the
same side. Their source signatures now mark the domain without changing the
retail `i16` ABI.

## Resistance outcomes

`GetActionResistance` returns zero when there is no target actor, and
`ApplyResistanceOutcome` sets `BATTLE_ACTION_IMMUNE` for that same result.
`ATTACK_RESIST_IMMUNE` now names zero beside the negative special outcomes;
positive resistance values remain damage rates.

`ApplySkillResistanceOutcome` reports a reflected follow-up (-1), suppressed
follow-up (0), or a target follow-up (1) from the shared action result. The
condition-skill caller uses all three; damage callers ignore the return.
`ResistanceFollowup` names this result. The attack helpers also use the
existing `ConditionId` names for Blind, Dance and Berserk.

## Restore amount codes

`ComputeRestoreAmount` returns zero immediately for amount code 0; this is
`RESTORE_AMOUNT_NONE` beside the already named quarter, half and full codes.

## Attack attribute columns

Retail object records P2029 (Urd) and P2042 (Heqet) store resistance bytes
`50,50,0,50,50,50,0,50,50,50` and `50,50,50,50,50,50,0,0,0,50`.
Their null columns align with the [Urd](https://megatenwiki.com/wiki/Urdr)
and [Heqet](https://www.megatenwiki.com/wiki/Heqet) resistance tables, whose
headings place Sword, Phys and Ruin at indices 0, 1 and 9. The disc's ET0004
records independently put Megido in column 0, Damu in column 1 and Shibaboo
in column 9, matching those skill labels. These are resistance-column names:
column 1 also occurs on 21 melee weapons and 21 ammunition records, while
column 0 occurs on 98 melee weapons and seven ammunition records in ET0001.

## Deferred identities

Unresolved identities and behavior-only names:

- The shared result word's other noncombat encodings. ET0004 kind-3 skills
  select tally slots 0, 1, 4, 5 and 7..14; the other named slots have direct
  reader evidence.
  Slot 5 is set by Tetrakarn and Counterattack and reflects attacks with
  the Sword resistance attribute. `CheckBattleProtection` directly reflects
  Physical-attribute attacks when slot 6 is set, so its enum name records
  that effect. `UseBattleTallySkill` could set the slot from an effect code of
  6, but no current ET0004 skill has that code. Its authored name remains
  unknown. The barrier-cleared message
  in MS00DF entry 3 uses the generic barrier label from MS7F00 entry 52, so it
  does not distinguish slots 5 and 6.
  MS00DD entry 5 dispatches base action results 0..10 to entries 10..19 and
  97; the messages and combat resolver identify miss, no effect, graze,
  success, critical, lethal, immune, reflect, HP/MP absorb and protection.
  The resistance bytes 251..255 map to reflected or absorbed damage. The
  remaining tally effects still need their script and record relationships decoded.
  The combat resolver identifies result-word tags 0x50, 0x70 and 0x80
  for HP, MP and experience draining skills.
  `ApplyRestoreEffect` returns 2 if a fatal condition remains without a
  revival attempt, 3 when it has a condition or pool change to report, and
  6 on its other completed paths. A condition-specific effect can restore
  HP yet return 6 when it had no condition to report. `RestoreResult` names
  these outcomes before they enter the shared result word, where the same
  numbers also name battle messages.
  Attribute 10 bypasses the resistance array with a fixed value of 50, so it
  has a behavior-based enum member. The current ET0004 skills and ET0001
  attack items do not use it.
- Skill kinds 9, 10 and 14 have no records in the current ET0004 skill table.
  The original PC-98 ET0004 also has 309 records and none of those kinds.
  Kinds 9 and 10 name their shared battle-tally clearing handler, with 10
  marked as the alternate dispatch code; their distinct authored identities
  remain unknown. Kind 14 now names its observed field-effect
  call followed by an unconditional success report, the same path used by
  kinds 12 and 13. Its authored category remains unknown.
- Target-area codes 1, 3..5, 6..7 and 17. The collector proves selected-only
  (0), line (2), visible-grid (6..7) and weapon-hit (0xff) behavior. Codes
  6 and 7 have distinct record meanings still unproven. The gun
  path uses 7; ET0001 also gives 7 to items 110, 111, 116 (kind 5) and
  140 (software), so it is not gun-specific.
  ET0001 gives code 4 to sixteen items: eleven stat incenses (56..66),
  three temporary enhancers (67..69), the Necronomicon (122) and a pearl
  (168). All sixteen also have target flags 2, target count 1 and range 0,
  so those fields cannot separate code 4's meaning. Its only code-2 item is
  the Core Shield (113). No item record uses codes 3, 5 or 17. The item
  records show code 4 is authored, despite its absence from ET0004, but
  they do not identify a distinct target geometry.
  In ET0004, code 3 has 34 mostly close-range attacks, code 5 has only two
  spear attacks, and code 17 has only six single-target remedy/revival skills.
  Code 1 has 140 records, including basic melee attacks, guns, elemental
  spells, restoration and field skills. Codes 1 and 3 both contain kind-1
  attacks with target flags 2 and range 1, so those fields do not separate
  the two area values. The PC-98 ET0004 assigns area 1 to 25 skills that
  Windows changed to area 6; it has no area-4 skill either.
  The records' own descriptions confirm these groupings: code-5 skills 294
  and 296 describe spear attacks; code-17 skills 108..113 cure poison,
  paralysis, petrification and curses or revive the fallen. Code-3 skill 198
  says it hits four enemies, while code-1 skill 169 says it panics all enemies,
  so neither code 1 nor 3 alone identifies a single-versus-group target.
  All six code-17 skills also have target flags 0x11, which select the roster
  picker in `RunPartyCommandInput`; that picker reads the flags, not the area.
  `CollectTargets` sends area 17 through the same default cell collector as
  areas 1, 3, 4 and 5. The original PC-98 dispatcher at raw `DDS98.EXE`
  offset 0x3496a makes the same split: code 2 takes the line path, codes 6
  and 7 take the visible-grid path, and the other codes take the cell path.
  Its cell collector at 0x34bd8 special-cases 0xff and 0 as Windows does.
  Their distinct selection meanings remain unproven.
- Field-effect code 0x1a is named for its observed `SpawnActorGroup` behavior,
  shared with Sabatoma (0x19). Neither version's 309 ET0004 skill records
  selects 0x1a, so its authored skill identity remains unknown.
  Code 0x23 returns `FIELD_EFFECT_DONE`
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
- All 63 disc cells with code 0x88 and all 19 with code 0x89 are treasure-box
  records. Both map to `CELL_EVENT_FADE_SCENE`; the dead legacy frame selector
  gives 0x88 the first open/closed pair and 0x89 the second. The Windows
  renderer uses the first pair for both, so the enum names describe the
  recorded frame distinction without claiming different Windows visuals.
- Code 0x4f occurs in 64 treasure-box records. The legacy selector gives it
  the fourth frame pair; the Windows renderer does not distinguish it from
  the first pair. Its `CellCode` name records the frame relation only.
- Codes 0x8b, 0x8c and 0x8f occur in area object lists. `CheckCellEvent`
  ignores their kind-10 result for object records, and the direct object-cell
  queries recognize only 0x8d and 0x8e. The analogous PC-98 object scans
  at raw `DDS98.EXE` offsets 0x3f4da and 0x3f536 also compare only 0x8d and
  0x8e. The Windows map has 97 code-0x8b records (85 on Shibuya level 2),
  twelve code-0x8c records (ten in Chuu), and nine code-0x8f records (in
  Ueno Shrine, Takamagahara and Chuu). The 85 Shibuya code-0x8b rows have
  zero flag pairs and the same three payload bytes (40, 6, 0xff); the ten
  Chuu code-0x8c rows and all nine code-0x8f rows have zero flag and payload
  bytes. Their distinct map roles remain
  unproven. The original PC-98 disc has 23 code-0x8c object records versus
  Windows' 12; the eleven extra records are on Shanshan City level 5.
  `CELL_UNUSED_FLOOR_PROPERTY` has a kind-10 table row but no cell record in
  either disc's area maps. `CELL_INERT` likewise has a kind-13 table row but
  no cell record in any of the 109 Windows or 100 PC-98 area maps.
  `RunCellEvent` has no kind-13 branch. The PC-98 `RunCellEvent` switch at
  raw `DDS98.EXE` offset 0x40b63
  dispatches through a 14-entry jump table; kinds 10 and 13 both jump to
  its return at 0x40de1. These table-only cell-code names and
  `CELL_EVENT_INERT` describe observed behavior; their authored purposes are
  unknown.
  `CopyExitAt`'s numeric kind 7 is the chute event kind.
- Field-object image codes -1 and 0..4 name the mirrored side, facing rows,
  acting row and reaction frame. The fifth frame of the disc's five-BMP actor
  images is a distinct reaction pose. `FlashHitObject` selects it after a pool
  change; the vanish animation and hidden state reuse it, with the lit bit
  selecting the animation lighting path.
- Wall kind 6 occurs on 360 sides across seven disc areas. The wall-stop table
  gives it geometry class 0 and movement class 3. `GetWallAt` treats it as
  absent when flooding visible cells, and `BuildRoomGeometry` draws no quad;
  `WALL_KIND_INVISIBLE_BARRIER` describes this invisible movement block.
- Wall kind 12 occurs on 32 sides across eleven disc areas. The wall-stop table
  gives it geometry class 3 and movement class 0. `BuildRoomGeometry` draws a
  quad for it, so `WALL_KIND_PASSABLE_WALL` describes its visible but passable
  behavior. Neither behavior name assigns an in-world identity.
  `WALL_STOP_SOLID` names class 3 for either selected mode: wall kind 6 has it
  only for movement, while kind 12 has it only for geometry. Classes 1 and 2
  both fold to the door-like map code and remain distinct numeric table values.
- The disc has 97 wall-kind-2 sides and 14 wall-kind-11 sides. Every one has a
  door record at the same cell and facing, with the same low-nibble kind. The
  kind-2 record bars a step while its flag is clear; `IsStepBarred` always
  skips kind 11. The `WallKind` names record this flag-controlled distinction.
  The seven wall kinds used by the disc are 0, 1, 2, 3, 6, 11 and 12; all
  seven now have enum members. Kind 3 has solid geometry and movement classes.
- Item kinds 5 and 6. ET0001 has fifteen kind-5 records spanning charms,
  incense, a shield, dummy items and apparent scenario items; its one kind-6
  record is the Necronomicon. The PC-98 records 107..122 are byte-identical
  to Windows, with the same kind assignments. `DecodeItemRecord` gives kind 5
  targeting and message fields but gives kind 6 neither; `ApplyItemEffect`
  sends both to handlers with the same no-effect outcome. The field item
  picker invokes a skill only for weapons and accessories, so kind 6's extra
  decoded bytes do not establish a skill-book role. The kind-5 descriptions
  promise distinct effects for talismans 107/108, incense 110/111/118 and
  the Core Shield 113. Items 115..117 and 121 are dummies or undescribed.
  Fourteen kind-5 records use MS00DE entry 53, a generic use message. The
  Guardian Set (114) instead uses entry 51, which tests and changes event
  flag bank 1, index 4 while displaying its uniform message. That script
  side effect does not identify the whole kind. The shared no-effect handler
  does not recover the other intended effects or either category name. The
  only two `OpListBagByCategory` calls in each version's scripts (MS000B/4
  and MS00D8/10) both request category 21, all priced non-scenario items;
  neither script selects kind 5 or 6 by its category number.
  Kind 0 is ET0001 item ID 0's zero-price empty record. It is the only
  kind-0 record among the 745 item entries, and the gun command rejects that
  kind. `ITEM_KIND_NONE` names it without assigning an identity to kinds 5 or 6.
