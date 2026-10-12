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

// Batch g: the rooms 49-62 and the items 63 and 64.

#include "ivanoff/machine.h"
#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

namespace {

enum {
	kCStringFree = 0x49fec5,   ///< FUN_0049fec5: CString destructor
	kNewMessage = 0x476bc0,    ///< FUN_00476bc0: a message on the heap
	kRedrawNow = 0x476a40      ///< FUN_00476a40 (the objects' vtable slot 7)
};

enum {
	kCmdFrameTrigger = 9,      ///< p1 = frame, p2 = a heap message or 0
	kCmdSetMouseCursor = 0x24,
	kCmdFlags = 0x27
};

enum {
	kMsgSpeechDone = 0x2e,     ///< p2 = the text [unverified]
	kMsgPlayAnim = 0x2f,
	kMsgAnimNotify = 0x30,     ///< p2 = the animation that ended
	kMsgCanLook = 0x3b,        ///< result 1 if child p1 can be looked at [unverified]
	kMsgLook = 0x3c            ///< look at child p1 [unverified]
};

/** The two characters. */
enum {
	kMagnus = 0xf,
	kMyggen = 0x10
};

/** The p2 values of the rooms' kCmdPlay (play modes) [unverified]. */
const int32 kPlayA = (int32)0xffff3fff;
const int32 kPlayB = (int32)0xc0003fff;
const int32 kPlayOnce = (int32)0x8000c001;

/**
 * The original handlers run with a Message (0x2c bytes) on the heap;
 * holding a block of that size keeps the heap addresses of later
 * allocations the same.
 */
class HeapBlock {
public:
	HeapBlock(Machine &m) : _m(m), _addr(0) {}
private:
	Machine &_m;
	uint32 _addr;
};

/** Accepts of the items (FUN_004209d0 -> FUN_00423a60): no clicks while dragging. */
class ItemAccepts : public ObjectScript {
public:
	ItemAccepts(EmuObject *obj) : ObjectScript(obj) {}
	bool accepts(Message &msg) override {
		if (var(kVarDragMode, kGameState).get() == 1 && msg.msg == kMsgClick)
			return false;
		return true;
	}
};

/** A script of this batch: Base's accepts and handleMessage() with the heap block held. */
template<class Base>
class HeldScript : public Base {
public:
	HeldScript(EmuObject *obj) : Base(obj) {}

	bool accepts(Message &msg) override {
		HeapBlock block(this->machine());
		return Base::accepts(msg);
	}

	void handle(Message &msg) override {
		HeapBlock block(this->machine());
		handleMessage(msg);
	}

protected:
	virtual void handleMessage(Message &msg) = 0;

	int32 currentRoom() { return this->var(1, kGameState).get(); }

	/** Var slots 0x54 / 0x5c (v++ / v--), the temporary copy on the stack. */
	void postStep(VarRef v, uint32 slot) {
		if (slot == 0x54)
			v.postIncrement();
		else
			v.postDecrement();
	}
	void postIncrement(VarRef v) { postStep(v, 0x54); }
	void postDecrement(VarRef v) { postStep(v, 0x5c); }

	/** Writes the object's event mask (+0x18). */
	void setEventMask(uint32 mask) {
		Script::setEventMask(mask);
	}

	bool isRunning(uint16 target, uint16 child) {
		return this->cmd(target, kCmdIsRunning, child) != 0;
	}

	void gotoRoom(int32 room) { this->send(kGameState, kMsgGotoRoom, room); }

	/** Takes the dropped item p1 out of the hand. */
	void takeDroppedItem(Message &msg) {
		VarRef sprite = this->var(0x30, (uint16)msg.p1);
		this->cmd((uint16)msg.p1, kCmdStop, (uint16)sprite.get());
		this->setCursor(0);
	}
};

typedef HeldScript<ItemAccepts> RoomScript;
typedef HeldScript<ItemScript> HeldItem;

// --- Object 49: room 0x31 (vtable 0x4b4c04, FUN_0045b250) ---

class Object49 : public RoomScript {
public:
	Object49(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x31)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	/** A door with a lamp: a click lights the lamp (state 1 -> 2), then opens the door. */
	struct Door {
		uint16 var, lamp, door;
		int32 x, y, room;
	};
	static const Door kDoors[6];

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLook:
			switch (msg.p1) {
			case 0x1da: send(kMyggen, kMsgSay, 0x6cd06cc); break;
			case 0x1dd: send(kMyggen, kMsgSay, 0x6cb06ca); break;
			case 0x1e0: send(kMyggen, kMsgSay, 0x6cf06ce); break;
			case 0x1e5: send(kMyggen, kMsgSay, 0x6d106d0); break;
			case 0x1e9:
				if (var(0x54).get() == 0)
					send(kSpeech, kMsgSay, 0x6d906d8);
				break;
			case 0x1ef: send(kMyggen, kMsgSay, 0x6d306d2); break;
			case 0x1f0: send(kMyggen, kMsgSay, 0x6c906c8); break;
			default: break;
			}
			break;
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x1e1);
			cmd(kSelf, kCmdPlay, 0x1e1);
			cmd(kSelf, kCmdSetZ, 0x1e1, 100);
			// The original order: variables 0x53, 0x54, 0x55, 0x56, 0x57, 0x58.
			static const int order[6] = { 0, 4, 5, 3, 2, 1 };
			for (int i = 0; i < 6; i++) {
				const Door &d = kDoors[order[i]];
				int32 v = var(d.var).get();
				cmd(kSelf, kCmdPlay, d.lamp, 0xffff, v == 2 ? kPlayA : kPlayB, 0);
				cmd(kSelf, kCmdSetPos, d.lamp, d.x, d.y, 0x69);
				v = var(d.var).get();
				cmd(kSelf, v == 2 ? kCmdPlay : kCmdLoad, d.door);
				cmd(kSelf, kCmdSetZ, d.door, 0x6e);
			}
			static const uint16 kStatic[7] = { 0x1e4, 0x1f0, 0x1dd, 0x1da, 0x1e0, 0x1e5, 0x1ef };
			for (int i = 0; i < 7; i++) {
				cmd(kSelf, kCmdPlay, kStatic[i]);
				cmd(kSelf, kCmdSetZ, kStatic[i], 0x6e);
			}
			break;
		}
		case kMsgEntered:
			if (var(0x43).get() == 1) {
				send(kGameState, 10);
			} else {
				var(0x43).set(1);
				send(kMagnus, kMsgSay, 0x6c506c4, kMyggen);
			}
			break;
		case kMsgCanLook:
			switch (msg.p1) {
			case 0x1da:
			case 0x1dd:
			case 0x1e0:
			case 0x1e5:
			case 0x1e9:
			case 0x1ef:
			case 0x1f0:
				msg.result = 1;
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x1e4) {
				gotoRoom(0x24);
				break;
			}
			for (int i = 0; i < 6; i++) {
				const Door &d = kDoors[i];
				if (msg.p1 == d.door) {
					gotoRoom(d.room);
				} else if (msg.p1 == d.lamp) {
					int32 v = var(d.var).get();
					if (d.lamp == 0x1e9 && v == 0) {
						cmd(kSelf, kCmdPlay, 0x198);
					} else if (v == 1) {
						var(d.var).set(2);
						cmd(kSelf, kCmdResume, d.lamp);
						cmd(kSelf, kCmdPlay, 0x199);
					} else if (v == 2) {
						gotoRoom(d.room);
					}
				}
			}
			break;
		case kMsgAnimDone:
			for (int i = 0; i < 6; i++) {
				if (msg.p1 == kDoors[i].lamp)
					cmd(kSelf, kCmdPlay, kDoors[i].door);
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x1e9)
				msg.result = (uint32)msg.p1 >> 16 == 0x310;
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x310 && msg.p2 == 0x1e9 && var(0x54).get() == 0) {
				takeDroppedItem(msg);
				var(0x54).set(1);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x31, (uint16)msg.p1).set(1);
				cmd(kSelf, kCmdPlay, 0x19a);
				send(kSpeech, kMsgSay, 0x6db06da);
			}
			break;
		default:
			break;
		}
	}
};

