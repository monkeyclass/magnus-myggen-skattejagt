# Porting the game logic to native code

The game logic of `mm2run.exe` (about 120 C++ classes) is now native C++ in
`scummvm/engines/ivanoff/scripts/`. This document describes how it was
ported and checked, for changes to it.

## History

1. The engine first ran the original game logic in an x86 interpreter,
   with the engine functions it calls trapped and implemented natively.
2. Each class's slots 5/6 (`accepts`, `handle`) were ported to a native
   `Script`, chosen by the class's vtable (`scripts/registry.h`), while the
   objects, variables and children still came from the original code.
3. The variables (`vars.cpp`) and the setup (`scripts/setup.cpp`, generated
   by `tools/port/gensetup.py` from a recording of the original setup) were
   made native, then the interpreter was removed.

Every step was checked against the original code with the deterministic
traces below; the interpreter is in the git history (before 2026-10-12,
commit `29e3dfa`) for making new reference traces.

## Checking a port

All runs are deterministic with `virtual_clock=<ms>` and `random_seed`, so
a run with the native logic must produce exactly the same message and
command trace (`-d4`) as the same run with the original code (reference
traces made with an interpreter version and `native_scripts=none`). Tools:

* `debug_events` (see `ivanoff.cpp`): `ms:esc`, `ms:click:x,y`,
  `ms:crawl:count,interval` (clicks random clickable things),
  `ms:fuzz:count,interval` (random messages to the objects, built from
  `fuzz_dict`; `fuzz_vars=true` also sets their variables;
  `fuzz_vtables` limits it to some classes), `ms:quit`.
* `coverage=<file>` (interpreter versions only): the executed instructions of the original code.
* `tools/port/fuzzdict.py`: builds `fuzz_dict` from the typed decompilation.
* `tools/port/coverage.py`: per class, how much of the handler a set of
  runs executed.

A fault of the original code (for example a message naming an object that
doesn't exist) ends a debug run the same way with or without the native
logic.

## Reading the decompilation

`tools/ghidra/ApplyEngineTypes.java` types the engine API, `Message`,
`Var` (with its vtable) and `GameObject` in Ghidra and decompiles the game
code (0x401000-0x476000). The output is not committed (it is the
original's code). Mapping to the native API:

| Decompilation | Native |
|---|---|
| `msg->msg`, `msg->p1`, `msg->result`, `msg->handled` | `msg.msg`, `msg.p1`, ... (a `Message &`) |
| `send(this,t,m,p1,p2,p3)` | `send(t, m, p1, p2, p3)` |
| `post(this,...)` | `post(...)` |
| `cmd(this,t,c,child,p1,p2,p3)` | `cmd(t, c, child, p1, p2, p3)` |
| `var(this,id)` / `varOf(this,id,obj)` | `var(id)` / `var(id, obj)` |
| `(*v->vtbl->get)(v)` | `v.get()` |
| `(*v->vtbl->set)(v,x)`, `add`, `sub` | `v.set(x)`, `v.add(n)`, `v.sub(n)` |
| `(*v->vtbl->at)(v,k)` | `v.at(k)` (element of a map variable) |
| `(*v->vtbl->x54)(v,tmp,...)` + destructor `FUN_0049fec5(tmp+0xc)` | `v.postIncrement()` (`v++`; the copy it returns is discarded); `x5c`: `v.postDecrement()` |
| `(*v->vtbl->toString)(v,out)` / `assignString` | `v.toString()` / `v.assignString(s)` |
| `newCmd`/`newMsg` + command 9 | `cmdFrameTrigger(target, child, frame, makeCmd(...))` |
| `setCursor(this,c)` | `setCursor(c)` |
| `newMsg(this,t,m,p1,p2,p3)` + `startTimer(this,id,ms,msg)` | `startTimer(id, ms, t, m, p1, p2, p3)` |
| `newCmd(...)` + `startTimer` | `startCmdTimer(id, ms, t, c, child, p1, p2, p3)` |
| `stopTimer(this,id)` | `stopTimer(id)` |
| `setKeyFocus(this,o)` | `setKeyFocus(o)` |
| `getChild(this,id)` / `getChildOf(this,id,o)` | `getChild(id)` / `getChild(id, o)` |
| `DAT_xxxxxxxx` (globals) | `global32(0x...)` / `setGlobal32(0x..., v)` |
| `this+0x18` (event mask) | `setEventMask(mask)` |

Base classes (the `if (msg->handled == 0) BASE(this, msg)` at the end of
most handlers):

| Original | Native |
|---|---|
| slot 5 `FUN_004554f0`, `FUN_0045da60`, `FUN_0041c2c0` (return 1) | `Script::accepts` (default) |
| slot 5 `FUN_004209d0`, `FUN_00452b40` (→ `FUN_00423a60`) | `ItemScript::accepts` |
| `Object4b51a4_handle` (`FUN_0041b910`), `FUN_004344f0` | `ObjectScript::handle` |
| `FUN_00423aa0` | `ItemScript::handle` |
| `Object4b45e4_handle` (`FUN_004532e0`) | `Object99::handle` |

## Pitfalls

* **Order of reads.** Reading a random, cycler or shuffle variable changes
  it, and C++ leaves the order of evaluation of function arguments
  unspecified. Keep the original order of `get()` calls; use temporaries
  when one statement has several.
* **Ghidra artifacts.** MSVC interleaves the pushes of two calls; Ghidra
  sometimes shows the arguments of the outer call as `iVar6 = 0;` lines
  before the inner one, or as extra arguments of `get` (`get(v,0,0,0)`):
  `get` takes no arguments. `(*v->vtbl->get)()` without arguments reads
  `v`. When something looks odd (`unaff_ESI`, `extraout_`, `CONCAT`,
  `in_stack_`, a `switchD_` label), read the disassembly
  (`objdump -d -M intel` of the address range). `unaff_ESI` in a handler
  is usually `msg` or `this`.
* **Types.** Child and object ids are 16 bits (`(ushort)`), comparisons on
  `(short)` are signed, `(uint)msg->p1 >> 0x10` is unsigned. Keep the
  casts that matter.
* **Results.** `msg->result` is only set where the original sets it.
* **Strings.** Text commands take a string as p1: `cmdString()`.
  The handlers that build strings with the original's string classes
  (`FUN_0040f750` and friends, MFC CString) need care; port the logic,
  not the string class.

## Style

* ScummVM's code style (tabs, braces, `Common::` types).
* One class per original class, named after the object (`Object22`), or
  after the vtable when the id is set at run time (`Object4b52c4`), with a
  comment naming the vtable and the original handler, and what the object
  is when known. Rename to meaningful names when the meaning is clear.
* Use the names of `world.h` for messages and commands where they fit;
  keep hex constants (child ids, text ids) otherwise. Mark guesses
  **[unverified]**.
