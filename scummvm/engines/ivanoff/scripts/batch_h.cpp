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

// Batch h: items that lie in a room until they are picked up (objects
// 66-103 and some classes whose object id is set at run time).

#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

/**
 * The common part of most items: the sprite shown in a room at a fixed
 * position while the item lies there. On kMsgLeave of the room the sprite
 * is unloaded; on kMsgEnter it is loaded, placed and played if isShown().
 */
template<class Base>
class PlacedItem : public Base {
public:
	PlacedItem(EmuObject *obj, uint16 room, uint16 sprite, int32 x, int32 y, int32 z = 0x6e) :
		Base(obj), _room(room), _sprite(sprite), _x(x), _y(y), _z(z) {}

protected:
	/** Default: neither picked up (0x2d) nor used (0x31). */
	virtual bool isShown() {
		return this->var(0x2d).get() == 0 && this->var(0x31).get() == 0;
	}

	/** kMsgLeave / kMsgEnter for the item's room. */
	void handlePlace(Message &msg) {
		if (msg.msg == kMsgLeave) {
			if (msg.p1 == _room)
				this->cmd(kSelf, kCmdUnload, _sprite);
		} else if (msg.msg == kMsgEnter) {
			if (msg.p1 == _room) {
				this->cmd(kSelf, kCmdLoad, _sprite);
				this->cmd(kSelf, kCmdSetPos, _sprite, _x, _y, _z);
				if (isShown())
					this->cmd(kSelf, kCmdPlay, _sprite);
			}
		}
	}

	uint16 _room, _sprite;
	int32 _x, _y, _z;
};

/**
 * An item (base FUN_00423aa0) that can be looked at (0x3b answers 1,
 * 0x3c says the text) and, when room is not 0, lies in a room.
 */
class RoomItem : public PlacedItem<ItemScript> {
public:
	RoomItem(EmuObject *obj, uint32 text, uint16 room = 0, uint16 sprite = 0, int32 x = 0, int32 y = 0) :
		PlacedItem<ItemScript>(obj, room, sprite, x, y), _text(text) {}

	void handle(Message &msg) override {
		handleRoomItem(msg);
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}

protected:
	/** 0x3b: may the item be looked at? */
	virtual int32 canLook() { return 1; }

	void handleRoomItem(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
		case kMsgEnter:
			if (_room)
				handlePlace(msg);
			break;
		case 0x3b:
			msg.result = canLook();
			break;
		case 0x3c:
			send(kSpeech, kMsgSay, _text);
			break;
		default:
			break;
		}
	}

	uint32 _text;
};

/** A RoomItem shown while it is not picked up (0x31 is not checked). */
class RoomItemNotUsed : public RoomItem {
public:
	RoomItemNotUsed(EmuObject *obj, uint32 text, uint16 room, uint16 sprite, int32 x, int32 y) :
		RoomItem(obj, text, room, sprite, x, y) {}

protected:
	bool isShown() override { return var(0x2d).get() == 0; }
};

/**
 * A RoomItem that takes another item (partner): dropping the partner on
 * it uses both up and gives the item product.
 */
class CombineItem : public RoomItem {
public:
	CombineItem(EmuObject *obj, uint32 text, uint16 room, uint16 sprite, int32 x, int32 y,
	            uint16 partner, uint16 product) :
		RoomItem(obj, text, room, sprite, x, y), _partner(partner), _product(product) {}

	void handle(Message &msg) override {
		switch (msg.msg) {
		case 0x43:
			msg.result = msg.p1 == _partner;
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			msg.result = msg.p2 == var(0x30).get() && (int16)msg.p1 == _partner;
			break;
		case kMsgDrop:
			if ((int16)msg.p1 == _partner) {
				cmd(msg.p1, kCmdStop, var(0x30, _partner).get());
				setCursor(0);
				if (msg.p2 == var(0x30).get()) {
					cmd(kSelf, kCmdStop, var(0x30).get());
					var(0x2d, msg.p1).set(0);
					var(0x2d).set(0);
					var(0x2d, _product).set(1);
					var(0x31, msg.p1).set(1);
					var(0x31).set(1);
					send(10, 0x20);
				}
			}
			break;
		default:
			handleRoomItem(msg);
			break;
		}
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}

private:
	uint16 _partner, _product;
};

/** A kind of object 99 (FUN_004532e0) that lies in a room. */
class Object99Item : public PlacedItem<Object99> {
public:
	Object99Item(EmuObject *obj, uint16 room, uint16 sprite, int32 x, int32 y) :
		PlacedItem<Object99>(obj, room, sprite, x, y) {}

