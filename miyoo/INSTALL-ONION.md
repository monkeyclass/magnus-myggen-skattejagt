# Magnus & Myggen 2 on the Miyoo Mini (Onion OS)

The game runs in ScummVM with this project's `ivanoff` engine
([`scummvm/`](../scummvm/README.md)), built for the Miyoo Mini and packaged as an
Onion OS app. You need the **Nordic retail CD (2002)**, or an image of it.

The CD image is on archive.org:
<https://archive.org/details/magnus-myggen-den-store-skattejagt>: `MM2NORD.bin` (681,889,488 bytes,
MD5 `dfa8c80a4cb58c2787d0ca2ae45702ec`) and `MM2NORD.cue`. It has all four languages and no copy
protection. Convert it to an `.iso` for `prepare-game.sh`:

    python3 linux/bin2iso.py MM2NORD.bin MM2NORD.iso

(or `bchunk MM2NORD.bin MM2NORD.cue MM2NORD`). Don't use the 2006 "Superstarter" disc: it is
a time-limited trial.

## 1. Get the app

Download `MagnusMyggen2-OnionOS.zip` from the project's
[releases](https://github.com/monkeyclass/magnus-myggen-skattejagt/releases). It is built by
GitHub Actions (`.github/workflows/miyoo.yml`); each push also leaves the zip as a workflow
artifact. To publish a new release: Actions → *Miyoo Mini build* → *Run workflow*, with a tag
name such as `v0.2.0`. Or build it yourself on Linux:

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

    miyoo/prepare-game.sh MM2NORD.iso dist/App/MagnusMyggen2/game DK

(or a mounted CD, e.g. `/media/cdrom`, instead of the `.iso`). With the release zip,
unzip it first and use its `App/MagnusMyggen2/game` as the game folder.

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
| X | Esc: the game's menu, skip a cut-scene |
| Y | space |
| Start | ScummVM menu (save, load, options, quit) |

The game's keys (Esc, arrows, space, F5 load, F6 save) can be remapped in ScummVM's
options (Keymaps).

## Saving

Use the game's own save screen, or save and load from ScummVM's menu (Start); both
work where the original allows saving. Saves and settings go to `saves/` in the app
folder. `log.txt` there has the log of the last run.

## Status

**[unverified]** Not yet tested on a device. The game logic is native C++; it was checked
against the original game's code with automatic runs, not by playing it through.
v0.2.1 fixes a memory leak of v0.2.0 that would have run the Miyoo out of memory.
