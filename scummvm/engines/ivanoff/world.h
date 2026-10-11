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

#ifndef IVANOFF_WORLD_H
#define IVANOFF_WORLD_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/list.h"
#include "common/serializer.h"
#include "common/str.h"

namespace Ivanoff {

class Child;
class GameObject;
class IvanoffEngine;

/** Object IDs the engine knows about. */
enum {
	kBroadcast = 0,      ///< target 0: every top-level object
	kSelf = 0xffff,      ///< target 0xffff: the sending object
	kGameState = 0x11,   ///< object 17: current room, clock, navigation
	kSpeech = 0x12       ///< object 18: subtitles and voice
};

/** Messages and events (Message::msg). */
enum {
	kMsgInit = 1,
	kMsgStart = 2,
	kMsgNewGame = 3,
	kMsgGotoRoom = 4,     ///< p1 = room
	kMsgLeave = 5,        ///< broadcast, p1 = room being left
	kMsgExit = 6,         ///< to the current room: play the exit, then send 7 to kGameState
	kMsgSwitchRoom = 7,
	kMsgEnter = 8,        ///< broadcast, p1 = new room, p2 = previous room
	kMsgEntered = 9,      ///< to the new room, p1 = previous room
	kMsgSay = 0x2d,       ///< to kSpeech, p1 = (voice << 16) | text
	kMsgClick = 5000,     ///< p1 = child id, p2 = (y << 16) | x
	kMsgButtonUp = 0x1389,
	kMsgRightClick = 0x138a,
	kMsgDoubleClick = 0x138b,
	kMsgMouseMove = 0x138c,
	kMsgMouseEnter = 0x138d,
	kMsgMouseLeave = 0x138e,
	kMsgAnimDone = 0x138f, ///< p1 = child id
	kMsgSoundDone = 0x1390, ///< p1 = child id
	kMsgKeyUp = 0x1391,
	kMsgKeyDown = 0x1392,
	kMsgCanDrag = 0x1393,
	kMsgDragStart = 0x1394,
	kMsgDragStartAlt = 0x1395,
	kMsgDropTest = 0x1396,
	kMsgDropTestAlt = 0x1397,
	kMsgDragEnd = 0x1398,
	kMsgDropped = 0x1399,
	kMsgDrop = 0x139a,
	kMsgClickNoDrag = 0x139b,
	kMsgWhoIsDragged = 0x139c,
	kMsgIdle = 0x139e
};

/** Child commands (Message::cmd). */
enum {
	kCmdLoad = 1,
	kCmdUnload = 2,
	kCmdPlay = 3,
	kCmdStop = 4,
	kCmdEnable = 5,
	kCmdSetPos = 6,
	kCmdSetZ = 7,
	kCmdSetOffset = 8,
	kCmdSetClip = 9,
	kCmdUsePalette = 10,
	kCmdPause = 0xb,
	kCmdResume = 0xc,
	kCmdSetText = 0xf,        ///< text sprites: p1 = string
	kCmdSetView = 0x11,
	kCmdScrollTo = 0x12,
	kCmdScrollBy = 0x13,
	kCmdMove = 0x14,          ///< attach to the mouse cursor (drag), p1 = (y << 16) | x grab point
	kCmdDetach = 0x15,        ///< detach from the mouse cursor
	kCmdRelPos = 0x16,
	kCmdHitTest = 0x17,       ///< hotspots: hit test; sprites: position (y << 16) | x
	kCmdGetPos = 0x17,
	kCmdGetZ = 0x18,
	kCmdGetSize = 0x19,
	kCmdIsLoaded = 0x1a,
	kCmdIsPlaying = 0x1b,
	kCmdIsPaused = 0x1d,
	kCmdIsBusy = 0x20,
	kCmdIsRunning = 0x21,
	kCmdSetCursor = 0x24,
	kCmdGetFrame = 0x25,
	kCmdGotoFrame = 0x26,
	kCmdSetFlags = 0x27,
	kCmdGetFlags = 0x28,
	kCmdAddTag = 0x29,
	kCmdRemoveTag = 0x2a,
	kCmdHasTag = 0x2b,
	kCmdEnableInput = 0x2c,
	kCmdHotspotHit = 0x2d
};

/**
 * A message. Layout and semantics follow the original 0x2c-byte struct
 * (FUN_0047c3c0); see docs/ENGINE.md.
 */
struct Message {
	uint16 target = kSelf;
	int mode = 0;      ///< 1 object message, 2 child command
	int32 cmd = 0;     ///< child command; 0 for object messages
	uint16 msg = 0;    ///< message number, or child id for child commands
	int32 handled = 0;
	int32 p1 = 0, p2 = 0, p3 = 0;
	int32 result = 0;

