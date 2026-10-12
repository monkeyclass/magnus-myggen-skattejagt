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

// Batch e: the inventory bar and its buttons (objects 9-13), the rooms
// 0x1c (object 30), 0x24 (object 36) and 0x37 (object 55), and object 82.

#include "ivanoff/machine.h"
#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

namespace {

enum {
	kCStringCtor = 0x49ff33,   ///< FUN_0049ff33: CString(const char *)
	kCStringCopy = 0x49fc3a,   ///< FUN_0049fc3a: CString copy constructor
	kCStringFree = 0x49fec5,   ///< FUN_0049fec5: CString destructor
	kCStringFormat = 0x49f000, ///< FUN_0049f000: CString::Format (cdecl)
	kCStringEmpty = 0x4c70b0,  ///< the data pointer of an empty CString
	kNewCommand = 0x476c50     ///< FUN_00476c50: a command message on the heap
};

/**
 * The original handlers run with a Message (0x2c bytes) on the heap
 * (EmuObject::accepts/handle allocate it); holding a block of that size
 * keeps the heap addresses of later allocations the same.
 */
class HeapBlock {
public:
	HeapBlock(Machine &m) : _m(m), _addr(0) {}
private:
	Machine &_m;
	uint32 _addr;
};

/**
 * Var slots 0x54 / 0x5c (v++ / v--): the temporary copy they return (a
 * variable object, 0x1c bytes, with a CString at +0xc) lives on the
 * stack in the original, so it is kept off the heap here too.
 */
static void postStep(Machine &m, VarRef v, uint32 slot) {
	if (slot == 0x54)
		v.postIncrement();
	else
		v.postDecrement();
}

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

	void postIncrement(VarRef v) { postStep(this->machine(), v, 0x54); }
	void postDecrement(VarRef v) { postStep(this->machine(), v, 0x5c); }

	/** Writes the object's event mask (+0x18). */
	void setEventMask(uint32 mask) {
		Script::setEventMask(mask);
	}

	/** Variable slot +0xc: the values of two variables are equal. */
	static bool sameValue(VarRef a, VarRef b) {
		return a.eq(b);
	}

	/**
	 * Sets a text sprite to the text of a variable (slot 0x3c, a CString
	 * by value, copied once more and released before the command, as the
	 * original does).
	 */
	void cmdVarText(uint16 child, VarRef v) {
		this->cmdString(kSelf, kCmdSetText, child, v.toString());
	}

};

} // End of anonymous namespace

// --- Object 9 ---

/** Object 9 (vtable 0x4b5104, FUN_00434590): the trash can of the inventory bar (child 0x54d) [unverified]. */
class Object9 : public HeldScript<Script> {
public:
	Object9(EmuObject *obj) : HeldScript<Script>(obj) {}

protected:
	/** Children over which the drag cursor stays the plain one. */
	static bool plainCursor(int16 child) {
		static const int16 kChildren[] = {
			1, 0x31, 0x3d, 0x51, 0x6a, 0xe9, 0xf9, 0x124, 0x125, 0x177, 0x191, 0x1e1,
			0x231, 0x242, 0x25b, 0x26f, 0x28c, 0x398, 0x427, 0x436, 0x44d, 0x45f, 0x472,
			0x494, 0x4cd, 0x4ec, 0x4fd, 0x50f, 0x525, 0x53d, 0x546, 0x559, 0x571, 0x581,
			0x59d, 0x46c, 0x23e
		};
		for (uint i = 0; i < ARRAYSIZE(kChildren); i++)
			if (kChildren[i] == child)
				return true;
		return false;
	}

	void handleMessage(Message &msg) override {
		uint16 item = msg.p1;
		uint16 child = (uint32)msg.p1 >> 16;
		switch (msg.msg) {
		case 0xf:
			cmd(kSelf, kCmdPlay, 0x54d);
			cmd(kSelf, kCmdSetPos, 0x54d, 0x180, 0x1a0, 0x3a99);
			break;
		case 0x10:
			cmd(kSelf, kCmdUnload, 0x54d);
			break;
		case kMsgCanDrag:
			msg.result = var(kVarDragMode, kGameState).get();
			break;
		case kMsgDragStartAlt:
			cmd(kSelf, kCmdEnable, 0x54d);
			setCursor(0x54c);
			break;
		case kMsgDropTest:
			if (var(kVarBusy, kGameState).get() != 0) {
				msg.result = 0;
				break;
			}
			if (((cmd(item, kCmdGetFlags, child) & 0x300) == 0x100 && send(item, 0x3b, child) != 0) || child == 0x54d)
				msg.result = 1;
			else
				msg.result = 0;
			break;
		case kMsgDropTestAlt:
			if (send(item, 0x3b, child))
				send(item, 0x3c, child);
			msg.result = child == 0x54d;
			break;
		case kMsgDragEnd:
			if (msg.p2 != 0 && !plainCursor((int16)((uint32)msg.p2 >> 16)))
				setCursor(0x54e);
			else
				setCursor(0x54c);
			break;
		case kMsgDropped:
			setCursor(0);
			cmd(kSelf, kCmdEnable, 0x54d, 1);
			break;
		case kMsgDrop:
			if (send(item, 0x3b, child))
				send(item, 0x3c, child);
			msg.result = child != 0x54d;
			break;
		default:
			break;
		}
	}
};

// --- Object 10 ---

/**
 * Object 10 (vtable 0x4b50e4, FUN_00458f70): the inventory. 0x1e opens it,
 * 0x1f closes it, 0x20 lays out the items (each item answers 0x21 with 0x22).
 *
 * Variables: 0x1c open, 0x1d the selected item ((child << 16) | object).
 * Globals: 0x4cab74 the palette before opening, 0x4cab78-0x4cab80 the
 * layout cursor (column, x, y), 0x4cab84 layout in progress.
 */
class Object10 : public HeldScript<Script> {
public:
	Object10(EmuObject *obj) : HeldScript<Script>(obj) {}

protected:
	enum {
		kSavedPalette = 0x4cab74,
		kColumn = 0x4cab78,
		kX = 0x4cab7c,
		kY = 0x4cab80,
		kLayout = 0x4cab84
	};

