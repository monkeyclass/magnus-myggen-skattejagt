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

bool ResourcePack::open(const Common::Path &idxPath, const Common::Path &datPath, bool hasHeader) {
	Common::File idx;
	if (!idx.open(idxPath) || !_dat.open(datPath))
		return false;

	// The shared pack's count is also where the language pack's ids start.
	if (hasHeader)
		_headerCount = idx.readUint32LE();

	while (idx.pos() + 12 <= idx.size()) {
		uint32 id = idx.readUint32LE();
		Entry e;
		e.offset = idx.readUint32LE();
		e.size = idx.readUint32LE();
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
	if (!_shared.open("mm2.idx", "mm2.dat", true))
		return false;

	Common::Path dir(language);
	return _language.open(dir.join("mm2lng.idx"), dir.join("mm2lng.dat"), false);
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
