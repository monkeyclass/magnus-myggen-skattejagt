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

// Rooms: objects that handle their messages only while the game is in
// their room (variable 1 of the game state object).

#include "common/system.h"

#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

/** Room variables. */
enum {
	kVarRoom = 1,        ///< object 17: the current room
	kVarVisited = 0x43   ///< a room: 1 once the room's intro was said
};

/**
 * The base of the rooms of this batch: accepts like an item
 * (FUN_004209d0 -> FUN_00423a60: no clicks while an item is dragged),
 * handles like the base object (FUN_004344f0).
 */
class RoomScript : public ObjectScript {
public:
	RoomScript(GameObject *obj) : ObjectScript(obj) {}

	bool accepts(Message &msg) override {
		if (var(kVarDragMode, kGameState).get() == 1 && msg.msg == kMsgClick)
			return false;
		return true;
	}

protected:
	int32 currentRoom() { return var(kVarRoom, kGameState).get(); }

	/** The common answer to kMsgEntered: say the intro once, then tell 17 the room is ready. */
	void entered(uint16 speaker, int32 text, int32 p2) {
		if (var(kVarVisited).get() == 1) {
			send(kGameState, 10);
		} else {
			var(kVarVisited).set(1);
			send(speaker, kMsgSay, text, p2);
		}
	}

	/** Stops the dragged item's sprite (variable 0x30 of the item p1). */
	void stopDragged(int32 p1) {
		uint16 item = p1;
		uint16 sprite = var(0x30, item).get();
		cmd(item, kCmdStop, sprite);
	}

	/** CTime::GetCurrentTime() (FUN_0049ebbc); only its month is used, which this returns (0-11). */
	uint32 currentTime() {
		TimeDate t;
		g_system->getTimeAndDate(t);
		return (uint32)t.tm_mon;
	}

	/** CTime(t).GetLocalTm(NULL)->tm_mon == 11 (FUN_0049ebcf) */
	bool isDecember(uint32 t) {
		return t == 11;
	}

	/** The time read at the start of the message (Object25, Object43). */
	uint32 _now = 0;
};

/** Object 23 (vtable 0x4b4f44, FUN_00403e20), room 0x17. */
class Object23 : public RoomScript {
public:
	Object23(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x17)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	bool canTalk() {
		return var(0x9000).get() == 2 && var(0x35, 0x52).at(0x17).get() == 0;
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x31);
			cmd(kSelf, kCmdPlay, 0x31);
			cmd(kSelf, kCmdSetZ, 0x31, 100);
			cmd(kSelf, kCmdPlay, 0x34);
			cmd(kSelf, kCmdSetZ, 0x34, 0x78);
			if (var(0x8b, 0x1d).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x23e);
				cmd(kSelf, kCmdSetZ, 0x23e, 0x77);
				cmd(kSelf, kCmdLoad, 0x35, 0xffff);
				cmd(kSelf, kCmdSetPos, 0x35, 0, 5, 0x78);
			}
			int32 flags = var(0x9000).get() != 2 ? (int32)0xc0003fff : (int32)0xffff3fff;
			cmd(kSelf, kCmdPlay, 0x32, 0xffff, flags, 0);
			cmd(kSelf, kCmdSetPos, 0x32, 0x1ac, 0xf5, 0x65);
			if (var(0x9000).get() == 2) {
				cmd(kSelf, kCmdSetFlags, 0x32, 0);
				cmd(kSelf, kCmdSetCursor, 0x32, (int32)0x80000003);
			} else {
				cmd(kSelf, kCmdSetFlags, 0x32, 1);
				cmd(kSelf, kCmdSetCursor, 0x32, (int32)0x80000006);
			}
			break;
		}
		case kMsgEntered:
			entered(0x10, 0x5bd05bc, 0xf);
			break;
		case 0x3b:
			if (msg.p1 == 0x31 || msg.p1 == 0x23e || (msg.p1 == 0x32 && canTalk()))
				msg.result = 1;
			break;
		case 0x3c:
			if (msg.p1 == 0x31) {
				send(kSpeech, kMsgSay, 0x5cd05cc);
			} else if (msg.p1 == 0x32) {
				if (canTalk()) {
					send(0xf, kMsgSay, 0x5c705c6);
					cmd(0x52, kCmdEnable, 0x23c, 1);
				}
			} else if (msg.p1 == 0x23e) {
				cmd(kSelf, kCmdPlay, 0x35, 0);
			}
			break;
		case 0x45:
			cmd(kSelf, kCmdResume, 0x32);
			var(0x9000).set(2);
			cmd(kSelf, kCmdSetCursor, 0x32, (int32)0x80000003);
			break;
		case kMsgClick:
			if (msg.p1 == 0x32) {
				if (var(0x9000).get() == 1)
					send(0x3c, 0x44);
			} else if (msg.p1 == 0x34) {
				send(kGameState, kMsgGotoRoom, 0x37);
			} else if (msg.p1 == 0x23e) {
				cmd(kSelf, kCmdPlay, 0x35, 0xffff);
			}
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x32)
				send(0xf, kMsgSay, 0x5c305c2, 0x10);
			else if (msg.p1 == 0x35)
				send(kGameState, kMsgGotoRoom, 0x37);
			break;
		default:
			break;
		}
	}
};

/** Object 24 (vtable 0x4b4f24, FUN_004050b0), room 0x18. */
class Object24 : public RoomScript {
public:
	Object24(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x18) {
			switch (msg.msg) {
			case kMsgEntered:
				entered(0xf, 0x5d005cf, 0x10);
				break;
			case kMsgLeave:
				cmd(kSelf, kCmdUnload, 0);
				break;
			case kMsgExit:
				send(kGameState, kMsgSwitchRoom);
				break;
			case kMsgEnter:
				cmd(kSelf, kCmdUsePalette, 0x3d);
				cmd(kSelf, kCmdPlay, 0x3d);
				cmd(kSelf, kCmdSetZ, 0x3d, 100);
				cmd(kSelf, kCmdPlay, 0x3e);
				cmd(kSelf, kCmdSetZ, 0x3e, 0x65);
				cmd(kSelf, kCmdPlay, 0x4d);
				cmd(kSelf, kCmdSetZ, 0x4d, 0x66);
				break;
			case 0x3b:
				if (msg.p1 == 0x4d)
					msg.result = 1;
				break;
			case 0x3c:
				if (msg.p1 == 0x4d)
					send(kSpeech, kMsgSay, 0x5e205e1);
				break;
			case kMsgClick:
				if (msg.p1 == 0x3e)
					send(kGameState, kMsgGotoRoom, 0x2c);
				break;
			default:
				break;
			}
		}
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}
};

