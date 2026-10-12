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

// Batch d: large rooms (objects 22, 26, 40, 42).

#include "ivanoff/machine.h"
#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

enum {
	kCStringCopy = 0x49fc3a, ///< FUN_0049fc3a: CString copy constructor
	kCStringFree = 0x49fec5  ///< FUN_0049fec5: CString destructor
};

/**
 * A room with the item accepts function (slot 5 FUN_004209d0 ->
 * FUN_00423a60: no clicks while an item is dragged) and the base handler
 * FUN_004344f0 (ObjectScript).
 */
class RoomScript : public ObjectScript {
public:
	RoomScript(EmuObject *obj) : ObjectScript(obj) {}

	bool accepts(Message &msg) override {
		HeapBlock block(machine());
		if (var(kVarDragMode, kGameState).get() == 1 && msg.msg == kMsgClick)
			return false;
		return true;
	}

	void handle(Message &msg) override {
		HeapBlock block(machine());
		handleMessage(msg);
	}

protected:
	virtual void handleMessage(Message &msg) = 0;

	/**
	 * The original handlers keep a Message (0x2c bytes) on the heap while
	 * they run; holding a block of that size keeps the heap addresses of the
	 * other objects' allocations the same.
	 */
	class HeapBlock {
	public:
		HeapBlock(Machine &m) : _m(m) { _addr = m.mem().alloc(0x2c); }
		~HeapBlock() { _m.mem().free(_addr); }
	private:
		Machine &_m;
		uint32 _addr;
	};

	/**
	 * Sets the text of a text sprite to the text of a variable (slot 0x3c,
	 * a CString returned by value, copied once more as the original does).
	 */
	void cmdVarText(uint16 child, VarRef v) {
		Emu::Memory &mem = machine().mem();
		uint32 str = mem.alloc(4);
		uint32 copy = mem.alloc(4);
		uint32 arg = str;
		v.callSlot(0x3c, 1, &arg);
		machine().cpu().call(kCStringCopy, copy, str);
		cmd(kSelf, kCmdSetText, child, (int32)mem.r32(copy));
		machine().cpu().call(kCStringFree, copy);
		machine().cpu().call(kCStringFree, str);
		mem.free(copy);
		mem.free(str);
	}
};

// --- Object 22 ---

/**
 * Object 22 (vtable 0x4b4f64, FUN_00401c90): room 0x16, a puzzle of nine
 * pieces (0xf, 0x2f, 5, 0xc, 8, 0x27, 0x2d, 0x15, 0x25) dropped on the
 * slots 0x1c-0x24.
 *
 * Variables: 0x64 map slot -> piece, 0x65 map slot -> position,
 * 0x66 map piece -> position, 0x67 map piece -> slot, 0x68, 0x69.
 */
class Object22 : public RoomScript {
public:
	Object22(EmuObject *obj) : RoomScript(obj) {}
	void handleMessage(Message &msg) override;

private:
	static const uint16 kPieces[9];

	static bool isPiece(int32 id) {
		for (int i = 0; i < 9; i++)
			if (id == kPieces[i])
				return true;
		return false;
	}

	static bool isSlot(int32 id) {
		return id >= 0x1c && id <= 0x24;
	}

	/** The start position (y << 16) | x of a piece. */
	static int32 homePos(int32 piece) {
		switch (piece) {
		case 5: return 0xeb0063;
		case 8: return 0x91002d;
		case 0xc: return 0xd70096;
		case 0xf: return 0xaf004c;
		case 0x15: return 0x370021;
		case 0x25: return 0xfa007c;
		case 0x27: return 0x6c0073;
		case 0x2d: return 0xa9008a;
		case 0x2f: return 0x2e0096;
		default: return 0;
		}
	}

	/** Every piece is in its slot (piece i in slot 0x1c + i). */
	bool solved() {
		for (int i = 0; i < 9; i++)
			if (var(0x67).at(kPieces[i]).get() != 0x1c + i)
				return false;
		return true;
	}

	void enableExit(int32 enable) {
		if (var(0x35, 0x52).at(0x16).get() == 0)
			cmd(0x52, kCmdEnable, 0x23c, enable);
	}

	/** After a piece moved: the frame (0x2b) and the exit. */
	void checkSolved() {
		if (solved()) {
			cmd(kSelf, kCmdStop, 0x2b);
			enableExit(1);
		} else {
			cmd(kSelf, kCmdPlay, 0x2b);
			enableExit(0);
		}
	}

	void placePiece(uint16 piece) {
		cmd(kSelf, kCmdLoad, piece);
		VarRef a = var(0x66).at(piece);
		VarRef b = var(0x66).at(piece);
		int32 y = (int16)((uint32)a.get() >> 16);
		int32 x = (int16)b.get();
		cmd(kSelf, kCmdSetPos, piece, x, y, 0x78);
		if (var(0x67).at(piece).get() != 0) {
			uint16 slot = var(0x67).at(piece).get();
			int32 z = cmd(kSelf, kCmdGetZ, slot);
			cmd(kSelf, kCmdSetZ, piece, z + 1);
		}
	}

	void onEnter();
};

const uint16 Object22::kPieces[9] = { 0xf, 0x2f, 5, 0xc, 8, 0x27, 0x2d, 0x15, 0x25 };

void Object22::onEnter() {
	static const uint16 kSprites[] = {
		1, 7, 0x2c, 0x29, 0xb, 0x1b, 0x2a,
		0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24
	};
	static const int32 kZ[] = {
		100, 0x6e, 0x6e, 0x6e, 0x6e, 0x6e, 0x78,
		0xdc, 0xe6, 0xdc, 0xd2, 200, 0xf0, 0x104, 0xfa, 0xdc
	};

	cmd(kSelf, kCmdUsePalette, 1);
	for (int i = 0; i < ARRAYSIZE(kSprites); i++) {
		cmd(kSelf, kCmdPlay, kSprites[i]);
		cmd(kSelf, kCmdSetZ, kSprites[i], kZ[i]);
	}
	for (int i = 0; i < 9; i++)
		placePiece(kPieces[i]);

	if (solved()) {
		if (var(0x35, 0x52).at(0x16).get() == 0)
			cmd(0x52, kCmdEnable, 0x23c, 1);
	} else {
		cmd(kSelf, kCmdPlay, 0x2b);
		cmd(kSelf, kCmdSetPos, 0x2b, 0x13b, 200, 0x65);
	}

	cmd(kSelf, kCmdSetPos, 2, 0x4e, 0xbe, 0x65);
	if (var(0x68).get() == 0) {
		cmd(kSelf, kCmdPlay, 2, 0xffff, (int32)0xc0003fff);
		cmd(kSelf, kCmdSetCursor, 2, (int32)0x80000006);
	} else {
		cmd(kSelf, kCmdPlay, 2, 0, (int32)0xffff3fff);
		send(kSelf, 0x31);
	}

	if (var(0x69).get() == 0) {
		cmd(kSelf, kCmdLoad, 4);
		cmd(kSelf, kCmdPlay, 3);
		cmd(kSelf, kCmdSetPos, 3, 0x150, 0x159, 0x65);
		cmd(kSelf, kCmdSetCursor, 0x2a, (int32)0x80000006);
	} else {
		cmd(kSelf, kCmdPlay, 4);
	}
	cmd(kSelf, kCmdSetPos, 4, 0x14d, 0x15a, 0x65);
}