	Message() {}
	Message(uint16 target_, int32 cmd_, uint16 msg_, int32 p1_ = 0, int32 p2_ = 0, int32 p3_ = 0, int mode_ = 0) :
		target(target_), mode(mode_), cmd(cmd_), msg(msg_), p1(p1_), p2(p2_), p3(p3_) {}
};

/**
 * A variable owned by an object (FUN_0047ca20 and subclasses).
 * Variables flagged "save" are written to save games as
 * (object id, var id, length, value) records, like the original.
 */
class Var {
public:
	Var(uint16 id, bool save) : _id(id), _save(save) {}
	virtual ~Var() {}

	uint16 getId() const { return _id; }
	bool isSaved() const { return _save; }

	/** vtable +0x38. Some kinds change their state when read. */
	virtual int32 get() { return _value; }
	/** vtable +0x2c */
	virtual void set(int32 value) { _value = value; }
	/** vtable +0x30 */
	void add(int32 n) { set(get() + n); }
	/** vtable +0x34 */
	void sub(int32 n) { set(get() - n); }
	/** The value without the side effects of get(). */
	int32 peek() const { return _value; }

	virtual void sync(Common::Serializer &s) { s.syncAsSint32LE(_value); }

protected:
	uint16 _id;
	bool _save;
	int32 _value = 0;
};

/** FUN_00476d70: a string variable. */
class StringVar : public Var {
public:
	StringVar(uint16 id, bool save) : Var(id, save) {}
	Common::String _string;
	void sync(Common::Serializer &s) override { s.syncString(_string); }
};

/**
 * FUN_00476eb0 (vtable 0x4b60a0): get() returns the value, then steps it
 * towards max, wrapping to min. set() sets max and resets to min.
 */
class CyclerVar : public Var {
public:
	CyclerVar(uint16 id, bool save, int32 min, int32 max) : Var(id, save), _min(min), _max(max) { _value = min; }
	int32 get() override;
	void set(int32 value) override { _max = value; _value = _min; }
private:
	int32 _min, _max;
};

/** FUN_00476e20 (vtable 0x4b6030): get() returns a random value in [min, max]. */
class RandomVar : public Var {
public:
	RandomVar(uint16 id, bool save, int32 min, int32 max) : Var(id, save), _min(min), _max(max) {}
	int32 get() override;
	void set(int32 value) override { _max = value; }
private:
	int32 _min, _max;
};

/** FUN_00476f40 (vtable 0x4b6110): get() returns the next value of a random permutation of [min, max]. */
class ShuffleVar : public Var {
public:
	ShuffleVar(uint16 id, bool save, int32 min, int32 max);
	int32 get() override;
	void set(int32 value) override;
private:
	void reset();
	int32 _min, _max;
	Common::Array<int32> _pool;
	uint _used = 0;
};

/**
 * A top-level object: a room, a sub-scene or a global controller.
 * Mirrors the original class with vtable slots 3-6:
 * createVars, createChildren, accepts, handle.
 */
class GameObject {
public:
	GameObject(uint16 id) : _id(id) {}
	virtual ~GameObject();

	uint16 getId() const { return _id; }

	/** Slot 3: create the object's variables. */
	virtual void createVars() {}
	/** Slot 4: create the object's children. */
	virtual void createChildren() {}
	/** Slot 5: does the object accept this message? */
	virtual bool accepts(Message &msg) { return true; }
	/** Slot 6: handle a message. This is where the game logic lives. */
	virtual void handle(Message &msg) {}

