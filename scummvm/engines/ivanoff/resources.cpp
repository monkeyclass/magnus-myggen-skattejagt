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
#include "common/memstream.h"

#include "ivanoff/resources.h"

namespace Ivanoff {

bool ResourcePack::open(const Common::Path &idxPath, const Common::Path &datPath) {
	Common::File idx;
	if (!idx.open(idxPath) || !_dat.open(datPath))
		return false;

	// The first word is a count that doesn't match the number of entries in
	// every pack, so the entries are read until the end of the file instead.
	_headerCount = idx.readUint32LE();
	uint32 count = (idx.size() - 4) / 12;

	uint32 a[3];
	for (uint32 i = 0; i < count; i++) {
		for (int j = 0; j < 3; j++)
			a[j] = idx.readUint32LE();

		Entry e;
		uint16 id;
		if (i == 0) {
			// Language packs start with {offset 0, size, id}.
			_languageLayout = (a[0] == 0 && a[1] != 0 && a[2] > 0x100);
		}
		if (_languageLayout) {
			e.offset = a[0];
			e.size = a[1];
			id = a[2];
		} else {
			id = a[0];
			e.offset = a[1];
			e.size = a[2];
		}
		_entries[id] = e;
	}

	debug(1, "ResourcePack: %s has %d entries", datPath.toString().c_str(), _entries.size());
	return true;
}

uint32 ResourcePack::getSize(uint16 id) const {
	return _entries.contains(id) ? _entries[id].size : 0;
}

Common::SeekableReadStream *ResourcePack::getResource(uint16 id) {
	if (!_entries.contains(id))
		return nullptr;

	const Entry &e = _entries[id];
	_dat.seek(e.offset);
	return _dat.readStream(e.size);
}

bool Resources::open(const Common::String &language) {
	if (!_shared.open("mm2.idx", "mm2.dat"))
		return false;

	Common::Path dir(language);
	return _language.open(dir.join("mm2lng.idx"), dir.join("mm2lng.dat"));
}

Common::SeekableReadStream *Resources::get(uint16 id) {
	if (id < _shared.getHeaderCount())
		return _shared.getResource(id);
	return _language.getResource(id);
}

Common::String Resources::getText(uint16 id) {
	Common::SeekableReadStream *s = _language.getResource(id);
	if (!s)
		return Common::String();

	Common::String text;
	while (!s->eos()) {
		char c = s->readByte();
		if (s->eos() || c == 0)
			break;
		text += c;
	}
	delete s;
	return text;
}

} // End of namespace Ivanoff
