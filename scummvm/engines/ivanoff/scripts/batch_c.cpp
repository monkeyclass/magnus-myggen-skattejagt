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

// Batch c: the intro and title screens (objects 2-4), the map of the
// countries (33), the hidden-letters game (46), the quiz (47) and the
// recipe book (48).

#include "common/debug.h"

#include "ivanoff/machine.h"
#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

/**
 * Helpers of this batch's classes: engine functions that script.h does not
 * wrap, and the web link shared by objects 33, 47 and 48.
 *
 * Like the interpreter, which keeps a copy of the message on its heap while
 * an original handler runs, the scripts hold a 0x2c block for the duration
 * of accepts() and handle(): messages allocated by the handler itself
 * (newCommand) then get the same addresses as with the original code.
 */
class BatchCScript : public ObjectScript {
public:
	BatchCScript(EmuObject *obj) : ObjectScript(obj) {}

	bool accepts(Message &msg) override {
		HeapBlock block(machine());
		return acceptsMessage(msg);
	}

	void handle(Message &msg) override {
		HeapBlock block(machine());
		handleMessage(msg);
	}

protected:
	virtual bool acceptsMessage(Message &msg) { return true; }
	virtual void handleMessage(Message &msg) = 0;

	/** FUN_00423a60 (ItemScript::accepts): no clicks while an item is dragged. */
	bool acceptsUnlessDragging(Message &msg) {
		if (var(kVarDragMode, kGameState).get() == 1 && msg.msg == kMsgClick)
			return false;
		return true;
	}

	/** FUN_00477190: sets the globals 0x4c6970-0x4c6978 [unverified: sound format]. */
	void setSoundFormat(int32 rate, int32 channels, int32 x) {
		setGlobal32(0x4c6970, rate);
		setGlobal32(0x4c6974, channels);
		setGlobal32(0x4c6978, x);
	}

	/** Variable slot +0xc: the values of two variables are equal. */
	static bool sameValue(VarRef a, VarRef b) {
		return a.eq(b);
	}

	/** Variable slot +0x3c: the value as a string (MFC CString by value). */
	Common::String stringOf(VarRef v) {
		return v.toString();
	}

	/**
	 * Child 0x59e of objects 33, 47 and 48: the button that opens the web
	 * site named by variable urlVar of object 8. The original greys the
	 * button for 2 s; it passes the result of the command that restores the
	 * flags (instead of a new message) to the timer, so the button is
	 * restored at once and the timer only starts if that result is not 0.
	 */
	void clickWebLink(uint16 urlVar) {
		int32 flags = cmd(kSelf, kCmdGetFlags, 0x59e);
		cmd(kSelf, kCmdSetFlags, 0x59e, 0);
		openLink(stringOf(var(urlVar, 8)));
		// The original hands this result (always 0) to FUN_00477140 as the
		// timer's message, so no timer starts.
		cmd(getId(), kCmdSetFlags, 0x59e, flags);
	}

private:
	/**
	 * FUN_0044e4a0 + FUN_0044e590 (ShellExecute "open", or the browser
	 * registered for http). The engine refuses to start programs, like the
	 * interpreter does for the original (same log line as Machine, so the
	 * traces compare equal); the registry fallback finds nothing either.
	 */
	void openLink(const Common::String &url) {
		(void)url;
		debug(1, "Machine: SHELL32.DLL!ShellExecuteA refused");
	}

	class HeapBlock {
	public:
		HeapBlock(Machine &m) : _m(m), _addr(0) {}
	private:
		Machine &_m;
		uint32 _addr;
	};
};

// --- Object 2 ---

/**
 * Object 2 (vtable 0x4b5184, FUN_00422500): the credits, a chain of
 * animations and sounds. Escape (key 0x1b), a click or the last animation
 * ends it.
 */
class Object2 : public BatchCScript {
public:
	Object2(EmuObject *obj) : BatchCScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgStart:
			start();
			break;
		case kMsgClick:
			finish();
			break;
		case kMsgAnimDone:
			animDone(msg.p1);
			break;
		case kMsgSoundDone:
			soundDone(msg.p1);
			break;
		case kMsgKeyDown:
			if (msg.p1 == 0x1b)
				finish();
			break;
		case 0x8002:
			sound8002(msg.p1);
			break;
		default:
			break;
		}
	}

