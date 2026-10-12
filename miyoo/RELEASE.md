Onion OS app for the **Miyoo Mini**: ScummVM with this project's `ivanoff` engine for
*Magnus & Myggen 2: Den store skattejagt*.

**No game files are included.** You need the Nordic retail CD (2002). Unzip to the root
of the SD card, then put `MM2.DAT`, `MM2.IDX`, the language folder (`DK/`, `N/`, `S/` or
`SF/`), `LAB/` and `mm2run.exe` into `App/MagnusMyggen2/game/`. `miyoo/prepare-game.sh`
does this from the CD or an .iso. Full instructions: [`miyoo/INSTALL-ONION.md`](https://github.com/monkeyclass/magnus-myggen-skattejagt/blob/main/miyoo/INSTALL-ONION.md).

Status (v0.2.1): fixes a memory leak of v0.2.0 that would run the Miyoo out of memory
after a minute or so in rooms with looping animations (an FLC decoder kept every frame's
changed areas), and lowers peak memory by reading full-screen animations from the data file
as they play: about 53 MB on desktop over a long automatic walk (was 136 MB and growing).

v0.2.0: the game logic is native C++ now (no x86 interpreter), so it should run
much faster than v0.1.0. It was checked against the original code with automatic runs, not
by playing it through, and not yet on a device. New: saving and loading from ScummVM's
menu, button mapping (A click, B right click, X menu/skip, arrows, Y space), the arrow-key
games and the volume slider work.