/** Object 25 (vtable 0x4b4f04, FUN_00407440), room 0x19. */
class Object25 : public RoomScript {
public:
	Object25(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		_now = currentTime();
		if (currentRoom() == 0x19)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			// In December, a Christmas animation before leaving.
			if (isDecember(_now) && var(0x8d, 0x1d).get() != 0) {
				cmd(kSelf, kCmdPlay, 100, 0xffff);
				cmd(kSelf, kCmdPlay, 99);
				break;
			}
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdUsePalette, 0x51);
			cmd(kSelf, kCmdPlay, 0x51);
			cmd(kSelf, kCmdSetZ, 0x51, 100);
			cmd(kSelf, kCmdPlay, 0x52);
			cmd(kSelf, kCmdSetZ, 0x52, 0x65);
			cmd(kSelf, kCmdPlay, 0x53);
			cmd(kSelf, kCmdSetZ, 0x53, 0x65);
			cmd(kSelf, kCmdPlay, 0x50);
			cmd(kSelf, kCmdSetZ, 0x50, 0x65);
			cmd(kSelf, kCmdPlay, 0x54, 0xffff, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0x54, 0xfe, 0x11b, 0x73);
			cmd(kSelf, kCmdLoad, 100, 0xffff, (int32)0xc000c001);
			cmd(kSelf, kCmdSetPos, 100, 0x34, 0x20, 0x65);
			cmd(kSelf, kCmdPlay, 0x56, 0xffff, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0x56, 0, 0x53, 0x65);
			cmd(kSelf, kCmdLoad, 0x58, 0xffff);
			cmd(kSelf, kCmdSetPos, 0x58, 0x1da, 0, 0x78);
			break;
		case kMsgEntered:
			if (var(kVarVisited).get() == 1) {
				send(kGameState, 10);
			} else if (var(0x8d, 0x1d).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x58, 0xffff);
			} else {
				var(kVarVisited).set(1);
				send(0xf, kMsgSay, 0x5e605e5, 0x10);
			}
			break;
		case 0x3b:
			if ((msg.p1 > 0x4f && msg.p1 < 0x52) || msg.p1 == 0x56)
				msg.result = 1;
			break;
		case 0x3c:
			if (msg.p1 == 0x50 || msg.p1 == 0x56) {
				send(kSpeech, kMsgSay, 0x5f605f5);
			} else if (msg.p1 == 0x51) {
				int32 state = var(0x44).get();
				if (state == 0)
					send(kSpeech, kMsgSay, 0x5f805f7);
				else if (state == 1)
					send(kSpeech, kMsgSay, 0x5fa05f9);
			}
			break;
		case kMsgClick:
			switch (msg.p1) {
			case 0x50:
			case 0x56:
				if (var(kVarBusy, kGameState).get() == 0) {
					cmd(kSelf, kCmdResume, 0x56, (int32)0x80000001);
					cmd(kSelf, kCmdPlay, 0x57);
				}
				break;
			case 0x52:
				send(kGameState, kMsgGotoRoom, 0x1a);
				break;
			case 0x53:
				send(kGameState, kMsgGotoRoom, 0x1c);
				break;
			case 0x54:
				cmd(kSelf, kCmdResume, 0x54);
				cmd(kSelf, kCmdPlay, 0x55);
				break;
			default:
				break;
			}
			break;
		case kMsgAnimDone:
			switch (msg.p1) {
			case 0x56:
				cmd(0xf, kCmdStop, 0x5e);
				cmd(0xf, kCmdPlay, 0x59);
				cmd(0x10, kCmdPlay, 0x60);
				send(0x10, kMsgSay, 0x5f005ef);
				break;
			case 0x58:
				send(kGameState, 10);
				send(kGameState, kMsgGotoRoom, 0x1c);
				break;
			case 0x5b:
				cmd(0xf, kCmdStop, 0x5b);
				cmd(0xf, kCmdPlay, 0x5d);
				break;
			case 0x5c:
				cmd(0xf, kCmdStop, 0x5c);
				cmd(0xf, kCmdPlay, 0x5e);
				break;
			case 100:
				send(0xf, kMsgSay, 0x5f205f1, 0x10);
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

/** Object 27 (vtable 0x4b4ec4, FUN_0040abc0), room 0x1b (and its part of room 0x25). */
class Object27 : public RoomScript {
public:
	Object27(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		int32 room = currentRoom();
		if (room == 0x1b)
			handleRoom(msg);
		else if (room == 0x25)
			handleRoom25(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	static bool isKey(int32 p1) {
		int16 sprite = (int16)((uint32)p1 >> 16);
		return sprite == 0x2d0 || sprite == 0x2d3 || sprite == 0x2d6;
	}

	void handleRoom25(Message &msg) {
		if (msg.msg == 0x38) {
			cmd(kSelf, kCmdUsePalette, 0xe9);
			cmd(kSelf, kCmdPlay, 0xe9);
			cmd(kSelf, kCmdSetZ, 0xe9, 300);
			cmd(kSelf, kCmdPlay, 0xe4, 0xffff, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0xe4, 0x94, 0x8a, 0x12f);
			cmd(kSelf, kCmdLoad, 0xf5, 0x25, (int32)0x8000c001);
			cmd(kSelf, kCmdSetPos, 0xf5, 0x101, 0, 0x133);
			cmd(0xf, kCmdPlay, 0xed, 0x1b);
			cmd(0xf, kCmdSetPos, 0xed, 0x60, 0xee, 0x154);
			cmd(0x10, kCmdPlay, 0xf3, 0x1b);
			cmd(0x10, kCmdSetPos, 0xf3, 0xb, 0x4d, 0x155);
		} else if (msg.msg == 0x39) {
			cmd(kSelf, kCmdUnload, 0xe9);
			cmd(kSelf, kCmdUnload, 0xea);
			cmd(kSelf, kCmdUnload, 0xeb);
			cmd(kSelf, kCmdUnload, 0xe5);
			cmd(kSelf, kCmdUnload, 0xe7);
			cmd(kSelf, kCmdUnload, 0xe6);
			cmd(kSelf, kCmdUnload, 0xe4);
			cmd(kSelf, kCmdUnload, 0xf5);
			cmd(0xf, kCmdUnload, 0xed);
			cmd(0x10, kCmdUnload, 0xf3);
		} else if (msg.msg == kMsgAnimDone && msg.p1 == 0xed) {
			cmd(kSelf, kCmdPlay, 0xf5, 0x25);
		}
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			if (msg.p1 == 0x25)
				send(0xf, 0x2f, 0xf0, 0xec001b, (int32)0x8000c001);
			else
				send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0xe9);
			cmd(kSelf, kCmdPlay, 0xe9);
			cmd(kSelf, kCmdSetZ, 0xe9, 100);
			cmd(kSelf, kCmdPlay, 0xe5);
			cmd(kSelf, kCmdSetZ, 0xe5, 0x65);
			cmd(kSelf, kCmdLoad, 0xe7);
			cmd(kSelf, kCmdSetZ, 0xe7, 0x68);
			cmd(kSelf, kCmdPlay, 0xeb);
			cmd(kSelf, kCmdSetZ, 0xeb, 0x66);
			cmd(kSelf, kCmdLoad, 0xea);
			cmd(kSelf, kCmdSetZ, 0xea, 0x66);
			int32 command;
			if (var(0x61).get() == 0) {
				cmd(kSelf, kCmdPlay, 0xe7);
				cmd(kSelf, kCmdPlay, 0xe6, 0xffff, (int32)0xc0003fff);
				cmd(kSelf, kCmdSetPos, 0xe6, 0x94, 0x8a, 0x67);
				command = kCmdLoad;
			} else {
				cmd(kSelf, kCmdPlay, 0xea);
				cmd(kSelf, kCmdLoad, 0xe6, 0xffff, (int32)0xc0003fff);
				cmd(kSelf, kCmdSetPos, 0xe6, 0x94, 0x8a, 0x67);
				command = kCmdPlay;
			}
			cmd(kSelf, command, 0xe4, 0xffff, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0xe4, 0x94, 0x8a, 0x67);
			cmd(kSelf, kCmdLoad, 0xf5);
			cmd(kSelf, kCmdSetPos, 0xf5, 0x101, 0, 0x6b);
			break;
		}
		case kMsgEntered:
			entered(0xf, 0x6260625, 0x10);
			break;
		case 0x30:
			if (msg.p2 == 0xf0) {
				cmd(0xf, kCmdStop, 0xec);
				send(kGameState, kMsgSwitchRoom);
			}
			break;
		case 0x3b:
			if (msg.p1 == 0xe5 || msg.p1 == 0xe7 || msg.p1 == 0xe9)
				msg.result = 1;
			break;
		case 0x3c:
			if (msg.p1 == 0xe5) {
				int32 state = var(0x45).get();
				if (state == 0)
					send(kSpeech, kMsgSay, 0x6360635);
				else if (state == 1)
					send(kSpeech, kMsgSay, 0x63a0639);
			} else if (msg.p1 == 0xe7) {
				int32 state = var(0x46).get();
				if (state == 0)
					send(0x10, kMsgSay, 0x62c062b);
				else if (state == 1)
					send(0x10, kMsgSay, 0x6380637);
				else if (state == 2)
					send(0x10, kMsgSay, 0x6320631);
			} else if (msg.p1 == 0xe9) {
				int32 state = var(0x44).get();
				if (state == 0)
					send(kSpeech, kMsgSay, 0x6340633);
				else if (state == 1)
					send(kSpeech, kMsgSay, 0x63c063b);
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0xea)
				send(kGameState, kMsgGotoRoom, 0x25);
			else if (msg.p1 == 0xeb)
				send(kGameState, kMsgGotoRoom, 0x26);
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0xe4) {
				send(0xf, kMsgSay, 0x62e062d);
				cmd(kSelf, kCmdStop, 0xea);
				cmd(kSelf, kCmdPlay, 0xe7);
				var(0x61).set(0);
			} else if (msg.p1 == 0xe6) {
				send(0xf, kMsgSay, 0x62e062d);
				cmd(kSelf, kCmdPlay, 0xea);
				cmd(kSelf, kCmdStop, 0xe7);
				var(0x61).set(1);
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0xe7 && isKey(msg.p1))
				msg.result = 1;
			break;
		case kMsgDrop:
			if (isKey(msg.p1)) {
				stopDragged(msg.p1);
				setCursor(0);
				if (msg.p2 == 0xe7) {
					var(0x2d, (uint16)msg.p1).set(0);
					var(0x31, (uint16)msg.p1).set(1);
					send(0xf, kMsgSay, 0x630062f);
					cmd(kSelf, kCmdPlay, 0xe8);
				}
			}
			if (var(0x31, 0x43).get() != 0 && var(0x31, 0x44).get() != 0 && var(0x31, 0x45).get() != 0) {
				cmd(kSelf, kCmdResume, 0xe6);
				cmd(kSelf, kCmdPlay, 0x199);
			}
			break;
		default:
			break;
		}
	}
};

/** Object 28 (vtable 0x4b4ea4, FUN_0040bcb0), room 0x1c. */
class Object28 : public RoomScript {
public:
	Object28(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x1c)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void resumeAndPlay(uint16 resume, uint16 play, int32 p1 = 0) {
		cmd(kSelf, kCmdResume, resume, p1);
		cmd(kSelf, kCmdPlay, play);
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			if (msg.p1 == 0x19)
				send(0xf, 0x2f, 0x106, 0x104001c, (int32)0x8000c001);
			else
				send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0xf9);
			cmd(kSelf, kCmdPlay, 0xf9);
			cmd(kSelf, kCmdSetZ, 0xf9, 100);
			cmd(kSelf, kCmdPlay, 0x101);
			cmd(kSelf, kCmdSetZ, 0x101, 0x65);
			cmd(kSelf, kCmdPlay, 0xf8);
			cmd(kSelf, kCmdSetZ, 0xf8, 0x65);
			cmd(kSelf, kCmdPlay, 0xff);
			cmd(kSelf, kCmdSetZ, 0xff, 0x65);
			cmd(kSelf, kCmdPlay, 0x10a);
			cmd(kSelf, kCmdSetZ, 0x10a, 0x65);
			int32 flags = var(0xaa).get() != 2 ? (int32)0xc0003fff : (int32)0xffff3fff;
			cmd(kSelf, kCmdPlay, 0xfa, 0, flags, 0);
			cmd(kSelf, kCmdSetPos, 0xfa, 0x1bc, 0xee, 0x70);
			cmd(kSelf, kCmdPlay, 0xfc, 0xffff, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0xfc, 0x1bf, 0x7f, 0x70);
			int32 command = var(0xa9).get() != 2 ? kCmdLoad : kCmdPlay;
			cmd(kSelf, command, 0xfd, 0xffff, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0xfd, 0x12, 0x14f, 0x70);
			command = var(0xa9).get() != 1 ? kCmdLoad : kCmdPlay;
			cmd(kSelf, command, 0xfe, 0xffff, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0xfe, 0x12, 0x14f, 0x70);
			cmd(kSelf, kCmdPlay, 0x102, 0, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0x102, 10, 0x17, 0x74);
			cmd(kSelf, kCmdPlay, 0x10c, 0, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0x10c, 0xfd, 0, 0x73);
			cmd(kSelf, kCmdPlay, 0x10e, 0, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0x10e, 0x250, 0x5a, 0x73);
			cmd(kSelf, kCmdPlay, 0x110, 0, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0x110, 0x18d, 0x161, 0x74);
			break;
		}
		case kMsgEntered:
			entered(0xf, 0x640063f, 0x10);
			break;
		case 0x2e:
			if (msg.p2 == 0x66d) {
				send(10, 0x1f);
				send(0x1e, 0x49);
			}
			break;
		case 0x30:
			if (msg.p2 == 0x106) {
				cmd(0xf, kCmdStop, 0x104);
				send(kGameState, kMsgSwitchRoom);
			}
			break;
		case 0x3b:
			if (msg.p1 == 0xf9 || msg.p1 == 0xfc)
				msg.result = 1;
			break;
		case 0x3c:
			if (msg.p1 == 0xf9)
				send(kSpeech, kMsgSay, 0x66a0669);
			else if (msg.p1 == 0xfc)
				send(kSpeech, kMsgSay, 0x66c066b);
			break;
		case 0x4a:
			if (var(0x2d, 0x47).get() == 0 && var(0x31, 0x47).get() == 0) {
				cmd(kSelf, kCmdStop, 0xfc);
				cmd(kSelf, kCmdPlay, 0xfc);
				send(0xf, kMsgSay, 0x6480647, 0x10);
			} else {
				cmd(kSelf, kCmdStop, 0xfc);
				cmd(kSelf, kCmdPlay, 0xfc, 0xffff, (int32)0xc0003fff);
			}
			break;
		case kMsgClick:
			switch (msg.p1) {
			case 0xf8:
				send(kGameState, kMsgGotoRoom, 0x19);
				break;
			case 0xfa:
				if (var(0xaa).get() == 1) {
					var(0xaa).set(2);
					resumeAndPlay(0xfa, 0xfb);
				}
				if (var(0xaa).get() == 0)
					cmd(kSelf, kCmdPlay, 0x198);
				break;
			case 0xfc:
				send(0x10, kMsgSay, 0x66e066d, 0x1c);
				break;
			case 0xfd:
				resumeAndPlay(0xfd, 0xfb);
				break;
			case 0xfe:
				resumeAndPlay(0xfe, 0xfb);
				break;
			case 0xff:
				cmd(kSelf, kCmdPlay, 0x100);
				break;
			case 0x101:
				send(kGameState, kMsgGotoRoom, 0x31);
				break;
			case 0x102:
				resumeAndPlay(0x102, 0x103, (int32)0x80000001);
				break;
			case 0x10a:
				cmd(kSelf, kCmdPlay, 0x10b);
				break;
			case 0x10c:
				resumeAndPlay(0x10c, 0x10d, (int32)0x80000001);
				break;
			case 0x10e:
				resumeAndPlay(0x10e, 0x10f, (int32)0x80000001);
				break;
			case 0x110:
				resumeAndPlay(0x110, 0x111, (int32)0x80000001);
				break;
			default:
				break;
			}
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0xfd) {
				cmd(kSelf, kCmdStop, 0xfd);
				cmd(kSelf, kCmdPlay, 0xfe, 0xffff, (int32)0xc0003fff);
				var(0xa9).set(1);
			} else if (msg.p1 == 0xfe) {
				if (var(0x2d, 0x58).get() == 0 && var(0x31, 0x58).get() == 0)
					cmd(0x58, kCmdPlay, 0x307);
				var(0xac).set(1);
				var(0xa9).set(2);
				cmd(kSelf, kCmdStop, 0xfe);
				cmd(kSelf, kCmdPlay, 0xfd, 0xffff, (int32)0xc0003fff);
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0xfa) {
				int16 sprite = (int16)((uint32)msg.p1 >> 16);
				msg.result = (sprite == 0x2dc || sprite == 0x2db) ? 1 : 0;
			}
			break;
		case kMsgDrop: {
			int16 sprite = (int16)((uint32)msg.p1 >> 16);
			if (sprite == 0x2dc || (sprite == 0x2db && msg.p2 == 0xfa)) {
				stopDragged(msg.p1);
				setCursor(0);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x31, (uint16)msg.p1).set(1);
				var(0xaa).set(1);
				cmd(kSelf, kCmdPlay, 0x19a);
			}
			break;
		}
		default:
			break;
		}
	}
};

