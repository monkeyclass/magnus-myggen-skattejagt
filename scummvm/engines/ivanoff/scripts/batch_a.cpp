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

// Batch a: the toolbar (object 8), the save/load screen (object 14) and
// the game state object (object 17), with the settings file helpers they
// share (the original's settings class, FUN_00474410 and following).

#include "common/hashmap.h"

#include "ivanoff/sprite.h"
#include "ivanoff/files.h"
#include "ivanoff/ivanoff.h"
#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

namespace {

/** Globals of the original. */
enum {
	kGlobSettings = 0x4cab98,    ///< the settings object
	kGlobVolume = 0x4c6940,      ///< 16 bits: the sound volume, level * 0x101
	kGlobSound1 = 0x4c6970,      ///< FUN_00477190 parameters
	kGlobSound2 = 0x4c6974,
	kGlobSound3 = 0x4c6978,
	kGlobToolbarPalette = 0x4caa40,
	kGlobVolumeDrag = 0x4caa48,
	kGlobSaveScreenPalette = 0x4caa5c,
	kGlobNextMsg = 0x4caa60,     ///< message to post when the save screen closes
	kGlobNextTarget = 0x4caa64,  ///< 16 bits: its target
	kGlobQuitPalette = 0x4cab68,
	kGlobNextRoom = 0x4cab70
};

bool endsWithBackslash(const Common::String &s) {
	return !s.empty() && s.lastChar() == '\\';
}

} // End of anonymous namespace

/**
 * The settings (the class at 0x474410-0x476190, DAT_004cab98): an INI file
 * and the installer's registry values. In this engine the save folder is
 * always C:\\MM2\\appdata (ScummVM's save files) and the file is
 * settings.dat there, with one section, [SETTINGS].
 */
class GameSettings {
public:
	GameSettings() : _m(*g_engine->_files) {}

	/** FUN_004749b0 (GetPrivateProfileIntA) */
	int32 getInt(const Common::String &key, int32 def) {
		Common::String v;
		return _m.iniGet(kFile, "SETTINGS", key, v) ? atoi(v.c_str()) : def;
	}

	/** FUN_004748e0 */
	void setInt(const Common::String &key, int32 value) {
		Common::String v = Common::String::format("%d", value);
		_m.iniSet(kFile, "SETTINGS", key, &v);
	}

	/** FUN_00474a60: is the key in the settings file? */
	bool hasKey(const Common::String &key) {
		Common::String v;
		return _m.iniGet(kFile, "SETTINGS", key, v);
	}

	/** FUN_00474780: a value of HKLM\\Software\\IVANOFF Interactive\\MM2, or "". */
	Common::String gameRegString(const Common::String &value) {
		Common::String name = value;
		name.toLowercase();
		return _m.registryValue(name);
	}

	/** FUN_00474650: the folder of the save games. */
	Common::String saveDir() {
		return kDir;
	}

private:
	static const char *const kDir;
	static const char *const kFile;
	GameFiles &_m;
};

const char *const GameSettings::kDir = "C:\\MM2\\appdata\\";
const char *const GameSettings::kFile = "C:\\MM2\\appdata\\settings.dat";

/**
 * Helpers shared by the classes of this batch.
 *
 * When the interpreter runs an original slot 5 or 6, the engine passes it
 * the message in a 0x2c-byte block of the game's heap, which stays
 * allocated while the handler sends further messages. The heap's layout
 * shows in the traces (objects pass heap addresses), so the native
 * handlers hold such a block too.
 */
class BatchAScript : public Script {
public:
	BatchAScript(GameObject *obj) : Script(obj) {}

	bool accepts(Message &msg) override {
		// Object4b51a4_accepts (FUN_004554f0): takes everything.
		return true;
	}

	void handle(Message &msg) override {
		handleMessage(msg);
	}

protected:
	virtual void handleMessage(Message &msg) = 0;

	/** The original makes the timer's message in the heap (FUN_00476bc0), the engine frees it. */
	void startTimer(uint16 timerId, int32 ms, uint16 target, uint16 msg, int32 p1 = 0, int32 p2 = 0, int32 p3 = 0) {
		Script::startTimer(timerId, ms, target, msg, p1, p2, p3);
	}


	GameSettings settings() { return GameSettings(); }

	/** The object's +0x18 field: the key events it receives. */

	/** Vtable slot 7 of the objects (FUN_00476a40). */

	/** The global sound volume (DAT_004c6940). */
	void setVolumeGlobal(int32 level) { setGlobal32(kGlobVolume, (uint16)(level * 0x101)); }

	/** Variable slot +0x3c: the text of a string variable. */
	Common::String varString(const VarRef &v) {
		return v.toString();
	}

	/** Variable slot +0x28: assigns a string (a CString by value, released by the callee). */
	void assignString(VarRef v, const Common::String &s) {
		v.assignString(s);
	}

