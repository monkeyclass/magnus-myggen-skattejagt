# Magnus & Myggen: Den store skattejagt — porting notes

Research and tooling for running *Magnus & Myggen 2: Den store skattejagt*
(Ivanoff Interactive, 1997; known in English as *Skipper & Skeeto: The Great Treasure Hunt*)
on modern systems.

This repository contains **no game data**. You need your own copy of the CD.

## Status

| Target | Status |
|---|---|
| Linux, Wine 9 | **Works.** Intro, navigation, Danish speech and subtitles, saving and loading verified. See [`linux/INSTALL-LINUX.md`](linux/INSTALL-LINUX.md). |
| Linux, Proton | **Works.** Tested with GE-Proton10-25 via `install-mm2.sh --proton`: intro, navigation, saving and loading. |
| ScummVM → Miyoo Mini (OnionOS) | Not started. The game is **not** Macromedia Director (unlike the first game in the series), so it needs a new ScummVM engine. Data formats are mostly decoded; see [`NOTES.md`](NOTES.md). |

## Which disc

- **Use:** the Nordic retail CD (DK/N/S/SF on one disc, 2002 build of `mm2run.exe`, no copy protection).
- **Avoid:** the 2006 "Superstarter" release, which is a time-limited trial build.
- The archive.org dump of the Norwegian 1997 CD (`magnus-og-myggen-2`) is damaged: the installer, the index files and the Norwegian speech are zero-filled.

## Contents

- `linux/install-mm2.sh`: installs the game from the CD or an ISO into a Wine or Proton prefix, without the InstallShield setup.
- `linux/bin2iso.py`: converts a MODE1/2352 `.bin` to `.iso`.
- `tools/mm2pack.py`: lists and extracts entries from the game's `.idx`/`.dat` resource packs.
- `tools/xref.py`, `tools/dis.py`: small capstone/pefile helpers used for reverse-engineering `mm2run.exe`.
- `NOTES.md`: the full research log (findings, decisions, open questions).
- `docs/PLAN-original.md`: the original plan, which assumed a Director game.
