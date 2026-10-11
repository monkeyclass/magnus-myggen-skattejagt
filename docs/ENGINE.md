# Ivanoff "MM2" engine: reverse-engineering notes

Source: `mm2run.exe`, 2002 build (839,680 B, MD5 `ea69e5c57c774fb24beaf176885156a2`), from
the Nordic retail CD. Decompiled with Ghidra 12.1.4 headless (`tools/ghidra/`). Addresses
below are virtual addresses in that exe. Everything here is **inferred from the decompilation**
unless marked otherwise.

## Code layout

| Range | Content | Size (decompiled lines) |
|---|---|---|
| 0x401000–0x46ffff | game logic: one class per object/room (about 120) | ~62,000 |
| 0x476000–0x47dfff | engine core: objects, messages, variables, containers | ~8,000 |
| 0x47e000–0x47ffff | view/input: cursor, hit-testing, drag & drop, timers | ~1,800 |
| 0x47f000–0x480fff | audio: waveOut playback/mixing | ~2,000 |
| 0x480000–0x484fff | files, resource packs, AVI reading via mmio | ~5,200 |
| 0x485000–0x48afff | graphics: palette, DIB sections, compositing, text | ~6,200 |
| 0x48b000–0x490fff | video: Cinepak (ICDecompress), FLC | ~6,500 |
| 0x490000– | MFC 4.2 and the MSVC 6 runtime (statically linked) | |

The game is an MFC application. Only a small layer talks to Windows: GDI (palette, DIB
section, `StretchDIBits`, `BitBlt`, text), WinMM (`waveOut*`, `mmio*`), VfW (`ICDecompress`),
`SetCursor`, `GetTickCount`, `PostMessage`.

## Objects

Every game entity is an object with a 16-bit **ID**. Objects live in a global map keyed by ID.

`FUN_0043f760` is the setup routine: it creates all top-level objects (119 of them) in order. For each one it:

1. allocates the object,
2. calls a constructor with the object's ID,
3. stores the class vtable,
4. calls vtable slot 3 (`+0x0c`), which creates the object's variables,
5. calls slot 4 (`+0x10`), which creates its child objects,
6. registers the object (`FUN_0047bb40`).

Top-level object classes (vtables, 8 slots):

| Slot | Meaning |
|---|---|
| 0 | `GetRuntimeClass` (MFC) |
| 1 | destructor |
| 3 (`+0x0c`) | **create variables** (`FUN_00476ce0` and friends) |
| 4 (`+0x10`) | **create children** (sprites, sounds, hotspots) |
| 5 (`+0x14`) | "accepts message?" |
| 6 (`+0x18`) | **message handler**: the room/game logic |

Base constructors: `FUN_00476910` (plain object) and `FUN_00441de0` (room; derived).
`docs/rooms.txt` lists every object's ID, vtable and the addresses and sizes of slots 3–6.

Known IDs:

| ID | Role |
|---|---|
| 0 | engine kernel: command 4 = switch to room `p1`; `0x1a` = ? |
| 17 (`0x11`) | game state: midnight clock (msg `0x14` adds minutes, 60 → room 62), navigation messages (msg 4) |
| 18 (`0x12`) | speech/text: msg `0x2d` with `p1 = (voiceId << 16) \| textId` says a line |
| 36 | castle hall (start room) |
| 40, 42 | labyrinths 1 and 3 |
| 49 | portrait gallery |
| 10–128 | rooms and sub-scenes; 242–244, 300–302, 353–360, 788–796, 1148–1657 are other objects |

### Variables

Objects own variables, which are child objects with an ID that are created in slot 3:

| Function | Kind |
|---|---|
| `FUN_00476ce0(obj, id, value)` | integer (vtable `0x4b5450`) |
| `FUN_00476d70(obj, id, CString)` | string (vtable `0x4b54bc`) |
| `FUN_00476e20`, `FUN_00476eb0`, `FUN_00476f40`, `FUN_00476fd0` | ranged/cyclic variants (`0x47cfa0`, `0x47d180`, `0x47d4f0`, `0x47d850`) |

Access:

- `FUN_00477050(obj, id)` returns a variable of the same object; `FUN_00477060(obj, id, otherObjId)` returns one of another object.
- Variable vtable `+0x38` = get value, `+0x2c` = set value, `+0x30` = increment.

## Messages

A message is a 0x2c-byte struct built by `FUN_0047c3c0`:

