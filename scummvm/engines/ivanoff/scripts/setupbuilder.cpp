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

#include "ivanoff/ivanoff.h"
#include "ivanoff/machine.h"
#include "ivanoff/scripts/setup.h"

namespace Ivanoff {

void SetupBuilder::object(uint16 id, uint32 vtable) {
	_obj = _machine->newObject(id, vtable);
}

void SetupBuilder::registerObject() {
	_machine->registerObject(_obj);
}

void SetupBuilder::addInt(uint16 id, int32 value, bool save) {
	GameVar *v = new IntVar(id, _obj->getId());
	v->set(value);
	v->_save = save;
	_obj->_vars.add(v);
}

void SetupBuilder::addString(uint16 id, const char *value, bool save) {
	GameVar *v = new StringVar(id, _obj->getId());
	v->assignString(value);
	v->_save = save;
	_obj->_vars.add(v);
}

void SetupBuilder::addRandom(uint16 id, int32 min, int32 max, bool save) {
	GameVar *v = new RandomVar(id, _obj->getId(), min, max);
	v->_save = save;
	_obj->_vars.add(v);
}

void SetupBuilder::addCycler(uint16 id, int32 min, int32 max, bool save) {
	GameVar *v = new CyclerVar(id, _obj->getId(), min, max);
	v->_save = save;
	_obj->_vars.add(v);
}

void SetupBuilder::addShuffle(uint16 id, int32 min, int32 max, bool save) {
	GameVar *v = new ShuffleVar(id, _obj->getId(), min, max);
	v->_save = save;
	_obj->_vars.add(v);
}

void SetupBuilder::addMap(uint16 id, bool save) {
	GameVar *v = new MapVar(id, _obj->getId());
	v->_save = save;
	_obj->_vars.add(v);
}

void SetupBuilder::add(Child *c) {
	_obj->addChild(c);
	_last = c;
}

void SetupBuilder::add(uint16 objectId, Child *c) {
	GameObject *obj = g_engine->_world->get(objectId);
	if (!obj)
		error("SetupBuilder: no object %d", objectId);
	obj->addChild(c);
	_last = c;
}

void SetupBuilder::setMode(Child *c, int32 mode) {
	if (Sprite *s = dynamic_cast<Sprite *>(c))
		s->_mode = mode;
}

void SetupBuilder::setInitialPos(Child *c, int32 x, int32 y) {
	if (Sprite *s = dynamic_cast<Sprite *>(c))
		s->setInitialPos(x, y);
}

void SetupBuilder::frameTrigger(Child *c, int32 frame, const Message &msg) {
	if (Anim *a = dynamic_cast<Anim *>(c))
		a->setFrameTrigger(frame, &msg);
}

void SetupBuilder::frameTrigger(Child *c, int32 frame) {
	if (Anim *a = dynamic_cast<Anim *>(c))
		a->setFrameTrigger(frame, nullptr);
}

} // End of namespace Ivanoff
