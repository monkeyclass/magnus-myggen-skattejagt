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

#ifndef IVANOFF_MACHINE_H
#define IVANOFF_MACHINE_H

#include "common/file.h"
#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/path.h"
#include "common/str.h"

#include "ivanoff/emu/cpu.h"
#include "ivanoff/emu/memory.h"
#include "ivanoff/world.h"

namespace Ivanoff {

class Child;
class Machine;
class Script;

/**
 * A top-level object whose logic is the original game code, run by the
 * interpreter. Its memory (and its variables) live in the emulated address
 * space at _addr.
 */
class EmuObject : public GameObject {
public:
	EmuObject(Machine *m, uint16 id, uint32 addr) : GameObject(id), _machine(m), _addr(addr) {}
	~EmuObject() override;

	bool accepts(Message &msg) override;
	void handle(Message &msg) override;
	uint32 eventMask() override;

	uint32 getAddr() const { return _addr; }
	uint32 _childContainer = 0;
	/** The engine's own object (no game code): ignores messages. */
	bool _native = false;
	/** The native logic replacing the original slots 5 and 6, if ported. */
	Script *_script = nullptr;
	/** The original class (vtable address). */
	uint32 getVtable() const;

private:
	Machine *_machine;
	uint32 _addr;
};

/**
 * Runs the game logic of mm2run.exe: the code of the game's objects and the
 * MFC/C runtime helpers it uses execute in the interpreter, while the engine
 * functions they call (messages, children, timers, cursor) are trapped and
 * implemented natively by World and the Child classes.
 *
 * See docs/ENGINE.md for the original engine's API.
 */
class Machine : public Emu::TrapHandler {
public:
	Machine();
	~Machine() override;

	/** Loads the executable and installs the traps. */
	bool load(const Common::Path &exePath);
	/** Runs the C++ static initializers of the executable (CRT _initterm). */
	void runInitializers();
	/** Creates and registers all objects (natively, or with "native_setup=false" by the original setup routine). */
	void createObjects();
	/** An object of the given class, with its memory in the interpreter (scripts/setup.h). */
	EmuObject *newObject(uint16 id, uint32 vtable);
	void registerObject(EmuObject *obj);
	/** Attaches the native logic of the ported classes ("native_scripts": all, none or a list of vtables). */
	void attachScripts();

	void trap(Emu::CPU &cpu, uint32 addr) override;

	Emu::Memory &mem() { return _mem; }
	Emu::CPU &cpu() { return _cpu; }

	/** Writes msg into an emulated message struct (FUN_0047c3c0). */
	void writeMessage(uint32 addr, const Message &msg);
	void readMessage(uint32 addr, Message &msg);
	uint32 allocMessage(const Message &msg);

	EmuObject *objectAt(uint32 addr) const;
	/** The engine's input object, owner of the sprite cursors. */
	EmuObject *kernel() const { return _kernel; }
	Child *childAt(uint32 addr) const;
	/** The game's INI files (GetPrivateProfileString and the others). */
	bool iniGet(const Common::String &file, const Common::String &section, const Common::String &key, Common::String &value);
	void iniSet(const Common::String &file, const Common::String &section, const Common::String &key, const Common::String *value);
	/** A registry value ("80000002\\software\\...\\name", lower case), or "". */
	Common::String registryValue(const Common::String &path) const {
		return _registry.contains(path) ? _registry[path] : Common::String();
	}

	/** Saves / loads the variables of all objects (the original's .m2s format). */
	bool saveWorld(const Common::String &file, uint32 version);
	bool loadWorld(const Common::String &file, uint32 version);
	/** The interpreter's handle on a native variable, and back. */
	uint32 varShim(GameVar *v);
	GameVar *shimVar(uint32 addr) const;

	/** A path in the game folder for a Windows path (C:\\MM2\\...), or "". */
	Common::String gamePath(const Common::String &winPath) {
		Common::String g, save;
		return mapPath(winPath, g, save) ? g : Common::String();
	}

	/** The id of the object at addr (its +4 field). */
	uint16 objectId(uint32 addr) { return _mem.r16(addr + 4); }

private:
	bool loadPE(const Common::Path &exePath);
	void installEngineTraps();
	void installChildTraps(uint32 vtable, int slots);
	void addTrap(uint32 addr, uint16 cleanup, int id);

	void engineCall(Emu::CPU &cpu, int id);
	void childMethod(Emu::CPU &cpu, uint32 addr);
	void varMethod(Emu::CPU &cpu, uint32 slot);
	/** Constructs an MFC CString at out (FUN_0049ff33). */
	void makeCString(uint32 out, const Common::String &text);
	void importCall(Emu::CPU &cpu, const Common::String &name);

	// Files (machine_files.cpp). C:\MM2\ is the game folder; files in
	// C:\MM2\appdata\ are ScummVM save files.
	struct EmuFile {
		Common::String saveName; ///< for files in appdata, else empty
		Common::Array<byte> data;
		uint32 pos = 0;
		bool dirty = false;
	};
	bool fileCall(Emu::CPU &cpu, const Common::String &name, uint32 &result);
	bool mapPath(const Common::String &winPath, Common::String &gamePath, Common::String &saveName);
	uint32 openFile(const Common::String &winPath, uint32 access, uint32 disposition);
	void closeFile(uint32 handle);
	Common::HashMap<uint32, EmuFile *> _files;
	bool readWholeFile(const Common::String &winPath, Common::String &text);
	void writeWholeFile(const Common::String &winPath, const Common::String &text);
	bool readWholeFile(const Common::String &winPath, Common::Array<byte> &data);
	bool writeWholeFile(const Common::String &winPath, const Common::Array<byte> &data);

	Child *createChild(uint32 addr, Child *child, uint32 vtable);
	void syncChild(Child *c);

	Emu::Memory _mem;
	Emu::CPU _cpu;

	uint32 _imageBase = 0, _imageSize = 0;
	uint32 _world = 0;
	EmuObject *_kernel = nullptr;
	uint32 _app = 0;

	Common::HashMap<uint32, int> _engineTraps;
	Common::HashMap<uint32, Common::String> _imports;
	Common::HashMap<uint32, bool> _childTraps;

	Common::HashMap<uint32, EmuObject *> _objects;
	Common::HashMap<uint32, EmuObject *> _containers;
	Common::HashMap<uint32, Child *> _children;
	uint32 _varStubs = 0, _varVtable = 0;
	Common::DumpFile *_record = nullptr; ///< record_setup
	struct PtrHash {
		uint operator()(const GameVar *p) const { return (uint)(uintptr)p; }
	};
	Common::HashMap<GameVar *, uint32, PtrHash> _shimOf;
	Common::HashMap<uint32, GameVar *> _shimVar;

	uint32 _lastError = 0;
	Common::Array<uint32> _tls;

	// A minimal registry: key paths start with the root key as hex (80000002 = HKLM).
	Common::HashMap<Common::String, bool> _registryKeys;
	Common::HashMap<Common::String, Common::String> _registry;
	Common::HashMap<uint32, Common::String> _regKeys;
	uint32 _nextHandle = 0x1000;
};

} // End of namespace Ivanoff

#endif