const Object49::Door Object49::kDoors[6] = {
	{ 0x53, 0x1dc, 0x1db, 0xb, 0xbf, 0x16 },
	{ 0x58, 0x1df, 0x1de, 0x24a, 0xbe, 0x1c },
	{ 0x57, 0x1e3, 0x1e2, 0x1d4, 0xcf, 0x23 },
	{ 0x56, 0x1e7, 0x1e6, 0x16b, 0xd4, 0x2c },
	{ 0x54, 0x1e9, 0x1e8, 0x86, 0xd0, 0x2d },
	{ 0x55, 0x1f2, 0x1f1, 0xef, 0xd3, 0x34 }
};

// --- Object 50: room 0x32 (vtable 0x4b4be4, FUN_0045c220) ---

class Object50 : public RoomScript {
public:
	Object50(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x32)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgSpeechDone:
			if (msg.p2 == 0x925) {
				while (send(kGameState, 0x14, 1) != 0)
					;
				gotoRoom(0x3e);
			}
			break;
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdUsePalette, 0x4ec);
			cmd(kSelf, kCmdPlay, 0x4ec);
			cmd(kSelf, kCmdSetZ, 0x4ec, 100);
			cmd(kSelf, kCmdPlay, 0x4ed);
			cmd(kSelf, kCmdSetZ, 0x4ed, 0x69);
			cmd(kSelf, kCmdPlay, 0x4e5);
			cmd(kSelf, kCmdSetZ, 0x4e5, 0x69);
			cmd(kSelf, kCmdPlay, 0x4e6);
			cmd(kSelf, kCmdSetZ, 0x4e6, 0x69);
			cmd(kMagnus, kCmdLoad, 0x129);
			cmd(kMagnus, kCmdSetPos, 0x129, 0x203, 0x119, 0x3a93);
			cmd(kMyggen, kCmdLoad, 0x12d);
			cmd(kMyggen, kCmdSetPos, 0x12d, 0x1eb, 0xe9, 0x3a93);
			var(0x88).set(0);
			break;
		case kMsgEntered:
			cmd(kMagnus, kCmdPlay, 0x4e7);
			cmd(kMyggen, kCmdPlay, 0x4ea);
			if (var(0x43).get() == 1) {
				send(kGameState, 10);
			} else {
				var(0x43).set(1);
				send(kMagnus, kMsgSay, 0x920091f, kMyggen);
			}
			break;
		case kMsgCanLook:
			if (msg.p1 == 0x4ec)
				msg.result = 1;
			break;
		case kMsgLook:
			if (msg.p1 == 0x4ec) {
				int32 v = var(0x44).get();
				if (v == 0)
					send(kSpeech, kMsgSay, 0x9280927);
				else if (v == 1)
					send(kSpeech, kMsgSay, 0x92a0929);
				else if (v == 2)
					send(kSpeech, kMsgSay, 0x92c092b);
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x4e5) {
				if (var(0x88).get() == 0) {
					send(kMyggen, kMsgSay, 0x9240923);
					postIncrement(var(0x88));
				} else {
					cmd(kSelf, kCmdUnload, 0x4ec);
					cmd(kSelf, kCmdUnload, 0x4ed);
					cmd(kSelf, kCmdUnload, 0x4e6);
					cmd(kSelf, kCmdEnable, 0x4e5);
					cmd(kMagnus, kCmdUnload, 0x4e7);
					var(0x10, kMagnus).set(0x129);
					var(0x11, kMagnus).set(0x129);
					cmd(kMagnus, kCmdPlay, 0x129);
					cmd(kMyggen, kCmdUnload, 0x4ea);
					var(0x10, kMyggen).set(0x12d);
					var(0x11, kMyggen).set(0x12d);
					cmd(kMyggen, kCmdPlay, 0x12d);
					send(kSpeech, kMsgSay, 0x92e092d, kMyggen);
				}
			} else if (msg.p1 == 0x4e6) {
				gotoRoom(0x25);
			} else if (msg.p1 == 0x4ed) {
				gotoRoom(0x33);
			}
			break;
		default:
			break;
		}
	}
};

// --- Object 51: room 0x33 (vtable 0x4b4bc4, FUN_0045ca80) ---

class Object51 : public RoomScript {
public:
	Object51(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x33)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLook:
			switch (msg.p1) {
			case 0x4ee: send(kSpeech, kMsgSay, 0x93e093d); break;
			case 0x4ef: send(kSpeech, kMsgSay, 0x93a0939); break;
			case 0x4fd: send(kSpeech, kMsgSay, 0x93c093b); break;
			case 0x4fe: send(kSpeech, kMsgSay, 0x9420941); break;
			case 0x4ff: send(kSpeech, kMsgSay, 0x940093f); break;
			default: break;
			}
			break;
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			stopTimer(1);
			break;
		case kMsgExit:
			if (msg.p1 == 0x32) {
				if (isRunning(kSelf, 0x4fb) || isRunning(kSelf, 0x4fa)) {
					cmd(kMagnus, kCmdStop, 0x4f2);
					cmd(kMagnus, kCmdPlay, 0x4f4, 0x33, kPlayOnce);
					cmd(kMyggen, kCmdStop, 0x4f7);
					cmd(kMyggen, kCmdPlay, 0x4f9, 0x33, kPlayOnce);
					cmd(kMagnus, kCmdPlay, 0x5e9);
					send(0xd, 0x3d, kMagnus, 0x5ea, 0x5e9);
					break;
				}
			} else if (msg.p1 == 0x3b) {
				send(kMagnus, kMsgPlayAnim, 0x4f5, 0x4f20033, kPlayOnce);
				break;
			}
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			enter();
			break;
		case kMsgEntered:
			cmd(kMagnus, kCmdPlay, 0x4f2);
			cmd(kMyggen, kCmdPlay, 0x4f7);
			if (var(0x43).get() == 1) {
				if (var(0x1a, 0x15).get() == 0)
					send(kGameState, 10);
				else
					send(kMagnus, kMsgSay, 0x9340933, kMyggen);
			} else {
				var(0x43).set(1);
				send(kMagnus, kMsgSay, 0x930092f, kMyggen);
			}
			break;
		case kMsgAnimNotify:
			if (msg.p2 == 0x4f5)
				send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgCanLook:
			switch (msg.p1) {
			case 0x4ee:
			case 0x4ef:
			case 0x4fd:
			case 0x4fe:
			case 0x4ff:
				msg.result = 1;
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			switch (msg.p1) {
			case 0x4f0:
				gotoRoom(0x28);
				break;
			case 0x4f1:
			case 0x4ff:
				if (var(0x8a, 0x1d).get() == 0) {
					if (!isRunning(kSelf, 0x4fb) && !isRunning(kSelf, 0x4fa))
						cmd(kSelf, kCmdPlay, 0x4fb, 0xffff);
				} else {
					gotoRoom(0x3b);
				}
				break;
			case 0x4fc:
				gotoRoom(0x32);
				break;
			default:
				break;
			}
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x4f4 || msg.p1 == 0x4f9) {
				// Both characters have left: leave the room.
				if (!isRunning(kSelf, msg.p1 == 0x4f4 ? 0x4f9 : 0x4f4))
					send(kGameState, kMsgSwitchRoom);
			} else if (msg.p1 == 0x4fb) {
				cmd(kSelf, kCmdStop, 0x4fb);
				cmd(kSelf, kCmdPlay, 0x4fa);
				startTimer(1, 1000, kGameState, kMsgGotoRoom, 0x32);
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x4ef)
				msg.result = (uint32)msg.p1 >> 16 == 0x302;
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x302 && msg.p2 == 0x4ef) {
				takeDroppedItem(msg);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x31, (uint16)msg.p1).set(1);
				var(0x6d).set(1);
				cmd(kSelf, kCmdPlay, 0x4f1);
				send(kMagnus, kMsgSay, 0x9380937);
			}
			break;
		default:
			break;
		}
	}

	void enter() {
		cmd(kSelf, kCmdUsePalette, 0x4fd);
		cmd(kSelf, kCmdPlay, 0x4fd);
		cmd(kSelf, kCmdSetZ, 0x4fd, 100);
		cmd(kSelf, var(0x1a, 0x15).get() != 0 ? kCmdPlay : kCmdLoad, 0x4f0);
		cmd(kSelf, kCmdSetZ, 0x4f0, 0x65);
		if (var(0x8a, 0x1d).get() == 0) {
			cmd(kSelf, kCmdPlay, 0x4ff);
			cmd(kSelf, kCmdSetZ, 0x4ff, 0x65);
		}
		cmd(kSelf, var(0x1a, 0x15).get() != 0 ? kCmdLoad : kCmdPlay, 0x4fc);
		cmd(kSelf, kCmdSetZ, 0x4fc, 0x65);
		cmd(kSelf, kCmdPlay, 0x4fe);
		cmd(kSelf, kCmdSetZ, 0x4fe, 0x65);
		cmd(kSelf, kCmdPlay, 0x4ef);
		cmd(kSelf, kCmdSetZ, 0x4ef, 0x65);
		cmd(kSelf, kCmdPlay, 0x4ee);
		cmd(kSelf, kCmdSetZ, 0x4ee, 0x65);
		cmd(kSelf, kCmdLoad, 0x4ee);
		cmd(kSelf, kCmdSetZ, 0x4ee, 0x65);
		if (var(0x1a, 0x15).get() == 0) {
			if (var(0x6d).get() == 0)
				cmd(kSelf, kCmdLoad, 0x4f1);
			else
				cmd(kSelf, kCmdPlay, 0x4f1, 0xffff, kPlayA);
			cmd(kSelf, kCmdSetPos, 0x4f1, 0x182, 0x3b, 0x66);
			cmd(kSelf, kCmdLoad, 0x4fb);
			cmd(kSelf, kCmdSetPos, 0x4fb, 0xd7, 0x3b, 0x67);
			cmd(kSelf, kCmdLoad, 0x4fa);
			cmd(kSelf, kCmdSetPos, 0x4fa, 0x128, 0x8b, 0x67);
		}
	}
};