void Object22::handleMessage(Message &msg) {
	switch (msg.msg) {
	case kMsgClick:
		switch (msg.p1) {
		case 2:
			if (var(0x68).get() != 0)
				break;
			cmd(kSelf, kCmdResume, 2);
			cmd(kSelf, kCmdSetCursor, 2, (int32)0x80000003);
			var(0x68).set(1);
			break;
		case 7:
			send(kGameState, kMsgGotoRoom, 0x31);
			break;
		case 0x1b:
			send(0x30, 0x32);
			break;
		case 0x2a:
			if (var(0x69).get() != 0)
				break;
			cmd(kSelf, kCmdStop, 3);
			cmd(kSelf, kCmdPlay, 4);
			cmd(0x56, kCmdPlay, 0x301);
			cmd(kSelf, kCmdSetCursor, 0x2a, (int32)0x80000003);
			var(0x69).set(1);
			break;
		default:
			break;
		}
		break;
	case kMsgAnimDone:
		if (msg.p1 == 2)
			send(kSelf, 0x31);
		break;
	case kMsgDragStartAlt: {
		cmd(kSelf, kCmdMove, msg.p1, 0, 1, 0);
		setCursor(kCursorDrag);
		uint16 piece = msg.p1;
		if (var(0x67).at(piece).get() != 0) {
			VarRef slot = var(0x67).at(piece);
			VarRef pieces = var(0x64);
			pieces.at(slot.get()).set(0);
		}
		break;
	}
	case kMsgDropTest:
		if (isPiece((int16)((uint32)msg.p1 >> 16))) {
			if (isSlot(msg.p2) && var(0x64).at((uint16)msg.p2).get() == 0) {
				setCursor(kCursorDropNo);
				msg.result = 1;
				break;
			}
			setCursor(kCursorDrag);
		}
		break;
	case kMsgDropped:
		if (isPiece(msg.p1))
			cmd(kSelf, kCmdDetach, msg.p1);
		break;
	case kMsgDrop: {
		if (!isPiece((int16)((uint32)msg.p1 >> 16)))
			break;
		uint16 piece = (uint32)msg.p1 >> 16;
		if (isSlot(msg.p2) && var(0x64).at((uint16)msg.p2).get() == 0) {
			var(0x64).at((uint16)msg.p2).set(piece);
			var(0x67).at(piece).set(msg.p2);
			VarRef slotPos = var(0x65).at((uint16)msg.p2);
			VarRef piecePos = var(0x66).at(piece);
			piecePos.set(slotPos.get());
			VarRef a = var(0x65).at((uint16)msg.p2);
			VarRef b = var(0x65).at((uint16)msg.p2);
			int32 y = (int16)((uint32)a.get() >> 16);
			int32 x = (int16)b.get();
			cmd(kSelf, kCmdSetPos, piece, x, y, 0);
			int32 z = cmd(kSelf, kCmdGetZ, msg.p2);
			cmd(kSelf, kCmdSetZ, piece, z + 1);
		}
		setCursor(0);
		checkSolved();
		break;
	}
	case kMsgClickNoDrag:
		if (isPiece(msg.p1)) {
			int32 pos = homePos(msg.p1);
			cmd(kSelf, kCmdDetach, msg.p1);
			cmd(kSelf, kCmdSetPos, msg.p1, (int16)pos, (int16)((uint32)pos >> 16));
			var(0x67).at((uint16)msg.p1).set(0);
			var(0x66).at((uint16)msg.p1).set(pos);
		}
		checkSolved();
		setCursor(0);
		break;
	case 0x3c:
		switch (msg.p1) {
		case 0xb:
			send(kSpeech, kMsgSay, 0x5b905b8);
			break;
		case 0x1b: {
			int32 v = var(0x44).get();
			if (v == 0)
				send(0x10, kMsgSay, 0x5b105b0);
			else if (v == 1)
				send(kSpeech, kMsgSay, 0x5b305b2);
			else if (v == 2)
				send(kSpeech, kMsgSay, 0x5b505b4);
			break;
		}
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x29:
			if (var(0x68).get() == 0)
				send(0xf, kMsgSay, 0x5a905a8, 0x10);
			break;
		case 0x2a:
			if (var(0x69).get() == 0)
				send(0x10, kMsgSay, 0x5ad05ac);
			break;
		case 0x2c:
			send(kSpeech, kMsgSay, 0x5b705b6);
			break;
		default:
			break;
		}
		break;
	case kMsgLeave:
		if (msg.p1 == 0x16) {
			static const uint16 kSprites[] = {
				1, 7, 0x29, 0x2c, 0xb, 0x1b, 0x2a,
				0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24,
				0xf, 0x2f, 5, 0xc, 8, 0x27, 0x2d, 0x15, 0x25,
				0x2b, 2, 4, 3
			};
			for (int i = 0; i < ARRAYSIZE(kSprites); i++)
				cmd(kSelf, kCmdUnload, kSprites[i]);
		}
		break;
	case kMsgExit:
		send(kGameState, kMsgSwitchRoom);
		break;
	case kMsgEnter:
		if (msg.p1 == 0x16)
			onEnter();
		break;
	case kMsgEntered:
		if (var(0x43).get() == 1) {
			send(kGameState, 10);
		} else {
			var(0x43).set(1);
			send(0xf, kMsgSay, 0x5a505a4, 0x10);
		}
		break;
	case 0xd: {
		for (int i = 0; i < 9; i++)
			var(0x66).at(kPieces[i]).set(homePos(kPieces[i]));
		static const int32 kSlotPos[9] = {
			0xe40131, 0xf70194, 0xe401ae, 0xd00198, 0xce0123,
			0x1070102, 0x11f0163, 0x10c01d8, 0xe400d5
		};
		for (int i = 0; i < 9; i++)
			var(0x65).at(0x1c + i).set(kSlotPos[i]);
		break;
	}
	case 0x31:
		for (int i = 0; i < 9; i++)
			cmd(kSelf, kCmdPlay, kPieces[i]);
		break;
	case 0x3b: {
		int32 id = msg.p1;
		if (id == 0xb || id == 0x1b || id == 0x2c) {
			msg.result = 1;
			break;
		}
		if ((isSlot(id) || id == 0x29) && var(0x68).get() == 0) {
			msg.result = 1;
			break;
		}
		msg.result = var(0x69).get() == 0 && msg.p1 == 0x2a;
		break;
	}
	default:
		break;
	}
	if (msg.handled == 0)
		ObjectScript::handle(msg);
}