	void handle(Message &msg) override {
		handlePlace(msg);
		if (msg.handled == 0)
			Object99::handle(msg);
	}
};

// --- Items in a room, looked at with a text ---

#define ROOM_ITEM(name, text, room, sprite, x, y) \
	class name : public RoomItem { \
	public: \
		name(EmuObject *obj) : RoomItem(obj, text, room, sprite, x, y) {} \
	};

/** Object 66 (vtable 0x4b4a04, FUN_004059c0): shown while not picked up. */
class Object66 : public RoomItemNotUsed {
public:
	Object66(EmuObject *obj) : RoomItemNotUsed(obj, 0x7de07df, 0x2b, 0x2cc, 0xfa, 0x11d) {}
};

/** Object 68 (vtable 0x4b49c4, FUN_00405f10). */
ROOM_ITEM(Object68, 0x7e207e3, 0x20, 0x2d2, 0x14b, 0x10c)
/** Object 69 (vtable 0x4b49a4, FUN_004061c0). */
ROOM_ITEM(Object69, 0x7e407e5, 0x17, 0x2d5, 0x114, 0xe9)
/** Object 70 (vtable 0x4b4984, FUN_00406470). */
ROOM_ITEM(Object70, 0x7e607e7, 0x19, 0x2d8, 0xed, 0x155)
/** Object 71 (vtable 0x4b4964, FUN_0040c930). */
ROOM_ITEM(Object71, 0x7e807e9, 0x1c, 0x2db, 0x1d6, 0x9f)
/** Object 87 (vtable 0x4b4764, FUN_00424270). */
ROOM_ITEM(Object87, 0x8060807, 0x2b, 0x304, 0xbd, 0xeb)
/** Vtable 0x4b52e4 (FUN_0045a1c0). */
ROOM_ITEM(Object4b52e4, 0x8280829, 0x2c, 0x36a, 0x11d, 0x193)
/** Vtable 0x4b5384 (FUN_0045eb70). */
ROOM_ITEM(Object4b5384, 0x8320833, 0x26, 0x377, 0x46, 0x17e)
/** Vtable 0x4b53a4 (FUN_0045ee20). */
ROOM_ITEM(Object4b53a4, 0x8340835, 0x3a, 0x37a, 0x1fe, 0x17d)
/** Vtable 0x4b53e4 (FUN_004742c0). */
ROOM_ITEM(Object4b53e4, 0x83a083b, 0x26, 0x388, 0x1b0, 0x111)
/** Vtable 0x4b5404 (FUN_004764d0, outside the typed decompilation). */
ROOM_ITEM(Object4b5404, 0x83c083d, 0x1c, 0x38b, 0x151, 0xcc)

#undef ROOM_ITEM

// --- Items that are not in a room (only looked at) ---

#define LOOK_ITEM(name, text) \
	class name : public RoomItem { \
	public: \
		name(EmuObject *obj) : RoomItem(obj, text) {} \
	};

/** Object 74 (vtable 0x4b4904, FUN_00412960). */
LOOK_ITEM(Object74, 0x7ee07ef)
/** Object 75 (vtable 0x4b48e4, FUN_00413860). */
LOOK_ITEM(Object75, 0x7f007f1)
/** Object 78 (vtable 0x4b4884, FUN_004141f0). */
LOOK_ITEM(Object78, 0x7f607f7)
/** Object 83 (vtable 0x4b47e4, FUN_0041fda0). */
LOOK_ITEM(Object83, 0x7fe07ff)
/** Object 89 (vtable 0x4b4724, FUN_00424790). */
LOOK_ITEM(Object89, 0x80e080f)
/** Object 95 (vtable 0x4b4664, FUN_0044b4f0). */
LOOK_ITEM(Object95, 0x8120813)
/** Vtable 0x4b5304 (FUN_0045a810). */
LOOK_ITEM(Object4b5304, 0x82c082d)
/** Vtable 0x4b5344 (FUN_0045e5a0). */
LOOK_ITEM(Object4b5344, 0x82e082f)
/** Vtable 0x4b53c4 (FUN_00472b10). */
LOOK_ITEM(Object4b53c4, 0x8380839)

#undef LOOK_ITEM