	// --- the original engine's helper API (names after the original functions) ---

	/** FUN_00476ad0: send an object message, synchronously. Returns the result. */
	int32 send(uint16 target, uint16 msg, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0);
	/** FUN_00476a60: post an object message; delivered later from the main loop. */
	void post(uint16 target, uint16 msg, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0);
	/** FUN_00476b40: send a child command. Returns the result. */
	int32 cmd(uint16 target, int32 command, uint16 childId, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0);
	/** cmd() on one of this object's own children. */
	int32 cmd(int32 command, uint16 childId, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0) {
		return cmd(kSelf, command, childId, p1, p2, p3);
	}
	/** FUN_00477140: deliver msg after ms milliseconds (timer slot id). */
	void startTimer(uint16 timerId, int32 ms, const Message &msg);
	/** FUN_00476bc0 + FUN_00477140 combined. */
	void startTimer(uint16 timerId, int32 ms, uint16 target, uint16 msg, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0);
	/** FUN_00477170 */
	void stopTimer(uint16 timerId);
	/** FUN_00477120: set the global cursor (0 = default). */
	void setCursor(uint32 cursor);

	/** FUN_00477050 */
	Var &var(uint16 id);
	/** FUN_00477060: a variable of another object. */
	Var &var(uint16 id, uint16 objectId);

	/** FUN_00476ce0 */
	Var &addVar(uint16 id, int32 value, bool save);
	/** FUN_00476d70 */
	StringVar &addStringVar(uint16 id, const Common::String &value, bool save);
	/** FUN_00476eb0 */
	Var &addCycler(uint16 id, int32 min, int32 max, bool save);
	/** FUN_00476e20 */
	Var &addRandom(uint16 id, int32 min, int32 max, bool save);
	/** FUN_00476f40 */
	Var &addShuffle(uint16 id, int32 min, int32 max, bool save);

	/** FUN_004770a0 */
	Child *addChild(Child *child);
	Child *getChild(uint16 id);
	const Common::Array<Child *> &getChildren() const { return _children; }

	/** Dispatch a child command (FUN_0047c6b0). */
	void dispatchChildCommand(Message &msg);

	void syncVars(Common::Serializer &s);

	/** +0x18: which key events the object receives (FUN_0047bde0). */
	virtual uint32 eventMask() { return _flags18; }
	uint32 _flags18 = 0;

protected:
	uint16 _id;
	Common::Array<Var *> _vars;
	Common::Array<Child *> _children;
};

/** A timer entry (FUN_0047ead0). */
struct Timer {
	GameObject *owner;
	uint16 id;
	uint32 due;
	Message msg;
};

/**
 * The world: the registry of top-level objects, message delivery,
 * the posted-message queue and the timers.
 */
class World {
public:
	World(IvanoffEngine *vm) : _vm(vm) {}
	~World();

	void add(GameObject *obj);
	GameObject *get(uint16 id) const;
	const Common::Array<GameObject *> &getObjects() const { return _order; }

	/** Delivers an object message (FUN_00476790 / FUN_0047bc20). */
	int32 deliver(GameObject *from, Message &msg);
	/** Delivers a child command (FUN_004767f0 / FUN_0047bd10). */
	int32 deliverCommand(GameObject *from, Message &msg);
	/** Delivers a key/mouse broadcast to objects that subscribe to it (FUN_0047bde0). */
	void deliverFiltered(Message &msg);

	void post(const Message &msg);
	void startTimer(GameObject *owner, uint16 id, uint32 ms, const Message &msg);
	void stopTimer(GameObject *owner, uint16 id);

	/** Runs due timers and posted messages. */
	void update();

	void sync(Common::Serializer &s);

	/** World +0x14 (FUN_00477240): the only object receiving key events, 0 = all. */
	uint16 _keyFocus = 0;

private:
	IvanoffEngine *_vm;
	Common::HashMap<uint16, GameObject *> _objects;
	Common::Array<GameObject *> _order;
	Common::List<Message> _posted;
	Common::List<Timer> _timers;
};

} // End of namespace Ivanoff

#endif