// --- Object 26 ---

/**
 * Object 26 (vtable 0x4b4ee4, FUN_00408550): room 0x1a, five items
 * (variables 0xa0-0xa4: (sprite << 16) | size, chosen from the map 0xa5 by
 * 0x9f) to stack on the slots 0x65-0x69 in order of size.
 *
 * Variables: 0xa6 the free slot, 0xa7 map slot -> item (1-5),
 * 0xa8 dragging allowed.
 */
class Object26 : public RoomScript {
public:
	Object26(EmuObject *obj) : RoomScript(obj) {}
	void handleMessage(Message &msg) override;

private:
	/** The item (1-5) whose sprite is key, or 0. */
	int32 itemOf(uint32 key) {
		for (int k = 1; k <= 5; k++)
			if (key == (uint32)var(0x9f + k).get() >> 16)
				return k;
		return 0;
	}

	/** The variable of the item on a slot, or 0. */
	uint32 itemOnSlot(int32 slot) {
		for (int k = 1; k <= 5; k++)
			if (var(0xa7).at(slot).get() == k)
				return var(0x9f + k).get();
		return 0;
	}

	void clearSlots() {
		for (int i = 1; i < 6; i++)
			var(0xa7).at(i).set(0);
	}

	void placeItems(bool play);
	void layoutSlots();
	void checkStack();
	void takeOff(Message &msg);
	void handleRoom(Message &msg);
};

void Object26::placeItems(bool play) {
	static const int32 kPos[5][3] = {
		{ 0xa5, 0x150, 0x96 },
		{ 0xe3, 0x13e, 0x8c },
		{ 0x127, 0x156, 0xa0 },
		{ 500, 0x136, 0x96 },
		{ 0x221, 0x15e, 0xa0 }
	};
	for (int k = 0; k < 5; k++) {
		if (play) {
			uint16 sprite = (uint32)var(0xa0 + k).get() >> 16;
			cmd(kSelf, kCmdPlay, sprite);
		}
		uint16 sprite = (uint32)var(0xa0 + k).get() >> 16;
		cmd(kSelf, kCmdSetPos, sprite, kPos[k][0], kPos[k][1], kPos[k][2]);
	}
}

void Object26::layoutSlots() {
	// 0x52: draws the items on the slots, from the top (5) down.
	static const int32 kSlotPos[6] = { 0, 0x13f0194, 0x10c0192, 0xd90197, 0xa60192, 0x73019b };
	bool first = true;
	for (int i = 5; i > 0; i--) {
		uint32 item = itemOnSlot(i);
		if (item >> 16 == 0)
			continue;
		if (first) {
			int32 next = i == 5 ? 5 : i + 1;
			var(0xa6).set(0x64 + next);
			first = false;
		}
		int32 pos = kSlotPos[i];
		uint16 slot = var(0xa6).get();
		int32 z = cmd(kSelf, kCmdGetZ, slot);
		cmd(kSelf, kCmdSetPos, item >> 16, (int16)pos, (int16)((uint32)pos >> 16), z + 1);
	}
	if (first)
		var(0xa6).set(0x65);
}

void Object26::checkStack() {
	// 0x53: all five stacked, the largest at the bottom?
	if (var(0xa7).at(5).get() == 0)
		return;
	uint32 s1 = itemOnSlot(1);
	uint32 s2 = itemOnSlot(2);
	uint32 s3 = itemOnSlot(3);
	uint32 s4 = itemOnSlot(4);
	uint32 s5 = itemOnSlot(5);
	if ((s2 & 0xffff) < (s1 & 0xffff) && (s3 & 0xffff) < (s2 & 0xffff) &&
	    (s4 & 0xffff) < (s3 & 0xffff) && (s5 & 0xffff) < (s4 & 0xffff)) {
		for (int i = 1; i < 6; i++) {
			uint16 sprite = itemOnSlot(i) >> 16;
			cmd(kSelf, kCmdSetFlags, sprite);
			cmd(kSelf, kCmdSetCursor, sprite, (int32)0x80000003);
		}
		cmd(kSelf, kCmdResume, 0xe3);
		cmd(kSelf, kCmdPlay, 0xe2);
		send(0x10, kMsgSay, 0x6020601);
	} else {
		startTimer(4, 1000, 0x1a, 0x54);
		var(0xa8).set(1);
	}
}

void Object26::takeOff(Message &msg) {
	VarRef pos = var(0x2e);
	pos.set(cmd(kSelf, kCmdGetPos, msg.p1));
	int32 grab = cmd(kSelf, kCmdRelPos, msg.p1, msg.p2);
	cmd(kSelf, kCmdMove, msg.p1, grab, 1, 0);
	setCursor(kCursorDrag);
	// Take the item off its slot; the ones above it move down.
	for (int i = 1; i < 6; i++) {
		int32 item = itemOf(msg.p1);
		if (var(0xa7).at(i).get() != item)
			continue;
		if (i == 5)
			break;
		for (; i < 5; i++) {
			VarRef above = var(0xa7).at(i + 1);
			VarRef here = var(0xa7).at(i);
			here.set(above.get());
			var(0xa7).at(i + 1).set(0);
		}
		send(kSelf, 0x52);
		return;
	}
	var(0xa7).at(5).set(0);
	send(kSelf, 0x52);
}