	void deselect() {
		VarRef sel = var(0x1d);
		VarRef sel2 = var(0x1d);
		uint16 child = (uint32)sel.get() >> 16;
		cmd(sel2.get(), kCmdStop, child);
		send(var(0x1d).get(), 0x24, 0);
		var(0x1d).set(0);
	}

	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgLeave:
			if (var(0x1c).get() != 0)
				send(kSelf, 0x1f);
			break;
		case 0xf:
			cmd(kSelf, kCmdPlay, 0x551);
			cmd(kSelf, kCmdSetPos, 0x551, 0x1c0, 0x1a0, 0x3a99);
			cmd(kSelf, kCmdPlay, 0x552);
			cmd(kSelf, kCmdSetPos, 0x552, 0x1c0, 0x1a0, 0x3a99);
			cmd(kSelf, kCmdEnable, 0x552);
			setEventMask(0x10000);
			cmd(kSelf, kCmdLoad, 0x23a);
			break;
		case 0x10:
			setEventMask(0);
			cmd(kSelf, kCmdUnload, 0x551);
			cmd(kSelf, kCmdUnload, 0x552);
			cmd(kSelf, kCmdUnload, 0x54f);
			cmd(kSelf, kCmdUnload, 0x23a);
			break;
		case 0x1e:
			if (var(0x1c).get() == 0 && var(kVarDragMode, kGameState).get() == 0 &&
					var(0x1e, 0xe).get() == 0 && var(0x40, 0x21).get() == 0 &&
					var(0x38, 0x30).get() == 0 && var(0xd, 8).get() == 0 &&
					var(0x1e, 0xe).get() == 0) {
				setGlobal32(kSavedPalette, cmd(kSelf, kCmdUsePalette, 0x54f));
				cmd(kSelf, kCmdPlay, 0x54f);
				cmd(kSelf, kCmdSetPos, 0x54f, 0, 0, 100000);
				var(0x1c).set(1);
				cmd(kSelf, kCmdEnable, 0x551);
				cmd(kSelf, kCmdEnable, 0x552, 1);
				send(kSelf, 0x20);
			}
			break;
		case 0x1f:
			if (var(0x1c).get() != 0) {
				cmd(kSelf, kCmdStop, 0x23e);
				cmd(kSelf, kCmdStop, 0x550);
				if (var(0x1d).get() != 0)
					deselect();
				setGlobal32(kLayout, 0);
				send(kBroadcast, 0x21, 0);
				int32 palette = global32(kSavedPalette);
				if (palette != 0)
					cmd(palette, kCmdUsePalette, (uint32)palette >> 16);
				cmd(kSelf, kCmdStop, 0x54f);
				var(0x1c).set(0);
				cmd(kSelf, kCmdEnable, 0x551, 1);
				cmd(kSelf, kCmdEnable, 0x552, 0);
				int32 dragged = send(32000, kMsgWhoIsDragged);
				if (dragged != 0)
					var(0x2d, dragged).set(0);
			}
			break;
		case 0x20:
			if (var(0x1c).get() != 0) {
				if (msg.p1 != 0) {
					setGlobal32(kLayout, 0);
					send(kBroadcast, 0x21, 0);
				}
				setGlobal32(kX, 0x13);
				setGlobal32(kY, 10);
				setGlobal32(kColumn, 0);
				setGlobal32(kLayout, 1);
				send(kBroadcast, 0x21, 1);
			}
			break;
		case 0x22:
			// An item shows itself: p1 object, p2 sprite, p3 slot (1-based) or 0.
			if (global32(kLayout) == 1) {
				if (cmd(msg.p1, kCmdIsBusy, msg.p2) != 0)
					break;
				if (cmd(msg.p1, kCmdIsPlaying, msg.p2) == 0)
					cmd(msg.p1, kCmdPlay, msg.p2);
				cmd(msg.p1, kCmdEnable, msg.p2, 1);
				if (msg.p3 != 0) {
					int32 slot = msg.p3 - 1;
					cmd(msg.p1, kCmdSetPos, msg.p2, (slot % 9) * 0x42 + 0x13, (slot / 9) * 0x42 + 10, 0x186a1);
					break;
				}
				cmd(msg.p1, kCmdSetPos, msg.p2, global32(kX), global32(kY), 0x186a1);
				setGlobal32(kColumn, global32(kColumn) + 1);
				if (global32(kColumn) != 9) {
					setGlobal32(kX, global32(kX) + 0x42);
				} else {
					setGlobal32(kX, 0x13);
					setGlobal32(kY, global32(kY) + 0x42);
					setGlobal32(kColumn, 0);
				}
			} else {
				if (cmd(msg.p1, kCmdIsBusy, msg.p2) == 0)
					cmd(msg.p1, kCmdEnable, msg.p2, 0);
			}
			break;
		case 0x25:
			setEventMask(msg.p1 != 0 ? 0x10000 : 0);
			break;
		case 0x26:
			var(0x2d, msg.p1).set(1);
			cmd(kSelf, kCmdPlay, 0x23a, 0xffff, 0, msg.p1);
			break;
		case 0x42:
			if (var(0x1c).get() != 0 && (int16)send(32000, kMsgWhoIsDragged) != (int16)msg.p1) {
				cmd(kSelf, kCmdPlay, 0x23e);
				cmd(kSelf, kCmdSetZ, 0x23e, 0x186a9);
				cmd(kSelf, kCmdPlay, 0x550);
				cmd(kSelf, kCmdSetPos, 0x550, 0xdc, 0x6c, 0x186aa);
				cmd(msg.p1, kCmdPlay, (uint32)msg.p1 >> 16);
				cmd(msg.p1, kCmdSetPos, (uint32)msg.p1 >> 16, 0x100, 0x90, 0x186b4);
				var(0x1d).set(msg.p1);
				send(msg.p1, 0x24, 1);
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x551 || msg.p1 == 0x552) {
				send(kSelf, var(0x1c).get() != 0 ? 0x1f : 0x1e);
			} else if (msg.p1 == 0x23e) {
				if (var(kVarBusy, kGameState).get() == 0) {
					cmd(kSelf, kCmdStop, 0x23e);
					cmd(kSelf, kCmdStop, 0x550);
					if (var(0x1d).get() != 0)
						deselect();
				}
			}
			break;
		case kMsgMouseEnter:
			if (var(0x1c).get() == 0) {
				int32 open = 0;
				int32 dragged = send(32000, kMsgWhoIsDragged);
				if (dragged != 0)
					open = var(0x2c, dragged).get();
				cmd(kSelf, kCmdEnable, 0x551, open == 0);
				cmd(kSelf, kCmdEnable, 0x552, open);
			}
			break;
		case kMsgMouseLeave:
			if (var(0x1c).get() == 0) {
				cmd(kSelf, kCmdEnable, 0x551, 1);
				cmd(kSelf, kCmdEnable, 0x552, 0);
			}
			break;
		case kMsgSoundDone:
			if (msg.p1 == 0x23a)
				send(kBroadcast, 0x23, msg.p2);
			break;
		case kMsgKeyDown:
			if (msg.p1 == 0x20)
				send(kSelf, var(0x1c).get() != 0 ? 0x1f : 0x1e);
			break;
		case kMsgDropTest:
			if ((uint32)msg.p1 >> 16 != 0x54d)
				msg.result = var(0x2c, msg.p1).get();
			else
				msg.result = msg.p2 == 0x23e;
			break;
		case kMsgDropTestAlt:
			if (msg.p2 == 0x54f) {
				msg.result = 1;
			} else if (msg.p2 == 0x23e) {
				if ((uint32)msg.p1 >> 16 == 0x54d) {
					send(kSelf, kMsgClick, 0x23e);
					msg.result = 0;
				}
			} else if (msg.p2 == 0x551 || msg.p2 == 0x552) {
				if (var(0x1c).get() != 0) {
					send(kSelf, 0x1f, 0);
					msg.result = 0;
				} else if ((uint32)msg.p1 >> 16 == 0x54d) {
					send(kSelf, 0x1e, 0);
					msg.result = 0;
				} else {
					msg.result = 1;
				}
			}
			break;
		case kMsgDrop: {
			uint16 item = msg.p1;
			cmd(msg.p1, kCmdDetach, var(0x30, item).get());
			var(0x2d, item).set(1);
			cmd(msg.p1, kCmdStop, var(0x30, item).get());
			if (var(0x1c).get() != 0)
				send(kSelf, 0x20);
			setCursor(0);
			if (var(0x32, item).get() == 0)
				cmd(kSelf, kCmdPlay, 0x23a, 0xffff, 0, msg.p1);
			VarRef held = var(0x2d, item);
			VarRef was = var(0x32, item);
			was.set(held.get());
			break;
		}
		default:
			break;
		}
	}
};