| Offset | Field |
|---|---|
| +0x04 | target object ID (u16; 0xffff = self) |
| +0x08 | delivery mode: 0 send, 1 timer/heap, 2 child-command |
| +0x0c | child command number (0 = not a child command) |
| +0x10 | message number (u16), or the target **child ID** for child commands |
| +0x14 | "handled" flag |
| +0x18, +0x1c, +0x20 | parameters p1, p2, p3 |
| +0x24 | return value |

Sending:

- `FUN_00476ad0(this, target, msg, p1, p2, p3)` sends synchronously to an object's slot 6. It returns `+0x24`.
- `FUN_00476a60(...)` posts asynchronously, via `PostMessage(hwnd, WM_APP+5)`.
- `FUN_00476b40(this, target, cmd, childId, p1, p2, p3)` sends a **child command**: to child `childId` of
  object `target`, or with `childId` 0 to all children, or with `childId` `0xfdff`–`0xfe03` to all children of that type.
- `FUN_00476bc0` / `FUN_00476c50` build heap messages, used with timers:
  `FUN_00477140(this, timerId, ms, msg)` delivers `msg` after `ms` milliseconds, and
  `FUN_00477170(this, timerId)` cancels it. Timers are a sorted list checked against `GetTickCount` (`0x47ead0`–`0x47eb90`).

Events the engine sends to objects (message numbers):

| Msg | Meaning (inferred) |
|---|---|
| 5000 (`0x1388`) | click on child `p1` (resource ID of the sprite/hotspot) |
| `0x3b`, `0x3c` | hover / right-click-ish queries; `0x3c` usually says a line |
| `0x1393` | may this object be dragged? |
| `0x1396` | drop test: dragged `(objId<<16\|ownerId)` over target `p2` |
| `0x1397`–`0x139b` | drag start / end / drop / click-without-drag |
| `0x139c` | which object is being dragged? |

## Child objects (sprites)

Children are created in slot 4 and added with `FUN_004770a0`. They are addressed by their
**resource ID**, which is also the ID of the data in `mm2.dat`.

| Constructor | Class (type code at +0x28) | Resource |
|---|---|---|
| `FUN_00477b20(id, rate, flags, ?)` | bitmap (`0xfe01`, vt `0x4b55a8`) | BMP |
| `FUN_00478140(id, rate, flags, ?)` | animation (`0xfdff`, vt `0x4b56f8`) | FLC |
| `FUN_004786a0(id, flags)` | hotspot (`0xfe03`, vt `0x4b5848`) | `.HSP` |
| `FUN_00478e40(id, ?)` | sound (`0xfe00`, vt `0x4b5900`) | WAV |

`FUN_00477a90(child, cursorId)` sets the cursor shown while hovering (`0x80000000` = none;
`0x80000003`, `0x80000006`, … are cursor IDs).

Object base (`FUN_00477560`), fields:

| Offset | Field |
|---|---|
| +4 | ID |
| +0x20 | 1 |
| +0x28 | type |
| +0x30 | flags (`&0x7ff`; `0x80` = draggable, `0x100`/`0x300` = drop behaviour) |
| +0x34 | cursor |
| +0x44 | CString |

Sprite base (`FUN_004812d0`) adds:

| Offset | Field |
|---|---|
| +0x58 | loaded resource |
| +0x5c | rate/param |
| +0x60 | param |
| +0x64, +0x68 | x, y |

Child commands (`+0x0c`) and their virtual methods (`FUN_004814a0` sprite base, `FUN_00477950` object base):

| Cmd | vtable | Notes |
|---|---|---|
| 1 | +0x38 | show / load |
| 2 | +0x3c | hide / unload |
| 3 | +0x40(p1,p2,p3) | play (animations: from frame p1; sounds: start) |
| 4 | +0x44 | stop |
| 5 | +0x48(p1) | |
| 6 | +0x4c(x, y, z) | set position (`p3` 0 → -1) |
| 7 | +0x50(p1) | z-order |
| 8 | +0x54(p1,p2,p3) | |
| 9 | +0x58(p1,p2) | |
| 10 | +0x5c → result | |
| 0xb, 0xc | anim: +0x60, +0x64(x,y) | anim-specific |
| 0x11–0x13 | +0x90..+0x98 | |
| 0x14 | +0x68(x16, y16, p2) | move |
| 0x15 | +0x6c | |
| 0x16, 0x17 | hotspot: point-in-region tests | |
| 0x18, 0x19 | queries | |
| 0x1a, 0x1b, 0x1d | read object fields | |
| 0x1c, 0x1e, 0x1f, 0x20, 0x22, 0x23 | queries | |
| 0x24 | set cursor | |
| 0x25, 0x26 | anim: current frame / goto frame | |
| 0x27 | set flags | |
| 0x28 | get flags | |
| 0x29–0x2b | flag bit set/clear/test (`+0x1c/+0x20/+0x24`) | |
| 0x2c | enable/disable | |
| 0x2d | hotspot: hit test | |
| 0x35 | bitmap-specific | |