void Object26::handleRoom(Message &msg) {
	switch (msg.msg) {
	case kMsgDragStartAlt:
		takeOff(msg);
		break;
	case 0x54:
		clearSlots();
		send(kSelf, 0x52);
		placeItems(false);
		var(0xa8).set(0);
		break;
	case kMsgClick:
		if (msg.p1 == 0x6b) {
			send(kGameState, kMsgGotoRoom, 0x19);
		} else if (msg.p1 == 0x6c) {
			if (var(kVarBusy, kGameState).get() == 0 && var(kVarDragMode, kGameState).get() == 0)
				send(0x21, 0x4c);
		} else if (msg.p1 == 0xe2) {
			send(kGameState, kMsgGotoRoom, 0x2e);
		}
		break;
	case kMsgCanDrag:
		msg.result = var(0xa8).get();
		break;
	case kMsgDropTest:
		if (itemOf((uint32)msg.p1 >> 16) == 0)
			break;
		if (msg.p2 == var(0xa6).get()) {
			setCursor(kCursorDropNo);
			msg.result = 1;
		} else {
			setCursor(kCursorDrag);
		}
		break;
	case kMsgDrop: {
		uint16 sprite = (uint32)msg.p1 >> 16;
		if (itemOf(sprite) != 0) {
			cmd(kSelf, kCmdDetach, sprite);
			int32 slot = var(0xa6).get();
			if (slot >= 0x65 && slot <= 0x69) {
				int32 item = itemOf((uint32)msg.p1 >> 16);
				var(0xa7).at(slot - 0x64).set(item);
			}
		}
		setCursor(0);
		send(kSelf, 0x52);
		send(kSelf, 0x53);
		break;
	}
	case kMsgClickNoDrag: {
		static const int32 kPos[6][2] = {
			{ 0, 0 },
			{ 0x15000a5, 0x96 },
			{ 0x13e00e3, 0x8c },
			{ 0x1560127, 0xa0 },
			{ 0x13601f4, 0x96 },
			{ 0x15e0221, 0xa0 }
		};
		if (itemOf(msg.p1) != 0) {
			int32 k = itemOf(msg.p1);
			cmd(kSelf, kCmdDetach, msg.p1);
			int32 pos = kPos[k][0];
			cmd(kSelf, kCmdSetPos, msg.p1, (int16)pos, (int16)((uint32)pos >> 16), kPos[k][1]);
		}
		setCursor(0);
		break;
	}
	case 0x53:
		checkStack();
		break;
	case kMsgLeave:
		stopTimer(4);
		cmd(kSelf, kCmdUnload, 0);
		break;
	case kMsgExit:
		send(kGameState, kMsgSwitchRoom);
		break;
	case kMsgEnter:
		cmd(kSelf, kCmdUsePalette, 0x6a);
		cmd(kSelf, kCmdPlay, 0x6a);
		cmd(kSelf, kCmdSetZ, 0x6a, 100);
		cmd(kSelf, kCmdPlay, 0xe3, 0, (int32)0xc0003fff);
		cmd(kSelf, kCmdSetPos, 0xe3, 0x1a7, 0x11, 0x6e);
		cmd(kSelf, kCmdPlay, 0x6b);
		cmd(kSelf, kCmdSetZ, 0x6b, 0x78);
		cmd(kSelf, kCmdPlay, 0x6c);
		cmd(kSelf, kCmdSetZ, 0x6c, 0x7d);
		cmd(kSelf, kCmdLoad, 0xe2);
		cmd(kSelf, kCmdSetZ, 0xe2, 0x78);
		// Five items chosen at random [unverified: 0x9f is a shuffle].
		for (int k = 0; k < 5; k++) {
			VarRef choice = var(0x9f);
			VarRef items = var(0xa5);
			VarRef item = items.at(choice.get());
			int32 value = item.get();
			var(0xa0 + k).set(value);
		}
		placeItems(true);
		cmd(kSelf, kCmdPlay, 0x65);
		cmd(kSelf, kCmdSetZ, 0x65, 0x82);
		for (uint16 slot = 0x66; slot <= 0x69; slot++) {
			cmd(kSelf, kCmdPlay, slot, 0x82);
			cmd(kSelf, kCmdSetZ, slot, 0x82);
		}
		clearSlots();
		var(0xa6).set(0x65);
		var(0xa8).set(0);
		break;
	case kMsgEntered:
		if (var(0x43).get() == 1) {
			send(kGameState, 10);
		} else {
			var(0x43).set(1);
			send(0xf, kMsgSay, 0x5fc05fb, 0x10);
		}
		break;
	case 0x23:
		send(0x10, kMsgSay, 0x60005ff);
		break;
	case 0x3b:
		if (itemOf(msg.p1) != 0 || msg.p1 == 0xe3 || msg.p1 == 0xe2 || msg.p1 == 0x6c)
			msg.result = 1;
		else
			msg.result = 0;
		break;
	case 0x3c:
		if (itemOf(msg.p1) != 0) {
			int32 v = var(0x44).get();
			if (v == 0)
				send(kSpeech, kMsgSay, 0x6140613);
			else if (v == 1)
				send(kSpeech, kMsgSay, 0x6160615);
			else if (v == 2)
				send(kSpeech, kMsgSay, 0x6220621);
		}
		if (msg.p1 == 0x6c) {
			int32 v = var(0x46).get();
			if (v == 0)
				send(kSpeech, kMsgSay, 0x61c061b);
			else if (v == 1)
				send(kSpeech, kMsgSay, 0x61e061d);
			else if (v == 2)
				send(kSpeech, kMsgSay, 0x620061f);
		} else if (msg.p1 > 0xe1 && msg.p1 < 0xe4) {
			int32 v = var(0x45).get();
			if (v == 0)
				send(kSpeech, kMsgSay, 0x6180617);
			else if (v == 1)
				send(kSpeech, kMsgSay, 0x61a0619);
		}
		break;
	case 0x52:
		layoutSlots();
		break;
	default:
		break;
	}
}

void Object26::handleMessage(Message &msg) {
	if (var(1, kGameState).get() == 0x1a)
		handleRoom(msg);

	if (msg.msg == 0xd) {
		static const int32 kItems[0x18] = {
			0x1a60001, 0x1a70002, 0x1a10006, 0x1aa0007, 0x1ae0008, 0x1ac000a,
			0x1ab000b, 0x1a9000c, 0x19e000d, 0x1af000f, 0x1a30011, 0x1a20014,
			0x1a5001a, 0x1ad001c, 0x1a4001d, 0x1b4001e, 0x19f0021, 0x19d002f,
			0x1b20032, 0x1b1004e, 0x1a0004f, 0x1a80050, 0x1b00052, 0x1b3005c
		};
		for (int i = 0; i < 0x18; i++)
			var(0xa5).at(i + 1).set(kItems[i]);
	}
	if (msg.handled == 0)
		ObjectScript::handle(msg);
}

// --- Objects 40 and 42 ---

/**
 * The two maze rooms: a view sprite per position (map 0x73), the
 * position a step back leads to (map 0x74); hotspots lead to another
 * position.
 *
 * Variables: 0x70 current position, 0x71 next position, 0x72 the view
 * sprite shown, 0x75 walking, 0xb0/0xb1 things found.
 */
class MazeRoom : public RoomScript {
public:
	MazeRoom(EmuObject *obj, int32 room, int32 start, int32 z, uint16 exitRoom) :
		RoomScript(obj), _room(room), _start(start), _z(z), _exitRoom(exitRoom) {}

protected:
	int32 _room;
	int32 _start;
	int32 _z;
	uint16 _exitRoom;

	/** Shows the view of the position 0x71 and walks there. */
	void showNext() {
		VarRef next = var(0x71);
		VarRef views = var(0x73);
		uint16 view = views.at(next.get()).get();
		cmd(kSelf, kCmdPlay, view, _z, (int32)0xc0003fff);
		var(0x72).set(view);
		send(kSelf, 0x3a, var(0x71).get());
	}

