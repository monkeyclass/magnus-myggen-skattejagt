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

#ifndef IVANOFF_FILES_H
#define IVANOFF_FILES_H

#include "common/array.h"
#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/str.h"

namespace Ivanoff {

/**
 * The files the game uses, by their Windows names: C:\\MM2\\ is the game's
 * folder, and files in C:\\MM2\\appdata\\ (settings and save games) are
 * ScummVM save files "<target>-<name>". Also the INI functions and the
 * installer's registry values the game reads.
 */
class GameFiles {
public:
	GameFiles();

	bool readFile(const Common::String &winPath, Common::Array<byte> &data);
	bool readFile(const Common::String &winPath, Common::String &text);
	/** Only save files can be written. */
	bool writeFile(const Common::String &winPath, const Common::Array<byte> &data);
	bool writeFile(const Common::String &winPath, const Common::String &text);
	bool exists(const Common::String &winPath);

	/** The path in the game folder of a Windows path, or "". */
	Common::String gamePath(const Common::String &winPath) const {
		Common::String g, save;
		return mapPath(winPath, g, save) ? g : Common::String();
	}

	/** GetPrivateProfileString and WritePrivateProfileString (value nullptr: delete). */
	bool iniGet(const Common::String &file, const Common::String &section, const Common::String &key, Common::String &value);
	void iniSet(const Common::String &file, const Common::String &section, const Common::String &key, const Common::String *value);

	/** A value of HKLM\\Software\\IVANOFF Interactive\\MM2 (lower case name), or "". */
	Common::String registryValue(const Common::String &name) const;

	/** Saves / loads the variables of all objects (the original's .m2s format, FUN_0047bea0 / FUN_0047bfa0). */
	bool saveWorld(const Common::String &file, uint32 version);
	bool loadWorld(const Common::String &file, uint32 version);
	void saveWorld(Common::Array<byte> &data, uint32 version);
	bool loadWorld(const Common::Array<byte> &data, uint32 version);

	enum { kSaveVersion = 0x10100 };

private:
	bool mapPath(const Common::String &winPath, Common::String &gamePath, Common::String &saveName) const;
	Common::HashMap<Common::String, Common::String> _registry;
};

} // End of namespace Ivanoff

#endif