// --- Object 11 ---

/** Object 11 (vtable 0x4b50c4, FUN_00470ea0): a button of the inventory bar (child 0x559). */
class Object11 : public HeldScript<Script> {
public:
	Object11(EmuObject *obj) : HeldScript<Script>(obj) {}

protected:
	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case 0xf:
			cmd(kSelf, kCmdPlay, 0x559);
			cmd(kSelf, kCmdSetPos, 0x559, 0, 0x1a0, 15000);
			break;
		case 0x10:
			cmd(kSelf, kCmdUnload, 0x559);
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			msg.result = (uint32)msg.p1 >> 16 == 0x54d;
			break;
		default:
			break;
		}
	}
};

// --- Object 12 ---

/** Object 12 (vtable 0x4b50a4, FUN_00474080): a gauge of the inventory bar (children 0x55a, 0x55b). */
class Object12 : public HeldScript<Script> {
public:
	Object12(EmuObject *obj) : HeldScript<Script>(obj) {}

protected:
	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case 0xf:
			cmd(kSelf, kCmdPlay, 0x55a);
			cmd(kSelf, kCmdSetPos, 0x55a, 0x200, 0x1a0, 0x3a99);
			cmd(kSelf, kCmdPlay, 0x55b, 0, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0x55b, 0x200, 0x1a0, 0x3a9a);
			break;
		case 0x10:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case 0x15:
			cmd(kSelf, kCmdGotoFrame, 0x55b, msg.p1);
			break;
		default:
			break;
		}
	}
};

// --- Object 13 ---

/**
 * Object 13 (vtable 0x4b5084, FUN_0046c830): the message log of the
 * inventory bar: text sprite 0xb02, buttons 0x553 (down), 0x554, 0x555
 * (replay the voice), 0x556 (up). Up to 500 entries.
 *
 * Variables: 0x26 map text -> text style, 0x27 map entry -> (voice << 16) | text,
 * 0x28 map entry -> (sound << 16) | object, 0x29 last entry, 0x2a count,
 * 0x2b shown entry. Globals: 0x4cab88 the sound playing, 0x4cab8c it was loaded here.
 */
class Object13 : public HeldScript<Script> {
public:
	Object13(EmuObject *obj) : HeldScript<Script>(obj) {}

protected:
	enum {
		kSound = 0x4cab88,
		kSoundLoaded = 0x4cab8c
	};

	void unloadSound() {
		int32 sound = global32(kSound);
		cmd(sound, kCmdUnload, (uint32)sound >> 16);
	}

	void setVoiceCursor(int32 sound) {
		cmd(kSelf, kCmdSetCursor, 0x555, sound != 0 ? (int32)0x80000009 : (int32)0x80000003);
	}

	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case 0xf:
			cmd(kSelf, kCmdPlay, 0xb02);
			cmd(kSelf, kCmdSetPos, 0xb02, 5, 0x1a5, 0x3a99);
			cmd(kSelf, kCmdSetView, 0xb02, 0x154, 0x33);
			cmd(kSelf, kCmdPlay, 0x555);
			cmd(kSelf, kCmdSetZ, 0x555, 0x3a9a);
			cmd(kSelf, kCmdPlay, 0x556);
			cmd(kSelf, kCmdSetPos, 0x556, 0x15e, 0x1a0, 0x3a99);
			cmd(kSelf, kCmdPlay, 0x553);
			cmd(kSelf, kCmdSetPos, 0x553, 0x15e, 0x1b5, 0x3a99);
			cmd(kSelf, kCmdPlay, 0x554);
			cmd(kSelf, kCmdSetPos, 0x554, 0x15e, 0x1ca, 0x3a99);
			var(0x26).at(0xf).set(0x11);
			var(0x26).at(0x10).set(0x17);
			var(0x26).at(0x12).set(0x1a);
			break;
		case 0x10:
			cmd(kSelf, kCmdUnload, 0xb02);
			cmd(kSelf, kCmdUnload, 0x555);
			cmd(kSelf, kCmdUnload, 0x556);
			cmd(kSelf, kCmdUnload, 0x553);
			cmd(kSelf, kCmdUnload, 0x554);
			break;
		case 0x17:
			var(0x2b).set(0);
			send(kSelf, 0x3e, 0);
			break;
		case 0x3d: {
			// A new entry: p1 text, p2 voice, p3 sound child.
			int32 style = var(0x26).at((uint16)msg.p1).get();
			if (style == 0)
				style = 0x14;
			cmd(kSelf, 0x10, 0xb02, style);
			cmd(kSelf, 0xe, 0xb02, msg.p1, msg.p2);
			uint32 text = ((uint32)(uint16)msg.p2 << 16) | (uint16)msg.p1;
			uint32 sound = 0;
			if (msg.p3 != 0)
				sound = ((uint32)(uint16)msg.p3 << 16) | (uint16)msg.p1;
			postIncrement(var(0x29));
			if (var(0x2a).get() < 500)
				postIncrement(var(0x2a));
			if (var(0x29).get() > 500)
				var(0x29).set(1);
			{
				VarRef last = var(0x29);
				VarRef texts = var(0x27);
				texts.at(last.get()).set(text);
			}
			{
				VarRef last = var(0x29);
				VarRef sounds = var(0x28);
				sounds.at(last.get()).set(sound);
			}
			{
				VarRef last = var(0x29);
				VarRef shown = var(0x2b);
				shown.set(last.get());
			}
			setVoiceCursor(sound);
			break;
		}
		case 0x3e:
			if (msg.p1 < 0) {
				if (cmd(kSelf, kCmdScrollBy, 0xb02, 0, -0x33) == 0)
					send(kSelf, 0x3f, -1);
			} else if (msg.p1 == 0) {
				send(kSelf, 0x3f, 0);
			} else {
				if (cmd(kSelf, kCmdScrollBy, 0xb02, 0, 0x33) == 0)
					send(kSelf, 0x3f, 1);
			}
			break;
		case 0x3f: {
			int32 old = var(0x2b).get();
			if (msg.p1 < 0) {
				if (var(0x2b).get() > 1) {
					VarRef shown = var(0x2b);
					VarRef last = var(0x29);
					int32 prev = shown.get() - 1;
					if (prev != last.get())
						postDecrement(var(0x2b));
				} else if (var(0x2a).get() == 500 && var(0x29).get() != 500) {
					var(0x2b).set(500);
				}
			} else if (msg.p1 == 0) {
				VarRef last = var(0x29);
				VarRef shown = var(0x2b);
				shown.set(last.get());
			} else {
				if (var(0x2b).get() < 500) {
					VarRef last = var(0x29);
					VarRef shown = var(0x2b);
					if (!sameValue(shown, last))
						postIncrement(var(0x2b));
				} else if (var(0x2a).get() == 500 && var(0x29).get() != 500) {
					var(0x2b).set(1);
				}
			}
			if (var(0x2b).get() == old)
				break;
			uint32 text;
			int32 sound;
			{
				VarRef shown = var(0x2b);
				VarRef texts = var(0x27);
				text = texts.at(shown.get()).get();
			}
			{
				VarRef shown = var(0x2b);
				VarRef sounds = var(0x28);
				sound = sounds.at(shown.get()).get();
			}
			int32 style = var(0x26).at((uint16)text).get();
			if (style == 0)
				style = 0x14;
			cmd(kSelf, 0x10, 0xb02, style);
			cmd(kSelf, 0xe, 0xb02, text & 0xffff, text >> 16);
			setVoiceCursor(sound);
			break;
		}
		case 0x40: {
			if (var(kVarBusy, kGameState).get() != 0)
				break;
			VarRef shown = var(0x2b);
			VarRef sounds = var(0x28);
			int32 sound = sounds.at(shown.get()).get();
			setGlobal32(kSound, sound);
			uint16 object = sound;
			uint16 child = (uint32)sound >> 16;
			if (object == 0 || child == 0) {
				setGlobal32(kSoundLoaded, 0);
				break;
			}
			setGlobal32(kSoundLoaded, cmd(object, kCmdIsLoaded, child) == 0);
			cmd(object, kCmdPlay, child, 0xd);
			break;
		}
		case 0x41:
			if (global32(kSound) != 0)
				unloadSound();
			break;
		case kMsgClick:
			switch (msg.p1) {
			case 0x553:
				send(kSelf, 0x3e, 1);
				break;
			case 0x554:
				send(kSelf, 0x3e, 0);
				break;
			case 0x555:
				send(kSelf, 0x40);
				break;
			case 0x556:
				send(kSelf, 0x3e, -1);
				break;
			default:
				break;
			}
			break;
		case kMsgSoundDone:
			if (global32(kSoundLoaded) == 1)
				unloadSound();
			break;
		default:
			break;
		}
	}
};

