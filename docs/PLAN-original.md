# Plan: Magnus & Myggen 2 on ScummVM → OnionOS

Hand this file to Claude Code at the root of a working directory. Work phase by phase. **Stop and report at every "STOP" gate.**

## Context

- **Game:** *Magnus & Myggen 2: Den store skattejakten* (Ivanoff Interactive, 1997, Norwegian; also Swedish/Finnish releases). Original Danish title: *Den store skattejagt*. Source: https://archive.org/details/magnus-og-myggen-2 (one 566 MB `.ISO`).
- **Engine:** Macromedia Director. This is inferred, not verified. Game 1 of the series is in ScummVM's Director detection table as `skeeto1` ("Skipper & Skeeto: Tales from Paradise Park", comment "Magnus & Myggen"). Game 1 is Director 5, detected but not playable as of June 2025 (ScummVM bug #15896).
- **Goal:** The game is fully playable in ScummVM, the fixes are upstreamed, and it runs on a Miyoo Mini / Mini Plus running OnionOS.
- **Target hardware constraints:** ARMv7 Cortex-A7, about 128 MB RAM, screen 640×480 (Mini Plus / Mini v1–3) or 750×560 (Mini v4).

## Ground rules for Claude Code

- Never commit game data, ISO contents, or decompiled game scripts to any repo. Keep them in `./gamedata/` and `./analysis/`, and add both to `.gitignore`.
- Keep a running log in `NOTES.md` covering findings, blockers, decisions, and open questions. Mark every assumption as **[unverified]** until it has been checked.
- ScummVM changes stay in `engines/director/` unless a change elsewhere is clearly required. If it is, explain why before making it.
- Use small, focused commits. Each one fixes one blocker and follows ScummVM's code formatting conventions.
- Don't guess CLI flags or APIs. Check `--help`, the source, or the docs first.

---

## Phase 0: Environment

1. Install the build dependencies for ScummVM on Linux: SDL2, a compiler, zlib, libpng, freetype, libjpeg, libmad/vorbis/flac, `p7zip`, and `xxd`.
2. Clone `https://github.com/scummvm/scummvm`, then build a debug binary with only the Director engine:
   ```
   ./configure --disable-all-engines --enable-engine=director --enable-debug
   make -j$(nproc)
   ```
3. Get ProjectorRays (`https://github.com/ProjectorRays/ProjectorRays`). It is a Director decompiler. Build it, or use a release binary.
4. Get the game data. Use the ISO path the user provides. If none is given, download `https://archive.org/download/magnus-og-myggen-2/Magnus%20og%20Myggen%202.ISO`.
5. Extract the ISO into `./gamedata/mm2/`. Check whether it is a hybrid Mac/PC disc: run `7z l`, and look for an HFS partition. If it is hybrid, extract both sides into `pc/` and `mac/`.

**Done when:** `./scummvm --version` lists the Director engine, and the game files are on disk.

## Phase 1: Identify the game

1. Write the full file tree to `analysis/filelist.txt`. Include each file's size and its MD5 hashes: the full-file MD5 and the first-5000-bytes MD5, which is what ScummVM detection uses.
2. Locate the following:
   - the projector `.exe` (16-bit and 32-bit variants, if both exist)
   - movie files (`.dir`, `.dxr`, `.cxt`, `.cst`)
   - the `Xtras/` folder and any `.x32`/`.xtr` files
   - XObjects and DLLs
   - video files (`.avi`, `.mov`)
   - audio files
   - any `.ini` files
3. Determine the Director version from the file headers (`RIFX`/`XFIR` byte order, then the version fields) and from the projector's strings.
4. List every Xtra and XObject the game ships with or references. Compare that list against what exists in `engines/director/lingo/xlibs/`.
5. Decompile all movies with ProjectorRays into `analysis/lingo/`. If the projector embeds movies, extract those too.

**STOP:** Report the following:
- the Director version
- the platform(s) on the disc
- the Xtra/XObject list, marked as present or missing in ScummVM
- the size of the Lingo codebase (number of scripts, number of handlers)
- anything unusual

## Phase 2: Baseline run