	/** CString::Format of the original runtime, with two integers. */
	Common::String formatInts(const Common::String &fmt, int32 a, int32 b) {
		// CString::Format of the original runtime (the format comes from the game's texts).
		return Common::String::format(fmt.c_str(), a, b);
	}

	/** Restores a palette saved by kCmdUsePalette ((child << 16) | object, 0 for none). */
	void restorePalette(uint32 palette) {
		if (palette != 0)
			cmd((uint16)palette, kCmdUsePalette, (uint16)(palette >> 16));
	}
};

// --- Object 8 ---

/**
 * Object 8 (vtable 0x4b5124, handler FUN_0040e500): the toolbar, with the
 * volume slider (0x160), the music and web link switches (0xb2e, 0xb2f),
 * the load, save, close and quit buttons (0xb27-0xb2a) and the clock
 * (0x548-0x54b).
 *
 * Variables: 8 web links off, 9-0xb web link addresses, 0xd open,
 * 0xe sound level (0-255). Object 4's variable 0xb8: music on.
 */
class Object8 : public BatchAScript {
public:
	Object8(GameObject *obj) : BatchAScript(obj) {}

	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgLeave:
			if (var(0xd).get() != 0)
				send(kSelf, 0x1d);
			break;
		case 0xd:
			loadSettings();
			break;
		case 0xf:
			for (uint16 c = 0x548; c <= 0x54b; c++) {
				cmd(kSelf, kCmdLoad, c);
				cmd(kSelf, kCmdSetPos, c, 0x240, 0x1a0, 0x3a99);
			}
			send(kSelf, 0x16, var(5, kGameState).get());
			setEventMask(0x10000);
			break;
		case 0x10:
			cmd(kSelf, kCmdUnload, 0);
			setEventMask(0);
			break;
		case 0x16:
			showClock(msg.p1);
			break;
		case 0x1a: {
			GameSettings s = settings();
			s.setInt("WWWLinksOff", var(8).get());
			s.setInt("BkgMusicOn", var(0xb8, 4).get());
			s.setInt("SoundLevel", var(0xe).get());
			break;
		}
		case 0x1c:
			open();
			break;
		case 0x1d:
			close();
			break;
		case kMsgClick:
			click(msg);
			break;
		case kMsgButtonUp:
		case kMsgMouseLeave:
			if (msg.p1 == 0x160)
				setGlobal32(kGlobVolumeDrag, 0);
			break;
		case kMsgMouseMove:
			if (msg.p1 == 0x160 && global32(kGlobVolumeDrag) != 0)
				setLevel(0x14f - (int16)((uint32)msg.p2 >> 16));
			break;
		case kMsgKeyDown:
			switch (msg.p1) {
			case 0x26: // up
				setLevel(var(0xe).get() + 0x19);
				break;
			case 0x28: // down
				setLevel(var(0xe).get() - 0x19);
				break;
			case 0x72: // F3
				toggleMusic();
				break;
			case 0x76: // F7
				if (var(0xd).get() == 0)
					send(kSelf, 0x1c);
				else
					send(kSelf, 0x1d);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

private:
	void loadSettings() {
		var(8).set(0);
		var(0xb8, 4).set(1);
		GameSettings s = settings();
		var(8).set(s.getInt("WWWLinksOff", 0));
		int32 music = s.getInt("BkgMusicOn", 1);
		var(0xb8, 4).set(music > 0);
		static const char *const kLinks[3] = { "WWWAstronomyLink", "WWWElementsLink", "WWWStarLink" };
		for (int i = 0; i < 3; i++) {
			Common::String link = s.gameRegString(kLinks[i]);
			if (!link.empty())
				assignString(var(9 + i), link);
		}
		var(0xd).set(0);
	}

	/** The clock face for the minutes played (0-60). */
	void showClock(int32 minutes) {
		if (minutes < 0x10) {
			if (cmd(kSelf, kCmdIsPlaying, 0x548))
				return;
			cmd(kSelf, kCmdPlay, 0x548);
			cmd(kSelf, kCmdStop, 0x549);
			cmd(kSelf, kCmdStop, 0x54a);
		} else if (minutes < 0x1f) {
			if (cmd(kSelf, kCmdIsPlaying, 0x549))
				return;
			cmd(kSelf, kCmdStop, 0x548);
			cmd(kSelf, kCmdPlay, 0x549);
			cmd(kSelf, kCmdStop, 0x54a);
		} else if (minutes < 0x2e) {
			if (cmd(kSelf, kCmdIsPlaying, 0x54a))
				return;
			cmd(kSelf, kCmdStop, 0x548);
			cmd(kSelf, kCmdStop, 0x549);
			cmd(kSelf, kCmdPlay, 0x54a);
		} else {
			if (cmd(kSelf, kCmdIsPlaying, 0x54b))
				return;
			cmd(kSelf, kCmdStop, 0x548);
			cmd(kSelf, kCmdStop, 0x549);
			cmd(kSelf, kCmdStop, 0x54a);
			cmd(kSelf, kCmdPlay, 0x54b);
			return;
		}
		cmd(kSelf, kCmdStop, 0x54b);
	}

	void open() {
		if (var(0xd).get() != 0)
			return;
		if (var(kVarBusy, kGameState).get() != 0)
			return;
		if (var(kVarDragMode, kGameState).get() != 0)
			return;
		if (var(0x1c, 10).get() != 0)
			return;
		if (send(32000, kMsgWhoIsDragged) != 0)
			return;
		if (var(0x1e, 0xe).get() != 0)
			return;
		var(0xd).set(1);
		setGlobal32(kGlobToolbarPalette, cmd(kSelf, kCmdUsePalette, 0x138));
		cmd(kSelf, kCmdPlay, 0x138);
		cmd(kSelf, kCmdSetZ, 0x138, 0x5212);
		cmd(kSelf, kCmdPlay, 0x160);
		cmd(kSelf, kCmdSetPos, 0x160, 0x1cc, 0x4f - var(0xe).get(), 0x520d);
		cmd(kSelf, kCmdPlay, 0xb2e);
		cmd(kSelf, kCmdSetPos, 0xb2e, 0x8d, 0x108, 0x521c);
		cmd(kSelf, kCmdEnable, 0xb2e, var(0xb8, 4).get());
		cmd(kSelf, kCmdPlay, 0xb2f);
		cmd(kSelf, kCmdSetPos, 0xb2f, 0x8d, 0x152, 0x521c);
		cmd(kSelf, kCmdEnable, 0xb2f, var(8).get() == 0);
		cmd(kSelf, kCmdPlay, 0x136);
		cmd(kSelf, kCmdSetZ, 0x136, 0x5221);
		cmd(kSelf, kCmdSetView, 0xb27, 0x136, 0x2d);
		cmd(kSelf, kCmdPlay, 0xb27);
		cmd(kSelf, kCmdSetPos, 0xb27, 0x89, 0x33, 0x5212);
		cmd(kSelf, kCmdSetView, 0xb28, 0x136, 0x2d);
		cmd(kSelf, kCmdPlay, 0xb28);
		cmd(kSelf, kCmdSetPos, 0xb28, 0x89, 0x79, 0x5212);
		cmd(kSelf, kCmdSetView, 0xb2a, 0x136, 0x2d);
		cmd(kSelf, kCmdPlay, 0xb2a);
		cmd(kSelf, kCmdSetPos, 0xb2a, 0x89, 0xbf, 0x5212);
		cmd(kSelf, kCmdPlay, 0xb29);
		cmd(kSelf, kCmdSetView, 0xb29, 500, 0x1e);
		cmd(kSelf, kCmdSetPos, 0xb29, 0x46, 0x19a, 0x5212);
		setGlobal32(kGlobVolumeDrag, 0);
	}

	void close() {
		if (var(0xd).get() == 0)
			return;
		static const uint16 kChildren[] = { 0x138, 0x160, 0xb2e, 0xb2f, 0xb27, 0xb28, 0xb29, 0xb2a, 0x136 };
		for (uint i = 0; i < ARRAYSIZE(kChildren); i++)
			cmd(kSelf, kCmdStop, kChildren[i]);
		restorePalette(global32(kGlobToolbarPalette));
		var(0xd).set(0);
	}

	void click(Message &msg) {
		int32 child = msg.p1;
		if (child == 0xb27 || child == 0xb28) {
			// Load or save: object 14
			if (var(0x1e).get() == 0) {
				send(kSelf, 0x1d);
				post(0xe, child == 0xb27 ? 0x28 : 0x27);
			}
		} else if (child == 0xb29) {
			send(kSelf, 0x1d);
		} else if (child == 0xb2a) {
			// Quit
			send(kSelf, 0x1d);
			post(kGameState, 0x139d);
		} else if (child >= 0x548 && child < 0x54c) {
			send(kSelf, 0x1c);
		} else if (child == 0x160) {
			setGlobal32(kGlobVolumeDrag, 1);
			setLevel(0x14f - (int16)((uint32)msg.p2 >> 16));
		} else if (child == 0x136) {
			if (msg.p3 == 1) {
				toggleMusic();
			} else if (msg.p3 == 2) {
				VarRef off = var(8);
				VarRef off2 = var(8);
				off2.set(off.get() == 0);
				cmd(kSelf, kCmdEnable, 0xb2f, var(8).get() == 0);
				cmd(kBroadcast, kCmdEnable, 0xfe74, var(8).get() == 0);
			}
		}
	}

	void toggleMusic() {
		VarRef music = var(0xb8, 4);
		VarRef music2 = var(0xb8, 4);
		music2.set(music.get() == 0);
		cmd(kSelf, kCmdEnable, 0xb2e, var(0xb8, 4).get());
		if (var(0xb8, 4).get() != 0)
			send(4, kMsgEnter, var(1, kGameState).get());
	}

	/** The sound level, 0-255, and the slider's position. */
	void setLevel(int32 level) {
		if (level < 0)
			level = 0;
		else if (level > 0xff)
			level = 0xff;
		var(0xe).set(level);
		cmd(kSelf, kCmdSetPos, 0x160, 0x1cc, 0x4f - var(0xe).get(), 0);
		setVolumeGlobal(var(0xe).get());
	}
};

// --- Object 14 ---

namespace {

/** The picture of a room in the save game list, by room index. */
uint16 roomPicture(int32 index) {
	static const uint16 kPictures[] = {
		0, 0x13a, 0x13b, 0x13c, 0x13d, 0x13e, 0x13f, 0x140, 0x141, 0x143, 0x144, 0x145,
		0x146, 0x147, 0x148, 0x149, 0x14a, 0x14b, 0x14e, 0x14f, 0x150, 0x151, 0x152,
		0x153, 0x154, 0x155, 0x157, 0x158, 0x159, 0x15a, 0x15b, 0x15c, 0x15d, 0x14c, 0, 0x14d
	};
	if (index < 0 || index >= (int32)ARRAYSIZE(kPictures))
		return 0;
	return kPictures[index];
}

/** The index of a room in the settings file (1-35; 0 for the rooms that can't be saved). */
int32 roomIndex(int32 room) {
	static const int32 kRooms[] = {
		0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1f, 0x20, 0x31, 0x37, 0x22, 0x23,
		0x24, 0x25, 0x26, 0x27, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x32, 0x33, 0x35, 0x36, 0x34,
		0x3a, 0x38, 0x39, 0x3b, 0x28, -1, 0x2a
	};
	for (int i = 0; i < (int)ARRAYSIZE(kRooms); i++)
		if (kRooms[i] == room)
			return i + 1;
	return 0;
}

/** The position of save slot 1-16, (y << 16) | x; 0 for others. */
uint32 slotPos(int32 slot) {
	if (slot < 1 || slot > 16)
		return 0;
	static const uint16 kX[4] = { 0xd, 0xac, 0x149, 0x1e7 };
	static const uint16 kY[4] = { 0xc, 0x79, 0xe7, 0x155 };
	return ((uint32)kY[(slot - 1) / 4] << 16) | kX[(slot - 1) % 4];
}

} // End of anonymous namespace

/**
 * Object 14 (vtable 0x4b5064, handler FUN_00431c40): the save and load
 * screen. 16 slots: labels 0xb33-0xb42, pictures 0xb55-0xb64; 0x15e the
 * slot area (p3 of a click: the slot), 0x156 the slot frame, 0xb53 OK,
 * 0xb54 cancel.
 *
 * Variables: 0x1e open, 0x1f saving (1) or loading (0), 0x20 the picture
 * of the room to save, 0x21 the slot chosen to load, 0x22 map slot ->
 * room index, 0x23 the slot chosen to save, 0x24 a game was started,
 * 0x25 the label format.
 *
 * Messages: 0x27 save screen, 0x28 load screen (p1, p2: a message to post
 * when it closes; p3 the cancel button's text), 0x29 close, 0x2a show the
 * screen and the saved games (result: their number), 0x2b load slot p1,
 * 0x2c save slot p1 (p2 the room index).
 */
class Object14 : public BatchAScript {
public:
	Object14(GameObject *obj) : BatchAScript(obj) {}

	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgLeave:
			if (var(0x1e).get() != 0)
				send(kSelf, 0x29);
			break;
		case 0xd:
			var(0x24).set(1);
			setEventMask(0x10000);
			var(0x1e).set(0);
			break;
		case 0x27:
			openSave(msg);
			break;
		case 0x28:
			openLoad(msg);
			break;
		case 0x29:
			cmd(kSelf, kCmdUnload, 0);
			restorePalette(global32(kGlobSaveScreenPalette));
			var(0x1e).set(0);
			postNext();
			break;
		case 0x2a:
			msg.result = open();
			break;
		case 0x2b:
			if (msg.p1 >= 1 && msg.p1 <= 0x10)
				load(msg.p1);
			break;
		case kMsgEngineLoad:
			msg.result = loadPending();
			break;
		case 0x2c:
			if (msg.p1 >= 1 && msg.p1 <= 0x10 && msg.p2 >= 1 && msg.p2 <= 0x23)
				save(msg.p1, msg.p2);
			break;
		case kMsgClick:
			click(msg);
			break;
		case kMsgKeyDown:
			if (msg.p1 == 0x74 || msg.p1 == 0x75) { // F5 load, F6 save
				if (var(0x1e).get() != 0)
					return;
				if (var(0xd, 8).get() != 0)
					return;
				send(kSelf, msg.p1 == 0x74 ? 0x28 : 0x27);
			}
			break;
		default:
			break;
		}
	}