// --- Object 30 ---

/**
 * Object 30 (vtable 0x4b4e64, FUN_00411950): room 0x1c, a throwing game.
 * The marker 0x173 moves (0x4b: x 0x92, y 0x93); space starts and stops
 * it, the arrow keys aim; the dart 0x169 flies to it and hits one of the
 * targets 0x16c-0x171 (score in text sprite 0xb10).
 *
 * Variables: 0x91 game running, 0x92, 0x93 the aim, 0x94 state
 * (0 aiming, 1 moving, 2 thrown, 3).
 */
class Object30 : public HeldScript<Script> {
public:
	Object30(EmuObject *obj) : HeldScript<Script>(obj) {}

protected:
	void dartLanded() {
		var(0x94).set(2);
		int32 x = var(0x92).get() * 11 - 10;
		int32 y = 0x1c0 - var(0x93).get() * 9;
		int32 points = (uint32)cmd(kSelf, kCmdHotspotHit, 0x174, x, y) >> 16;
		int32 factor = 1;
		if (cmd(kSelf, kCmdHotspotHit, 0x16c, x, y) != 0) {
			if (cmd(kSelf, kCmdHotspotHit, 0x16d, x, y) == 0) {
				factor = 2;
			} else if (cmd(kSelf, kCmdHotspotHit, 0x16e, x, y) == 0) {
				// factor 1
			} else if (cmd(kSelf, kCmdHotspotHit, 0x16f, x, y) == 0) {
				factor = 3;
			} else if (cmd(kSelf, kCmdHotspotHit, 0x170, x, y) == 0) {
				// factor 1
			} else if (cmd(kSelf, kCmdHotspotHit, 0x171, x, y) == 0) {
				points = 0x19;
			} else {
				points = 0x32;
			}
		}
		if (points == 0x32) {
			send(0x10, kMsgSay, 0x65a0659, 0x1e);
		} else if (points == 0x19 || factor == 3) {
			send(0x10, kMsgSay, 0x660065f);
		} else if (factor == 2) {
			send(0x10, kMsgSay, 0x64e064d);
		} else if (points > 0) {
			send(0x10, kMsgSay, 0x6540653);
		} else {
			switch (var(0x49).get()) {
			case 0:
				send(0xf, kMsgSay, 0x64c064b);
				break;
			case 1:
				send(0xf, kMsgSay, 0x6520651);
				break;
			case 2:
				send(0xf, kMsgSay, 0x6580657);
				break;
			case 3:
				send(0xf, kMsgSay, 0x65e065d);
				break;
			default:
				break;
			}
		}
		points *= factor;

		cmdString(kSelf, kCmdSetText, 0xb10, Common::String::format("%d", points));
	}

	void throwDart() {
		int32 x0 = var(0x92).get() * 11 + 0x2d;
		int32 y0 = var(0x93).get() * 2 + 0x163;
		int32 x1 = var(0x92).get() * 11 - 10;
		int32 y1 = 0x1c0 - var(0x93).get() * 9;
		cmd(kSelf, kCmdSetClip, 0x169, 3);
		cmd(kSelf, kCmdSetClip, 0x169, 7);
		cmdFrameTrigger(kSelf, 0x169, 3, makeCmd(0x1e, kCmdSetPos, 0x169, x0 + ((x1 - x0) / 3) * 2, y0 + ((y1 - y0) / 3) * 2, 0));
		cmdFrameTrigger(kSelf, 0x169, 7, makeCmd(0x1e, kCmdSetPos, 0x169, x1, y1, 0));
		cmd(kSelf, kCmdPlay, 0x169, 0x1e);
		cmd(kSelf, kCmdSetPos, 0x169, x0, y0, 0x4e66);
		cmd(kSelf, kCmdPlay, 0x176);
	}

	void startGame() {
		if (var(0x91).get() != 0)
			return;
		var(0x91).set(1);
		var(0x10, 0xf).set(0);
		var(0x10, 0x10).set(0);
		var(0x11, 0xf).set(0);
		var(0x11, 0x10).set(0);
		cmd(kSelf, kCmdUsePalette, 0x172);
		cmd(kSelf, kCmdPlay, 0x172);
		cmd(kSelf, kCmdPlay, 0x16b);
		cmd(kSelf, kCmdPlay, 0x16c);
		cmd(kSelf, kCmdPlay, 0x16d);
		cmd(kSelf, kCmdPlay, 0x16e);
		cmd(kSelf, kCmdPlay, 0x16f);
		cmd(kSelf, kCmdPlay, 0x170);
		cmd(kSelf, kCmdPlay, 0x171);
		cmd(kSelf, kCmdPlay, 0x174);
		cmd(kSelf, kCmdPlay, 0x175);
		cmd(kSelf, kCmdPlay, 0x16a);
		cmd(kSelf, kCmdPlay, 0x173, 0x1e, (int32)0xc0003fff);
		cmd(kSelf, kCmdPlay, 0xb10);
		cmd(kSelf, kCmdSetPos, 0xb10, 0x1ae, 10, 0x4e2a);
		cmd(kSelf, kCmdSetView, 0xb10, 200, 0x28);
		cmdString(kSelf, kCmdSetText, 0xb10, "0");
		send(kSelf, 0x4b, 0x11, 1);
		setKeyFocus(getId());
		setEventMask(0x30000);
	}

