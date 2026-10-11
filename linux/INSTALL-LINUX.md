# Magnus & Myggen: Den store skattejagt on Linux (Wine / Proton)

This covers the **Nordic retail CD** (Danish/Norwegian/Swedish/Finnish on one disc;
archive.org item `magnus-myggen-den-store-skattejagt`, `MM2NORD.bin/.cue`).
It has no copy protection, and the game doesn't need the CD once installed.

Don't use the 2006 "Superstarter" disc. It's a time-limited trial build and stops
with "This trial game has expired" unless it's activated through Ivanoff's launcher.

## What you need

- The CD, or an image of it. A `.bin/.cue` can be converted to `.iso` with
  `bchunk`, or with `bin2iso.py` in this folder (`python3 bin2iso.py MM2NORD.bin MM2NORD.iso`).
- `unshield` and `p7zip` (`7z`): `sudo apt install unshield p7zip-full`
- Wine 9 or newer, **or** a Proton build (Steam's Proton or GE-Proton).

## Install with plain Wine (tested)

```sh
./install-mm2.sh MM2NORD.iso            # Danish (default)
./install-mm2.sh MM2NORD.iso --lang N   # Norwegian; S = Swedish, SF = Finland-Swedish
```

Then start it with:

```sh
WINEPREFIX=~/.local/share/mm2/prefix wine ~/.local/share/mm2/prefix/drive_c/Program\ Files/Magnus2/mm2run.exe
```

The script:

1. unpacks `mm2run.exe` (2002 build) from `DATA1.CAB` with `unshield`,
2. copies `MM2.DAT/IDX`, the language pack and the 78 labyrinth videos (`LAB/`) from the disc,
3. writes the five registry values the original installer writes
   (`HKLM\Software\IVANOFF Interactive\mm2`: `LabPath`, `Language`, `ProgramPath`,
   `Resource file`, `Resource local file`), and
4. turns on a **640×480 Wine virtual desktop** for the prefix.

You don't need to run the original InstallShield setup. If you'd rather use it, it
works under Wine after `wine net start rpcss` and with the Windows version set to XP
(`winecfg -v winxp`).

## Install with Proton (not tested here)

Proton is Wine underneath. The same script works with Proton's own `proton` launcher:

```sh
./install-mm2.sh MM2NORD.iso \
    --proton ~/.steam/steam/steamapps/common/"Proton - Experimental"/proton \
    --prefix ~/.local/share/mm2/proton
```

It prints the exact launch command when it's done.

To start it from Steam:

1. Steam → *Games* → *Add a Non-Steam Game…* → pick
   `~/.local/share/mm2/proton/pfx/drive_c/Program Files/Magnus2/mm2run.exe`.
2. In the shortcut's *Properties → Compatibility*, tick *Force the use of a specific
   Steam Play compatibility tool* and choose the Proton you used above.
3. Launch it once. Steam creates its own prefix under
   `~/.steam/steam/steamapps/compatdata/<number>/`, where `<number>` is the newest folder.
   Re-run the script with `--prefix` set to that folder, so the registry values and the
   virtual desktop land in the prefix Steam actually uses.

## Known issues and fixes

| Symptom | Cause | Fix |
|---|---|---|
| Black screen, game audio plays | The game draws on the Windows desktop, not its own window | Use the virtual desktop. The script sets this up; or run `wine explorer /desktop=mm2,640x480 mm2run.exe` |
| Game freezes on the first spoken line | No working audio output; it waits for speech to finish | Make sure PulseAudio/PipeWire is running. Under plain ALSA with no device, the game hangs |
| Clicks are ignored | Game window doesn't have focus, or a line is still being spoken | Click once into the window; wait for the subtitle to finish |
| InstallShield sits at "forbereder…" forever | Wine's RPC service didn't start | `wine net start rpcss`, then run `SETUP.EXE` again |
| "…kan inte köras under Windows NT" (Swedish 2006 setup) | Old OS check in the installer | `winecfg -v winxp` before running setup |

## Where things are stored

- Saves: `C:\ProgramData\IVANOFF\MM2\2.0\SavedGame<N>.m2s`
  (i.e. `<prefix>/drive_c/ProgramData/IVANOFF/MM2/2.0/`), plus `settings.dat` next to them.
- Labyrinth videos: Cinepak AVI, 480×360. Wine's built-in `iccvid.dll` decodes them.

## What was verified (Wine 9.0, Xvfb, PulseAudio null sink)

- Intro FLC animations, Danish speech and subtitles (æ/ø/å render correctly)
- Navigation (hall → portrait gallery)
- Options menu (candle icon), save to slot 1, quit, restart, load slot 1
- Fresh 64-bit prefix, Wine's default Windows 10 mode, **no CD drive mapped**

Not verified yet: the labyrinth sequences, the minigames, or a complete playthrough.