// --- Object 52: room 0x34 (vtable 0x4b4ba4, FUN_0045f730) ---

class Object52 : public RoomScript {
public:
	Object52(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x34)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	enum {
		kMsgGame = 0x57,     ///< next step of the game (variable 0x96 counts)
		kMsgAnswer = 0x58    ///< p1 = the chosen answer [unverified]
	};

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgSpeechDone:
			if (msg.p2 == 0xa26)
				gotoRoom(0x31);
			break;
		case kMsgCanLook:
			msg.result = msg.p1 == 0x53d;
			break;
		case kMsgLook:
			if (msg.p1 == 0x53d)
				send(kSpeech, kMsgSay, 0xa290a28);
			break;
		case kMsgGame:
			gameStep();
			break;
		case kMsgAnswer:
			if (msg.p1 == var(0x98).get()) {
				cmd(kMagnus, kCmdEnable, 0x531, 0);
				cmd(kMagnus, kCmdEnable, 0x532, 0);
				setEventMask(0);
				send(kMagnus, kMsgSay, 0xa270a26, 0x34);
			} else {
				if (var(0x97).get() > 1)
					break;
				postIncrement(var(0x97));
				if (var(0x97).get() != 2)
					break;
				send(kMagnus, kMsgSay, 0xa230a22, kMyggen);
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x52c)
				gotoRoom(0x31);
			else if (msg.p1 == 0x52d)
				gotoRoom(0x37);
			break;
		case kMsgAnimDone:
			switch (msg.p1) {
			case 0x52f:
				send(kGameState, 10);
				break;
			case 0x53a:
			case 0x53c:
			case 0xb4f:
			case 0xb51:
				setEventMask(0);
				cmd(kMagnus, kCmdEnable, 0x531, 0);
				cmd(kMagnus, kCmdEnable, 0x532, 0);
				cmd(kMagnus, kCmdPlay, 0x530);
				cmd(kSelf, kCmdPlay, 0x52d);
				var(0x24, 0xe).set(1);
				break;
			default:
				break;
			}
			break;
		case kMsgKeyDown:
			if (msg.p1 == 0x25) {
				// Left arrow.
				cmd(kSelf, kCmdStop, 0x52b);
				int32 frames;
				if (var(0x98).get() == 2) {
					var(0x98).set(1);
					frames = (int32)0x80020000;
				} else if (var(0x98).get() == 1) {
					var(0x98).set(0);
					frames = (int32)0x80040002;
				} else {
					break;
				}
				cmd(kMagnus, kCmdEnable, 0x531, 1);
				cmd(kMagnus, kCmdEnable, 0x532, 0);
				cmd(kMagnus, kCmdResume, 0x531, frames);
			} else if (msg.p1 == 0x27) {
				// Right arrow.
				cmd(kSelf, kCmdStop, 0x52b);
				int32 frames;
				if (var(0x98).get() == 0) {
					var(0x98).set(1);
					frames = (int32)0x80020000;
				} else if (var(0x98).get() == 1) {
					var(0x98).set(2);
					frames = (int32)0x80040002;
				} else {
					break;
				}
				cmd(kMagnus, kCmdEnable, 0x531, 0);
				cmd(kMagnus, kCmdEnable, 0x532, 1);
				cmd(kMagnus, kCmdResume, 0x532, frames);
			}
			break;
		case kMsgLeave:
			setEventMask(0);
			var(0x24, 0xe).set(1);
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdUsePalette, 0x53d);
			cmd(kSelf, kCmdPlay, 0x53d);
			cmd(kSelf, kCmdSetZ, 0x53d, 100);
			cmd(kSelf, kCmdPlay, 0x52c);
			cmd(kSelf, kCmdSetZ, 0x52c, 0x69);
			if (var(0x8b, 0x1d).get() == 0 && msg.p2 != 0x37) {
				var(0x96).set(0);
				cmd(kSelf, kCmdLoad, 0x52d);
				cmd(kSelf, kCmdLoad, 0x53a);
				cmd(kSelf, kCmdSetPos, 0x53a, 0, 0x3a, 0x78);
				cmd(kSelf, kCmdLoad, 0x53b);
				cmd(kSelf, kCmdSetPos, 0x53b, 0xae, 0x17, 0x78);
				cmd(kSelf, kCmdLoad, 0x53c);
				cmd(kSelf, kCmdSetPos, 0x53c, 0x152, 0x39, 0x78);
				cmd(kSelf, kCmdLoad, 0xb4f);
				cmd(kSelf, kCmdSetPos, 0xb4f, 0, 0x3a, 0x78);
				cmd(kSelf, kCmdLoad, 0xb50);
				cmd(kSelf, kCmdSetPos, 0xb50, 0xae, 0x17, 0x78);
				cmd(kSelf, kCmdLoad, 0xb51);
				cmd(kSelf, kCmdSetPos, 0xb51, 0x152, 0x39, 0x78);
				cmd(kSelf, kCmdLoad, 0x52f);
				cmd(kSelf, kCmdSetPos, 0x52f, 0x5d, 0, 0x73);
				cmd(kSelf, kCmdLoad, 0x52b);
				cmd(kSelf, kCmdSetPos, 0x52b, 0x226, 0x14f, 300);
				cmd(kSelf, kCmdLoad, 0x538);
				cmd(kSelf, kCmdLoad, 0x539);
			} else {
				cmd(kSelf, kCmdPlay, 0x52d);
			}
			cmd(kSelf, kCmdSetZ, 0x52d, 0x69);
			break;
		case kMsgEntered:
			if (var(0x8b, 0x1d).get() != 0 || msg.p1 == 0x37) {
				if (var(0x43).get() != 1)
					var(0x43).set(1);
				send(kGameState, 10);
				break;
			}
			var(0x24, 0xe).set(0);
			if (var(0x43).get() == 1) {
				cmd(kSelf, kCmdPlay, 0x52f, 0x34);
				break;
			}
			var(0x43).set(1);
			cmd(kSelf, kCmdPlay, 0x52f);
			send(kMyggen, kMsgSay, 0xa1f0a1e, kMagnus);
			break;
		case 0xb:
			if (var(0x8b, 0x1d).get() != 0)
				break;
			setEventMask(0x10000);
			cmd(kSelf, kCmdPlay, 0x52b);
			var(0x98).set(0);
			var(0x10, kMagnus).set(0);
			var(0x11, kMagnus).set(0);
			var(0x10, kMyggen).set(0);
			var(0x11, kMyggen).set(0);
			cmd(kMyggen, kCmdStop, 0x535);
			cmd(kMyggen, kCmdPlay, 0x536, 0, kPlayOnce);
			cmd(kMagnus, kCmdStop, 0x530);
			cmd(kMagnus, kCmdPlay, 0x532, 0, kPlayB);
			cmd(kMagnus, kCmdPlay, 0x531, 0, kPlayB);
			cmd(kMagnus, kCmdEnable, 0x532, 1);
			cmd(kMagnus, kCmdEnable, 0x531, 0);
			var(0x97).set(0);
			send(kSelf, kMsgGame);
			break;
		default:
			break;
		}
	}

	/** Plays child (or its copy while it runs) with kMsgGame sent at frame 0x2c. */
	void playWithTrigger(uint16 child, uint16 copy, int32 p1) {
		uint16 c = isRunning(kSelf, child) ? copy : child;
		cmd(kSelf, kCmdFrameTrigger, c, 0x2c, 0);
		cmdFrameTrigger(kSelf, c, 0x2c, makeMsg(0x34, kMsgGame));
		cmd(kSelf, kCmdPlay, c, p1, kPlayOnce);
	}

	void gameStep() {
		int32 notify = 0;
		if (var(0x96).get() == 10)
			return;
		if (var(0x96).get() == 9)
			notify = 0x34;
		postIncrement(var(0x96));
		int32 step = var(0x95).get();
		if (step == 0) {
			playWithTrigger(0x53a, 0xb4f, notify);
			cmd(kSelf, kCmdPlay, isRunning(kSelf, 0x53b) ? 0xb50 : 0x53b, 0, kPlayOnce);
		} else if (step == 1) {
			playWithTrigger(0x53a, 0xb4f, notify);
			uint16 c = isRunning(kSelf, 0x53c) ? 0xb51 : 0x53c;
			cmd(kSelf, kCmdFrameTrigger, c, 0x2c, 0);
			cmd(kSelf, kCmdPlay, c, 0, kPlayOnce);
		} else if (step == 2) {
			cmd(kSelf, kCmdPlay, isRunning(kSelf, 0x53b) ? 0xb50 : 0x53b, 0, kPlayOnce);
			playWithTrigger(0x53c, 0xb51, notify);
		}
	}
};

