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

#include "common/textconsole.h"

#include "ivanoff/ivanoff.h"
#include "ivanoff/machine.h"
#include "ivanoff/script.h"
#include "ivanoff/sprite.h"

namespace Ivanoff {

enum {
	kVarLookup = 0x477050,
	kVarLookupOf = 0x477060,
	kCStringDtor = 0x49fec5
};

// --- VarRef ---

uint32 VarRef::callSlot(uint32 slot, uint32 nargs, const uint32 *args) const {
	if (!_addr) {
		// The original reads the vtable of a null pointer.
		g_engine->debugFault("unmapped address");
		warning("VarRef: call of +%02x on a missing variable", slot);
		return 0;
	}
	Emu::Memory &mem = _machine->mem();
	return _machine->cpu().call(mem.r32(mem.r32(_addr) + slot), _addr, nargs, args);
}

int32 VarRef::get() const {
	return (int32)callSlot(0x38);
}

void VarRef::set(int32 value) {
	uint32 a = value;
	callSlot(0x2c, 1, &a);
}

void VarRef::add(int32 n) {
	uint32 a = n;
	callSlot(0x30, 1, &a);
}

void VarRef::sub(int32 n) {
	uint32 a = n;
	callSlot(0x34, 1, &a);
}

VarRef VarRef::at(int32 key) const {
	uint32 a = key;
	return VarRef(_machine, callSlot(0x40, 1, &a));
}

void VarRef::remove(int32 key) {
	uint32 a = key;
	callSlot(0x44, 1, &a);
}

bool VarRef::eq(const VarRef &other) const {
	uint32 a = other._addr;
	return (callSlot(0x0c, 1, &a) & 0xff) != 0;
}

void VarRef::callCopySlot(uint32 slot) {
	// The copy is a variable object (0x1c bytes, FUN_0047cad0) with a
	// CString at +0x0c. The original keeps it on the stack; so does this,
	// which keeps the heap as the original leaves it.
	Emu::CPU &cpu = _machine->cpu();
	cpu.r[Emu::ESP] -= 0x20;
	uint32 out = cpu.r[Emu::ESP];
	_machine->mem().w32(out, 0);
	uint32 arg = out;
	callSlot(slot, 1, &arg);
	cpu.call(kCStringDtor, out + 0xc);
	cpu.r[Emu::ESP] += 0x20;
}

// --- Script ---

Script::Script(EmuObject *obj) : _obj(obj), _machine(g_engine->_machine) {
}

uint16 Script::getId() const {
	return _obj->getId();
}

uint32 Script::self() const {
	return _obj->getAddr();
}

int32 Script::send(uint16 target, uint16 msg, int32 p1, int32 p2, int32 p3) {
	Message m(target, 0, msg, p1, p2, p3);
	if (m.target == kSelf)
		m.target = getId();
	g_engine->_world->deliver(_obj, m);
	return m.result;
}

void Script::post(uint16 target, uint16 msg, int32 p1, int32 p2, int32 p3) {
	Message m(target, 0, msg, p1, p2, p3);
	if (m.target == kSelf)
		m.target = getId();
	g_engine->_world->post(m);
}

int32 Script::cmd(uint16 target, int32 command, uint16 child, int32 p1, int32 p2, int32 p3) {
	if (command == 0)
		return 0;
	Message m(target, command, child, p1, p2, p3);
	if (m.target == kSelf)
		m.target = getId();
	g_engine->_world->deliverCommand(_obj, m);
	return m.result;
}

int32 Script::cmdString(uint16 target, int32 command, uint16 child, const Common::String &str, int32 p2, int32 p3) {
	Emu::Memory &mem = _machine->mem();
	uint32 a = mem.alloc(str.size() + 1);
	mem.write(a, str.c_str(), str.size() + 1);
	int32 result = cmd(target, command, child, (int32)a, p2, p3);
	mem.free(a);
	return result;
}

VarRef Script::var(uint16 id) {
	return VarRef(_machine, _machine->cpu().call(kVarLookup, self(), id));
}

VarRef Script::var(uint16 id, uint16 objectId) {
	return VarRef(_machine, _machine->cpu().call(kVarLookupOf, self(), id, objectId));
}

void Script::setCursor(uint32 cursor) {
	g_engine->setGlobalCursor(cursor);
}

void Script::startTimer(uint16 timerId, int32 ms, uint16 target, uint16 msg, int32 p1, int32 p2, int32 p3) {
	// FUN_00476bc0 makes the message (target kSelf is this object, mode 1),
	// FUN_00477140 ignores timers without id or time.
	if (!timerId || ms <= 0)
		return;
	Message m(target == kSelf ? getId() : target, 0, msg, p1, p2, p3, 1);
	_obj->startTimer(timerId, ms, m);
}

void Script::startCmdTimer(uint16 timerId, int32 ms, uint16 target, int32 command, uint16 child, int32 p1, int32 p2, int32 p3) {
	if (!timerId || ms <= 0)
		return;
	Message m(target == kSelf ? getId() : target, command, child, p1, p2, p3, 2);
	_obj->startTimer(timerId, ms, m);
}

void Script::stopTimer(uint16 timerId) {
	_obj->stopTimer(timerId);
}

void Script::setKeyFocus(uint16 objectId) {
	g_engine->_world->_keyFocus = objectId;
}

Child *Script::getChild(uint16 id, uint16 objectId) {
	GameObject *obj = objectId == kSelf ? nullptr : g_engine->_world->get(objectId);
	if (!obj)
		obj = _obj;
	return obj->getChild(id);
}

void Script::callHandler(uint32 addr, Message &msg) {
	uint32 m = _machine->allocMessage(msg);
	_machine->cpu().call(addr, self(), m);
	_machine->readMessage(m, msg);
	_machine->mem().free(m);
}

bool Script::callAccepts(uint32 addr, Message &msg) {
	uint32 m = _machine->allocMessage(msg);
	bool ok = _machine->cpu().call(addr, self(), m) != 0;
	_machine->readMessage(m, msg);
	_machine->mem().free(m);
	return ok;
}

uint32 Script::callGame(uint32 addr, uint32 nargs, const uint32 *args) {
	return _machine->cpu().call(addr, self(), nargs, args);
}

int32 Script::global32(uint32 addr) const {
	return (int32)_machine->mem().r32(addr);
}

void Script::setGlobal32(uint32 addr, int32 value) {
	_machine->mem().w32(addr, value);
}

} // End of namespace Ivanoff