private:
	static const int32 kLoop = (int32)0x80020002;

	void start() {
		setSoundFormat(0x5622, 2, 0x15e);
		cmd(kSelf, kCmdLoad, 0x295);
		cmd(kSelf, kCmdLoad, 0x294);
		cmd(kSelf, kCmdSetPos, 0, 0x28, 0x1e, 10);
		cmd(kSelf, kCmdSetCursor, 0, (int32)0x80000005);
		cmd(kSelf, kCmdUsePalette, 0x293);
		cmd(kSelf, kCmdPlay, 0x293, 2);
		cmd(kSelf, kCmdSetCursor, 0x23f, (int32)0x80000004);
		cmd(kSelf, kCmdSetZ, 0x23f);
		cmd(kSelf, kCmdPlay, 0x23f);
		setCursor(0);
		setKeyFocus(2);
		setEventMask(0x10000);
	}

	void finish() {
		setEventMask(0);
		setKeyFocus(0);
		cmd(kSelf, kCmdUnload, 0);
		unloadChildren();
		setSoundFormat(0x5622, 2, 0xdc);
		post(kGameState, kMsgNewGame);
	}

	/** Unloads a and b, plays c then d with p1 = 2. */
	void next(uint16 a, uint16 b, uint16 c, uint16 d) {
		cmd(kSelf, kCmdUnload, a);
		cmd(kSelf, kCmdUnload, b);
		cmd(kSelf, kCmdPlay, c);
		cmd(kSelf, kCmdPlay, d, 2);
	}

	/** Unloads a and b, plays c with p1 = 2. */
	void next(uint16 a, uint16 b, uint16 c) {
		cmd(kSelf, kCmdUnload, a);
		cmd(kSelf, kCmdUnload, b);
		cmd(kSelf, kCmdPlay, c, 2);
	}

	void load(const uint16 *ids, uint n) {
		for (uint i = 0; i < n; i++)
			cmd(kSelf, kCmdLoad, ids[i]);
	}

	void animDone(int32 anim) {
		switch (anim) {
		case 0x293: {
			static const uint16 ids[] = { 0x7c8, 0x7c9, 0x7ca, 0x29c, 0x29b, 0x297, 0x298, 0x299, 0x29a };
			cmd(kSelf, kCmdUnload, 0x294);
			cmd(kSelf, kCmdUnload, 0x293, 2);
			redrawNow();
			load(ids, ARRAYSIZE(ids));
			cmd(kSelf, kCmdUsePalette, 0x296);
			cmd(kSelf, kCmdPlay, 0x296, 0xffff);
			break;
		}
		case 0x296:
			cmd(kSelf, kCmdUnload, 0x296);
			cmd(kSelf, kCmdPlay, 0x297);
			cmd(kSelf, kCmdPlay, 0x7c9, 2);
			break;
		case 0x298:
			cmd(kSelf, kCmdUnload, 0x298);
			cmd(kSelf, kCmdPlay, 0x299);
			cmd(kSelf, kCmdPlay, 0x7ca, 2);
			break;
		case 0x29a: {
			static const uint16 ids[] = { 0x29e, 0x7cb, 0x7cc, 0x7cd, 0x7ce, 0x2a4, 0x29f, 0x2a0, 0x2a1, 0x2a2, 0x2a3, 0x2a5 };
			cmd(kSelf, kCmdUnload, 0x29a);
			cmd(kSelf, kCmdUnload, 0x29c);
			cmd(kSelf, kCmdUnload, 0x29b);
			load(ids, ARRAYSIZE(ids));
			cmd(kSelf, kCmdUsePalette, 0x29d);
			cmd(kSelf, kCmdPlay, 0x29d, 2);
			break;
		}
		case 0x29d:
			cmd(kSelf, kCmdUnload, 0x29d);
			cmd(kSelf, kCmdUnload, 0x29e);
			cmd(kSelf, kCmdPlay, 0x29f);
			cmd(kSelf, kCmdPlay, 0x7cb, 2);
			break;
		case 0x2a2:
			cmd(kSelf, kCmdUnload, 0x2a2);
			cmd(kSelf, kCmdPlay, 0x2a3, kLoop);
			cmd(kSelf, kCmdPlay, 0x7ce, kLoop);
			break;
		case 0x2a3:
			cmd(kSelf, kCmdUnload, 0x2a3);
			if (cmd(kSelf, kCmdIsPlaying, 0x7ce))
				cmd(kSelf, kCmdPlay, 0x2a5);
			break;
		case 0x2a6:
			cmd(kSelf, kCmdUnload, 0x2a6);
			cmd(kSelf, kCmdPlay, 0x2a7);
			cmd(kSelf, kCmdPlay, 0x7cf, 2);
			break;
		case 0x2a8:
			cmd(kSelf, kCmdUnload, 0x2a8);
			cmd(kSelf, kCmdPlay, 0x2a9);
			cmd(kSelf, kCmdPlay, 0x7d0, 2);
			break;
		case 0x2ab:
			cmd(kSelf, kCmdUnload, 0x2ab);
			cmd(kSelf, kCmdLoad, 0x2ae);
			cmd(kSelf, kCmdUsePalette, 0x2ad);
			cmd(kSelf, kCmdPlay, 0x2ad, 2);
			break;
		case 0x2ad: {
			static const uint16 ids[] = { 0x7d2, 0x7d3, 0x2b0, 0x2b1, 0x2b2, 0x2b3 };
			cmd(kSelf, kCmdUnload, 0x2ad);
			cmd(kSelf, kCmdUnload, 0x2ae);
			load(ids, ARRAYSIZE(ids));
			cmd(kSelf, kCmdUsePalette, 0x2af);
			cmd(kSelf, kCmdPlay, 0x2af, 2);
			break;
		}
		case 0x2af:
			cmd(kSelf, kCmdUnload, 0x2af);
			cmd(kSelf, kCmdPlay, 0x2b0);
			cmd(kSelf, kCmdPlay, 0x7d2, 2);
			break;
		case 0x2b1:
			cmd(kSelf, kCmdUnload, 0x2b1);
			cmd(kSelf, kCmdPlay, 0x2b2);
			cmd(kSelf, kCmdPlay, 0x7d3, 2);
			break;
		case 0x2b3: {
			static const uint16 ids[] = { 0x2b6, 0x7d4, 0x7d5, 0x7d6, 0x7d7, 0x2b7, 0x2b8, 0x2b9, 0x2ba, 0x2bb, 0x2bc, 0x2bd, 0x2be, 0x2b5 };
			cmd(kSelf, kCmdUnload, 0x2b3);
			load(ids, ARRAYSIZE(ids));
			cmd(kSelf, kCmdUsePalette, 0x2b4);
			cmd(kSelf, kCmdPlay, 0x2b4, 2);
			break;
		}
		case 0x2b4:
			cmd(kSelf, kCmdUnload, 0x2b4);
			cmd(kSelf, kCmdUnload, 0x2b6);
			cmd(kSelf, kCmdPlay, 0x2b7, kLoop);
			cmd(kSelf, kCmdPlay, 0x7d4, kLoop);
			break;
		case 0x2b5:
			cmd(kSelf, kCmdUnload, 0x2b5);
			cmd(kSelf, kCmdLoad, 0x2c0);
			cmd(kSelf, kCmdLoad, 0x2c1);
			cmd(kSelf, kCmdUsePalette, 0x2bf);
			cmd(kSelf, kCmdPlay, 0x2bf);
			break;
		case 0x2b7:
			cmd(kSelf, kCmdUnload, 0x2b7);
			if (cmd(kSelf, kCmdIsPlaying, 0x7d4))
				cmd(kSelf, kCmdPlay, 0x2b8);
			break;
		case 0x2ba:
			cmd(kSelf, kCmdUnload, 0x2ba);
			cmd(kSelf, kCmdPlay, 0x2bb, kLoop);
			cmd(kSelf, kCmdPlay, 0x7d6, kLoop);
			break;
		case 0x2bb:
			cmd(kSelf, kCmdUnload, 0x2bb);
			if (cmd(kSelf, kCmdIsPlaying, 0x7d6))
				cmd(kSelf, kCmdPlay, 0x2bc);
			break;
		case 0x2bd:
			cmd(kSelf, kCmdUnload, 0x2bd);
			if (cmd(kSelf, kCmdIsPlaying, 0x7d7))
				cmd(kSelf, kCmdPlay, 0x2be);
			break;
		default:
			break;
		}
	}

	void soundDone(int32 sound) {
		switch (sound) {
		case 0x295:
			cmd(kSelf, kCmdPlay, 0x295, 2);
			break;
		case 0x2c1:
			finish();
			break;
		case 0x7c9:
			next(0x297, 0x7c9, 0x298);
			break;
		case 0x7ca:
			next(0x299, 0x7ca, 0x29a);
			break;
		case 0x7cb:
			next(0x29f, 0x7cb, 0x2a0, 0x7cc);
			break;
		case 0x7cc:
			next(0x2a0, 0x7cc, 0x2a1, 0x7cd);
			break;
		case 0x7cd:
			next(0x2a1, 0x7cd, 0x2a2);
			break;
		case 0x7cf:
			next(0x2a7, 0x7cf, 0x2a8);
			break;
		case 0x7d0:
			next(0x2a9, 0x7d0, 0x2aa, 0x7d1);
			break;
		case 0x7d1:
			next(0x2aa, 0x7d1, 0x2ab);
			break;
		case 0x7d2:
			next(0x2b0, 0x7d2, 0x2b1);
			break;
		case 0x7d3:
			next(0x2b2, 0x7d3, 0x2b3);
			break;
		case 0x7d5:
			next(0x2b9, 0x7d5, 0x2ba);
			break;
		default:
			break;
		}
	}

	/** Message 0x8002 [unverified: a looped sound or animation ended], p1 = the child. */
	void sound8002(int32 child) {
		switch (child) {
		case 0x2a3: {
			static const uint16 ids[] = { 0x7cf, 0x7d0, 0x7d1, 0x2ac, 0x2a7, 0x2a8, 0x2a9, 0x2aa, 0x2ab };
			cmd(kSelf, kCmdUnload, 0x7ce);
			load(ids, ARRAYSIZE(ids));
			cmd(kSelf, kCmdUsePalette, 0x2a6);
			cmd(kSelf, kCmdPlay, 0x2a6, 2);
			break;
		}
		case 0x7ce:
			cmd(kSelf, kCmdUnload, 0x2a5);
			cmd(kSelf, kCmdUnload, 0x7ce);
			cmd(kSelf, kCmdUsePalette, 0x2a6);
			cmd(kSelf, kCmdPlay, 0x2a6, 2);
			break;
		case 0x7d4:
			cmd(kSelf, kCmdUnload, 0x2b8);
			// fall through
		case 0x2b7:
			cmd(kSelf, kCmdUnload, 0x7d4);
			cmd(kSelf, kCmdPlay, 0x2b9);
			cmd(kSelf, kCmdPlay, 0x7d5, 2);
			break;
		case 0x7d6:
			cmd(kSelf, kCmdUnload, 0x7d6);
			cmd(kSelf, kCmdUnload, 0x2bc);
			cmd(kSelf, kCmdPlay, 0x2bd, kLoop);
			cmd(kSelf, kCmdPlay, 0x7d7, kLoop);
			break;
		case 0x2bb:
			cmd(kSelf, kCmdUnload, 0x7d6);
			cmd(kSelf, kCmdPlay, 0x2bd, kLoop);
			cmd(kSelf, kCmdPlay, 0x7d7, kLoop);
			break;
		case 0x7d7:
			cmd(kSelf, kCmdUnload, 0x2be);
			// fall through
		case 0x2bd:
			cmd(kSelf, kCmdUnload, 0x7d7);
			cmd(kSelf, kCmdPlay, 0x2b5, 2);
			break;
		default:
			break;
		}
	}
};

// --- Object 3 ---

/** Object 3 (vtable 0x4b5164, FUN_00418f80): the opening animation. */
class Object3 : public BatchCScript {
public:
	Object3(EmuObject *obj) : BatchCScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case 0x1b:
			setKeyFocus(3);
			setSoundFormat(0xac44, 2, 0xdc);
			cmd(kSelf, kCmdSetPos, 0, 0x14, 0x1e, 10);
			cmd(kSelf, kCmdSetCursor, 0, (int32)0x80000002);
			cmd(kSelf, kCmdUsePalette, 0x1d3);
			cmd(kSelf, kCmdSetZ, 0x23f);
			cmd(kSelf, kCmdPlay, 0x23f);
			cmd(kSelf, kCmdPlay, 0x1d3);
			cmd(kSelf, kCmdPlay, 0x6bf, 3);
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x1d5) {
				cmd(kSelf, kCmdUnload, 0x1d5);
				cmd(kSelf, kCmdPlay, 0x1d6);
				cmd(kSelf, kCmdPlay, 0x6c1, 3);
			} else if (msg.p1 == 0x1d7) {
				cmd(kSelf, kCmdUnload, 0x1d7);
				cmd(kSelf, kCmdPlay, 0x1d8);
				cmd(kSelf, kCmdPlay, 0x6c2, 3);
			} else if (msg.p1 == 0x1d9) {
				cmd(kSelf, kCmdUnload, 0);
				send(kGameState, 0x19);
			}
			break;
		case kMsgSoundDone:
			switch (msg.p1) {
			case 0x6bf:
				cmd(kSelf, kCmdUnload, 0x1d3);
				cmd(kSelf, kCmdUnload, 0x6bf);
				cmd(kSelf, kCmdPlay, 0x1d4);
				cmd(kSelf, kCmdPlay, 0x6c0, 3);
				break;
			case 0x6c0:
				cmd(kSelf, kCmdUnload, 0x1d4);
				cmd(kSelf, kCmdUnload, 0x6c0);
				cmd(kSelf, kCmdPlay, 0x1d5, 3);
				break;
			case 0x6c1:
				cmd(kSelf, kCmdUnload, 0x1d6);
				cmd(kSelf, kCmdUnload, 0x6c1);
				cmd(kSelf, kCmdPlay, 0x1d7, 3);
				break;
			case 0x6c2:
				cmd(kSelf, kCmdUnload, 0x1d8);
				cmd(kSelf, kCmdUnload, 0x6c2);
				cmd(kSelf, kCmdPlay, 0x1d9, 3);
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

// --- Object 4 ---

/**
 * Object 4 (vtable 0x4b5144, FUN_00404570): the background music. Variable
 * 0xb7 maps rooms to their music; 0xb4 is the next music, 0xb5 the current
 * one (0xffff: none yet), 0xb6 the one to unload; 0xb8 music on.
 */
class Object4 : public BatchCScript {
public:
	Object4(EmuObject *obj) : BatchCScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgEnter:
			enterRoom(msg.p1);
			break;
		case 0xd:
			initMusic();
			break;
		case kMsgSoundDone:
			musicDone();
			break;
		default:
			break;
		}
	}

