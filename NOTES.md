# NOTES — Magnus & Myggen 2 / Den store skattejagt → ScummVM → OnionOS

## Decisions
- 2026-10-10: User picked the **Danish "Superstater" release** instead of the Norwegian ISO named in PLAN.md
  (https://archive.org/details/den-store-skattejagt-superstater-version).
  Consequence: the detection entry should use `Common::DA_DNK`, not `NB_NOR` **[unverified until disc is inspected]**.

## Phase 0 status
- [x] Build deps installed (SDL2, zlib, png, freetype, jpeg, mad, vorbis, flac, p7zip, xxd, boost, mpg123, hfsutils).
- [x] ScummVM cloned at `13339950` (2026-10-10), configured `--disable-all-engines --enable-engine=director --enable-debug`.
      `./scummvm --version` → 2026.4.1git13339950; `--list-engines` shows `director`.
- [x] ProjectorRays built from `6f9bceb` (v0.2.1). Usage: `projectorrays decompile <in> [-o out] [--dump-scripts]`.
- [x] Game data: archive.org was allowed after a policy change. ISO `Den Store Skattejagt - Superstater version.iso`, 138,088,448 bytes,
      MD5 8571e0226918303cfad5a4d8b943daab (matches the archive.org metadata). Saved at `gamedata/iso/mm2_dk_superstarter.iso`.
- [x] The disc is **PC only**: plain ISO 9660 (volume "Magnus o…"), with no HFS partition. Extracted to `gamedata/mm2/disc/`.
      The game files are inside InstallShield cabinets (DATA1.HDR/CAB + DATA2.CAB). Unpacked with `unshield` 1.5.1 into `gamedata/mm2/cab/`.

## Findings (from ScummVM source)
- `skeeto1` = "Skipper & Skeeto: Tales from Paradise Park" (comment "Magnus & Myggen"). It already has Danish entries:
  - D4.04 `MAGNUS.EXE` + `MAGNUS0.DXR`, DA_DNK, labelled "Magnus & Myggen: Leg og Lær" (detection_tables.h:3310). This might be a different title filed under skeeto1 **[unverified]**.
  - D5.01 `START16.EXE`/`START32.EXE` + `MAGNUSGB.CXT`/`MAGNUS1.DXR` (detection_tables.h:8018-8025).
- `skeeto10` = "Skipper & Skeeto: Fun in the Park" (mm7main.exe, DA_DNK, D8.51).
- There is no `skeeto2` ID and nothing matching "skattej". The proposed ID is still free.

## Open questions
- Does the Superstater release differ from the original retail CD (budget re-release, maybe with a different projector)? **[unverified]**

## Phase 1 findings — **the plan's core assumption is false for this release**
- **This is not a Director game.** No RIFX/XFIR containers, no .dir/.dxr/.cxt/.cst, no Xtras, no Macromedia/Lingo strings in any EXE.
- `mm2run.exe` (868,352 B, MD5 65097221a47c267658657d9895b298e2) is a custom Win32 engine:
  PE i386, linker 6.0 (MSVC 6), built 2006-02-19. Imports GDI32/USER32/WINMM/MSVFW32 (Video for Windows → AVI),
  COMCTL32/comdlg32/SHELL32/ADVAPI32. Registry key `Software\IVANOFF Interactive\MM2`; saves to `settings.dat`.
  Version resource: "IVANOFF Interactive, Magnus & Myggen 2, DK".
- `mmsuper.exe` = "IVANOFF Super Starter" (a learning-program shell, also MSVC, no Director). `LAUNCHER.EXE` = the autorun menu.
- Data: two custom `.idx` + `.dat` packs (see analysis/resource-types.txt):
  - `mm2.dat` (shared DK/SE/NO, 80 MB, 1438 entries; idx = u32 count + {u32 id, u32 off, u32 len}): 639 BMP, 82 WAV, 326 small `.HSP`(?) records,
    and about 390 length-prefixed binary blobs (probably sprite/animation data, maybe compressed) **[unverified]**.
  - `mm2lng.dat` (Danish, 104 MB, 1300 entries; idx = {off, len, id}): 649 WAV (voice), 637 text strings, 1 BMP.
- The archive.org uploader notes that the game "runs reasonably on Windows 10 with occasional crashes".
- Unclear whether the 1997 original *Den store skattejagt* was Director and this 2006 Superstarter edition is a rewrite.
  The Norwegian ISO in PLAN.md (566 MB, 1997) may well be Director **[unverified]**.

## 2026-10-10 (cont.) — the Norwegian 1997 ISO and a new direction
- User (now away) asked me to continue on my own, and added a **second goal: run it under Proton on Linux**.
- **Norwegian ISO** (`magnus-og-myggen-2`, 593,975,296 B, MD5 7f0dbd5b…, matches archive.org) is **also not Director**. It has the
  same engine family: `mm2.exe` (1997-06-16, MSVC 4.2, a 206 KB launcher), plus `mm2.dat`/`mm2dk.dat` identical (MD5) to the 2006 release,
  plus `mm2n`/`mm2s`/`mm2sf` packs and 78 `Lab/*.avi` labyrinth videos (RIFF AVI).
  → **The series moved off Director by game 2. The ScummVM Director plan (PLAN.md phases 2–4) does not apply to this game.**
- **The Norwegian dump is damaged** (see analysis/no1997-damage.txt): `mm2inst.exe` (the 1997 engine is inside it), `UNWISE.EXE`, every `.idx`, and 99% of
  `mm2n.dat` are zero-filled sectors. `mm2.dat`, `mm2dk.dat` and `Lab/` are intact. Unusable as an install source.
- **2006 Superstarter = a timed trial ("prøvespil").** `mm2run.exe` has an "appman" trial manager:
  HKLM `appmanfile` → a file holding the start timestamp (keyed to the volume serial), HKCU `appmansetting`; when these are missing it says
  "This trial game has expired." The help text says play is limited by minutes and unlocked by buying a code online.
  **Decision: I won't bypass the trial.** The 2006 build is useful only for reverse-engineering reference, not as the Proton target.
- The InstallShield 6 setup of the 2006 disc hangs in Wine (IKernel.exe DCOM, a known issue). `unshield` extraction works.
- The 2006 exe expects `LabPath` (registry) for the labyrinth AVIs, but the 2006 disc doesn't ship them **[unverified: crash cause?]**.
- Found better dumps on archive.org:
  - `magnus-myggen-den-store-skattejagt`: `MM2NORD.bin/.cue` (682 MB), Nordic retail, "Copy Protection: None found".
  - `magnus-myggan-den-stora-skattjakten-sweden`: a redump.org-verified 7z (161 MB), Games4Kids re-release, "DRM free".
  - `skipper_skeeto-the_great_treasure_hunt_uk_covermount`: the English version (225 MB zip). Possibly useful for a ScummVM English variant.
- Tools: `tools/xref.py` (string → code xrefs), `tools/dis.py` (annotated disassembly with capstone).

## 2026-10-10 (cont.) — Linux/Proton track: WORKING (Wine 9; Proton itself not available here)
- **Use the Nordic retail CD** `magnus-myggen-den-store-skattejagt` (`MM2NORD.bin`, 681,889,488 B,
  MD5 dfa8c80a4cb58c2787d0ca2ae45702ec, Aaru dump). It has 4 languages (DK/N/S/SF), all 78 Lab AVIs and
  InstallShield cabinets with a **2002 build of mm2run.exe** (839,680 B, MD5 ea69e5c57c774fb24beaf176885156a2).
  **That build has no trial/appman code**, though it still has a "Please insert" string (the CD isn't needed after a full install).
- Swedish redump (Games4Kids 2006, MD5 of 7z e9d0f7ff…) ships the **2006** exe (same as the DK trial). Its installer writes
  `appmanfile` → `mmmsys.dat` (64 B), which unlocks it as a full version. It also works under Wine after a real install.
- Wine findings:
  1. InstallShield 6 hangs when RpcSs isn't running → `wine net start rpcss` first.
  2. The SE 2006 setup refuses "Windows NT" → `winecfg -v winxp`.
  3. **Black screen** without a virtual desktop: the engine paints on the desktop DC; its own X window stays 1×1.
     → Wine virtual desktop 640×480 (`HKCU\Software\Wine\Explorer\Desktops\Default=640x480`).
  4. It **hangs on speech** with no audio device (ALSA fails) → needs PulseAudio/PipeWire (tested with a pulse null sink).
  5. Needs window focus for clicks.
- Registry the game needs (2002 build): `HKLM\Software\IVANOFF Interactive\mm2` with `LabPath`, `Language`, `ProgramPath`,
  `Resource file`, `Resource local file`. In a 64-bit prefix → write with `/reg:32` (Wow6432Node).
- Saves: `C:\ProgramData\IVANOFF\MM2\2.0\SavedGame<N>.m2s` (+ `settings.dat`).
- Deliverables: `linux/install-mm2.sh` (scripted install for Wine or Proton, no InstallShield),
  `linux/INSTALL-LINUX.md`, `linux/bin2iso.py`.
- Verified: intro, navigation, DK speech+subtitles, menu, save, restart, load, fresh win64 prefix with no CD.
- **Not verified:** real Proton (GE-Proton binaries can't be downloaded in this sandbox, since GitHub releases are blocked),
  the labyrinth, minigames, a full playthrough.

## ScummVM / Miyoo track — what this means
- **No Director.** A ScummVM port means a **new engine** (proposed name `ivanoff` or `magnus`), reimplementing `mm2run.exe`.
  The game logic is compiled C++ (MFC 4.2), not scripted, so scene logic must be reverse-engineered from the exe.
  RE reference: the 2002 build (no DRM code).
- Data formats decoded so far (tools/mm2pack.py):
  - `.idx`: 12-byte {id, offset, size} records. `mm2.idx` starts with a u32 count (1439), which is also where the language packs' ids start; `mm2lng.idx` has no count. (Corrected: an earlier note read the language index 4 bytes off, as {offset, size, id}.)
  - `mm2.dat` (1438 entries): 639 Windows BMP (8-bit), 82 WAV, 326 `.HSP` hotspot tables, about 390 **Autodesk FLC** animations
    (magic 0xAF12; scenes 560×420/640×420, 8-bit; chunks SETPAL/BRUN/COPY/SS2, all supported by `video/flic_decoder.cpp`).
  - `.HSP`: ".HSP", u32 count, then per hotspot {u32 id, u32 type (1 = rect via 2 points, 2 = polygon), u32 npts, npts×(u32 x, u32 y)} **[layout inferred]**.
  - `mm2lng.dat`: text (cp1252) at id N, voice WAV probably at N+1 **[unverified pairing]**.
  - Lab AVIs: Cinepak 480×360, no audio (ScummVM has `image/codecs/cinepak.cpp`).
  - Screen: 640×480 (640×420 scene + 60 px UI bar), so it fits the Miyoo 640×480 1:1.
- Rough size: a point-and-click with dozens of rooms, puzzles and minigames, hard-coded in C++, probably 3–6 weeks of RE + engine work.

## 2026-10-11 — Proton verified
- GE-Proton10-25 (sha512 checked) downloaded from the GitHub release URL. The `releases/latest` page is blocked in the sandbox,
  but `releases/download/<tag>/<asset>` works. GE-Proton11-x tags exist, but `GE-ProtonX-Y.tar.gz` assets under that name return 404.
- `linux/install-mm2.sh --proton` → registry lands in `pfx/system.reg` under `Wow6432Node`; virtual desktop set in `user.reg`.
- Verified under Proton: intro, navigation, save slot 1, wineserver kill + relaunch, load slot 1. Saves are in
  `pfx/drive_c/ProgramData/IVANOFF/MM2/2.0/`.
- Still not verified (Wine or Proton): the labyrinth (Cinepak AVIs), minigames, a full playthrough, and the Steam shortcut flow.

## 2026-10-11 — Labyrinth: two Wine bugs found and fixed
- **Reaching it:** the room objects are created in one setup routine (around 0x43f000–0x443000 in the 2002 exe), each with
  `push <room id>` and a vtable. The labyrinth handlers (the code that reads `LabPath`) sit in the vtables of rooms
  **40** and **42** (Lab1 / Lab3). Room IDs run from 10 to 128 (hall = 36, portrait gallery = 49).
- **Save format (.m2s), inferred:** a 12-byte header, then records `{u16 type, u16 id, u32 len, data}`. The current room is
  in records (17,1) and (82,53). `settings.dat` lists `SavedGame<N>=<thumbnail room>` and `SavedGameTime<N>`.
  Editing the room in a save (`tools/m2s_setroom.py`) and loading it jumps straight there. Verified with 49 → gallery, 40/42 → labyrinth.
- **Symptom:** labyrinth video area black (Wine 9 / GE-Proton10-25 stock).
- **Cause 1 — winmm `mmioSetBuffer()`:** the game reads frames with mmioSeek + mmioSetBuffer(frame buf) + mmioAdvance.
  When it seeks inside the loaded 8 KB buffer, Wine leaves the disk offset at the end of the read-ahead, and `MMIO_SetBuffer`
  sets `lBufOffset = lDiskOffset`, so every frame is read 8 KB too far. Still present in Wine master (2026-10-09).
  Fix: seek the IO proc to the logical position before switching buffers. New test `test_mmioSetBuffer_position`:
  4 failures with stock winmm, 0 with the fix. **[expected Windows behaviour inferred from the game working there; the test is not yet run on Windows]**
- **Cause 2 — iccvid 8-bit output:** the game decodes into 8 bpp. Wine's DecompressQuery rejects 8 bpp, but DecompressBegin
  accepts it and decode_cinepak has no 8-bit path, so it writes 24-bit pixels into the 8-bit buffer (noise and heap corruption).
  Fix: decode at 32 bpp into a private frame, then map to the output palette with a cached RGB555 → index table.
  Wine's own test already expected 8-bit Query == ICERR_OK on Windows (it was a `todo_wine`); it passes now.
- `ICM_DECOMPRESS_GET_PALETTE` (0x401e) is still unimplemented in Wine (FIXME). The game copes; colours look right with its own palette.
- Delivery: `linux/wine-fix/` (patches for master and 10.0, prebuilt stripped DLLs with the builtin marker cleared, `build.sh`).
  `install-mm2.sh` copies them next to mm2run.exe and sets AppDefaults\mm2run.exe\DllOverrides winmm, iccvid = native,builtin.
- Upstream TODO: send both master patches to wine-devel (GitLab MR at gitlab.winehq.org). Wine requires a real-name author and
  that the new test passes on Windows (testbot).

## 2026-10-11 — ScummVM engine "ivanoff" (Miyoo Mini target)
- **Approach:** the game logic (about 120 classes, 0x401000–0x476000) is not re-implemented. A small x86 interpreter
  (`scummvm/engines/ivanoff/emu/`) runs it from the user's `mm2run.exe`, together with the MFC/C runtime code it uses.
  About 40 engine functions (messages, children, timers, cursor, save/load) are patched with UD2 traps and implemented
  natively; every Win32 import is a trap too. Unimplemented engine code or Win32 calls stop with their name, which is how
  the gaps were found. The interpreter was checked against unicorn on ~20,000 real instructions (`tools/emutest/`).
- **Cost:** the game logic runs under 100k x86 instructions per second. The screen is redrawn only when a sprite changes.
- **Verified in ScummVM (headless runs with frame dumps):** intro (skippable with Esc), title, hall, walking between rooms,
  dialogue with voices and subtitles (the hall subtitle matches the Wine screenshot pixel for pixel), the menu, the save
  and load screens, saving and loading (same `.m2s` format; a Wine save loads), the labyrinth videos and navigation, drag
  start/hover of the tool bar items, walking characters.
- **Mistakes found on the way:** the language index has no count and stores `{id, offset, size}` like `mm2.idx`; reading it
  4 bytes off had shifted every language resource by one (`tools/mm2pack.py` fixed). Sound loop counts were inverted.
  `CREATE_NEW` must fail on an existing file, otherwise `settings.dat` gets emptied at start.
- **Miyoo Mini:** `miyoo/build.sh` (toolchain v0.0.3 in /opt), `miyoo/prepare-game.sh`, Onion app in `miyoo/app/`.
  The ARM build runs under `qemu-arm` with the toolchain's libraries. **[unverified on a device]**
- **[unverified]** text box mode 2 = right-aligned; image field +0x20 (command 0x18); command 9's p3.

## 2026-10-11 — Native rewrite of the game logic: started
- **Goal:** replace the interpreted game logic with native C++ class by class, without a playthrough by a person.
  Guide: `docs/PORTING.md`.
- **Hybrid:** objects, variables and children are still created by the original setup code; a ported class gets a native
  `Script` for its slots 5/6 (`accepts`/`handle`), chosen by vtable. Variables stay in the interpreter's memory and are
  used through `VarRef` (their methods by vtable slot), so random/cycler/shuffle variables behave exactly as before.
- **Checking without playing:** `virtual_clock=<ms>` makes runs reproducible (sounds end after their computed length), so
  the `-d4` message/command trace of a run with the native logic must equal the trace with the original code. Inputs:
  room saves + random clicks (`crawl`), random walks, and a message fuzzer (`fuzz`, `fuzz_vars`) that sends each class
  the message numbers and constants found in its handler and sets its variables. `coverage=<file>` reports the executed
  original instructions. Fuzzing reaches about 50% of the handlers' instructions; clicking alone much less.
  A deliberate off-by-one in a ported item showed up in 7 of 8 walks.
- **Ghidra:** declaring `this` explicitly in the variable vtable prototypes fixed decompiled calls that had lost their
  arguments (MSVC interleaves the pushes of two calls). `get` never takes arguments.
- **Setup code:** all but three (`0x426c40`, `0x42b0c0`, `0x4575f0`) of the ~230 createVars/createChildren functions are
  straight-line constructor calls, so the native setup can be generated from the state after setup. **[plan]**
- **2026-10-12: all message handlers are native.** All 118 classes (119 objects) run native `accepts`/`handle`
  (`scripts/base.cpp`, `items.cpp`, `batch_a..h.cpp`), ported by 8 parallel agents from the typed decompilation, each
  checked by targeted fuzzing in its rooms. Final check with everything native: 190 runs (rooms, walks, fuzzing)
  identical to the original, plus 126 extra fuzz runs for rooms 52/57/59.
  Fuzzing coverage per handler varies (2-99% before extra seeds; most 40-90%); uncovered paths were reviewed
  against the disassembly only. **[unverified by play]** Weakest: rooms 57 (32%), 30 (46%), 36 (43%), 59 (48%).
- **Hybrid artefacts to remove in phase 2:** native handlers hold a 0x2c heap block and keep temporaries on the
  interpreter stack so that heap addresses (which appear in commands, e.g. frame-trigger messages) match the
  original; strings and settings (`StdString`, `GameSettings` in batch a) live in the interpreter's heap.
- **Phase 2 (next):** native variables (same save format), native setup (createVars/createChildren generated from
  the state after the original setup), then the interpreter can go.
- **2026-10-12: phase 2, native variables and setup.** The seven variable classes are native (`vars.h`, with the
  original's save records and MFC CMap iteration order); original code still reaches them through trap shims.
  The objects, variables and 2,762 children are created by `scripts/setup.cpp`, generated by
  `tools/port/gensetup.py` from a recording (`record_setup=`) of the original setup (`native_setup=false` runs the
  original). The settings class is native (INI `C:\MM2\appdata\settings.dat`). Check: 189/190 runs identical; the
  one difference is a fuzz run where the original runs out of interpreter heap (its leaks) and stops. Saves made
  through the menu are byte-identical to the original's (rooms 22, 36).
- **2026-10-12: the interpreter is gone.** The engine is native: no x86 interpreter, no traps, no emulated memory.
  The scripts use native variables, strings, frame-trigger messages (`Message::trigger`) and text commands
  (`Message::text`); the globals are a native table with mm2run.exe's initial values; the files, INI and
  registry values are `files.cpp`. `mm2run.exe` is still read for its cursors. Check against the original
  (interpreted) code's traces: 189/190 runs identical (the other: the original runs out of heap while fuzzing);
  menu saves byte-identical. The checking tools still need a build of an earlier version (git history) to make
  new baselines.
- The in-game volume slider now sets the mixer volume of the game's sounds (DAT_004c6940 was written but unused).
- **2026-10-12: ScummVM integration.** Save/load through ScummVM (launcher, in-game menu, autosave): a ScummVM
  save holds the original's records, and loading runs the original load screen's room switch (object 14,
  `kMsgEngineLoad`); allowed when the original allows F5/F6. Keymapper (Esc, arrows, space, F5/F6, joystick).
- **Bug found:** keyboard input passed ScummVM key codes for arrows and F-keys, which the game logic (Windows
  virtual key codes) doesn't know: the arrow-key games and F5/F6 couldn't work. The deterministic checks never
  used the keyboard (only clicks and messages), so they didn't catch it. **[unverified by play]**
- **2026-10-12: more fuzzing.** The message fuzzing now also starts in the 20 rooms it skipped (25, 26, 28, 30-33,
  37, 39, 41, 42, 45, 50, 51, 54, 56-59, 62): 120 new runs, all identical to the original. Handler coverage of the
  original code by the checks: 66% (was about 50%). The check is now 310 runs (309 identical; the other is the
  original's heap exhaustion). Keyboard input is checked only as fuzzed key messages, not through the key mapping.
- Longer fuzzing (4000 messages, with and without variable fuzzing) in the weakest rooms (26, 29, 33, 34, 43, 46,
  57, 59): 48 runs, all identical; coverage 67%. Fuzzing is saturated: the rest of these handlers needs game state
  that only play reaches (the climbing game 46 gets its arrow-key messages but stays at 27%).
- **2026-10-12: memory leak found and fixed.** Desktop RSS grew ~5 MB/s in rooms with looping animations (25, 28):
  `Video::FlicDecoder` collects a dirty rect per decoded frame until `clearDirtyRects()`, which `Anim` never
  called. Found with valgrind massif. Also, animation resources over 256 KB are now streamed from the .dat
  (`SafeSeekableSubReadStream`, main thread only; sounds stay in memory because the mixer thread reads them). Peak
  RSS over a 600 s walk: 53 MB (was 136 MB and growing). The Miyoo Mini has 128 MB; v0.2.0 would have run out.
- **2026-10-13: first device test (Miyoo Mini, v0.2.1): the pointer couldn't be moved**; doors opened and closed and the
  clock ran. Cause: the game keymap bound the arrow actions to `JOY_UP`… (the Miyoo's d-pad) and was named
  `ivanoff-default`, so the Miyoo backend's "clear the arrows of `engine-default`" didn't apply and the d-pad sent
  arrow keys to the game (which steps through hotspots) instead of moving the virtual mouse **[unverified: fix not
  yet tried on the device]**. Fix: keymap id `engine-default`, no joystick defaults for the arrows.
- **2026-10-13: animations that should stand still were looping.** Play flag `0x40000000` (with `0x80000000` and a
  start frame) was read as "loop"; the original (FUN_004817f0 sets anim +0xd0, FUN_00484290 then stops the player
  at the first frame) shows the start frame and stops. Visible as the clock gauge of the inventory bar (object 12,
  child 0x55b) running all the time and doors opening and closing (`0xc0003fff` closed, `0xffff3fff` open), reported
  from the device. The trace checks didn't catch it: the baselines ran on this engine's sprite code too.
  Checked with frame dumps: idle in the hall, the old build changes the clock area every frame, the new one nothing.
- **2026-10-13: doors didn't open (device report: "clicking a door hangs").** Reproduced on desktop by playing through
  a new control channel (`debug_control=<file>`: click/pclick/key/shot/animdump commands appended while the game
  runs in real time) and compared with the original exe under Wine (Xvfb + xdotool + a PulseAudio null sink; without
  a sound device the original never finishes its first line and ignores input). Original: first click opens the
  door (frame 0 → 8), second goes through. Cause: the animation player. From FUN_00484290 / FUN_0047fc70:
  - flag `0x40000000` (+0xd0) pauses the sprite (slot +0x60, FUN_0047b430) on its start frame; a resume plays on;
  - at the end frame the sprite is paused (resumable), and the owner notified;
  - a negative end (+0xd4) stops the sprite at the end (message 0x8002 → child command 4), it isn't "reverse".
  The door's resume did nothing because the held door wasn't "paused". Fixed; checked visually (door frames
  matched pixel-for-pixel against the original's screenshots, then the room change to the library).
- **Use-palette returns the previous palette's owner** (FUN_0047f4f0, engine +0x58: (child << 16) | object). The game
  logic keeps it to restore the palette when the quit dialog, save screen, options and inventory close; ours returned
  0, which left the dialog's palette on the room (seen after Esc → "Nej").
- **Animation loop count** = the constructor's parameter (like sounds: 0 = forever, 1 = once). Without an explicit
  end frame (or with end 0x3fff) the player wraps to frame 0 until the count is reached (FUN_00489290,
  FUN_00478060), and only then notifies and pauses. Ours played everything once, so idle loops froze (e.g. the
  "forgotten moment" coins of object 82). Checked against the original's screenshots: same coin positions over time.
- **Engine audit against the decompilation** (all child commands; 13 findings) and fixes:
  - frame player as FUN_00484290/FUN_00489290: per tick, the frame's triggers fire, then the step; an end E stops on
    frame E (end 0: one step past the last frame); 0x3fff/none loops up to the count; done is notified in the tick
    the end is reached; the hold pauses on the first tick; play keeps the last end/hide unless p2 bit 31; the loop
    count lives with the frame player (reset at load); goto-frame and resume fire no triggers;
  - the done notification stays set for later ends (until a play without one), so resumed animations notify again;
  - set-cursor (FUN_00477a90): no-cursor clears the flags, a cursor sets 0x10 (hoverable), the cursor changes at
    once when the mouse is over the child; set-flags (FUN_00477830) gives input children cursor 0x80000003; the
    flags are no longer masked (get-flags returns 0x10 too).
  Not yet done (low): z ≤ 0 ignored by set-z, move-by z, scroll wrap/result, TextResource fallback, hotspot no-ops.
- **Room sweep against the original:** for 35 rooms, the same save loaded in the original (Wine) and in ours,
  12 screenshots each: static difference < 1.4% everywhere (animation phase, subtitles excluded), same animated
  areas in all but room 55 (Magnus doesn't move while talking in ours) and timing artefacts (52, 34).
  Tools: /tmp-only scripts, see docs/PORTING.md "Checking against the original".
- **Sprite slot +0x78 (FUN_00481c30) sets the origin, not an initial position.** The frame player draws at
  position − origin (FUN_00489ad6 / FUN_00489570); hotspots ignore it (FUN_00478990). Ours set the position, so the
  22 sprites with an origin (e.g. the planets of room 22 after opening the chest) were drawn in the wrong place.
  Found with a click sweep against the original (`tools/port/play/clicks.sh`: same save, every clickable target in
  turn, screenshots compared).