// --- Object 53: room 0x35 (vtable 0x4b4b84, FUN_004607d0) ---

class Object53 : public RoomScript {
public:
	Object53(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x35)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLook:
			switch (msg.p1) {
			case 0x503:
				send(kSpeech, kMsgSay, 0xa010a00);
				break;
			case 0x505:
				if (var(0x6c).get() == 0) {
					int32 v = var(0x44).get();
					if (v == 0)
						send(kSpeech, kMsgSay, 0x9f709f6);
					else if (v == 1)
						send(kSpeech, kMsgSay, 0x9fb09fa);
					else if (v == 2)
						send(kSpeech, kMsgSay, 0x9fd09fc);
				}
				break;
			case 0x50f: {
				int32 v = var(0x45).get();
				if (v == 0)
					send(kSpeech, kMsgSay, 0xa030a02);
				else if (v == 1)
					send(kSpeech, kMsgSay, 0xa050a04);
				break;
			}
			case 0x510:
				send(kSpeech, kMsgSay, 0x9ff09fe);
				break;
			case 0x511:
				if (var(0x6b).get() == 0)
					send(kMagnus, kMsgSay, 0x9f909f8);
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x503)
				gotoRoom(0x1d);
			else if (msg.p1 == 0x504)
				gotoRoom(0x24);
			else if (msg.p1 == 0x510)
				gotoRoom(0x36);
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x505 && (uint32)msg.p1 >> 16 == 0x2c7)
				msg.result = var(0x6c).get() == 0;
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x2c7 && msg.p2 == 0x505) {
				takeDroppedItem(msg);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x6c).set(1);
				cmd(0x3f, kCmdPlay, 0x2c6, 0xffff);
				cmd(kSelf, kCmdStop, 0x505);
				cmd(kSelf, kCmdPlay, 0x502);
			}
			break;
		case kMsgCanLook:
			switch (msg.p1) {
			case 0x503:
			case 0x50f:
			case 0x510:
				msg.result = 1;
				break;
			case 0x505:
				msg.result = var(0x6c).get() == 0;
				break;
			case 0x511:
				msg.result = var(0x6b).get() == 0;
				break;
			default:
				break;
			}
			break;
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			if (msg.p1 == 0x1d)
				send(kMagnus, kMsgPlayAnim, 0x509, 0x5060035, kPlayOnce);
			else
				send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdUsePalette, 0x50f);
			cmd(kSelf, kCmdPlay, 0x50f);
			cmd(kSelf, kCmdSetZ, 0x50f, 100);
			cmd(kSelf, kCmdLoad, 0x510);
			cmd(kSelf, kCmdSetZ, 0x510, 0x69);
			cmd(kSelf, kCmdLoad, 0x503);
			cmd(kSelf, kCmdSetZ, 0x503, 0x69);
			cmd(kSelf, kCmdPlay, 0x504);
			cmd(kSelf, kCmdSetZ, 0x504, 0x69);
			if (var(0x6c).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x505);
				cmd(kSelf, kCmdSetZ, 0x505, 0x69);
			}
			cmd(kSelf, kCmdLoad, 0x512);
			cmd(kSelf, kCmdLoad, 0x514);
			cmd(kSelf, kCmdLoad, 0x511);
			cmd(kSelf, kCmdSetPos, 0x511, 300, 0xfe, 0x91);
			if (var(0x6a).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x511, 0xffff, kPlayB, 0xffff);
				cmd(kSelf, kCmdPlay, 0x510);
			} else {
				cmd(kSelf, kCmdPlay, 0x503);
			}
			cmd(kSelf, kCmdLoad, 0x513);
			cmd(kSelf, kCmdSetPos, 0x513, 300, 0xff, 0x91);
			break;
		case kMsgEntered:
			if (msg.p1 == 0x36) {
				send(kMagnus, kMsgPlayAnim, 0x507, 0x5060035, kPlayOnce);
				break;
			}
			cmd(kMagnus, kCmdPlay, 0x506);
			cmd(kMyggen, kCmdPlay, 0x50b);
			if (var(0x43).get() != 1) {
				var(0x43).set(1);
				send(kMagnus, kMsgSay, 0x9ef09ee, kMyggen);
			} else {
				send(kGameState, 10);
			}
			break;
		case kMsgAnimNotify:
			switch (msg.p2) {
			case 0x507:
				if (!isRunning(kMyggen, 0x50c))
					send(kGameState, 10);
				break;
			case 0x509:
				cmd(kMagnus, kCmdStop, 0x506);
				if (!isRunning(kMyggen, 0x50e))
					send(kGameState, kMsgSwitchRoom);
				break;
			case 0x50c:
				if (!isRunning(kMagnus, 0x507))
					send(kGameState, 10);
				break;
			case 0x50e:
				cmd(kMyggen, kCmdStop, 0x50b);
				if (!isRunning(kMagnus, 0x509))
					send(kGameState, kMsgSwitchRoom);
				break;
			case 0x511:
				cmd(kSelf, kCmdStop, 0x511);
				cmd(kSelf, kCmdPlay, 0x503);
				var(0x6a).set(1);
				var(0x6b).set(1);
				cmd(kSelf, kCmdStop, 0x510);
				break;
			case 0x513:
				cmd(kSelf, kCmdStop, 0x513);
				cmd(kSelf, kCmdPlay, 0x511, 0xffff, kPlayB, 0xffff);
				var(0x6a).set(0);
				cmd(kSelf, kCmdPlay, 0x510);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
};

// --- Object 54: room 0x36 (vtable 0x4b4b64, FUN_004614a0) ---