private:
	void enterRoom(int32 room) {
		if (var(0xb8).get() == 0)
			return;
		if (var(0xb5).get() == 0xffff) {
			// The map variable takes 16-bit keys.
			VarRef music = var(0xb7).at((uint16)room);
			VarRef current = var(0xb5);
			current.set(music.get());
			if (var(0xb5).get() != 0) {
				cmd(kSelf, kCmdPlay, var(0xb5).get(), 0xffff);
				return;
			}
			var(0xb5).set(0xffff);
		} else {
			VarRef music = var(0xb7).at((uint16)room);
			VarRef nextMusic = var(0xb4);
			nextMusic.set(music.get());
			if (var(0xb4).get() != 0)
				return;
			var(0xb4).set(0xffff);
		}
	}

	void initMusic() {
		static const struct {
			uint16 room;
			uint16 music;
		} kMusic[] = {
			{ 0x17, 0x3c }, { 0x1b, 0xf7 }, { 0x1f, 0x501 }, { 0x20, 0x501 },
			{ 0x24, 0x501 }, { 0x28, 0x42f }, { 0x29, 0x42f }, { 0x2a, 0x42f },
			{ 0x2b, 0x46b }, { 0x2c, 0x46b }, { 0x2e, 0x19c }, { 0x31, 0x3c },
			{ 0x32, 0x500 }, { 0x33, 0x500 }, { 0x34, 0x19b }, { 0x35, 0x57f },
			{ 0x36, 0x57f }, { 0x38, 0x19b }, { 0x39, 0x57f }, { 0x3a, 0x57f },
			{ 0x3e, 0x256 }
		};
		for (uint i = 0; i < ARRAYSIZE(kMusic); i++)
			var(0xb7).at(kMusic[i].room).set(kMusic[i].music);
	}

	void musicDone() {
		if (var(0xb8).get() == 0) {
			var(0xb5).set(0xffff);
			return;
		}
		if (sameValue(var(0xb4), var(0xb5)))
			var(0xb4).set(0);
		if (var(0xb4).get() == 0 && var(0xb5).get() != 0) {
			// The current music again.
			if (var(0xb5).get() != 0xffff)
				cmd(kSelf, kCmdPlay, var(0xb5).get(), 0xffff);
		} else {
			if (var(0xb4).get() != 0xffff)
				cmd(kSelf, kCmdPlay, var(0xb4).get(), 0xffff);
			VarRef current = var(0xb5);
			VarRef old = var(0xb6);
			old.set(current.get());
			VarRef nextMusic = var(0xb4);
			VarRef current2 = var(0xb5);
			current2.set(nextMusic.get());
			var(0xb4).set(0);
		}
		if (var(0xb6).get() != 0) {
			cmd(kSelf, kCmdUnload, var(0xb6).get());
			var(0xb6).set(0);
		}
	}
};

// --- Object 33 ---

/**
 * Object 33 (vtable 0x4b4e04, FUN_00417c40): the map of the countries in
 * room 0x1a. Variables: 0x40 open, 0x41 the country shown, 0x42 its name
 * sprite while the mouse is over the map, 0x44 the speech toggle.
 */
class Object33 : public BatchCScript {
public:
	Object33(EmuObject *obj) : BatchCScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		if (var(1, kGameState).get() != 0x1a)
			return;
		switch (msg.msg) {
		case kMsgLeave:
			stopTimer(5);
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgEnter:
			enter();
			break;
		case 0x4c:
			open();
			break;
		case 0x4d:
			close();
			break;
		case kMsgClick:
			click(msg);
			break;
		case kMsgMouseMove:
			mouseMove(msg);
			break;
		case kMsgMouseEnter:
			if (msg.p1 != 0x1b5)
				break;
			if (var(0x42).get() != 0)
				cmd(kSelf, kCmdStop, var(0x42).get());
			var(0x41).set(0);
			var(0x42).set(0);
			break;
		case kMsgAnimDone:
			if (msg.p1 != 0x1b9)
				break;
			cmd(kSelf, kCmdStop, 0x1b9);
			cmd(kSelf, kCmdPlay, 0x1b5);
			cmd(kSelf, kCmdPlay, 0x1b7);
			cmd(kSelf, kCmdPlay, 0x1b8);
			cmd(kSelf, kCmdPlay, 0x1b6);
			if (var(8, 8).get() != 0)
				break;
			cmd(kSelf, kCmdSetPos, 0x59e, 0x2f, 0x169, 0x4e2a);
			cmd(kSelf, kCmdPlay, 0x59e);
			cmd(kSelf, kCmdEnable, 0x59e, 1);
			break;
		default:
			break;
		}
	}

