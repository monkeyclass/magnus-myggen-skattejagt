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

#ifndef IVANOFF_VARS_H
#define IVANOFF_VARS_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/str.h"

namespace Ivanoff {

/**
 * A map from 16-bit keys to values that iterates like MFC's
 * CMapWordToPtr (17 buckets by key >> 4, newest first in a bucket):
 * the game's variable containers and map variables are such maps, and
 * their order shows in save files and in map iteration.
 */
template<class T>
class WordMap {
public:
	enum { kBuckets = 17 };

	bool lookup(uint16 key, T &value) const {
		const Common::Array<Entry> &b = _buckets[bucket(key)];
		for (uint i = 0; i < b.size(); i++)
			if (b[i].key == key) {
				value = b[i].value;
				return true;
			}
		return false;
	}
	/** operator[]: the value of key, created (default value) when missing. */
	T &at(uint16 key) {
		Common::Array<Entry> &b = _buckets[bucket(key)];
		for (uint i = 0; i < b.size(); i++)
			if (b[i].key == key)
				return b[i].value;
		Entry e;
		e.key = key;
		e.value = T();
		b.insert_at(0, e);
		_count++;
		return b[0].value;
	}
	void clear() {
		for (int i = 0; i < kBuckets; i++)
			_buckets[i].clear();
		_count = 0;
	}
	uint size() const { return _count; }

	/** Iteration as GetStartPosition/GetNextAssoc: a position is (bucket << 16) | index + 1, 0 = end. */
	uint32 start() const { return next(0, 0); }
	uint32 nextPos(uint32 pos) const { return next(pos >> 16, (pos & 0xffff)); }
	uint16 keyAt(uint32 pos) const { return _buckets[pos >> 16][(pos & 0xffff) - 1].key; }
	T &valueAt(uint32 pos) { return _buckets[pos >> 16][(pos & 0xffff) - 1].value; }

private:
	struct Entry {
		uint16 key;
		T value;
	};
	static uint bucket(uint16 key) { return (key >> 4) % kBuckets; }
	uint32 next(uint b, uint i) const {
		for (; b < kBuckets; b++, i = 0)
			if (i < _buckets[b].size())
				return (b << 16) | (i + 1);
		return 0;
	}
	Common::Array<Entry> _buckets[kBuckets];
	uint _count = 0;
};

/** The records of a save file (FUN_0048d3f0 / FUN_0048d450): key, size, data. */
class SaveRecords {
public:
	void write(uint32 key, const Common::Array<byte> &data);
	void writeInts(uint32 key, const int32 *values, uint n);
	/** The first record with this key. */
	bool read(uint32 key, Common::Array<byte> &data) const;
	bool readInts(uint32 key, Common::Array<int32> &values) const;

	Common::Array<byte> _bytes;
};

/**
 * A variable of a game object. The classes and their behaviour follow the
 * original's (vtables 0x4b5fc4 base, 0x4b5450 number, 0x4b54bc string,
 * 0x4b6030 random, 0x4b60a0 cycler, 0x4b6110 shuffle, 0x4b618c map); the
 * methods are named after the vtable slots.
 */
class GameVar {
public:
	GameVar(uint16 id, uint16 owner) : _id(id), _owner(owner) {}
	virtual ~GameVar() {}

	uint16 getId() const { return _id; }
	/** +0x18: the key of the variable's save record. */
	uint32 key() const { return ((uint32)_id << 16) | _owner; }

	// Comparisons with another variable (+0x0c-+0x1c).
	virtual bool eq(GameVar &o) { return false; }
	virtual bool gt(GameVar &o) { return false; }
	virtual bool lt(GameVar &o) { return false; }
	virtual bool ge(GameVar &o) { return false; }
	virtual bool le(GameVar &o) { return false; }
	/** +0x20 */
	virtual void setRaw(int32 v) { _value = v; }
	/** +0x24 */
	virtual void setString(const Common::String &s) { _string = s; }
	/** +0x28 */
	virtual void assignString(const Common::String &s) { _string = s; }
	/** +0x2c */
	virtual void set(int32 v) { setRaw(v); }
	/** +0x30 */
	void add(int32 n) { setRaw(getInt() + n); }
	/** +0x34 */
	void sub(int32 n) { setRaw(getInt() - n); }
	/** +0x38 */
	virtual int32 get() { return _value; }
	/** +0x3c */
	virtual Common::String toString() { return _string; }
	/** +0x40: map variables select a key; others return themselves. */
	virtual GameVar *at(int32 key) { return this; }
	/** +0x44: map variables iterate; others return themselves. */
	virtual GameVar *next(int32 restart) { return this; }
	/** +0x48 */
	virtual int32 getInt() { return _value; }
	/** +0x4c */
	virtual int32 count() { return 1; }
	/** +0x50 */
	Common::String getString() const { return _string; }
	/** +0x54 / +0x58: v++, ++v (the copy +0x54 returns is not used by the game). */
	virtual void increment() {}
	/** +0x5c / +0x60 */
	virtual void decrement() {}
	/** +0x64 */
	virtual void save(SaveRecords &f) {}
	/** +0x68 */
	virtual void load(const SaveRecords &f) {}