class Object54 : public RoomScript {
public:
	Object54(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x36)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgClick:
			if (msg.p1 == 0x524) {
				gotoRoom(0x35);
			} else if (msg.p1 == 0x527) {
				int32 v = var(0xb3).get();
				if (v == 0) {
					send(kMagnus, kMsgPlayAnim, 0x51b, 0x5190036, kPlayOnce);
					var(0xb3).set(1);
				} else if (v == 1) {
					send(kMagnus, kMsgSay, 0xa0b0a0a, kMyggen);
				} else if (v == 2) {
					gotoRoom(0x39);
				}
			}
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x517) {
				var(0xb3).set(2);
				send(kMagnus, kMsgSay, 0xa0f0a0e, kMyggen);
			} else if (msg.p1 == 0x529) {
				cmd(kSelf, kCmdStop, 0x515);
				cmd(kSelf, kCmdSetZ, 0x529, 0x6b);
				if (var(0xb3).get() == 0) {
					var(0xb3).set(2);
					cmd(kSelf, kCmdPlay, 0x517, 0, kPlayA);
					send(kMagnus, kMsgSay, 0xa0f0a0e, kMyggen);
				} else {
					cmd(kSelf, kCmdPlay, 0x517, 0xffff);
				}
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x52a)
				msg.result = (uint32)msg.p1 >> 16 == 0x38c;
			break;
		case kMsgDrop:
			if (msg.p2 == 0x52a && (uint32)msg.p1 >> 16 == 0x38c) {
				takeDroppedItem(msg);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x31, (uint16)msg.p1).set(1);
				cmd(kSelf, kCmdPlay, 0x529, 0xffff);
			}
			break;
		case kMsgLook:
			if (msg.p1 == 0x515) {
				int32 v = var(0x45).get();
				if (v == 0)
					send(kSpeech, kMsgSay, 0xa190a18);
				else if (v == 1)
					send(kSpeech, kMsgSay, 0xa1b0a1a);
				else if (v == 2)
					send(kSpeech, kMsgSay, 0xa1d0a1c);
			} else if (msg.p1 == 0x52a) {
				int32 v = var(0x44).get();
				if (v == 0)
					send(kSpeech, kMsgSay, 0xa150a14);
				else if (v == 1)
					send(kSpeech, kMsgSay, 0xa170a16);
			}
			break;
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			if (msg.p1 == 0x39)
				send(kMagnus, kMsgPlayAnim, 0x51d, 0x5190036, kPlayOnce);
			else
				send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x525);
			cmd(kSelf, kCmdPlay, 0x525);
			cmd(kSelf, kCmdPlay, 0x527);
			cmd(kSelf, kCmdSetZ, 0x527, 0x69);
			cmd(kSelf, kCmdPlay, 0x524);
			cmd(kSelf, kCmdSetZ, 0x524, 0x69);
			int32 v = var(0xb3).get();
			cmd(kSelf, kCmdPlay, 0x515, 0xffff, v == 1 ? kPlayA : kPlayB);
			cmd(kSelf, kCmdSetPos, 0x515, 0x146, 0, 0x99);
			cmd(kSelf, kCmdLoad, 0x517);
			cmd(kSelf, kCmdSetPos, 0x517, 0x145, 0, 0x99);
			VarRef a = var(0xb3);
			VarRef b = var(0xb3);
			int32 mode = a.get() == 2 ? kPlayA : kPlayB;
			cmd(kSelf, b.get() == 2 ? kCmdPlay : kCmdLoad, 0x529, 0xffff, mode);
			cmd(kSelf, kCmdSetPos, 0x529, 0x11e, 0x90, var(0xb3).get() == 2 ? 0x6b : 0x9a);
			cmd(kSelf, kCmdLoad, 0x528);
			cmd(kSelf, kCmdSetPos, 0x528, 0x11e, 0xb9, 0x6b);
			cmd(kSelf, kCmdPlay, 0x52a);
			cmd(kSelf, kCmdSetZ, 0x52a, 0x6b);
			break;
		}
		case kMsgEntered:
			if (msg.p1 == 0x35) {
				send(kMagnus, kMsgPlayAnim, 0x51a, 0x5190036, kPlayOnce);
				break;
			}
			cmd(kMyggen, kCmdPlay, 0x51f);
			send(kGameState, 10);
			break;
		case kMsgAnimNotify:
			switch (msg.p2) {
			case 0x51a:
			case 0x520:
				if (isRunning(kMyggen, 0x520) || isRunning(kMagnus, 0x51a))
					break;
				if (var(0x43).get() != 1) {
					var(0x43).set(1);
					send(kMagnus, kMsgSay, 0xa070a06, kMyggen);
				} else {
					send(kGameState, 10);
				}
				break;
			case 0x51d:
				cmd(kMagnus, kCmdStop, 0x519);
				if (!isRunning(kMyggen, 0x523))
					send(kGameState, kMsgSwitchRoom);
				break;
			case 0x523:
				cmd(kMyggen, kCmdStop, 0x51f);
				if (!isRunning(kMagnus, 0x51d))
					send(kGameState, kMsgSwitchRoom);
				break;
			default:
				break;
			}
			break;
		case kMsgCanLook:
			if (msg.p1 == 0x515)
				msg.result = var(0xb3).get() == 1;
			else if (msg.p1 == 0x52a)
				msg.result = var(0xb3).get() != 2;
			break;
		default:
			break;
		}
	}
};

// --- Object 56: room 0x38 (vtable 0x4b4b24, FUN_004711b0) ---

class Object56 : public RoomScript {
public:
	Object56(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x38)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	enum {
		kMsgSetMeter1 = 0x50,  ///< p1 = 0-5: frame of 0x56c
		kMsgSetMeter2 = 0x51   ///< p1 = 0-15: frame of 0x56d
	};

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgClick:
			if (msg.p1 == 0x567 && var(0x9d).get() == 0 && var(kVarBusy, kGameState).get() == 0) {
				var(0x9d).set(1);
				var(0x9c).set(msg.p3);
				VarRef anim = var(0x10, kMagnus);
				cmd(kMagnus, kCmdStop, (uint16)anim.get());
				cmd(kMagnus, kCmdPlay, 0x55f, 0x38, kPlayOnce);
			}
			break;
		case kMsgSetMeter1:
			if (msg.p1 > 5)
				msg.p1 = 5;
			cmd(kSelf, kCmdGotoFrame, 0x56c, msg.p1);
			break;
		case kMsgSetMeter2:
			if (msg.p1 > 0xf)
				msg.p1 = 0xf;
			cmd(kSelf, kCmdGotoFrame, 0x56d, 0xf - msg.p1);
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x55f)
				animDone();
			break;
		case kMsgSpeechDone:
			if (msg.p2 == 0xa4d) {
				send(kBroadcast, kMsgLeave, 0x38);
				send(3, 0x1b);
			} else if (msg.p2 == 0xa53) {
				send(kGameState, 10);
				cmd(kSelf, kCmdPlay, 0x567);
			} else if (msg.p2 == 0xa5d) {
				gotoRoom(0x3e);
			}
			break;
		case kMsgLeave:
			var(0x24, 0xe).set(0);
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			send(kBroadcast, 0x10);
			var(0x24, 0xe).set(1);
			cmd(kSelf, kCmdUsePalette, 0x571);
			cmd(kSelf, kCmdPlay, 0x571);
			cmd(kSelf, kCmdSetZ, 0x571, 100);
			cmd(kSelf, kCmdPlay, 0x56c, 0, kPlayB);
			cmd(kSelf, kCmdSetPos, 0x56c, 0x207, 399, 0x6e);
			cmd(kSelf, kCmdPlay, 0x56d, 0, kPlayB);
			cmd(kSelf, kCmdSetPos, 0x56d, 0xe, 0x191, 0x6e);
			cmd(kSelf, kCmdLoad, 0x56e);
			cmd(kSelf, kCmdSetPos, 0x56e, 0xf2, 0x90, 0xb4);
			cmd(kSelf, kCmdLoad, 0x56f);
			cmd(kSelf, kCmdSetPos, 0x56f, 0xf2, 0xdd, 0xb4);
			cmd(kSelf, kCmdLoad, 0x570);
			cmd(kSelf, kCmdSetPos, 0x570, 0xf2, 0x101, 0xb4);
			cmd(kSelf, kCmdLoad, 0x567);
			cmd(kSelf, kCmdSetZ, 0x567, 0xaa);
			send(kSelf, kMsgSetMeter2, var(0x19, kMagnus).get());
			send(kSelf, kMsgSetMeter1, var(0x1b, 0x15).get());
			var(0x9d).set(0);
			break;
		case kMsgEntered:
			send(0x15, kMsgSay, 0xa500a4f, kMagnus);
			break;
		default:
			break;
		}
	}

	void animDone() {
		uint16 target = 0;
		int32 v = var(0x9e).get();
		if (v == 1)
			target = 0x563;
		else if (v == 2)
			target = 0x564;
		else if (v == 3)
			target = 0x565;

		static const uint16 kAnims[3] = { 0x56e, 0x56f, 0x570 };
		int32 choice = var(0x9c).get();
		if (choice >= 1 && choice <= 3) {
			if (target == 0x562 + choice)
				cmd(kSelf, kCmdPlay, kAnims[choice - 1], 0, kPlayOnce);
			else
				cmd(kSelf, kCmdPlay, kAnims[choice - 1], 0x15, (int32)0x8000fff6);
		}

		VarRef magnus = var(0x10, kMagnus);
		cmd(kMagnus, kCmdPlay, (uint16)magnus.get());
		VarRef other = var(0x10, 0x15);
		cmd(0x15, kCmdStop, (uint16)other.get());
		cmd(0x15, kCmdPlay, target, 0x15, kPlayOnce);
		postDecrement(var(0x19, kMagnus));
		if (var(0x19, kMagnus).get() == 0)
			var(0x24, 0xe).set(0);
		send(kSelf, kMsgSetMeter2, var(0x19, kMagnus).get());
	}
};

