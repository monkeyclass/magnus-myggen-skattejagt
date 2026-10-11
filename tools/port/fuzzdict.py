#!/usr/bin/env python3
"""Builds the dictionary for the engine's "fuzz" debug event from the typed
Ghidra output (tools/ghidra/ApplyEngineTypes.java): per class of object,
the message numbers its handler tests, the constants it uses and the
variables it reads (id, or id:object).

usage: fuzzdict.py typed.c rooms.txt > fuzz.dict
"""
import re
import sys

BASE_MSGS = [5, 8, 0x13, 0x21, 0x2f, 0x3b, 0x3c, 0x42, 0x43, 5000, 0x138f, 0x1390,
             0x1393, 0x1394, 0x1395, 0x1396, 0x1397, 0x1398, 0x1399, 0x139a, 0x139b]


def functions(path):
    funcs, name, addr, body = {}, None, None, []
    for line in open(path, encoding='latin-1'):
        m = re.match(r'// ==== (\S+) @ ([0-9a-f]+)', line)
        if m:
            if name:
                funcs[addr] = (name, ''.join(body))
            name, addr, body = m.group(1), int(m.group(2), 16), []
        else:
            body.append(line)
    if name:
        funcs[addr] = (name, ''.join(body))
    return funcs


def main():
    funcs = functions(sys.argv[1])
    by_name = {n: a for a, (n, _) in funcs.items()}
    for line in open(sys.argv[2]):
        vt = re.search(r'vt=([0-9a-f]+)', line)
        s6 = re.search(r's6=([0-9a-f]+)', line)
        if not vt or not s6:
            continue
        start = int(s6.group(1), 16)
        if start not in funcs:
            continue
        # The handler and what it calls in the game code.
        seen, todo = set(), [start]
        while todo:
            a = todo.pop()
            if a in seen or a not in funcs:
                continue
            seen.add(a)
            for callee in re.findall(r'\b((?:FUN|Object[0-9a-f]+)_[0-9a-f_a-z]+)\(', funcs[a][1]):
                if callee in by_name:
                    todo.append(by_name[callee])
                elif callee.startswith('FUN_'):
                    todo.append(int(callee[4:], 16))
        body = funcs[start][1]
        msgs = set(BASE_MSGS)
        for c in re.findall(r'case (0x[0-9a-f]+|\d+):', body):
            msgs.add(int(c, 0))
        for c in re.findall(r'(?:msg->msg|uVar\d+|sVar\d+) [=!<>]=? (0x[0-9a-f]+|\d+)', body):
            v = int(c, 0)
            if 0 < v < 0x13a0:
                msgs.add(v)
        params = set()
        for a in seen:
            for c in re.findall(r'(?<![\w.])(0x[0-9a-f]+|\d+)(?![\w.])', funcs[a][1]):
                v = int(c, 0)
                if v > 0xffffffff:
                    continue
                if v > 0xffff:
                    params.update([v & 0xffff, v >> 16])
                params.add(v)
        params = sorted(p for p in params if p < 0x80000000)[:400]
        # Variables: var(this, id) and varOf(this, id, object).
        vars_ = set()
        for a in seen:
            for v in re.findall(r'\bvar\(this,(0x[0-9a-f]+|\d+)\)', funcs[a][1]):
                vars_.add('%x' % int(v, 0))
            for v, o in re.findall(r'\bvarOf\(this,(0x[0-9a-f]+|\d+),(0x[0-9a-f]+|\d+)\)', funcs[a][1]):
                vars_.add('%x:%x' % (int(v, 0), int(o, 0)))
        print('%s %s %s %s' % (vt.group(1), ','.join('%x' % m for m in sorted(msgs)),
                               ','.join('%x' % p for p in params), ','.join(sorted(vars_)) or '-'))


main()
