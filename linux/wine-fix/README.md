# Labyrinth video fix for Wine / Proton

Without this fix, the two labyrinths (game rooms 40 and 42) show a black or
garbled picture under every Wine and Proton version, up to and including Wine
`master` as of October 2026. `install-mm2.sh` installs the fix by default.

## What's wrong

The labyrinth videos are 480×360 Cinepak AVIs, but the game doesn't play them through
AVIFile or MCI. It reads each frame itself through WinMM's `mmio` API and decodes it
with `ICDecompress` into an 8-bit palettised buffer. That path runs into two Wine bugs:

1. **`winmm`: `mmioSetBuffer()` loses the file position.**
   The game seeks to a frame that lies inside the already-loaded 8 KB buffer, then
   switches to a frame-sized buffer and calls `mmioAdvance()`. Wine starts the new
   buffer at the *disk* offset (the end of the old read-ahead) instead of the
   *logical* position, so every frame is read from the wrong place.
   Symptom in a `WINEDEBUG=+iccvid` log: `CVID: strip overflow (more than 32)`.
2. **`iccvid`: no 8-bit output.**
   `ICM_DECOMPRESS_BEGIN` accepts an 8-bit output format, but the decoder has no 8-bit
   path. It writes 24-bit pixels into the 8-bit buffer, so you get noise and memory
   corruption. Windows' Cinepak supports 8-bit output; Wine's own test suite already
   expects it (a `todo_wine` in `dlls/iccvid/tests/iccvid.c`).

## The fix

- `patches/master/`: the two fixes as separate commits against Wine `master`
  (the `mmio` one includes a new conformance test), meant for upstreaming.
- `patches/wine-10.0/`: the same fixes backported to Wine 10.0, which Proton 10 is
  based on. `winmm.dll` and `iccvid.dll` in this folder are built from it.
- `build.sh`: rebuilds both DLLs from `wine-10.0` plus the backport.

The DLLs are copied next to `mm2run.exe`, and only that program is told to prefer them
(registry: `HKCU\Software\Wine\AppDefaults\mm2run.exe\DllOverrides`, with
`winmm` and `iccvid` set to `native,builtin`). Nothing else in the prefix changes.

The DLLs have Wine's "builtin" marker cleared. Otherwise Wine ignores a Wine-built
DLL in the application folder and keeps using its own copy.

To do the same by hand (for example with Steam's launch options) instead of using the registry:

```
WINEDLLOVERRIDES="winmm,iccvid=n,b" %command%
```

## Verification

| Test | Stock | Patched |
|---|---|---|
| Wine `winmm` mmio tests incl. the new `test_mmioSetBuffer_position` (GE-Proton10-25) | 4 failures | 0 failures (384 tests) |
| Wine `master` iccvid tests (patched `master` build) | the 8-bit query is a `todo_wine` | 0 failures (54 tests), 8-bit query passes |
| Labyrinth 1 and 2 under GE-Proton10-25, fresh install via `install-mm2.sh` | black / garbled | video plays, correct colours |

| File | SHA-256 |
|---|---|
| `winmm.dll` | `09e0533eeb5f5e0339ed1c7f1dd9c5c33ca917cf0e29a1adaf196af123ea356d` |
| `iccvid.dll` | `10456be816215fd3f2a439e028fe85b5987d211ed6adbcfd11e46abe58c0574b` |

These are the exact output of `build.sh`. A rebuild is functionally the same, but
not bit-identical (the build ID and paths differ).

## Licence

These DLLs are built from Wine and are covered by the GNU LGPL 2.1 or later. The
corresponding source is Wine `wine-10.0`
(https://gitlab.winehq.org/wine/wine/-/tree/wine-10.0) plus the patch in
`patches/wine-10.0/`. `build.sh` reproduces the build.