// --- Object 57: room 0x39 (vtable 0x4b4b04, FUN_00471c70) ---

class Object57 : public RoomScript {
public:
	Object57(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x39)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x57d)
				msg.result = (uint32)msg.p1 >> 16 == 0x36b;
			break;
		case kMsgDrop:
			if (msg.p2 == 0x57d && (uint32)msg.p1 >> 16 == 0x36b) {
				takeDroppedItem(msg);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x31, (uint16)msg.p1).set(1);
				send(kMyggen, kMsgSay, 0xa740a73);
				cmd(kSelf, kCmdPlay, 0x57c, 0xffff);
			}
			break;
		case kMsgSoundDone:
			if (msg.p1 == 0xa79 && !isRunning(kMyggen, 0x57a) && !isRunning(kMagnus, 0x575) &&
			    !isRunning(0x39, 0x572))
				gotoRoom(0x36);
			break;
		case kMsgLook:
			switch (msg.p1) {
			case 0x573:
				if (var(0x8d, 0x1d).get() == 0) {
					cmd(kSelf, kCmdPlay, 0x572, 0xffff);
					send(kMagnus, kMsgSay, 0xa760a75, kMyggen);
				} else {
					int32 v = var(0x45).get();
					if (v == 0) {
						send(kSpeech, kMsgSay, 0xa820a81);
					} else if (v == 1) {
						send(kSpeech, kMsgSay, 0xa840a83);
						cmd(0x52, kCmdEnable, 0x23c, 1);
					}
				}
				break;
			case 0x57d: {
				int32 v;
				if (var(0x31, 0x71).get() == 0) {
					v = var(0x44).get();
					if (v == 0) {
						send(kMyggen, kMsgSay, 0xa720a71);
						break;
					}
					v--;
				} else {
					v = var(0x46).get();
				}
				if (v == 0)
					send(kSpeech, kMsgSay, 0xa7e0a7d);
				else if (v == 1)
					send(kSpeech, kMsgSay, 0xa800a7f);
				break;
			}
			case 0x581:
				send(kSpeech, kMsgSay, 0xa860a85);
				break;
			case 0x582:
				send(kMagnus, kMsgSay, 0xa6a0a69, kMyggen);
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x57e)
				gotoRoom(0x36);
			else if (msg.p1 == 0x580)
				gotoRoom(0x3a);
			break;
		case kMsgAnimDone:
			switch (msg.p1) {
			case 0x572:
				cmd(kMagnus, kCmdStop, 0x572);
				if (isRunning(kMyggen, 0x57a) || isRunning(kMagnus, 0xa79))
					break;
				if (!isRunning(kMyggen, 0x57a))
					gotoRoom(0x36);
				break;
			case 0x575:
				cmd(kMagnus, kCmdStop, 0x575);
				if (isRunning(kMyggen, 0x57a) || isRunning(kMagnus, 0xa79))
					break;
				if (!isRunning(0x39, 0x572))
					gotoRoom(0x36);
				break;
			case 0x57a:
				cmd(kMyggen, kCmdStop, 0x57a);
				if (isRunning(kMagnus, 0x575) || isRunning(kMagnus, 0xa79) || isRunning(0x39, 0x572))
					break;
				gotoRoom(0x36);
				break;
			case 0x57c:
				cmd(kSelf, kCmdStop, 0x57c);
				cmd(0x6e, kCmdPlay, 0x360);
				var(0xae).set(1);
				break;
			default:
				break;
			}
			break;
		case kMsgCanLook:
			switch (msg.p1) {
			case 0x573:
				if (var(0x8d, 0x1d).get() != 0 && var(0x35, 0x52).at(0x17).get() != 0)
					break;
				msg.result = 1;
				break;
			case 0x57d:
			case 0x581:
			case 0x582:
				msg.result = 1;
				break;
			default:
				break;
			}
			break;
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			if (msg.p1 == 0x3a) {
				send(kMagnus, kMsgPlayAnim, 0x577, 0x5740039, kPlayOnce);
				cmd(kMyggen, kCmdPlay, 0xa6d);
				send(0xd, 0x3d, kMyggen, 0xa6e, 0xa6d);
			} else {
				send(kGameState, kMsgSwitchRoom);
			}
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdUsePalette, 0x581);
			cmd(kSelf, kCmdPlay, 0x581);
			cmd(kSelf, kCmdPlay, 0x580);
			cmd(kSelf, kCmdSetZ, 0x580, 0x65);
			cmd(kSelf, kCmdPlay, 0x57e);
			cmd(kSelf, kCmdSetZ, 0x57e, 0x65);
			cmd(kSelf, kCmdPlay, 0x582);
			cmd(kSelf, kCmdSetZ, 0x582, 0x65);
			cmd(kSelf, kCmdPlay, 0x573);
			cmd(kSelf, kCmdSetZ, 0x573, 0x65);
			cmd(kSelf, kCmdPlay, 0x57d);
			cmd(kSelf, kCmdSetPos, 0x57d, 0x8b, 0xd8, 0x69);
			cmd(kSelf, kCmdLoad, 0x572);
			cmd(kSelf, kCmdSetPos, 0x572, 0x92, 0x86, 0x69);
			cmd(kSelf, kCmdLoad, 0x57c);
			cmd(kSelf, kCmdSetPos, 0x57c, 0x93, 0xe9, 0x69);
			break;
		case kMsgEntered:
			if (var(0x43).get() == 1) {
				send(kGameState, 10);
			} else {
				var(0x43).set(1);
				send(kMyggen, kMsgSay, 0xa660a65, kMagnus);
			}
			break;
		case 0x23:
			send(kMyggen, kMsgSay, 0xa700a6f);
			break;
		case kMsgSpeechDone:
			if (msg.p2 == 0xa6d) {
				if (!isRunning(kMagnus, 0x577))
					send(kGameState, kMsgSwitchRoom);
			} else if (msg.p2 == 0xa77) {
				cmd(kSelf, kCmdResume, 0x572);
			}
			break;
		case kMsgAnimNotify:
			if (msg.p2 == 0x577) {
				cmd(kMagnus, kCmdStop, 0x574);
				if (!isRunning(kMyggen, 0xa6d))
					send(kGameState, kMsgSwitchRoom);
			}
			break;
		default:
			break;
		}
	}
};

// --- Object 58: room 0x3a (vtable 0x4b4ae4, FUN_00472770) ---