	void handleMessage(Message &msg) override {
		if (var(1, kGameState).get() != 0x1c)
			return;
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdLoad, 0);
			cmd(kSelf, kCmdSetZ, 0x172, 20000);
			cmd(kSelf, kCmdSetZ, 0x16b, 0x4e21);
			cmd(kSelf, kCmdSetZ, 0x16c, 0x4e22);
			cmd(kSelf, kCmdSetZ, 0x16d, 0x4e23);
			cmd(kSelf, kCmdSetZ, 0x16e, 0x4e24);
			cmd(kSelf, kCmdSetZ, 0x16f, 0x4e25);
			cmd(kSelf, kCmdSetZ, 0x170, 0x4e26);
			cmd(kSelf, kCmdSetZ, 0x171, 0x4e27);
			cmd(kSelf, kCmdSetPos, 0x175, 10, 0x19a, 0x4e84);
			cmd(kSelf, kCmdSetPos, 0x16a, 0x226, 0x195, 0x4e84);
			cmd(kSelf, kCmdSetZ, 0x173, 0x4e6b);
			break;
		case 0x2e:
			if (msg.p2 == 0x659) {
				send(kSelf, 0x4a);
				send(0x1c, 0x4a);
			}
			break;
		case 0x49:
			startGame();
			break;
		case 0x4a:
			if (var(0x91).get() != 0) {
				setEventMask(0);
				setKeyFocus(0);
				cmd(kSelf, kCmdStop, 0);
				cmd(0x1c, kCmdUsePalette, 0xf9);
				var(0x10, 0xf).set(0x104);
				var(0x10, 0x10).set(0x108);
				var(0x11, 0xf).set(0x105);
				var(0x11, 0x10).set(0x109);
				var(0x91).set(0);
			}
			break;
		case 0x4b:
			var(0x92).set(msg.p1);
			var(0x93).set(msg.p2);
			cmd(kSelf, kCmdSetPos, 0x173, msg.p1 * 11 + 0x2d, msg.p2 * 2 + 0x163, 0);
			break;
		case kMsgClick:
			switch (msg.p1) {
			case 0x16b:
			case 0x16d:
			case 0x16f: {
				int32 who = var(0x44).get();
				if (who == 0)
					send(kSpeech, kMsgSay, 0x6560655);
				else if (who == 1)
					send(kSpeech, kMsgSay, 0x650064f);
				break;
			}
			case 0x16c:
				send(kSpeech, kMsgSay, 0x6660665);
				break;
			case 0x16e:
				send(kSpeech, kMsgSay, 0x6640663);
				break;
			case 0x170:
				send(kSpeech, kMsgSay, 0x6620661);
				break;
			case 0x171:
				send(kSpeech, kMsgSay, 0x65c065b);
				break;
			case 0x172:
				if (var(0x94).get() == 2 || var(0x94).get() == 0)
					send(kSelf, 0x4a);
				break;
			default:
				break;
			}
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x169)
				dartLanded();
			else if (msg.p1 == 0x173)
				throwDart();
			break;
		case kMsgKeyUp:
			if (var(0x94).get() != 1 && msg.p1 == 0x20) {
				if (var(0x94).get() == 3) {
					var(0x94).set(0);
				} else {
					var(0x94).set(1);
					cmd(kSelf, kCmdResume, 0x173, (int32)0x801b0001);
				}
			}
			break;
		case kMsgKeyDown:
			if (var(0x94).get() == 1)
				break;
			if (var(0x94).get() == 2) {
				var(0x94).set(0);
				cmd(kSelf, kCmdStop, 0x169);
				cmd(kSelf, kCmdGotoFrame, 0x173);
				send(kSelf, 0x4b, var(0x92).get(), 1);
				if (msg.p1 == 0x20)
					var(0x94).set(3);
			} else if (msg.p1 == 0x20) {
				if (cmd(kSelf, kCmdIsPlaying, 0x175) != 0)
					cmd(kSelf, kCmdStop, 0x175);
				if (var(0x94).get() == 3)
					var(0x94).set(0);
				if (var(0x93).get() != 0x32) {
					int32 y = var(0x93).get() + 1;
					int32 x = var(0x92).get();
					send(kSelf, 0x4b, x, y);
				}
			} else if (msg.p1 == 0x25 || msg.p1 == 0x27) {
				// Left and right arrows.
				if (cmd(kSelf, kCmdIsPlaying, 0x16a) != 0)
					cmd(kSelf, kCmdStop, 0x16a);
				int32 limit = msg.p1 == 0x25 ? 1 : 0x32;
				if (var(0x92).get() != limit) {
					int32 y = var(0x93).get();
					int32 x = var(0x92).get() + (msg.p1 == 0x25 ? -1 : 1);
					send(kSelf, 0x4b, x, y);
				}
			}
			break;
		default:
			break;
		}
	}
};

// --- Object 36 ---

/**
 * Object 36 (vtable 0x4b4da4, FUN_0041ec00): room 0x24, five doors
 * (0x26c, 0x270, 0x272, 0x283 and the hatch 0x269); 0x26c and 0x283 are
 * opened with the items 0x308 and 0x305.
 *
 * Variables: 0x43 visited, 0x4e, 0x4f, 0x50, 0x51 door state (0 locked,
 * 1 closed, 2 open), 0x52 hatch open.
 */
class Object36 : public HeldScript<ItemScript> {
public:
	Object36(EmuObject *obj) : HeldScript<ItemScript>(obj) {}

protected:
	/** cmd Play p2 of a door: 0xc0003fff closed, 0xffff3fff open. */
	static int32 doorMode(bool open) {
		return open ? (int32)0xffff3fff : (int32)0xc0003fff;
	}

	/** A key item p1 (object | child << 16) over door p2. */
	static bool keyFits(const Message &msg) {
		uint32 child = (uint32)msg.p1 >> 16;
		return (msg.p2 == 0x26c && child == 0x308) || (msg.p2 == 0x283 && child == 0x305);
	}

	void unlock(Message &msg, uint16 doorVar) {
		if (var(doorVar).get() != 0)
			return;
		uint16 item = msg.p1;
		cmd(msg.p1, kCmdStop, var(0x30, item).get());
		setCursor(0);
		var(doorVar).set(1);
		var(0x2d, item).set(0);
		var(0x31, item).set(1);
		cmd(kSelf, kCmdPlay, 0x19a);
		send(kSpeech, kMsgSay, 0x7040703);
	}

	/** A click on a door in state 1 opens it, in state 2 goes through. */
	void clickDoor(int32 state, uint16 doorVar, uint16 child, int32 cursor, int32 room) {
		if (state == 1) {
			var(doorVar).set(2);
			cmd(kSelf, kCmdSetCursor, child, cursor);
			cmd(kSelf, kCmdResume, child);
			cmd(kSelf, kCmdPlay, 0x199);
		} else if (state == 2) {
			send(kGameState, kMsgGotoRoom, room);
		}
	}

