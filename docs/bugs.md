# Bugs and glitches

The known defects of the Windows release of 偽典・女神転生 (`DDS.EXE`), what
causes each, and what the [play build](play.md) does about it. The layout
follows pokecrystal's `docs/bugs_and_glitches.md`; the flags follow pret's
`BUGFIX`/`UBFIX` split.

Every entry has a class:

- **compat**: undefined behaviour, crashes, hangs, and differences between
  operating systems, drivers or Wine, including frame pacing. Fixed under
  `GITEN_COMPAT`.
- **bugfix**: a defect in the game's own logic (dropped input, soft-locks,
  data errors). Fixed under `GITEN_BUGFIX`, which implies `GITEN_COMPAT`.
- **QoL**: a change of behaviour that retail intends, however inconvenient
  (tempo, extra keys). Never under either flag; none is implemented.
- **unfixed**: a defect that is not fixed, with the reason.

The matching build defines neither flag and compiles the retail spelling,
which each fix keeps in its `#else` branch. The play build defines
`GITEN_BUGFIX`. A fix starts with a `// @bug` comment naming the trigger and
the retail consequence ([markers](comment-markers.md)).

**Save impact** says whether a fix changes what a save file holds or how an
old save loads. `SaveClock` writes the whole `GameClock`, pacing fields
included, but `LoadClock` restores only the days, the moon, the hour and the
minute, so changes to pacing are save-safe.

