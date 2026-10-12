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

#include "common/config-manager.h"
#include "common/textconsole.h"

#include "ivanoff/ivanoff.h"
#include "ivanoff/files.h"
#include "ivanoff/script.h"
#include "ivanoff/sprite.h"

namespace Ivanoff {

// --- Script ---

uint16 Script::getId() const {
	return _obj->getId();
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
	if (command == 0)
		return 0;
	Message m(target, command, child, 0, p2, p3);
	m.text = str.c_str();
	if (m.target == kSelf)
		m.target = getId();
	g_engine->_world->deliverCommand(_obj, m);
	return m.result;
}

void Script::cmdFrameTrigger(uint16 target, uint16 child, int32 frame, const Message &trigger) {
	Message m(target, 9, child, frame);
	m.trigger = &trigger;
	if (m.target == kSelf)
		m.target = getId();
	g_engine->_world->deliverCommand(_obj, m);
}

Message Script::makeMsg(uint16 target, uint16 msg, int32 p1, int32 p2, int32 p3) const {
	return Message(target == kSelf ? getId() : target, 0, msg, p1, p2, p3, 1);
}

Message Script::makeCmd(uint16 target, int32 command, uint16 child, int32 p1, int32 p2, int32 p3) const {
	return Message(target == kSelf ? getId() : target, command, child, p1, p2, p3, 2);
}

void Script::redrawNow() {
	g_engine->redrawNow();
}

void Script::unloadChildren() {
	const Common::Array<Child *> &ch = _obj->getChildren();
	for (uint i = 0; i < ch.size(); i++) {
		Message m(getId(), kCmdUnload, ch[i]->getId());
		ch[i]->handleCommand(m);
	}
}

bool Script::saveGame(const Common::String &file) {
	return g_engine->_files->saveWorld(file, 0x10100);
}

bool Script::loadGame(const Common::String &file) {
	return g_engine->_files->loadWorld(file, 0x10100);
}

void Script::quitGame() {
	g_engine->quitGame();
}

void Script::setEventMask(uint32 mask) {
	_obj->_flags18 = mask;
}


VarRef Script::var(uint16 id) {
	return VarRef(_obj->var(id));
}

VarRef Script::var(uint16 id, uint16 objectId) {
	// FUN_00477060. The fuzzer's messages name objects that don't exist;
	// the original would read through a null object.
	GameObject *obj = g_engine->_world->get(objectId);
	if (!obj && ConfMan.hasKey("fuzz_dict"))
		obj = g_engine->_world->get(kEngineObject);
	if (!obj) {
		g_engine->debugFault("unmapped address");
		error("Script: variable %d of missing object %d", id, objectId);
	}
	return VarRef(obj->var(id));
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

int32 Script::global32(uint32 addr) const {
	return g_engine->getGlobal(addr);
}

void Script::setGlobal32(uint32 addr, int32 value) {
	g_engine->setGlobal(addr, value);
}

} // End of namespace Ivanoff
