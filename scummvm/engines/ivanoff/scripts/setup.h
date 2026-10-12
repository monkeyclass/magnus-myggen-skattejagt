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

#ifndef IVANOFF_SCRIPTS_SETUP_H
#define IVANOFF_SCRIPTS_SETUP_H

#include "ivanoff/sprite.h"
#include "ivanoff/world.h"

namespace Ivanoff {

class EmuObject;
class Machine;

/**
 * Creates the game's objects, their variables and their children the way
 * the original setup code does (scripts/setup.cpp is generated from it).
 */
class SetupBuilder {
public:
	SetupBuilder(Machine *m) : _machine(m) {}

	/** FUN_00476910 and the class's constructor. */
	void object(uint16 id, uint32 vtable);
	/** FUN_0047bb40 */
	void registerObject();

	// Variables (FUN_00476ce0 and the following).
	void addInt(uint16 id, int32 value, bool save);
	void addString(uint16 id, const char *value, bool save);
	void addRandom(uint16 id, int32 min, int32 max, bool save);
	void addCycler(uint16 id, int32 min, int32 max, bool save);
	void addShuffle(uint16 id, int32 min, int32 max, bool save);
	void addMap(uint16 id, bool save);

	// Children (FUN_00477b20 and the following); add() gives one to the object.
	Child *bitmap(uint16 id, int32 transparent, uint32 flags, int32 mode) { return new Bitmap(id, transparent, flags, mode); }
	Child *anim(uint16 id, int32 transparent, uint32 flags, uint16 param) { return new Anim(id, transparent, flags, param); }
	Child *hotspot(uint16 id, uint32 flags) { return new Hotspot(id, flags); }
	Child *sound(uint16 id, uint16 loops) { return new Sound(id, loops); }
	Child *textResource(uint16 id) { return new TextResource(id); }
	Child *text(uint16 id, int32 fg, int32 bg, const char *font, int32 size, int32 wrap, int32 align,
	            int32 charset, uint32 source, uint32 flags) {
		return new TextSprite(id, fg, bg, font, size, wrap, align, charset, source, flags);
	}
	Child *video(uint16 id, int32 transparent, const char *file, uint32 flags, int32 param) {
		return new VideoSprite(id, transparent, file, flags, param);
	}
	/** FUN_004770a0 */
	void add(Child *c);
	/** A child of another object (the engine's object, 0xfffe). */
	void add(uint16 objectId, Child *c);
	/** The last child added. */
	Child *last() { return _last; }

	// Child settings (vtable slots of the original's classes).
	/** +0x28: the drawing mode. */
	void setMode(Child *c, int32 mode);
	/** +0x78 */
	void setInitialPos(Child *c, int32 x, int32 y);
	/** +0x58: a message sent when an animation shows a frame. */
	void frameTrigger(Child *c, int32 frame, const Message &msg);
	void frameTrigger(Child *c, int32 frame);

private:
	Machine *_machine;
	EmuObject *_obj = nullptr;
	Child *_last = nullptr;
};

/** scripts/setup.cpp */
void createObjects(SetupBuilder &b);

} // End of namespace Ivanoff

#endif