	/** +0x14 */
	bool _save = false;

protected:
	uint16 _id, _owner;
	int32 _value = 0;
	Common::String _string;
};

/** Vtable 0x4b5450 (FUN_00476ce0) */
class IntVar : public GameVar {
public:
	IntVar(uint16 id, uint16 owner) : GameVar(id, owner) {}
	bool eq(GameVar &o) override { int32 b = o.getInt(); return getInt() == b; }
	bool gt(GameVar &o) override { int32 b = o.getInt(); return getInt() > b; }
	bool lt(GameVar &o) override { int32 b = o.getInt(); return getInt() < b; }
	bool ge(GameVar &o) override { int32 b = o.getInt(); return getInt() >= b; }
	bool le(GameVar &o) override { int32 b = o.getInt(); return getInt() <= b; }
	void setString(const Common::String &s) override;
	void assignString(const Common::String &s) override { setString(s); }
	Common::String toString() override;
	void increment() override { _value++; }
	void decrement() override { _value--; }
	void save(SaveRecords &f) override;
	void load(const SaveRecords &f) override;
};

/** Vtable 0x4b54bc (FUN_00476d70) */
class StringVar : public GameVar {
public:
	StringVar(uint16 id, uint16 owner) : GameVar(id, owner) {}
	bool eq(GameVar &o) override { return cmp(o) == 0; }
	bool gt(GameVar &o) override { return cmp(o) > 0; }
	bool lt(GameVar &o) override { return cmp(o) < 0; }
	bool ge(GameVar &o) override { return cmp(o) >= 0; }
	bool le(GameVar &o) override { return cmp(o) <= 0; }
	void setRaw(int32 v) override;
	int32 get() override;
	void save(SaveRecords &f) override;
	void load(const SaveRecords &f) override;
private:
	int cmp(GameVar &o) { Common::String b = o.getString(); return strcmp(_string.c_str(), b.c_str()); }
};

/** Vtable 0x4b6030 (FUN_00476e20): each read is a new random number in [min, max]. */
class RandomVar : public IntVar {
public:
	RandomVar(uint16 id, uint16 owner, int32 min, int32 max) : IntVar(id, owner), _min(min), _max(max) {}
	void set(int32 v) override { _max = v; }
	int32 get() override { roll(); return _value; }
	int32 getInt() override { roll(); return _value; }
	Common::String toString() override;
	void increment() override {}
	void decrement() override {}
	void save(SaveRecords &f) override;
	void load(const SaveRecords &f) override;
private:
	void roll();
	int32 _min, _max;
};

/** Vtable 0x4b60a0 (FUN_00476eb0): a read returns the value, then steps it towards max, wrapping to min. */
class CyclerVar : public IntVar {
public:
	CyclerVar(uint16 id, uint16 owner, int32 min, int32 max) : IntVar(id, owner), _min(min), _max(max) { _value = min; }
	void set(int32 v) override { _max = v; _value = _min; }
	int32 get() override;
	Common::String toString() override;
	void increment() override {}
	void decrement() override {}
	void save(SaveRecords &f) override;
	void load(const SaveRecords &f) override;
private:
	int32 _min, _max;
};

/** Vtable 0x4b6110 (FUN_00476f40): reads deal [min, max] in random order. */
class ShuffleVar : public IntVar {
public:
	ShuffleVar(uint16 id, uint16 owner, int32 min, int32 max);
	void set(int32 v) override;
	int32 get() override;
	Common::String toString() override;
	void increment() override {}
	void decrement() override {}
	void save(SaveRecords &f) override;
	void load(const SaveRecords &f) override;
private:
	void setMin(int32 v);
	void refill();
	int32 _min = 0, _max = 0;
	Common::Array<int32> _pool;
	int32 _used = 0;
};

/** Vtable 0x4b618c (FUN_00476fd0): a map from 16-bit keys to numbers, used through a current key. */
class MapVar : public IntVar {
public:
	MapVar(uint16 id, uint16 owner) : IntVar(id, owner) {}
	void set(int32 v) override;
	int32 get() override { return _value; }
	int32 getInt() override { return _value; }
	GameVar *at(int32 key) override;
	GameVar *next(int32 restart) override;
	int32 count() override { return _map.size(); }
	void increment() override {}
	void decrement() override {}
	void save(SaveRecords &f) override;
	void load(const SaveRecords &f) override;
private:
	WordMap<int32> _map;
	uint16 _key = 0;
	uint32 _pos = 0;      ///< +0x20, 0xffffffff = before the start
};

/** The variables of an object (FUN_0047c0a0). */
class VarContainer {
public:
	~VarContainer();
	/** FUN_0047c260: replaces a variable with the same id. */
	void add(GameVar *v);
	/** FUN_0047c210: a missing variable is a reset placeholder. */
	GameVar *get(uint16 id);
	bool has(uint16 id) const;
	void save(SaveRecords &f);
	void load(const SaveRecords &f);
private:
	WordMap<GameVar *> _vars;
	Common::Array<GameVar *> _all;
	GameVar _placeholder = GameVar(0, 0);
};

} // End of namespace Ivanoff

#endif
