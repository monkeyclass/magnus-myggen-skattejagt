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

#ifndef IVANOFF_RESOURCES_H
#define IVANOFF_RESOURCES_H

#include "common/file.h"
#include "common/hashmap.h"
#include "common/path.h"
#include "common/stream.h"
#include "common/str.h"

namespace Ivanoff {

/**
 * A resource pack: an .idx file listing {id, offset, size} entries into a
 * .dat file. The shared pack's index (mm2.idx) starts with a count, the
 * language packs' (mm2lng.idx) don't (FUN_00480a60).
 */
class ResourcePack {
public:
	bool open(const Common::Path &idxPath, const Common::Path &datPath, bool hasHeader);
	bool hasResource(uint16 id) const { return _entries.contains(id); }
	/** The count in the .idx header. */
	uint32 getHeaderCount() const { return _headerCount; }
	uint32 getSize(uint16 id) const;
	/** Returns a new memory stream with the resource, or nullptr. */
	Common::SeekableReadStream *getResource(uint16 id);

private:
	struct Entry {
		uint32 offset;
		uint32 size;
	};

	Common::HashMap<uint16, Entry> _entries;
	Common::File _dat;
	uint32 _headerCount = 0;
};

/** All resources: the shared pack plus the language pack. */
class Resources {
public:
	bool open(const Common::String &language);
	/** Ids below the shared pack's header count are in the shared pack, others in the language pack (FUN_00480f20). */
	Common::SeekableReadStream *get(uint16 id);
	bool has(uint16 id) const { return id < _shared.getHeaderCount() ? _shared.hasResource(id) : _language.hasResource(id); }
	/** Returns a subtitle/text resource from the language pack (cp1252). */
	Common::String getText(uint16 id);

private:
	ResourcePack _shared;
	ResourcePack _language;
};

} // End of namespace Ivanoff

#endif