/** Object 29 (vtable 0x4b4e84, FUN_004104a0), room 0x1d. */
class Object29 : public RoomScript {
public:
	Object29(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x1d)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	/** The five places (hotspots) and the item each one takes. */
	static int32 takes(int32 place) {
		switch (place) {
		case 0x11a: return 799;
		case 0x11c: return 0x2de;
		case 0x11e: return 0x2d9;
		case 0x120: return 0x2e4;
		case 0x122: return 0x378;
		default: return -1;
		}
	}

	/** The dragged item p1 is used up. */
	void useUp(int32 p1) {
		stopDragged(p1);
		setCursor(0);
	}

	void placeItem(Message &msg, uint16 flag, uint16 anim) {
		useUp(msg.p1);
		var(flag).set(1);
		var(0x2d, (uint16)msg.p1).set(0);
		var(0x31, (uint16)msg.p1).set(1);
		cmd(kSelf, kCmdPlay, anim);
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x124);
			cmd(kSelf, kCmdPlay, 0x124);
			cmd(kSelf, kCmdSetZ, 0x124, 100);
			cmd(kSelf, kCmdPlay, 0x33b);
			cmd(kSelf, kCmdSetPos, 0x33b, 0x25f, 0x91, 0x66);
			if (var(0x89).get() != 0) {
				cmd(kSelf, kCmdPlay, 0x125);
				cmd(kSelf, kCmdSetZ, 0x125, 200);
				cmd(kSelf, kCmdPlay, 0x33a);
				cmd(kSelf, kCmdSetPos, 0x33a, 0x25f, 0x91, 0xc9);
			}
			cmd(kSelf, kCmdLoad, 0x12f);
			cmd(kSelf, kCmdSetPos, 0x12f, 0, 0, 0x6a);
			cmd(kSelf, kCmdLoad, 0x126);
			cmd(kSelf, kCmdSetPos, 0x126, 0, 0xd, 0x6b);
			cmd(kSelf, kCmdLoad, 0x131);
			cmd(kSelf, kCmdSetPos, 0x131, 0xc9, 0, 0x6c);
			cmd(kSelf, kCmdLoad, 0x117);
			cmd(kSelf, kCmdSetPos, 0x117, 0x14b, 199, 0x6b);
			cmd(kSelf, kCmdLoad, 0x134);
			cmd(kSelf, kCmdSetPos, 0x134, 0x1f2, 0, 0x6a);
			playPlace(0x8a, 0x11a);
			cmd(kSelf, kCmdSetPos, 0x11a, 0x17, 0xd2, 0x69);
			playPlace(0x8b, 0x11c);
			cmd(kSelf, kCmdSetPos, 0x11c, 0x93, 0xcd, 0x6a);
			playPlace(0x8c, 0x11e);
			cmd(kSelf, kCmdSetPos, 0x11e, 0x109, 0xcf, 0x6b);
			playPlace(0x8d, 0x120);
			cmd(kSelf, kCmdSetPos, 0x120, 0x164, 0xd6, 0x6a);
			playPlace(0x8e, 0x122);
			cmd(kSelf, kCmdSetPos, 0x122, 0x1b8, 0xd3, 0x69);
			cmd(kSelf, kCmdPlay, 0x133);
			cmd(kSelf, kCmdSetZ, 0x133, 0xcd);
			cmd(kSelf, kCmdPlay, 0x130);
			cmd(kSelf, kCmdSetZ, 0x130, 0x69);
			cmd(kSelf, kCmdPlay, 0x127);
			cmd(kSelf, kCmdSetZ, 0x127, 0x69);
			cmd(kSelf, kCmdPlay, 0x132);
			cmd(kSelf, kCmdSetZ, 0x132, 0x69);
			cmd(kSelf, kCmdPlay, 0x118);
			cmd(kSelf, kCmdSetZ, 0x118, 0x69);
			cmd(kSelf, kCmdPlay, 0x135);
			cmd(kSelf, kCmdSetZ, 0x135, 0x69);
			break;
		}
		case kMsgEntered:
			entered(0xf, 0x6710670, 0x10);
			break;
		case 0x3b:
			switch (msg.p1) {
			case 0x130:
			case 0x118:
			case 0x124:
			case 0x127:
			case 0x132:
			case 0x135:
			case 0x33a:
				msg.result = 1;
				break;
			default:
				break;
			}
			break;
		case 0x3c:
			switch (msg.p1) {
			case 0x130:
				send(kSpeech, kMsgSay, 0x67f067e);
				break;
			case 0x118:
				send(kSpeech, kMsgSay, 0x6810680);
				break;
			case 0x124: {
				int32 state = var(0x44).get();
				if (state == 0)
					send(kSpeech, kMsgSay, 0x6870686);
				else if (state == 1)
					send(kSpeech, kMsgSay, 0x6890688);
				break;
			}
			case 0x127:
				send(kSpeech, kMsgSay, 0x6830682);
				break;
			case 0x132:
				send(kSpeech, kMsgSay, 0x67d067c);
				break;
			case 0x135:
				send(kSpeech, kMsgSay, 0x6850684);
				break;
			case 0x33a:
				send(kSpeech, kMsgSay, 0x67b067a);
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x133)
				send(kGameState, kMsgGotoRoom, 0x35);
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt: {
			uint32 item = (uint32)msg.p1 >> 16;
			if (msg.p2 == 0x33a) {
				if (var(0x89).get() == 1)
					msg.result = item == 0x389;
				if (var(0x89).get() == 2)
					msg.result = item == 0x319;
			} else if (msg.p2 < 0x33a) {
				int32 wanted = takes(msg.p2);
				if (wanted >= 0)
					msg.result = item == (uint32)wanted;
			}
			break;
		}
		case kMsgDrop: {
			uint32 item = (uint32)msg.p1 >> 16;
			if (msg.p2 == 0x33a) {
				if (item == 0x389 && var(0x89).get() == 1) {
					useUp(msg.p1);
					var(0x89).set(2);
					var(0x2d, (uint16)msg.p1).set(0);
					var(0x31, (uint16)msg.p1).set(1);
				}
				if (item == 0x319 && var(0x89).get() == 2) {
					useUp(msg.p1);
					var(0x89).set(0);
					var(0x2d, (uint16)msg.p1).set(0);
					var(0x31, (uint16)msg.p1).set(1);
					cmd(kSelf, kCmdUnload, 0x125);
					cmd(kSelf, kCmdUnload, 0x33a);
					cmd(0xf, kCmdUnload, 0x129);
					cmd(0x10, kCmdUnload, 0x12d);
					var(0x10, 0xf).set(0x128);
					var(0x11, 0xf).set(0x12a);
					cmd(0xf, kCmdPlay, 0x128);
					var(0x10, 0x10).set(300);
					var(0x11, 0x10).set(0x12e);
					cmd(0x10, kCmdPlay, 300);
				}
			} else if (msg.p2 < 0x33a && takes(msg.p2) >= 0 && item == (uint32)takes(msg.p2)) {
				switch (msg.p2) {
				case 0x11a:
					placeItem(msg, 0x8a, 0x12f);
					break;
				case 0x11c:
					placeItem(msg, 0x8b, 0x126);
					break;
				case 0x11e:
					placeItem(msg, 0x8c, 0x131);
					break;
				case 0x120:
					placeItem(msg, 0x8d, 0x117);
					break;
				case 0x122:
					placeItem(msg, 0x8e, 0x134);
					break;
				default:
					break;
				}
			}
			break;
		}
		default:
			break;
		}
	}

	/** A place: hidden while empty, shown (and clickable) once its item is there. */
	void playPlace(uint16 flag, uint16 place) {
		int32 flags = var(flag).get() != 0 ? (int32)0xffff3fff : (int32)0xc0003fff;
		cmd(kSelf, kCmdPlay, place, 0xffff, flags, 0);
	}
};