class Object58 : public RoomScript {
public:
	Object58(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x3a) {
			switch (msg.msg) {
			case 0x23:
				send(kMyggen, kMsgSay, 0xa400a3f);
				break;
			case kMsgLeave:
				cmd(kSelf, kCmdUnload, 0);
				break;
			case kMsgExit:
				send(kGameState, kMsgSwitchRoom);
				break;
			case kMsgEnter:
				cmd(kSelf, kCmdUsePalette, 0x546);
				cmd(kSelf, kCmdPlay, 0x546);
				cmd(kSelf, kCmdSetZ, 0x546, 100);
				cmd(kSelf, kCmdPlay, 0x547);
				cmd(kSelf, kCmdSetZ, 0x547, 0x65);
				cmd(kSelf, kCmdPlay, 0x544);
				cmd(kSelf, kCmdSetZ, 0x544, 0x65);
				cmd(kSelf, kCmdPlay, 0x53e);
				cmd(kSelf, kCmdSetPos, 0x53e, 0x1a2, 0, 0x66);
				break;
			case kMsgEntered:
				if (var(0x43).get() == 1) {
					send(kGameState, 10);
				} else {
					var(0x43).set(1);
					send(kMagnus, kMsgSay, 0xa3c0a3b, kMyggen);
				}
				break;
			case kMsgCanLook:
				if (msg.p1 == 0x544 || msg.p1 == 0x546)
					msg.result = 1;
				break;
			case kMsgLook:
				if (msg.p1 == 0x544)
					send(kMagnus, kMsgSay, 0xa420a41);
				else if (msg.p1 == 0x546)
					send(kSpeech, kMsgSay, 0xa4c0a4b);
				break;
			case kMsgClick:
				if (msg.p1 == 0x544)
					gotoRoom(0x2f);
				else if (msg.p1 == 0x547)
					gotoRoom(0x39);
				break;
			default:
				break;
			}
		}
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}
};

// --- Object 59: room 0x3b (vtable 0x4b4ac4, FUN_004730f0), the four switches ---

class Object59 : public RoomScript {
public:
	Object59(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x3b)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	enum {
		kMsgTimeUp = 0x4e,   ///< p1 = 1, 2: the steps of the timeout
		kMsgCheck = 0x4f     ///< checks the four switches
	};

	/** Switch k (element k of map variable 0x9a): the children "on" and "off". */
	void toggle(int k, uint16 on, uint16 off, int32 p1) {
		var(0x9a).at(k).set(p1 == on);
		cmd(kSelf, kCmdEnable, off, var(0x9a).at(k).get());
		cmd(kSelf, kCmdEnable, on, var(0x9a).at(k).get() == 0);
		cmd(kSelf, kCmdPlay, 0x58a);
		send(kSelf, kMsgCheck);
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgClick:
			if (var(kVarBusy, kGameState).get() != 0)
				break;
			switch (msg.p1) {
			case 0x37e:
			case 0x37f:
				toggle(1, 0x37e, 0x37f, msg.p1);
				break;
			case 0x380:
			case 0x381:
				toggle(4, 0x380, 0x381, msg.p1);
				break;
			case 0x382:
			case 0x383:
				toggle(2, 0x382, 0x383, msg.p1);
				break;
			case 0x384:
			case 0x385:
				toggle(3, 0x384, 0x385, msg.p1);
				break;
			case 0x598:
				gotoRoom(0x33);
				break;
			default:
				break;
			}
			break;
		case kMsgCheck:
			if (var(0x9a).at(2).get() == 0 && var(0x9a).at(1).get() == 0 &&
			    var(0x9a).at(3).get() != 0 && var(0x9a).at(4).get() != 0) {
				stopTimer(3);
				cmd(kSelf, kCmdResume, 0x583);
				cmd(kSelf, kCmdPlay, 0x584);
				cmd(kSelf, kCmdFlags, 0xfe72, 0);
				cmd(kSelf, kCmdSetMouseCursor, 0xfe72, (int32)0x80000003);
				cmd(0x15, kCmdFlags, 0x59c, 1);
				cmd(0x15, kCmdSetMouseCursor, 0x59c, (int32)0x80000006);
			}
			break;
		case kMsgTimeUp:
			if (msg.p1 == 1) {
				cmd(kSelf, kCmdStop, 0x586);
				cmd(kSelf, kCmdPlay, 0x587);
				startTimer(3, 60000, 0x3b, kMsgTimeUp, 2);
			} else if (msg.p1 == 2) {
				cmd(kSelf, kCmdStop, 0x587);
				cmd(kSelf, kCmdPlay, 0x588);
				redrawNow();
				gotoRoom(0x3e);
			}
			break;
		case kMsgSoundDone:
			if (msg.p1 == 0x589 && var(0x9b).get() != 0)
				cmd(kSelf, kCmdPlay, 0x589, 0x3b);
			break;
		case kMsgAnimDone:
			if (msg.p1 != 0x583)
				break;
			if (cmd(kSelf, kCmdGetFrame, 0x583) != 0xb) {
				var(0x9b).set(0);
				cmd(kSelf, kCmdStop, 0x589);
				send(kMyggen, kMsgSay, 0xaa00a9f, 0x3b);
			} else {
				var(0x9b).set(1);
				send(kMagnus, kMsgSay, 0xa9a0a99, kMyggen);
			}
			break;
		case kMsgLook:
			switch (msg.p1) {
			case 0x583:
				send(kSpeech, kMsgSay, 0xaa40aa3);
				break;
			case 0x586:
			case 0x587:
				send(kSpeech, kMsgSay, 0xaa80aa7);
				break;
			case 0x59d:
				send(kSpeech, kMsgSay, 0xaa60aa5);
				break;
			default:
				break;
			}
			break;
		case kMsgLeave:
			stopTimer(3);
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			enter();
			break;
		case kMsgEntered:
			if (var(0x43).get() == 1) {
				send(kGameState, 10);
			} else {
				var(0x43).set(1);
				send(kMagnus, kMsgSay, 0xa880a87, kMyggen);
			}
			break;
		case kMsgSpeechDone:
			switch (msg.p2) {
			case 0xa95:
				cmd(kSelf, kCmdPlay, 0x586);
				cmd(kSelf, kCmdPlay, 0x589, 0x3b);
				send(kMyggen, kMsgSay, 0xa980a97, 0x3b);
				break;
			case 0xa97:
				cmd(kSelf, kCmdResume, 0x583, (int32)0x800b8000);
				cmd(kSelf, kCmdPlay, 0x585);
				break;
			case 0xa9b:
				startTimer(3, 5000, kSpeech, kMsgSay, 0xaa20aa1, kMyggen);
				break;
			case 0xa9d:
				var(kVarDragMode, kGameState).set(0);
				setCursor(0);
				cmd(kSelf, kCmdFlags, 0xfe72, 1);
				cmd(kSelf, kCmdSetMouseCursor, 0xfe72, (int32)0x80000006);
				startTimer(3, 30000, 0x3b, kMsgTimeUp, 1);
				break;
			case 0xa9f:
				cmd(kSelf, kCmdPlay, 0x598);
				cmd(kSelf, kCmdPause, 0x586);
				cmd(kSelf, kCmdPause, 0x587);
				break;
			default:
				break;
			}
			break;
		case kMsgCanLook:
			if (var(0x9b).get() != 0 &&
			    (msg.p1 == 0x586 || msg.p1 == 0x587 || msg.p1 == 0x583 || msg.p1 == 0x59d))
				msg.result = 1;
			else
				msg.result = 0;
			break;
		default:
			break;
		}
	}

	void enter() {
		cmd(kSelf, kCmdUsePalette, 0x59d);
		cmd(kSelf, kCmdPlay, 0x59d);
		cmd(kSelf, kCmdSetZ, 0x59d, 100);
		cmd(kSelf, kCmdLoad, 0x598);
		cmd(kSelf, kCmdSetZ, 0x598, 0x6e);
		// The four switches: the "on" child and the disabled "off" child.
		static const struct {
			uint16 on, off;
			int32 x, y;
		} kSwitches[4] = {
			{ 0x37e, 0x37f, 0x10f, 0x166 },
			{ 0x380, 0x381, 0x146, 0x168 },
			{ 0x382, 0x383, 0xf7, 0x166 },
			{ 0x384, 0x385, 300, 0x167 }
		};
		for (int i = 0; i < 4; i++) {
			cmd(kSelf, kCmdPlay, kSwitches[i].on);
			cmd(kSelf, kCmdSetPos, kSwitches[i].on, kSwitches[i].x, kSwitches[i].y, 0x6e);
			cmd(kSelf, kCmdPlay, kSwitches[i].off);
			cmd(kSelf, kCmdEnable, kSwitches[i].off, 0);
			cmd(kSelf, kCmdSetPos, kSwitches[i].off, kSwitches[i].x, kSwitches[i].y, 0x6e);
		}
		cmd(kSelf, kCmdPlay, 0x583, 0x3b, kPlayB);
		cmd(kSelf, kCmdSetPos, 0x583, 0x1c0, 0, 0x78);
		cmd(kSelf, kCmdLoad, 0x586);
		cmd(kSelf, kCmdSetPos, 0x586, 0, 0, 200);
		cmd(kSelf, kCmdLoad, 0x587);
		cmd(kSelf, kCmdSetPos, 0x587, 0, 0, 200);
		cmd(kSelf, kCmdLoad, 0x588);
		cmd(kSelf, kCmdSetPos, 0x588, 0, 0, 200);
		cmd(kSelf, kCmdLoad, 0x584);
		cmd(kSelf, kCmdLoad, 0x585);
		cmd(kSelf, kCmdLoad, 0x589);
		cmd(kSelf, kCmdLoad, 0x58a);
		cmd(kSelf, kCmdFlags, 0xfe72, 0);
		cmd(kSelf, kCmdSetMouseCursor, 0xfe72, (int32)0x80000003);
		var(0x9a).at(2).set(0);
		var(0x9a).at(1).set(0);
		var(0x9a).at(3).set(0);
		var(0x9a).at(4).set(0);
		var(0x9b).set(0);
	}
};