private:
	struct Country {
		int32 id;
		uint16 flag;     ///< the child showing the country
		uint16 text1;    ///< text of child 0xb11
		uint16 text2;    ///< text of child 0xb12
		uint16 sound;    ///< played with the flag
		const char *name; ///< string of child 0xb13 (nullptr: none)
	};

	static const Country *findCountry(int32 id) {
		static const Country kCountries[] = {
			{ 0x01, 0x1c4, 0x7af, 0x75c, 0x75b, "H" },
			{ 0x02, 0x1c5, 0x7a7, 0x768, 0x767, "He" },
			{ 0x06, 0x1bf, 0x5e4, 0x76a, 0x769, "C" },
			{ 0x07, 0x1c8, 0x8f1, 0x76c, 0x76b, "N" },
			{ 0x08, 0x1cc, 0x918, 0x76e, 0x76d, "O" },
			{ 0x0a, 0x1ca, 0x8ef, 0x770, 0x76f, "Ne" },
			{ 0x0b, 0x1c9, 0x8ee, 0x772, 0x771, "Na" },
			{ 0x0c, 0x1c7, 0x8dd, 0x774, 0x773, "Mg" },
			{ 0x0d, 0x1bc, 0x59f, 0x776, 0x775, "Al" },
			{ 0x0f, 0x1cd, 0x6ea, 0x748, 0x747, "P" },
			{ 0x11, 0x1c1, 0x66f, 0x74a, 0x749, "Cl" },
			{ 0x14, 0x1c0, 0x5e3, 0x74c, 0x74b, "Ca" },
			{ 0x1a, 0x1c3, 0x6c3, 0x74e, 0x74d, "Fe" },
			{ 0x1c, 0x1cb, 0x8f0, 0x750, 0x74f, "Ni" },
			{ 0x1d, 0x1c2, 0x693, 0x752, 0x751, "Cu" },
			{ 0x1e, 0x1d2, 0xab1, 0x754, 0x753, "Zn" },
			{ 0x21, 0x1bd, 0x5a3, 0x756, 0x755, "As" },
			{ 0x2f, 0x1bb, 0x5a2, 0x758, 0x757, "Ag" },
			{ 0x32, 0x1d0, 0xa2a, 0x75a, 0x759, "Sn" },
			{ 0x4e, 0x1cf, 0x91d, 0x75e, 0x75d, "Pt" },
			{ 0x4f, 0x1be, 0x5ce, 0x760, 0x75f, "Au" },
			{ 0x50, 0x1c6, 0x7ae, 0x762, 0x761, "Hg" },
			{ 0x52, 0x1ce, 0x91e, 0x764, 0x763, "Pb" },
			{ 0x5c, 0x1d1, 0xaa9, 0x766, 0x765, "U" }
		};
		static const Country kNone = { 0, 0, 0, 0, 0, 0 };
		for (uint i = 0; i < ARRAYSIZE(kCountries); i++)
			if (kCountries[i].id == id)
				return &kCountries[i];
		return &kNone;
	}

	void enter() {
		cmd(kSelf, kCmdLoad, 0);
		cmd(kSelf, kCmdSetZ, 0x1b5, 20000);
		cmd(kSelf, kCmdSetZ, 0x1b7, 0x4e2a);
		cmd(kSelf, kCmdSetZ, 0x1b6, 0x4e2b);
		cmd(kSelf, kCmdSetZ, 0x1b9, 20000);
		cmd(kSelf, kCmdSetZ, 0x1b8, 0x4e2a);
		cmd(kSelf, kCmdSetZ, 0x23f, 0x4fb0);
		cmd(kSelf, kCmdSetPos, 0x1ba, 0xa8, 0x28, 0x5014);
		cmd(kSelf, kCmdSetPos, 0xb12, 0xb2, 0xf3, 0x501e);
		cmd(kSelf, kCmdSetView, 0xb12, 0x11b, 0x35);
		cmd(kSelf, kCmdSetPos, 0xb11, 0xb2, 0x127, 0x501e);
		cmd(kSelf, kCmdSetView, 0xb11, 0x11b, 0x8c);
		cmd(kSelf, kCmdSetPos, 0xb13, 0xb2, 0x2c, 0x501e);
		cmd(kSelf, kCmdSetView, 0xb13, 0x11b, 0x13);
	}

	void open() {
		if (var(0x40).get() != 0)
			return;
		var(0x40).set(1);
		var(0x10, 0xf).set(0);
		var(0x10, 0x10).set(0);
		var(0x11, 0xf).set(0);
		var(0x11, 0x10).set(0);
		cmd(kSelf, kCmdUsePalette, 0x1b5);
		cmd(kSelf, kCmdPlay, 0x1b9, 0x21);
	}

	void close() {
		if (var(0x40).get() == 0)
			return;
		stopTimer(5);
		cmd(kSelf, kCmdStop, 0);
		cmd(0x1a, kCmdUsePalette, 0x6a);
		var(0x10, 0xf).set(0x6d);
		var(0x10, 0x10).set(0x70);
		var(0x11, 0xf).set(0x6e);
		var(0x11, 0x10).set(0x71);
		var(0x40).set(0);
	}

	void click(Message &msg) {
		switch (msg.p1) {
		case 0x1b5:
			send(kSelf, 0x4d);
			break;
		case 0x1b6:
			showCountry(msg.p3);
			break;
		case 0x1b8: {
			// The speech toggle.
			int32 mode = var(0x44).get();
			if (mode == 0) {
				if (cmd(kSpeech, kCmdIsPlaying, 0x607))
					cmd(kSpeech, kCmdStop, 0x607);
				cmd(kSpeech, kCmdPlay, 0x623);
			} else if (mode == 1) {
				if (cmd(kSpeech, kCmdIsPlaying, 0x623))
					cmd(kSpeech, kCmdStop, 0x623);
				cmd(kSpeech, kCmdPlay, 0x607);
			}
			break;
		}
		case 0x23f:
			cmd(kSelf, kCmdStop, 0x23f);
			cmd(kSelf, kCmdStop, 0x1ba);
			cmd(kSelf, kCmdStop, 0xb11);
			cmd(kSelf, kCmdStop, 0xb12);
			cmd(kSelf, kCmdStop, 0xb13);
			cmd(kSelf, kCmdStop, findCountry(var(0x41).get())->flag);
			break;
		case 0x59e:
			clickWebLink(10);
			break;
		default:
			break;
		}
	}

	void showCountry(int32 id) {
		var(0x41).set(id);
		const Country *c = findCountry(id);
		Common::String name;
		if (c->name)
			name = c->name;
		cmd(kSelf, kCmdPlay, 0x23f);
		cmd(kSelf, kCmdPlay, 0x1ba);
		cmd(kSelf, kCmdPlay, 0xb11);
		cmd(kSelf, kCmdPlay, 0xb12);
		cmd(kSelf, kCmdPlay, 0xb13);
		cmd(kSelf, 0xe, 0xb11, 0x21, c->text1);
		cmd(kSelf, 0xe, 0xb12, 0x21, c->text2);
		cmdString(kSelf, kCmdSetText, 0xb13, name);
		cmd(kSelf, kCmdPlay, c->flag);
		cmd(kSelf, kCmdSetPos, c->flag, 0xb3, 0x4e, 0x501e);
		if (c->sound)
			cmd(kSelf, kCmdPlay, c->sound);
	}

	void mouseMove(Message &msg) {
		if (msg.p1 < 0x1b6 || msg.p1 > 0x1b7)
			return;
		int32 id = msg.p3;
		if (var(0x41).get() == id)
			return;
		if (var(0x42).get() != 0)
			cmd(kSelf, kCmdStop, var(0x42).get());
		var(0x41).set(id);
		// The names of the countries: child ids by country (DAT_004c6264).
		static const uint16 kLabels[110] = {
			0x00, 0x96, 0x97, 0xa1, 0x7c, 0x7a, 0x80, 0xa8, 0xb0, 0x8f, 0xac, 0xa9,
			0xa5, 0x74, 0xc5, 0xb2, 0xc1, 0x85, 0x76, 0x9e, 0x81, 0xc3, 0xce, 0xdb,
			0x88, 0xa6, 0x90, 0x87, 0xad, 0x8a, 0xe0, 0x93, 0x95, 0x77, 0xc4, 0x7f,
			0x9f, 0xbc, 0xc8, 0xde, 0xe1, 0xaa, 0xa7, 0xcb, 0xc0, 0xbe, 0xb5, 0x73,
			0x82, 0x9c, 0xc7, 0xc2, 0xcc, 0x9b, 0xdd, 0x89, 0x7b, 0xa0, 0x83, 0xb8,
			0xab, 0xb6, 0xc6, 0x8e, 0x94, 0xca, 0x8b, 0x9a, 0x8c, 0xd0, 0xdf, 0xa3,
			0x98, 0xc9, 0xdc, 0xbd, 0xb1, 0x9d, 0xb9, 0x79, 0x99, 0xcf, 0xb4, 0x7d,
			0xb7, 0x78, 0xbf, 0x92, 0xbb, 0x72, 0xcd, 0xb3, 0xd1, 0xaf, 0xba, 0x75,
			0x86, 0x7e, 0x84, 0x8d, 0x91, 0xa4, 0xae, 0xa2, 0xd6, 0xd5, 0xd3, 0xd7,
			0xd4, 0xd2
		};
		uint16 label = id >= 0 && id < 110 ? kLabels[id] : 0;
		if (label == 0)
			return;
		cmd(kSelf, kCmdPlay, label);
		int16 width = cmd(kSelf, kCmdGetSize, label);
		cmd(kSelf, kCmdSetPos, label, 0x102 - width / 2, 0x5a, 0x4e2f);
		var(0x42).set(label);
	}
};

// --- Object 47 ---

/**
 * Object 47 (vtable 0x4b4c44, FUN_00450390): the quiz in room 0x2f.
 *
 * Variables: 0x80 the level, 0x81-0x84 the sprites of the question (0x83:
 * the picture 0x4a8-0x4b0, whose texts go to children 0xb2b/0xb2c), 0x85 an
 * answer was chosen, 0x86 the number of answers, 0x87 the help is shown.
 */
class Object47 : public BatchCScript {
public:
	Object47(EmuObject *obj) : BatchCScript(obj) {}

protected:
	bool acceptsMessage(Message &msg) override {
		// FUN_004209d0
		return acceptsUnlessDragging(msg);
	}