/** Object 73 (vtable 0x4b4924, FUN_0040fca0): message 0xd marks it picked up. */
class Object73 : public RoomItem {
public:
	Object73(EmuObject *obj) : RoomItem(obj, 0x7ec07ed) {}
	void handle(Message &msg) override {
		if (msg.msg == 0xd)
			var(0x2d).set(1);
		else
			handleRoomItem(msg);
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}
};

// --- Items that take another item ---

#define COMBINE_ITEM(name, text, room, sprite, x, y, partner, product) \
	class name : public CombineItem { \
	public: \
		name(EmuObject *obj) : CombineItem(obj, text, room, sprite, x, y, partner, product) {} \
	};

/** Object 77 (vtable 0x4b48a4, FUN_00413e30): takes item 0x4c, gives 0x4b. */
COMBINE_ITEM(Object77, 0x7f407f5, 0x1b, 0x2e9, 0x105, 0xf, 0x4c, 0x4b)
/** Object 79 (vtable 0x4b4864, FUN_004143a0): takes item 0x75, gives 0x74. */
COMBINE_ITEM(Object79, 0x7f807f9, 0x16, 0x2ee, 0x1fc, 0x163, 0x75, 0x74)
/** Object 84 (vtable 0x4b47c4, FUN_0041ff50): takes item 0x55, gives 0x53. */
COMBINE_ITEM(Object84, 0x8000801, 0x23, 0x2f9, 0x61, 0x171, 0x55, 0x53)
/** Object 85 (vtable 0x4b47a4, FUN_00420370): takes item 0x54, gives 0x53. */
COMBINE_ITEM(Object85, 0x8020803, 0x35, 0x2fc, 0x17a, 0xd4, 0x54, 0x53)
/** Object 91 (vtable 0x4b46e4, FUN_00424d80): takes item 0x5a, gives 0x59. */
COMBINE_ITEM(Object91, 0x80c080d, 0x31, 0x30d, 0x44, 0x179, 0x5a, 0x59)
/** Vtable 0x4b52c4 (FUN_00459da0): takes item 0x40, gives 0x79. */
COMBINE_ITEM(Object4b52c4, 0x8260827, 0x18, 0x367, 0x1b5, 0x101, 0x40, 0x79)
/** Vtable 0x4b5364 (FUN_0045e750): takes item 0x4f, gives 0x74. */
COMBINE_ITEM(Object4b5364, 0x8300831, 0x1c, 0x374, 0x128, 0x162, 0x4f, 0x74)

#undef COMBINE_ITEM

/** Object 90 (vtable 0x4b4704, FUN_00424940): takes item 0x5b, gives 0x59; shown once item 0x61 is used. */
class Object90 : public CombineItem {
public:
	Object90(EmuObject *obj) : CombineItem(obj, 0x80a080b, 0x1f, 0x30a, 0x244, 0x152, 0x5b, 0x59) {}

protected:
	bool isShown() override {
		return var(0x2d).get() == 0 && var(0x31).get() == 0 && var(0x31, 0x61).get() != 0;
	}
};

// --- Items with other conditions ---

/** Object 80 (vtable 0x4b4844, FUN_00419430): looked at once var 0x4b of room 0x22 is 1. */
class Object80 : public RoomItemNotUsed {
public:
	Object80(EmuObject *obj) : RoomItemNotUsed(obj, 0x7fa07fb, 0x22, 0x2f1, 0xbb, 0xf5) {}

protected:
	int32 canLook() override { return var(0x4b, 0x22).get() == 1; }
};

/** Object 81 (vtable 0x4b4824, FUN_004196d0): like object 80. */
class Object81 : public RoomItem {
public:
	Object81(EmuObject *obj) : RoomItem(obj, 0x7fc07fd, 0x22, 0x2f4, 0x245, 0x134) {}

protected:
	int32 canLook() override { return var(0x4b, 0x22).get() == 1; }
};

/** Object 86 (vtable 0x4b4784, FUN_00423fa0). */
class Object86 : public RoomItem {
public:
	Object86(EmuObject *obj) : RoomItem(obj, 0x8040805, 0x16, 0x301, 0x153, 0x15d) {}

protected:
	bool isShown() override {
		return var(0x2d, 0x56).get() == 0 && var(0x69, 0x16).get() != 0 && var(0x31).get() == 0;
	}
};

/** Object 88 (vtable 0x4b4744, FUN_00424520). */
class Object88 : public RoomItem {
public:
	Object88(EmuObject *obj) : RoomItem(obj, 0x8080809, 0x1c, 0x307, 0x4c, 0x17f) {}

protected:
	bool isShown() override {
		return var(0x2d).get() == 0 && var(0x31).get() == 0 && var(0xac, 0x1c).get() != 0;
	}
};

