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

#include "common/debug.h"
#include "common/textconsole.h"

#include "ivanoff/emu/cpu.h"

namespace Ivanoff {
namespace Emu {

static inline uint32 sizeMask(int size) {
	return size == 1 ? 0xff : size == 2 ? 0xffff : 0xffffffff;
}

static inline uint32 signBit(int size) {
	return size == 1 ? 0x80 : size == 2 ? 0x8000 : 0x80000000;
}

static inline int32 sext(uint32 v, int size) {
	return size == 1 ? (int32)(int8)v : size == 2 ? (int32)(int16)v : (int32)v;
}

CPU::CPU(Memory &mem) : _mem(mem) {
	memset(r, 0, sizeof(r));
	memset(_trace, 0, sizeof(_trace));
	memset(_st, 0, sizeof(_st));
}

void CPU::addTrap(uint32 addr, uint16 cleanup) {
	_traps[addr] = cleanup;
	_mem.w8(addr, 0x0f);
	_mem.w8(addr + 1, 0x0b);
}

uint32 CPU::call(uint32 addr, uint32 thisPtr, uint32 nargs, const uint32 *args) {
	uint32 savedEip = eip;
	uint32 savedEsp = r[ESP];
	for (int i = (int)nargs - 1; i >= 0; i--)
		push(args[i]);
	push(kReturnAddress);
	r[ECX] = thisPtr;
	eip = addr;
	run();
	uint32 result = r[EAX];
	r[ESP] = savedEsp;
	eip = savedEip;
	return result;
}

void CPU::run() {
	while (eip != kReturnAddress)
		step();
}

void CPU::dumpState() {
	debugN("Emu: EIP=%08x EAX=%08x ECX=%08x EDX=%08x EBX=%08x ESP=%08x EBP=%08x ESI=%08x EDI=%08x\n",
	       _startEip, r[EAX], r[ECX], r[EDX], r[EBX], r[ESP], r[EBP], r[ESI], r[EDI]);
	debugN("Emu: last instructions:");
	for (int i = 0; i < kTraceSize; i++)
		debugN(" %08x", _trace[(_tracePos + i) % kTraceSize]);
	debugN("\nEmu: stack:");
	for (int i = 0; i < 16; i++) {
		uint32 a = r[ESP] + 4 * i;
		if (_mem.isMapped(a))
			debugN(" %08x", _mem.r32(a));
	}
	debugN("\n");
}

void CPU::unimplemented(const char *what) {
	dumpState();
	error("Emu: unimplemented %s at %08x (bytes %02x %02x %02x %02x)", what, _startEip,
	      _mem.r8(_startEip), _mem.r8(_startEip + 1), _mem.r8(_startEip + 2), _mem.r8(_startEip + 3));
}

// --- operands ---

void CPU::decodeModRM() {
	uint8 m = fetch8();
	_mod = m >> 6;
	_reg = (m >> 3) & 7;
	_rm = m & 7;
	if (_mod == 3)
		return;

	uint32 a;
	if (_rm == 4) {
		uint8 sib = fetch8();
		int scale = sib >> 6, index = (sib >> 3) & 7, base = sib & 7;
		if (base == 5 && _mod == 0)
			a = fetch32();
		else
			a = r[base];
		if (index != 4)
			a += r[index] << scale;
	} else if (_rm == 5 && _mod == 0) {
		a = fetch32();
	} else {
		a = r[_rm];
	}
	if (_mod == 1)
		a += (int8)fetch8();
	else if (_mod == 2)
		a += fetch32();
	if (_fs)
		a += fsBase;
	_ea = a;
}

uint32 CPU::getReg(int idx, int size) const {
	if (size == 4)
		return r[idx];
	if (size == 2)
		return r[idx] & 0xffff;
	return idx < 4 ? r[idx] & 0xff : (r[idx - 4] >> 8) & 0xff;
}

void CPU::setReg(int idx, int size, uint32 v) {
	if (size == 4)
		r[idx] = v;
	else if (size == 2)
		r[idx] = (r[idx] & 0xffff0000) | (v & 0xffff);
	else if (idx < 4)
		r[idx] = (r[idx] & 0xffffff00) | (v & 0xff);
	else
		r[idx - 4] = (r[idx - 4] & 0xffff00ff) | ((v & 0xff) << 8);
}

uint32 CPU::getRM(int size) {
	return _mod == 3 ? getReg(_rm, size) : readMem(_ea, size);
}

void CPU::setRM(int size, uint32 v) {
	if (_mod == 3)
		setReg(_rm, size, v);
	else
		writeMem(_ea, size, v);
}

// --- flags and ALU ---

void CPU::setSZP(uint32 v, int size) {
	v &= sizeMask(size);
	zf = v == 0;
	sf = (v & signBit(size)) != 0;
	uint8 b = v & 0xff;
	b ^= b >> 4;
	b ^= b >> 2;
	b ^= b >> 1;
	pf = !(b & 1);
}

uint32 CPU::alu(int op, uint32 a, uint32 b, int size) {
	uint32 mask = sizeMask(size), sign = signBit(size);
	a &= mask;
	b &= mask;
	uint32 res;
	switch (op) {
	case 0: // add
	case 2: { // adc
		uint32 c = (op == 2 && cf) ? 1 : 0;
		uint64 wide = (uint64)a + b + c;
		res = (uint32)wide & mask;
		cf = wide > mask;
		of = ((a ^ res) & (b ^ res) & sign) != 0;
		af = ((a ^ b ^ res) & 0x10) != 0;
		break;
	}
	case 3: // sbb
	case 5: // sub
	case 7: { // cmp
		uint32 c = (op == 3 && cf) ? 1 : 0;
		res = (a - b - c) & mask;
		cf = (uint64)a < (uint64)b + c;
		of = ((a ^ b) & (a ^ res) & sign) != 0;
		af = ((a ^ b ^ res) & 0x10) != 0;
		break;
	}
	case 1:
		res = a | b;
		cf = of = af = false;
		break;
	case 4:
		res = a & b;
		cf = of = af = false;
		break;
	default:
		res = a ^ b;
		cf = of = af = false;
		break;
	}
	setSZP(res, size);
	return res;
}

uint32 CPU::shift(int op, uint32 a, uint32 count, int size) {
	int bits = size * 8;
	uint32 mask = sizeMask(size), sign = signBit(size);
	count &= 0x1f;
	a &= mask;
	if (count == 0)
		return a;

	uint32 res;
	switch (op) {
	case 0: { // rol
		int c = count % bits;
		res = c ? ((a << c) | (a >> (bits - c))) & mask : a;
		cf = res & 1;
		of = ((res & sign) != 0) != cf;
		return res;
	}
	case 1: { // ror
		int c = count % bits;
		res = c ? ((a >> c) | (a << (bits - c))) & mask : a;
		cf = (res & sign) != 0;
		of = ((res ^ (res << 1)) & sign) != 0;
		return res;
	}
	case 2: { // rcl
		res = a;
		for (uint32 i = 0; i < count % (bits + 1); i++) {
			bool c = (res & sign) != 0;
			res = ((res << 1) | (cf ? 1 : 0)) & mask;
			cf = c;
		}
		of = ((res & sign) != 0) != cf;
		return res;
	}
	case 3: { // rcr
		res = a;
		for (uint32 i = 0; i < count % (bits + 1); i++) {
			bool c = res & 1;
			res = (res >> 1) | (cf ? sign : 0);
			cf = c;
		}
		of = ((res ^ (res << 1)) & sign) != 0;
		return res;
	}
	case 4:
	case 6: // shl
		res = count < (uint32)bits ? (a << count) & mask : 0;
		cf = count <= (uint32)bits ? (a >> (bits - count)) & 1 : false;
		of = ((res & sign) != 0) != cf;
		break;
	case 5: // shr
		res = count < (uint32)bits ? a >> count : 0;
		cf = count <= (uint32)bits ? (a >> (count - 1)) & 1 : false;
		of = (a & sign) != 0;
		break;
	default: { // sar
		int32 sa = sext(a, size);
		if (count < (uint32)bits) {
			res = (uint32)(sa >> count) & mask;
			cf = (sa >> (count - 1)) & 1;
		} else {
			res = (sa < 0 ? 0xffffffff : 0) & mask;
			cf = sa < 0;
		}
		of = false;
		break;
	}
	}
	setSZP(res, size);
	return res;
}

bool CPU::cond(int c) const {
	bool v;
	switch (c >> 1) {
	case 0: v = of; break;
	case 1: v = cf; break;
	case 2: v = zf; break;
	case 3: v = cf || zf; break;
	case 4: v = sf; break;
	case 5: v = pf; break;
	case 6: v = sf != of; break;
	default: v = zf || (sf != of); break;
	}
	return (c & 1) ? !v : v;
}

uint32 CPU::getFlags() const {
	return (cf ? 1 : 0) | 2 | (pf ? 4 : 0) | (af ? 0x10 : 0) | (zf ? 0x40 : 0) | (sf ? 0x80 : 0) |
	       0x200 | (df ? 0x400 : 0) | (of ? 0x800 : 0);
}

void CPU::setFlags(uint32 f) {
	cf = f & 1;
	pf = f & 4;
	af = f & 0x10;
	zf = f & 0x40;
	sf = f & 0x80;
	df = f & 0x400;
	of = f & 0x800;
}

void CPU::branch(uint32 target) {
	if (target >= _guardStart && target < _guardEnd && !_traps.contains(target) && !isAllowed(target)) {
		dumpState();
		error("Emu: call to unimplemented engine function %08x from %08x", target, _startEip);
	}
	eip = target;
}

// --- string instructions ---

void CPU::stringOp(uint8 op, int size, int rep) {
	int delta = df ? -size : size;
	bool compare = (op & 0xf6) == 0xa6; // cmps, scas
	uint32 src = r[ESI] + (_fs ? fsBase : 0);

	for (;;) {
		if (rep && r[ECX] == 0)
			break;
		switch (op & 0xfe) {
		case 0xa4: // movs
			writeMem(r[EDI], size, readMem(src, size));
			r[ESI] += delta;
			r[EDI] += delta;
			src += delta;
			break;
		case 0xa6: // cmps
			alu(7, readMem(src, size), readMem(r[EDI], size), size);
			r[ESI] += delta;
			r[EDI] += delta;
			src += delta;
			break;
		case 0xaa: // stos
			writeMem(r[EDI], size, getReg(EAX, size));
			r[EDI] += delta;
			break;
		case 0xac: // lods
			setReg(EAX, size, readMem(src, size));
			r[ESI] += delta;
			src += delta;
			break;
		default: // scas
			alu(7, getReg(EAX, size), readMem(r[EDI], size), size);
			r[EDI] += delta;
			break;
		}
		if (!rep)
			break;
		r[ECX]--;
		if (compare) {
			if (rep == 0xf3 && !zf)
				break;
			if (rep == 0xf2 && zf)
				break;
		}
	}
}

// --- group 3: test/not/neg/mul/imul/div/idiv ---

void CPU::group3(int size) {
	decodeModRM();
	uint32 mask = sizeMask(size);
	uint32 v = getRM(size);
	switch (_reg) {
	case 0:
	case 1:
		alu(4, v, fetchImm(size), size);
		break;
	case 2:
		setRM(size, ~v & mask);
		break;
	case 3: {
		uint32 res = alu(5, 0, v, size);
		cf = (v & mask) != 0;
		setRM(size, res);
		break;
	}
	case 4: // mul
		if (size == 1) {
			uint32 res = (r[EAX] & 0xff) * v;
			setReg(EAX, 2, res);
			cf = of = (res & 0xff00) != 0;
		} else if (size == 2) {
			uint32 res = (r[EAX] & 0xffff) * v;
			setReg(EAX, 2, res);
			setReg(EDX, 2, res >> 16);
			cf = of = (res >> 16) != 0;
		} else {
			uint64 res = (uint64)r[EAX] * v;
			r[EAX] = (uint32)res;
			r[EDX] = (uint32)(res >> 32);
			cf = of = r[EDX] != 0;
		}
		break;
	case 5: // imul
		if (size == 1) {
			int32 res = (int32)(int8)r[EAX] * (int8)v;
			setReg(EAX, 2, res);
			cf = of = res != (int8)res;
		} else if (size == 2) {
			int32 res = (int32)(int16)r[EAX] * (int16)v;
			setReg(EAX, 2, res);
			setReg(EDX, 2, (uint32)res >> 16);
			cf = of = res != (int16)res;
		} else {
			int64 res = (int64)(int32)r[EAX] * (int32)v;
			r[EAX] = (uint32)res;
			r[EDX] = (uint32)((uint64)res >> 32);
			cf = of = res != (int32)res;
		}
		break;
	case 6: // div
		if (v == 0)
			unimplemented("division by zero");
		if (size == 1) {
			uint32 n = r[EAX] & 0xffff;
			setReg(EAX, 1, n / v);
			setReg(4, 1, n % v); // AH
		} else if (size == 2) {
			uint32 n = ((r[EDX] & 0xffff) << 16) | (r[EAX] & 0xffff);
			setReg(EAX, 2, n / v);
			setReg(EDX, 2, n % v);
		} else {
			uint64 n = ((uint64)r[EDX] << 32) | r[EAX];
			r[EAX] = (uint32)(n / v);
			r[EDX] = (uint32)(n % v);
		}
		break;
	default: { // idiv
		int32 d = sext(v, size);
		if (d == 0)
			unimplemented("division by zero");
		if (size == 1) {
			int32 n = (int16)(r[EAX] & 0xffff);
			setReg(EAX, 1, n / d);
			setReg(4, 1, n % d);
		} else if (size == 2) {
			int32 n = (int32)(((r[EDX] & 0xffff) << 16) | (r[EAX] & 0xffff));
			setReg(EAX, 2, n / d);
			setReg(EDX, 2, n % d);
		} else {
			int64 n = (int64)(((uint64)r[EDX] << 32) | r[EAX]);
			r[EAX] = (uint32)(n / d);
			r[EDX] = (uint32)(n % d);
		}
		break;
	}
	}
}

// --- the main decoder ---

void CPU::step() {
	_trace[_tracePos] = eip;
	_tracePos = (_tracePos + 1) % kTraceSize;
	_instructions++;
	_startEip = eip;

	_os = 4;
	_rep = 0;
	_fs = false;
	uint8 op;
	for (;;) {
		op = fetch8();
		if (op == 0x66)
			_os = 2;
		else if (op == 0xf2 || op == 0xf3)
			_rep = op;
		else if (op == 0x64)
			_fs = true;
		else if (op == 0x26 || op == 0x2e || op == 0x36 || op == 0x3e || op == 0xf0)
			continue;
		else
			break;
	}
	int os = _os;

	if (op < 0x40 && (op & 7) < 6) {
		int aop = op >> 3;
		uint32 res;
		switch (op & 7) {
		case 0:
		case 1: {
			int size = (op & 1) ? os : 1;
			decodeModRM();
			res = alu(aop, getRM(size), getReg(_reg, size), size);
			if (aop != 7)
				setRM(size, res);
			break;
		}
		case 2:
		case 3: {
			int size = (op & 1) ? os : 1;
			decodeModRM();
			res = alu(aop, getReg(_reg, size), getRM(size), size);
			if (aop != 7)
				setReg(_reg, size, res);
			break;
		}
		case 4:
			res = alu(aop, getReg(EAX, 1), fetch8(), 1);
			if (aop != 7)
				setReg(EAX, 1, res);
			break;
		default:
			res = alu(aop, getReg(EAX, os), fetchImm(os), os);
			if (aop != 7)
				setReg(EAX, os, res);
			break;
		}
		return;
	}

	switch (op) {
	case 0x0f:
		twoByte();
		break;

	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: {
		bool c = cf;
		setReg(op & 7, os, alu(0, getReg(op & 7, os), 1, os));
		cf = c;
		break;
	}
	case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: {
		bool c = cf;
		setReg(op & 7, os, alu(5, getReg(op & 7, os), 1, os));
		cf = c;
		break;
	}
	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		if (os == 2) {
			r[ESP] -= 2;
			_mem.w16(r[ESP], getReg(op & 7, 2));
		} else {
			push(r[op & 7]);
		}
		break;
	case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
		if (os == 2) {
			uint16 v = _mem.r16(r[ESP]);
			r[ESP] += 2;
			setReg(op & 7, 2, v);
		} else {
			uint32 v = pop();
			r[op & 7] = v;
		}
		break;
	case 0x60: {
		uint32 sp = r[ESP];
		push(r[EAX]); push(r[ECX]); push(r[EDX]); push(r[EBX]);
		push(sp); push(r[EBP]); push(r[ESI]); push(r[EDI]);
		break;
	}
	case 0x61:
		r[EDI] = pop(); r[ESI] = pop(); r[EBP] = pop(); pop();
		r[EBX] = pop(); r[EDX] = pop(); r[ECX] = pop(); r[EAX] = pop();
		break;
	case 0x68:
		push(os == 2 ? fetch16() : fetch32());
		break;
	case 0x6a:
		push((uint32)(int32)(int8)fetch8());
		break;
	case 0x69:
	case 0x6b: {
		decodeModRM();
		int32 a = sext(getRM(os), os);
		int32 b = op == 0x6b ? (int8)fetch8() : sext(fetchImm(os), os);
		int64 res = (int64)a * b;
		setReg(_reg, os, (uint32)res);
		cf = of = res != sext((uint32)res, os);
		break;
	}
	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
	case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: {
		int8 rel = (int8)fetch8();
		if (cond(op & 0xf))
			eip += rel;
		break;
	}
	case 0x80:
	case 0x82:
	case 0x81:
	case 0x83: {
		int size = op == 0x81 || op == 0x83 ? os : 1;
		decodeModRM();
		uint32 imm = op == 0x83 ? (uint32)(int32)(int8)fetch8() : fetchImm(size);
		uint32 res = alu(_reg, getRM(size), imm, size);
		if (_reg != 7)
			setRM(size, res);
		break;
	}
	case 0x84:
	case 0x85: {
		int size = op == 0x85 ? os : 1;
		decodeModRM();
		alu(4, getRM(size), getReg(_reg, size), size);
		break;
	}
	case 0x86:
	case 0x87: {
		int size = op == 0x87 ? os : 1;
		decodeModRM();
		uint32 a = getRM(size);
		setRM(size, getReg(_reg, size));
		setReg(_reg, size, a);
		break;
	}
	case 0x88:
		decodeModRM();
		setRM(1, getReg(_reg, 1));
		break;
	case 0x89:
		decodeModRM();
		setRM(os, getReg(_reg, os));
		break;
	case 0x8a:
		decodeModRM();
		setReg(_reg, 1, getRM(1));
		break;
	case 0x8b:
		decodeModRM();
		setReg(_reg, os, getRM(os));
		break;
	case 0x8d:
		decodeModRM();
		if (_mod == 3)
			unimplemented("lea with register operand");
		setReg(_reg, os, _ea);
		break;
	case 0x8f: {
		uint32 v = pop();
		decodeModRM();
		setRM(os, v);
		break;
	}
	case 0x90:
		break;
	case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: {
		uint32 a = getReg(EAX, os);
		setReg(EAX, os, getReg(op & 7, os));
		setReg(op & 7, os, a);
		break;
	}
	case 0x98:
		if (os == 2)
			setReg(EAX, 2, (uint32)(int32)(int8)r[EAX]);
		else
			r[EAX] = (uint32)(int32)(int16)r[EAX];
		break;
	case 0x99:
		if (os == 2)
			setReg(EDX, 2, (r[EAX] & 0x8000) ? 0xffff : 0);
		else
			r[EDX] = (r[EAX] & 0x80000000) ? 0xffffffff : 0;
		break;
	case 0x9c:
		push(getFlags());
		break;
	case 0x9d:
		setFlags(pop());
		break;
	case 0x9e:
		setFlags((getFlags() & ~0xd5) | ((r[EAX] >> 8) & 0xd5));
		break;
	case 0x9f:
		setReg(4, 1, getFlags() & 0xff);
		break;
	case 0xa0:
	case 0xa1:
	case 0xa2:
	case 0xa3: {
		uint32 a = fetch32() + (_fs ? fsBase : 0);
		int size = (op & 1) ? os : 1;
		if (op < 0xa2)
			setReg(EAX, size, readMem(a, size));
		else
			writeMem(a, size, getReg(EAX, size));
		break;
	}
	case 0xa4: case 0xa5: case 0xa6: case 0xa7:
	case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
		stringOp(op, (op & 1) ? os : 1, _rep);
		break;
	case 0xa8:
		alu(4, getReg(EAX, 1), fetch8(), 1);
		break;
	case 0xa9:
		alu(4, getReg(EAX, os), fetchImm(os), os);
		break;
	case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		setReg(op & 7, 1, fetch8());
		break;
	case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
		setReg(op & 7, os, fetchImm(os));
		break;
	case 0xc0:
	case 0xc1: {
		int size = op == 0xc1 ? os : 1;
		decodeModRM();
		uint32 count = fetch8();
		setRM(size, shift(_reg, getRM(size), count, size));
		break;
	}
	case 0xd0:
	case 0xd1:
	case 0xd2:
	case 0xd3: {
		int size = (op & 1) ? os : 1;
		decodeModRM();
		uint32 count = op >= 0xd2 ? r[ECX] & 0xff : 1;
		setRM(size, shift(_reg, getRM(size), count, size));
		break;
	}
	case 0xc2: {
		uint16 n = fetch16();
		eip = pop();
		r[ESP] += n;
		break;
	}
	case 0xc3:
		eip = pop();
		break;
	case 0xc6:
		decodeModRM();
		setRM(1, fetch8());
		break;
	case 0xc7:
		decodeModRM();
		setRM(os, fetchImm(os));
		break;
	case 0xc8: {
		uint16 size = fetch16();
		uint8 level = fetch8();
		if (level != 0)
			unimplemented("enter with nesting");
		push(r[EBP]);
		r[EBP] = r[ESP];
		r[ESP] -= size;
		break;
	}
	case 0xc9:
		r[ESP] = r[EBP];
		r[EBP] = pop();
		break;
	case 0xd7:
		setReg(EAX, 1, _mem.r8(r[EBX] + (r[EAX] & 0xff) + (_fs ? fsBase : 0)));
		break;
	case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
		fpu(op);
		break;
	case 0xe0:
	case 0xe1:
	case 0xe2: {
		int8 rel = (int8)fetch8();
		r[ECX]--;
		bool take = r[ECX] != 0;
		if (op == 0xe0)
			take = take && !zf;
		else if (op == 0xe1)
			take = take && zf;
		if (take)
			eip += rel;
		break;
	}
	case 0xe3: {
		int8 rel = (int8)fetch8();
		if (r[ECX] == 0)
			eip += rel;
		break;
	}
	case 0xe8: {
		int32 rel = (int32)fetch32();
		push(eip);
		branch(eip + rel);
		break;
	}
	case 0xe9: {
		int32 rel = (int32)fetch32();
		branch(eip + rel);
		break;
	}
	case 0xeb: {
		int8 rel = (int8)fetch8();
		eip += rel;
		break;
	}
	case 0xf5:
		cf = !cf;
		break;
	case 0xf6:
		group3(1);
		break;
	case 0xf7:
		group3(os);
		break;
	case 0xf8:
		cf = false;
		break;
	case 0xf9:
		cf = true;
		break;
	case 0xfa:
	case 0xfb:
		break;
	case 0xfc:
		df = false;
		break;
	case 0xfd:
		df = true;
		break;
	case 0xfe:
	case 0xff: {
		int size = op == 0xff ? os : 1;
		decodeModRM();
		switch (_reg) {
		case 0:
		case 1: {
			bool c = cf;
			setRM(size, alu(_reg == 0 ? 0 : 5, getRM(size), 1, size));
			cf = c;
			break;
		}
		case 2: {
			if (op == 0xfe)
				unimplemented("fe /2");
			uint32 target = getRM(4);
			push(eip);
			branch(target);
			break;
		}
		case 4:
			if (op == 0xfe)
				unimplemented("fe /4");
			branch(getRM(4));
			break;
		case 6:
			if (op == 0xfe)
				unimplemented("fe /6");
			push(getRM(4));
			break;
		default:
			unimplemented("ff group");
		}
		break;
	}
	default:
		unimplemented("opcode");
	}
}

void CPU::twoByte() {
	uint8 op = fetch8();
	int os = _os;

	if (op >= 0x80 && op <= 0x8f) {
		int32 rel = (int32)fetch32();
		if (cond(op & 0xf))
			eip += rel;
		return;
	}
	if (op >= 0x90 && op <= 0x9f) {
		decodeModRM();
		setRM(1, cond(op & 0xf) ? 1 : 0);
		return;
	}
	if (op >= 0x40 && op <= 0x4f) {
		decodeModRM();
		uint32 v = getRM(os);
		if (cond(op & 0xf))
			setReg(_reg, os, v);
		return;
	}
	if (op >= 0xc8 && op <= 0xcf) {
		uint32 v = r[op & 7];
		r[op & 7] = (v >> 24) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | (v << 24);
		return;
	}

	switch (op) {
	case 0x0b: {
		// UD2: a trap into native code.
		uint32 addr = _startEip;
		if (!_traps.contains(addr) || !_handler)
			unimplemented("UD2 (no trap)");
		uint16 cleanup = _traps[addr];
		_handler->trap(*this, addr);
		eip = pop();
		r[ESP] += cleanup;
		break;
	}
	case 0x31: {
		uint64 t = _instructions;
		r[EAX] = (uint32)t;
		r[EDX] = (uint32)(t >> 32);
		break;
	}
	case 0xa3: case 0xab: case 0xb3: case 0xbb: case 0xba: {
		decodeModRM();
		int32 bit;
		int kind;
		if (op == 0xba) {
			bit = fetch8();
			kind = _reg - 4;
			if (kind < 0)
				unimplemented("0f ba");
			bit &= os * 8 - 1;
		} else {
			bit = (int32)getReg(_reg, os);
			kind = op == 0xa3 ? 0 : op == 0xab ? 1 : op == 0xb3 ? 2 : 3;
			if (_mod != 3) {
				_ea += (bit >> 5) * 4;
				bit &= 31;
			} else {
				bit &= os * 8 - 1;
			}
		}
		uint32 v = getRM(os);
		cf = (v >> bit) & 1;
		if (kind == 1)
			setRM(os, v | (1u << bit));
		else if (kind == 2)
			setRM(os, v & ~(1u << bit));
		else if (kind == 3)
			setRM(os, v ^ (1u << bit));
		break;
	}
	case 0xa4: case 0xa5: case 0xac: case 0xad: {
		decodeModRM();
		uint32 count = (op & 1) ? r[ECX] & 0x1f : fetch8() & 0x1f;
		if (os != 4)
			unimplemented("16-bit shld/shrd");
		if (count == 0)
			break;
		uint32 dst = getRM(4), src = getReg(_reg, 4), res;
		if (op <= 0xa5) {
			res = (dst << count) | (src >> (32 - count));
			cf = (dst >> (32 - count)) & 1;
		} else {
			res = (dst >> count) | (src << (32 - count));
			cf = (dst >> (count - 1)) & 1;
		}
		of = ((res ^ dst) & 0x80000000) != 0;
		setSZP(res, 4);
		setRM(4, res);
		break;
	}
	case 0xaf: {
		decodeModRM();
		int64 res = (int64)sext(getReg(_reg, os), os) * sext(getRM(os), os);
		setReg(_reg, os, (uint32)res);
		cf = of = res != sext((uint32)res, os);
		break;
	}
	case 0xb1: {
		decodeModRM();
		uint32 v = getRM(os);
		alu(7, getReg(EAX, os), v, os);
		if (zf)
			setRM(os, getReg(_reg, os));
		else
			setReg(EAX, os, v);
		break;
	}
	case 0xc1: {
		decodeModRM();
		uint32 v = getRM(os);
		uint32 sum = alu(0, v, getReg(_reg, os), os);
		setReg(_reg, os, v);
		setRM(os, sum);
		break;
	}
	case 0xb6:
		decodeModRM();
		setReg(_reg, os, getRM(1));
		break;
	case 0xb7:
		decodeModRM();
		setReg(_reg, os, getRM(2));
		break;
	case 0xbe:
		decodeModRM();
		setReg(_reg, os, (uint32)(int32)(int8)getRM(1));
		break;
	case 0xbf:
		decodeModRM();
		setReg(_reg, os, (uint32)(int32)(int16)getRM(2));
		break;
	case 0xbc:
	case 0xbd: {
		decodeModRM();
		uint32 v = getRM(os);
		zf = v == 0;
		if (v) {
			int i;
			if (op == 0xbc)
				for (i = 0; !(v & (1u << i)); i++) {}
			else
				for (i = os * 8 - 1; !(v & (1u << i)); i--) {}
			setReg(_reg, os, i);
		}
		break;
	}
	default:
		unimplemented("two-byte opcode");
	}
}

// --- x87 ---

void CPU::fcompare(double a, double b) {
	_fsw &= ~0x4500;
	if (a != a || b != b)
		_fsw |= 0x4500;
	else if (a < b)
		_fsw |= 0x0100;
	else if (a == b)
		_fsw |= 0x4000;
}

int64 CPU::fround(double v) const {
	switch ((_fcw >> 10) & 3) {
	case 0: {
		double f = floor(v);
		double d = v - f;
		if (d > 0.5 || (d == 0.5 && fmod(f, 2.0) != 0))
			f += 1;
		return (int64)f;
	}
	case 1:
		return (int64)floor(v);
	case 2:
		return (int64)ceil(v);
	default:
		return (int64)v;
	}
}

static double farith(int op, double dst, double src) {
	switch (op) {
	case 0: return dst + src;
	case 1: return dst * src;
	case 4: return dst - src;
	case 5: return src - dst;
	case 6: return dst / src;
	default: return src / dst;
	}
}

void CPU::fpu(uint8 op) {
	decodeModRM();
	int reg = _reg;

	if (_mod != 3) {
		uint32 a = _ea;
		switch (op) {
		case 0xd8:
		case 0xdc:
		case 0xda:
		case 0xde: {
			double v;
			if (op == 0xd8) {
				uint32 bits = _mem.r32(a);
				float f;
				memcpy(&f, &bits, 4);
				v = f;
			} else if (op == 0xdc) {
				uint64 bits = _mem.r32(a) | ((uint64)_mem.r32(a + 4) << 32);
				memcpy(&v, &bits, 8);
			} else if (op == 0xda) {
				v = (int32)_mem.r32(a);
			} else {
				v = (int16)_mem.r16(a);
			}
			if (reg == 2 || reg == 3) {
				fcompare(st(0), v);
				if (reg == 3)
					fpop();
			} else {
				st(0) = farith(reg, st(0), v);
			}
			return;
		}
		case 0xd9:
			switch (reg) {
			case 0: {
				uint32 bits = _mem.r32(a);
				float f;
				memcpy(&f, &bits, 4);
				fpush(f);
				return;
			}
			case 2:
			case 3: {
				float f = (float)st(0);
				uint32 bits;
				memcpy(&bits, &f, 4);
				_mem.w32(a, bits);
				if (reg == 3)
					fpop();
				return;
			}
			case 5:
				_fcw = _mem.r16(a);
				return;
			case 7:
				_mem.w16(a, _fcw);
				return;
			default:
				break;
			}
			break;
		case 0xdb:
			switch (reg) {
			case 0:
				fpush((int32)_mem.r32(a));
				return;
			case 2:
			case 3:
				_mem.w32(a, (uint32)(int32)fround(st(0)));
				if (reg == 3)
					fpop();
				return;
			default:
				break;
			}
			break;
		case 0xdd:
			switch (reg) {
			case 0: {
				uint64 bits = _mem.r32(a) | ((uint64)_mem.r32(a + 4) << 32);
				double v;
				memcpy(&v, &bits, 8);
				fpush(v);
				return;
			}
			case 2:
			case 3: {
				double v = st(0);
				uint64 bits;
				memcpy(&bits, &v, 8);
				_mem.w32(a, (uint32)bits);
				_mem.w32(a + 4, (uint32)(bits >> 32));
				if (reg == 3)
					fpop();
				return;
			}
			case 7:
				_mem.w16(a, (_fsw & ~0x3800) | (_top << 11));
				return;
			default:
				break;
			}
			break;
		case 0xdf:
			switch (reg) {
			case 0:
				fpush((int16)_mem.r16(a));
				return;
			case 2:
			case 3:
				_mem.w16(a, (uint16)(int16)fround(st(0)));
				if (reg == 3)
					fpop();
				return;
			case 5:
				fpush((double)(int64)(_mem.r32(a) | ((uint64)_mem.r32(a + 4) << 32)));
				return;
			case 7: {
				uint64 v = (uint64)fround(st(0));
				_mem.w32(a, (uint32)v);
				_mem.w32(a + 4, (uint32)(v >> 32));
				fpop();
				return;
			}
			default:
				break;
			}
			break;
		default:
			break;
		}
		unimplemented("x87 memory instruction");
	}

	int i = _rm;
	switch (op) {
	case 0xd8:
		if (reg == 2 || reg == 3) {
			fcompare(st(0), st(i));
			if (reg == 3)
				fpop();
		} else {
			st(0) = farith(reg, st(0), st(i));
		}
		return;
	case 0xdc:
	case 0xde: {
		if (op == 0xde && reg == 3 && i == 1) { // fcompp
			fcompare(st(0), st(1));
			fpop();
			fpop();
			return;
		}
		if (reg == 2 || reg == 3)
			break;
		// The register forms swap sub/subr and div/divr.
		int r2 = reg >= 4 ? reg ^ 1 : reg;
		st(i) = farith(r2, st(i), st(0));
		if (op == 0xde)
			fpop();
		return;
	}
	case 0xd9:
		switch (reg) {
		case 0: {
			double v = st(i);
			fpush(v);
			return;
		}
		case 1: {
			double t = st(0);
			st(0) = st(i);
			st(i) = t;
			return;
		}
		case 4:
			if (i == 0) {
				st(0) = -st(0);
				return;
			}
			if (i == 1) {
				st(0) = fabs(st(0));
				return;
			}
			if (i == 4) {
				fcompare(st(0), 0.0);
				return;
			}
			break;
		case 5:
			if (i == 0) {
				fpush(1.0);
				return;
			}
			if (i == 6) {
				fpush(0.0);
				return;
			}
			break;
		case 7:
			if (i == 2) {
				st(0) = sqrt(st(0));
				return;
			}
			if (i == 4) {
				st(0) = (double)fround(st(0));
				return;
			}
			break;
		default:
			break;
		}
		break;
	case 0xda:
		if (reg == 5 && i == 1) { // fucompp
			fcompare(st(0), st(1));
			fpop();
			fpop();
			return;
		}
		break;
	case 0xdb:
		if (reg == 4 && (i == 2 || i == 3)) { // fnclex, fninit
			if (i == 3) {
				_top = 0;
				_fcw = 0x37f;
			}
			_fsw = 0;
			return;
		}
		break;
	case 0xdd:
		if (reg == 0) // ffree
			return;
		if (reg == 2 || reg == 3) {
			st(i) = st(0);
			if (reg == 3)
				fpop();
			return;
		}
		if (reg == 4 || reg == 5) { // fucom(p)
			fcompare(st(0), st(i));
			if (reg == 5)
				fpop();
			return;
		}
		break;
	case 0xdf:
		if (reg == 4 && i == 0) {
			setReg(EAX, 2, (_fsw & ~0x3800) | (_top << 11));
			return;
		}
		break;
	default:
		break;
	}
	unimplemented("x87 register instruction");
}

} // End of namespace Emu
} // End of namespace Ivanoff
