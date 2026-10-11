/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef IVANOFF_EMU_CPU_H
#define IVANOFF_EMU_CPU_H

#include "common/array.h"
#include "common/hashmap.h"

#include "ivanoff/emu/memory.h"

namespace Ivanoff {
namespace Emu {

enum Reg { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI };

class CPU;

/** Native implementations of trapped functions. */
class TrapHandler {
public:
	virtual ~TrapHandler() {}
	/**
	 * Called when the CPU reaches a trapped address. Arguments are at
	 * [esp + 4] and up, ECX holds "this". Set EAX for the result; the CPU
	 * then returns to the caller and pops the trap's cleanup bytes.
	 */
	virtual void trap(CPU &cpu, uint32 addr) = 0;
};

/**
 * An interpreter for the 32-bit x86 code that MSVC 6 generates (integer
 * instructions, string instructions and a small x87 subset). Code runs in a
 * flat address space; native code is reached through "traps": addresses
 * that hold the UD2 instruction.
 */
class CPU {
public:
	/** Native code calls into emulated code return here. */
	static const uint32 kReturnAddress = 0x0000f000;

	CPU(Memory &mem);

	uint32 r[8];
	uint32 eip = 0;
	uint32 fsBase = 0;
	bool cf = false, pf = false, af = false, zf = false, sf = false, of = false, df = false;

	Memory &mem() { return _mem; }

	void setTrapHandler(TrapHandler *h) { _handler = h; }
	/** Patches addr with a trap; on return, cleanup bytes of arguments are popped. */
	void addTrap(uint32 addr, uint16 cleanup);
	bool isTrap(uint32 addr) const { return _traps.contains(addr); }

	/** Native → emulated call (thiscall when thisPtr is used). Returns EAX. */
	uint32 call(uint32 addr, uint32 thisPtr, uint32 nargs, const uint32 *args);
	uint32 call(uint32 addr, uint32 thisPtr) { return call(addr, thisPtr, 0, nullptr); }
	uint32 call(uint32 addr, uint32 thisPtr, uint32 a1) { return call(addr, thisPtr, 1, &a1); }
	uint32 call(uint32 addr, uint32 thisPtr, uint32 a1, uint32 a2) {
		uint32 a[2] = { a1, a2 };
		return call(addr, thisPtr, 2, a);
	}

	/** Trap argument i (0-based) of the current trapped call. */
	uint32 arg(int i) { return _mem.r32(r[ESP] + 4 + 4 * i); }

	void push(uint32 v) {
		r[ESP] -= 4;
		_mem.w32(r[ESP], v);
	}
	uint32 pop() {
		uint32 v = _mem.r32(r[ESP]);
		r[ESP] += 4;
		return v;
	}

	/** Executes one instruction (for tests). */
	void stepOnce() { step(); }

	/** Prints the recent instruction trace. */
	void dumpState();

	/** Range of addresses that must only be reached through traps... */
	uint32 _guardStart = 0, _guardEnd = 0;
	/** ...except these, which may be interpreted. */
	void allowRange(uint32 start, uint32 end) {
		_allowed.push_back(start);
		_allowed.push_back(end);
	}
	bool isAllowed(uint32 addr) const {
		for (uint i = 0; i < _allowed.size(); i += 2)
			if (addr >= _allowed[i] && addr < _allowed[i + 1])
				return true;
		return false;
	}

	uint64 _instructions = 0;

	/** Debugging aid: marks the executed instructions of [base, base + size). */
	Common::Array<byte> _coverage;
	uint32 _coverageBase = 0;

private:
	void run();
	void step();

	uint8 fetch8() { return _mem.r8(eip++); }
	uint16 fetch16() {
		uint16 v = _mem.r16(eip);
		eip += 2;
		return v;
	}
	uint32 fetch32() {
		uint32 v = _mem.r32(eip);
		eip += 4;
		return v;
	}
	uint32 fetchImm(int size) { return size == 1 ? fetch8() : size == 2 ? fetch16() : fetch32(); }

	// ModR/M
	void decodeModRM();
	int _mod = 0, _reg = 0, _rm = 0;
	int _os = 4;   ///< operand size (0x66 prefix: 2)
	int _rep = 0;  ///< 0, 0xf2 or 0xf3
	uint32 _ea = 0;
	bool _fs = false;

	uint32 getReg(int idx, int size) const;
	void setReg(int idx, int size, uint32 v);
	uint32 getRM(int size);
	void setRM(int size, uint32 v);
	uint32 readMem(uint32 a, int size) { return size == 1 ? _mem.r8(a) : size == 2 ? _mem.r16(a) : _mem.r32(a); }
	void writeMem(uint32 a, int size, uint32 v) {
		if (size == 1)
			_mem.w8(a, v);
		else if (size == 2)
			_mem.w16(a, v);
		else
			_mem.w32(a, v);
	}

	// ALU
	uint32 alu(int op, uint32 a, uint32 b, int size);
	uint32 shift(int op, uint32 a, uint32 count, int size);
	void setSZP(uint32 v, int size);
	bool cond(int c) const;
	uint32 getFlags() const;
	void setFlags(uint32 f);
	void branch(uint32 target);

	void stringOp(uint8 op, int size, int rep);
	void group3(int size);
	void fpu(uint8 op);
	void twoByte();

	[[noreturn]] void unimplemented(const char *what);

	Memory &_mem;
	TrapHandler *_handler = nullptr;
	Common::HashMap<uint32, uint16> _traps;
	Common::Array<uint32> _allowed;

	uint32 _startEip = 0;
	static const int kTraceSize = 64;
	uint32 _trace[kTraceSize];
	int _tracePos = 0;

	// x87
	double _st[8];
	int _top = 0;
	uint16 _fsw = 0, _fcw = 0x27f;
	double &st(int i) { return _st[(_top + i) & 7]; }
	void fpush(double v) {
		_top = (_top - 1) & 7;
		_st[_top] = v;
	}
	double fpop() {
		double v = _st[_top];
		_top = (_top + 1) & 7;
		return v;
	}
	void fcompare(double a, double b);
	int64 fround(double v) const;
};

} // End of namespace Emu
} // End of namespace Ivanoff

#endif