/** Object 97 (vtable 0x4b4624, FUN_0044e800). */
class Object97 : public RoomItem {
public:
	Object97(EmuObject *obj) : RoomItem(obj, 0x73e073d, 0x22, 0x33e, 0x126, 0x9b) {}

protected:
	bool isShown() override {
		return var(0x2d).get() == 0 && var(0x4c, 0x22).get() == 1 && var(0x31).get() == 0;
	}
	int32 canLook() override { return var(0x4b, 0x22).get() == 1; }
};

/** Object 92 (vtable 0x4b46c4, FUN_00426080): the text depends on the sprite looked at. */
class Object92 : public RoomItem {
public:
	Object92(EmuObject *obj) : RoomItem(obj, 0, 0x23, 0x312, 0x4d, 0xf) {}
	void handle(Message &msg) override {
		if (msg.msg == 0x3c)
			look(msg.p1);
		else
			handleRoomItem(msg);
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}

private:
	void look(int32 sprite) {
		if (sprite == 0x312) {
			int32 state = var(0x44).get();
			if (state == 1)
				send(kSpeech, kMsgSay, 0x78c078b);
			else if (state == 0)
				send(kSpeech, kMsgSay, 0x78a0789);
		} else if (sprite == 0x311 || sprite == 0x313) {
			send(kSpeech, kMsgSay, 0x78a0789);
		}
	}
};

/** Object 93 (vtable 0x4b46a4, FUN_0043efd0): goes with object 94. */
class Object93 : public ItemScript {
public:
	Object93(EmuObject *obj) : ItemScript(obj) {}
	void handle(Message &msg) override {
		switch (msg.msg) {
		case kMsgEnter:
			if (var(0x30).get() == 0x319) {
				var(0x30).set(0x315);
				var(0x2f).set(0x314);
			}
			break;
		case 0x3b:
			msg.result = 1;
			break;
		case 0x43: {
			uint32 dragged = (uint32)send(32000, kMsgWhoIsDragged) >> 16;
			if (dragged == 0x315 || dragged == 0x317)
				msg.result = msg.p1 == 0x5e;
			break;
		}
		default:
			break;
		}
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}
};

/** Object 94 (vtable 0x4b4684, FUN_0043f240). */
class Object94 : public RoomItemNotUsed {
public:
	Object94(EmuObject *obj) : RoomItemNotUsed(obj, 0x8100811, 0x24, 0x31b, 0x259, 0x153) {}
	void handle(Message &msg) override {
		switch (msg.msg) {
		case 0x3b:
			msg.result = 1;
			if (var(0x30, 0x5d).get() == 0x315 || var(0x2d, 0x5d).get() == 0) {
				cmd(0x5e, kCmdSetFlags, 0x31a, 1);
				cmd(0x5e, kCmdSetCursor, 0x31a, (int32)kCursorDropYes);
			} else {
				cmd(0x5e, kCmdSetFlags, 0x31a, 0);
				cmd(0x5e, kCmdSetCursor, 0x31a, (int32)0x80000003);
			}
			break;
		case 0x43: {
			uint32 dragged = (uint32)send(32000, kMsgWhoIsDragged) >> 16;
			if (dragged == 0x315 || dragged == 0x317)
				msg.result = msg.p1 == 0x5d;
			break;
		}
		case kMsgClick:
			send(10, kMsgClick, 0x23e);
			cmd(0x5d, kCmdStop, 0x315);
			var(0x2d).set(1);
			var(0x2d, 0x5d).set(1);
			var(0x30, 0x5d).set(0x317);
			var(0x2f, 0x5d).set(0x316);
			send(10, 0x20);
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt: {
			uint32 sprite = (uint32)msg.p1 >> 16;
			if (sprite == 0x315 || sprite == 0x317)
				msg.result = msg.p2 == var(0x30).get();
			break;
		}
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x317 && dropOn(msg)) {
				var(0x2d, msg.p1).set(1);
				var(0x30, msg.p1).set(0x319);
				var(0x2f, msg.p1).set(0x318);
				var(0x2d).set(1);
				cmd(kSelf, kCmdPlay, 0x47c);
				send(10, 0x20);
			}
			if ((uint32)msg.p1 >> 16 == 0x315 && dropOn(msg)) {
				var(0x2d, msg.p1).set(1);
				var(0x30, msg.p1).set(0x317);
				var(0x2f, msg.p1).set(0x316);
				var(0x2d).set(1);
				send(10, 0x20);
			}
			break;
		default:
			handleRoomItem(msg);
			break;
		}
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}