private:
	/** Posts the message given when the screen was opened. */
	void postNext() {
		uint16 target = (uint16)global32(kGlobNextTarget);
		if (target != 0)
			post(target, (uint16)global32(kGlobNextMsg));
	}

	void setNext(const Message &msg) {
		setGlobal32(kGlobNextTarget, (uint16)msg.p1);
		setGlobal32(kGlobNextMsg, msg.p2);
	}

	/** The save folder, with a final backslash. */
	Common::String saveDir() {
		Common::String path = settings().saveDir();
		if (!endsWithBackslash(path))
			path += "\\";
		return path;
	}

	void openSave(const Message &msg) {
		setNext(msg);
		if (var(0x24).get() == 0) {
			if (var(1, kGameState).get() == 0x28 || var(1, kGameState).get() == 0x2a) {
				if ((uint16)global32(kGlobNextTarget) == 0)
					cmd(kSpeech, kCmdPlay, 0x717);
			}
		}
		if (var(0x24).get() != 0 && var(kVarBusy, kGameState).get() == 0 &&
		    var(kVarDragMode, kGameState).get() == 0 && var(0x1c, 10).get() == 0 &&
		    send(32000, kMsgWhoIsDragged) == 0 && var(0x38, 0x30).get() == 0) {
			send(kBroadcast, 0x18);
			var(0x1f).set(1);
			uint16 picture = roomPicture(roomIndex(var(1, kGameState).get()));
			if (picture == 0)
				return;
			var(0x23).set(0);
			var(0x20).set(picture);
			send(kSelf, 0x2a);
			cmd(kSelf, 0xe, 0xb53, 8, 0x692);
			cmd(kSelf, 0xe, 0xb54, 8, 0x68d);
			cmd(kSelf, kCmdLoad, picture);
			cmd(kSelf, kCmdSetCursor, 0x15e, (int32)kCursorDropNo);
			return;
		}
		postNext();
	}

	void openLoad(const Message &msg) {
		setNext(msg);
		if (var(kVarBusy, kGameState).get() != 0)
			return;
		if (var(kVarDragMode, kGameState).get() != 0)
			return;
		if (var(0x1c, 10).get() != 0)
			return;
		if (send(32000, kMsgWhoIsDragged) != 0)
			return;
		if (var(1, kGameState).get() == 0x34 && var(0x24).get() == 0)
			return;
		var(0x1f).set(0);
		var(0x21).set(0);
		if (send(kSelf, 0x2a) == 0) {
			send(kSelf, 0x29);
			return;
		}
		cmd(kSelf, 0xe, 0xb53, 8, 0x68e);
		int32 text = msg.p3 == 1 ? 0x68f : msg.p3 == 2 ? 0x691 : 0x68c;
		cmd(kSelf, 0xe, 0xb54, 8, text);
		cmd(kSelf, kCmdSetCursor, 0x15e, (int32)kCursorDropYes);
	}

	/** Shows the screen and the saved games; returns their number. */
	int32 open() {
		var(0x1e).set(1);
		setCursor(kCursorWait);
		setGlobal32(kGlobSaveScreenPalette, cmd(kSelf, kCmdUsePalette, 0x139));
		cmd(kSelf, kCmdPlay, 0x139);
		cmd(kSelf, kCmdSetZ, 0x139, 21000);
		cmd(kSelf, kCmdPlay, 0x15e);
		cmd(kSelf, kCmdSetZ, 0x15e, 0x5212);
		cmd(kSelf, kCmdPlay, 0xb53);
		cmd(kSelf, kCmdSetPos, 0xb53, 10, 0x1b8, 0x5212);
		cmd(kSelf, kCmdSetView, 0xb53, 300, 0x23);
		cmd(kSelf, kCmdPlay, 0xb54);
		cmd(kSelf, kCmdSetPos, 0xb54, 0x154, 0x1b8, 0x5212);
		cmd(kSelf, kCmdSetView, 0xb54, 300, 0x23);
		cmd(kSelf, kCmdPlay, 0x156);
		cmd(kSelf, kCmdSetZ, 0x156, 0x520b);
		cmd(kSelf, kCmdEnable, 0x156, 0);
		for (int32 slot = 1; slot <= 16; slot++) {
			uint16 label = 0xb33 + slot - 1;
			uint32 pos = slotPos(slot);
			cmd(kSelf, kCmdPlay, label);
			cmd(kSelf, kCmdSetPos, label, (int16)pos, (int16)(pos >> 16), 0x521c);
		}
		for (int32 slot = 1; slot <= 16; slot++)
			var(0x22).at(slot).set(0);

		Common::String dir = saveDir();
		GameSettings s = settings();
		int32 count = 0;
		for (int32 slot = 1; slot <= 16; slot++) {
			uint16 label = 0xb33 + slot - 1;
			cmdString(kSelf, kCmdSetText, label, "");
			Common::String name = Common::String::format("SavedGame%i", slot);
			int32 room = s.getInt(name, 0);
			Common::String file = dir + name + ".m2s";
			int32 time = s.getInt(Common::String::format("SavedGameTime%i", slot), 0);
			if (room == 0 || !fileExists(file))
				continue;
			uint32 pos = slotPos(slot);
			uint16 picture = roomPicture(room);
			if (picture == 0)
				continue;
			var(0x22).at(slot).set(room);
			// The picture child loads the room's picture (its +0x50 field).
			uint16 thumb = 0xb55 + slot - 1;
			Child *c = getChild(thumb);
			if (c)
				c->setAltResource(picture);
			uint16 id = c ? c->getId() : thumb;
			cmd(kSelf, kCmdPlay, id);
			cmd(kSelf, kCmdSetPos, id, (int16)pos, (int16)(pos >> 16), 0x520d);
			Common::String text = formatInts(varString(var(0x25)), slot, time);
			cmdString(kSelf, kCmdSetText, label, text);
			count++;
		}
		setCursor(0);
		return count;
	}

	/** OpenFile(OF_EXIST) */
	bool fileExists(const Common::String &path) {
		return g_engine->_files->exists(path);
	}

	void load(int32 slot) {
		Common::String file = saveDir() + Common::String::format("SavedGame%i", slot) + ".m2s";
		loadFile(file);
		setCursor(0);
	}

	void loadFile(const Common::String &file) {
		int32 oldRoom = var(1, kGameState).get();
		setCursor(kCursorWait);
		if (!loadGame(file))
			return;
		afterLoad(oldRoom);
	}

	/** Loads the game ScummVM's load gives (not in the original), as loadFile() does. */
	bool loadPending() {
		// Nothing to post when the screen closes: it wasn't opened.
		setGlobal32(kGlobNextTarget, 0);
		int32 oldRoom = var(1, kGameState).get();
		setCursor(kCursorWait);
		if (!g_engine->_files->loadWorld(g_engine->_pendingLoad, GameFiles::kSaveVersion)) {
			setCursor(0);
			return false;
		}
		afterLoad(oldRoom);
		setCursor(0);
		return true;
	}

	/** The end of loadFile(): leave the old room, enter the loaded one. */
	void afterLoad(int32 oldRoom) {
		int32 room = var(1, kGameState).get();
		if (cmd(0xb, kCmdIsPlaying, 0x559) == 0 && room != 0x3e && room != 0x28 &&
		    room != 0x2a && room != 0x38)
			send(kBroadcast, 0xf);
		send(kSelf, 0x29);
		if (oldRoom != 0) {
			var(1, kGameState).set(oldRoom);
			send(kBroadcast, kMsgLeave, oldRoom, room);
		}
		redrawNow();
		var(1, kGameState).set(0);
		send(kGameState, 0x14, -1);
		send(kGameState, kMsgGotoRoom, room);
		send(kBroadcast, 0x17);
	}

	void save(int32 slot, int32 roomIdx) {
		Common::String dir = saveDir();
		GameSettings s = settings();
		Common::String name = Common::String::format("SavedGame%i", slot);
		s.setInt(name, roomIdx);
		Common::String file = dir + name + ".m2s";
		s.setInt(Common::String::format("SavedGameTime%i", slot), var(5, kGameState).get());
		saveGame(file);
	}

	void click(Message &msg) {
		if (var(0x1f).get() == 0) {
			// Loading
			if (msg.p1 == 0x15e) {
				int32 slot = msg.p3;
				if (slot == 0) {
					var(0x21).set(0);
					return;
				}
				if (var(0x21).get() != 0 && var(0x22).at(var(0x21).get()).get() != 0 &&
				    slotPos(var(0x21).get()) != 0)
					cmd(kSelf, kCmdEnable, 0x156, 0);
				var(0x21).set(slot);
				if (var(0x21).get() == 0)
					return;
				if (var(0x22).at(var(0x21).get()).get() == 0)
					return;
				uint32 pos = slotPos(var(0x21).get());
				if (pos == 0)
					return;
				cmd(kSelf, kCmdSetPos, 0x156, (int16)pos - 5, (int16)(pos >> 16) - 4, 0);
				cmd(kSelf, kCmdEnable, 0x156, 1);
				return;
			}
			if (msg.p1 == 0xb53) {
				if (var(0x22).at(var(0x21).get()).get() == 0)
					return;
				send(kSelf, 0x2b, var(0x21).get());
				return;
			}
		} else {
			// Saving
			if (msg.p1 == 0x15e && var(0x20).get() != 0) {
				int32 slot = msg.p3;
				uint32 pos = slotPos(slot);
				int32 picture = var(0x20).get();
				if (picture == 0)
					return;
				int16 x = pos, y = pos >> 16;
				cmd(kSelf, kCmdPlay, 0x156);
				cmd(kSelf, kCmdSetPos, 0x156, x - 5, y - 4, 0);
				cmd(kSelf, kCmdEnable, 0x156, 1);
				cmd(kSelf, kCmdPlay, (uint16)picture);
				cmd(kSelf, kCmdSetPos, (uint16)picture, x, y, 0x5230);
				var(0x23).set(slot);
				return;
			}
			if (msg.p1 == 0xb53) {
				if (var(0x23).get() == 0)
					return;
				int32 idx = roomIndex(var(1, kGameState).get());
				send(kSelf, 0x2c, var(0x23).get(), idx);
				send(kSelf, 0x29);
				return;
			}
		}
		if (msg.p1 == 0xb54)
			send(kSelf, 0x29);
	}
};

