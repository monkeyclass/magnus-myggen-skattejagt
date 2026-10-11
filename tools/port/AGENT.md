# Porting batch instructions (shared by all porting agents)

You port classes of the game "Magnus & Myggen 2" from the original x86 code
(run in an interpreter by our ScummVM engine "ivanoff") to native C++.

Read first:
- /home/user/magnus-myggen-skattejagt/docs/PORTING.md  (the guide: API mapping, pitfalls, style)
- /home/user/agents/X/engines/ivanoff/script.h, scripts/scripts.h, scripts/base.cpp, scripts/items.cpp
  (the API and the examples already ported: follow their style exactly)
  (X = your batch letter)

Your batch: the classes listed in /home/user/analysis/port/batch_X.txt, one per line:
  class <object id or vt=...> <vtable> s5=<accepts fn> s6=<handler fn> own=<functions only your class uses> lines=<n>
The vtables, comma separated, are in /home/user/analysis/port/batch_X.vt.
Helper functions used by several classes are the "shared" lines of
/home/user/analysis/port/classes.txt; the ones whose users are all in your
batch are yours to port (once). Shared functions used outside your batch:
the bases (see PORTING.md) are already ported; for others use callGame().

Tools:
- /home/user/tools/tfn.sh ADDR...   typed decompilation of game functions (ADDR like 00413a10)
- /home/user/tools/fn.sh ADDR...    untyped decompilation, also covers 0x476000+ (e.g. 004764d0)
- /home/user/analysis/decomp/game.asm   objdump -d -M intel of 0x401000-0x476000 (grep "^  413a10:")
  For other ranges: objdump -d -M intel --start-address=0x... --stop-address=0x... /home/user/gamedata/mm2-scummvm/mm2run.exe
- Build: cd /home/user/agents/X && make -j2 2>&1 | grep -E "error|warning" (your own ScummVM tree)
- Check: /home/user/tools/portcheck.sh /home/user/agents/X X <vtables>
  Runs the game deterministically with your classes native and with the
  original code, compares the traces, prints the first difference of each
  differing run and the coverage of your handlers by the fuzzing runs.
  Takes a few minutes (other agents share the 4 CPUs). Logs in /tmp/claude-0/det/X/.
  You can check a subset of vtables while working, and must check all of them at the end.

Rules:
- Only create/edit /home/user/agents/X/engines/ivanoff/scripts/batch_X.cpp and
  /home/user/agents/X/engines/ivanoff/scripts/registry_X.h (SCRIPT(0xVTABLE, ClassName) lines).
  Do not edit any other file, any other agent's tree, /home/user/scummvm or the git repository. No git commands.
- If you need an API that script.h lacks, write a small helper in your batch file (a local
  base class or a static function) and mention it in your report.
- Write native code against the API, readable, in ScummVM style. Do not paste decompiler
  output or its variable names; keep the original handler addresses in the class comments.
- Correctness first: the native code must do exactly what the original does, in the same
  order (see the pitfalls). When the decompilation is unclear, read the disassembly.
- Iterate until portcheck reports "different 0". A difference names the first diverging
  message/command; compare that path of your code with the original.
- Where coverage is low, review the uncovered paths against the disassembly with extra care.
- Don't run more than one portcheck at a time.

Report at the end (short): classes ported, final portcheck line ("runs: same N, different M")
and the coverage lines, helpers handled with callGame or local helpers, and anything you are
unsure about.