	/** After a step: view is the transition sprite (0: the view of 0x70). */
	void walk(int32 p1, uint16 view) {
		cmd(kSelf, kCmdStop, 0xfe03);
		cmd(kSelf, kCmdStop, 0xfe71);
		if (p1 == 0x3d3) {
			cmd(kSelf, kCmdStop, var(0x72).get());
			cmd(kSelf, kCmdStop, var(0x70).get());
			showNext();
		} else {
			var(0x75).set(1);
			setCursor(kCursorWait);
			if (view == 0) {
				VarRef cur = var(0x70);
				VarRef views = var(0x73);
				view = views.at(cur.get()).get();
			}
			if (var(0x72).get() != 0)
				cmd(kSelf, kCmdStop, var(0x72).get());
			cmd(kSelf, kCmdPlay, view, _z, (int32)0x80003fff);
			var(0x72).set(view);
		}
	}

	/** 0x3d3: back to the previous position. */
	void stepBack() {
		VarRef cur = var(0x70);
		VarRef back = var(0x74);
		VarRef prev = back.at(cur.get());
		var(0x71).set(prev.get());
	}

	/** 0x43c: the step counter (object 15, variable 0x19). */
	void countStep() {
		cmd(kSelf, kCmdStop, 0x43c);
		cmd(kSelf, kCmdRemoveTag, 0x43c, var(0x70).get());
		var(0x19, 0xf).postIncrement();
		cmdVarText(0xb2d, var(0x19, 0xf));
		send(0x10, kMsgSay, var(0xb2).get() == 0 ? 0x88e : 0x894);
	}

	/** Messages other than enter and clicks; returns true when done. */
	bool handleCommon(Message &msg) {
		switch (msg.msg) {
		case kMsgEntered:
			send(kGameState, 10);
			return true;
		case kMsgLeave:
			cmd(kSelf, kCmdStop, var(0x72).get());
			cmd(kSelf, kCmdStop, var(0x70).get());
			cmd(kSelf, kCmdStop, 0);
			return true;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			return true;
		case 0x3a:
			cmd(kSelf, kCmdPlay, msg.p1);
			var(0x70).set(msg.p1);
			if (msg.p1 != _start)
				cmd(kSelf, kCmdPlay, 0x3d3);
			var(0x75).set(0);
			setCursor(0);
			return true;
		case kMsgAnimDone:
			cmd(kSelf, kCmdStop, var(0x70).get());
			if (var(0x71).get() == 0)
				send(kGameState, kMsgGotoRoom, _exitRoom);
			else
				send(kSelf, 0x3a, var(0x71).get());
			return true;
		default:
			return msg.msg != kMsgClick;
		}
	}

	/** The common part of the enter message. */
	void enterCommon() {
		cmd(kSelf, kCmdLoad, 0);
		cmd(kSelf, kCmdUsePalette, 0x436);
		cmd(kSelf, kCmdPlay, 0x436);
		cmd(kSelf, kCmdSetZ, 0x436, 200);
		cmd(kSelf, kCmdPlay, 0xb65);
		cmd(kSelf, kCmdSetPos, 0xb65, 0x1c2, 0x14, 0xd2);
		cmd(kSelf, kCmdPlay, 0xb2d);
		cmd(kSelf, kCmdSetPos, 0xb2d, 0x208, 0x15, 0xd2);
	}

	void setMap(uint16 id, const uint16 (*pairs)[2], int count) {
		for (int i = 0; i < count; i++)
			var(id).at(pairs[i][0]).set(pairs[i][1]);
	}
};

/** Object 40 (vtable 0x4b4d24, FUN_00428f40): room 0x28, a maze. */
class Object40 : public MazeRoom {
public:
	Object40(EmuObject *obj) : MazeRoom(obj, 0x28, 0xfe41, 0x28, 0x2a) {}
	void handleMessage(Message &msg) override;

private:
	static bool isStep(int32 id) {
		switch (id) {
		case 0x3d3: case 0x3af: case 0x3a5: case 0x3a7: case 0x3a8: case 0x3a9:
		case 0x3ab: case 0x3ac: case 0x3ad: case 0x3ae: case 0x3a4: case 0x3b0:
		case 0x3b2: case 0x3b3: case 0x3b4: case 0x3b5: case 0x3b7: case 0x3b8:
		case 0x3b9: case 0x3ba: case 0x3bb: case 0x3bc: case 0x3bd: case 0x3be:
		case 0x3bf: case 0x3c0: case 0x3c1: case 0x3c3: case 0x3c2: case 0x3c4:
		case 0x3c5: case 0x3c6: case 0x3c7: case 0x3c8: case 0x3c9: case 0x3ca:
		case 0x3cb: case 0x3cc: case 0x3cd: case 0x3cf: case 0x3d0: case 0x3d1:
		case 0x3d2:
			return true;
		default:
			return false;
		}
	}

	void onEnter();

	/** A thing found by a click. */
	void find(uint16 sprite, int32 x, uint16 tag, int32 text) {
		if (var(0xb0).at(sprite).get() != 0)
			return;
		cmd(kSelf, kCmdSetPos, sprite, x, 0x1e, 0xd2);
		cmd(kSelf, kCmdRemoveTag, sprite, tag);
		cmd(kSelf, kCmdRemoveTag, sprite, 0xfe71);
		var(0xb0).at(sprite).set(1);
		send(0xf, kMsgSay, text);
	}

	/** A way that needs a thing; false (message done) when it is missing. */
	bool useThing(uint16 sprite) {
		if (var(0xb0).at(sprite).get() == 0) {
			send(0x10, kMsgSay, 0x890);
			return false;
		}
		if (cmd(kSelf, kCmdIsPlaying, sprite) != 0) {
			send(0x10, kMsgSay, 0x892);
			cmd(kSelf, kCmdStop, sprite);
		}
		return true;
	}
};