1. Add the game with ScummVM's fallback detection, using the "add anyway" director fallback. Run it with verbose logging:
   ```
   ./scummvm -d 5 --debugflags=<check available director channels with --list-debugflags=director> --path=gamedata/mm2/pc <target>
   ```
2. Save the full log to `analysis/run-001.log`.
3. Record exactly where execution stops, plus every `STUB`, `unimplemented`, `warning`, and `error` line, deduplicated and counted.
4. If the user has game 1 (`skeeto1`), run the same baseline on it. The same blockers likely affect both games, so fixes can be validated against two titles.

**STOP:** Present a ranked blocker list (show-stoppers first), with a rough effort estimate for each item.

## Phase 3: Detection entry

1. Add a game ID (proposed: `skeeto2`; check for collisions first) to `directorGames[]`. Use the English series title that matches `skeeto1`, and add a comment giving the Norwegian title.
2. Add a `gameDescriptions[]` entry modelled on existing Director entries for the same version. It should use the projector plus one distinctive data file, `Common::NB_NOR`, and `kPlatformWindows`, with the correct Director version number.
3. Verify the game is detected without the fallback.

## Phase 4: Fix loop (the core work)

For each blocker, in priority order:

1. Reproduce it with a minimal command, and identify the Lingo handler involved from `analysis/lingo/`.
2. Classify it:
   - Lingo builtin or property
   - Xtra/XObject
   - score/sprite/ink
   - palette/transition
   - sound channel
   - digital video
   - cursor
   - MIAW
   - file I/O and saves
   - fonts/text encoding (Norwegian æøå in Mac/Win charsets)
3. Implement the fix in the engine, following the patterns of neighbouring code. Do not add game-specific hacks unless upstream precedent exists. If one is unavoidable, put it in the existing quirks mechanism.
4. Where feasible, add a Lingo test under the engine's existing test setup.
5. Re-run, update `NOTES.md`, and commit.

**STOP every ~5 fixes, or whenever a fix needs an architectural change:** report progress and wait for the user.

## Phase 5: Playthrough validation

1. Build a checklist of every scene/room and minigame from the decompiled scripts, plus save/load and quit.
2. Play through using ScummVM's debugger/console, jumping to movies directly where needed. Log visual or audio glitches separately from crashes.
3. Compare against the original behaviour. The reference can be a playthrough video, or running the original in 86Box/Win98 if the user sets that up.

**Done when:** the game can be completed start to finish with no crashes, and saves work.

## Phase 6: Upstream

1. Rebase onto current `master` and run ScummVM's formatting checks.
2. Open a PR (or a series of PRs, split as detection / engine fixes / Xtras). Write clear descriptions and include the MD5 data.
3. Update or open a ScummVM bug ticket listing the game's files and MD5s.

## Phase 7: OnionOS build

1. **[unverified]** Determine how Onion ships ScummVM: standalone port or RetroArch core, which version, and which engines are enabled. Read the Onion repo (`OnionUI/Onion`) to find out.
2. **[unverified]** ScummVM has a Miyoo Mini host target (check `configure` for `miyoomini`). Cross-compile with the Miyoo Mini toolchain (e.g. the `union-miyoomini-toolchain` Docker image), with the Director engine enabled, from the branch that contains our fixes.
3. Package the build so it replaces or sits alongside Onion's ScummVM without breaking other games. Document the install steps for the SD card.
4. Do a performance pass on the device. Measure frame pacing, RAM use, video playback, and audio stutter. If something is slow, profile it and consider engine-level optimizations or device-specific options (e.g. lower video quality).

**Done when:** the game launches from Onion's game list and is fully playable on the device. Install steps are written down in `INSTALL-ONION.md`.

---

## Open questions for the user

- Is the ISO hybrid Mac/PC? The Mac side is sometimes better supported by ScummVM.
- Do you also own game 1 (or other Magnus & Myggen titles), for cross-testing?
- Which device(s) exactly: Mini v2/v3, v4, or Plus?
- Upstream PR under your GitHub account, or local-only first?

## Kickoff prompt

> Read PLAN.md. Execute Phase 0 and Phase 1, keep NOTES.md updated, and stop at the Phase 1 STOP gate with your report. The ISO is at `<path>`.
