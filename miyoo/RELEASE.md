Onion OS app for the **Miyoo Mini**: ScummVM with this project's `ivanoff` engine for
*Magnus & Myggen 2: Den store skattejagt*.

**No game files are included.** You need the Nordic retail CD (2002). Unzip to the root
of the SD card, then put `MM2.DAT`, `MM2.IDX`, the language folder (`DK/`, `N/`, `S/` or
`SF/`), `LAB/` and `mm2run.exe` into `App/MagnusMyggen2/game/`. `miyoo/prepare-game.sh`
does this from the CD or an .iso. Full instructions: [`miyoo/INSTALL-ONION.md`](https://github.com/monkeyclass/magnus-myggen-skattejagt/blob/main/miyoo/INSTALL-ONION.md).

Status: tested as the same ARM build under `qemu-arm` (intro, rooms, dialogue with
subtitles, saving and loading, the labyrinths), not yet on a device. The game has not been
played through.