// --- Object 17 ---

/**
 * Object 17 (vtable 0x4b5004, handler FUN_00457b70): the game state: the
 * current room and the room changes, the clock, the logo at the start
 * (0x2c2, 0x2c3), the new game picture (0xa3a), the end (0x116) and the
 * quit dialog (0x15f, 0xb30-0xb32).
 *
 * Variables: 1 room, 2 busy, 4 changing room, 5 minutes played (0-60),
 * 6 no clock, 7 quit dialog open.
 */
class Object17 : public BatchAScript {
public:
	Object17(GameObject *obj) : BatchAScript(obj) {}

	void handleMessage(Message &msg) override {
		switch (msg.msg) {
		case kMsgInit:
			init();
			break;
		case kMsgNewGame:
			var(7).set(0);
			var(0xf, 2).set(0);
			redrawNow();
			cmd(kSelf, kCmdPlay, 0xa3a);
			cmd(kSelf, kCmdUsePalette, 0xa3a);
			cmd(kSelf, kCmdSetZ, 0xa3a, 16000);
			send(kBroadcast, 0xd);
			startTimer(7, 4000, 0xe, 0x28, 0x11, 0xc, 1);
			break;
		case kMsgGotoRoom:
			if (var(kVarBusy).get() != 0)
				return;
			if (var(kVarDragMode).get() == 0) {
				setCursor(kCursorWait);
				var(kVarDragMode).set(1);
				setGlobal32(kGlobNextRoom, msg.p1);
			}
			if (var(1).get() == 0)
				send(kSelf, kMsgSwitchRoom);
			else
				send((uint16)var(1).get(), kMsgExit, msg.p1);
			break;
		case kMsgSwitchRoom: {
			int32 old = var(1).get();
			send(kBroadcast, kMsgLeave, var(1).get(), msg.p1);
			send(kGameState, 0x14, 1);
			var(1).set(global32(kGlobNextRoom));
			send(kBroadcast, kMsgEnter, global32(kGlobNextRoom), old);
			send((uint16)global32(kGlobNextRoom), kMsgEntered, old);
			break;
		}
		case 10:
			if (var(kVarDragMode).get() != 0) {
				setCursor(0);
				var(kVarDragMode).set(0);
			}
			send(kBroadcast, 0xb);
			break;
		case 0xc:
			if (var(1).get() != 0x3e && var(1).get() != 0x28 && var(1).get() != 0x2a && var(1).get() != 0x38)
				send(kBroadcast, 0xf);
			stopTimer(7);
			cmd(kSelf, kCmdUnload, 0xa3a);
			setEventMask(0x10000);
			if (var(1).get() == 0)
				send(kSelf, kMsgGotoRoom, 0x24);
			break;
		case 0x10:
			var(6).set(1);
			break;
		case 0x14:
			addMinutes(msg);
			break;
		case 0x19:
			cmd(kSelf, kCmdUsePalette, 0x116);
			cmd(kSelf, kCmdSetZ, 0x116, 120000);
			cmd(kSelf, kCmdPlay, 0x116);
			startTimer(6, 4000, kSelf, kMsgClick, 0x116);
			break;
		case kMsgClick:
			click(msg);
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0xb66) {
				cmd(kSelf, kCmdUnload, 0xb66);
				startLogo();
			}
			break;
		case kMsgSoundDone:
			if (msg.p1 == 0x2c2) {
				cmd(kSelf, kCmdUnload, 0x2c3);
				cmd(kSelf, kCmdUnload, 0x2c2);
				setCursor(kCursorWait);
				var(0xf, 2).set(1);
				post(2, kMsgStart);
			}
			break;
		case kMsgKeyDown:
			if (msg.p1 == 0x1b) // Escape
				send(kSelf, 0x139d);
			break;
		case 0x139d:
			quitDialog();
			break;
		default:
			break;
		}
	}