void Object40::onEnter() {
	static const uint16 kViews[][2] = {
		{ 0xfe1f, 0xab4 }, { 0xfe20, 0xab5 }, { 0xfe21, 0xab6 }, { 0xfe23, 0xab7 },
		{ 0xfe25, 0xab8 }, { 0xfe26, 0xab9 }, { 0xfe28, 0xaba }, { 0xfe29, 0xabd },
		{ 0xfe2a, 0xabe }, { 0xfe2c, 0xabf }, { 0xfe2f, 0xac0 }, { 0xfe33, 0xac2 },
		{ 0xfe35, 0xac3 }, { 0xfe36, 0xac4 }, { 0xfe38, 0xac6 }, { 0xfe39, 0xac9 },
		{ 0xfe3a, 0xaca }, { 0xfe3b, 0xacb }, { 0xfe3c, 0xacc }, { 0xfe3e, 0xacd },
		{ 0xfe3f, 0xacf }, { 0xfe40, 0xad0 }, { 0xfe41, 0xad1 }, { 0xfe43, 0xad2 },
		{ 0xfe44, 0xad3 }, { 0xfe45, 0xad4 }, { 0xfe46, 0xad5 }, { 0xfe47, 0xad6 },
		{ 0xfe48, 0xad7 }, { 0xfe49, 0xad8 }, { 0xfe4b, 0xad9 }, { 0xfe4c, 0xada },
		{ 0xfe4d, 0xadb }, { 0xfe4e, 0xadc }, { 0xfe4f, 0xadd }
	};
	static const uint16 kBack[][2] = {
		{ 0xfe1f, 0xfe20 }, { 0xfe20, 0xfe21 }, { 0xfe21, 0xfe28 }, { 0xfe22, 0xfe23 },
		{ 0xfe23, 0xfe25 }, { 0xfe25, 0xfe2c }, { 0xfe26, 0xfe1f }, { 0xfe27, 0xfe28 },
		{ 0xfe28, 0xfe29 }, { 0xfe29, 0xfe38 }, { 0xfe2a, 0xfe38 }, { 0xfe2c, 0xfe2a },
		{ 0xfe2e, 0xfe2f }, { 0xfe2f, 0xfe28 }, { 0xfe30, 0xfe2f }, { 0xfe32, 0xfe33 },
		{ 0xfe33, 0xfe3a }, { 0xfe35, 0xfe36 }, { 0xfe36, 0xfe44 }, { 0xfe37, 0xfe38 },
		{ 0xfe38, 0xfe3f }, { 0xfe39, 0xfe38 }, { 0xfe3a, 0xfe39 }, { 0xfe3b, 0xfe26 },
		{ 0xfe3c, 0xfe35 }, { 0xfe3e, 0xfe3f }, { 0xfe3f, 0xfe40 }, { 0xfe40, 0xfe41 },
		{ 0xfe43, 0xfe3c }, { 0xfe44, 0xfe4b }, { 0xfe45, 0xfe3e }, { 0xfe46, 0xfe45 },
		{ 0xfe47, 0xfe46 }, { 0xfe48, 0xfe47 }, { 0xfe49, 0xfe43 }, { 0xfe4a, 0xfe49 },
		{ 0xfe4b, 0xfe4c }, { 0xfe4c, 0xfe4d }, { 0xfe4d, 0xfe4e }, { 0xfe4e, 0xfe4f },
		{ 0xfe4f, 0xfe48 }
	};
	static const uint16 kThings[] = { 0x430, 0x432, 0x433, 0x434, 0x431 };

	send(kBroadcast, 0x10);
	var(0x24, 0xe).set(1);
	setMap(0x73, kViews, ARRAYSIZE(kViews));
	setMap(0x74, kBack, ARRAYSIZE(kBack));
	for (int i = 0; i < ARRAYSIZE(kThings); i++)
		var(0xb0).at(kThings[i]).set(0);
	enterCommon();
	cmd(kSelf, kCmdSetText, 0xb2d, 0x4c6260); // "" (a constant of mm2run.exe)
	var(0x19, 0xf).set(0);
	cmd(kSelf, kCmdSetZ, 0xfe03, 0x7d);
	cmd(kSelf, kCmdSetPos, 0xfe04, 0x50, 0x78, 0x78);
	for (int i = 0; i < ARRAYSIZE(kThings); i++)
		cmd(kSelf, kCmdSetPos, kThings[i], 0x11e, 0x160, 0xaf);
	cmd(kSelf, kCmdSetPos, 0x43c, 0x11e, 0x160, 0xaf);
	var(0x71).set(0xfe41);
	showNext();
	send(kSpeech, kMsgSay, 0x89c);
}

void Object40::handleMessage(Message &msg) {
	if (var(1, kGameState).get() != _room)
		goto base;
	if (msg.msg == kMsgEnter) {
		onEnter();
		goto base;
	}
	if (handleCommon(msg))
		goto base;
	if (var(0x75).get() != 0)
		goto base;

	switch (msg.p1) {
	case 0x430:
		find(0x430, 0x1e, 0xfe32, 0x88d);
		break;
	case 0x431:
		find(0x431, 0x136, 0xfe4a, 0x895);
		break;
	case 0x432:
		find(0x432, 100, 0xfe22, 0x893);
		break;
	case 0x433:
		find(0x433, 0xaa, 0xfe4d, 0x891);
		break;
	case 0x434:
		find(0x434, 0xf0, 0xfe30, 0x896);
		break;
	case 0x43c:
		countStep();
		break;
	default:
		break;
	}

	if (!isStep(msg.p1))
		goto base;
	var(0x24, 0xe).set(0);
	{
		uint16 view = 0;
		int32 next = -1;
		switch (msg.p1) {
		case 0x3a4: next = 0xfe26; break;
		case 0x3a5: next = 0xfe1f; break;
		case 0x3a7: next = 0xfe20; break;
		case 0x3a8: next = 0xfe22; break;
		case 0x3a9: next = 0xfe23; break;
		case 0x3ab: next = 0xfe3b; break;
		case 0x3ac: next = 0xfe21; break;
		case 0x3ad:
			var(0x71).set(0xfe27);
			view = 0xabb;
			break;
		case 0x3ae:
			var(0x71).set(0xfe2f);
			view = 0xabc;
			break;
		case 0x3af: next = 0xfe28; break;
		case 0x3b0:
			if (!useThing(0x430))
				return;
			next = 0xfe2c;
			break;
		case 0x3b2: next = 0xfe25; break;
		case 0x3b3: next = 0xfe2e; break;
		case 0x3b4:
			var(0x71).set(0xfe30);
			view = 0xac1;
			break;
		case 0x3b5: next = 0xfe32; break;
		case 0x3b7: next = 0xfe3c; break;
		case 0x3b8: next = 0xfe35; break;
		case 0x3b9:
			var(0x71).set(0xfe29);
			view = 0xac5;
			break;
		case 0x3ba: next = 0xfe2a; break;
		case 0x3bb:
			var(0x71).set(0xfe37);
			view = 0xac7;
			break;
		case 0x3bc:
			var(0x71).set(0xfe39);
			view = 0xac8;
			break;
		case 0x3bd: next = 0xfe3a; break;
		case 0x3be: next = 0xfe33; break;
		case 0x3bf:
			// The way out (next position 0).
			if (!useThing(0x431))
				return;
			next = 0;
			break;
		case 0x3c0:
			if (!useThing(0x434))
				return;
			next = 0xfe43;
			break;
		case 0x3c1: next = 0xfe45; break;
		case 0x3c2:
			var(0x71).set(0xfe38);
			view = 0xace;
			break;
		case 0x3c3: next = 0xfe3e; break;
		case 0x3c4: next = 0xfe3f; break;
		case 0x3c5: next = 0xfe40; break;
		case 0x3c6: next = 0xfe49; break;
		case 0x3c7: next = 0xfe36; break;
		case 0x3c8: next = 0xfe46; break;
		case 0x3c9: next = 0xfe47; break;
		case 0x3ca:
			if (!useThing(0x432))
				return;
			next = 0xfe48;
			break;
		case 0x3cb: next = 0xfe4f; break;
		case 0x3cc: next = 0xfe4a; break;
		case 0x3cd:
			if (!useThing(0x433))
				return;
			next = 0xfe44;
			break;
		case 0x3cf: next = 0xfe4b; break;
		case 0x3d0: next = 0xfe4c; break;
		case 0x3d1: next = 0xfe4d; break;
		case 0x3d2: next = 0xfe4e; break;
		case 0x3d3:
			stepBack();
			break;
		default:
			break;
		}
		if (next >= 0)
			var(0x71).set(next);
		walk(msg.p1, view);
	}

base:
	if (msg.handled == 0)
		ObjectScript::handle(msg);
}