	void handleMessage(Message &msg) override {
		if (var(1, kGameState).get() == 0x24)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			switch (msg.p1) {
			case 0x1f:
				send(0xf, 0x2f, 0x276, 0x2740024, (int32)0x8000c001);
				break;
			case 0x26:
				send(0xf, 0x2f, 0x278, 0x2740024, (int32)0x8000c001);
				break;
			case 0x2b:
				send(0xf, 0x2f, 0x279, 0x2740024, (int32)0x8000c001);
				break;
			case 0x31:
				send(0xf, 0x2f, 0x277, 0x2740024, (int32)0x80003fff);
				break;
			case 0x35:
				send(0xf, 0x2f, 0x27a, 0x2740024, (int32)0x8000c001);
				break;
			default:
				send(kGameState, kMsgSwitchRoom);
				break;
			}
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x26f);
			cmd(kSelf, kCmdPlay, 0x26f);
			cmd(kSelf, kCmdSetZ, 0x26f, 100);
			cmd(kSelf, kCmdPlay, 0x272, 0xffff, doorMode(var(0x4e).get() == 2));
			cmd(kSelf, kCmdSetPos, 0x272, 0x51, 0xb9, 0x69);
			cmd(kSelf, kCmdPlay, 0x270, 0xffff, doorMode(var(0x4f).get() == 2));
			cmd(kSelf, kCmdSetPos, 0x270, 0xb8, 0xd4, 0x69);
			cmd(kSelf, kCmdPlay, 0x26e);
			cmd(kSelf, kCmdSetZ, 0x26e, 0x69);
			cmd(kSelf, kCmdPlay, 0x26c, 0xffff, doorMode(var(0x50).get() == 2));
			cmd(kSelf, kCmdSetPos, 0x26c, 0x1a9, 0xd6, 0x69);
			cmd(kSelf, kCmdPlay, 0x283, 0xffff, doorMode(var(0x51).get() == 2));
			cmd(kSelf, kCmdSetPos, 0x283, 0x20b, 0xbe, 0x69);
			cmd(kSelf, kCmdPlay, 0x269, 0xffff, doorMode(var(0x52).get() != 0), 0xffff);
			cmd(kSelf, kCmdSetPos, 0x269, 0xb4, 0xc2, 0x69);
			int32 cursor;
			if (var(0x52).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x26b);
				cmd(kSelf, kCmdSetZ, 0x26b, 0x6a);
				cmd(kSelf, kCmdSetFlags, 0x269, 1);
				cursor = (int32)0x80000006;
			} else {
				cmd(kSelf, kCmdSetFlags, 0x269, 0);
				cursor = (int32)0x80000003;
			}
			cmd(kSelf, kCmdSetCursor, 0x269, cursor);
			cmd(kSelf, kCmdPlay, 0x268, 0xffff);
			cmd(kSelf, kCmdSetPos, 0x268, 0x16f, 0xc2, 0x69);
			cmd(kSelf, kCmdSetCursor, 0x272, var(0x4e).get() == 2 ? 0x167 : (int32)0x80000006);
			cmd(kSelf, kCmdSetCursor, 0x270, var(0x4f).get() == 2 ? 0x167 : (int32)0x80000006);
			cmd(kSelf, kCmdSetCursor, 0x26c, var(0x50).get() == 2 ? 0x161 : (int32)0x80000006);
			cmd(kSelf, kCmdSetCursor, 0x283, var(0x51).get() == 2 ? 0x161 : (int32)0x80000006);
			break;
		}
		case kMsgEntered:
			if (var(0x43).get() == 1) {
				send(kGameState, 10);
			} else {
				var(0x43).set(1);
				send(0xf, kMsgSay, 0x78e078d, 0x10);
			}
			break;
		case 0x30: {
			// An exit animation ended (p2): hide the walker, go on.
			uint16 target = 0x10;
			uint16 child;
			switch (msg.p2) {
			case 0x269:
				cmd(0x3f, kCmdPlay, 0x2c5);
				return;
			case 0x276:
			case 0x277:
			case 0x278:
			case 0x279:
			case 0x27a:
				cmd(0xf, kCmdStop, 0x274);
				child = msg.p2 + 8;
				break;
			case 0x27e:
			case 0x27f:
			case 0x280:
			case 0x281:
				cmd(0x10, kCmdStop, 0x27c);
				child = msg.p2 - 8;
				target = 0xf;
				break;
			case 0x282:
				cmd(0x10, kCmdStop, 0x27c);
				if (cmd(0xf, kCmdIsRunning, 0x27a) == 0)
					send(kGameState, kMsgSwitchRoom);
				return;
			default:
				return;
			}
			if (cmd(target, kCmdIsRunning, child) == 0)
				send(kGameState, kMsgSwitchRoom);
			break;
		}
		case 0x3b:
			switch (msg.p1) {
			case 0x268:
			case 0x26b:
			case 0x26c:
			case 0x272:
			case 0x283:
				msg.result = 1;
				break;
			case 0x269:
				if (var(0x52).get() == 0)
					msg.result = 1;
				break;
			default:
				break;
			}
			break;
		case 0x3c:
			switch (msg.p1) {
			case 0x269: {
				int32 who = var(0x44).get();
				if (who == 0)
					send(kSpeech, kMsgSay, 0x79e079d);
				else if (who == 1)
					send(kSpeech, kMsgSay, 0x7a0079f);
				break;
			}
			case 0x268:
				send(kSpeech, kMsgSay, 0x79e079d);
				break;
			case 0x26b:
				send(kSpeech, kMsgSay, 0x7a207a1);
				break;
			case 0x272:
				send(kSpeech, kMsgSay, 0x7980797);
				break;
			case 0x26c:
			case 0x283: {
				bool left = msg.p1 == 0x26c;
				if (var(left ? 0x50 : 0x51).get() != 0) {
					send(kSpeech, kMsgSay, left ? 0x79c079b : 0x79a0799);
					break;
				}
				int32 state = var(left ? 0x46 : 0x45).get();
				if (state == 0)
					send(kSpeech, kMsgSay, 0x7140713);
				else if (state == 1)
					send(kSpeech, kMsgSay, 0x7160715);
				else if (state == 2)
					send(kSpeech, kMsgSay, 0x7120711);
				break;
			}
			default:
				break;
			}
			break;
		case kMsgClick:
			var(0x4f).get();
			switch (msg.p1) {
			case 0x269:
				if (var(0x52).get() == 0) {
					cmd(kSelf, kCmdSetFlags, 0x269);
					cmd(kSelf, kCmdSetCursor, 0x269);
					send(kSelf, 0x2f, 0x269);
					var(0x52).set(1);
					cmd(kSelf, kCmdStop, 0x26b);
				}
				break;
			case 0x26c: {
				int32 state = var(0x50).get();
				if (state == 0)
					cmd(kSelf, kCmdPlay, 0x198);
				else
					clickDoor(state, 0x50, 0x26c, 0x161, 0x1f);
				break;
			}
			case 0x26e:
				send(kGameState, kMsgGotoRoom, 0x31);
				break;
			case 0x270:
				clickDoor(var(0x4f).get(), 0x4f, 0x270, 0x167, 0x26);
				break;
			case 0x272:
				clickDoor(var(0x4e).get(), 0x4e, 0x272, 0x167, 0x2b);
				break;
			case 0x283: {
				int32 state = var(0x51).get();
				if (state == 0)
					cmd(kSelf, kCmdPlay, 0x198);
				else
					clickDoor(state, 0x51, 0x283, 0x161, 0x35);
				break;
			}
			default:
				break;
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (keyFits(msg))
				msg.result = 1;
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x305 && msg.p2 == 0x283)
				unlock(msg, 0x51);
			if ((uint32)msg.p1 >> 16 == 0x308 && msg.p2 == 0x26c)
				unlock(msg, 0x50);
			break;
		default:
			break;
		}
	}
};

// --- Object 55 ---

/**
 * Object 55 (vtable 0x4b4b44, FUN_0046e540): room 0x37, a board of 30
 * switches with two sprites each (k, k + 1). The door 500 opens when
 * variable 0x6f reaches 6.
 *
 * Variables: 0x43 visited, 0x6e map switch -> flipped, 0x6f.
 */