private:
	void startLogo() {
		cmd(kSelf, kCmdUsePalette, 0x2c3);
		cmd(kSelf, kCmdPlay, 0x2c3);
		cmd(kSelf, kCmdPlay, 0x2c2, 0xffff);
	}

	void init() {
		// The original sets var 6 (no clock) for "/notime" on the command
		// line; there is no command line here.
		// FUN_00477190 [unverified: sound parameters]
		setGlobal32(kGlobSound1, 0x5622);
		setGlobal32(kGlobSound2, 2);
		setGlobal32(kGlobSound3, 0xdc);
		int32 level = 0xf0;
		GameSettings s = settings();
		if (s.hasKey("SoundLevel"))
			level = s.getInt("SoundLevel", 0);
		else
			s.setInt("SoundLevel", level);
		var(0xe, 8).set(level);
		if (var(0xe, 8).get() < 0)
			var(0xe, 8).set(0);
		if (var(0xe, 8).get() > 0xff)
			var(0xe, 8).set(0xff);
		setVolumeGlobal(var(0xe, 8).get());
		startLogo();
	}

	void addMinutes(Message &msg) {
		if (var(6).get() != 0)
			return;
		int32 t = var(5).get() + msg.p1;
		if (t < 0 || t > 0x3c)
			return;
		msg.result = 1;
		var(5).add(msg.p1);
		send(0xc, 0x15, var(5).get());
		send(8, 0x16, var(5).get());
		if (var(5).get() == 0x3c) {
			// Time is up
			setGlobal32(kGlobNextRoom, 0x3e);
			send(kBroadcast, kMsgGotoRoom, 0x3e);
		}
	}

	void closeQuitDialog() {
		static const uint16 kChildren[] = { 0x23f, 0x15f, 0xb30, 0xb31, 0xb32 };
		for (uint i = 0; i < ARRAYSIZE(kChildren); i++)
			cmd(kSelf, kCmdUnload, kChildren[i]);
	}

	void click(Message &msg) {
		switch (msg.p1) {
		case 0xa3a:
			stopTimer(7);
			send(0xe, 0x28, 0x11, 0xc, 1);
			break;
		case 0x116: {
			// The end: save the settings and quit.
			send(kBroadcast, 0x1a);
			cmd(kBroadcast, kCmdUnload, 0);
			quitGame();
			break;
		}
		case 0x2c3:
			cmd(kSelf, kCmdUnload, 0x2c3);
			cmd(kSelf, kCmdUnload, 0x2c2);
			setCursor(kCursorWait);
			post(kSelf, kMsgNewGame);
			break;
		case 0xb31:
			// Quit: the save screen first
			closeQuitDialog();
			restorePalette(global32(kGlobQuitPalette));
			var(7).set(0);
			post(0xe, 0x27, 0x11, 0x19);
			break;
		case 0xb32:
			// Don't quit
			closeQuitDialog();
			var(7).set(0);
			restorePalette(global32(kGlobQuitPalette));
			break;
		default:
			break;
		}
	}

	void quitDialog() {
		if (var(kVarBusy).get() != 0)
			return;
		if (var(kVarDragMode).get() != 0)
			return;
		if (send(32000, kMsgWhoIsDragged) != 0)
			return;
		if (var(7).get() != 0)
			return;
		if (var(0xf, 2).get() != 0) {
			post(2, kMsgKeyDown, 0x1b);
			return;
		}
		if (cmd(kSelf, kCmdIsPlaying, 0x116) != 0)
			return;
		var(7).set(1);
		setGlobal32(kGlobQuitPalette, cmd(kSelf, kCmdUsePalette, 0x15f));
		cmd(kSelf, kCmdPlay, 0x15f);
		cmd(kSelf, kCmdSetZ, 0x15f, 110000);
		cmd(kSelf, kCmdPlay, 0xb30);
		cmd(kSelf, kCmdSetPos, 0xb30, 0x78, 0xdc, 0x1adc4);
		cmd(kSelf, kCmdSetView, 0xb30, 400, 200);
		cmd(kSelf, 0xe, 0xb30, 8, 0x690);
		cmd(kSelf, kCmdPlay, 0xb31);
		cmd(kSelf, kCmdSetPos, 0xb31, 10, 0x1b8, 0x1adc4);
		cmd(kSelf, kCmdSetView, 0xb31, 300, 0x23);
		cmd(kSelf, 0xe, 0xb31, 8, 0x68b);
		cmd(kSelf, kCmdPlay, 0xb32);
		cmd(kSelf, kCmdSetPos, 0xb32, 0x154, 0x1b8, 0x1adc4);
		cmd(kSelf, kCmdSetView, 0xb32, 300, 0x23);
		cmd(kSelf, 0xe, 0xb32, 8, 0x68a);
		cmd(kSpeech, kCmdPlay, 0x719);
	}
};

IVANOFF_SCRIPT(Object8) { return new Object8(obj); }
IVANOFF_SCRIPT(Object14) { return new Object14(obj); }
IVANOFF_SCRIPT(Object17) { return new Object17(obj); }

} // End of namespace Ivanoff