	void handleMessage(Message &msg) override {
		if (var(1, kGameState).get() == 0x2f)
			handleInRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	/** The texts of the picture (variable 0x83) for child 0xb2c. */
	static int32 pictureText1(int32 picture) {
		switch (picture) {
		case 0x4a8: return 0x63e;
		case 0x4a9: return 0x6ec;
		case 0x4aa: return 0x7ad;
		case 0x4ab: return 0x8a6;
		case 0x4ac: return 0x907;
		case 0x4ad: return 0x91a;
		case 0x4ae: return 0x91c;
		case 0x4af: return 0xaab;
		default: return 0xaad;
		}
	}

	/** ... and for child 0xb2b. */
	static int32 pictureText2(int32 picture) {
		switch (picture) {
		case 0x4a8: return 0x63d;
		case 0x4a9: return 0x6eb;
		case 0x4aa: return 0x7ac;
		case 0x4ab: return 0x8a5;
		case 0x4ac: return 0x906;
		case 0x4ad: return 0x919;
		case 0x4ae: return 0x91b;
		case 0x4af: return 0xaaa;
		default: return 0xaac;
		}
	}

	void showPictureTexts() {
		int32 picture = var(0x83).get();
		cmd(kSelf, 0xe, 0xb2c, 0x2f, pictureText1(picture));
		cmd(kSelf, 0xe, 0xb2b, 0x2f, pictureText2(picture));
	}

	/** Stops (or plays, or enables) the sprite in variable id if there is one. */
	void stopSprite(uint16 id) {
		if (var(id).get() != 0)
			cmd(kSelf, kCmdStop, var(id).get());
	}

	bool isQuizChild(int32 child) {
		int32 picture = var(0x83).get();
		return child == picture || child == 0x4c9 || child == 0xb2b || child == 0xb2c ||
			child == 0x4d5 || child == 0x4d4 || child == 0x4b1;
	}

	void handleInRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			stopTimer(5);
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
				send(0x10, kMsgSay, 0x90f090e, 0x2f);
			} else {
				var(0x43).set(1);
				send(0xf, kMsgSay, 0x9090908, 0x10);
			}
			break;
		case 0x2e:
			if (msg.p2 == 0x90e)
				cmd(kSelf, kCmdPlay, 0x4c9, 0x2f);
			break;
		case 0x3b: {
			int32 child = msg.p1;
			msg.result = isQuizChild(child) || child == 0x4cd;
			break;
		}
		case 0x3c:
			if (msg.p1 == 0x4cd)
				send(kSpeech, kMsgSay, 0x9170916);
			else if (isQuizChild(msg.p1))
				send(kSpeech, kMsgSay, 0x9150914);
			break;
		case 0x48:
			cmd(kSelf, kCmdEnable, 0x4b1, 1);
			break;
		case 0x55:
			newQuestion(msg.p1);
			break;
		case 0x56:
			nextPicture();
			break;
		case kMsgClick:
			click(msg.p1);
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x4c9)
				start();
			break;
		default:
			break;
		}
	}

	void enter() {
		static const struct {
			uint16 child;
			int16 x, y;
		} kButtons[] = {
			{ 0xb1e, 0x20, 0x6e }, { 0xb1f, 0x6b, 0x6e }, { 0xb20, 0x20, 0x7f },
			{ 0xb21, 0x6b, 0x7f }, { 0xb22, 0x20, 0x90 }, { 0xb23, 0x6b, 0x90 },
			{ 0xb24, 0x20, 0xa1 }, { 0xb25, 0x6b, 0xa1 }, { 0xb26, 0x20, 0xb2 }
		};
		cmd(kSelf, kCmdUsePalette, 0x4cd);
		cmd(kSelf, kCmdLoad, 0);
		cmd(kSelf, kCmdSetPos, 0x4c9, 0, 0x46, 0xaa);
		cmd(kSelf, kCmdSetPos, 0xfe01, 0x80, 0, 0x6e);
		for (uint16 picture = 0x4a8; picture <= 0x4b0; picture++)
			cmd(kSelf, kCmdSetPos, picture, 0x20, 0x68, 0xaf);
		cmd(kSelf, kCmdSetPos, 0x4cd, 0, 0, 100);
		cmd(kSelf, kCmdPlay, 0x4cd);
		cmd(kSelf, kCmdSetPos, 0x4b1, 0x8e, 0x4c, 0xaf);
		if (var(8, 8).get() == 0)
			cmd(kSelf, kCmdSetPos, 0x59e, 0x20, 0x4c, 0xaf);
		cmd(kSelf, kCmdSetPos, 0x285, 0x57, 0x4c, 0xaf);
		cmd(kSelf, kCmdSetZ, 0xfe03, 0x6f);
		cmd(kSelf, kCmdPlay, 0x4d7);
		cmd(kSelf, kCmdPlay, 0x4d9);
		cmd(kSelf, kCmdPlay, 0x4d6);
		cmd(kSelf, kCmdSetZ, 0x4d6, 0x65);
		cmd(kSelf, kCmdEnable, 0x4d6, 0);
		cmd(kSelf, kCmdSetPos, 0x4d5, 0xac, 0xe3, 0xaf);
		cmd(kSelf, kCmdSetPos, 0x4d4, 0xac, 0x137, 0xaf);
		cmd(kSelf, kCmdSetPos, 0xb2c, 0x1f, 0xce, 0xaf);
		cmd(kSelf, kCmdSetView, 0xb2c, 0x96, 0x10);
		cmd(kSelf, kCmdSetPos, 0xb2b, 0x20, 0xe0, 0xaf);
		cmd(kSelf, kCmdSetView, 0xb2b, 0x8c, 100);
		for (uint i = 0; i < ARRAYSIZE(kButtons); i++)
			cmd(kSelf, kCmdSetPos, kButtons[i].child, kButtons[i].x, kButtons[i].y, 0xaf);
		cmd(kSelf, kCmdSetView, 0xfe75, 0x5a, 0x10);
		var(0x87).set(0);
	}

	/** The intro animation ended: the quiz starts. */
	void start() {
		cmd(kSelf, kCmdStop, 0x4d9);
		cmd(kSelf, kCmdPlay, 0x4b1);
		cmd(kSelf, kCmdPlay, 0x285);
		cmd(kSelf, kCmdEnable, 0x4b1, 0);
		if (var(8, 8).get() == 0) {
			cmd(kSelf, kCmdPlay, 0x59e);
			cmd(kSelf, kCmdEnable, 0x59e, 1);
		}
		cmd(kSelf, kCmdPlay, 0xb2c);
		cmd(kSelf, kCmdPlay, 0xb2b);
		cmd(kSelf, kCmdPlay, 0x4d5);
		cmd(kSelf, kCmdPlay, 0x4d4);
		send(kSelf, 0x55, var(0x80).get());
		send(kGameState, 0xa);
		cmd(kSelf, kCmdPlay, 0x4de);
	}

	/** Message 0x55: shows question p1 (1-9). */
	void newQuestion(int32 question) {
		static const struct {
			uint16 s81, s84, s82, s83;
		} kQuestions[] = {
			{ 0x4b9, 0x4ba, 0x4b8, 0x4a8 },
			{ 0x4bc, 0x4bd, 0x4bb, 0x4a9 },
			{ 0x4bf, 0x4c0, 0x4be, 0x4aa },
			{ 0x4c2, 0x4c3, 0x4c1, 0x4ab },
			{ 0x4cb, 0x4cc, 0x4ca, 0x4ac },
			{ 0x4cf, 0x4d0, 0x4ce, 0x4ad },
			{ 0x4d2, 0x4d3, 0x4d1, 0x4ae },
			{ 0x4d9, 0x4da, 0x4d8, 0x4af },
			{ 0x4dc, 0x4dd, 0x4db, 0x4b0 }
		};
		cmd(kSelf, kCmdEnable, 0x4b1, 0);
		cmd(kSelf, kCmdEnable, 0x4d6, var(0x86).get() < 9 ? 1 : 0);
		stopSprite(0x81);
		stopSprite(0x82);
		stopSprite(0x84);
		stopSprite(0x83);
		if (question >= 1 && question <= 9) {
			const auto &q = kQuestions[question - 1];
			var(0x81).set(q.s81);
			var(0x84).set(q.s84);
			var(0x82).set(q.s82);
			var(0x83).set(q.s83);
		}
		cmd(kSelf, kCmdPlay, var(0x81).get());
		cmd(kSelf, kCmdPlay, var(0x82).get());
		cmd(kSelf, kCmdPlay, var(0x83).get());
		showPictureTexts();
		var(0x85).set(0);
	}

	/** Message 0x56: the next picture. */
	void nextPicture() {
		stopSprite(0x83);
		int32 picture = var(0x83).get();
		if (picture >= 0x4a8 && picture <= 0x4af)
			picture++;
		else
			picture = 0x4a8;
		// 0x4af -> 0x4b0; anything else, 0x4b0 included -> 0x4a8
		var(0x83).set(picture);
		if (var(0x83).get() != 0)
			cmd(kSelf, kCmdPlay, var(0x83).get());
		showPictureTexts();
	}

	void click(int32 child) {
		switch (child) {
		case 0x285:
			toggleHelp();
			break;
		case 0x4b1:
			if (var(0x87).get() == 0 && var(0x85).get() != 0) {
				send(kSelf, 0x55, var(0x80).get());
				send(0x10, kMsgSay, 0x9110910);
			}
			break;
		case 0x4b8: case 0x4bb: case 0x4be: case 0x4c1: case 0x4ca:
		case 0x4ce: case 0x4d1: case 0x4d8: case 0x4db:
			answer();
			break;
		case 0x4d4:
			cmd(kSelf, kCmdScrollBy, 0xb2b, 0, 100);
			break;
		case 0x4d5:
			cmd(kSelf, kCmdScrollBy, 0xb2b, 0, -100);
			break;
		case 0x4d6:
			if (var(0x87).get() == 0)
				send(kGameState, 0x14, 1);
			break;
		case 0x4d7:
			send(kGameState, kMsgGotoRoom, 0x3a);
			break;
		case 0x59e:
			clickWebLink(0xb);
			break;
		default:
			if (child >= 0x4a8 && child <= 0x4b0) {
				if (var(0x87).get() == 0)
					send(kSelf, 0x56);
			} else if (child >= 0xb1e && child <= 0xb26) {
				helpTopic(child);
			}
			break;
		}
	}

	void toggleHelp() {
		if (var(0x87).get() == 0) {
			if (var(0x83).get() != 0)
				cmd(kSelf, kCmdEnable, var(0x83).get(), 0);
			cmd(kSelf, kCmdPlay, 0xfe75, 0x4b, 0x14);
			cmd(kSelf, 0xe, 0xb2c, 0x2f, 0x5a1);
			cmd(kSelf, 0xe, 0xb2b, 0x2f, 0x5a0);
			var(0x87).set(1);
		} else {
			if (var(0x83).get() != 0)
				cmd(kSelf, kCmdEnable, var(0x83).get(), 1);
			cmd(kSelf, kCmdStop, 0xfe75, 0x4b, 0x14);
			showPictureTexts();
			var(0x87).set(0);
		}
	}

	/** One of the answers was clicked. */
	void answer() {
		if (var(0x87).get() != 0)
			return;
		stopSprite(0x81);
		stopSprite(0x82);
		if (var(0x84).get() != 0)
			cmd(kSelf, kCmdPlay, var(0x84).get());
		var(0x85).set(1);
		cmd(kSelf, kCmdEnable, 0x4d6, 0);
		var(0x86).postIncrement();
		if (var(0x86).get() == 3 && var(0x35, 0x52).at(0x2e).get() == 0)
			send(0xf, kMsgSay, 0x9130912);
		if (var(0x86).get() < 10) {
			cmd(0x52, kCmdSetCursor, 0x23c, (int32)0x80000007);
			cmd(0x52, kCmdPlay, 0x23c);
			cmd(0x52, kCmdEnable, 0x23c, 1);
			cmd(0x52, kCmdSetPos, 0x23c, 0x23a, 300, 200);
			var(0x37, 0x52).set(0);
		} else {
			cmd(kSelf, kCmdEnable, 0x4b1, 1);
		}
	}

	/** Children 0xb1e-0xb26: the help topics. */
	void helpTopic(int32 child) {
		static const uint16 kTexts[][2] = {
			{ 0x5a1, 0x5a0 }, { 0x6bd, 0x6bc }, { 0x7a9, 0x7a8 }, { 0x7ab, 0x7aa },
			{ 0x83f, 0x83e }, { 0x871, 0x870 }, { 0x905, 0x904 }, { 0xab0, 0xaaf },
			{ 0xab3, 0xab2 }
		};
		const uint16 *t = kTexts[child - 0xb1e];
		cmd(kSelf, 0xe, 0xb2c, 0x2f, t[0]);
		cmd(kSelf, 0xe, 0xb2b, 0x2f, t[1]);
	}
};