private:
	/** Stops the dragged sprite; true when it was dropped on this item's sprite. */
	bool dropOn(Message &msg) {
		cmd(msg.p1, kCmdStop, var(0x30, msg.p1).get());
		setCursor(0);
		if (msg.p2 != var(0x30).get())
			return false;
		cmd(kSelf, kCmdStop, var(0x30).get());
		return true;
	}
};

/** Object 96 (vtable 0x4b4644, FUN_0044b6e0): two sprites, in rooms 0x27 and 0x25. */
class Object96 : public ItemScript {
public:
	Object96(EmuObject *obj) : ItemScript(obj) {}
	void handle(Message &msg) override {
		switch (msg.msg) {
		case 0x3c:
			send(kSpeech, kMsgSay, 0x8140815);
			break;
		case kMsgLeave:
			if (msg.p1 == 0x27)
				cmd(kSelf, kCmdUnload, 0x322);
			if (msg.p1 == 0x25)
				cmd(kSelf, kCmdUnload, 0x321);
			break;
		case kMsgEnter:
			if (msg.p1 == 0x27) {
				cmd(kSelf, kCmdLoad, 0x322);
				cmd(kSelf, kCmdSetPos, 0x322, 0x20d, 0xe8, 0x6e);
				if (var(0x2d).get() == 0 && var(0x31).get() == 0 && var(0xaf, 0x27).get() != 0)
					cmd(kSelf, kCmdPlay, 0x322);
			}
			if (msg.p1 == 0x25) {
				cmd(kSelf, kCmdLoad, 0x321);
				cmd(kSelf, kCmdSetPos, 0x321, 0x131, 0x12f, 0x6b);
				if (var(0x2d).get() == 0 && var(0x31).get() == 1)
					cmd(kSelf, kCmdPlay, 0x321);
			}
			break;
		case 0x3b:
			msg.result = 1;
			break;
		case kMsgDropTest: {
			int32 takes = msg.p2 == 0x321 && (int16)msg.p1 == 0x79;
			msg.result = takes;
			msg.handled = takes;
			break;
		}
		case kMsgDropTestAlt:
			msg.result = msg.p2 == 0x321 && (int16)msg.p1 == 0x79;
			break;
		case kMsgDrop:
			cmd(msg.p1, kCmdStop, var(0x30, msg.p1).get());
			setCursor(0);
			var(0x2d, msg.p1).set(1);
			if ((uint32)msg.p1 >> 16 == 0x37d && msg.p2 == 0x321 && var(0x62, 0x25).get() != 0) {
				cmd(kSelf, kCmdPlay, 0x28f);
				int32 notify = 0;
				if (var(0x31, 0x60).get() == 1) {
					var(0x2d, msg.p1).set(0);
					var(0x31, msg.p1).set(1);
					var(0x31, 0x60).set(2);
					var(0x62, 0x25).set(2);
					notify = 0xffff;
				}
				cmd(0x25, kCmdPlay, 0x28e, notify, (int32)0x8000c001, 0);
			}
			break;
		default:
			break;
		}
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}
};

