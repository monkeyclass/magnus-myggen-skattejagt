# Interpreter fuzz test

Compares the ScummVM engine's x86 interpreter (`scummvm/engines/ivanoff/emu`)
with [unicorn](https://www.unicorn-engine.org/) on single instructions taken
from `mm2run.exe`, with random register values. Flags that x86 leaves undefined
are ignored.

    pip install unicorn capstone pefile
    # in a configured ScummVM tree with the engine copied in:
    g++ -O1 -std=c++11 -I. -Iengines -DPOSIX -DHAVE_CONFIG_H -o harness \
        path/to/harness.cpp engines/ivanoff/emu/cpu.cpp engines/ivanoff/emu/memory.cpp \
        common/hashmap.o common/memorypool.o common/str.o common/str-base.o
    python3 fuzz.py 15000 1    # cases, random seed

`fuzz.py` expects the harness and `mm2run.exe` at the paths at its top.