class Object55 : public HeldScript<ItemScript> {
public:
	Object55(EmuObject *obj) : HeldScript<ItemScript>(obj) {}

protected:
	struct Sprite {
		uint16 child;
		int16 x, y;
		bool fixed; ///< positioned even when the puzzle is solved
	};

	static const Sprite kSprites[60];

	/** The second sprites that count for the solution when shown. */
	static bool countsUp(uint16 child) {
		return child == 0x220 || child == 0x230 || child == 0x202 || child == 0x20e || child == 0x216 || child == 0x21e;
	}

	static bool isSwitch(int32 child) {
		for (uint i = 0; i < ARRAYSIZE(kSprites); i++)
			if (kSprites[i].child == child)
				return true;
		return false;
	}

	void checkSolved() {
		if (var(0x6f).get() == 6) {
			cmd(kSelf, kCmdResume, 500);
			cmd(kSelf, kCmdEnable, 499, 1);
			send(0xf, kMsgSay, 0x6e506e4);
		}
	}

	void handleMessage(Message &msg) override {
		if (var(1, kGameState).get() == 0x37)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0x231);
			cmd(kSelf, kCmdUnload, 499);
			cmd(kSelf, kCmdUnload, 0x237);
			cmd(kSelf, kCmdUnload, 500);
			for (uint i = 0; i < ARRAYSIZE(kSprites); i++)
				cmd(kSelf, kCmdUnload, kSprites[i].child);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x231);
			cmd(kSelf, kCmdPlay, 0x231);
			cmd(kSelf, kCmdPlay, 499);
			cmd(kSelf, kCmdSetZ, 499, 0x67);
			cmd(kSelf, kCmdPlay, 0x237);
			cmd(kSelf, kCmdSetZ, 0x237, 0x65);
			cmd(kSelf, kCmdSetPos, 500, 0xe4, 0x24, 0x65);
			cmd(kSelf, kCmdPlay, 500, 0, var(0x6f).get() != 6 ? (int32)0xc0003fff : (int32)0xffff3fff);
			cmd(kSelf, kCmdEnable, 499, var(0x6f).get() == 6);
			// Command 0 does nothing: the movable sprites keep their place once solved.
			int32 setPos = var(0x6f).get() != 6 ? kCmdSetPos : 0;
			for (uint i = 0; i < ARRAYSIZE(kSprites); i++) {
				const Sprite &s = kSprites[i];
				cmd(kSelf, s.fixed ? (int32)kCmdSetPos : setPos, s.child, s.x, s.y, 0x66);
			}
			int32 unsolved = var(0x6f).get() != 6; // command 1 (load) or nothing
			for (uint i = 0; i < ARRAYSIZE(kSprites); i += 2) {
				uint16 child = kSprites[i].child;
				cmd(kSelf, var(0x6e).at(child).get() != 0 ? unsolved : (int32)kCmdPlay, child);
				cmd(kSelf, var(0x6e).at(child).get() != 0 ? (int32)kCmdPlay : unsolved, child + 1);
			}
			break;
		}
		case kMsgEntered:
			if (var(0x43).get() == 1) {
				send(kGameState, 10);
			} else {
				var(0x43).set(1);
				send(0xf, kMsgSay, 0x6dd06dc, 0x10);
			}
			break;
		case 0x3b:
			msg.result = isSwitch(msg.p1) || msg.p1 == 500;
			break;
		case 0x3c:
			if (msg.p1 > 499 && msg.p1 < 0x231) {
				int32 who = var(0x44).get();
				if (who == 0)
					send(kSpeech, kMsgSay, 0x6e706e6);
				else if (who == 1)
					send(kSpeech, kMsgSay, 0x6e906e8);
			}
			break;
		case kMsgClick:
			if (msg.p1 == 499) {
				send(kGameState, kMsgGotoRoom, 0x17);
			} else if (msg.p1 == 0x237) {
				send(kGameState, kMsgGotoRoom, 0x34);
			} else if (isSwitch(msg.p1)) {
				uint16 child = msg.p1;
				// The first sprite of a pair has an odd id.
				bool first = (child & 1) != 0;
				uint16 other = first ? child + 1 : child - 1;
				cmd(kSelf, kCmdStop, child);
				cmd(kSelf, kCmdPlay, other);
				if (first) {
					var(0x6e).at(child).set(1);
					if (countsUp(other))
						postIncrement(var(0x6f));
					else
						postDecrement(var(0x6f));
				} else {
					var(0x6e).at(other).set(0);
					if (countsUp(child))
						postDecrement(var(0x6f));
					else
						postIncrement(var(0x6f));
				}
				checkSolved();
			}
			break;
		default:
			break;
		}
	}
};

const Object55::Sprite Object55::kSprites[60] = {
	{ 0x209, 0xf2, 0x54, true }, { 0x20a, 0xf2, 0x54, false },
	{ 0x21f, 0x100, 0x3e, false }, { 0x220, 0x100, 0x3e, true },
	{ 0x223, 0x116, 0x2f, true }, { 0x224, 0x116, 0x2f, false },
	{ 0x225, 0x135, 0x28, true }, { 0x226, 0x135, 0x28, false },
	{ 0x227, 0x153, 0x2f, true }, { 0x228, 0x153, 0x2f, false },
	{ 0x229, 0x168, 0x3e, true }, { 0x22a, 0x168, 0x3e, false },
	{ 0x22b, 0x176, 0x55, true }, { 0x22c, 0x176, 0x55, false },
	{ 0x22d, 0xed, 0x72, true }, { 0x22e, 0xed, 0x72, false },
	{ 0x22f, 0x10e, 0x74, false }, { 0x230, 0x10e, 0x74, true },
	{ 0x1f5, 0x132, 0x73, true }, { 0x1f6, 0x132, 0x73, false },
	{ 0x1f7, 0x155, 0x73, true }, { 0x1f8, 0x155, 0x73, false },
	{ 0x1f9, 0x178, 0x74, true }, { 0x1fa, 0x178, 0x74, false },
	{ 0x1fb, 0xeb, 0x94, true }, { 0x1fc, 0xeb, 0x94, false },
	{ 0x1fd, 0xeb, 0xb4, true }, { 0x1fe, 0xeb, 0xb4, false },
	{ 0x1ff, 0x178, 0x94, true }, { 0x200, 0x178, 0x94, false },
	{ 0x201, 0x178, 0xb4, false }, { 0x202, 0x178, 0xb4, true },
	{ 0x203, 0x133, 0xcf, true }, { 0x204, 0x133, 0xcf, false },
	{ 0x205, 0x155, 0xcf, true }, { 0x206, 0x155, 0xcf, false },
	{ 0x207, 0x178, 0xd0, true }, { 0x208, 0x178, 0xd0, false },
	{ 0x20b, 0xea, 0xeb, true }, { 0x20c, 0xea, 0xeb, false },
	{ 0x20d, 0xea, 0x10e, false }, { 0x20e, 0xea, 0x10e, true },
	{ 0x20f, 0xea, 0x130, true }, { 0x210, 0xea, 0x130, false },
	{ 0x211, 0xea, 0x14f, true }, { 0x212, 0xea, 0x14f, false },
	{ 0x213, 0x178, 0xef, true }, { 0x214, 0x178, 0xef, false },
	{ 0x215, 0x178, 0x10f, false }, { 0x216, 0x178, 0x10f, true },
	{ 0x217, 0x178, 0x131, true }, { 0x218, 0x178, 0x131, false },
	{ 0x219, 0x178, 0x150, true }, { 0x21a, 0x178, 0x150, false },
	{ 0x21b, 0x10b, 0x150, true }, { 0x21c, 0x10b, 0x150, false },
	{ 0x21d, 0x130, 0x150, false }, { 0x21e, 0x130, 0x150, true },
	{ 0x221, 0x156, 0x150, true }, { 0x222, 0x156, 0x150, false }
};

