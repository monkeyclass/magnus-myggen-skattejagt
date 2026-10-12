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

// Items that lie in a room until they are picked up.

#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

/** Object 72 (vtable 0x4b4944, FUN_0040dd80). */
class Item72 : public ItemScript {
public:
	Item72(GameObject *obj) : ItemScript(obj) {}
	void handle(Message &msg) override {
		if (msg.msg == 0x3b)
			msg.result = 1;
		else if (msg.msg == 0x3c)
			send(kSpeech, kMsgSay, 0x7ea07eb);
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}
};

/** Object 67 (vtable 0x4b49e4, FUN_00405c60). */
class Item67 : public ItemScript {
public:
	Item67(GameObject *obj) : ItemScript(obj) {}
	void handle(Message &msg) override {
		switch (msg.msg) {
		case kMsgLeave:
			if (msg.p1 == 0x2c)
				cmd(kSelf, kCmdUnload, 0x2cf);
			break;
		case kMsgEnter:
			if (msg.p1 == 0x2c) {
				cmd(kSelf, kCmdLoad, 0x2cf);
				cmd(kSelf, kCmdSetPos, 0x2cf, 0x1ae, 0x140, 0x6e);
				if (var(0x2d).get() == 0 && var(0x31).get() == 0)
					cmd(kSelf, kCmdPlay, 0x2cf);
			}
			break;
		case 0x3b:
			msg.result = 1;
			break;
		case 0x3c:
			send(kSpeech, kMsgSay, 0x7e007e1);
			break;
		default:
			break;
		}
		if (msg.handled == 0)
			ItemScript::handle(msg);
	}
};

/** Object 76 (vtable 0x4b48c4, FUN_00413a10). */
class Item76 : public ItemScript {
public:
	Item76(GameObject *obj) : ItemScript(obj) {}
	void handle(Message &msg) override {
		switch (msg.msg) {
		case kMsgLeave:
			if (msg.p1 == 0x39)
				cmd(kSelf, kCmdUnload, 0x2e6);
			break;
		case kMsgEnter:
			if (msg.p1 == 0x39) {
				cmd(kSelf, kCmdLoad, 0x2e6);
				cmd(kSelf, kCmdSetPos, 0x2e6, 0x126, 0xee, 0x6e);
				if (var(0x2d).get() == 0 && var(0x31).get() == 0)
					cmd(kSelf, kCmdPlay, 0x2e6);
			}
			break;
		case 0x3b:
			msg.result = 1;
			break;
		case 0x3c:
			send(kSpeech, kMsgSay, 0x7f207f3);
			break;
		case 0x43:
			msg.result = msg.p1 == 0x4d;
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			msg.result = msg.p2 == var(0x30).get() && (int16)msg.p1 == 0x4d;
			break;
		case kMsgDrop:
			if ((int16)msg.p1 == 0x4d) {
				cmd(msg.p1, kCmdStop, var(0x30, 0x4d).get());
				setCursor(0);
				if (msg.p2 == var(0x30).get()) {
					cmd(kSelf, kCmdStop, var(0x30).get());
					var(0x2d, msg.p1).set(0);
					var(0x2d).set(0);
					var(0x2d, 0x4b).set(1);
					var(0x31, msg.p1).set(1);
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

void Item99::handle(Message &msg) {
	// FUN_004532e0
	switch (msg.msg) {
	case 0x3b:
		msg.result = 1;
		break;
	case 0x3c:
		send(kSpeech, kMsgSay, 0x8220825);
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
			uint16 item = msg.p1;
			cmd(msg.p1, kCmdStop, var(0x30, item).get());
			setCursor(0);
			if (msg.p2 == var(0x30).get()) {
				cmd(kSelf, kCmdStop, var(0x30).get());
				var(0x2d).set(0);
				if ((uint16)msg.p1 == 0x62) {
					var(0x34, 0x62).postIncrement();
				} else {
					var(0x2d, msg.p1).set(0);
					var(0x2d, 0x62).set(1);
					var(0x34, 0x62).add(2);
					var(0x31, msg.p1).set(1);
				}
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

/** Object 100 (vtable 0x4b45c4, FUN_00452b50), a kind of object 99. */
class Item100 : public Item99 {
public:
	Item100(GameObject *obj) : Item99(obj) {}
	void handle(Message &msg) override {
		if (msg.msg == kMsgLeave && msg.p1 == 0x24) {
			cmd(kSelf, kCmdUnload, 0x34b);
		} else if (msg.msg == kMsgEnter && msg.p1 == 0x24) {
			cmd(kSelf, kCmdLoad, 0x34b);
			cmd(kSelf, kCmdSetPos, 0x34b, 0x89, 0x135, 0x6e);
			if (var(0x2d).get() == 0 && var(0x31).get() == 0)
				cmd(kSelf, kCmdPlay, 0x34b);
		}
		if (msg.handled == 0)
			Item99::handle(msg);
	}
};

IVANOFF_SCRIPT(Item67) { return new Item67(obj); }
IVANOFF_SCRIPT(Item72) { return new Item72(obj); }
IVANOFF_SCRIPT(Item76) { return new Item76(obj); }
IVANOFF_SCRIPT(Item99) { return new Item99(obj); }
IVANOFF_SCRIPT(Item100) { return new Item100(obj); }

} // End of namespace Ivanoff
