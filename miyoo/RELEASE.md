Onion OS app for the **Miyoo Mini**: ScummVM with this project's `ivanoff` engine for
*Magnus & Myggen 2: Den store skattejagt*.

**No game files are included.** You need the Nordic retail CD (2002). Unzip to the root
of the SD card, then put `MM2.DAT`, `MM2.IDX`, the language folder (`DK/`, `N/`, `S/` or
`SF/`), `LAB/` and `mm2run.exe` into `App/MagnusMyggen2/game/`. `miyoo/prepare-game.sh`
does this from the CD or an .iso. Full instructions: [`miyoo/INSTALL-ONION.md`](https://github.com/monkeyclass/magnus-myggen-skattejagt/blob/main/miyoo/INSTALL-ONION.md).

Status (v0.2.6): carrying items works like the original: Esc with an item in hand no longer
opens the quit dialog (which got you stuck), an item that misses its target goes back without
leaving its picture behind, the target under the pointer updates without moving it, and the
hand cursors have no black background. The game no longer hangs when there is no sound output.

v0.2.5: sprites with an origin are drawn in the right place (e.g. the planets after
opening the chest in the star room), and the remaining small differences found by the audit
against the original code are fixed. A click on every clickable thing in every room was
compared with the original (screenshots): no differences left besides timing.

v0.2.4: doors open and you can leave the hall again; the found "forgotten moments"
animate; dialogs and the inventory restore the room's colours when they close; cursors over
clickable things update at once. These come from comparing the engine with the original game
running under Wine (same save, same clicks, screenshots), room by room, and from an audit of
the animation player and the sprite commands against the original code.

v0.2.3: animations that should stand still no longer loop: the clock of the
inventory bar now only changes when you change rooms, and doors stay open or closed (a play
flag meaning "show the first frame and stop" was read as "loop").

v0.2.2: fixes the controls on the device. In v0.2.0/v0.2.1 the d-pad sent arrow
keys to the game instead of moving the pointer (the game then stepped through the room's
hotspots: doors opening and closing, the clock running). The game's arrow keys (for the
arrow-key games) now have no default button; set them in Start → Options → Keymaps if needed.

v0.2.1: fixes a memory leak of v0.2.0 that would run the Miyoo out of memory
after a minute or so in rooms with looping animations (an FLC decoder kept every frame's
changed areas), and lowers peak memory by reading full-screen animations from the data file
as they play: about 53 MB on desktop over a long automatic walk (was 136 MB and growing).

v0.2.0: the game logic is native C++ now (no x86 interpreter), so it should run
much faster than v0.1.0. It was checked against the original code with automatic runs, not
by playing it through, and not yet on a device. New: saving and loading from ScummVM's
menu, button mapping (A click, B right click, X menu/skip, arrows, Y space), the arrow-key
games and the volume slider work.