// --- Object 48 ---

/**
 * Object 48 (vtable 0x4b4c24, FUN_00455500): the recipe book, opened in
 * room 0x16. Variables: 0x38 open, 0x39 the recipe shown (the child of its
 * picture), 0x3a-0x3f the voices of its buttons per recipe, 0x44 a voice
 * is playing.
 */
class Object48 : public BatchCScript {
public:
	Object48(EmuObject *obj) : BatchCScript(obj) {}

protected:
	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgLeave:
			if (msg.p1 != 0x16)
				break;
			stopTimer(5);
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgEnter:
			if (msg.p1 == 0x16)
				enter();
			break;
		case 0xd:
			initVoices();
			break;
		case 0x32:
			open();
			break;
		case 0x33:
			if (var(0x38).get() == 0)
				break;
			stopTimer(5);
			cmd(kSelf, kCmdStop, 0);
			cmd(0x16, kCmdUsePalette, 1);
			var(0x38).set(0);
			break;
		case 0x34:
			showRecipe(msg.p1);
			break;
		case kMsgClick:
			click(msg);
			break;
		case kMsgSoundDone:
			if (msg.p1 != kSpeech)
				break;
			var(0x44).set(0);
			cmd(kSpeech, kCmdStop, 0xfe00);
			break;
		default:
			break;
		}
	}

private:
	void enter() {
		static const uint16 kRecipeButtons[][4] = {
			// child, x, y, view width
			{ 0xb03, 0x13c, 0xf5, 100 }, { 0xb04, 0x1a5, 0xf5, 0x94 },
			{ 0xb05, 0x13c, 0x109, 100 }, { 0xb06, 0x1a5, 0x109, 0x94 },
			{ 0xb07, 0x13c, 0x11d, 100 }, { 0xb08, 0x1a5, 0x11d, 0x94 },
			{ 0xb09, 0x13c, 0x14f, 100 }, { 0xb0a, 0x1a5, 0x14f, 0x94 },
			{ 0xb0b, 0x13c, 0x163, 100 }, { 0xb0c, 0x1a5, 0x163, 0x94 },
			{ 0xb0d, 0x13c, 0x177, 100 }, { 0xb0e, 0x1a5, 0x177, 0x94 }
		};
		static const uint16 kPictureOrder[] = { 6, 9, 10, 0xd, 0xe, 0x14, 0x26, 0x28, 0x2e, 0x30 };
		cmd(kSelf, kCmdLoad, 0x16);
		for (uint i = 0; i < ARRAYSIZE(kPictureOrder); i++) {
			cmd(kSelf, kCmdLoad, kPictureOrder[i]);
			cmd(kSelf, kCmdSetPos, kPictureOrder[i], 0x177, 0x55, 19999);
		}
		cmd(kSelf, kCmdLoad, 0x17);
		cmd(kSelf, kCmdSetZ, 0x17, 0x4e21);
		cmd(kSelf, kCmdLoad, 0x19);
		cmd(kSelf, kCmdSetZ, 0x19, 0x4e25);
		cmd(kSelf, kCmdLoad, 0x1a);
		cmd(kSelf, kCmdSetZ, 0x1a, 0x4e25);
		cmd(kSelf, kCmdLoad, 0x18);
		cmd(kSelf, kCmdSetZ, 0x18, 0x4e25);
		for (uint i = 0; i < ARRAYSIZE(kRecipeButtons); i++) {
			uint16 child = kRecipeButtons[i][0];
			cmd(kSelf, kCmdLoad, child);
			cmd(kSelf, kCmdSetPos, child, kRecipeButtons[i][1], kRecipeButtons[i][2], 0x4e21);
			// 0xb08 is taller.
			cmd(kSelf, kCmdSetView, child, kRecipeButtons[i][3], child == 0xb08 ? 0x3c : 0x14);
		}
	}

	void initVoices() {
		static const uint16 kRecipes[] = { 6, 9, 10, 0xd, 0xe, 0x14, 0x26, 0x28, 0x2e, 0x30 };
		static const uint16 kVoices[6][10] = {
			{ 0x9a2, 0x9b2, 0x9dc, 0x9aa, 0x994, 0x9cc, 0x9d4, 0x9bc, 0x9c4, 0x99c },
			{ 0x99f, 0x9ad, 0x9d8, 0x9a5, 0x991, 0x9c7, 0x9cf, 0x9b7, 0x9bf, 0x997 },
			{ 0x727, 0x9ae, 0x9d9, 0x9a6, 0x992, 0x9c8, 0x9d0, 0x9b8, 0x9c0, 0x998 },
			{ 0x9a0, 0x9af, 0x727, 0x9a7, 0x993, 0x9c9, 0x9d1, 0x9b9, 0x9c1, 0x999 },
			{ 0x9a1, 0x9b0, 0x9da, 0x9a8, 0x989, 0x9ca, 0x9d2, 0x9ba, 0x9c2, 0x99a },
			{ 0x727, 0x9b1, 0x9db, 0x9a9, 0x98a, 0x9cb, 0x9d3, 0x9bb, 0x9c3, 0x99b }
		};
		for (uint v = 0; v < 6; v++)
			for (uint i = 0; i < ARRAYSIZE(kRecipes); i++)
				var(0x3a + v).at(kRecipes[i]).set(kVoices[v][i]);
	}

	void open() {
		static const uint16 kChildren[] = {
			0x17, 0x19, 0x1a, 0xb03, 0xb04, 0xb05, 0xb06, 0xb07, 0xb08, 0xb09,
			0xb0a, 0xb0b, 0xb0c, 0xb0d, 0xb0e
		};
		// The recipe number (message 0x34) of each picture.
		static const uint16 kOrder[] = { 0xe, 0x26, 0xd, 0x14, 0x30, 0x2e, 6, 0x28, 9 };
		if (var(0x38).get() != 0)
			return;
		var(0x38).set(1);
		cmd(kSelf, kCmdUsePalette, 0x16);
		cmd(kSelf, kCmdPlay, 0x16);
		cmd(kSelf, kCmdSetZ, 0x16, 20000);
		for (uint i = 0; i < ARRAYSIZE(kChildren); i++)
			cmd(kSelf, kCmdPlay, kChildren[i]);
		if (var(8, 8).get() == 0) {
			cmd(kSelf, kCmdSetPos, 0x59e, 0xf, 0x1b8, 0x4e21);
			cmd(kSelf, kCmdPlay, 0x59e);
			cmd(kSelf, kCmdEnable, 0x59e, 1);
		}
		int32 recipe = 10;
		for (uint i = 0; i < ARRAYSIZE(kOrder); i++) {
			if (var(0x39).get() == kOrder[i]) {
				recipe = i + 1;
				break;
			}
		}
		if (recipe == 10) {
			// The last test: 10 for picture 10, else 1.
			if (var(0x39).get() != 10)
				recipe = 1;
		}
		send(kSelf, 0x34, recipe);
	}

	/** Message 0x34: shows recipe p1 (1-10). */
	void showRecipe(int32 recipe) {
		static const struct {
			uint16 picture;
			uint16 voice;
			uint16 texts[7];  ///< children 0xb04, 0xb06, 0xb08, 0xb09, 0xb0a, 0xb0c, 0xb0e
		} kRecipes[] = {
			{ 0x0e, 0x994, { 0x995, 0x955, 0x958, 0x959, 0x95a, 0x957, 0x956 } },
			{ 0x26, 0x9d4, { 0x9d5, 0x967, 0x96a, 0x96b, 0x96c, 0x969, 0x968 } },
			{ 0x0d, 0x9aa, { 0x9ab, 0x94f, 0x952, 0x953, 0x954, 0x951, 0x950 } },
			{ 0x14, 0x9cc, { 0x9cd, 0x961, 0x964, 0x965, 0x966, 0x963, 0x962 } },
			{ 0x30, 0x99c, { 0x99d, 0x979, 0x97c, 0x97d, 0x97e, 0x97b, 0x97a } },
			{ 0x2e, 0x9c4, { 0x9c5, 0x973, 0x976, 0x977, 0x978, 0x975, 0x974 } },
			{ 0x06, 0x9a2, { 0x9a3, 0x943, 0x946, 0x947, 0x948, 0x945, 0x944 } },
			{ 0x28, 0x9bc, { 0x9bd, 0x96d, 0x970, 0x971, 0x972, 0x96f, 0x96e } },
			{ 0x09, 0x9b2, { 0x9b3, 0x949, 0x94c, 0x94d, 0x94e, 0x94b, 0x94a } },
			{ 0x0a, 0x9dc, { 0x9dd, 0x95b, 0x95e, 0x95f, 0x960, 0x95d, 0x95c } }
		};
		static const uint16 kTextChildren[] = { 0xb04, 0xb06, 0xb08, 0xb09, 0xb0a, 0xb0c, 0xb0e };
		if (var(0x39).get() != 0)
			cmd(kSelf, kCmdStop, var(0x39).get());
		if (var(0x39).get() == 6)
			cmd(kSelf, kCmdStop, 0x18);
		if (var(0x44).get() == 1) {
			var(0x44).set(0);
			cmd(kSpeech, kCmdStop, 0xfe00);
		}
		if (recipe >= 1 && recipe <= 10) {
			const auto &r = kRecipes[recipe - 1];
			var(0x39).set(r.picture);
			cmd(kSpeech, kCmdPlay, r.voice, 0x30);
			var(0x44).set(1);
			for (uint i = 0; i < ARRAYSIZE(kTextChildren); i++)
				cmd(kSelf, 0xe, kTextChildren[i], 0x12, r.texts[i]);
			// Recipe 7 (picture 6) has a second picture.
			if (r.picture == 6)
				cmd(kSelf, kCmdPlay, 0x18);
		}
		cmd(kSelf, kCmdPlay, var(0x39).get());
	}

	/** A voice of the recipe shown: variable map of the button. */
	void sayFromMap(uint16 map) {
		VarRef recipe = var(0x39);
		VarRef voices = var(map);
		uint16 voice = voices.at(recipe.get()).get();
		cmd(kSpeech, kCmdPlay, voice);
		var(0x44).set(1);
	}

	void say(uint16 voice) {
		cmd(kSpeech, kCmdPlay, voice);
		var(0x44).set(1);
	}

	void click(Message &msg) {
		if (var(0x44).get() == 1) {
			var(0x44).set(0);
			cmd(kSpeech, kCmdStop, 0xfe00);
		}
		switch (msg.p1) {
		case 0x16:
			send(kSelf, 0x33);
			break;
		case 0x18:
			send(kSelf, 0x34, 10);
			break;
		case 0x19:
			send(kSelf, 0x34, msg.p3);
			break;
		case 0x1a:
			say(0x9b4);
			break;
		case 0x59e:
			clickWebLink(9);
			break;
		case 0xb03:
			say(0x9ec);
			break;
		case 0xb04:
			sayFromMap(0x3a);
			break;
		case 0xb05:
			say(0x97f);
			break;
		case 0xb06:
			sayFromMap(0x3b);
			break;
		case 0xb07:
			say(0x981);
			break;
		case 0xb08:
			sayFromMap(0x3c);
			break;
		case 0xb09:
			say(0x983);
			break;
		case 0xb0a:
			sayFromMap(0x3d);
			break;
		case 0xb0b:
			say(0x985);
			break;
		case 0xb0c:
			sayFromMap(0x3e);
			break;
		case 0xb0d:
			say(0x987);
			break;
		case 0xb0e:
			sayFromMap(0x3f);
			break;
		default:
			break;
		}
	}
};