## Resources

See `tools/mm2pack.py`. `mm2.dat` holds BMP (8-bit), FLC, WAV and `.HSP`. The language pack
`<LANG>/mm2lng.dat` holds subtitles (cp1252) and voice WAVs. Labyrinth videos are separate
Cinepak AVIs in `Lab/`.

Both indexes are `{u32 id, u32 offset, u32 size}` records. `mm2.idx` starts with a u32
count (1439); ids below it are in `mm2.dat`, all others in the language pack
(`FUN_00480f20`). `mm2lng.idx` has no count.

`.HSP`: `".HSP"`, u32 count, then per region `{u32 id, u32 type, u32 n, n × {i32 x, i32 y}}`;
type 0 rectangle, 1 ellipse (two corners), 2 polygon (alternate fill).

## Findings from the ScummVM engine

Verified by running the game logic against the native implementation in
[`scummvm/engines/ivanoff`](../scummvm/engines/ivanoff).

**Child commands** (in addition to the table above):

| Cmd | Meaning |
|---|---|
| 8 | move by (p1, p2); walking characters get it from animation frame messages |
| 9 | (frame, heap message): send the message each time the animation shows the frame; message 0 removes the frame's messages |
| 0x11 | show a w × h part of the image (text boxes: the box) |
| 0x12, 0x13 | scroll the shown part to / by (x, y), clamped unless p3; 0x13 returns the applied delta |
| 0x17 | position, y << 16 \| x (hotspots: centre of region p2, or of all if p1 = 0) |
| 0x19 | image size, h << 16 \| w |
| 0x2c | p1 = 0: the whole bounding box counts as a hit (no transparency test) |
| 0x35 | load a BMP from memory (p1 pointer, p2 size): save thumbnails |

- Child id ≥ 0xfdff in a command means "every child whose type is that, or that carries the
  tag" (tags are set with command 0x29, `FUN_00477910`).
- The game writes some child fields directly: `+0x50` (u16) is a resource id to load instead
  of the child's own (the menu's check marks).
- Unloading a child hides it first.
- Sound children: the constructor's parameter is a loop count (1 once, 0 forever).

**More child types**: `0xfe02` (`FUN_00479400`) is a string from the language pack; play(p1)
shows it in text box p1 = (child << 16) | object. `0xfe04` (`FUN_0047af30`) is an animation
whose frames come from an AVI file (the labyrinths). `0xfe05` (`FUN_004797f0`) is a text box:
(fg, bg, font, size in tenths of a point, wrap, mode, style, source, flags), drawn with
`CFont::CreatePointFont`, mode 0/1/2 aligning the lines [unverified: mode 2 = right].
Text box commands: 0xd font, 0xe source string child, 0xf literal text, 0x10 colours, 0x11 box.

**Input** (`FUN_0047f2b0`–`FUN_0047f8a0`, `FUN_0047e480`–`FUN_0047e7e0`): the topmost child with a
matching input flag gets the event; keys go to objects whose `+0x18` has bit 0x10000 (down)
or 0x20000 (up), or only to the object in world `+0x14` (`FUN_00477240`). Drag and drop has two
modes: flag 0x80 press-drag-release, 0x100 click to pick up and click to drop (0x300: also
ask 0x1397). Cursor values without the high bit are sprites of the engine's input object (the
second argument of the setup routine), drawn at the mouse position.

**Startup**: C++ static initializers (`0x4c6000`–`0x4c612c`), then `FUN_0043f760(world,
inputObject)`, then message 1 to object 17. The application object (`FUN_0047b5d0`) only holds
the command line (`+0x70`, searched for "notime").

**Settings and saves**: `C:\...\appdata\settings.dat` is an INI file (`SoundLevel`,
`SavedGameN`, `SavedGameTimeN`), created with `CREATE_NEW`. Saving (`FUN_0047bea0(file,
version)`) writes a header record and then every object's variables through an MFC `CFile`;
loading checks the version first.

## Save files

See `tools/m2s.py`. Records are `{u16 type, u16 id, u32 len, data}`. The current room is in
records (17,1) and (82,53), i.e. variables of objects 17 and 82.
