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

#include "ivanoff/ivanoff.h"
#include "ivanoff/machine.h"

namespace Ivanoff {

using Emu::CPU;
using Emu::ESP;

static const uint32 kInvalidHandle = 0xffffffff;

bool Machine::mapPath(const Common::String &winPath, Common::String &gamePath, Common::String &saveName) {
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

uint32 Machine::openFile(const Common::String &winPath, uint32 access, uint32 disposition) {
	Common::String gamePath, saveName;
	if (!mapPath(winPath, gamePath, saveName)) {
		_lastError = 3; // ERROR_PATH_NOT_FOUND
		return kInvalidHandle;
	}
	bool write = (access & 0x40000000) != 0;
	EmuFile *f = new EmuFile();
	f->saveName = saveName;

	Common::SeekableReadStream *in = nullptr;
	if (!saveName.empty()) {
		if (disposition != 2 && disposition != 1) // not CREATE_ALWAYS / CREATE_NEW
			in = g_system->getSavefileManager()->openForLoading(saveName);
	} else {
		if (write) {
			warning("Machine: write to game file %s refused", winPath.c_str());
			delete f;
			_lastError = 5; // ERROR_ACCESS_DENIED
			return kInvalidHandle;
		}
		Common::File *file = new Common::File();
		if (file->open(Common::Path(gamePath, '/')))
			in = file;
		else
			delete file;
	}

	if (in) {
		if (disposition == 1) { // CREATE_NEW on an existing file
			delete in;
			delete f;
			_lastError = 80; // ERROR_FILE_EXISTS
			return kInvalidHandle;
		}
		f->data.resize(in->size());
		if (in->size())
			in->read(&f->data[0], in->size());
		delete in;
		if (disposition == 5) // TRUNCATE_EXISTING
			f->data.clear();
	} else if (disposition == 3 || disposition == 5 || saveName.empty()) {
		delete f;
		_lastError = 2; // ERROR_FILE_NOT_FOUND
		return kInvalidHandle;
	} else {
		f->dirty = true;
	}

	uint32 h = 0x7000 + _nextHandle++;
	_files[h] = f;
	debug(2, "Machine: opened %s (%s) as %x, %d bytes", winPath.c_str(), saveName.empty() ? gamePath.c_str() : saveName.c_str(),
	      h, f->data.size());
	_lastError = 0;
	return h;
}

void Machine::closeFile(uint32 h) {
	EmuFile *f = _files[h];
	if (f->dirty && !f->saveName.empty()) {
		Common::OutSaveFile *out = g_system->getSavefileManager()->openForSaving(f->saveName, false);
		if (out) {
			if (!f->data.empty())
				out->write(&f->data[0], f->data.size());
			out->finalize();
			delete out;
		}
	}
	delete f;
	_files.erase(h);
}

bool Machine::readWholeFile(const Common::String &winPath, Common::String &text) {
	uint32 h = openFile(winPath, 0x80000000, 3);
	if (h == kInvalidHandle)
		return false;
	EmuFile *f = _files[h];
	text = f->data.empty() ? Common::String() : Common::String((const char *)&f->data[0], f->data.size());
	closeFile(h);
	return true;
}

void Machine::writeWholeFile(const Common::String &winPath, const Common::String &text) {
	uint32 h = openFile(winPath, 0xc0000000, 2);
	if (h == kInvalidHandle)
		return;
	EmuFile *f = _files[h];
	f->data.resize(text.size());
	if (text.size())
		memcpy(&f->data[0], text.c_str(), text.size());
	f->dirty = true;
	closeFile(h);
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

bool Machine::iniGet(const Common::String &file, const Common::String &section, const Common::String &key, Common::String &value) {
	Common::String text;
	if (!readWholeFile(file, text))
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

void Machine::iniSet(const Common::String &file, const Common::String &section, const Common::String &key, const Common::String *value) {
	Common::String text;
	readWholeFile(file, text);
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
	writeWholeFile(file, out);
}

bool Machine::fileCall(CPU &cpu, const Common::String &name, uint32 &result) {
	if (name == "KERNEL32.DLL!GetPrivateProfileStringA") {
		Common::String file = _mem.readString(cpu.arg(5));
		Common::String value;
		if (!cpu.arg(0) || !cpu.arg(1) ||
		    !iniGet(file, _mem.readString(cpu.arg(0)), _mem.readString(cpu.arg(1)), value))
			value = _mem.readString(cpu.arg(2));
		uint32 n = cpu.arg(4);
		if (n > 0) {
			if (value.size() >= n)
				value = Common::String(value.c_str(), n - 1);
			_mem.writeString(cpu.arg(3), value);
		}
		result = value.size();
	} else if (name == "KERNEL32.DLL!GetPrivateProfileIntA") {
		Common::String value;
		if (iniGet(_mem.readString(cpu.arg(3)), _mem.readString(cpu.arg(0)), _mem.readString(cpu.arg(1)), value))
			result = (uint32)atoi(value.c_str());
		else
			result = cpu.arg(2);
	} else if (name == "KERNEL32.DLL!WritePrivateProfileStringA") {
		Common::String value = _mem.readString(cpu.arg(2));
		iniSet(_mem.readString(cpu.arg(3)), _mem.readString(cpu.arg(0)), _mem.readString(cpu.arg(1)),
		       cpu.arg(2) ? &value : nullptr);
		result = 1;
	} else if (name == "KERNEL32.DLL!CreateFileA") {
		result = openFile(_mem.readString(cpu.arg(0)), cpu.arg(1), cpu.arg(4));
	} else if (name == "KERNEL32.DLL!GetFileAttributesA") {
		Common::String gamePath, saveName;
		Common::String path = _mem.readString(cpu.arg(0));
		result = 0xffffffff;
		if (mapPath(path, gamePath, saveName)) {
			if (!saveName.empty()) {
				if (saveName.hasSuffix("-") || saveName.hasSuffix("_"))
					result = 0x10; // the folder itself
				else if (g_system->getSavefileManager()->listSavefiles(saveName).size())
					result = 0x80;
			} else if (gamePath.empty() || Common::File::exists(Common::Path(gamePath, '/'))) {
				result = 0x80;
			}
		}
		if (result == 0xffffffff)
			_lastError = 2;
	} else if (name == "KERNEL32.DLL!DeleteFileA") {
		Common::String gamePath, saveName;
		result = 0;
		if (mapPath(_mem.readString(cpu.arg(0)), gamePath, saveName) && !saveName.empty())
			result = g_system->getSavefileManager()->removeSavefile(saveName) ? 1 : 0;
	} else if (name == "KERNEL32.DLL!ReadFile" || name == "KERNEL32.DLL!WriteFile") {
		if (!_files.contains(cpu.arg(0))) {
			result = 0;
			_lastError = 6; // ERROR_INVALID_HANDLE
			return true;
		}
		EmuFile *f = _files[cpu.arg(0)];
		uint32 buf = cpu.arg(1), n = cpu.arg(2);
		if (name.hasSuffix("ReadFile")) {
			n = MIN<uint32>(n, f->pos < f->data.size() ? f->data.size() - f->pos : 0);
			if (n)
				_mem.write(buf, &f->data[f->pos], n);
		} else {
			if (f->pos + n > f->data.size())
				f->data.resize(f->pos + n);
			if (n)
				_mem.read(buf, &f->data[f->pos], n);
			f->dirty = true;
		}
		f->pos += n;
		if (cpu.arg(3))
			_mem.w32(cpu.arg(3), n);
		result = 1;
	} else if (name == "KERNEL32.DLL!SetFilePointer") {
		if (!_files.contains(cpu.arg(0))) {
			result = 0xffffffff;
			return true;
		}
		EmuFile *f = _files[cpu.arg(0)];
		int32 dist = (int32)cpu.arg(1);
		if (cpu.arg(2))
			_mem.w32(cpu.arg(2), 0);
		int64 base = cpu.arg(3) == 0 ? 0 : cpu.arg(3) == 1 ? f->pos : f->data.size();
		f->pos = (uint32)MAX<int64>(0, base + dist);
		result = f->pos;
	} else if (name == "KERNEL32.DLL!GetFileSize") {
		result = _files.contains(cpu.arg(0)) ? _files[cpu.arg(0)]->data.size() : 0xffffffff;
		if (cpu.arg(1))
			_mem.w32(cpu.arg(1), 0);
	} else if (name == "KERNEL32.DLL!SetEndOfFile") {
		if (_files.contains(cpu.arg(0))) {
			EmuFile *f = _files[cpu.arg(0)];
			f->data.resize(f->pos);
			f->dirty = true;
		}
		result = 1;
	} else if (name == "KERNEL32.DLL!CloseHandle") {
		if (_files.contains(cpu.arg(0)))
			closeFile(cpu.arg(0));
		result = 1;
	} else if (name == "KERNEL32.DLL!FlushFileBuffers") {
		result = 1;
	} else if (name == "KERNEL32.DLL!GetFileType") {
		result = 1; // FILE_TYPE_DISK
	} else {
		return false;
	}
	return true;
}

} // End of namespace Ivanoff