// --- Object 46 ---

/**
 * Object 46 (vtable 0x4b4c64, FUN_0044bef0): the game in room 0x2e where
 * Magnus (object 15) climbs on shelves with the arrow keys to find hidden
 * things.
 *
 * Variables: 0x79-0x7c the positions ((y << 16) | x) of the places on
 * shelves 1-4 by column (0: none), 0x7d the column, 0x7e the shelf,
 * 0x7f Magnus's animation (0x488: standing).
 */
class Object46 : public BatchCScript {
public:
	Object46(EmuObject *obj) : BatchCScript(obj) {}

protected:
	bool acceptsMessage(Message &msg) override {
		// FUN_004209d0
		return acceptsUnlessDragging(msg);
	}

	void handleMessage(Message &msg) override {
		if (var(1, kGameState).get() == 0x2e)
			handleInRoom(msg);
		if (msg.msg == 0xd)
			initPlaces();
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	enum {
		kMagnus = 0xf,
		kStanding = 0x488,
		kCmdFrameTrigger = 9  ///< p1 = frame, p2 = message to post then (0: none) [unverified]
	};

	static bool isThing(int32 child) {
		return (child >= 0x494 && child <= 0x4a0);
	}

	int32 shelf() {
		return var(0x7e).get();
	}

	/** The position on shelf map at the column (+ delta). */
	int32 placeIn(uint16 map, int32 delta = 0) {
		VarRef column = var(0x7d);
		VarRef places = var(map);
		return places.at(column.get() + delta).get();
	}

	/** The position of the current place (0 if the shelf is not 1-4). */
	int32 currentPlace() {
		if (shelf() == 1)
			return placeIn(0x79);
		if (shelf() == 2)
			return placeIn(0x7a);
		if (shelf() == 3)
			return placeIn(0x7b);
		if (shelf() == 4)
			return placeIn(0x7c);
		return 0;
	}

	/** values[i] for shelf first + i (0 if none matches). */
	int32 byShelf(int32 first, const uint16 *values, int n) {
		for (int i = 0; i < n - 1; i++)
			if (shelf() == first + i)
				return values[i];
		return shelf() == first + n - 1 ? values[n - 1] : 0;
	}

	/** The vertical offset of a climb from/to the upper shelves. */
	int32 climbOffset() {
		if (shelf() == 1)
			return -0x14;
		if (shelf() == 2)
			return -0x1e;
		shelf();
		return 0;
	}

	/** Plays Magnus's animation anim at position pos. */
	void showMagnus(uint16 anim, int32 pos, int32 p2 = 0) {
		cmd(kMagnus, kCmdStop, var(0x7f).get());
		cmd(kMagnus, kCmdPlay, anim, 0x2e, p2);
		cmd(kMagnus, kCmdSetPos, anim, (int16)pos, (int16)((uint32)pos >> 16), 0x96);
		var(0x7f).set(anim);
	}

	/** Magnus can move: he is paused or standing. */
	bool canMove() {
		if (cmd(kMagnus, kCmdIsPaused, var(0x7f).get()) != 0)
			return true;
		return var(0x7f).get() == kStanding;
	}

	void handleInRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			setEventMask(0);
			setKeyFocus(0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			enter();
			break;
		case kMsgEntered:
			if (var(0x43).get() == 1) {
				send(kGameState, 0xa);
			} else {
				var(0x43).set(1);
				send(0x10, kMsgSay, 0x8f308f2, 0xf);
			}
			break;
		case 0xb:
			cmd(kSelf, kCmdPlay, 0x47d);
			break;
		case 0x2e:
			if (msg.p2 == 0x8fc)
				send(kSelf, 0x47);
			break;
		case 0x3b:
			msg.result = isThing(msg.p1);
			break;
		case 0x3c:
			if (!isThing(msg.p1))
				break;
			switch (var(0x44).get()) {
			case 0:
				send(kSpeech, kMsgSay, 0x8ff08fe);
				break;
			case 1:
				send(kSpeech, kMsgSay, 0x9030902);
				break;
			default:
				break;
			}
			break;
		case 0x47:
			// Magnus back at the start.
			cmd(kMagnus, kCmdStop, var(0x7f).get());
			var(0x7d).set(3);
			var(0x7e).set(3);
			var(0x7f).set(kStanding);
			cmd(kMagnus, kCmdPlay, kStanding);
			cmd(kMagnus, kCmdSetPos, kStanding, 0x4c, 0x5e, 0x93);
			send(kGameState, 0x14, 1);
			break;
		case kMsgClick:
			if (msg.p1 != 0x4a1)
				break;
			if (!canMove())
				break;
			send(kGameState, kMsgGotoRoom, 0x1a);
			break;
		case kMsgAnimDone:
			switch (msg.p1) {
			case 0x47f: case 0x481: case 0x482: case 0x484: case 0x485:
			case 0x487: case 0x489: case 0x48a: case 0x48b: case 0x48c:
			case 0x48d: case 0x48e: case 0x48f: case 0x490:
				arrived();
				break;
			default:
				break;
			}
			break;
		case kMsgKeyDown:
			keyDown(msg.p1);
			break;
		default:
			break;
		}
	}

	void enter() {
		static const struct {
			uint16 child;
			int16 x, y, z;
		} kThings[] = {
			{ 0x495, 0x1c6, 0x67, 0x6f }, { 0x499, 0x12c, 0x91, 0x70 },
			{ 0x49a, 0x1a7, 0xc6, 0x71 }, { 0x49b, 0x191, 0x66, 0x6f },
			{ 0x49c, 0xe7, 0x90, 0x70 }, { 0x49d, 0x1ca, 0x125, 0x72 },
			{ 0x49e, 0x162, 0x65, 0x6f }, { 0x49f, 0x8c, 0xc4, 0x71 },
			{ 0x4a0, 0x179, 0x123, 0x72 }, { 0x496, 0x218, 0x90, 0x70 },
			{ 0x497, 0, 0xc1, 0x71 }, { 0x498, 0x12d, 0x121, 0x72 }
		};
		cmd(kSelf, kCmdUsePalette, 0x494);
		cmd(kSelf, kCmdPlay, 0x494);
		cmd(kSelf, kCmdSetZ, 0x494, 100);
		for (uint i = 0; i < ARRAYSIZE(kThings); i++) {
			// Paused on their first frame [unverified].
			cmd(kSelf, kCmdPlay, kThings[i].child, 0, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, kThings[i].child, kThings[i].x, kThings[i].y, kThings[i].z);
		}
		cmd(kSelf, kCmdSetPos, 0x4a1, 0x5d, 0x59, 0x70);
		cmd(kSelf, kCmdPlay, 0x4a1);
		cmd(kSelf, kCmdLoad, 0x47d);
		cmd(kSelf, kCmdSetPos, 0x47d, 10, 0x154, 300);
		setKeyFocus(getId());
		setEventMask(0x10000);
		var(0x7d).set(3);
		var(0x7e).set(3);
		var(0x7f).set(kStanding);
	}

