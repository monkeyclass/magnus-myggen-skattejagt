# Magnus & Myggen 2 on the Miyoo Mini (Onion OS)

The game runs in ScummVM with this project's `ivanoff` engine
([`scummvm/`](../scummvm/README.md)), built for the Miyoo Mini and packaged as an
Onion OS app. You need the **Nordic retail CD (2002)**, or an image of it.

## 1. Get the app

Download `MagnusMyggen2-OnionOS.zip` from the project's
[releases](https://github.com/monkeyclass/magnus-myggen-skattejagt/releases). It is built by
GitHub Actions (`.github/workflows/miyoo.yml`); each push also leaves the zip as a workflow
artifact. Or build it yourself on Linux:

1. Install the Miyoo Mini toolchain (1.5 GB unpacked):

        wget https://github.com/shauninman/miyoomini-toolchain-buildroot/releases/download/v0.0.3/miyoomini-toolchain.tar.xz
        sudo tar -C /opt -xf miyoomini-toolchain.tar.xz

2. Get the ScummVM source and build:

        git clone https://github.com/scummvm/scummvm.git
        miyoo/build.sh scummvm dist

   This makes `dist/App/MagnusMyggen2` (about 28 MB).

## 2. Add the game files

`prepare-game.sh` copies the needed files from the CD (or an `.iso`) and unpacks
`mm2run.exe` from the CD's installer cabinet. It needs `unshield` (and `7z` for an
`.iso`):

    miyoo/prepare-game.sh /media/cdrom dist/App/MagnusMyggen2/game DK

The last argument is the language: `DK`, `N`, `S` or `SF`. Only that language's
folder is copied (about 240 MB in all). For another language than Danish, also
write its code in `dist/App/MagnusMyggen2/language.txt`: `nb`, `sv` or `fi`.

By hand: copy `MM2.DAT`, `MM2.IDX`, the language folder (`DK/` …) and `LAB/` from
the CD into `game/`, and `mm2run.exe` from an installed copy
(`C:\Program Files\Magnus2`).

## 3. Copy to the SD card

Copy `dist/App/MagnusMyggen2` to `App/` on the SD card. It appears in Onion's
**Apps** list as *Magnus & Myggen 2*.

## Controls

| Button | |
|---|---|
| D-pad | move the pointer (with R: slowly) |
| A | click |
| B | right click |
| Y | Esc: skip a cut-scene |
| Start | ScummVM menu (quit, options) |

These are ScummVM's defaults on the Miyoo Mini and can be changed in its menu.

## Saving

Use the game's own save screen. Saves and settings go to `saves/` in the app
folder. `log.txt` there has the log of the last run.

## Status

**[unverified]** Tested with the ARM build under `qemu-arm` (same binary, same
libraries from the toolchain), not yet on a device. The game logic needs little
CPU (under 100k emulated instructions per second); the screen is only redrawn
when something changes.
