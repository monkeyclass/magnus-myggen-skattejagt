# Magnus & Myggen: Den store skattejagt — porting notes

Research and tooling for running *Magnus & Myggen 2: Den store skattejagt*
(Ivanoff Interactive, 1997; known in English as *Skipper & Skeeto: The Great Treasure Hunt*)
on modern systems.

This repository contains **no game data**. You need your own copy of the CD.

## Status

| Target | Status |
|---|---|
| Linux, Wine 9 | **Works.** Intro, navigation, Danish speech and subtitles, saving and loading verified. See [`linux/INSTALL-LINUX.md`](linux/INSTALL-LINUX.md). |
| Linux, Proton | **Works.** Tested with GE-Proton10-25 via `install-mm2.sh --proton`: intro, navigation, saving and loading, and both labyrinths. |
| Labyrinth videos under Wine/Proton | **Fixed here.** Two Wine bugs; the patched DLLs and patches are in [`linux/wine-fix/`](linux/wine-fix/README.md) and get installed automatically. |
| ScummVM engine `ivanoff` | **Works up to the labyrinths** (intro, rooms, dialogue with subtitles, drag and drop, labyrinth videos). It interprets the game logic of your `mm2run.exe` and implements the engine natively; see [`scummvm/README.md`](scummvm/README.md). Not yet played through. |
| Miyoo Mini (Onion OS) | **Packaged, not yet tried on a device.** The ARM build runs under `qemu-arm`. See [`miyoo/INSTALL-ONION.md`](miyoo/INSTALL-ONION.md). |

## Which disc

- **Use:** the Nordic retail CD (DK/N/S/SF on one disc, 2002 build of `mm2run.exe`, no copy protection).
- **Avoid:** the 2006 "Superstarter" release, which is a time-limited trial build.
- The archive.org dump of the Norwegian 1997 CD (`magnus-og-myggen-2`) is damaged: the installer, the index files and the Norwegian speech are zero-filled.

## Contents

- `scummvm/engines/ivanoff/`: the ScummVM engine (x86 interpreter for the game logic, native engine).
- `miyoo/`: build and packaging for the Miyoo Mini (`build.sh`, `prepare-game.sh`, the Onion app files).
- `tools/emutest/`: compares the engine's x86 interpreter with unicorn.

- `linux/install-mm2.sh`: installs the game from the CD or an ISO into a Wine or Proton prefix, without the InstallShield setup.
- `linux/bin2iso.py`: converts a MODE1/2352 `.bin` to `.iso`.
- `linux/wine-fix/`: the labyrinth fix: Wine patches (for `master` and for 10.0), prebuilt DLLs, and a build script.
- `tools/mm2pack.py`: lists and extracts entries from the game's `.idx`/`.dat` resource packs.
- `tools/xref.py`, `tools/dis.py`: small capstone/pefile helpers used for reverse-engineering `mm2run.exe`.
- `NOTES.md`: the full research log (findings, decisions, open questions).
- `docs/PLAN-original.md`: the original plan, which assumed a Director game.