/** Object 98 (vtable 0x4b4604, FUN_00452480): takes the items 0x64-0x6f; its sprite shows a count (var 0x34). */
class Object98 : public ItemScript {
public:
	Object98(EmuObject *obj) : ItemScript(obj) {}
	void handle(Message &msg) override {
		switch (msg.msg) {
		case 0x21:
			if (var(0x2d).get() != 0) {
				cmd(kSelf, kCmdUnload, var(0x30).get());
				int32 count = var(0x34).get();
				if (count >= 2 && count <= 12) {
					// The sprite in the inventory (0x30); the one before it is 0x2f.
					static const uint16 kSprites[11] = {
						0x329, 0x32b, 0x32d, 0x32f, 0x331, 0x333, 0x335, 0x337, 0x325, 0x327, 0x339
					};
					uint16 sprite = kSprites[count - 2];
					var(0x30).set(sprite);
					var(0x2f).set(sprite - 1);
				}
			}
			break;
		case 0x3b:
			msg.result = 1;
			break;
		case 0x3c:
			if (var(0x34).get() == 0xc)
				send(kSpeech, kMsgSay, 0x8200821);
			else
				send(kSpeech, kMsgSay, 0x8230824);
			break;
		case 0x43:
			msg.result = takes(msg.p1);
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			msg.result = msg.p2 == var(0x30).get() && takes(msg.p1);
			break;
		case kMsgDrop:
			if (takes(msg.p1)) {
				cmd(msg.p1, kCmdStop, var(0x30, msg.p1).get());
				setCursor(0);
				if (msg.p2 == var(0x30).get()) {
					var(0x2d, msg.p1).set(0);
					var(0x31, msg.p1).set(1);
					var(0x34, 0x62).postIncrement();
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

private:
	static bool takes(int32 p1) {
		int16 item = p1;
		return item >= 0x64 && item <= 0x6f;
	}
};

/** Vtable 0x4b5324 (FUN_0045a9d0): in room 0x20; a click uses it up. */
class Object4b5324 : public RoomItem {
public:
	Object4b5324(EmuObject *obj) : RoomItem(obj, 0x82a082b, 0x20, 0x36d, 0xcd, 0x14a) {}
	void handle(Message &msg) override {
		if (msg.msg == kMsgClick) {
			send(10, kMsgClick, 0x23e);
			cmd(kSelf, kCmdStop, 0x36e);
			var(0x2d).set(0);
			var(0x31).set(1);
			var(0x2d, 0x4a).set(1);
			var(0x2d, 0x72).set(1);
			send(10, 0x20, 1);
		} else {
			handleRoomItem(msg);
		}
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}
};

// --- Kinds of object 99 that lie in a room ---

#define OBJECT99_ITEM(name, room, sprite, x, y) \
	class name : public Object99Item { \
	public: \
		name(EmuObject *obj) : Object99Item(obj, room, sprite, x, y) {} \
	};

/** Object 101 (vtable 0x4b45a4, FUN_00452d80). */
OBJECT99_ITEM(Object101, 0x1c, 0x342, 0x195, 0x10f)
/** Object 102 (vtable 0x4b4584, FUN_00452fb0). */
OBJECT99_ITEM(Object102, 0x2d, 0x345, 499, 0x15c)
/** Vtable 0x4b51c4 (FUN_00453770). */
OBJECT99_ITEM(Object4b51c4, 0x2b, 0x34e, 0x72, 0x168)
/** Vtable 0x4b51e4 (FUN_00453990). */
OBJECT99_ITEM(Object4b51e4, 0x1b, 0x351, 0x235, 0x172)
/** Vtable 0x4b5204 (FUN_00453bc0). */
OBJECT99_ITEM(Object4b5204, 0x1a, 0x354, 0xe, 0x140)
/** Vtable 0x4b5224 (FUN_00453de0). */
OBJECT99_ITEM(Object4b5224, 0x35, 0x357, 0x65, 0x125)
/** Vtable 0x4b5244 (FUN_00454000). */
OBJECT99_ITEM(Object4b5244, 0x1f, 0x35a, 0x1a6, 0x135)
/** Vtable 0x4b5264 (FUN_00454230). */
OBJECT99_ITEM(Object4b5264, 0x3a, 0x35d, 0x121, 0x11a)
/** Vtable 0x4b52a4 (FUN_004546b0). */
OBJECT99_ITEM(Object4b52a4, 0x19, 0x363, 0x214, 0xf3)

#undef OBJECT99_ITEM

/** Object 103 (vtable 0x4b4564, FUN_004531e0): shown once var 0xae of room 0x23 is set. */
class Object103 : public Object99Item {
public:
	Object103(EmuObject *obj) : Object99Item(obj, 0x23, 0x348, 0x1a7, 0x15f) {}

protected:
	bool isShown() override {
		return var(0x2d).get() == 0 && var(0x31).get() == 0 && var(0xae, 0x23).get() != 0;
	}
};

/** Vtable 0x4b5284 (FUN_00454460): shown once var 0xae of room 0x39 is set. */
class Object4b5284 : public Object99Item {
public:
	Object4b5284(EmuObject *obj) : Object99Item(obj, 0x39, 0x360, 0xbe, 0x16e) {}

protected:
	bool isShown() override {
		return var(0x2d).get() == 0 && var(0x31).get() == 0 && var(0xae, 0x39).get() != 0;
	}
};

#define SCRIPT(vtable, name) \
	IVANOFF_SCRIPT(name) { return new name(obj); }
#include "ivanoff/scripts/registry_h.h"
#undef SCRIPT

} // End of namespace Ivanoff