/** Object 42 (vtable 0x4b4ce4, FUN_0042ce50): room 0x2a, the second maze. */
class Object42 : public MazeRoom {
public:
	Object42(EmuObject *obj) : MazeRoom(obj, 0x2a, 0xfe61, 0x2a, 0x38) {}
	void handleMessage(Message &msg) override;

private:
	void onEnter();

	/** A thing found by a click (0xb1: 1 found, 2 used). */
	void find(uint16 sprite, int32 x, uint16 tag, int32 text) {
		if (var(0xb1).at(sprite).get() != 0)
			return;
		cmd(kSelf, kCmdSetPos, sprite, x, 0x1a, 0xd2);
		cmd(kSelf, kCmdRemoveTag, sprite, tag);
		cmd(kSelf, kCmdRemoveTag, sprite, 0xfe71);
		cmd(kSelf, kCmdSetCursor, sprite, (int32)0x80000003);
		var(0xb1).at(sprite).set(1);
		send(0xf, kMsgSay, text);
	}

	/** Nothing to use here: a remark chosen by a variable. */
	void remark(uint16 id) {
		int32 v = var(id).get();
		if (v == 0)
			send(kSpeech, kMsgSay, 0x89a);
		else if (v == 1)
			send(kSpeech, kMsgSay, 0x8a3);
	}
};

void Object42::onEnter() {
	static const uint16 kViews[][2] = {
		{ 0xfe51, 0xade }, { 0xfe52, 0xadf }, { 0xfe54, 0xae1 }, { 0xfe55, 0xae2 },
		{ 0xfe56, 0xae3 }, { 0xfe57, 0xae4 }, { 0xfe59, 0xae5 }, { 0xfe5a, 0xae7 },
		{ 0xfe5b, 0xae8 }, { 0xfe5c, 0xaea }, { 0xfe5e, 0xaec }, { 0xfe5f, 0xaed },
		{ 0xfe60, 0xaee }, { 0xfe61, 0xaef }, { 0xfe62, 0xaf1 }, { 0xfe63, 0xaf4 },
		{ 0xfe64, 0xaf5 }, { 0xfe65, 0xaf6 }, { 0xfe66, 0xaf7 }, { 0xfe67, 0xaf8 },
		{ 0xfe68, 0xaf9 }, { 0xfe69, 0xafa }, { 0xfe6a, 0xafc }, { 0xfe6b, 0xafd },
		{ 0xfe6c, 0xafe }, { 0xfe6f, 0xaff }, { 0xfe70, 0xb00 }
	};
	static const uint16 kBack[][2] = {
		{ 0xfe50, 0xfe51 }, { 0xfe51, 0xfe52 }, { 0xfe52, 0xfe57 }, { 0xfe53, 0xfe52 },
		{ 0xfe54, 0xfe55 }, { 0xfe55, 0xfe56 }, { 0xfe56, 0xfe5b }, { 0xfe57, 0xfe5c },
		{ 0xfe58, 0xfe59 }, { 0xfe59, 0xfe5e }, { 0xfe5a, 0xfe54 }, { 0xfe5b, 0xfe5c },
		{ 0xfe5c, 0xfe61 }, { 0xfe5e, 0xfe62 }, { 0xfe5f, 0xfe5a }, { 0xfe60, 0xfe66 },
		{ 0xfe62, 0xfe68 }, { 0xfe63, 0xfe62 }, { 0xfe64, 0xfe5f }, { 0xfe65, 0xfe64 },
		{ 0xfe66, 0xfe6b }, { 0xfe67, 0xfe61 }, { 0xfe68, 0xfe67 }, { 0xfe69, 0xfe63 },
		{ 0xfe6a, 0xfe65 }, { 0xfe6b, 0xfe6a }, { 0xfe6c, 0xfe69 }, { 0xfe6e, 0xfe6f },
		{ 0xfe6f, 0xfe70 }, { 0xfe70, 0xfe6c }
	};
	static const uint16 kThings[] = { 0x437, 0x438, 0x439, 0x43b, 0x43a };
	static const uint16 kSprites[] = { 0x437, 0x438, 0x439, 0x43a, 0x43b, 0x431 };

	send(kBroadcast, 0x10);
	var(0x24, 0xe).set(1);
	setMap(0x73, kViews, ARRAYSIZE(kViews));
	setMap(0x74, kBack, ARRAYSIZE(kBack));
	for (int i = 0; i < ARRAYSIZE(kThings); i++)
		var(0xb1).at(kThings[i]).set(0);
	var(0xb0).set(0);
	enterCommon();
	cmdVarText(0xb2d, var(0x19, 0xf));
	cmd(kSelf, kCmdSetZ, 0xfe03, 0x7d);
	cmd(kSelf, kCmdSetPos, 0xfe04, 0x50, 0x78, 0x78);
	for (int i = 0; i < ARRAYSIZE(kSprites); i++)
		cmd(kSelf, kCmdSetPos, kSprites[i], 0x11e, 0x160, 0xaf);
	cmd(kSelf, kCmdSetPos, 0x43c, 0x11e, 0x160, 0xaf);
	for (int i = 0; i < ARRAYSIZE(kSprites); i++)
		cmd(kSelf, kCmdSetCursor, kSprites[i], (int32)0x80000007);
	var(0x71).set(0xfe61);
	showNext();
	send(kSpeech, kMsgSay, 0x89f);
}