	void keyDown(int32 key) {
		if (var(2, kGameState).get() != 0)
			return;
		if (!canMove())
			return;
		if (cmd(kSelf, kCmdIsPlaying, 0x47d))
			cmd(kSelf, kCmdStop, 0x47d);
		switch (key) {
		case 0x26:
			climbUp();
			break;
		case 0x28:
			climbDown();
			break;
		case 0x25:
			walkLeft();
			break;
		case 0x27:
			walkRight();
			break;
		default:
			break;
		}
	}

	/**
	 * The climbing animations move Magnus by step on the frames 4, 8, ...
	 * (n triggers), by messages to his animation.
	 */
	void setClimbTriggers(uint16 anim, int n, int32 step) {
		for (int i = 1; i <= n; i++) {
			cmdFrameTrigger(kMagnus, anim, i * 4, makeCmd(kMagnus, kCmdSetOffset, anim, step));
		}
	}

	void clearClimbTriggers(uint16 anim, int n) {
		for (int i = 1; i <= n; i++)
			cmd(kMagnus, kCmdFrameTrigger, anim, i * 4, 0);
	}

	void climbUp() {
		static const uint16 kAnims[] = { 0x481, 0x484, 0x487 };
		if (shelf() == 1)
			return;
		uint16 map;
		if (shelf() == 2)
			map = 0x79;
		else if (shelf() == 3)
			map = 0x7a;
		else if (shelf() == 4)
			map = 0x7b;
		else
			return;
		if (placeIn(map) == 0)
			return;
		uint16 anim = byShelf(2, kAnims, 3);
		int32 from = currentPlace();
		cmd(kMagnus, kCmdStop, var(0x7f).get());
		clearClimbTriggers(anim, 4);
		var(0x7e).postDecrement();
		int32 offset = climbOffset();
		int32 to = currentPlace();
		int32 step = ((int16)to - (int16)from + offset) / 4;
		setClimbTriggers(anim, 4, step);
		cmd(kMagnus, kCmdPlay, anim, 0x2e);
		cmd(kMagnus, kCmdSetPos, anim, (int16)from, (int16)((uint32)from >> 16), 0x96);
		var(0x7f).set(anim);
	}

	void climbDown() {
		static const uint16 kAnims[] = { 0x47f, 0x482, 0x485 };
		if (shelf() == 4)
			return;
		uint16 map;
		if (shelf() == 3)
			map = 0x7c;
		else if (shelf() == 2)
			map = 0x7b;
		else if (shelf() == 1)
			map = 0x7a;
		else
			return;
		if (placeIn(map) == 0)
			return;
		uint16 anim = byShelf(1, kAnims, 3);
		int32 from = currentPlace();
		cmd(kMagnus, kCmdStop, var(0x7f).get());
		clearClimbTriggers(anim, 5);
		int32 offset = climbOffset();
		var(0x7e).postIncrement();
		int32 to = currentPlace();
		int32 step = ((int16)to - (int16)from - offset) / 5;
		setClimbTriggers(anim, 5, step);
		cmd(kMagnus, kCmdPlay, anim, 0x2e);
		cmd(kMagnus, kCmdSetPos, anim, (int16)from, (int16)((uint32)from >> 16), 0x96);
		var(0x7f).set(anim);
	}

	/** The map of the current shelf, 0 if none. */
	uint16 shelfMap() {
		if (shelf() == 1)
			return 0x79;
		if (shelf() == 2)
			return 0x7a;
		if (shelf() == 3)
			return 0x7b;
		if (shelf() == 4)
			return 0x7c;
		return 0;
	}

	void walkLeft() {
		static const uint16 kAnims[] = { 0x489, 0x48b, 0x48d, 0x48f };
		uint16 map = shelfMap();
		if (!map || placeIn(map, -1) == 0)
			return;
		var(0x7d).postDecrement();
		uint16 anim = byShelf(1, kAnims, 4);
		showMagnus(anim, currentPlace());
	}

	void walkRight() {
		static const uint16 kAnims[] = { 0x48a, 0x48c, 0x48e, 0x490 };
		uint16 map = shelfMap();
		if (!map || placeIn(map, 1) == 0)
			return;
		if (var(0x7f).get() == kStanding) {
			uint16 anim = byShelf(1, kAnims, 4);
			showMagnus(anim, currentPlace());
		} else {
			cmd(kMagnus, kCmdResume, var(0x7f).get());
		}
		var(0x7d).postIncrement();
	}

	/** Magnus arrived at a place: shows the thing hidden there, if any. */
	void arrived() {
		static const struct {
			int32 column, shelf;
			uint16 thing;
		} kHidden[] = {
			{ 2, 3, 0x497 }, { 4, 3, 0x49f }, { 5, 2, 0x49c }, { 6, 2, 0x499 },
			{ 6, 4, 0x498 }, { 7, 1, 0x49e }, { 7, 4, 0x4a0 }, { 8, 1, 0x49b },
			{ 8, 3, 0x49a }, { 8, 4, 0x49d }, { 9, 1, 0x495 }, { 10, 2, 0x496 }
		};
		for (uint i = 0; i < ARRAYSIZE(kHidden); i++) {
			if (var(0x7d).get() == kHidden[i].column && var(0x7e).get() == kHidden[i].shelf) {
				found(kHidden[i].thing);
				return;
			}
		}
		static const uint16 kAnims[] = { 0x48a, 0x48c, 0x48e, 0x490 };
		uint16 anim = byShelf(1, kAnims, 4);
		if (anim == 0)
			return;
		int32 place = currentPlace();
		cmd(kMagnus, kCmdStop, var(0x7f).get());
		cmd(kMagnus, kCmdPlay, anim, 0x2e, (int32)0xc0003fff);
		cmd(kMagnus, kCmdSetPos, anim, (int16)place, (int16)((uint32)place >> 16), 0x96);
		var(0x7f).set(anim);
		if (var(0x7e).get() != 4 || var(0x7d).get() != 9)
			return;
		if (var(0x35, 0x52).at(0x2e).get() != 0)
			return;
		send(kSpeech, kMsgSay, 0x9010900, 0x2e);
		send(0x52, 0x48);
	}

	void found(uint16 thing) {
		static const uint16 kAnims[] = { 0x47e, 0x480, 0x483, 0x486 };
		uint16 anim = byShelf(1, kAnims, 4);
		if (anim == 0)
			return;
		int32 place = currentPlace();
		cmd(kSelf, kCmdResume, thing);
		showMagnus(anim, place);
		var(0x10, kMagnus).set(0);
		var(0x11, kMagnus).set(0);
		send(kMagnus, kMsgSay, 0x8fb08fa, 0x10);
	}

	/** Message 0xd: the places on the shelves. */
	void initPlaces() {
		static const int32 kShelf1[] = { 0xffc4, 1, 0x3c, 0x77, 0xb0, 0xe8, 0x120, 0x154, 0x18f, 0x1c5, 0x204 };
		static const int32 kShelf2[] = { 0xffab, 0xffe7, 0x28, 100, 0xa6, 0xec, 0x131, 0x16d, 0x1ad, 0x1ef };
		static const int32 kShelf3[] = { 0xaffb5, 0xa000a, 0xa0051, 0xa00a0, 0xa00ee, 0xa013b, 0xa0187, 0xa01d3 };
		static const int32 kShelf4[] = { 0x37000a, 0x370066, 0x3700c2, 0x37011e, 0x37017a, 0x3701d6, 0x370233 };
		for (uint i = 0; i < ARRAYSIZE(kShelf1); i++)
			var(0x79).at(1 + i).set(kShelf1[i]);
		for (uint i = 0; i < ARRAYSIZE(kShelf2); i++)
			var(0x7a).at(1 + i).set(kShelf2[i]);
		for (uint i = 0; i < ARRAYSIZE(kShelf3); i++)
			var(0x7b).at(2 + i).set(kShelf3[i]);
		for (uint i = 0; i < ARRAYSIZE(kShelf4); i++)
			var(0x7c).at(3 + i).set(kShelf4[i]);
	}
};

IVANOFF_SCRIPT(Object2) { return new Object2(obj); }
IVANOFF_SCRIPT(Object3) { return new Object3(obj); }
IVANOFF_SCRIPT(Object4) { return new Object4(obj); }
IVANOFF_SCRIPT(Object33) { return new Object33(obj); }
IVANOFF_SCRIPT(Object46) { return new Object46(obj); }
IVANOFF_SCRIPT(Object47) { return new Object47(obj); }
IVANOFF_SCRIPT(Object48) { return new Object48(obj); }

} // End of namespace Ivanoff