// --- Object 82 ---

/**
 * Object 82 (vtable 0x4b4804, FUN_0041a170): an item (sprite 0x23c) hidden
 * in many rooms; finding it (0x48) adds to the counter 0x36, shown in
 * the inventory (text 0xb0f) and spent with the buttons 0x23b / 0xb52.
 *
 * Variables: 0x35 map room -> found, 0x36 counter, 0x37 the hiding spot (child) in the room.
 */
class Object82 : public HeldScript<ItemScript> {
public:
	Object82(EmuObject *obj) : HeldScript<ItemScript>(obj) {}

protected:
	struct Spot {
		int32 room;
		uint16 child;
	};

	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgLeave:
			if (msg.p1 == 0x2e)
				cmd(kSelf, kCmdSetCursor, 0x23c, (int32)0x80000007);
			else
				cmd(kSelf, kCmdUnload, 0xfe03);
			cmd(kSelf, kCmdStop, 0x23c);
			var(0x37).set(0);
			break;
		case kMsgEnter:
			enterRoom(msg.p1);
			break;
		case 0x21:
			if (var(0x36).get() > 0 || msg.p1 == 0)
				send(10, 0x22, getId(), 0xb52, 0x36);
			msg.handled = 1;
			break;
		case 0x24:
			if (msg.p1 == 0) {
				cmd(kSelf, kCmdStop, 0xb0f);
			} else {
				cmd(kSelf, kCmdPlay, 0xb0f);
				cmd(kSelf, kCmdSetPos, 0xb0f, 0xdc, 0x110, 0x186b5);
				cmd(kSelf, kCmdSetView, 0xb0f, 200, 0x20);
				cmdVarText(0xb0f, var(0x36));
			}
			break;
		case 0x3b:
			if (msg.p1 == var(0x37).get()) {
				cmd(kSelf, kCmdEnable, var(0x37).get());
				cmd(kSelf, kCmdEnable, 0x23c, 1);
			}
			msg.result = 1;
			break;
		case 0x3c:
			if (var(1, kGameState).get() == 0x2e && var(0x7f, 0x2e).get() == 0x488 && msg.p1 == 0x23c)
				send(0xf, kMsgSay, 0x8f908f8);
			else
				send(kSpeech, kMsgSay, 0x71f0722);
			break;
		case 0x48: {
			switch (var(1, kGameState).get()) {
			case 0x16:
			case 0x17:
			case 0x2e:
				var(0x36).add(8);
				break;
			case 0x2f:
				var(0x36).add(2);
				send(0x2f, 0x48);
				break;
			default:
				var(0x36).add(3);
				break;
			}
			var(0x2d).set(1);
			VarRef current = var(1, kGameState);
			VarRef found = var(0x35);
			found.at(current.get()).set(1);
			cmd(kSelf, kCmdStop, 0x23c);
			break;
		}
		case kMsgClick:
			if (msg.p1 == 0x23b || msg.p1 == 0xb52) {
				if (var(0x36).get() < 1 || send(kGameState, 0x14, -1) == 0)
					break;
				postDecrement(var(0x36));
				if (msg.p1 == 0x23b)
					cmdVarText(0xb0f, var(0x36));
				if (var(0x36).get() == 0)
					send(10, 0x20, 1);
			}
			break;
		case kMsgCanDrag:
			if (var(kVarDragMode, kGameState).get() != 0 || var(1, kGameState).get() == 0x2e)
				msg.result = 1;
			else
				msg.result = 0;
			break;
		case kMsgDropped:
			if (msg.p1 == 0x23c)
				send(kSelf, 0x48, 0);
			break;
		default:
			break;
		}
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}

	void enterRoom(int32 room) {
		static const Spot kSpots[] = {
			{ 0x18, 0x41 }, { 0x19, 0x5f }, { 0x1a, 0x6f }, { 0x1b, 0xf1 }, { 0x1c, 0x107 },
			{ 0x1d, 299 }, { 0x1f, 0x18a }, { 0x20, 0x195 }, { 0x22, 0x24b }, { 0x23, 0x260 },
			{ 0x24, 0x27b }, { 0x25, 0x28d }, { 0x26, 0x3a0 }, { 0x27, 0x429 }, { 0x2b, 0x454 },
			{ 0x2c, 0x464 }, { 0x2d, 0x478 }, { 0x31, 0x1ec }, { 0x32, 0x4e9 }, { 0x33, 0x4f6 },
			{ 0x34, 0x534 }, { 0x35, 0x50a }, { 0x36, 0x51e }, { 0x37, 0x234 }, { 0x39, 0x578 },
			{ 0x3a, 0x541 }, { 0x3b, 0x592 }
		};
		int32 pos = -1;
		int32 z = 10;
		int32 enable = 0;
		switch (room) {
		case 0x16:
			if (var(0x35).at(0x16).get() == 0) {
				pos = 0x7e013f;
				z = 0x87;
				var(0x37).set(0);
			}
			break;
		case 0x17:
			if (var(0x35).at(0x17).get() == 0) {
				pos = 0xf501ac;
				var(0x37).set(0);
			}
			break;
		case 0x2e:
			if (var(0x35).at(0x2e).get() == 0) {
				pos = 0x12c023a;
				z = 0x14;
				enable = 1;
				cmd(kSelf, kCmdSetCursor, 0x23c, (int32)0x80000003);
				var(0x37).set(0);
			}
			break;
		default:
			for (uint i = 0; i < ARRAYSIZE(kSpots); i++) {
				if (kSpots[i].room != room)
					continue;
				uint16 child = kSpots[i].child;
				if (var(0x35).at(room).get() == 0) {
					cmd(kSelf, kCmdPlay, child);
					cmd(kSelf, kCmdSetZ, child, 0x6f);
					cmd(kSelf, kCmdEnable, child, 1);
					pos = cmd(kSelf, kCmdGetPos, child);
					var(0x37).set(child);
				}
				break;
			}
			break;
		}
		cmd(kSelf, kCmdPlay, 0x23c);
		cmd(kSelf, kCmdSetPos, 0x23c, (int16)pos, (int16)((uint32)pos >> 16), z + 100);
		cmd(kSelf, kCmdEnable, 0x23c, enable);
	}
};

IVANOFF_SCRIPT(Object9) { return new Object9(obj); }
IVANOFF_SCRIPT(Object10) { return new Object10(obj); }
IVANOFF_SCRIPT(Object11) { return new Object11(obj); }
IVANOFF_SCRIPT(Object12) { return new Object12(obj); }
IVANOFF_SCRIPT(Object13) { return new Object13(obj); }
IVANOFF_SCRIPT(Object30) { return new Object30(obj); }
IVANOFF_SCRIPT(Object36) { return new Object36(obj); }
IVANOFF_SCRIPT(Object55) { return new Object55(obj); }
IVANOFF_SCRIPT(Object82) { return new Object82(obj); }

} // End of namespace Ivanoff
