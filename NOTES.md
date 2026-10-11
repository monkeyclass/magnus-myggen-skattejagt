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
  - `.idx`: u32 count + 12-byte entries; `mm2.idx` = {id, offset, size}, language idx = {offset, size, id}.
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