/** Object 31 (vtable 0x4b4e44, FUN_00412c80), room 0x1f. */
class Object31 : public RoomScript {
public:
	Object31(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x1f)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void leaveTo24() {
		send(kGameState, 10);
		send(kGameState, kMsgGotoRoom, 0x24);
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			if (msg.p1 == 0x20)
				send(0xf, 0x2f, 0x184, 0x182001f, (int32)0x8000c001);
			else
				send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x177);
			cmd(kSelf, kCmdPlay, 0x177);
			cmd(kSelf, kCmdPlay, 0x386);
			cmd(kSelf, kCmdSetPos, 0x386, 0x251, 0x112, 0x65);
			int32 command = var(0x31, 0x61).get() != 0 ? kCmdPlay : kCmdLoad;
			cmd(kSelf, command, 0x33c);
			cmd(kSelf, kCmdSetPos, 0x33c, 0x241, 199, 0x65);
			cmd(kSelf, kCmdLoad, 0x17a);
			cmd(kSelf, kCmdSetPos, 0x17a, 0x171, 0x134, 0x66);
			cmd(kSelf, kCmdLoad, 0x179);
			cmd(kSelf, kCmdSetPos, 0x179, 0x171, 0x134, 0x66);
			cmd(kSelf, kCmdPlay, 0x180);
			cmd(kSelf, kCmdSetZ, 0x180, 0x66);
			cmd(kSelf, kCmdPlay, 0x18d);
			cmd(kSelf, kCmdSetZ, 0x18d, 0x66);
			cmd(kSelf, kCmdPlay, 0x181);
			cmd(kSelf, kCmdSetZ, 0x181, 0x66);
			break;
		}
		case kMsgEntered:
			if (var(kVarVisited).get() == 1) {
				if (var(0x8e, 0x1d).get() == 0) {
					cmd(0x14, kCmdPlay, 0x17c);
					cmd(kSelf, kCmdPlay, 0x17a);
					var(0x11, 0xf).set(0x189);
					cmd(0xf, kCmdUnload, 0x182);
					cmd(0x10, kCmdUnload, 0x18b);
					send(0xf, kMsgSay, 0x6990698, 0x1f);
				} else {
					send(kGameState, 10);
				}
			} else {
				var(kVarVisited).set(1);
				if (var(0x8e, 0x1d).get() == 0) {
					send(0xf, kMsgSay, 0x6950694);
					cmd(0x14, kCmdPlay, 0x17b, 0x14);
				} else {
					send(kGameState, 10);
				}
			}
			break;
		case 0x23:
			send(0x10, kMsgSay, 0x69d069c);
			break;
		case 0x2e:
			if (msg.p2 == 0x698) {
				cmd(0xf, kCmdUnload, 0x182);
				leaveTo24();
			}
			break;
		case 0x30:
			if (msg.p2 == 0x17d)
				leaveTo24();
			else if (msg.p2 == 0x184)
				send(kGameState, kMsgSwitchRoom);
			break;
		case 0x3b:
			if (msg.p1 == 0x18d || msg.p1 == 0x177 || msg.p1 == 0x181 || msg.p1 == 0x386)
				msg.result = 1;
			break;
		case 0x3c:
			if (msg.p1 == 0x18d) {
				send(kSpeech, kMsgSay, 0x6a906a8);
			} else if (msg.p1 == 0x177) {
				send(kSpeech, kMsgSay, 0x6ab06aa);
			} else if (msg.p1 == 0x181) {
				send(kSpeech, kMsgSay, 0x6f806f7);
			} else if (msg.p1 == 0x386) {
				if (var(0x31, 0x61).get() == 0) {
					int32 state = var(0x44).get();
					if (state != 0) {
						if (state == 1)
							send(kSpeech, kMsgSay, 0x6a506a4);
						break;
					}
				}
				send(kSpeech, kMsgSay, 0x69f069e);
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x180)
				send(kGameState, kMsgGotoRoom, 0x24);
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x386)
				msg.result = (uint32)msg.p1 >> 16 == 0x33f;
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x33f && msg.p2 == 0x386) {
				stopDragged(msg.p1);
				setCursor(0);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x31, (uint16)msg.p1).set(1);
				cmd(kSelf, kCmdPlay, 0x33c);
				cmd(0x5a, kCmdPlay, 0x30a);
				cmd(kSelf, kCmdPlay, 0x18e);
				send(kSpeech, kMsgSay, 0x6a706a6);
			}
			break;
		default:
			break;
		}
	}
};

/** Object 32 (vtable 0x4b4e24, FUN_004134e0), room 0x20. */
class Object32 : public RoomScript {
public:
	Object32(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x20) {
			switch (msg.msg) {
			case kMsgEntered:
				entered(0xf, 0x6ad06ac, 0x10);
				break;
			case kMsgLeave:
				cmd(kSelf, kCmdUnload, 0);
				break;
			case kMsgExit:
				send(kGameState, kMsgSwitchRoom);
				break;
			case kMsgEnter:
				cmd(kSelf, kCmdUsePalette, 0x191);
				cmd(kSelf, kCmdPlay, 0x191);
				cmd(kSelf, kCmdSetZ, 0x191, 100);
				cmd(kSelf, kCmdPlay, 400);
				cmd(kSelf, kCmdSetZ, 400, 100);
				cmd(kSelf, kCmdPlay, 399);
				cmd(kSelf, kCmdSetZ, 399, 100);
				cmd(kSelf, kCmdPlay, 0x192);
				cmd(kSelf, kCmdSetZ, 0x192, 100);
				break;
			case 0x3b:
				if (msg.p1 == 399 || msg.p1 == 0x191 || msg.p1 == 0x192)
					msg.result = 1;
				break;
			case 0x3c:
				if (msg.p1 == 399)
					send(kSpeech, kMsgSay, 0x6fc06fb);
				else if (msg.p1 == 0x191)
					send(kSpeech, kMsgSay, 0x6bb06ba);
				else if (msg.p1 == 0x192)
					send(kSpeech, kMsgSay, 0x6fe06fd);
				break;
			case kMsgClick:
				if (msg.p1 == 400)
					send(kGameState, kMsgGotoRoom, 0x1f);
				break;
			default:
				break;
			}
		}
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}
};

/** Object 34 (vtable 0x4b4de4, FUN_0041d240), room 0x22. */
class Object34 : public RoomScript {
public:
	Object34(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x22)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	/** Timer 2: after var 0x4d seconds, play animation 0x252 (stopping 0x251). */
	void startAnimTimer() {
		int32 seconds = var(0x4d).get();
		startTimer(2, seconds * 1000, kSelf, 0x2f, 0x252, 0x251ffff, (int32)0x8000c001);
	}

	void openSprites() {
		cmd(0x51, kCmdSetFlags, 0x2f4, 0x100);
		cmd(0x51, kCmdSetCursor, 0x2f4, (int32)0x80000007);
		cmd(0x50, kCmdSetFlags, 0x2f1, 0x100);
	}

	static bool isKey(int32 p1) {
		int16 sprite = (int16)((uint32)p1 >> 16);
		return sprite == 0x2f5 || sprite == 0x2f4;
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			stopTimer(2);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x242);
			cmd(kSelf, kCmdPlay, 0x242);
			cmd(kSelf, kCmdSetZ, 0x242, 100);
			cmd(kSelf, kCmdPlay, 0x243);
			cmd(kSelf, kCmdSetZ, 0x243, 0x65);
			cmd(kSelf, kCmdLoad, 0x253);
			cmd(kSelf, kCmdSetPos, 0x253, 0x15, 0xf2, 0x68);
			int32 cursor;
			if (var(0x4b).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x251);
				cmd(kSelf, kCmdSetPos, 0x251, 0x15, 0xfc, 0x69);
				cmd(kSelf, kCmdLoad, 0x252);
				cmd(kSelf, kCmdSetPos, 0x252, 0x15, 0xef, 0x6a);
				cmd(kSelf, kCmdLoad, 0x250);
				cmd(kSelf, kCmdSetPos, 0x250, 0x15, 0xfc, 0x6b);
				cmd(kSelf, kCmdPlay, 0x255, 0, (int32)0xc000c001);
				cmd(kSelf, kCmdSetPos, 0x255, 0x35, 0x33, 0x6c);
				cmd(kSelf, kCmdPlay, 0x240);
				cmd(kSelf, kCmdSetZ, 0x240, 0x65);
				cmd(kSelf, kCmdLoad, 0x241);
				cmd(0x51, kCmdSetFlags, 0x2f4, 0);
				cmd(0x51, kCmdSetCursor, 0x2f4, 0);
				cmd(0x50, kCmdSetFlags, 0x2f1, 0);
				cursor = 0;
			} else {
				cmd(kSelf, kCmdPlay, 0x253, 0, (int32)0xffff3fff);
				cmd(kSelf, kCmdPlay, 0x241);
				openSprites();
				cursor = (int32)0x80000007;
			}
			cmd(0x50, kCmdSetCursor, 0x2f1, cursor);
			cmd(kSelf, kCmdSetZ, 0x241, 0x66);
			cmd(kSelf, kCmdLoad, 0x340);
			cmd(kSelf, kCmdSetPos, 0x340, 0x126, 0x9b, 0x67);
			cmd(kSelf, kCmdLoad, 0x24f, 0xffff);
			cmd(kSelf, kCmdSetPos, 0x24f, 0x126, 0x9b, 0x67);
			if (var(0x4c).get() != 0)
				cmd(kSelf, kCmdPlay, 0x340);
			break;
		}
		case kMsgEntered:
			if (var(kVarVisited).get() != 1) {
				cmd(kSelf, kCmdPause, 0x251);
				var(kVarVisited).set(1);
				send(0xf, kMsgSay, 0x72a0729, 0x10);
				break;
			}
			send(kGameState, 10);
			if (var(0x4b).get() == 0)
				startAnimTimer();
			break;
		case 0x30:
			if (msg.p2 == 0x250) {
				startAnimTimer();
			} else if (msg.p2 == 0x252) {
				if (var(0x4b).get() == 0)
					startAnimTimer();
			} else if (msg.p2 == 0x253) {
				send(0x10, kMsgSay, 0x7380737);
				cmd(kSelf, kCmdStop, 0x251);
				cmd(kSelf, kCmdStop, 0x240);
				cmd(kSelf, kCmdPlay, 0x241);
				openSprites();
				cmd(0x50, kCmdSetCursor, 0x2f1, (int32)0x80000007);
			}
			break;
		case 0x3b:
			switch (msg.p1) {
			case 0x240:
			case 0x241:
			case 0x24f:
			case 0x250:
			case 0x251:
			case 0x252:
			case 0x253:
				msg.result = 1;
				break;
			case 0x242:
				msg.result = var(0x4b).get();
				break;
			default:
				break;
			}
			break;
		case 0x3c:
			switch (msg.p1) {
			case 0x240:
				if (var(0x4b).get() == 0)
					send(0x10, kMsgSay, 0x7320731, 0xf);
				break;
			case 0x241:
				send(kSpeech, kMsgSay, 0x73c073b);
				break;
			case 0x242:
				if (var(0x44).get() == 0)
					send(kSpeech, kMsgSay, 0x7420741);
				else
					send(kSpeech, kMsgSay, 0x7440743);
				break;
			case 0x24f:
				send(kSpeech, kMsgSay, 0x73e073d);
				break;
			case 0x250:
			case 0x251:
			case 0x252:
				send(kSpeech, kMsgSay, 0x73a0739);
				break;
			case 0x253:
				send(kSpeech, kMsgSay, 0x740073f);
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x243)
				send(kGameState, kMsgGotoRoom, 0x26);
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x24f && var(0x4c).get() == 0) {
				var(0x4c).set(1);
				cmd(kSelf, kCmdStop, 0x24f);
				cmd(kSelf, kCmdPlay, 0x340);
				cmd(0x61, kCmdPlay, 0x33e);
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x241) {
				if (isKey(msg.p1))
					msg.result = 1;
			} else if (msg.p2 == 0x251 || msg.p2 == 0x255) {
				if ((uint32)msg.p1 >> 16 == 0x372)
					msg.result = 1;
			}
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x372) {
				stopDragged(msg.p1);
				setCursor(0);
				int32 anim = 0;
				if (msg.p2 == 0x251) {
					stopTimer(2);
					var(0x2d, (uint16)msg.p1).set(1);
					anim = 0x246;
				} else if (msg.p2 == 0x255) {
					stopTimer(2);
					var(0x4b).set(1);
					var(0x2d, (uint16)msg.p1).set(0);
					var(0x31, (uint16)msg.p1).set(1);
					anim = 0x247;
				}
				if (anim)
					send(0xf, 0x2f, anim, 0x2440000, 0);
			}
			if (isKey(msg.p1) && msg.p2 == 0x241) {
				stopDragged(msg.p1);
				setCursor(0);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x31, (uint16)msg.p1).set(1);
				if (var(0x4b).get() != 0)
					cmd(kSelf, kCmdPlay, 0x24f, 0xffff);
			}
			break;
		default:
			break;
		}
	}
};

