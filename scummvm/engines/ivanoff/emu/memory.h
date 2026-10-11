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

#ifndef IVANOFF_EMU_MEMORY_H
#define IVANOFF_EMU_MEMORY_H

#include "common/array.h"
#include "common/endian.h"
#include "common/hashmap.h"
#include "common/str.h"

namespace Ivanoff {
namespace Emu {

/**
 * A sparse 32-bit address space made of 4 KB pages, with a simple heap.
 * All values are little-endian, as on x86.
 */
class Memory {
public:
	static const uint32 kPageBits = 12;
	static const uint32 kPageSize = 1 << kPageBits;

	static const uint32 kHeapStart = 0x01000000;
	static const uint32 kHeapEnd = 0x0f000000;

	Memory();
	~Memory();

	/** Maps zero-filled pages covering [addr, addr + size). */
	void map(uint32 addr, uint32 size);
	bool isMapped(uint32 addr) const;

	byte *ptr(uint32 addr) {
		byte **dir = _dir[addr >> 22];
		byte *page = dir ? dir[(addr >> kPageBits) & 0x3ff] : nullptr;
		if (!page)
			fault(addr);
		return page + (addr & (kPageSize - 1));
	}

	uint8 r8(uint32 a) { return *ptr(a); }
	uint16 r16(uint32 a) {
		if ((a & (kPageSize - 1)) <= kPageSize - 2)
			return READ_LE_UINT16(ptr(a));
		return r8(a) | (r8(a + 1) << 8);
	}
	uint32 r32(uint32 a) {
		if ((a & (kPageSize - 1)) <= kPageSize - 4)
			return READ_LE_UINT32(ptr(a));
		return r16(a) | (r16(a + 2) << 16);
	}
	void w8(uint32 a, uint8 v) { *ptr(a) = v; }
	void w16(uint32 a, uint16 v) {
		if ((a & (kPageSize - 1)) <= kPageSize - 2)
			WRITE_LE_UINT16(ptr(a), v);
		else {
			w8(a, v & 0xff);
			w8(a + 1, v >> 8);
		}
	}
	void w32(uint32 a, uint32 v) {
		if ((a & (kPageSize - 1)) <= kPageSize - 4)
			WRITE_LE_UINT32(ptr(a), v);
		else {
			w16(a, v & 0xffff);
			w16(a + 2, v >> 16);
		}
	}

	void read(uint32 addr, void *dst, uint32 size);
	void write(uint32 addr, const void *src, uint32 size);
	void fill(uint32 addr, byte value, uint32 size);
	/** Reads a zero-terminated string. */
	Common::String readString(uint32 addr, uint32 maxLen = 0x10000);
	/** Writes a zero-terminated string. */
	void writeString(uint32 addr, const Common::String &s);

	// --- heap ---
	uint32 alloc(uint32 size);
	void free(uint32 addr);
	uint32 realloc(uint32 addr, uint32 size);
	uint32 allocSize(uint32 addr);
	/** Allocates and writes a string. */
	uint32 allocString(const Common::String &s);

	/** Called on an access to an unmapped address. */
	void (*_faultHandler)(uint32 addr) = nullptr;

private:
	void fault(uint32 addr);

	byte **_dir[1024];
	Common::Array<byte *> _pages;

	uint32 _heapTop = kHeapStart;
	Common::HashMap<uint32, Common::Array<uint32> > _freeLists;
};

} // End of namespace Emu
} // End of namespace Ivanoff

#endif