Source links are listed [at the end](#sources) and cited by tag, such as
[FAQ] and [XP].

## Contents

- [Platform and timing](#platform-and-timing)
  - [The game speed follows the display's refresh rate](#the-game-speed-follows-the-displays-refresh-rate)
  - [Battles run away on fast machines: Dantalion II and endless enemy turns](#battles-run-away-on-fast-machines-dantalion-ii-and-endless-enemy-turns)
  - [One click walks two steps](#one-click-walks-two-steps)
  - [Nested frame loops take no window messages](#nested-frame-loops-take-no-window-messages)
  - [The main loop spins while the game is inactive](#the-main-loop-spins-while-the-game-is-inactive)
  - [Magenta backgrounds, black outlines and blackouts on Windows 10 and 11](#magenta-backgrounds-black-outlines-and-blackouts-on-windows-10-and-11)
- [Text](#text)
  - [An empty or oversized glyph renders with stack garbage](#an-empty-or-oversized-glyph-renders-with-stack-garbage)
  - [A glyph top outside the row table writes out of bounds](#a-glyph-top-outside-the-row-table-writes-out-of-bounds)
  - [Characters are missing on Windows XP](#characters-are-missing-on-windows-xp)
  - [Battle messages vanish before they can be read](#battle-messages-vanish-before-they-can-be-read)
- [Input](#input)
  - [Most clicks on a party panel in battle do nothing](#most-clicks-on-a-party-panel-in-battle-do-nothing)
  - [A stick or arrow-key tap in battle is lost](#a-stick-or-arrow-key-tap-in-battle-is-lost)
  - [Keyboard controls: WASD, number keys, automap key](#keyboard-controls-wasd-number-keys-automap-key)
  - [The ally re-input interval](#the-ally-re-input-interval)
- [Items](#items)
  - [A scenario item with no room locks the game in the discard menu](#a-scenario-item-with-no-room-locks-the-game-in-the-discard-menu)
  - [The discard menu with nothing to discard](#the-discard-menu-with-nothing-to-discard)
  - [The discard menu freezes or appears with room left](#the-discard-menu-freezes-or-appears-with-room-left)
  - [Blank item entries](#blank-item-entries)
- [Play time](#play-time)
  - [Play time counts loop passes, not time](#play-time-counts-loop-passes-not-time)
- [Scenario and event flags](#scenario-and-event-flags)
- [PC-98 only](#pc-98-only)
- [Exploits](#exploits)
- [Sources](#sources)

## Platform and timing

### The game speed follows the display's refresh rate

**Class:** compat. **Fixed** under `GITEN_COMPAT`.

**Symptom:** the whole game (walking, turning, battle time, message timers)
runs faster on a display above 60 Hz, and as fast as the host allows under
Wine or a compositor whose vertical-blank wait returns at once.

**Root cause:** `WinMain` (`src/Platform/winmain.cpp`) runs one `StepGame`
per pass, and `RenderFrame` paces the pass only with
`WaitForVerticalBlank`. Every timer in the game counts passes. On a slow
machine a pass also outlasts the vertical blank, so the speed depends on the
CPU as well [DDS414].

**Fix:** `WaitForFrame`, called from `RenderFrame` before the vertical-blank
wait, holds the loop to 60 passes a second on `timeGetTime`, the rate of the
60 Hz displays of the time. The frame is still flipped or copied right after
the vertical-blank wait, so pacing adds no delay between the blank and the
present.

**Save impact:** none.

**Sources:** [DDS414], [WIN11], [RE].

### Battles run away on fast machines: Dantalion II and endless enemy turns

**Class:** compat for the unbounded speed (fixed by the pacing above); QoL for
a slower battle tempo (not implemented).

**Symptom:** enemies act so fast that the player can hardly enter a command.
The FAQ notes that Dantalion II became very strong in the Windows version, the
more so the faster the machine [FAQ]; the Re Patch describes enemy AI taking
"endless turns" [RE].

**Root cause:** battle time advances in ticks. `TickGameClock`
(`src/Game/clock.c`) passes a tick every `g_clock.framesPerTick` passes, 5 as
`InitClock` sets it, so battle time is a count of loop passes and inherits the
refresh-rate dependence above.

**Fix:** the 60 Hz pacing bounds it: 12 ticks a second, as on a 60 Hz display
in 1999. Machines of the time often rendered fewer than 60 passes a second,
and players suggest that about 24 comes closer to the original feel [WIN11],
so battles may still be faster than they were on period hardware. A
battle-only tempo scaler would change intended behaviour and is QoL. The XP
patch tool's "battle wait" and the Re Patch's "battle wait" (default 5,
recommended 7) both patch the `framesPerTick` byte in `InitClock` [XP], [RE].
That slows the in-game clock as well as battles.

**Save impact:** none. `framesPerTick` is saved but not loaded.

**Sources:** [FAQ], [XP], [RE], [WIN11].

### One click walks two steps

**Class:** compat. **Fixed** by the pacing above (most likely).

**Symptom:** one click on the navigation pad moves the party two cells. It
is reported on Windows 11 without DxWnd, and DxWnd set up as the blog
describes cures it [WIN11].

**Root cause:** while the left button is held on the pad, `HandleInput`
repeats the move (`RepeatPadMove`) as soon as the previous one has finished.
When the move animation runs uncapped it finishes before the button is let
go.

**Fix:** the 60 Hz pacing restores the animation's length. Not reproduced.

**Save impact:** none.

**Sources:** [WIN11].

### Nested frame loops take no window messages

**Class:** compat. **Fixed** under `GITEN_COMPAT`.

**Symptom:** in the item-discard screen the window can turn Not Responding,
and under DxWnd in window mode the screen was reported to stall for minutes
until DxWnd's hook settings were changed [WIN11]. Switching away from the game
in that screen leaves the input acquired and the cursor confined.

**Root cause:** window messages are removed only in `WinMain`'s loop.
`RunBagDiscardMenu` (`src/Game/itemrecord.c`) runs its own loop through
`WaitMenuFrame` and `RunFrame` (`src/Platform/winmain.cpp`), which renders
frames without ever calling `PeekMessage`. Windows NT ghosts a window that
takes no messages for five seconds, `WM_ACTIVATEAPP` is never handled, and
tools that work through the message queue stall.

**Fix:** `PumpMessages` removes and dispatches pending messages as `WinMain`
does (`WM_QUIT` exits, as there) and runs at the start of every `RunFrame`.
Its callers run outside any window procedure, so a dispatch never re-enters
one. It does not explain the freeze on paging (see
[below](#the-discard-menu-freezes-or-appears-with-room-left)).

**Save impact:** none.

**Sources:** [WIN11], [RE-BLOG].

### The main loop spins while the game is inactive

**Class:** compat. **Fixed** under `GITEN_COMPAT`.

**Symptom:** after switching away from the game it keeps one CPU core busy.

**Root cause:** `WinMain`'s message loop is
`while (PeekMessage(...) || !s_appActive)`. While the game is inactive and no
message is pending, it loops without waiting and passes the last message it
removed to `TranslateMessage` and `DispatchMessage` again on every pass.

**Fix:** the same `PumpMessages` replaces that loop. While the game is
inactive, it waits in `WaitMessage`.

**Save impact:** none.

**Sources:** none; found in the reconstruction.

### Magenta backgrounds, black outlines and blackouts on Windows 10 and 11

**Class:** compat. **Unfixed.**

**Symptom:** on current Windows the dungeon background is drawn magenta,
characters and monsters get black outlines (a colour-key failure), and the 3D
view blacks out, showing a frame only while moving [WIN11].

**Root cause:** not located. The community attributes these problems to
palette and colour-key handling and to flips to the primary surface under
DWM composition [WIN11].

**Fix:** none in the source. The community uses DxWnd and dgVoodoo2
[WIN11]. The play build runs under Wine inside gamescope ([play](play.md)).

**Save impact:** none.

**Sources:** [WIN11].

## Text

### An empty or oversized glyph renders with stack garbage

**Class:** compat. **Fixed** under `GITEN_COMPAT`.

**Symptom:** garbage in or below some glyphs, or a wrong glyph, depending on
the font.

**Root cause:** `RenderGlyph` (`src/Text/font.cpp`) passes 64 bytes of its
128-byte buffer to `GetGlyphOutline` and ignores the result. GDI fails for a
glyph with no outline (U+3000) or a bitmap larger than the buffer and leaves
`metrics` unset. The row lookup then indexes with stack garbage. The XP patch
tool's "font garbage" fix for symbols [XP] probably addresses this defect or
the next one; which one is unconfirmed.

**Fix:** pass the whole buffer and clear `metrics` on `GDI_ERROR`, which
renders a blank glyph.

**Save impact:** none.

**Sources:** [XP].

### A glyph top outside the row table writes out of bounds

**Class:** compat. **Fixed** under `GITEN_COMPAT`.

**Symptom:** with a font other than MS Gothic, glyphs land on the wrong rows,
or the rows are copied outside the glyph buffer.

**Root cause:** `RenderGlyph` indexes its 24-entry row-offset table with the
glyph's `gmptGlyphOrigin.y` unchecked.

**Fix:** clamp the top to the table.

**Save impact:** none.

**Sources:** none; found under Wine.

### Characters are missing on Windows XP

**Class:** compat. **Fixed** under `GITEN_COMPAT`.

**Symptom:** some characters, among them the long vowel mark ー, are missing
from the text on Windows XP [XP], [DDS414].

**Root cause:** `StepScript` (`src/Script/scriptctx.c`) treats a character
that `_ismbcprint` rejects as an opcode. For a double-byte character
`_ismbcprint` checks the C1 type bits from `GetStringTypeA`, which differ by
Windows version. On XP some printable characters have none of the bits in
its mask (`0x157`). `ExecScriptOpcode` has no case for them, so they are
dropped.

**Fix:** `IsScriptTextChar` treats a double-byte character as text when its
trail byte is a Shift-JIS trail byte, without asking the system. Single
bytes keep `_ismbcprint`, whose tables the CRT builds from the code page
alone. The CRT itself is matched library code and stays unchanged.

**Save impact:** none.

**Sources:** [XP], [DDS414].

### Battle messages vanish before they can be read

**Class:** QoL. **Not implemented.**

**Symptom:** battle declarations and results disappear too fast to read
[WIN11], [RE-BLOG].

**Root cause:** `TickMessageWindow` (`src/Ui/message.c`) closes the shared
message window after `s_messageLifetime` passes, 15 by default
(`SetMessageLifetime`): a quarter of a second at 60 passes a second. The
battle flow (`RunBattleAction` in `src/Game/skilluse.c`, `RunShotState` in
`src/Gfx/shot.c`) moves on at the same speed.

**Fix:** none. Reading time is a tempo choice. The Re Patch inserts delays
of several hundred milliseconds at three points of the battle flow, and
raises the lifetime's default [RE]. Its delays busy-wait inside the step,
so nothing renders and no input or message is handled meanwhile. Its author
describes the resulting lag as intended [RE-BLOG]. Those delays should not be
copied. A reading hold would need to keep the loop running.

**Save impact:** none.

**Sources:** [RE], [RE-BLOG], [WIN11].

## Input

### Most clicks on a party panel in battle do nothing

**Class:** bugfix. **Fixed** under `GITEN_BUGFIX`.

**Symptom:** in battle, clicking a party member's panel opens its command
panel only about one time in five. The XP patch tool's "battle click fix"
exists for it [XP].

**Root cause:** `ReleasePartyPanel` (`src/Text/font.cpp`) returns at once in
battle unless `GetTickElapsed()` is set, and `StepGame` sets it only on the one
pass in `framesPerTick` on which the game clock ticks. `HandleInput`
(`src/Platform/winmain.cpp`) calls it on the one pass the button is let go,
so the click is dropped unless that pass is a tick. The gate also refuses a
click while the command machine has stopped the clock:
`RunPartyCommandInput` (`src/Game/partyaction.c`) clears the tick while a
command is being entered. The XP tool removes the gate with a NOP, and its
readme reports the result: when an ally is picked as a target, the ally's own
command panel opens in the middle of the pick, and choosing another
ally-targeted action from it crashes the game or freezes the PC [XP]. Retail
can open that panel too, on a tick pass: the target is picked on the press,
and by the release the machine is idle again.

**Fix:** `HandleInput` records whether a command was being entered when the
button went down. A release in place in the battle view is kept only when
neither the press nor the release came during command entry. It is then held
until the next pass on which the clock ticks, where it goes through
`ReleasePartyPanel` and the retail gate. On every pass until then it is
re-checked against what let it be held and what the press needed: the battle
goes on, the 3D view is still shown, no command is being entered, the command
panel is not shown and no text plane is open. It is dropped when any fails,
or on a new press. Outside the 3D view a release goes to `ReleasePartyPanel`
at once, as in retail.

**Save impact:** none.

**Sources:** [XP].

### A stick or arrow-key tap in battle is lost

**Class:** bugfix. **Fixed** under `GITEN_BUGFIX`.

**Symptom:** in battle a short tap on the joystick, or on the arrow keys that
stand in for it, often neither moves nor turns the party.

**Root cause:** `HandleInput` runs `RunJoystickMove` in battle only on a pass
with `GetTickElapsed()` set, one pass in five. The stick is read as its
current state, so a tap let go before the next tick is never seen.

**Fix:** a direction turned away on a pass without a tick, while no command is
being entered, is held. On the next pass the gate lets through, it stands in
for the stick if the stick has been let go by then. It is dropped on a tick
pass that the gate refuses, and whenever the battle has ended, a command is
being entered, the command panel is shown, a layer is being dragged or the 3D
view is left, so a tap never carries over into the field.

**Save impact:** none.

**Sources:** none; the same gate as the panel clicks.

### Keyboard controls: WASD, number keys, automap key

**Class:** QoL. **Not implemented.**

**Symptom:** walking takes the arrow keys or the mouse; commands, targets
and summon positions take the mouse; the automap has no key.

**Root cause:** by design. `ReadJoystick` folds only the arrow keys, Return,
Space and Shift into the joystick bits.

**Fix:** none. The Re Patch adds WASD walking (and Q/E side steps), keys 1-6
for command panels, single-target spells and summon positions, and Space for
the automap [RE].

**Save impact:** none.

**Sources:** [RE], [RE-BLOG].

### The ally re-input interval

**Class:** QoL. **Not implemented.**

**Symptom:** the wait after an ally acts, before it takes its next command,
suits mouse input. With keyboard command entry it favours the player [RE].

**Root cause:** by design.

**Fix:** none. The Re Patch offers a longer wait ("balanced") and keeps the
retail one as "original" [RE].

**Save impact:** none.

**Sources:** [RE].

## Items

### A scenario item with no room locks the game in the discard menu

**Class:** bugfix. **Fixed** under `GITEN_BUGFIX`.

**Symptom:** after receiving a scenario item while 16 scenario entries are
held, the "delete item" menu opens, then opens again after every discard,
until no item is left to list. That last menu cannot be closed.

**Root cause:** scenario items live only in the bag's last 16 entries
(`AddBagItems` and `AddScenarioBagItems` in `src/Game/itemrecord.c`), and a
new one always takes an empty entry, even when the same item is already held.
When none is empty, `StoreBagItem` opens `RunBagDiscardMenu` and retries until
the item fits. The menu lists only entries that are neither priceless nor
scenario items, so no discard frees a scenario entry. The menu turns cancel
off, so once its list is empty it can never be left.
The item table has 18 scenario items (the 8 lover's parts, the 4 cyber limbs,
the Newton wreckage, the culture tank and more), and the FAQ reports parts that
cannot be removed [FAQ]. Whether normal play reaches 16 held at once is
unproven.

**Fix:** `StoreBagItem` does not open the menu for a scenario item and leaves
it unstored.

**Save impact:** none; a save made afterwards lacks the item that could not
be stored.

**Sources:** [FAQ].

### The discard menu with nothing to discard

**Class:** unfixed; not reproducible.

**Symptom (hypothetical):** a bag full of priceless items would open a discard
menu with no entries and no cancel, and the game would hang.

**Root cause:** `RunBagDiscardMenu` does not handle an empty list. Other than
the scenario case above, it opens only when the 48 normal entries are full,
and every one of them would have to hold a priceless item. The item table
has 22 priceless items besides the scenario items (keycards, the named
swords, the dummy armour and a few others). Filling 48 entries would take at
least 26 extra copies of the ten unstackable ones (the swords and the
armour).

**Fix:** none needed.

**Save impact:** none.

**Sources:** none.

### The discard menu freezes or appears with room left

**Class:** unfixed; not located.

**Symptom:** reported with and without the Re Patch: after a weapon is lost to
Disarm, or put down in a conversation, the discard menu appears although the
item list shows empty entries, and turning its page often freezes the game
[RE-BLOG]. Another player reports a crash when choosing what to discard
[RE-BLOG].

**Root cause:** not located. The menu opens when `AddBagItems` finds no room
in the 48 normal entries; the 16 scenario entries may show as free space.
The missing message pump (fixed [above](#nested-frame-loops-take-no-window-messages))
explains the stalls under DxWnd, but not a freeze on turning the page.

**Fix:** none yet.

**Save impact:** none.

**Sources:** [RE-BLOG], [WIN11].

### Blank item entries

**Class:** unfixed; not located.

**Symptom:** blank entries appear in the item list, for example after a
weapon is knocked away by Disarm while the list is full. The PC-98 version
had a community cleaner program; otherwise only save editing removes them
[FAQ], [GUIDE].

**Root cause:** not located.

**Fix:** none.

**Save impact:** the entries are saved.

**Sources:** [FAQ], [GUIDE].

## Play time

### Play time counts loop passes, not time

**Class:** unfixed; nothing shows it.

**Symptom:** none visible. The play time saved with the game is about 24
times the real time played at 60 passes a second.

**Root cause:** `StepGame` (`src/Game/gameloop.c`) calls
`AdvancePlayTime(state ? 31 : 24)` on every pass, and `AdvancePlayTime`
(`src/Game/playtime.c`) counts units of 16.7232 ms: 401 ms a pass, 518 ms on
a long pass. That matches no pass rate the Windows build runs at. In retail
it follows the refresh rate and the CPU like every other pass count. The
60 Hz pacing makes it deterministic but not correct.

**Fix:** none. `SavePlayTime` and `LoadPlayTime` are its only readers, and
no screen of the Windows build shows it, so a fix would change the saves'
contents for no visible effect.

**Save impact:** n/a.

**Sources:** none; found in the reconstruction.

## Scenario and event flags

Soft-locks and oddities in the event scripts and flags, collected by the
community. All are **unfixed**: none is located in the code, and most live in
the script data rather than the executable. Save impact: each is a state that
gets saved; the community fixes them by editing saves [FAQ], [GUIDE].

- **Item handout never ends** at the Hatsudai shelter or the Pentagramma
  forward base when the party order has gaps; reorder the party [FAQ].
- **Ochanomizu shelter:** saving inside it during the rescue event, then
  returning by Traport after the event, traps the party inside [FAQ].
- **Sanshirō rejoins** after parting at the Ueno temple when visiting
  Akihabara, and may then be sent to the Millennium hospital and stuck
  [FAQ].
- **Ariake bridge:** returning to Ariake by Traport after the colosseum bomb
  event removes the bridge (a person in the colosseum takes the party
  across). Depending on the event order the bridge is laid only once, so
  Yamato Takeru may stay in the party [FAQ]. After chasing Decarabia to the
  Baal branch and returning by Traport, Yamato Takeru opens peace talks before
  Takamagahara, and the bridge is gone [GUIDE].
- **Jukai never appears** at Gokokuji if the party returns by Traport after
  Baal Hadad in Chūu before parting with Jukai at the exit [FAQ].
- **Baal castle unreachable** after defeating Adonis, returning below and
  saving [FAQ].
- **Rui (Asuka Rui) joins suddenly** when the scenario expects her in the
  party and she is not [FAQ].
- **Leftover parts:** scenario parts stay in the bag (around Ochanomizu and
  the culture tank, and in other cases), and Dr. Kusaka's lines go wrong
  [FAQ]. Related to the [scenario-item entry](#a-scenario-item-with-no-room-locks-the-game-in-the-discard-menu).
- **Harajuku rescue:** healing a dying member at the medical facility before
  reporting to the administration blocks the report and the story [GUIDE].
- **Old Shinbashi station:** outside the new moon, stepping backwards toward
  Toranomon and returning makes the party always face Toranomon, a dead end
  [GUIDE].
- **Hatsudai escape:** entering the hospital room used for the rest starts
  the B8 purification event [GUIDE].
- **Pentagramma forward base:** walking back to the door after the "spy"
  line lets the party in early; harmless [GUIDE].
- **Sonoda:** after the ground-zero event, the dead Sonoda stops the party on
  the way to the Tochō [FAQ]. Entering another 3D area from the Hulk and
  returning by Traport at the start of that event keeps him in the party
  [GUIDE].
- **Fainting counter:** after about 256 revivals a character that faints at
  low HP stops fainting, and may start again later [FAQ]; an 8-bit counter
  is suspected, not located.

## PC-98 only

These concern the PC-98 release and are **unfixed** and not located in the
Windows code. `DDS98.EXE` is only a naming witness here.

- **Dark three-body fusion yields a level 99 demon** in the first production
  lot; a fixed version was once available from user support [FAQ].
- **Hayasaka's revival** was confirmed only on PC-98, with unknown conditions
  [FAQ].
- The Ariake bridge event order was adjusted in a later version [FAQ].
- **Blank item entries** had a PC-98 cleaner program (DLSPACE) [GUIDE].

## Exploits

Known tricks that are not treated as defects [URA]:

- Leaving a magic or item window open in battle stops time for the enemy
  until timed conditions such as charm wear off.
- Unequipping everything before the Hatsudai virtual trainer carries the
  trainer's equipment back out.
- Recarm-type spells heal members who are not dying.
- Stats lowered by -nda spells return on re-equipping a weapon or armour.

## Sources

- [FAQ] S.E.I., 『偽典・女神転生』FAQ (updated 2007-09-06):
  <http://www.se-inst.com/html32/lib/dds98f_m.html>
- [XP] 偽典・女神転生リメイク計画, 偽典・女神転生用XPパッチツール rev.0.2β
  (2006), Readme.txt: <http://dds414.fc2web.com/>, redistributed via
  [GUIDE].
- [DDS414] 偽典・女神転生リメイク計画, Windows version page:
  <http://dds414.fc2web.com/dds/dds9x.htm>
- [RE] グリコマーマン, Giten Megami Tensei Re Patch v1.0.24 (2026), README
  and CHANGELOG: <https://uu.getuploader.com/guriko_merman/download/8>
- [RE-BLOG] グリコマーマン, 【偽典・女神転生】修正パッチ配布 and its comments:
  <https://gurikomerman.blog.2nt.com/blog-entry-38.html>
- [WIN11] グリコマーマン, Windows 11 play setup (DLL and DxWnd) and its
  comments: <https://gurikomerman.blog.2nt.com/blog-entry-35.html>
- [GUIDE] グリコマーマン, XP patch, save editor and guide archive, with the
  bug reports credited to 蒼:
  <https://gurikomerman.blog.2nt.com/blog-entry-29.html>
- [URA] 偽典・女神転生 裏技･バグ技:
  <https://web.archive.org/web/20181105154402/http://www.geocities.jp/a_touya2/giten/giten_urabagu.htm>