/** Object 35 (vtable 0x4b4dc4, FUN_0041e160), room 0x23. */
class Object35 : public RoomScript {
public:
	Object35(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x23)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x25b);
			cmd(kSelf, kCmdPlay, 0x25b);
			cmd(kSelf, kCmdSetZ, 0x25b, 100);
			cmd(kSelf, kCmdPlay, 0x25a);
			cmd(kSelf, kCmdSetZ, 0x25a, 0x65);
			cmd(kSelf, kCmdPlay, 0x264);
			cmd(kSelf, kCmdSetZ, 0x264, 0x65);
			cmd(kSelf, kCmdPlay, 0x25c);
			cmd(kSelf, kCmdSetPos, 0x25c, 0x1de, 0x145, 0x65);
			int32 flags = var(0xad).get() != 1 ? (int32)0xffff3fff : (int32)0xc0003fff;
			cmd(kSelf, kCmdPlay, 0x266, 0xffff, flags, 0);
			cmd(kSelf, kCmdSetPos, 0x266, 0xb0, 0x12, 0x67);
			cmd(kSelf, kCmdPlay, 599, 0, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 599, 0x8a, 0, 0x66);
			int32 command = var(0xa9).get() != 1 ? kCmdLoad : kCmdPlay;
			cmd(kSelf, command, 0x259, 0xffff, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0x259, 0x17e, 0x125, 0x68);
			command = var(0xa9).get() != 2 ? kCmdLoad : kCmdPlay;
			cmd(kSelf, command, 600, 0xffff, (int32)0xc0003fff);
			cmd(kSelf, kCmdSetPos, 600, 0x17e, 0x125, 0x68);
			if (var(0xae).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x263, 0xffff, (int32)0xc0003fff);
				cmd(kSelf, kCmdSetPos, 0x263, 0x177, 0x113, 0x6b);
			}
			break;
		}
		case kMsgEntered:
			entered(0xf, 0x7780777, 0x10);
			break;
		case 0x3b:
			if (msg.p1 == 0x25c || msg.p1 == 0x264 || msg.p1 == 0x266)
				msg.result = 1;
			break;
		case 0x3c:
			if (msg.p1 == 0x25c)
				send(kSpeech, kMsgSay, 0x7880787);
			else if (msg.p1 == 0x264)
				send(kSpeech, kMsgSay, 0x6fa06f9);
			else if (msg.p1 == 0x266)
				send(kSpeech, kMsgSay, 0x7840783);
			break;
		case kMsgClick:
			switch (msg.p1) {
			case 600:
				cmd(kSelf, kCmdResume, 600);
				break;
			case 0x259:
				cmd(kSelf, kCmdResume, 0x259);
				if (var(0xae).get() == 0) {
					cmd(kSelf, kCmdResume, 0x263);
					var(0xae).set(1);
				}
				break;
			case 0x25a:
				send(kGameState, kMsgGotoRoom, 0x31);
				break;
			case 0x25c:
				cmd(kSelf, kCmdPlay, 0x25d);
				break;
			case 0x266:
				cmd(kSelf, kCmdResume, 0x266);
				break;
			default:
				break;
			}
			break;
		case kMsgAnimDone:
			switch (msg.p1) {
			case 600:
				cmd(kSelf, kCmdStop, 600);
				cmd(kSelf, kCmdPlay, 0x259, 0xffff, (int32)0xc0003fff);
				var(0xa9).set(1);
				break;
			case 0x259:
				cmd(kSelf, kCmdStop, 0x259);
				cmd(kSelf, kCmdPlay, 600, 0xffff, (int32)0xc0003fff);
				var(0xa9).set(2);
				break;
			case 0x263:
				cmd(kSelf, kCmdStop, 0x263);
				cmd(0x67, kCmdPlay, 0x348);
				break;
			case 0x266:
				var(0xad).set(2);
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

/** Object 37 (vtable 0x4b4d84, FUN_004209e0), room 0x25. */
class Object37 : public RoomScript {
public:
	Object37(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x25)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void showWay() {
		cmd(kSelf, kCmdPlay, 0x291);
		cmd(kSelf, kCmdPlay, 0x290);
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x28c);
			cmd(kSelf, kCmdPlay, 0x28c);
			cmd(kSelf, kCmdSetZ, 0x28c, 100);
			cmd(kSelf, kCmdLoad, 0x290);
			cmd(kSelf, kCmdSetZ, 0x290, 0x65);
			cmd(kSelf, kCmdPlay, 0x28a);
			cmd(kSelf, kCmdSetZ, 0x28a, 0x65);
			cmd(kSelf, kCmdPlay, 0x286);
			cmd(kSelf, kCmdSetZ, 0x286, 0x65);
			cmd(kSelf, kCmdPlay, 0x28b);
			cmd(kSelf, kCmdSetZ, 0x28b, 0x65);
			cmd(kSelf, kCmdLoad, 0x28e);
			cmd(kSelf, kCmdSetPos, 0x28e, 0x114, 0x125, 0x6c);
			int32 cursor;
			if (var(99).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x289, 0xffff, (int32)0xc0003fff);
				cmd(kSelf, kCmdSetPos, 0x289, 0xd1, 0x11a, 0x6b);
				cursor = (int32)0x80000006;
			} else {
				cmd(kSelf, kCmdPlay, 0x289, 0xffff, (int32)0xffff3fff);
				cmd(kSelf, kCmdSetPos, 0x289, 0xd1, 0x11a, 0x6b);
				cursor = (int32)0x80000003;
			}
			cmd(kSelf, kCmdSetCursor, 0x289, cursor);
			cmd(kSelf, kCmdLoad, 0x288);
			cmd(kSelf, kCmdSetPos, 0x288, 0x10c, 0x121, 0x6a);
			cmd(kSelf, kCmdLoad, 0x292);
			cmd(kSelf, kCmdSetPos, 0x292, 0xe3, 0x24, 0x69);
			cmd(kSelf, kCmdLoad, 0x291);
			cmd(kSelf, kCmdSetPos, 0x291, 199, 0x23, 0x6c);
			int32 state = var(0x62).get();
			if (state == 0) {
				cmd(kSelf, kCmdPlay, 0x288, 0xffff, (int32)0xc0003fff);
				cmd(kSelf, kCmdPlay, 0x292);
			} else if (state == 1) {
				cmd(kSelf, kCmdPlay, 0x288, 0xffff, (int32)0xffff3fff);
				cmd(kSelf, kCmdPlay, 0x292);
			} else if (state == 2) {
				cmd(kSelf, kCmdPlay, 0x288, 0xffff, (int32)0xffff3fff);
				showWay();
			}
			break;
		}
		case kMsgEntered:
			entered(0xf, 0x7b107b0, 0x10);
			break;
		case 0x3b:
			if (var(99).get() != 0 && (msg.p1 == 0x288 || msg.p1 == 0x28c || msg.p1 == 0x292))
				msg.result = 1;
			break;
		case 0x3c:
			if (var(99).get() != 0) {
				if (msg.p1 == 0x288)
					send(kSpeech, kMsgSay, 0x7c307c2);
				else if (msg.p1 == 0x28c)
					send(kSpeech, kMsgSay, 0x7c707c6);
				else if (msg.p1 == 0x292)
					send(kSpeech, kMsgSay, 0x7c507c4);
			}
			break;
		case kMsgClick:
			switch (msg.p1) {
			case 0x286:
				cmd(kSelf, kCmdPlay, 0x287);
				break;
			case 0x289:
				if (var(99).get() == 0) {
					cmd(kSelf, kCmdResume, 0x289);
					var(99).set(1);
					cmd(kSelf, kCmdSetCursor, 0x289, (int32)0x80000003);
				}
				break;
			case 0x28a:
				send(kGameState, kMsgGotoRoom, 0x1b);
				break;
			case 0x290:
				send(kGameState, kMsgGotoRoom, 0x32);
				break;
			default:
				break;
			}
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x28e) {
				cmd(0x60, kCmdStop, 0x321);
				send(0x1b, 0x38);
			} else if (msg.p1 == 0x289) {
				send(0x10, kMsgSay, 0x7c107c0);
			} else if (msg.p1 == 0xf5) {
				send(0x1b, 0x39);
				cmd(kSelf, kCmdUsePalette, 0x28c);
				cmd(kSelf, kCmdStop, 0x292);
				showWay();
			} else if (msg.p1 == 0x288) {
				send(0xf, kMsgSay, 0x7b907b8, 0x10);
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (var(99).get() != 0) {
				uint32 item = (uint32)msg.p1 >> 16;
				if (item == 0x2f7)
					msg.result = msg.p2 == 0x288;
				else if (item == 0x323 || item == 0x37d)
					msg.result = msg.p2 == 0x28b;
			}
			break;
		case kMsgDrop: {
			stopDragged(msg.p1);
			setCursor(0);
			uint16 item = msg.p1;
			uint32 sprite = (uint32)msg.p1 >> 16;
			var(0x2d, item).set(1);
			if (sprite == 0x2f7 && msg.p2 == 0x288) {
				if (var(0x62).get() == 0 && var(99).get() != 0) {
					var(0x2d, item).set(0);
					var(0x31, item).set(1);
					cmd(kSelf, kCmdResume, 0x288);
					var(0x62).set(1);
				}
			}
			if (sprite == 0x37d && msg.p2 == 0x28b) {
				if (var(0x62).get() != 0) {
					cmd(kSelf, kCmdPlay, 0x28f);
					int32 p1 = 0;
					if (var(0x31, 0x60).get() != 0) {
						var(0x2d, item).set(0);
						var(0x31, item).set(1);
						var(0x31, 0x60).set(2);
						var(0x62).set(2);
						p1 = 0xffff;
					}
					cmd(kSelf, kCmdPlay, 0x28e, p1, (int32)0x8000c001);
				}
			}
			if (sprite == 0x323 && msg.p2 == 0x28b) {
				if (var(0x62).get() != 0) {
					var(0x2d, item).set(0);
					var(0x31, item).set(1);
					cmd(0x60, kCmdPlay, 0x321);
				}
			}
			break;
		}
		default:
			break;
		}
	}
};

