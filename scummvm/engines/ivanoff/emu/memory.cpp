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

#include "common/endian.h"
#include "common/textconsole.h"

#include "ivanoff/emu/memory.h"

namespace Ivanoff {
namespace Emu {

Memory::Memory() {
	memset(_dir, 0, sizeof(_dir));
}

Memory::~Memory() {
	for (uint i = 0; i < _pages.size(); i++)
		delete[] _pages[i];
	for (uint i = 0; i < 1024; i++)
		delete[] _dir[i];
}

void Memory::map(uint32 addr, uint32 size) {
	if (size == 0)
		return;
	uint32 first = addr >> kPageBits;
	uint32 last = (uint32)(((uint64)addr + size - 1) >> kPageBits);
	for (uint32 p = first; p <= last; p++) {
		byte **&dir = _dir[p >> 10];
		if (!dir) {
			dir = new byte *[1024];
			memset(dir, 0, sizeof(byte *) * 1024);
		}
		if (!dir[p & 0x3ff]) {
			byte *page = new byte[kPageSize];
			memset(page, 0, kPageSize);
			dir[p & 0x3ff] = page;
			_pages.push_back(page);
		}
	}
}

bool Memory::isMapped(uint32 addr) const {
	byte **dir = _dir[addr >> 22];
	return dir && dir[(addr >> kPageBits) & 0x3ff];
}

void Memory::fault(uint32 addr) {
	if (_faultHandler)
		_faultHandler(addr);
	error("Emu: access to unmapped address %08x", addr);
}

void Memory::read(uint32 addr, void *dst, uint32 size) {
	byte *d = (byte *)dst;
	while (size) {
		uint32 n = MIN<uint32>(size, kPageSize - (addr & (kPageSize - 1)));
		memcpy(d, ptr(addr), n);
		d += n;
		addr += n;
		size -= n;
	}
}

void Memory::write(uint32 addr, const void *src, uint32 size) {
	const byte *s = (const byte *)src;
	while (size) {
		uint32 n = MIN<uint32>(size, kPageSize - (addr & (kPageSize - 1)));
		memcpy(ptr(addr), s, n);
		s += n;
		addr += n;
		size -= n;
	}
}

void Memory::fill(uint32 addr, byte value, uint32 size) {
	while (size) {
		uint32 n = MIN<uint32>(size, kPageSize - (addr & (kPageSize - 1)));
		memset(ptr(addr), value, n);
		addr += n;
		size -= n;
	}
}

Common::String Memory::readString(uint32 addr, uint32 maxLen) {
	Common::String s;
	if (addr == 0)
		return s;
	for (uint32 i = 0; i < maxLen; i++) {
		char c = (char)r8(addr + i);
		if (!c)
			break;
		s += c;
	}
	return s;
}

void Memory::writeString(uint32 addr, const Common::String &s) {
	write(addr, s.c_str(), s.size() + 1);
}

// The heap: blocks have an 8-byte header holding their size. Freed blocks
// are kept in per-size free lists; the game allocates mostly small objects
// of a few fixed sizes, so this is enough.

uint32 Memory::alloc(uint32 size) {
	size = (MAX<uint32>(size, 1) + 7) & ~7;
	Common::HashMap<uint32, Common::Array<uint32> >::iterator it = _freeLists.find(size);
	if (it != _freeLists.end() && !it->_value.empty()) {
		uint32 a = it->_value.back();
		it->_value.pop_back();
		fill(a, 0, size);
		return a;
	}
	uint32 block = _heapTop;
	if ((uint64)block + 8 + size > kHeapEnd)
		error("Emu: out of heap");
	_heapTop += 8 + size;
	map(block, 8 + size);
	w32(block, size);
	w32(block + 4, 0x48454150); // 'HEAP'
	return block + 8;
}

uint32 Memory::allocSize(uint32 addr) {
	return r32(addr - 8);
}

void Memory::free(uint32 addr) {
	if (addr == 0)
		return;
	if (addr < kHeapStart + 8 || addr >= _heapTop || r32(addr - 4) != 0x48454150) {
		warning("Emu: free of invalid pointer %08x", addr);
		return;
	}
	_freeLists[allocSize(addr)].push_back(addr);
}

uint32 Memory::realloc(uint32 addr, uint32 size) {
	if (addr == 0)
		return alloc(size);
	if (size == 0) {
		free(addr);
		return 0;
	}
	uint32 old = allocSize(addr);
	if (size <= old)
		return addr;
	uint32 n = alloc(size);
	byte *tmp = new byte[old];
	read(addr, tmp, old);
	write(n, tmp, old);
	delete[] tmp;
	free(addr);
	return n;
}

uint32 Memory::allocString(const Common::String &s) {
	uint32 a = alloc(s.size() + 1);
	writeString(a, s);
	return a;
}

} // End of namespace Emu
} // End of namespace Ivanoff
