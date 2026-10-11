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

#ifndef IVANOFF_SCRIPTS_SCRIPTS_H
#define IVANOFF_SCRIPTS_SCRIPTS_H

#include "ivanoff/script.h"

namespace Ivanoff {

/** Cursors (resources of mm2run.exe, see IvanoffEngine::loadCursors). */
enum {
	kCursorWait = 0x80000002,
	kCursorDropYes = 0x80000006,
	kCursorDropNo = 0x80000007,
	kCursorDrag = 0x80000008
};

/** The game state object (17) and its variables. */
enum {
	kVarBusy = 2,      ///< object 17: an animation or speech is running
	kVarDragMode = 4   ///< object 17: 1 while an item is dragged [unverified]
};

/**
 * The base class of the game's objects (vtable 0x4b51a4, slot 6
 * FUN_0041b910): animations that end, speech, and the default answers to
 * the drag and drop questions.
 */
class ObjectScript : public Script {
public:
	ObjectScript(EmuObject *obj) : Script(obj) {}
	void handle(Message &msg) override;
};

/**
 * An item that lies in a room until it is picked up (accepts FUN_00423a60,
 * handler FUN_00423aa0): the objects 63-103 and others derive from it.
 *
 * Variables: 0x2d picked up, 0x2e saved position, 0x2f, 0x30 the sprite
 * in the inventory, 0x31 used, 0x32.
 */
class ItemScript : public ObjectScript {
public:
	ItemScript(EmuObject *obj) : ObjectScript(obj) {}
	bool accepts(Message &msg) override;
	void handle(Message &msg) override;
};

/**
 * Object 99 (vtable 0x4b45e4, FUN_004532e0), an item that takes the items
 * 0x62 and 0x64-0x6f; the base of objects 100-103 and others.
 */
class Object99 : public ItemScript {
public:
	Object99(EmuObject *obj) : ItemScript(obj) {}
	void handle(Message &msg) override;

private:
	static bool takes(int32 p1) {
		int16 item = p1;
		return (item >= 0x64 && item <= 0x6f) || item == 0x62;
	}
};

typedef Script *(*ScriptFactory)(EmuObject *obj);

/** One entry per ported class, by vtable address. */
struct ScriptEntry {
	uint32 vtable;
	ScriptFactory create;
};

#define IVANOFF_SCRIPT(name) \
	Script *create##name(EmuObject *obj)

} // End of namespace Ivanoff

#endif
