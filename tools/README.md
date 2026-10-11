# tools

- `mm2pack.py idx dat [list | dump ID out | hexdump ID [n]]`: reads Ivanoff `.idx`/`.dat` packs.
- `xref.py exe "string" [before] [after]`: finds code references to a string in a PE and disassembles around them.
- `dis.py exe START END`: linear disassembly of a VA range, annotating string and import references.
- `callers.py exe VA...`: finds the start of the function containing each VA, and its direct callers.
- `m2s.py save.m2s [other.m2s]`: dumps a save file's records, or the records that differ between two saves.
- `m2s_setroom.py in.m2s out.m2s ROOM`: copies a save with the current room changed. This is a testing aid
  for jumping to a scene (labyrinths: 40 and 42). Add the slot to `settings.dat` as `SavedGame<N>=<thumbnail room>`.

The Python tools need `pip install capstone pefile` (`mm2pack.py` needs nothing).
