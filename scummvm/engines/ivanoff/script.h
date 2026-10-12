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

#ifndef IVANOFF_SCRIPT_H
#define IVANOFF_SCRIPT_H

#include "common/str.h"

#include "ivanoff/vars.h"
#include "ivanoff/world.h"

namespace Ivanoff {

class Child;
class GameObject;

/**
 * A game variable (vars.h): a handle that is cheap to copy. Missing
 * variables are their container's placeholder, as in the original.
 */
class VarRef {
public:
	VarRef() {}
	explicit VarRef(GameVar *v) : _var(v) {}

	bool isValid() const { return _var != nullptr; }
	GameVar *var() const { return _var; }

	/** +0x38. Random, cycler and shuffle variables change when read. */
	int32 get() const { return _var->get(); }
	/** +0x2c */
	void set(int32 value) { _var->set(value); }
	/** +0x30 */
	void add(int32 n) { _var->add(n); }
	/** +0x34 */
	void sub(int32 n) { _var->sub(n); }
	/** +0x40: a map variable with the key selected. */
	VarRef at(int32 key) const { return VarRef(_var->at(key)); }
	/** +0x44: the next element of a map variable. */
	VarRef next(int32 restart) const { return VarRef(_var->next(restart)); }
	/** +0x0c: equal to another variable (compares the values). */
	bool eq(const VarRef &other) const { return _var->eq(*other._var); }
	/** +0x3c: the value as text. */
	Common::String toString() const { return _var->toString(); }
	/** +0x28: assigns a text. */
	void assignString(const Common::String &s) { _var->assignString(s); }
	/** +0x54: v++ (the original returns a copy of the old value, discarded by the game). */
	void postIncrement() { _var->increment(); }
	/** +0x5c: v-- */
	void postDecrement() { _var->decrement(); }

private:
	GameVar *_var = nullptr;
};

/**
 * The native logic of a top-level object: what the original classes do
 * in vtable slots 5 (accepts) and 6 (handle). While the rewrite is in
 * progress, the object itself (its variables and children) is still
 * created by the original setup code in the interpreter.
 *
 * Handlers are written against the original engine API, so they read
 * like the original game code: send(), cmd(), var(), startTimer()...
 */
class Script {
public:
	Script(GameObject *obj) : _obj(obj) {}
	virtual ~Script() {}

	/** Slot 5 (+0x14): does the object take the message? */
	virtual bool accepts(Message &msg) { return true; }
	/** Slot 6 (+0x18): handles the message. */
	virtual void handle(Message &msg) = 0;

	uint16 getId() const;

protected:
	// --- the original engine's API ---

	/** FUN_00476ad0 */
	int32 send(uint16 target, uint16 msg, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0);
	/** FUN_00476a60 */
	void post(uint16 target, uint16 msg, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0);
	/** FUN_00476b40 */
	int32 cmd(uint16 target, int32 command, uint16 child, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0);
	/** cmd 9: an animation sends trigger when it shows frame (FUN_00481b50). */
	void cmdFrameTrigger(uint16 target, uint16 child, int32 frame, const Message &trigger);
	/** FUN_00476bc0 / FUN_00476c50: a message or command to send later (target kSelf: this object). */
	Message makeMsg(uint16 target, uint16 msg, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0) const;
	Message makeCmd(uint16 target, int32 command, uint16 child, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0) const;
	/** Vtable slot 7 of the objects (FUN_00476a40): draws the screen now. */
	void redrawNow();
	/** FUN_00476a30: unloads all children of the object. */
	void unloadChildren();
	/** FUN_004771c0 / FUN_004771e0: saves / loads the game (version 0x10100). */
	bool saveGame(const Common::String &file);
	bool loadGame(const Common::String &file);
	/** FUN_00477200 */
	void quitGame();
	/** +0x18: which key events the object receives. */
	void setEventMask(uint32 mask);
	/** cmd() with a string as p1 (text commands 0xd and 0xf). */
	int32 cmdString(uint16 target, int32 command, uint16 child, const Common::String &str, int32 p2 = 0, int32 p3 = 0);
	/** FUN_00477050 */
	VarRef var(uint16 id);
	/** FUN_00477060: a variable of another object. */
	VarRef var(uint16 id, uint16 objectId);
	/** FUN_00477120 */
	void setCursor(uint32 cursor);
	/** FUN_00476bc0 + FUN_00477140: post a message after ms (target kSelf = this object). */
	void startTimer(uint16 timerId, int32 ms, uint16 target, uint16 msg, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0);
	/** FUN_00476c50 + FUN_00477140: a child command after ms. */
	void startCmdTimer(uint16 timerId, int32 ms, uint16 target, int32 command, uint16 child, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0);
	/** FUN_00477170 */
	void stopTimer(uint16 timerId);
	/** FUN_00477240 */
	void setKeyFocus(uint16 objectId);
	/** FUN_004770a0 / FUN_004770d0 */
	Child *getChild(uint16 id, uint16 objectId = kSelf);

	/** Global variables of the original (DAT_xxxxxxxx). */
	int32 global32(uint32 addr) const;
	void setGlobal32(uint32 addr, int32 value);

	GameObject *_obj;
};

/** The logic of an object of the given class (vtable of the original), or nullptr. */
Script *createScript(GameObject *obj, uint32 vtable);

} // End of namespace Ivanoff

#endif
