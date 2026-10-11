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

#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

// --- ObjectScript (vtable 0x4b51a4) ---

void ObjectScript::handle(Message &msg) {
	// FUN_0041b910
	switch (msg.msg) {
	case kMsgAnimDone:
		// p1 = the animation; p2 = (next animation << 16) | object to tell.
		if (var(0x4a).get() == 0) {
			int32 anim = msg.p1;
			if (anim != var(0x16).get())
				return;
			cmd(kSelf, kCmdStop, anim);
			cmd(kSelf, kCmdPlay, (uint32)msg.p2 >> 16);
			var(0x16).get();
			var(0x16).set(0);
			var(kVarBusy, kGameState).set(0);
			if (msg.p2 > 0)
				send(msg.p2, 0x30, getId(), (uint16)msg.p1);
		} else {
			uint16 next = (uint32)msg.p2 >> 16;
			if (next)
				cmd(kSelf, kCmdPlay, next);
			if (msg.p2 != 0) {
				var(3, kGameState).getString(0x5c, 0);
				send(msg.p2, 0x30, getId(), (uint16)msg.p1);
			}
		}
		break;
	case 0x13:
		if (msg.p1 != 0)
			setCursor(0);
		else if (var(kVarBusy, kGameState).get() == 0)
			setCursor(kCursorWait);
		break;
	case 0x2f:
		// Play animation p1 (p2 = (animation to stop << 16) | ..., passed on as the notification).
		if (var(0x4a).get() != 0) {
			if ((int16)msg.p2 != 0)
				var(3, kGameState).getString(0x54, 0);
		} else {
			if (var(0x16).get() != 0)
				return;
			if (var(kVarBusy, kGameState).get() != 0)
				return;
			var(0x16).set(msg.p1);
		}
		{
			int32 playing = cmd(kSelf, kCmdIsPlaying, msg.p1);
			uint16 stop = (uint32)msg.p2 >> 16;
			if (playing == 0) {
				if (stop)
					cmd(kSelf, kCmdStop, stop);
				cmd(kSelf, kCmdPlay, msg.p1, getId(), msg.p3, msg.p2);
			} else {
				if (stop)
					cmd(kSelf, kCmdStop, stop);
				cmd(kSelf, kCmdResume, msg.p1);
			}
		}
		break;
	case kMsgDropTest:
		if ((uint32)msg.p1 >> 16 != 0x54d)
			return;
		msg.result = send(kSelf, 0x3b, msg.p2);
		break;
	case kMsgDropTestAlt:
		if ((uint32)msg.p1 >> 16 != 0x54d)
			return;
		if (send(kSelf, 0x3b, msg.p2))
			send(kSelf, 0x3c, msg.p2);
		msg.result = msg.p2 == 0x54d;
		break;
	default:
		break;
	}
}

// --- ItemScript ---

bool ItemScript::accepts(Message &msg) {
	// FUN_00423a60: no clicks while an item is dragged.
	if (var(kVarDragMode, kGameState).get() == 1 && msg.msg == kMsgClick)
		return false;
	return true;
}

void ItemScript::handle(Message &msg) {
	// FUN_00423aa0
	switch (msg.msg) {
	case kMsgDragStartAlt: {
		VarRef pos = var(0x2e);
		pos.set(cmd(kSelf, kCmdGetPos, msg.p1));
		cmd(kSelf, kCmdStop, msg.p1);
		cmd(kSelf, kCmdLoad, var(0x30).get());
		cmd(kSelf, kCmdMove, var(0x30).get(), 0x200020, 1, 0);
		cmd(kSelf, kCmdEnable, var(0x30).get(), 1, 0, 0);
		setCursor(kCursorDrag);
		VarRef from = var(0x2d);
		VarRef to = var(0x32);
		to.set(from.get());
		break;
	}
	case 0x42:
		if (var(0x2d).get() != 0 && var(0x2f).get() != 0) {
			uint16 id = getId();
			send(10, 0x42, (var(0x2f).get() << 16) | id);
		}
		break;
	case 0x21:
		if (var(0x2d).get() != 0 && var(0x30).get() != 0)
			send(10, 0x22, getId(), var(0x30).get());
		break;
	case 0x3c:
		if (var(0x1c, 10).get() != 0)
			send(kSelf, 0x42);
		break;
	case kMsgCanDrag:
		if (var(kVarDragMode, kGameState).get() == 1)
			msg.result = 1;
		break;
	case kMsgDropTest:
		if (var(0x2d).get() == 0)
			msg.result = 0;
		else
			msg.result = send(kSelf, 0x43, (uint16)msg.p1);
		break;
	case kMsgDragEnd:
		if (msg.p2 == 0)
			setCursor(kCursorDrag);
		else
			setCursor(send(kSelf, 0x43, msg.p2 & 0xffff) ? kCursorDropYes : kCursorDropNo);
		break;
	case kMsgClickNoDrag: {
		cmd(kSelf, kCmdDetach, var(0x30).get());
		cmd(kSelf, kCmdPlay, msg.p1);
		VarRef pos = var(0x2e);
		VarRef pos2 = var(0x2e);
		int32 y = (int16)((uint32)pos.get() >> 16);
		int32 x = (int16)pos2.get();
		cmd(kSelf, kCmdSetPos, msg.p1, x, y, 0);
		setCursor(0);
		break;
	}
	default:
		break;
	}
	if (msg.handled == 0)
		ObjectScript::handle(msg);
}

IVANOFF_SCRIPT(ObjectScript) { return new ObjectScript(obj); }

} // End of namespace Ivanoff
