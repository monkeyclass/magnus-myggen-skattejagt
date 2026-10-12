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

#include "common/endian.h"

#include "ivanoff/ivanoff.h"
#include "ivanoff/vars.h"

namespace Ivanoff {

/** _rand() of the original (the engine's random source), 0-0x7fff. */
static int32 gameRand() {
	return g_engine->getRandomNumber(0x7fff);
}

/** strtol(s, nullptr, 10) as the original's FUN_004915de. */
static int32 parseNumber(const Common::String &s) {
	return (int32)strtol(s.c_str(), nullptr, 10);
}

// --- SaveRecords ---

void SaveRecords::write(uint32 key, const Common::Array<byte> &data) {
	byte h[8];
	WRITE_LE_UINT32(h, key);
	WRITE_LE_UINT32(h + 4, data.size());
	for (int i = 0; i < 8; i++)
		_bytes.push_back(h[i]);
	for (uint i = 0; i < data.size(); i++)
		_bytes.push_back(data[i]);
}

void SaveRecords::writeInts(uint32 key, const int32 *values, uint n) {
	Common::Array<byte> data(n * 4);
	for (uint i = 0; i < n; i++)
		WRITE_LE_UINT32(&data[i * 4], (uint32)values[i]);
	write(key, data);
}

bool SaveRecords::read(uint32 key, Common::Array<byte> &data) const {
	uint pos = 0;
	while (pos + 8 <= _bytes.size()) {
		uint32 k = READ_LE_UINT32(&_bytes[pos]);
		uint32 size = READ_LE_UINT32(&_bytes[pos + 4]);
		if (pos + 8 + size > _bytes.size())
			break;
		if (k == key) {
			data.resize(size);
			if (size)
				memcpy(data.data(), &_bytes[pos + 8], size);
			return true;
		}
		pos += 8 + size;
	}
	return false;
}

bool SaveRecords::readInts(uint32 key, Common::Array<int32> &values) const {
	Common::Array<byte> data;
	if (!read(key, data))
		return false;
	values.resize(data.size() / 4);
	for (uint i = 0; i < values.size(); i++)
		values[i] = (int32)READ_LE_UINT32(&data[i * 4]);
	return true;
}

// --- IntVar (vtable 0x4b5450) ---

void IntVar::setString(const Common::String &s) {
	// FUN_0047c790
	_value = parseNumber(s);
}

Common::String IntVar::toString() {
	// FUN_0047c7c0
	_string = Common::String::format("%d", _value);
	return _string;
}

void IntVar::save(SaveRecords &f) {
	// FUN_0047c940
	if (_save)
		f.writeInts(key(), &_value, 1);
}

void IntVar::load(const SaveRecords &f) {
	// FUN_0047c970
	Common::Array<int32> v;
	if (_save && f.readInts(key(), v) && v.size() >= 1)
		setRaw(v[0]);
}

// --- StringVar (vtable 0x4b54bc) ---

void StringVar::setRaw(int32 v) {
	// FUN_0047ce30
	_value = v;
	_string = Common::String::format("%d", v);
}

int32 StringVar::get() {
	// FUN_0047ce50
	_value = parseNumber(_string);
	return _value;
}

void StringVar::save(SaveRecords &f) {
	// FUN_0047ce80: the string with its terminator.
	if (!_save)
		return;
	Common::Array<byte> data(_string.size() + 1);
	memcpy(data.data(), _string.c_str(), _string.size() + 1);
	f.write(key(), data);
}

void StringVar::load(const SaveRecords &f) {
	// FUN_0047cee0
	Common::Array<byte> data;
	if (!_save || !f.read(key(), data))
		return;
	data.push_back(0);
	setString(Common::String((const char *)data.data()));
}

// --- RandomVar (vtable 0x4b6030) ---

void RandomVar::roll() {
	// FUN_0047cf50
	if (_min < 0)
		_min = 0;
	if (_max < _min)
		_max = _min;
	_value = ((_max - _min + 1) * gameRand()) / 0x7fff + _min;
}

Common::String RandomVar::toString() {
	// FUN_0047d050
	roll();
	_string = Common::String::format("%d", _value);
	return _string;
}

void RandomVar::save(SaveRecords &f) {
	// FUN_0047d0a0
	if (!_save)
		return;
	int32 v[2] = { _max, _min };
	f.writeInts(key(), v, 2);
}

void RandomVar::load(const SaveRecords &f) {
	// FUN_0047d0f0
	Common::Array<int32> v;
	if (_save && f.readInts(key(), v) && v.size() >= 2) {
		_max = v[0];
		_min = v[1];
	}
}

// --- CyclerVar (vtable 0x4b60a0) ---

int32 CyclerVar::get() {
	// FUN_0047d230 / FUN_0047d140: the value, then a step towards max.
	int32 v = _value;
	if (_max < _min)
		_value = _value <= _max ? _min : _value - 1;
	else
		_value = _max <= _value ? _min : _value + 1;
	return v;
}

Common::String CyclerVar::toString() {
	// FUN_0047d240: the text is the value before the step.
	_string = Common::String::format("%d", _value);
	get();
	return _string;
}

void CyclerVar::save(SaveRecords &f) {
	// FUN_0047d2a0
	if (!_save)
		return;
	int32 v[3] = { _value, _max, _min };
	f.writeInts(key(), v, 3);
}

void CyclerVar::load(const SaveRecords &f) {
	// FUN_0047d2f0
	Common::Array<int32> v;
	if (_save && f.readInts(key(), v) && v.size() >= 3) {
		_value = v[0];
		_max = v[1];
		_min = v[2];
	}
}

// --- ShuffleVar (vtable 0x4b6110) ---

ShuffleVar::ShuffleVar(uint16 id, uint16 owner, int32 min, int32 max) : IntVar(id, owner) {
	// FUN_00476f40: set(max), then FUN_0047d690(min).
	set(max);
	setMin(min);
}

void ShuffleVar::refill() {
	int32 n = _max - _min + 1;
	_pool.resize(MAX(n, 0));
	for (int32 i = 0; i < n; i++)
		_pool[i] = _min + i;
	_used = 0;
}

void ShuffleVar::set(int32 v) {
	// FUN_0047d5c0
	if (v < _min) {
		_max = _min;
		_min = v;
	} else {
		_max = v;
	}
	_value = _min;
	refill();
}

void ShuffleVar::setMin(int32 v) {
	// FUN_0047d690
	if (_max < v) {
		_min = _max;
		_max = v;
	} else {
		_min = v;
	}
	_value = _min;
	refill();
}

int32 ShuffleVar::get() {
	// FUN_0047d640 / FUN_0047d390
	int32 n = _max - _min + 1;
	if (_used == n) {
		for (int32 i = 0; i < n; i++)
			_pool[i] = _min + i;
		_used = 0;
	}
	int32 last = (_max - _min) - _used;
	// FUN_0047d350(0, last); the original can (once in 32768) pick last + 1.
	int32 i = ((last + 1) * gameRand()) / 0x7fff;
	if (i > last)
		i = last;
	if (last < 0 || (uint)last >= _pool.size())
		return _value;
	_value = _pool[i];
	_used++;
	_pool[i] = _pool[last];
	return _value;
}

Common::String ShuffleVar::toString() {
	// FUN_0047d650
	get();
	_string = Common::String::format("%d", _value);
	return _string;
}

void ShuffleVar::save(SaveRecords &f) {
	// FUN_0047d700: min, max, used, then the pool.
	if (!_save)
		return;
	Common::Array<int32> v;
	v.push_back(_min);
	v.push_back(_max);
	v.push_back(_used);
	int32 n = _max - _min + 1;
	for (int32 i = 0; i < n; i++)
		v.push_back(i < (int32)_pool.size() ? _pool[i] : 0);
	f.writeInts(key(), v.data(), v.size());
}

void ShuffleVar::load(const SaveRecords &f) {
	// FUN_0047d7a0
	Common::Array<int32> v;
	if (!_save || !f.readInts(key(), v) || v.size() < 3)
		return;
	_min = v[0];
	_max = v[1];
	_used = v[2];
	_pool.clear();
	for (uint i = 3; i < v.size(); i++)
		_pool.push_back(v[i]);
}

// --- MapVar (vtable 0x4b618c) ---

void MapVar::set(int32 v) {
	// FUN_0047d960
	_map.at(_key) = v;
	_value = v;
}

GameVar *MapVar::at(int32 key) {
	// FUN_0047d980: select the key; the value is 0 when it has none.
	_key = (uint16)key;
	int32 v;
	_value = _map.lookup(_key, v) ? v : 0;
	return this;
}

GameVar *MapVar::next(int32 restart) {
	// FUN_0047d9c0: the next element (key and value); restart starts over
	// without moving the stored position, which stays "before the start".
	if (_pos == 0 || restart)
		_pos = _map.size() ? 0xffffffff : 0;
	uint32 pos = _pos;
	if (pos == 0)
		return this; // the original reads a null element here
	if (pos == 0xffffffff)
		pos = _map.start();
	_key = _map.keyAt(pos);
	_value = _map.valueAt(pos);
	uint32 nextPos = _map.nextPos(pos);
	if (!restart)
		_pos = nextPos;
	return this;
}

void MapVar::save(SaveRecords &f) {
	// FUN_0047da30: the current key, then (key, value) pairs.
	if (!_save)
		return;
	Common::Array<int32> v;
	v.push_back(_key);
	for (uint32 p = _map.start(); p; p = _map.nextPos(p)) {
		v.push_back(_map.keyAt(p));
		v.push_back(_map.valueAt(p));
	}
	f.writeInts(key(), v.data(), v.size());
}

void MapVar::load(const SaveRecords &f) {
	// FUN_0047db10
	Common::Array<int32> v;
	if (!_save || !f.readInts(key(), v))
		return;
	if (v.size() >= 1)
		_key = (uint16)v[0];
	_map.clear();
	for (uint i = 1; i + 1 < v.size(); i += 2) {
		_map.at((uint16)v[i]) = v[i + 1];
		_value = v[i + 1];
	}
}

// --- VarContainer ---

VarContainer::~VarContainer() {
	for (uint i = 0; i < _all.size(); i++)
		delete _all[i];
}

void VarContainer::add(GameVar *v) {
	_vars.at(v->getId()) = v;
	_all.push_back(v);
}

bool VarContainer::has(uint16 id) const {
	GameVar *v;
	return _vars.lookup(id, v);
}

GameVar *VarContainer::get(uint16 id) {
	GameVar *v;
	if (_vars.lookup(id, v))
		return v;
	_placeholder.set(0);
	_placeholder.assignString("");
	return &_placeholder;
}

void VarContainer::save(SaveRecords &f) {
	// FUN_0047c290: in the map's order.
	for (uint32 p = _vars.start(); p; p = _vars.nextPos(p))
		_vars.valueAt(p)->save(f);
}

void VarContainer::load(const SaveRecords &f) {
	// FUN_0047c300
	for (uint32 p = _vars.start(); p; p = _vars.nextPos(p))
		_vars.valueAt(p)->load(f);
}

} // End of namespace Ivanoff