/** Object 38 (vtable 0x4b4d64, FUN_00425400), room 0x26. */
class Object38 : public RoomScript {
public:
	Object38(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x26)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			if (msg.p1 == 0x1b) {
				cmd(0xf, kCmdStop, 0x399);
				send(0xf, 0x2f, 0x39d, 0x3990026, (int32)0x8000c001);
			} else if (msg.p1 == 0x22) {
				send(0xf, 0x2f, 0x39f, 0x3990026, (int32)0x8000c001);
			} else {
				send(kGameState, kMsgSwitchRoom);
			}
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdUsePalette, 0x398);
			cmd(kSelf, kCmdPlay, 0x398);
			cmd(kSelf, kCmdSetZ, 0x398, 100);
			cmd(kSelf, kCmdPlay, 0x397);
			cmd(kSelf, kCmdSetZ, 0x397, 0x69);
			cmd(kSelf, kCmdPlay, 0x38f);
			cmd(kSelf, kCmdSetZ, 0x38f, 0x69);
			cmd(kSelf, kCmdLoad, 0x396);
			cmd(kSelf, kCmdSetZ, 0x396, 0x66);
			cmd(kSelf, kCmdSetPos, 0x395, 0x28, 0x9d, 0x65);
			if (var(0x5b).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x395, 0xffff, (int32)0xc0003fff, 0xffff);
			} else {
				cmd(kSelf, kCmdPlay, 0x395, 0xffff, (int32)0xffff3fff, 0);
				cmd(kSelf, kCmdPlay, 0x396, 0xffff, 0, 0);
			}
			cmd(kSelf, kCmdSetPos, 0x38d, 0x49, 0xf1, 0x68);
			cmd(kSelf, kCmdPlay, 0x38d, 0xffff, (int32)0xc0003fff, 0xffff);
			if (var(0x5c).get() != 0)
				cmd(kSelf, kCmdSetCursor, 0x38d, 0x165);
			if (var(0x5d).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x391, 0xffff, (int32)0xc000c001, 0xffff);
				cmd(kSelf, kCmdLoad, 0x390, 0xffff, (int32)0xc000c001, 0);
			} else {
				cmd(kSelf, kCmdPlay, 0x390, 0xffff, (int32)0xc000c001, 0xffff);
				cmd(kSelf, kCmdLoad, 0x391, 0xffff, (int32)0xc000c001, 0);
			}
			cmd(kSelf, kCmdSetPos, 0x391, 0x101, 0x135, 0x69);
			cmd(kSelf, kCmdSetPos, 0x390, 0x101, 0x135, 0x69);
			break;
		case kMsgEntered:
			entered(0x10, 0x8470846, 0xf);
			break;
		case 0x30:
			switch (msg.p2) {
			case 0x38d:
				var(0x11, 0xf).set(0);
				var(0x10, 0xf).set(0);
				if (var(0x5c).get() == 0) {
					cmd(0xf, kCmdPause, 0x39d);
					send(0xf, kMsgSay, 0x84b084a, 0x10);
					var(0x5c).set(1);
				}
				break;
			case 0x395:
				var(0x5b).set(1);
				cmd(kSelf, kCmdPlay, 0x396, 0xffff);
				break;
			case 0x39d:
			case 0x39f:
				send(kGameState, kMsgSwitchRoom);
				break;
			default:
				break;
			}
			if (msg.p1 == 0x39a) {
				cmd(0xf, kCmdPlay, 0x399);
				var(0x11, 0xf).set(0x39c);
				var(0x10, 0xf).set(0x399);
				send(0xf, kMsgSay, 0x8530852, 0xf);
				var(0x5e).set(0);
			} else if (msg.p1 == 0x39e) {
				send(0xf, kMsgSay, 0x84f084e, 0x10);
			}
			break;
		case 0x36:
			cmd(0xf, kCmdStop, 0x39e);
			cmd(0xf, kCmdPlay, 0x39a, 0x300026, (int32)0x8000c001);
			break;
		case 0x3b:
			switch (msg.p1) {
			case 0x38d:
			case 0x38f:
			case 0x395:
			case 0x396:
			case 0x398:
				msg.result = 1;
				break;
			default:
				break;
			}
			break;
		case 0x3c:
			switch (msg.p1) {
			case 0x38d:
				if (var(0x5c).get() == 0)
					send(kSpeech, kMsgSay, 0x8630862);
				else
					send(kSpeech, kMsgSay, 0x8650864);
				break;
			case 0x38f:
				send(kSpeech, kMsgSay, 0x85f085e);
				break;
			case 0x395:
			case 0x396:
				send(kSpeech, kMsgSay, 0x8610860);
				break;
			case 0x398:
				send(kSpeech, kMsgSay, 0x86f086e);
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			switch (msg.p1) {
			case 0x38d:
				if (var(0x5c).get() != 0 && var(0x5e).get() == 0)
					send(kGameState, kMsgGotoRoom, 0x1b);
				break;
			case 0x38f:
				if (var(0x5e).get() == 0) {
					var(0x5e).set(1);
					var(0x11, 0xf).set(0);
					var(0x10, 0xf).set(0);
					cmd(0xf, kCmdStop, 0x399);
					cmd(0xf, kCmdPlay, 0x39e, 0x300026, (int32)0x80003fff);
					cmd(kSelf, kCmdEnable, 0x38f, 0);
				}
				break;
			case 0x390:
				cmd(kSelf, kCmdResume, 0x390);
				cmd(kSelf, kCmdPlay, 0x392);
				break;
			case 0x391:
				cmd(kSelf, kCmdResume, 0x391);
				cmd(kSelf, kCmdPlay, 0x392);
				break;
			case 0x395:
				if (var(0x5e).get() == 0)
					send(kSelf, 0x2f, 0x395, 0xffff);
				break;
			case 0x396:
				if (var(0x5e).get() == 0)
					send(kGameState, kMsgGotoRoom, 0x22);
				break;
			case 0x397:
				if (var(0x5e).get() == 0)
					send(kGameState, kMsgGotoRoom, 0x24);
				break;
			default:
				break;
			}
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x390) {
				cmd(kSelf, kCmdStop, 0x390);
				cmd(kSelf, kCmdPlay, 0x391, 0xffff, (int32)0xc0003fff);
				var(0xa9).set(2);
			} else if (msg.p1 == 0x391) {
				cmd(kSelf, kCmdStop, 0x391);
				cmd(kSelf, kCmdPlay, 0x390, 0xffff, (int32)0xc0003fff);
				var(0xa9).set(1);
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x38d && (uint32)msg.p1 >> 16 == 0x2e0)
				msg.result = 1;
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x2e0 && msg.p2 == 0x38d) {
				stopDragged(msg.p1);
				setCursor(0);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x31, (uint16)msg.p1).set(1);
				send(kGameState, kMsgGotoRoom, 0x1b);
				cmd(kSelf, kCmdPlay, 0x393);
			}
			break;
		default:
			break;
		}
	}
};