// --- Object 62: room 0x3e (vtable 0x4b4a64, FUN_0045a4a0) ---

class Object62 : public RoomScript {
public:
	Object62(EmuObject *obj) : RoomScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (currentRoom() == 0x3e)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case 0x59:
			if (currentRoom() == 0x3e)
				send(kGameState, 0x19);
			break;
		case kMsgClick:
			if (msg.p1 == 0x4e2)
				post(0xe, 0x28, 0x3e, 0x59, 2);
			break;
		case kMsgKeyDown:
			if (msg.p1 == 0x1b)
				post(0xe, 0x28, 0x3e, 0x59, 2);
			else if (msg.p1 == 0x74)
				send(0xe, kMsgKeyDown, 0x74, msg.p2, msg.p3);
			break;
		case kMsgLook:
			if (msg.p1 == 0x4df)
				send(kSpeech, kMsgSay, 0x7080707);
			break;
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			setKeyFocus(0);
			setEventMask(0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdUsePalette, 0x4e2);
			cmd(kSelf, kCmdPlay, 0x4e2);
			cmd(kSelf, kCmdSetZ, 0x4e2, 100);
			cmd(kSelf, kCmdPlay, 0x4e0);
			cmd(kSelf, kCmdSetPos, 0x4e0, 0x4e, 0x120, 0x65);
			cmd(kSelf, kCmdPlay, 0x4e1);
			cmd(kSelf, kCmdSetPos, 0x4e1, 0x6c, 0x149, 0x65);
			cmd(kSelf, kCmdPlay, 0x4e3);
			cmd(kSelf, kCmdSetPos, 0x4e3, 0x16b, 0, 0x65);
			cmd(kSelf, kCmdPlay, 0x239);
			setKeyFocus(0x3e);
			setEventMask(0x10000);
			break;
		case kMsgEntered:
			send(kBroadcast, 0x10);
			send(kGameState, 10);
			break;
		default:
			break;
		}
	}
};

// --- Object 63 (vtable 0x4b4a44, FUN_004011a0), an item seen in rooms 0x24 and 0x35 ---

class Object63 : public HeldItem {
public:
	Object63(EmuObject *obj) : HeldItem(obj) {}

protected:
	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgCanLook:
			msg.result = 1;
			break;
		case kMsgLeave:
			if (msg.p1 == 0x24)
				cmd(kSelf, kCmdUnload, 0x2c5);
			if (msg.p1 == 0x35)
				cmd(kSelf, kCmdUnload, 0x2c6);
			break;
		case kMsgEnter:
			if (msg.p1 == 0x24) {
				cmd(kSelf, kCmdLoad, 0x2c5);
				cmd(kSelf, kCmdSetPos, 0x2c5, 0xc2, 0x141, 0x6e);
				if (var(0x2d).get() == 0 && var(0x6c, 0x35).get() == 0 && var(0x52, 0x24).get() != 0)
					cmd(kSelf, kCmdPlay, 0x2c5);
			}
			if (msg.p1 == 0x35) {
				cmd(kSelf, kCmdLoad, 0x2c6);
				cmd(kSelf, kCmdSetPos, 0x2c6, 0x13a, 0xe4, 0x6e);
				if (var(0x2d).get() == 0 && var(0x6c, 0x35).get() != 0)
					cmd(kSelf, kCmdPlay, 0x2c6);
			}
			break;
		case kMsgLook:
			send(kSpeech, kMsgSay, 0x7d807d9);
			break;
		case kMsgClick:
			if (msg.p1 == 0x2c6) {
				if (var(0x6a, 0x35).get() == 0) {
					cmd(0x35, kCmdResume, 0x511);
					cmd(0x35, kCmdPlay, 0x512);
				} else {
					send(0x35, kMsgPlayAnim, 0x513, 0x35, (int32)0x80003fff);
					cmd(0x35, kCmdPlay, 0x514);
				}
			}
			break;
		default:
			break;
		}
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}
};

// --- Object 64 (vtable 0x4b4a24, FUN_00404cb0), an item in room 0x36 that takes item 0x70 ---

class Object64 : public HeldItem {
public:
	Object64(EmuObject *obj) : HeldItem(obj) {}

protected:
	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgLeave:
			if (msg.p1 == 0x36)
				cmd(kSelf, kCmdUnload, 0x2c9);
			break;
		case kMsgEnter:
			if (msg.p1 == 0x36) {
				cmd(kSelf, kCmdLoad, 0x2c9);
				cmd(kSelf, kCmdSetPos, 0x2c9, 0x87, 0x152, 0x6e);
				if (var(0x2d).get() == 0 && var(0x31).get() == 0)
					cmd(kSelf, kCmdPlay, 0x2c9);
			}
			break;
		case kMsgCanLook:
			msg.result = 1;
			break;
		case kMsgLook:
			send(kSpeech, kMsgSay, 0x7da07db);
			break;
		case 0x43:
			msg.result = msg.p1 == 0x70;
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt: {
			int32 sprite = var(0x30).get();
			msg.result = msg.p2 == sprite && (int16)msg.p1 == 0x70;
			break;
		}
		case kMsgDrop:
			if ((int16)msg.p1 == 0x70) {
				VarRef held = var(0x30, 0x70);
				cmd((uint16)msg.p1, kCmdStop, (uint16)held.get());
				setCursor(0);
				if (msg.p2 == var(0x30).get()) {
					cmd(kSelf, kCmdStop, (uint16)var(0x30).get());
					var(0x2d, (uint16)msg.p1).set(0);
					var(0x2d).set(0);
					var(0x2d, 0x79).set(1);
					var(0x31, (uint16)msg.p1).set(1);
					var(0x31).set(1);
					send(10, 0x20);
				}
			}
			break;
		default:
			break;
		}
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}
};

} // End of anonymous namespace

IVANOFF_SCRIPT(Object49) { return new Object49(obj); }
IVANOFF_SCRIPT(Object50) { return new Object50(obj); }
IVANOFF_SCRIPT(Object51) { return new Object51(obj); }
IVANOFF_SCRIPT(Object52) { return new Object52(obj); }
IVANOFF_SCRIPT(Object53) { return new Object53(obj); }
IVANOFF_SCRIPT(Object54) { return new Object54(obj); }
IVANOFF_SCRIPT(Object56) { return new Object56(obj); }
IVANOFF_SCRIPT(Object57) { return new Object57(obj); }
IVANOFF_SCRIPT(Object58) { return new Object58(obj); }
IVANOFF_SCRIPT(Object59) { return new Object59(obj); }
IVANOFF_SCRIPT(Object62) { return new Object62(obj); }
IVANOFF_SCRIPT(Object63) { return new Object63(obj); }
IVANOFF_SCRIPT(Object64) { return new Object64(obj); }

} // End of namespace Ivanoff
