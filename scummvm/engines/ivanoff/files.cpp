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
#include "common/file.h"
#include "common/savefile.h"
#include "common/system.h"

#include "ivanoff/files.h"
#include "ivanoff/ivanoff.h"
#include "ivanoff/vars.h"
#include "ivanoff/world.h"

namespace Ivanoff {

GameFiles::GameFiles() {
	// The registry values the installer writes. The game folder is C:\\MM2.
	Common::String lang = g_engine->getLanguageDir();
	_registry["labpath"] = "C:\\MM2\\Lab\\";
	_registry["language"] = lang;
	_registry["programpath"] = "C:\\MM2\\";
	_registry["resource file"] = "C:\\MM2\\mm2";
	_registry["resource local file"] = "C:\\MM2\\" + lang + "\\mm2lng";
}

Common::String GameFiles::registryValue(const Common::String &name) const {
	return _registry.contains(name) ? _registry[name] : Common::String();
}

bool GameFiles::mapPath(const Common::String &winPath, Common::String &gamePath, Common::String &saveName) const {
	Common::String p = winPath;
	for (uint i = 0; i < p.size(); i++)
		if (p[i] == '/')
			p.setChar('\\', i);
	Common::String lower = p;
	lower.toLowercase();
	if (lower.hasPrefix("c:\\mm2\\"))
		p = p.substr(7);
	else if (lower.size() > 1 && lower[1] == ':')
		return false;

	Common::String rel = p;
	rel.toLowercase();
	if (rel.hasPrefix("appdata\\")) {
		Common::String rest = p.substr(8);
		for (uint i = 0; i < rest.size(); i++)
			if (rest[i] == '\\')
				rest.setChar('_', i);
		saveName = g_engine->getTarget() + "-" + rest;
		gamePath.clear();
	} else {
		saveName.clear();
		gamePath = p;
		for (uint i = 0; i < gamePath.size(); i++)
			if (gamePath[i] == '\\')
				gamePath.setChar('/', i);
	}
	return true;
}

bool GameFiles::readFile(const Common::String &winPath, Common::Array<byte> &data) {
	Common::String gamePath, saveName;
	if (!mapPath(winPath, gamePath, saveName))
		return false;
	Common::SeekableReadStream *in = nullptr;
	if (!saveName.empty()) {
		in = g_system->getSavefileManager()->openForLoading(saveName);
	} else {
		Common::File *f = new Common::File();
		if (f->open(Common::Path(gamePath, '/')))
			in = f;
		else
			delete f;
	}
	if (!in)
		return false;
	data.resize(in->size());
	if (in->size())
		in->read(&data[0], in->size());
	delete in;
	debug(2, "Files: read %s, %d bytes", winPath.c_str(), data.size());
	return true;
}

bool GameFiles::readFile(const Common::String &winPath, Common::String &text) {
	Common::Array<byte> data;
	if (!readFile(winPath, data))
		return false;
	text = data.empty() ? Common::String() : Common::String((const char *)&data[0], data.size());
	return true;
}

bool GameFiles::writeFile(const Common::String &winPath, const Common::Array<byte> &data) {
	Common::String gamePath, saveName;
	if (!mapPath(winPath, gamePath, saveName) || saveName.empty()) {
		warning("Files: write to %s refused", winPath.c_str());
		return false;
	}
	Common::OutSaveFile *out = g_system->getSavefileManager()->openForSaving(saveName, false);
	if (!out)
		return false;
	if (!data.empty())
		out->write(&data[0], data.size());
	out->finalize();
	bool ok = !out->err();
	delete out;
	debug(2, "Files: wrote %s, %d bytes", winPath.c_str(), data.size());
	return ok;
}

bool GameFiles::writeFile(const Common::String &winPath, const Common::String &text) {
	Common::Array<byte> data(text.size());
	if (text.size())
		memcpy(&data[0], text.c_str(), text.size());
	return writeFile(winPath, data);
}

bool GameFiles::exists(const Common::String &winPath) {
	Common::String gamePath, saveName;
	if (!mapPath(winPath, gamePath, saveName))
		return false;
	if (!saveName.empty())
		return !g_system->getSavefileManager()->listSavefiles(saveName).empty();
	return Common::File::exists(Common::Path(gamePath, '/'));
}

/** Splits an INI file into lines (without line ends). */
static void iniLines(const Common::String &text, Common::Array<Common::String> &lines) {
	Common::String line;
	for (uint i = 0; i < text.size(); i++) {
		if (text[i] == '\n') {
			lines.push_back(line);
			line.clear();
		} else if (text[i] != '\r') {
			line += text[i];
		}
	}
	if (!line.empty())
		lines.push_back(line);
}

bool GameFiles::iniGet(const Common::String &file, const Common::String &section, const Common::String &key, Common::String &value) {
	Common::String text;
	if (!readFile(file, text))
		return false;
	Common::Array<Common::String> lines;
	iniLines(text, lines);
	bool in = false;
	for (uint i = 0; i < lines.size(); i++) {
		Common::String l = lines[i];
		l.trim();
		if (l.hasPrefix("[")) {
			in = l.equalsIgnoreCase("[" + section + "]");
			continue;
		}
		if (!in)
			continue;
		int eq = l.findFirstOf('=');
		if (eq < 0)
			continue;
		Common::String k = l.substr(0, eq);
		k.trim();
		if (k.equalsIgnoreCase(key)) {
			value = l.substr(eq + 1);
			value.trim();
			return true;
		}
	}
	return false;
}

void GameFiles::iniSet(const Common::String &file, const Common::String &section, const Common::String &key, const Common::String *value) {
	Common::String text;
	readFile(file, text);
	Common::Array<Common::String> lines;
	iniLines(text, lines);

	int sectionLine = -1, keyLine = -1, insertAt = -1;
	for (uint i = 0; i < lines.size(); i++) {
		Common::String l = lines[i];
		l.trim();
		if (l.hasPrefix("[")) {
			if (sectionLine >= 0 && insertAt < 0)
				insertAt = i;
			if (l.equalsIgnoreCase("[" + section + "]"))
				sectionLine = i;
			continue;
		}
		if (sectionLine >= 0 && insertAt < 0) {
			int eq = l.findFirstOf('=');
			if (eq >= 0) {
				Common::String k = l.substr(0, eq);
				k.trim();
				if (k.equalsIgnoreCase(key))
					keyLine = i;
			}
		}
	}
	Common::String entry = value ? key + "=" + *value : Common::String();
	if (keyLine >= 0) {
		if (value)
			lines[keyLine] = entry;
		else
			lines.remove_at(keyLine);
	} else if (value) {
		if (sectionLine < 0) {
			lines.push_back("[" + section + "]");
			lines.push_back(entry);
		} else if (insertAt < 0) {
			lines.push_back(entry);
		} else {
			lines.insert_at(insertAt, entry);
		}
	}
	Common::String out;
	for (uint i = 0; i < lines.size(); i++)
		out += lines[i] + "\r\n";
	writeFile(file, out);
}

// A save file is a list of records (key, size, data): the version (key 0),
// then each object's variables in the world's order (FUN_0048d080 file class).

void GameFiles::saveWorld(Common::Array<byte> &data, uint32 version) {
	const Common::Array<GameObject *> &objs = g_engine->_world->getObjects();
	SaveRecords f;
	int32 v = version;
	f.writeInts(0, &v, 1);
	// The world keeps its objects in a CMapWordToPtr; the records follow its
	// iteration order (as the original's save files show).
	WordMap<GameObject *> map;
	for (uint i = 0; i < objs.size(); i++)
		map.at(objs[i]->getId()) = objs[i];
	for (uint32 p = map.start(); p; p = map.nextPos(p))
		map.valueAt(p)->_vars.save(f);
	data = f._bytes;
}

bool GameFiles::loadWorld(const Common::Array<byte> &data, uint32 version) {
	const Common::Array<GameObject *> &objs = g_engine->_world->getObjects();
	SaveRecords f;
	f._bytes = data;
	Common::Array<int32> v;
	if (!f.readInts(0, v) || v.empty() || (uint32)v[0] != version) {
		warning("Files: save file version %d, expected %d", v.empty() ? -1 : v[0], version);
		return false;
	}
	for (uint i = 0; i < objs.size(); i++)
		objs[i]->_vars.load(f);
	return true;
}

bool GameFiles::saveWorld(const Common::String &file, uint32 version) {
	Common::Array<byte> data;
	saveWorld(data, version);
	return writeFile(file, data);
}

bool GameFiles::loadWorld(const Common::String &file, uint32 version) {
	Common::Array<byte> data;
	if (!readFile(file, data)) {
		debug(1, "Files: can't load %s", file.c_str());
		return false;
	}
	return loadWorld(data, version);
}

} // End of namespace Ivanoff