/** Object 39 (vtable 0x4b4d44, FUN_004264c0), room 0x27. */
class Object39 : public RoomScript {
public:
	Object39(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x27)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdUsePalette, 0x427);
			cmd(kSelf, kCmdPlay, 0x427);
			cmd(kSelf, kCmdSetZ, 0x427, 100);
			cmd(kSelf, kCmdPlay, 0x428);
			cmd(kSelf, kCmdSetZ, 0x428, 0x65);
			if (var(0xaf).get() == 0 && var(0x62, 0x25).get() != 2) {
				cmd(kSelf, kCmdPlay, 0x42c);
				cmd(kSelf, kCmdSetPos, 0x42c, 0x4e, 0x4a, 0x68);
				cmd(kSelf, kCmdLoad, 0x42a, 0xffff, (int32)0x8000c001);
				cmd(kSelf, kCmdSetPos, 0x42a, 0x4e, 0x4a, 0x68);
				cmd(kSelf, kCmdLoad, 0x42b);
				cmd(kSelf, kCmdSetPos, 0x42b, 0x8c, 0x58, 0x68);
				cmd(kSelf, kCmdPlay, 0x426);
				cmd(kSelf, kCmdSetPos, 0x426, 0x20e, 0xe8, 0x65);
			} else {
				cmd(kSelf, kCmdPlay, 0x42b);
				cmd(kSelf, kCmdSetPos, 0x42b, 0x8c, 0x58, 0x68);
				cmd(kSelf, kCmdPlay, 0x42d, 0xffff);
			}
			cmd(kSelf, kCmdPlay, 0x425);
			cmd(kSelf, kCmdSetZ, 0x425, 0x6c);
			cmd(kSelf, kCmdPlay, 0x42e);
			cmd(kSelf, kCmdSetZ, 0x42e, 0x66);
			break;
		case kMsgEntered:
			entered(0x10, 0x8730872, 0xf);
			break;
		case 0x3b:
			if (msg.p1 == 0x425 || msg.p1 == 0x427 || msg.p1 == 0x42e)
				msg.result = 1;
			break;
		case 0x3c:
			if (msg.p1 == 0x425)
				send(kSpeech, kMsgSay, 0x8830882);
			else if (msg.p1 == 0x427)
				send(kSpeech, kMsgSay, 0x8890888);
			else if (msg.p1 == 0x42e)
				send(kSpeech, kMsgSay, 0x8850884);
			break;
		case kMsgClick:
			if (msg.p1 == 0x428)
				send(kGameState, kMsgGotoRoom, 0x2b);
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x42a) {
				cmd(0x27, kCmdPlay, 0x42b);
				cmd(0x27, kCmdUnload, 0x42a);
				cmd(0x27, kCmdUnload, 0x426);
				cmd(0x60, kCmdPlay, 0x322);
				var(0xaf).set(1);
				var(kVarDragMode, kGameState).set(0);
				setCursor(0);
			}
			break;
		case kMsgSoundDone:
			if (msg.p1 == 0x42d)
				cmd(kSelf, kCmdPlay, 0x42d, 0xffff);
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x425)
				msg.result = (uint32)msg.p1 >> 16 == 0x319;
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x319 && msg.p2 == 0x425 && var(0xaf).get() == 0) {
				stopDragged(msg.p1);
				setCursor(0);
				var(0x2d, (uint16)msg.p1).set(0);
				cmd(kSelf, kCmdStop, 0x42c);
				cmd(kSelf, kCmdPlay, 0x42a, 0xffff);
				var(kVarDragMode, kGameState).set(1);
				setCursor(kCursorWait);
				cmd(kSelf, kCmdPlay, 0x42d, 0xffff);
				send(0xf, kMsgSay, 0x8770876, 0x10);
			}
			break;
		default:
			break;
		}
	}
};

/** Object 41 (vtable 0x4b4d04, FUN_0042af10), room 0x29. */
class Object41 : public RoomScript {
public:
	Object41(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x29) {
			if (msg.msg == kMsgExit)
				send(kGameState, kMsgSwitchRoom);
			else if (msg.msg == kMsgEntered)
				send(kGameState, 10);
		}
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}
};

/** Object 43 (vtable 0x4b4cc4, FUN_0042f440), room 0x2b. */
class Object43 : public RoomScript {
public:
	Object43(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		_now = currentTime();
		if (currentRoom() == 0x2b)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	/** Variable 5 of the game state object [unverified: the game clock]. */
	int32 gameClock() { return var(5, kGameState).get(); }

	/** Is the animation child of object still running? */
	bool isRunning(uint16 object, uint16 child) {
		return cmd(object, kCmdIsRunning, child) != 0;
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			if (msg.p1 == 0x27) {
				int32 anim = var(0x10, 0xf).get();
				send(0xf, 0x2f, 0x453, anim << 16 | 0x2b, (int32)0x8000c001);
				anim = var(0x10, 0x10).get();
				send(0x10, 0x2f, 0x45a, anim << 16 | 0x2b, (int32)0x8000c001);
			} else {
				send(kGameState, kMsgSwitchRoom);
			}
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x44d);
			cmd(kSelf, kCmdPlay, 0x44d);
			cmd(kSelf, kCmdPlay, 0x447);
			cmd(kSelf, kCmdSetZ, 0x447, 100);
			cmd(kSelf, kCmdPlay, 0x43e);
			cmd(kSelf, kCmdSetZ, 0x43e, 0x65);
			cmd(kSelf, kCmdPlay, 0x440);
			cmd(kSelf, kCmdSetZ, 0x440, 0x65);
			cmd(kSelf, kCmdPlay, 0x43d);
			cmd(kSelf, kCmdSetZ, 0x43d, 0x66);
			cmd(kSelf, kCmdPlay, 0x45b);
			cmd(kSelf, kCmdSetZ, 0x45b, 0x65);
			if (gameClock() < 0x10) {
				cmd(kSelf, kCmdPlay, 0x443);
				cmd(kSelf, kCmdSetPos, 0x443, 0x8a, 0x10b, 0x65);
				cmd(kSelf, kCmdPlay, 0x441, 0xffff);
			} else if (gameClock() < 0x1f) {
				cmd(kSelf, kCmdPlay, 0x444);
				cmd(kSelf, kCmdSetPos, 0x444, 0x98, 0x11b, 0x65);
				cmd(kSelf, kCmdPlay, 0x442, 0xffff);
			} else if (gameClock() > 0x2d) {
				cmd(kSelf, kCmdPlay, 0x446);
				cmd(kSelf, kCmdSetZ, 0x446, 0x65);
			} else {
				cmd(kSelf, kCmdPlay, 0x445);
				cmd(kSelf, kCmdSetPos, 0x445, 0xb0, 300, 0x65);
				cmd(kSelf, kCmdPlay, 0x442, 0xffff);
			}
			int32 command;
			if (var(0x59).get() == 1) {
				cmd(kSelf, kCmdPlay, 0x448);
				cmd(kSelf, kCmdSetPos, 0x448, 0x1b0, 0x8f, 0x6a);
				cmd(kSelf, kCmdLoad, 1099);
				cmd(kSelf, kCmdSetPos, 1099, 0x1b0, 0x8f, 0x66);
				command = kCmdLoad;
			} else {
				command = kCmdPlay;
			}
			cmd(kSelf, command, 0x449);
			cmd(kSelf, kCmdSetZ, 0x43e, 0x66);
			cmd(kSelf, kCmdSetZ, 0x449, 0x6a);
			cmd(kSelf, kCmdLoad, 0x44a);
			cmd(kSelf, kCmdSetPos, 0x44a, 0x1b0, 0x8f, 0x66);
			// In December, a Christmas decoration.
			if (isDecember(_now)) {
				cmd(kSelf, kCmdPlay, 0x43f);
				cmd(kSelf, kCmdSetPos, 0x43f, 0x12e, 0x7c, 0x65);
			}
			break;
		}
		case kMsgEntered:
			entered(0xf, 0x8a808a7, 0x10);
			break;
		case 0x23:
			if ((int16)msg.p1 == 0x68)
				send(0xf, kMsgSay, 0x8b408b3, 0x10);
			else
				send(0x10, kMsgSay, 0x8ac08ab);
			break;
		case 0x30:
			if (msg.p2 == 0x453) {
				uint16 anim = var(0x10, 0xf).get();
				cmd(0xf, kCmdStop, anim);
				if (!isRunning(0x10, 0x45a))
					send(kGameState, kMsgSwitchRoom);
			} else if (msg.p2 == 0x45a) {
				uint16 anim = var(0x10, 0x10).get();
				cmd(0x10, kCmdStop, anim);
				if (!isRunning(0xf, 0x453))
					send(kGameState, kMsgSwitchRoom);
			}
			break;
		case 0x3b:
			switch (msg.p1) {
			case 0x43d:
			case 0x43e:
			case 0x440:
			case 0x443:
			case 0x444:
			case 0x445:
			case 0x449:
			case 0x44d:
			case 0x45b:
				msg.result = 1;
				break;
			default:
				break;
			}
			break;
		case 0x3c:
			switch (msg.p1) {
			case 0x43d:
				send(kSpeech, kMsgSay, 0x8ca08c9);
				break;
			case 0x43e:
				if (var(0x44).get() == 0)
					send(kSpeech, kMsgSay, 0x8c408c3);
				else
					send(kSpeech, kMsgSay, 0x8c608c5);
				break;
			case 0x440:
				send(kSpeech, kMsgSay, 0x8c808c7);
				break;
			case 0x443:
			case 0x444:
			case 0x445:
			case 0x446:
				if (gameClock() < 0x1f)
					send(kSpeech, kMsgSay, 0x8bc08bb);
				else if (gameClock() < 0x2e)
					send(kSpeech, kMsgSay, 0x8be08bd);
				else
					send(kSpeech, kMsgSay, 0x8c008bf);
				break;
			case 0x449:
				send(kSpeech, kMsgSay, 0x8cc08cb);
				break;
			case 0x44d:
				send(kSpeech, kMsgSay, 0x8ba08b9);
				break;
			case 0x45b:
				send(kSpeech, kMsgSay, 0x8b808b7);
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x447)
				send(kGameState, kMsgGotoRoom, 0x24);
			else if (msg.p1 == 0x449)
				send(kGameState, kMsgGotoRoom, 0x27);
			break;
		case kMsgAnimDone:
			if (msg.p1 == 1099) {
				send(0x10, kMsgSay, 0x8ae08ad, 0xf);
				cmd(kSelf, kCmdPlay, 0x449);
			} else if (msg.p1 == 0x452 || msg.p1 == 0x459) {
				bool running;
				if (msg.p1 == 0x452) {
					cmd(0xf, kCmdStop, 0x452);
					cmd(0xf, kCmdPlay, 0x44f);
					running = isRunning(0x10, 0x459);
				} else {
					cmd(0x10, kCmdStop, 0x459);
					cmd(0x10, kCmdPlay, 0x456);
					running = isRunning(0xf, 0x452);
				}
				if (!running) {
					cmd(kSelf, kCmdPlay, 0x44c);
					cmd(kSelf, kCmdResume, 1099);
				}
			}
			break;
		case kMsgSoundDone:
			if (msg.p1 == 0x441)
				cmd(kSelf, kCmdPlay, 0x441, 0xffff);
			else if (msg.p1 == 0x442)
				cmd(kSelf, kCmdPlay, 0x442, 0xffff);
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (msg.p2 == 0x448)
				msg.result = (uint32)msg.p1 >> 16 == 0x313;
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x313 && msg.p2 == 0x448) {
				stopDragged(msg.p1);
				setCursor(0);
				var(0x2d, (uint16)msg.p1).set(0);
				var(0x31, (uint16)msg.p1).set(1);
				cmd(kSelf, kCmdStop, 0x448);
				cmd(0xf, kCmdStop, 0x44e);
				cmd(0xf, kCmdPlay, 0x452, 0x2b);
				var(0x10, 0xf).set(0x44f);
				var(0x11, 0xf).set(0x451);
				cmd(0x10, kCmdStop, 0x455);
				cmd(0x10, kCmdPlay, 0x459, 0x2b);
				var(0x10, 0x10).set(0x456);
				var(0x11, 0x10).set(0x458);
				cmd(kSelf, kCmdPlay, 1099, 0xffff, (int32)0xc000c001);
				var(0x59).set(2);
			}
			break;
		default:
			break;
		}
	}
};

