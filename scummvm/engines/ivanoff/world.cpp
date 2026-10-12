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
#include "common/system.h"

#include "ivanoff/ivanoff.h"
#include "ivanoff/machine.h"
#include "ivanoff/sprite.h"
#include "ivanoff/world.h"

namespace Ivanoff {

// --- GameObject ---

GameObject::~GameObject() {
	for (uint i = 0; i < _children.size(); i++)
		delete _children[i];
}

int32 GameObject::send(uint16 target, uint16 msg, int32 p1, int32 p2, int32 p3) {
	Message m(target, 0, msg, p1, p2, p3);
	return g_engine->_world->deliver(this, m);
}

void GameObject::post(uint16 target, uint16 msg, int32 p1, int32 p2, int32 p3) {
	Message m(target == kSelf ? _id : target, 0, msg, p1, p2, p3);
	g_engine->_world->post(m);
}

int32 GameObject::cmd(uint16 target, int32 command, uint16 childId, int32 p1, int32 p2, int32 p3) {
	if (command == 0)
		return 0;
	Message m(target, command, childId, p1, p2, p3);
	return g_engine->_world->deliverCommand(this, m);
}

void GameObject::startTimer(uint16 timerId, int32 ms, const Message &msg) {
	if (timerId == 0 || ms <= 0)
		return;
	Message m = msg;
	if (m.target == kSelf)
		m.target = _id;
	g_engine->_world->startTimer(this, timerId, ms, m);
}

void GameObject::startTimer(uint16 timerId, int32 ms, uint16 target, uint16 msg, int32 p1, int32 p2, int32 p3) {
	startTimer(timerId, ms, Message(target, 0, msg, p1, p2, p3, 1));
}

void GameObject::stopTimer(uint16 timerId) {
	if (timerId != 0)
		g_engine->_world->stopTimer(this, timerId);
}

void GameObject::setCursor(uint32 cursor) {
	g_engine->setGlobalCursor(cursor);
}

GameVar *GameObject::var(uint16 id) {
	return _vars.get(id);
}

GameVar *GameObject::var(uint16 id, uint16 objectId) {
	// FUN_00477060: a variable of another object.
	GameObject *obj = g_engine->_world->get(objectId);
	if (!obj)
		error("var(%d, %d): no such object", id, objectId);
	return obj->var(id);
}

Child *GameObject::addChild(Child *child) {
	child->setOwner(this);
	_children.push_back(child);
	return child;
}

Child *GameObject::getChild(uint16 id) {
	for (uint i = 0; i < _children.size(); i++)
		if (_children[i]->getId() == id)
			return _children[i];
	return nullptr;
}

void GameObject::dispatchChildCommand(Message &msg) {
	// FUN_0047c6b0
	uint16 target = msg.msg;
	if (target == 0) {
		for (uint i = 0; i < _children.size(); i++)
			_children[i]->handleCommand(msg);
	} else if (target < kTypeAnim) {
		Child *c = getChild(target);
		if (c)
			c->handleCommand(msg);
		else
			debug(2, "Object %d: command %d for missing child %d", _id, msg.cmd, target);
	} else {
		// A type (0xfdff-0xfe05) or a tag set with command 0x29 (FUN_00477910).
		for (uint i = 0; i < _children.size(); i++)
			if (_children[i]->testBit(target))
				_children[i]->handleCommand(msg);
	}
}

// --- World ---

World::~World() {
	for (uint i = 0; i < _order.size(); i++)
		delete _order[i];
}

void World::add(GameObject *obj) {
	_objects[obj->getId()] = obj;
	_order.push_back(obj);
}

GameObject *World::get(uint16 id) const {
	return _objects.contains(id) ? _objects[id] : nullptr;
}

int32 World::deliver(GameObject *from, Message &msg) {
	if (msg.target == kSelf)
		msg.target = from ? from->getId() : kBroadcast;
	msg.mode = 1;
	if (msg.msg != kMsgMouseMove)
		debug(3, "msg %d -> %d (%d, %d, %d)", msg.msg, msg.target, msg.p1, msg.p2, msg.p3);

	if (msg.target == kBroadcast) {
		// FUN_0047bc20: every object gets its own copy.
		for (uint i = 0; i < _order.size(); i++) {
			Message copy = msg;
			if (_order[i]->accepts(copy))
				_order[i]->handle(copy);
		}
		msg.result = 0;
		return 0;
	}

	GameObject *obj = get(msg.target);
	if (!obj) {
		debug(2, "Message %d to missing object %d", msg.msg, msg.target);
		return 0;
	}
	if (obj->accepts(msg))
		obj->handle(msg);
	return msg.result;
}

int32 World::deliverCommand(GameObject *from, Message &msg) {
	if (msg.cmd == 0)
		return 0;
	if (msg.target == kSelf)
		msg.target = from ? from->getId() : kBroadcast;
	msg.mode = 2;
	if (gDebugLevel >= 4) {
		// Text commands pass strings: print them, not their (varying) addresses.
		Emu::Memory &mem = g_engine->_machine->mem();
		if (msg.cmd == 9 && (msg.trigger || (msg.p2 && mem.isMapped(msg.p2)))) {
			// Frame triggers pass a message.
			Message t;
			if (msg.trigger)
				t = *msg.trigger;
			else
				g_engine->_machine->readMessage(msg.p2, t);
			debug(4, "cmd %d -> %d.%d (%d, [%d %d %d %d (%d, %d, %d)], %d)", msg.cmd, msg.target, msg.msg, msg.p1,
			      t.target, t.mode, t.cmd, t.msg, t.p1, t.p2, t.p3, msg.p3);
		} else if ((msg.cmd == 0xd || msg.cmd == 0xf) && msg.text) {
			debug(4, "cmd %d -> %d.%d (\"%s\", %d, %d)", msg.cmd, msg.target, msg.msg, Common::String(msg.text).substr(0, 255).c_str(), msg.p2, msg.p3);
		} else if ((msg.cmd == 0xd || msg.cmd == 0xf) && msg.p1 && mem.isMapped(msg.p1))
			debug(4, "cmd %d -> %d.%d (\"%s\", %d, %d)", msg.cmd, msg.target, msg.msg, mem.readString(msg.p1, 256).c_str(), msg.p2, msg.p3);
		else
			debug(4, "cmd %d -> %d.%d (%d, %d, %d)", msg.cmd, msg.target, msg.msg, msg.p1, msg.p2, msg.p3);
	}

	if (msg.target == kBroadcast) {
		for (uint i = 0; i < _order.size(); i++) {
			Message copy = msg;
			_order[i]->dispatchChildCommand(copy);
		}
		msg.result = 0;
		return 0;
	}

	GameObject *obj = get(msg.target);
	if (!obj)
		return 0;
	obj->dispatchChildCommand(msg);
	return msg.result;
}

void World::deliverFiltered(Message &msg) {
	// FUN_0047bde0: key events go to the objects that subscribe to them.
	uint32 bit = msg.msg == kMsgKeyUp ? 0x20000 : msg.msg == kMsgKeyDown ? 0x10000 : 0x40000;
	for (uint i = 0; i < _order.size(); i++) {
		GameObject *o = _order[i];
		if (!(o->eventMask() & bit))
			continue;
		if (_keyFocus && _keyFocus != o->getId())
			continue;
		Message copy = msg;
		copy.target = o->getId();
		if (o->accepts(copy))
			o->handle(copy);
	}
}

void World::post(const Message &msg) {
	_posted.push_back(msg);
}

void World::startTimer(GameObject *owner, uint16 id, uint32 ms, const Message &msg) {
	stopTimer(owner, id);
	Timer t;
	t.owner = owner;
	t.id = id;
	t.due = g_engine->getMillis() + ms;
	t.msg = msg;
	_timers.push_back(t);
}

void World::stopTimer(GameObject *owner, uint16 id) {
	for (Common::List<Timer>::iterator it = _timers.begin(); it != _timers.end(); ) {
		if (it->owner == owner && it->id == id)
			it = _timers.erase(it);
		else
			++it;
	}
}

void World::update() {
	uint32 now = g_engine->getMillis();

	// FUN_0047eb90: fire due timers. Timer messages built with mode 1 are
	// object messages, mode 2 child commands.
	for (Common::List<Timer>::iterator it = _timers.begin(); it != _timers.end(); ) {
		if (it->due <= now) {
			Message m = it->msg;
			it = _timers.erase(it);
			if (m.cmd != 0)
				deliverCommand(nullptr, m);
			else
				deliver(nullptr, m);
		} else {
			++it;
		}
	}

	// Posted messages (WM_APP+5).
	while (!_posted.empty()) {
		Message m = _posted.front();
		_posted.pop_front();
		deliver(nullptr, m);
	}
}

} // End of namespace Ivanoff
