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

#include "ivanoff/world.h"

namespace Ivanoff {

class Child;
class EmuObject;
class Machine;

/**
 * A game variable (the original's variable classes, FUN_0047ca20 and
 * subclasses). While objects are created by the original setup code the
 * variables live in the interpreter's memory; this handle calls their
 * methods by vtable slot.
 */
class VarRef {
public:
	VarRef() {}
	VarRef(Machine *m, uint32 addr) : _machine(m), _addr(addr) {}

	bool isValid() const { return _addr != 0; }
	uint32 addr() const { return _addr; }

	/** +0x38. Random, cycler and shuffle variables change when read. */
	int32 get() const;
	/** +0x2c */
	void set(int32 value);
	/** +0x30 */
	void add(int32 n);
	/** +0x34 */
	void sub(int32 n);
	/** +0x40: the element of a map variable (created when missing). */
	VarRef at(int32 key) const;
	/** +0x44: removes the element of a map variable. */
	void remove(int32 key);
	/** +0x0c-+0x1c: comparisons with a number. */
	bool eq(int32 value) const;
	/** +0x54: map variables, (key, value). */
	uint32 x54(int32 a, int32 b);
	/** +0x5c: map variables, (key, value). */
	uint32 x5c(int32 a, int32 b);

	/**
	 * +0x54 / +0x5c: methods returning an MFC CString by value (hidden
	 * first argument). Returns the string; the temporary is released.
	 */
	Common::String getString(uint32 slot, int32 a);

	/** Calls the method at vtable offset slot with this variable as this. */
	uint32 callSlot(uint32 slot, uint32 nargs = 0, const uint32 *args = nullptr) const;

private:
	Machine *_machine = nullptr;
	uint32 _addr = 0;
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
	Script(EmuObject *obj);
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

	// --- transition helpers ---

	/** The object's address in the interpreter's memory. */
	uint32 self() const;
	Machine &machine() const { return *_machine; }
	/** Runs a not yet ported handler of the original with this message (thiscall). */
	void callHandler(uint32 addr, Message &msg);
	/** Runs a not yet ported accepts function of the original (thiscall). */
	bool callAccepts(uint32 addr, Message &msg);
	/** Runs a not yet ported function of the original with this as the object. */
	uint32 callGame(uint32 addr, uint32 nargs = 0, const uint32 *args = nullptr);
	/** Global variables of the original (DAT_xxxxxxxx). */
	int32 global32(uint32 addr) const;
	void setGlobal32(uint32 addr, int32 value);

	EmuObject *_obj;
	Machine *_machine;
};

/** The native logic of an object, or nullptr while it is not ported. */
Script *createScript(EmuObject *obj);

} // End of namespace Ivanoff

#endif
