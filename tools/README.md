# tools

- `mm2pack.py idx dat [list | dump ID out | hexdump ID [n]]`: reads Ivanoff `.idx`/`.dat` packs.
- `xref.py exe "string" [before] [after]`: finds code references to a string in a PE and disassembles around them.
- `dis.py exe START END`: linear disassembly of a VA range, annotating string and import references.

The Python tools need `pip install capstone pefile` (`mm2pack.py` needs nothing).