/** Object 44 (vtable 0x4b4ca4, FUN_004302e0), room 0x2c. */
class Object44 : public RoomScript {
public:
	Object44(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x2c)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	/** DAT_004caa58: set when the way to room 0x31 was taken through 0x45c. */
	static const uint32 kGlobalExitAnim = 0x4caa58;

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			if (msg.p1 == 0x31 && global32(kGlobalExitAnim) != 0)
				cmd(kSelf, kCmdPlay, 0x469);
			else
				send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter:
			cmd(kSelf, kCmdUsePalette, 0x45f);
			cmd(kSelf, kCmdPlay, 0x45f);
			cmd(kSelf, kCmdSetZ, 0x45f, 100);
			cmd(kSelf, kCmdPlay, 0x45c);
			cmd(kSelf, kCmdSetZ, 0x45c, 0x65);
			cmd(kSelf, kCmdPlay, 0x45e);
			cmd(kSelf, kCmdSetZ, 0x45e, 0x65);
			cmd(kSelf, kCmdPlay, 0x468);
			cmd(kSelf, kCmdSetZ, 0x468, 0x65);
			cmd(kSelf, kCmdPlay, 0x46a);
			cmd(kSelf, kCmdSetZ, 0x46a, 0x65);
			cmd(kSelf, kCmdLoad, 0x469);
			cmd(kSelf, kCmdSetPos, 0x469, 0x102, 0x76, 0x70);
			cmd(kSelf, kCmdPlay, 0x460);
			cmd(kSelf, kCmdSetZ, 0x460, 0x66);
			cmd(kSelf, kCmdPlay, 0x45d);
			cmd(kSelf, kCmdSetZ, 0x45d, 0x66);
			break;
		case kMsgEntered:
			entered(0xf, 0x8ce08cd, 0x10);
			break;
		case 0x23:
			send(0x10, kMsgSay, 0x8d608d5);
			break;
		case 0x30: {
			bool running;
			if (msg.p2 == 0x462) {
				cmd(0xf, kCmdStop, 0x461);
				running = cmd(0x10, kCmdIsRunning, 0x466) != 0;
			} else if (msg.p2 == 0x466) {
				cmd(0x10, kCmdStop, 0x465);
				running = cmd(0xf, kCmdIsRunning, 0x462) != 0;
			} else {
				break;
			}
			if (!running)
				send(kGameState, kMsgSwitchRoom);
			break;
		}
		case 0x3b:
			switch (msg.p1) {
			case 0x45d:
			case 0x45f:
			case 0x460:
			case 0x468:
			case 0x46a:
				msg.result = 1;
				break;
			default:
				break;
			}
			break;
		case 0x3c:
			switch (msg.p1) {
			case 0x45d:
				send(kSpeech, kMsgSay, 0x6b506b4);
				break;
			case 0x45f:
				send(kSpeech, kMsgSay, 0x8da08d9);
				break;
			case 0x460:
				send(kSpeech, kMsgSay, 0x7060705);
				break;
			case 0x468:
				send(kSpeech, kMsgSay, 0x8dc08db);
				break;
			case 0x46a:
				send(kSpeech, kMsgSay, 0x8d808d7);
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x45c) {
				if (var(0x8c, 0x1d).get() == 0) {
					setGlobal32(kGlobalExitAnim, 1);
					send(kGameState, kMsgGotoRoom, 0x31);
				} else {
					setGlobal32(kGlobalExitAnim, 0);
					send(kGameState, kMsgGotoRoom, 0x18);
				}
			} else if (msg.p1 == 0x45e) {
				send(kGameState, kMsgGotoRoom, 0x31);
			}
			break;
		default:
			break;
		}
	}
};

/** Object 45 (vtable 0x4b4c84, FUN_0043e670), room 0x2d. */
class Object45 : public RoomScript {
public:
	Object45(GameObject *obj) : RoomScript(obj) {}
	void handle(Message &msg) override {
		if (currentRoom() == 0x2d)
			handleRoom(msg);
		if (msg.handled == 0)
			ObjectScript::handle(msg);
	}

private:
	/** The item p1 goes on place (state variable mine); other is the other place's state. */
	void putItem(int32 p1, uint16 mine, uint16 other) {
		if (var(mine).get() != 0)
			return;
		var(mine).set(1);
		if (var(other).get() != 0) {
			var(0x2d, (uint16)p1).set(0);
			var(0x31, (uint16)p1).set(1);
		} else {
			var(0x2d, (uint16)p1).set(1);
		}
	}

	void handleRoom(Message &msg) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgExit:
			send(kGameState, kMsgSwitchRoom);
			break;
		case kMsgEnter: {
			cmd(kSelf, kCmdUsePalette, 0x472);
			cmd(kSelf, kCmdPlay, 0x472);
			cmd(kSelf, kCmdSetZ, 0x472, 100);
			int32 flags = var(0x8f).get() != 2 ? (int32)0xc0003fff : (int32)0xffff3fff;
			cmd(kSelf, kCmdPlay, 0x46f, 0xffff, flags, 0);
			cmd(kSelf, kCmdSetPos, 0x46f, 0x202, 0x108, 0x66);
			flags = var(0x90).get() != 2 ? (int32)0xc0003fff : (int32)0xffff3fff;
			cmd(kSelf, kCmdPlay, 0x46e, 0xffff, flags, 0);
			cmd(kSelf, kCmdSetPos, 0x46e, 0xc, 0x108, 0x66);
			cmd(kSelf, kCmdPlay, 0x46c);
			cmd(kSelf, kCmdSetZ, 0x46c, 0x66);
			cmd(kSelf, kCmdPlay, 0x46d);
			cmd(kSelf, kCmdSetZ, 0x46d, 0x65);
			cmd(kSelf, kCmdPlay, 0x471);
			cmd(kSelf, kCmdSetZ, 0x471, 0x66);
			if (var(0x2d, 0x4e).get() == 0 && var(0x31, 0x4e).get() == 0) {
				cmd(kSelf, kCmdPlay, 0x47b);
				cmd(kSelf, kCmdSetZ, 0x47b, 0x66);
			}
			break;
		}
		case kMsgEntered:
			entered(0x10, 0x8df08de, 0xf);
			break;
		case 0x23:
			if (msg.p1 == 0x4e)
				send(kSpeech, kMsgSay, 0x8ed08ec);
			break;
		case 0x30:
			if (msg.p2 == 0x475)
				send(10, 0x26, 0x5f);
			break;
		case 0x3b:
			switch (msg.p1) {
			case 0x46c:
			case 0x46d:
			case 0x472:
			case 0x47b:
				msg.result = 1;
				break;
			default:
				break;
			}
			break;
		case 0x3c:
			switch (msg.p1) {
			case 0x46c:
			case 0x472:
				send(kSpeech, kMsgSay, 0x8e908e8);
				break;
			case 0x46d:
				send(kSpeech, kMsgSay, 0x8eb08ea);
				break;
			case 0x47b:
				if (var(0x2d, 0x4e).get() == 0) {
					send(10, 0x26, 0x4e);
					cmd(kSelf, kCmdStop, 0x47b);
				}
				break;
			default:
				break;
			}
			break;
		case kMsgClick:
			if (msg.p1 == 0x46e) {
				if (var(0x90).get() == 0) {
					send(0xf, kMsgSay, 0x8e708e6);
				} else if (var(0x90).get() == 1) {
					send(0xf, 0x2f, 0x475, 0x473002d, (int32)0x8000c001);
					var(0x90).set(2);
				}
			} else if (msg.p1 == 0x46f) {
				if (var(0x8f).get() == 0) {
					send(0xf, kMsgSay, 0x8e708e6);
				} else {
					cmd(kSelf, kCmdResume, 0x46f);
					var(0x8f).set(2);
				}
			} else if (msg.p1 == 0x471) {
				send(kGameState, kMsgGotoRoom, 0x31);
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt: {
			uint16 state;
			if (msg.p2 == 0x46e)
				state = 0x90;
			else if (msg.p2 == 0x46f)
				state = 0x8f;
			else
				break;
			msg.result = ((uint32)msg.p1 >> 16 == 0x2ec && var(state).get() == 0) ? 1 : 0;
			break;
		}
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x2ec) {
				stopDragged(msg.p1);
				setCursor(0);
				if (msg.p2 == 0x46e)
					putItem(msg.p1, 0x90, 0x8f);
				if (msg.p2 == 0x46f)
					putItem(msg.p1, 0x8f, 0x90);
			}
			break;
		default:
			break;
		}
	}
};

IVANOFF_SCRIPT(Object23) { return new Object23(obj); }
IVANOFF_SCRIPT(Object24) { return new Object24(obj); }
IVANOFF_SCRIPT(Object25) { return new Object25(obj); }
IVANOFF_SCRIPT(Object27) { return new Object27(obj); }
IVANOFF_SCRIPT(Object28) { return new Object28(obj); }
IVANOFF_SCRIPT(Object29) { return new Object29(obj); }
IVANOFF_SCRIPT(Object31) { return new Object31(obj); }
IVANOFF_SCRIPT(Object32) { return new Object32(obj); }
IVANOFF_SCRIPT(Object34) { return new Object34(obj); }
IVANOFF_SCRIPT(Object35) { return new Object35(obj); }
IVANOFF_SCRIPT(Object37) { return new Object37(obj); }
IVANOFF_SCRIPT(Object38) { return new Object38(obj); }
IVANOFF_SCRIPT(Object39) { return new Object39(obj); }
IVANOFF_SCRIPT(Object41) { return new Object41(obj); }
IVANOFF_SCRIPT(Object43) { return new Object43(obj); }
IVANOFF_SCRIPT(Object44) { return new Object44(obj); }
IVANOFF_SCRIPT(Object45) { return new Object45(obj); }

} // End of namespace Ivanoff
