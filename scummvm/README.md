# ScummVM engine "ivanoff"

A ScummVM engine for *Magnus & Myggen 2: Den store skattejagt* (Ivanoff Interactive, 2002).
It is the way to play the game on devices that can't run Windows programs,
such as the Miyoo Mini.

## How it works

The game is an MFC program. Its logic is about 120 C++ classes, one per room or
scene. They talk to a small engine (in `mm2run.exe`, 0x476000–0x490000) through
about 40 functions: send a message, give a command to a child (sprite, sound,
hotspot), start a timer, set the cursor. See [`docs/ENGINE.md`](../docs/ENGINE.md).

The engine doesn't reimplement those 62,000 lines of game logic. Instead:

- `emu/` is a small x86 interpreter (integer instructions, string instructions and
  a little x87) with a sparse 32-bit address space. It runs the game-logic code
  of the **user's own** `mm2run.exe`, plus the MFC/C runtime helpers that code uses
  (CString, collections). No game code is in this repository.
- `machine.cpp` loads the executable, points every Win32 import at a trap, and
  patches the engine functions that the game logic calls with traps (UD2). The
  traps run native code: `world.cpp` (objects, messages, timers), `sprite.cpp`
  (bitmaps, FLC animations, hotspots, sounds, texts) and `ivanoff.cpp` (drawing,
  input, cursors). Variables and message structs stay in emulated memory; the
  original engine code for them is plain data handling and is interpreted.
- `machine_files.cpp` maps `C:\MM2\` to the game folder and `C:\MM2\appdata\`
  (settings and saves) to ScummVM save files.
- `enginefuncs.h` lists the engine functions' addresses and how many argument
  bytes each pops (numbers only, generated with capstone).

The interpreter was checked against unicorn on ~20,000 random executions of
real instructions from the executable (`tools/emutest/`).

## Building

Copy `engines/ivanoff` into a ScummVM source tree (tested with the master
branch of October 2026), then:

    ./configure --enable-engine=ivanoff
    make

## Game files

From the Nordic retail CD (Danish, Norwegian, Swedish and Finnish in one):

| File | From |
|---|---|
| `MM2.DAT`, `MM2.IDX` | the CD |
| `DK/`, `N/`, `S/`, `SF/` | the CD (language packs; one is enough) |
| `LAB/` | the CD (labyrinth videos) |
| `mm2run.exe` | an installation (`Program Files\Magnus2`), or the CD's `DATA1.CAB` (`unshield`) |

## Status

**[unverified]** marks behaviour inferred but not yet checked against the original.

- Works: startup, the Ivanoff logo, the intro cut-scene (animations, voices,
  per-frame triggers; Esc skips it), the title, the castle hall with its
  dialogue, walking between rooms, the labyrinth (Cinepak videos, opened only
  while playing), drag and drop (both of the original's modes), sprite
  cursors, timers, the settings file.
- Texts (subtitles, counters, save screen labels) use the Liberation fonts from
  ScummVM's `fonts.dat`, which have Arial's and Times New Roman's metrics; the
  hall's subtitle matches the original (under Wine) pixel for pixel.
- Saving and loading through the game's own screens works. The files are the
  original's (`SavedGameN.m2s`, `settings.dat`), stored as ScummVM save files
  named `<target>-<file>`; a save made by the original game under Wine loads,
  and the format is the same byte count and records.
- Not done yet: ScummVM's own save/load dialogs; playing the whole game through.