void Object42::handleMessage(Message &msg) {
	if (var(1, kGameState).get() != _room)
		goto base;
	if (msg.msg == kMsgEnter) {
		onEnter();
		goto base;
	}
	if (handleCommon(msg))
		goto base;
	if (var(0x75).get() != 0)
		goto base;

	switch (msg.p1) {
	case 0x431:
		if (var(0xb0).get() == 0) {
			cmd(kSelf, kCmdSetPos, 0x431, 0x17c, 0x1e, 0xd2);
			cmd(kSelf, kCmdRemoveTag, 0x431, 0xfe6e);
			cmd(kSelf, kCmdRemoveTag, 0x431, 0xfe71);
			cmd(kSelf, kCmdSetCursor, 0x431, (int32)0x80000003);
			var(0xb0).set(1);
			send(0xf, kMsgSay, 0x895);
		}
		break;
	case 0x437:
		find(0x437, 5, 0xfe53, 0x897);
		break;
	case 0x438:
		find(0x438, 0x50, 0xfe5e, 0x88a);
		break;
	case 0x439:
		find(0x439, 0x9b, 0xfe58, 0x899);
		break;
	case 0x43a:
		find(0x43a, 0xe6, 0xfe50, 0x88c);
		break;
	case 0x43b:
		find(0x43b, 0x131, 0xfe66, 0x88b);
		break;
	case 0x43c:
		countStep();
		break;
	default:
		break;
	}

	if (cmd(kSelf, kCmdHasTag, msg.p1, 0xfe03) == 0)
		goto base;
	var(0x24, 0xe).set(0);
	{
		uint16 view = 0;
		int32 next = -1;
		uint16 used = 0; // a thing put to use (0xb1 -> 2)
		switch (msg.p1) {
		case 0x3d3: {
			stepBack();
			// Things used on the way back are free again.
			uint16 thing = 0;
			switch (var(0x70).get()) {
			case 0xfe59: thing = 0x438; break;
			case 0xfe62: thing = 0x437; break;
			case 0xfe6b: thing = 0x43a; break;
			case 0xfe70: thing = 0x439; break;
			default: break;
			}
			if (thing != 0 && var(0xb1).at(thing).get() == 2)
				var(0xb1).at(thing).set(1);
			break;
		}
		case 0x3ff: next = 0xfe50; break;
		case 0x400:
			if (var(0xb0).get() == 0) {
				send(0x10, kMsgSay, 0x890);
				return;
			}
			if (cmd(kSelf, kCmdIsPlaying, 0x431) != 0) {
				send(0x10, kMsgSay, 0x892);
				cmd(kSelf, kCmdStop, 0x431);
			}
			next = 0xfe51;
			break;
		case 0x401:
			var(0x71).set(0xfe53);
			view = 0xae0;
			break;
		case 0x402: next = 0xfe5a; break;
		case 0x403: next = 0xfe54; break;
		case 0x404: next = 0xfe55; break;
		case 0x405: next = 0xfe52; break;
		case 0x406:
			if (var(0xb1).at(0x438).get() == 1) {
				cmd(kSelf, kCmdStop, 0x438);
				view = 0xae6;
				var(0x71).set(0xfe59);
				used = 0x438;
			} else if (var(0xb1).at(0x438).get() != 2) {
				remark(0x45);
				return;
			} else {
				next = 0xfe58;
			}
			break;
		case 0x408: next = 0xfe5f; break;
		case 0x409: next = 0xfe56; break;
		case 0x40b: next = 0xfe57; break;
		case 0x40c:
			var(0x71).set(0xfe5b);
			view = 0xaeb;
			break;
		case 0x40d: next = 0xfe59; break;
		case 0x40e: next = 0xfe64; break;
		case 0x410:
			if (var(0xb1).at(0x43b).get() != 1) {
				int32 v = var(0x48).get();
				if (v == 0)
					send(kSpeech, kMsgSay, 0x89a);
				else if (v == 1)
					send(kSpeech, kMsgSay, 0x8a1);
				return;
			}
			cmd(kSelf, kCmdStop, 0x43b);
			view = 0xaee;
			var(0x71).set(0);
			used = 0x43b;
			break;
		case 0x412: next = 0xfe5c; break;
		case 0x413:
			send(kSpeech, kMsgSay, 0x8a2);
			return;
		case 0x414:
			var(0x71).set(0xfe67);
			view = 0xaf0;
			break;
		case 0x415:
		case 0x416:
			if (var(0xb1).at(0x437).get() == 1) {
				cmd(kSelf, kCmdStop, 0x437);
				view = 0xaf2;
				var(0x71).set(0xfe62);
				used = 0x437;
			} else if (var(0xb1).at(0x437).get() != 2) {
				if (msg.p1 == 0x416) {
					remark(0x44);
				} else {
					int32 v = var(0x44).get();
					if (v == 0)
						send(kSpeech, kMsgSay, 0x8a4);
					else if (v == 1)
						send(kSpeech, kMsgSay, 0x8a3);
				}
				return;
			} else if (msg.p1 == 0x416) {
				view = 0xaf3;
				next = 0xfe63;
			} else {
				next = 0xfe5e;
			}
			break;
		case 0x417: next = 0xfe69; break;
		case 0x418: next = 0xfe65; break;
		case 0x41a: next = 0xfe6a; break;
		case 0x41b:
			var(0x71).set(0xfe60);
			view = 0xaf7;
			break;
		case 0x41c: next = 0xfe68; break;
		case 0x41e: next = 0xfe62; break;
		case 0x41f: next = 0xfe6c; break;
		case 0x420:
			if (var(0xb1).at(0x43a).get() == 1) {
				cmd(kSelf, kCmdStop, 0x43a);
				view = 0xafb;
				var(0x71).set(0xfe6a);
				used = 0x43a;
			} else if (var(0xb1).at(0x43a).get() != 2) {
				remark(0x47);
				return;
			} else {
				next = 0xfe6b;
			}
			break;
		case 0x421: next = 0xfe66; break;
		case 0x422: next = 0xfe70; break;
		case 0x423: next = 0xfe6e; break;
		case 0x424:
			if (var(0xb1).at(0x439).get() == 1) {
				cmd(kSelf, kCmdStop, 0x439);
				view = 0xb01;
				var(0x71).set(0xfe70);
				used = 0x439;
			} else if (var(0xb1).at(0x439).get() != 2) {
				remark(0x46);
				return;
			} else {
				next = 0xfe6f;
			}
			break;
		default:
			break;
		}
		if (next >= 0)
			var(0x71).set(next);
		if (used != 0)
			var(0xb1).at(used).set(2);
		walk(msg.p1, view);
	}

base:
	if (msg.handled == 0)
		ObjectScript::handle(msg);
}

IVANOFF_SCRIPT(Object22) { return new Object22(obj); }
IVANOFF_SCRIPT(Object26) { return new Object26(obj); }
IVANOFF_SCRIPT(Object40) { return new Object40(obj); }
IVANOFF_SCRIPT(Object42) { return new Object42(obj); }

} // End of namespace Ivanoff
