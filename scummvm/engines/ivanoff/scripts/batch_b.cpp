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

// Batch b: the speaking characters (objects 15-21) and two puzzles of
// rooms 0x17 and 0x26 (objects 60 and 61).

#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

enum {
	kMsgSaid = 0x2e,    ///< from a speaker: p1 = speaker, p2 = the sound that ended
	kMsgPlayAnim = 0x2f ///< see ObjectScript: play animation p1
};

/**
 * The base of this batch's classes. When the interpreter runs an original
 * slot 5 or 6, the engine passes it the message in a 0x2c-byte block of
 * the game's heap, which stays allocated while the handler runs. The
 * heap's layout shows in the traces (objects pass heap addresses), so the
 * native handlers hold such a block too.
 */
class BatchBScript : public ObjectScript {
public:
	BatchBScript(GameObject *obj) : ObjectScript(obj) {}

	bool accepts(Message &msg) override {
		// FUN_0045da60 / FUN_0041c2c0 -> FUN_004554f0: takes everything.
		return true;
	}

	void handle(Message &msg) override {
		handleMessage(msg);
	}

protected:
	virtual void handleMessage(Message &msg) = 0;

private:
};

/**
 * A character that speaks (FUN_004547e0, the base of objects 15-21).
 *
 * kMsgSay: p1 = (text << 16) | sound, p2 = the object to tell when the
 * sound ends (kMsgSaid). While it speaks, the talking animation (var
 * 0x11) replaces the idle one (var 0x10).
 *
 * Variables: 0x10 idle animation, 0x11 talking animation, 0x12 the sound,
 * 0x13 the object to tell, 0x14 the talking animation playing, 0x15 the
 * idle animation was paused.
 */
class SpeakerScript : public BatchBScript {
public:
	SpeakerScript(GameObject *obj) : BatchBScript(obj) {}
	void handleMessage(Message &msg) override;
};

void SpeakerScript::handleMessage(Message &msg) {
	if (msg.msg == kMsgSay) {
		if (var(kVarBusy, kGameState).get() == 0) {
			send(13, 0x41);
			var(kVarBusy, kGameState).set(msg.p1);
			var(0x12).set((uint16)msg.p1);
			var(0x13).set(msg.p2);
			if (var(0x11).get() > 0) {
				VarRef idle = var(0x10);
				VarRef paused = var(0x15);
				paused.set(cmd(kSelf, kCmdIsPaused, idle.get()));
				cmd(kSelf, kCmdStop, var(0x10).get());
				VarRef talk = var(0x11);
				VarRef playing = var(0x14);
				playing.set(talk.get());
				cmd(kSelf, kCmdPlay, var(0x14).get());
			}
			uint16 text = (uint32)msg.p1 >> 16;
			if (text != 0)
				send(13, 0x3d, getId(), text, (uint16)msg.p1);
			uint16 id = getId();
			int32 loaded = cmd(id, kCmdIsLoaded, (uint16)msg.p1);
			cmd(kSelf, kCmdPlay, (uint16)msg.p1, id, 0, loaded == 0);
		}
	} else if (msg.msg == kMsgSoundDone) {
		if ((uint32)(uint16)msg.p1 == (uint32)var(0x12).get()) {
			if (var(0x14).get() > 0) {
				cmd(kSelf, kCmdStop, var(0x14).get());
				cmd(kSelf, kCmdPlay, var(0x10).get());
				if (var(0x15).get() != 0)
					cmd(kSelf, kCmdPause, var(0x10).get());
			}
			int32 sound = var(0x12).get();
			var(0x12).set(0);
			int32 notify = var(0x13).get();
			var(0x13).set(0);
			var(0x14).set(0);
			var(kVarBusy, kGameState).set(0);
			if (notify > 0)
				send(notify, kMsgSaid, getId(), sound & 0xffff);
			// p2: the sound was loaded for this speech only.
			if (msg.p2 != 0)
				cmd(kSelf, kCmdUnload, msg.p1);
		}
	}
	if (msg.handled == 0)
		ObjectScript::handle(msg);
}

/** Object 15 (vtable 0x4b5044, FUN_0043a0a0), Magnus [unverified]: his animations and voice in each room. */
class Object15 : public SpeakerScript {
public:
	Object15(GameObject *obj) : SpeakerScript(obj) {}
	void handleMessage(Message &msg) override;
};

void Object15::handleMessage(Message &msg) {
	switch (var(1, kGameState).get()) {
	case 0x16:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0x11);
			cmd(kSelf, kCmdUnload, 0x10);
			cmd(kSelf, kCmdUnload, 0x5a4);
			cmd(kSelf, kCmdUnload, 0x5a8);
			cmd(kSelf, kCmdUnload, 0x5ae);
			cmd(kSelf, kCmdUnload, 0x5a5);
			cmd(kSelf, kCmdUnload, 0x5a9);
			cmd(kSelf, kCmdUnload, 0x5af);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x10);
			var(0x11).set(0x11);
			cmd(kSelf, kCmdLoad, 0x11);
			cmd(kSelf, kCmdSetPos, 0x11, 0x77, 0x11f, 0x93);
			cmd(kSelf, kCmdPlay, 0x10);
			cmd(kSelf, kCmdSetPos, 0x10, 0x77, 0x11f, 0x93);
			cmd(kSelf, kCmdLoad, 0x5a4);
			cmd(kSelf, kCmdLoad, 0x5a8);
			cmd(kSelf, kCmdLoad, 0x5ae);
			cmd(kSelf, kCmdLoad, 0x5a5);
			cmd(kSelf, kCmdLoad, 0x5a9);
			cmd(kSelf, kCmdLoad, 0x5af);
		}
		break;
	case 0x17:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x36);
			var(0x11).set(0x38);
			cmd(kSelf, kCmdPlay, 0x36);
			cmd(kSelf, kCmdSetPos, 0x36, 0x80, 0x128, 0x93);
			cmd(kSelf, kCmdLoad, 0x38);
			cmd(kSelf, kCmdSetPos, 0x38, 0x80, 0x128, 0x93);
			cmd(kSelf, kCmdLoad, 0x37);
			cmd(kSelf, kCmdSetPos, 0x37, 0, 0x11b, 0xf7);
			cmd(kSelf, kCmdLoad, 0x5be);
			cmd(kSelf, kCmdLoad, 0x5ba);
			cmd(kSelf, kCmdLoad, 0x5c2);
			cmd(kSelf, kCmdLoad, 0x5c6);
			cmd(kSelf, kCmdLoad, 0x5bf);
			cmd(kSelf, kCmdLoad, 0x5bb);
			cmd(kSelf, kCmdLoad, 0x5c3);
			cmd(kSelf, kCmdLoad, 0x5c7);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x5bc) {
				send(kSelf, kMsgSay, 0x5bf05be, 0xffff);
			} else if (msg.p2 == 0x5be) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x18:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x3f);
			var(0x11).set(0x40);
			cmd(kSelf, kCmdPlay, 0x3f);
			cmd(kSelf, kCmdSetPos, 0x3f, 0x12f, 0xbb, 0x93);
			cmd(kSelf, kCmdLoad, 0x40);
			cmd(kSelf, kCmdSetPos, 0x40, 0x12f, 0xba, 0x93);
			cmd(kSelf, kCmdLoad, 0x5cf);
			cmd(kSelf, kCmdLoad, 0x5d5);
			cmd(kSelf, kCmdLoad, 0x5d7);
			cmd(kSelf, kCmdLoad, 0x5d0);
			cmd(kSelf, kCmdLoad, 0x5d6);
			cmd(kSelf, kCmdLoad, 0x5d8);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x5d3) {
				if (var(0x2d, 0x4a).get() == 0 && (uint32)send(32000, kMsgWhoIsDragged) >> 16 != 0x2e2)
					send(kSelf, kMsgSay, 0x5d605d5, 0x10);
				else
					send(kSelf, kMsgSay, 0x5d805d7);
			} else if (msg.p2 == 0x5d9) {
				send(kSelf, kMsgSay, 0x5d805d7);
			}
		}
		break;
	case 0x19:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x59);
			var(0x11).set(0x5a);
			cmd(kSelf, kCmdPlay, 0x59);
			cmd(kSelf, kCmdSetPos, 0x59, 0x172, 0x117, 0x93);
			cmd(kSelf, kCmdLoad, 0x5a);
			cmd(kSelf, kCmdSetPos, 0x5a, 0x171, 0x117, 0x93);
			cmd(kSelf, kCmdLoad, 0x5b, 0x19);
			cmd(kSelf, kCmdSetPos, 0x5b, 0x10e, 0xa9, 0x93);
			cmd(kSelf, kCmdLoad, 0x5c, 0x19);
			cmd(kSelf, kCmdSetPos, 0x5c, 0x177, 0x8c, 0x93);
			cmd(kSelf, kCmdLoad, 0x5d);
			cmd(kSelf, kCmdSetPos, 0x5d, 0x14c, 0x11d, 0x93);
			cmd(kSelf, kCmdLoad, 0x5e);
			cmd(kSelf, kCmdSetPos, 0x5e, 0x189, 0x121, 0x93);
			cmd(kSelf, kCmdLoad, 0x5e5);
			cmd(kSelf, kCmdLoad, 0x5ed);
			cmd(kSelf, kCmdLoad, 0x5e7);
			cmd(kSelf, kCmdLoad, 0x5e9);
			cmd(kSelf, kCmdLoad, 0x5f1);
			cmd(kSelf, kCmdLoad, 0x5e6);
			cmd(kSelf, kCmdLoad, 0x5ee);
			cmd(kSelf, kCmdLoad, 0x5e8);
			cmd(kSelf, kCmdLoad, 0x5ea);
			cmd(kSelf, kCmdLoad, 0x5f2);
		}
		break;
	case 0x1a:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x6d);
			var(0x11).set(0x6e);
			cmd(kSelf, kCmdPlay, 0x6d);
			cmd(kSelf, kCmdSetPos, 0x6d, 0x64, 0x105, 0xaa);
			cmd(kSelf, kCmdLoad, 0x6e);
			cmd(kSelf, kCmdSetPos, 0x6e, 0x64, 0x105, 0xaa);
			cmd(kSelf, kCmdLoad, 0x5fb);
			cmd(kSelf, kCmdLoad, 0x5fc);
		}
		break;
	case 0x1b:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0xec);
			var(0x11).set(0xef);
			cmd(kSelf, kCmdPlay, 0xec);
			cmd(kSelf, kCmdSetPos, 0xec, 0x67, 0xf3, 0x93);
			cmd(kSelf, kCmdLoad, 0xef);
			cmd(kSelf, kCmdSetPos, 0xef, 0x65, 0xf3, 0x93);
			cmd(kSelf, kCmdLoad, 0xf0);
			cmd(kSelf, kCmdSetPos, 0xf0, 0x63, 0xdc, 0x93);
			cmd(kSelf, kCmdLoad, 0x625);
			cmd(kSelf, kCmdLoad, 0x629);
			cmd(kSelf, kCmdLoad, 0x62d);
			cmd(kSelf, kCmdLoad, 0x62f);
			cmd(kSelf, kCmdLoad, 0x626);
			cmd(kSelf, kCmdLoad, 0x62a);
			cmd(kSelf, kCmdLoad, 0x62e);
			cmd(kSelf, kCmdLoad, 0x630);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x627) {
				send(kSelf, kMsgSay, 0x62a0629, 0xffff);
			} else if (msg.p2 == 0x629) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x1c:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x104);
			var(0x11).set(0x105);
			cmd(kSelf, kCmdPlay, 0x104);
			cmd(kSelf, kCmdSetPos, 0x104, 0x10e, 0x116, 0x93);
			cmd(kSelf, kCmdLoad, 0x105);
			cmd(kSelf, kCmdSetPos, 0x105, 0x10e, 0x116, 0x93);
			cmd(kSelf, kCmdLoad, 0x106);
			cmd(kSelf, kCmdSetPos, 0x106, 0x10f, 0xf8, 0x93);
			cmd(kSelf, kCmdLoad, 0x63f);
			cmd(kSelf, kCmdLoad, 0x645);
			cmd(kSelf, kCmdLoad, 0x647);
			cmd(kSelf, kCmdLoad, 0x640);
			cmd(kSelf, kCmdLoad, 0x646);
			cmd(kSelf, kCmdLoad, 0x648);
			cmd(kSelf, kCmdLoad, 0x64b);
			cmd(kSelf, kCmdLoad, 0x651);
			cmd(kSelf, kCmdLoad, 0x657);
			cmd(kSelf, kCmdLoad, 0x65d);
			cmd(kSelf, kCmdLoad, 0x64c);
			cmd(kSelf, kCmdLoad, 0x652);
			cmd(kSelf, kCmdLoad, 0x658);
			cmd(kSelf, kCmdLoad, 0x65e);
		}
		break;
	case 0x1d:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			cmd(kSelf, kCmdLoad, 0x128);
			cmd(kSelf, kCmdSetPos, 0x128, 0x62, 0x112, 0x93);
			cmd(kSelf, kCmdLoad, 0x12a);
			cmd(kSelf, kCmdSetPos, 0x12a, 0x62, 0x10f, 0x93);
			cmd(kSelf, kCmdLoad, 0x129);
			cmd(kSelf, kCmdSetPos, 0x129, 0x7d, 0x119, 0xf7);
			if (var(0x89, 0x1d).get() == 0) {
				var(0x10).set(0x128);
				var(0x11).set(0x12a);
				cmd(kSelf, kCmdPlay, 0x128);
			} else {
				var(0x10).set(0x129);
				var(0x11).set(0x129);
				cmd(kSelf, kCmdPlay, 0x129);
			}
			cmd(kSelf, kCmdLoad, 0x670);
			cmd(kSelf, kCmdLoad, 0x674);
			cmd(kSelf, kCmdLoad, 0x671);
			cmd(kSelf, kCmdLoad, 0x675);
		}
		break;
	case 0x1f:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x182);
			var(0x11).set(0x183);
			cmd(kSelf, kCmdPlay, 0x182);
			cmd(kSelf, kCmdSetPos, 0x182, 0x9a, 0xed, 0x93);
			cmd(kSelf, kCmdLoad, 0x183);
			cmd(kSelf, kCmdSetPos, 0x183, 0x9a, 0xed, 0x93);
			cmd(kSelf, kCmdLoad, 0x187);
			cmd(kSelf, kCmdSetPos, 0x187, 0x26, 0x7b, 0x93);
			cmd(kSelf, kCmdLoad, 0x188);
			cmd(kSelf, kCmdSetPos, 0x188, 0x68, 0xf3, 0x93);
			cmd(kSelf, kCmdLoad, 0x189);
			cmd(kSelf, kCmdSetPos, 0x189, 0, 0xb4, 0x93);
			cmd(kSelf, kCmdLoad, 0x184);
			cmd(kSelf, kCmdSetPos, 0x184, 0xb6, 0x10, 0x93);
			cmd(kSelf, kCmdLoad, 0x694);
			cmd(kSelf, kCmdLoad, 0x698);
			cmd(kSelf, kCmdLoad, 0x6a0);
			cmd(kSelf, kCmdLoad, 0x695);
			cmd(kSelf, kCmdLoad, 0x699);
			cmd(kSelf, kCmdLoad, 0x6a1);
		} else if (msg.msg == 0x3b) {
			if (msg.p1 == 0x182)
				msg.result = 1;
		} else if (msg.msg == 0x3c) {
			if (msg.p1 == 0x182)
				send(15, kMsgSay, 0x6a106a0, 0x10);
		} else if (msg.msg == kMsgDropTest || msg.msg == kMsgDropTestAlt) {
			if (msg.p2 == 0x182)
				msg.result = (uint32)msg.p1 >> 16 == 0x37b;
		} else if (msg.msg == kMsgDrop) {
			if ((uint32)msg.p1 >> 16 == 0x37b && msg.p2 == 0x182) {
				// The fish rod (0x37b) is given back [unverified].
				uint16 item = msg.p1;
				cmd(item, kCmdStop, var(0x30, item).get());
				setCursor(0);
				var(0x2d, item).set(1);
				var(0x31, item).set(0);
				send(kGameState, kMsgGotoRoom, 0x20);
			}
		}
		break;
	case 0x20:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x193);
			var(0x11).set(0x194);
			cmd(kSelf, kCmdPlay, 0x193);
			cmd(kSelf, kCmdSetPos, 0x193, 0x8f, 0xd8, 0x93);
			cmd(kSelf, kCmdLoad, 0x194);
			cmd(kSelf, kCmdSetPos, 0x194, 0x8f, 0xd7, 0x93);
			cmd(kSelf, kCmdLoad, 0x6ac);
			cmd(kSelf, kCmdLoad, 0x6ad);
		}
		break;
	case 0x22:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x244);
			var(0x11).set(0x249);
			cmd(kSelf, kCmdPlay, 0x244);
			cmd(kSelf, kCmdSetPos, 0x244, 0x189, 0x121, 0x93);
			cmd(kSelf, kCmdLoad, 0x249);
			cmd(kSelf, kCmdSetPos, 0x249, 0x189, 0x121, 0x93);
			cmd(kSelf, kCmdLoad, 0x245);
			cmd(kSelf, kCmdSetPos, 0x245, 0x19f, 0x122, 0x93);
			cmd(kSelf, kCmdLoad, 0x24a);
			cmd(kSelf, kCmdSetPos, 0x24a, 0x19f, 0x122, 0x93);
			cmd(kSelf, kCmdLoad, 0x247);
			cmd(kSelf, kCmdSetPos, 0x247, 0x76, 0x2a, 0x93);
			cmd(kSelf, kCmdLoad, 0x246);
			cmd(kSelf, kCmdSetPos, 0x246, 0x10, 0x113, 0x93);
			cmd(kSelf, kCmdLoad, 0x729);
			cmd(kSelf, kCmdLoad, 0x72d);
			cmd(kSelf, kCmdLoad, 0x733);
			cmd(kSelf, kCmdLoad, 0x72a);
			cmd(kSelf, kCmdLoad, 0x72e);
			cmd(kSelf, kCmdLoad, 0x734);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x72b) {
				send(kSelf, kMsgSay, 0x72e072d, 0x10);
			} else if (msg.p2 == 0x731) {
				send(kSelf, kMsgSay, 0x7340733, 0x10);
			}
		}
		break;
	case 0x23:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x25e);
			var(0x11).set(0x25f);
			cmd(kSelf, kCmdPlay, 0x25e);
			cmd(kSelf, kCmdSetPos, 0x25e, 0x102, 0x11d, 0x93);
			cmd(kSelf, kCmdLoad, 0x25f);
			cmd(kSelf, kCmdSetPos, 0x25f, 0x102, 0x11d, 0x93);
			cmd(kSelf, kCmdLoad, 0x777);
			cmd(kSelf, kCmdLoad, 0x77d);
			cmd(kSelf, kCmdLoad, 0x781);
			cmd(kSelf, kCmdLoad, 0x778);
			cmd(kSelf, kCmdLoad, 0x77e);
			cmd(kSelf, kCmdLoad, 0x782);
		}
		break;
	case 0x24:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x274);
			var(0x11).set(0x275);
			cmd(kSelf, kCmdPlay, 0x274);
			cmd(kSelf, kCmdSetPos, 0x274, 0x110, 0x10e, 0x93);
			cmd(kSelf, kCmdLoad, 0x275);
			cmd(kSelf, kCmdSetPos, 0x275, 0x110, 0x10d, 0x93);
			cmd(kSelf, kCmdLoad, 0x27a);
			cmd(kSelf, kCmdSetPos, 0x27a, 0x118, 0x102, 0x93);
			cmd(kSelf, kCmdLoad, 0x276);
			cmd(kSelf, kCmdSetPos, 0x276, 0x118, 0x104, 0x93);
			cmd(kSelf, kCmdLoad, 0x277);
			cmd(kSelf, kCmdSetPos, 0x277, 0x118, 0xe8, 0x93);
			cmd(kSelf, kCmdLoad, 0x278);
			cmd(kSelf, kCmdSetPos, 0x278, 0xb8, 0xff, 0x93);
			cmd(kSelf, kCmdLoad, 0x279);
			cmd(kSelf, kCmdSetPos, 0x279, 0x53, 0xff, 0x93);
			cmd(kSelf, kCmdLoad, 0x78d);
			cmd(kSelf, kCmdLoad, 0x791);
			cmd(kSelf, kCmdLoad, 0x78e);
			cmd(kSelf, kCmdLoad, 0x792);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x78f) {
				send(kSelf, kMsgSay, 0x7920791, 0x10);
			}
		}
		break;
	case 0x25:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0);
			var(0x11).set(0);
			cmd(kSelf, kCmdLoad, 0xed);
			cmd(kSelf, kCmdSetPos, 0xed, 0x60, 0xee, 0x93);
			cmd(kSelf, kCmdLoad, 0x7b0);
			cmd(kSelf, kCmdLoad, 0x7b4);
			cmd(kSelf, kCmdLoad, 0x7b8);
			cmd(kSelf, kCmdLoad, 0x7bc);
			cmd(kSelf, kCmdLoad, 0x7b1);
			cmd(kSelf, kCmdLoad, 0x7b5);
			cmd(kSelf, kCmdLoad, 0x7b9);
			cmd(kSelf, kCmdLoad, 0x7bd);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x7b2) {
				send(kSelf, kMsgSay, 0x7b507b4, 0x10);
			}
		}
		break;
	case 0x26:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x399);
			var(0x11).set(0x39c);
			cmd(kSelf, kCmdPlay, 0x399);
			cmd(kSelf, kCmdSetPos, 0x399, 0x128, 0x12a, 0x93);
			cmd(kSelf, kCmdLoad, 0x39c);
			cmd(kSelf, kCmdSetPos, 0x39c, 0x128, 0x12a, 0x93);
			cmd(kSelf, kCmdLoad, 0x39f);
			cmd(kSelf, kCmdSetPos, 0x39f, 0x29, 0xe1, 0x93);
			cmd(kSelf, kCmdLoad, 0x39d);
			cmd(kSelf, kCmdSetPos, 0x39d, 0x71, 0xf5, 0x93);
			cmd(kSelf, kCmdLoad, 0x39e);
			cmd(kSelf, kCmdSetPos, 0x39e, 0x124, 0x116, 0x93);
			cmd(kSelf, kCmdLoad, 0x39a);
			cmd(kSelf, kCmdSetPos, 0x39a, 0x124, 0x116, 0x93);
			cmd(kSelf, kCmdLoad, 0x848);
			cmd(kSelf, kCmdLoad, 0x84a);
			cmd(kSelf, kCmdLoad, 0x84e);
			cmd(kSelf, kCmdLoad, 0x852);
			cmd(kSelf, kCmdLoad, 0x849);
			cmd(kSelf, kCmdLoad, 0x84b);
			cmd(kSelf, kCmdLoad, 0x84f);
			cmd(kSelf, kCmdLoad, 0x853);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x846) {
				send(kSelf, kMsgSay, 0x8490848, 0xffff);
			} else if (msg.p2 == 0x848) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x852) {
				cmd(0x26, kCmdEnable, 0x38f, 1);
			}
		}
		break;
	case 0x27:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0);
			var(0x11).set(0);
			cmd(kSelf, kCmdLoad, 0x874);
			cmd(kSelf, kCmdLoad, 0x876);
			cmd(kSelf, kCmdLoad, 0x87a);
			cmd(kSelf, kCmdLoad, 0x87e);
			cmd(kSelf, kCmdLoad, 0x875);
			cmd(kSelf, kCmdLoad, 0x877);
			cmd(kSelf, kCmdLoad, 0x87b);
			cmd(kSelf, kCmdLoad, 0x87f);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x872) {
				send(kSelf, kMsgSay, 0x8750874, 0xffff);
			} else if (msg.p2 == 0x874) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x878) {
				send(kSelf, kMsgSay, 0x87b087a, 0x10);
			} else if (msg.p2 == 0x87c) {
				send(kSelf, kMsgSay, 0x87f087e, 0x10);
			}
		}
		break;
	case 0x28:
	case 0x29:
	case 0x2a:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			cmd(kSelf, kCmdLoad, 0x88d);
			cmd(kSelf, kCmdLoad, 0x88f);
			cmd(kSelf, kCmdLoad, 0x891);
			cmd(kSelf, kCmdLoad, 0x893);
			cmd(kSelf, kCmdLoad, 0x895);
			cmd(kSelf, kCmdLoad, 0x896);
			cmd(kSelf, kCmdLoad, 0x897);
			cmd(kSelf, kCmdLoad, 0x898);
			cmd(kSelf, kCmdLoad, 0x899);
			cmd(kSelf, kCmdLoad, 0x88a);
			cmd(kSelf, kCmdLoad, 0x88b);
			cmd(kSelf, kCmdLoad, 0x88c);
		}
		break;
	case 0x2b:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x44e);
			var(0x11).set(0x450);
			cmd(kSelf, kCmdLoad, 0x450);
			cmd(kSelf, kCmdSetPos, 0x450, 0x196, 0x123, 0x93);
			cmd(kSelf, kCmdPlay, 0x44e);
			cmd(kSelf, kCmdSetPos, 0x44e, 0x197, 0x123, 0x93);
			cmd(kSelf, kCmdLoad, 0x451);
			cmd(kSelf, kCmdSetPos, 0x451, 0x1b1, 0x11a, 0x93);
			cmd(kSelf, kCmdLoad, 0x44f);
			cmd(kSelf, kCmdSetPos, 0x44f, 0x1b2, 0x11e, 0x93);
			cmd(kSelf, kCmdLoad, 0x452);
			cmd(kSelf, kCmdSetPos, 0x452, 0x196, 0x11e, 0x93);
			cmd(kSelf, kCmdLoad, 0x453);
			cmd(kSelf, kCmdSetPos, 0x453, 0x1ad, 0x113, 0x93);
			cmd(kSelf, kCmdLoad, 0x8a7);
			cmd(kSelf, kCmdLoad, 0x8af);
			cmd(kSelf, kCmdLoad, 0x8b3);
			cmd(kSelf, kCmdLoad, 0x8a8);
			cmd(kSelf, kCmdLoad, 0x8b0);
			cmd(kSelf, kCmdLoad, 0x8b4);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x8ad) {
				send(kSelf, kMsgSay, 0x8b008af, 0x10);
			}
		}
		break;
	case 0x2c:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x461);
			var(0x11).set(0x463);
			cmd(kSelf, kCmdPlay, 0x461);
			cmd(kSelf, kCmdSetPos, 0x461, 0x9f, 0x119, 0x93);
			cmd(kSelf, kCmdLoad, 0x463);
			cmd(kSelf, kCmdSetPos, 0x463, 0x9f, 0x119, 0x93);
			cmd(kSelf, kCmdLoad, 0x462);
			cmd(kSelf, kCmdSetPos, 0x462, 0, 0x11c, 0x93);
			cmd(kSelf, kCmdLoad, 0x8cd);
			cmd(kSelf, kCmdLoad, 0x8d3);
			cmd(kSelf, kCmdLoad, 0x8cf);
			cmd(kSelf, kCmdLoad, 0x8ce);
			cmd(kSelf, kCmdLoad, 0x8d4);
			cmd(kSelf, kCmdLoad, 0x8d0);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x8d1) {
				send(kSelf, kMsgSay, 0x8d408d3, 0xffff);
			} else if (msg.p2 == 0x8d3) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x2d:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x473);
			var(0x11).set(0x476);
			cmd(kSelf, kCmdLoad, 0x476);
			cmd(kSelf, kCmdSetPos, 0x476, 0xff, 0x113, 0x93);
			cmd(kSelf, kCmdLoad, 0x477);
			cmd(kSelf, kCmdSetPos, 0x477, 0x14a, 0x102, 0x93);
			cmd(kSelf, kCmdPlay, 0x473);
			cmd(kSelf, kCmdSetPos, 0x473, 0x100, 0x113, 0x93);
			cmd(kSelf, kCmdLoad, 0x474);
			cmd(kSelf, kCmdSetPos, 0x474, 0x14a, 0x103, 0x93);
			cmd(kSelf, kCmdLoad, 0x475);
			cmd(kSelf, kCmdSetPos, 0x475, 0x26, 0xd7, 0x93);
			cmd(kSelf, kCmdLoad, 0x8e0);
			cmd(kSelf, kCmdLoad, 0x8e2);
			cmd(kSelf, kCmdLoad, 0x8e4);
			cmd(kSelf, kCmdLoad, 0x8e6);
			cmd(kSelf, kCmdLoad, 0x8e1);
			cmd(kSelf, kCmdLoad, 0x8e3);
			cmd(kSelf, kCmdLoad, 0x8e5);
			cmd(kSelf, kCmdLoad, 0x8e7);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x8de) {
				send(kSelf, kMsgSay, 0x8e108e0, 0xffff);
			} else if (msg.p2 == 0x8e0) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x2e:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0x491);
			cmd(kSelf, kCmdUnload, 0x488);
			cmd(kSelf, kCmdUnload, 0x8f4);
			cmd(kSelf, kCmdUnload, 0x8f8);
			cmd(kSelf, kCmdUnload, 0x8fa);
			cmd(kSelf, kCmdUnload, 0x8f5);
			cmd(kSelf, kCmdUnload, 0x8f9);
			cmd(kSelf, kCmdUnload, 0x8fb);
			cmd(kSelf, kCmdUnload, 0x47e);
			cmd(kSelf, kCmdUnload, 0x47f);
			cmd(kSelf, kCmdUnload, 0x480);
			cmd(kSelf, kCmdUnload, 0x481);
			cmd(kSelf, kCmdUnload, 0x482);
			cmd(kSelf, kCmdUnload, 0x483);
			cmd(kSelf, kCmdUnload, 0x484);
			cmd(kSelf, kCmdUnload, 0x485);
			cmd(kSelf, kCmdUnload, 0x486);
			cmd(kSelf, kCmdUnload, 0x487);
			cmd(kSelf, kCmdUnload, 0x489);
			cmd(kSelf, kCmdUnload, 0x48a);
			cmd(kSelf, kCmdUnload, 0x48b);
			cmd(kSelf, kCmdUnload, 0x48c);
			cmd(kSelf, kCmdUnload, 0x48d);
			cmd(kSelf, kCmdUnload, 0x48e);
			cmd(kSelf, kCmdUnload, 0x48f);
			cmd(kSelf, kCmdUnload, 0x490);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x488);
			var(0x11).set(0x491);
			cmd(kSelf, kCmdLoad, 0x491);
			cmd(kSelf, kCmdSetPos, 0x491, 0x4c, 0x5e, 0x93);
			cmd(kSelf, kCmdPlay, 0x488);
			cmd(kSelf, kCmdSetPos, 0x488, 0x4c, 0x5e, 0x93);
			cmd(kSelf, kCmdLoad, 0x8f4);
			cmd(kSelf, kCmdLoad, 0x8f8);
			cmd(kSelf, kCmdLoad, 0x8fa);
			cmd(kSelf, kCmdLoad, 0x8f5);
			cmd(kSelf, kCmdLoad, 0x8f9);
			cmd(kSelf, kCmdLoad, 0x8fb);
			cmd(kSelf, kCmdLoad, 0x47e);
			cmd(kSelf, kCmdLoad, 0x47f);
			cmd(kSelf, kCmdLoad, 0x480);
			cmd(kSelf, kCmdLoad, 0x481);
			cmd(kSelf, kCmdLoad, 0x482);
			cmd(kSelf, kCmdLoad, 0x483);
			cmd(kSelf, kCmdLoad, 0x484);
			cmd(kSelf, kCmdLoad, 0x485);
			cmd(kSelf, kCmdLoad, 0x486);
			cmd(kSelf, kCmdLoad, 0x487);
			cmd(kSelf, kCmdLoad, 0x489);
			cmd(kSelf, kCmdLoad, 0x48a);
			cmd(kSelf, kCmdLoad, 0x48b);
			cmd(kSelf, kCmdLoad, 0x48c);
			cmd(kSelf, kCmdLoad, 0x48d);
			cmd(kSelf, kCmdLoad, 0x48e);
			cmd(kSelf, kCmdLoad, 0x48f);
			cmd(kSelf, kCmdLoad, 0x490);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x8f2) {
				send(kSelf, kMsgSay, 0x8f508f4, 0x10);
			}
		}
		break;
	case 0x2f:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0x4c4);
			cmd(kSelf, kCmdUnload, 0x4c5);
			cmd(kSelf, kCmdUnload, 0x908);
			cmd(kSelf, kCmdUnload, 0x90c);
			cmd(kSelf, kCmdUnload, 0x912);
			cmd(kSelf, kCmdUnload, 0x909);
			cmd(kSelf, kCmdUnload, 0x90d);
			cmd(kSelf, kCmdUnload, 0x913);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x4c4);
			var(0x11).set(0x4c4);
			cmd(kSelf, kCmdPlay, 0x4c4);
			cmd(kSelf, kCmdSetPos, 0x4c4, 0x48, 0x169, 0x93);
			cmd(kSelf, kCmdLoad, 0x4c5);
			cmd(kSelf, kCmdSetPos, 0x4c5, 0x48, 0x167, 0x93);
			cmd(kSelf, kCmdLoad, 0x908);
			cmd(kSelf, kCmdLoad, 0x90c);
			cmd(kSelf, kCmdLoad, 0x912);
			cmd(kSelf, kCmdLoad, 0x909);
			cmd(kSelf, kCmdLoad, 0x90d);
			cmd(kSelf, kCmdLoad, 0x913);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x90a) {
				send(kSelf, kMsgSay, 0x90d090c, 0x10);
			}
		}
		break;
	case 0x31:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x1ea);
			var(0x11).set(0x1eb);
			cmd(kSelf, kCmdLoad, 0x1eb);
			cmd(kSelf, kCmdSetPos, 0x1eb, 0x167, 0x11c, 0x93);
			cmd(kSelf, kCmdPlay, 0x1ea);
			cmd(kSelf, kCmdSetPos, 0x1ea, 0x167, 0x11c, 0x93);
			cmd(kSelf, kCmdLoad, 0x6c4);
			cmd(kSelf, kCmdLoad, 0x6c5);
		}
		break;
	case 0x32:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x4e7);
			var(0x11).set(0x4e8);
			cmd(kSelf, kCmdLoad, 0x4e8);
			cmd(kSelf, kCmdSetPos, 0x4e8, 0x174, 0x107, 0x93);
			cmd(kSelf, kCmdLoad, 0x4e7);
			cmd(kSelf, kCmdSetPos, 0x4e7, 0x174, 0x107, 0x93);
			cmd(kSelf, kCmdLoad, 0x91f);
			cmd(kSelf, kCmdLoad, 0x920);
		}
		break;
	case 0x33:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x4f2);
			var(0x11).set(0x4f3);
			cmd(kSelf, kCmdLoad, 0x4f3);
			cmd(kSelf, kCmdSetPos, 0x4f3, 0x1b4, 0xfc, 0x93);
			cmd(kSelf, kCmdLoad, 0x4f2);
			cmd(kSelf, kCmdSetPos, 0x4f2, 0x1b4, 0xfc, 0x93);
			cmd(kSelf, kCmdLoad, 0x4f5);
			cmd(kSelf, kCmdSetPos, 0x4f5, 0x17c, 0x39, 0x93);
			cmd(kSelf, kCmdLoad, 0x4f4);
			cmd(kSelf, kCmdSetPos, 0x4f4, 0x1b4, 0xdd, 0x93);
			cmd(kSelf, kCmdLoad, 0x92f);
			cmd(kSelf, kCmdLoad, 0x933);
			cmd(kSelf, kCmdLoad, 0x937);
			cmd(kSelf, kCmdLoad, 0x5e9);
			cmd(kSelf, kCmdLoad, 0x930);
			cmd(kSelf, kCmdLoad, 0x934);
			cmd(kSelf, kCmdLoad, 0x938);
			cmd(kSelf, kCmdLoad, 0x5ea);
		}
		break;
	case 0x34:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x530);
			var(0x11).set(0x533);
			cmd(kSelf, kCmdLoad, 0x531);
			cmd(kSelf, kCmdSetPos, 0x531, 7, 0xf9, 0x93);
			cmd(kSelf, kCmdLoad, 0x532);
			cmd(kSelf, kCmdSetPos, 0x532, 7, 0xf9, 0x93);
			cmd(kSelf, kCmdPlay, 0x530);
			cmd(kSelf, kCmdSetPos, 0x530, 6, 0x10b, 0x93);
			cmd(kSelf, kCmdLoad, 0x533);
			cmd(kSelf, kCmdSetPos, 0x533, 6, 0x10b, 0x93);
			cmd(kSelf, kCmdLoad, 0xa20);
			cmd(kSelf, kCmdLoad, 0xa22);
			cmd(kSelf, kCmdLoad, 0xa26);
			cmd(kSelf, kCmdLoad, 0xa21);
			cmd(kSelf, kCmdLoad, 0xa23);
			cmd(kSelf, kCmdLoad, 0xa27);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa1e) {
				send(kSelf, kMsgSay, 0xa210a20, 0xffff);
			} else if (msg.p2 == 0xa20) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x35:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x506);
			var(0x11).set(0x508);
			cmd(kSelf, kCmdLoad, 0x508);
			cmd(kSelf, kCmdSetPos, 0x508, 0x103, 0xfe, 0x93);
			cmd(kSelf, kCmdLoad, 0x509);
			cmd(kSelf, kCmdSetPos, 0x509, 0xfd, 0xe5, 0x93);
			cmd(kSelf, kCmdPlay, 0x506);
			cmd(kSelf, kCmdSetPos, 0x506, 0x103, 0xfe, 0x93);
			cmd(kSelf, kCmdLoad, 0x507);
			cmd(kSelf, kCmdSetPos, 0x507, 0xcc, 0x25, 0x93);
			cmd(kSelf, kCmdLoad, 0x9ee);
			cmd(kSelf, kCmdLoad, 0x9f2);
			cmd(kSelf, kCmdLoad, 0x9f8);
			cmd(kSelf, kCmdLoad, 0x9ef);
			cmd(kSelf, kCmdLoad, 0x9f3);
			cmd(kSelf, kCmdLoad, 0x9f9);
		}
		break;
	case 0x36:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x519);
			var(0x11).set(0x51c);
			cmd(kSelf, kCmdPlay, 0x519);
			cmd(kSelf, kCmdSetPos, 0x519, 0x8d, 0xef, 0x93);
			cmd(kSelf, kCmdLoad, 0x51c);
			cmd(kSelf, kCmdSetPos, 0x51c, 0x8d, 0xee, 0x93);
			cmd(kSelf, kCmdLoad, 0x51a);
			cmd(kSelf, kCmdSetPos, 0x51a, 0x5c, 0xdc, 0x93);
			cmd(kSelf, kCmdLoad, 0x51d);
			cmd(kSelf, kCmdSetPos, 0x51d, 0xaa, 0, 0x93);
			cmd(kSelf, kCmdLoad, 0x51b);
			cmd(kSelf, kCmdSetPos, 0x51b, 0x66, 0xad, 0x93);
			cmd(kSelf, kCmdLoad, 0xa06);
			cmd(kSelf, kCmdLoad, 0xa0a);
			cmd(kSelf, kCmdLoad, 0xa0e);
			cmd(kSelf, kCmdLoad, 0xa12);
			cmd(kSelf, kCmdLoad, 0xa07);
			cmd(kSelf, kCmdLoad, 0xa0b);
			cmd(kSelf, kCmdLoad, 0xa0f);
			cmd(kSelf, kCmdLoad, 0xa13);
		}
		break;
	case 0x37:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0x233);
			cmd(kSelf, kCmdUnload, 0x232);
			cmd(kSelf, kCmdUnload, 0x6dc);
			cmd(kSelf, kCmdUnload, 0x6e0);
			cmd(kSelf, kCmdUnload, 0x6e4);
			cmd(kSelf, kCmdUnload, 0x6dd);
			cmd(kSelf, kCmdUnload, 0x6e1);
			cmd(kSelf, kCmdUnload, 0x6e5);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x232);
			var(0x11).set(0x233);
			cmd(kSelf, kCmdLoad, 0x233);
			cmd(kSelf, kCmdSetPos, 0x233, 0x50, 0xf0, 0x93);
			cmd(kSelf, kCmdPlay, 0x232);
			cmd(kSelf, kCmdSetPos, 0x232, 0x50, 0xf5, 0x93);
			cmd(kSelf, kCmdLoad, 0x6dc);
			cmd(kSelf, kCmdLoad, 0x6e0);
			cmd(kSelf, kCmdLoad, 0x6e4);
			cmd(kSelf, kCmdLoad, 0x6dd);
			cmd(kSelf, kCmdLoad, 0x6e1);
			cmd(kSelf, kCmdLoad, 0x6e5);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x6de) {
				send(kSelf, kMsgSay, 0x6e106e0, 0x10);
			}
		}
		break;
	case 0x38:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x55d);
			var(0x11).set(0x55e);
			cmd(kSelf, kCmdPlay, 0x55d);
			cmd(kSelf, kCmdSetPos, 0x55d, 0x56, 0x108, 0x93);
			cmd(kSelf, kCmdLoad, 0x55e);
			cmd(kSelf, kCmdSetPos, 0x55e, 0x56, 0x108, 0x93);
			cmd(kSelf, kCmdLoad, 0x55f);
			cmd(kSelf, kCmdSetPos, 0x55f, 0x2b, 0x104, 0x93);
			cmd(kSelf, kCmdLoad, 0xa51);
			cmd(kSelf, kCmdLoad, 0xa5d);
			cmd(kSelf, kCmdLoad, 0xa63);
			cmd(kSelf, kCmdLoad, 0xa52);
			cmd(kSelf, kCmdLoad, 0xa5e);
			cmd(kSelf, kCmdLoad, 0xa64);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa4f) {
				send(kSelf, kMsgSay, 0xa520a51, 0x10);
			} else if (msg.p2 == 0xa5b) {
				send(kSelf, kMsgSay, 0xa5e0a5d, 0x38);
			}
		}
		break;
	case 0x39:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x574);
			var(0x11).set(0x576);
			cmd(kSelf, kCmdPlay, 0x574);
			cmd(kSelf, kCmdSetPos, 0x574, 0x12b, 0xf8, 0x93);
			cmd(kSelf, kCmdLoad, 0x576);
			cmd(kSelf, kCmdSetPos, 0x576, 0x12b, 0xf8, 0x93);
			cmd(kSelf, kCmdLoad, 0x575);
			cmd(kSelf, kCmdSetPos, 0x575, 0, 0xda, 0x93);
			cmd(kSelf, kCmdLoad, 0x577);
			cmd(kSelf, kCmdSetPos, 0x577, 0x129, 0xf4, 0x93);
			cmd(kSelf, kCmdLoad, 0xa67);
			cmd(kSelf, kCmdLoad, 0xa69);
			cmd(kSelf, kCmdLoad, 0xa75);
			cmd(kSelf, kCmdLoad, 0xa79);
			cmd(kSelf, kCmdLoad, 0xa68);
			cmd(kSelf, kCmdLoad, 0xa6a);
			cmd(kSelf, kCmdLoad, 0xa76);
			cmd(kSelf, kCmdLoad, 0xa7a);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa65) {
				send(kSelf, kMsgSay, 0xa680a67, 0xffff);
			} else if (msg.p2 == 0xa67) {
				send(kGameState, 10);
			} else if (msg.p2 == 0xa77) {
				send(kSelf, kMsgSay, 0xa7a0a79);
			}
		}
		break;
	case 0x3a:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x53f);
			var(0x11).set(0x540);
			cmd(kSelf, kCmdPlay, 0x53f);
			cmd(kSelf, kCmdSetPos, 0x53f, 0x47, 0x122, 0x93);
			cmd(kSelf, kCmdLoad, 0x540);
			cmd(kSelf, kCmdSetPos, 0x540, 0x47, 0x120, 0x93);
			cmd(kSelf, kCmdLoad, 0xa3b);
			cmd(kSelf, kCmdLoad, 0xa49);
			cmd(kSelf, kCmdLoad, 0xa41);
			cmd(kSelf, kCmdLoad, 0xa3c);
			cmd(kSelf, kCmdLoad, 0xa4a);
			cmd(kSelf, kCmdLoad, 0xa42);
		}
		break;
	case 0x3b:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x58b);
			var(0x11).set(0x58d);
			cmd(kSelf, kCmdPlay, 0x58b);
			cmd(kSelf, kCmdSetPos, 0x58b, 0x1aa, 0xb2, 0x93);
			cmd(kSelf, kCmdLoad, 0x58d);
			cmd(kSelf, kCmdSetPos, 0x58d, 0x1aa, 0xb1, 0x93);
			cmd(kSelf, kCmdLoad, 0x58c);
			cmd(kSelf, kCmdSetPos, 0x58c, 0xfc, 0x4d, 0x93);
			cmd(kSelf, kCmdLoad, 0x58e);
			cmd(kSelf, kCmdSetPos, 0x58e, 0xfc, 0xd8, 0x93);
			cmd(kSelf, kCmdLoad, 0x58f);
			cmd(kSelf, kCmdSetPos, 0x58f, 0xd2, 0xa7, 0x93);
			cmd(kSelf, kCmdLoad, 0x590);
			cmd(kSelf, kCmdSetPos, 0x590, 0x16a, 0x36, 0x93);
			cmd(kSelf, kCmdLoad, 0x591);
			cmd(kSelf, kCmdSetPos, 0x591, 0x176, 0xd5, 0x93);
			cmd(kSelf, kCmdLoad, 0xa87);
			cmd(kSelf, kCmdLoad, 0xa8f);
			cmd(kSelf, kCmdLoad, 0xa91);
			cmd(kSelf, kCmdLoad, 0xa95);
			cmd(kSelf, kCmdLoad, 0xa99);
			cmd(kSelf, kCmdLoad, 0xa88);
			cmd(kSelf, kCmdLoad, 0xa90);
			cmd(kSelf, kCmdLoad, 0xa92);
			cmd(kSelf, kCmdLoad, 0xa96);
			cmd(kSelf, kCmdLoad, 0xa9a);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa93) {
				send(kSelf, kMsgSay, 0xa960a95, 0x3b);
			}
		} else if (msg.msg == kMsgAnimDone) {
			if (msg.p1 == 0x58f) {
				var(0x10).set(0x58c);
				var(0x11).set(0x58e);
				cmd(kSelf, kCmdPlay, 0x58c);
			}
		}
		break;
	default:
		break;
	}
	if (msg.handled == 0)
		SpeakerScript::handleMessage(msg);
}

/** Object 16 (vtable 0x4b5024, FUN_00446fd0), Myggen [unverified]: his animations and voice in each room. */
class Object16 : public SpeakerScript {
public:
	Object16(GameObject *obj) : SpeakerScript(obj) {}
	void handleMessage(Message &msg) override;
};

void Object16::handleMessage(Message &msg) {
	switch (var(1, kGameState).get()) {
	case 0x16:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0x13);
			cmd(kSelf, kCmdUnload, 0x12);
			cmd(kSelf, kCmdUnload, 0x5a6);
			cmd(kSelf, kCmdUnload, 0x5aa);
			cmd(kSelf, kCmdUnload, 0x5ac);
			cmd(kSelf, kCmdUnload, 0x5b0);
			cmd(kSelf, kCmdUnload, 0x5a7);
			cmd(kSelf, kCmdUnload, 0x5ab);
			cmd(kSelf, kCmdUnload, 0x5ad);
			cmd(kSelf, kCmdUnload, 0x5b1);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x12);
			var(0x11).set(0x13);
			cmd(kSelf, kCmdLoad, 0x13);
			cmd(kSelf, kCmdSetPos, 0x13, 0xd, 0xe4, 0x92);
			cmd(kSelf, kCmdPlay, 0x12);
			cmd(kSelf, kCmdSetPos, 0x12, 0xd, 0xe6, 0x92);
			cmd(kSelf, kCmdLoad, 0x5a6);
			cmd(kSelf, kCmdLoad, 0x5aa);
			cmd(kSelf, kCmdLoad, 0x5ac);
			cmd(kSelf, kCmdLoad, 0x5b0);
			cmd(kSelf, kCmdLoad, 0x5a7);
			cmd(kSelf, kCmdLoad, 0x5ab);
			cmd(kSelf, kCmdLoad, 0x5ad);
			cmd(kSelf, kCmdLoad, 0x5b1);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x5a4) {
				send(kSelf, kMsgSay, 0x5a705a6, 0x10);
			} else if (msg.p2 == 0x5a6) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x5a8) {
				send(kSelf, kMsgSay, 0x5ab05aa);
			}
		}
		break;
	case 0x17:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x39);
			var(0x11).set(0x3b);
			cmd(kSelf, kCmdPlay, 0x39);
			cmd(kSelf, kCmdSetPos, 0x39, 0x20, 0xc2, 0x92);
			cmd(kSelf, kCmdLoad, 0x3b);
			cmd(kSelf, kCmdSetPos, 0x3b, 0x16, 0xbf, 0x92);
			cmd(kSelf, kCmdLoad, 0x3a);
			cmd(kSelf, kCmdSetPos, 0x3a, 0, 0xbb, 0x92);
			cmd(kSelf, kCmdLoad, 0x5bc);
			cmd(kSelf, kCmdLoad, 0x5c0);
			cmd(kSelf, kCmdLoad, 0x5c4);
			cmd(kSelf, kCmdLoad, 0x5bd);
			cmd(kSelf, kCmdLoad, 0x5c1);
			cmd(kSelf, kCmdLoad, 0x5c5);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x5c2) {
				send(kSelf, kMsgSay, 0x5c505c4);
			}
		}
		break;
	case 0x18:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x49);
			var(0x11).set(0x4c);
			cmd(kSelf, kCmdPlay, 0x49);
			cmd(kSelf, kCmdSetPos, 0x49, 0x1a9, 0x13, 0x92);
			cmd(kSelf, kCmdLoad, 0x4c);
			cmd(kSelf, kCmdSetPos, 0x4c, 0x1a9, 0x13, 0x92);
			cmd(kSelf, kCmdLoad, 0x4a);
			cmd(kSelf, kCmdSetPos, 0x4a, 0x54, 5, 0x92);
			cmd(kSelf, kCmdLoad, 0x5d1);
			cmd(kSelf, kCmdLoad, 0x5d9);
			cmd(kSelf, kCmdLoad, 0x5d2);
			cmd(kSelf, kCmdLoad, 0x5da);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x5cf) {
				send(kSelf, kMsgSay, 0x5d205d1, 0xffff);
			} else if (msg.p2 == 0x5d1) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x5d5) {
				send(kSelf, kMsgSay, 0x5da05d9, 0x13);
			}
		}
		break;
	case 0x19:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x60);
			var(0x11).set(0x61);
			cmd(kSelf, kCmdPlay, 0x60);
			cmd(kSelf, kCmdSetPos, 0x60, 0x150, 0x79, 0x92);
			cmd(kSelf, kCmdLoad, 0x61);
			cmd(kSelf, kCmdSetPos, 0x61, 0x150, 0x79, 0x92);
			cmd(kSelf, kCmdLoad, 0x62);
			cmd(kSelf, kCmdSetPos, 0x62, 0x166, 0xac, 0x92);
			cmd(kSelf, kCmdLoad, 0x5eb);
			cmd(kSelf, kCmdLoad, 0x5ef);
			cmd(kSelf, kCmdLoad, 0x5f3);
			cmd(kSelf, kCmdLoad, 0x5ec);
			cmd(kSelf, kCmdLoad, 0x5f0);
			cmd(kSelf, kCmdLoad, 0x5f4);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x5e5) {
				send(kSelf, kMsgSay, 0x5ec05eb, 0xffff);
			} else if (msg.p2 == 0x5eb) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x5f1) {
				send(kSelf, kMsgSay, 0x5f405f3, 0xffff);
			} else if (msg.p2 == 0x5f3) {
				send(kGameState, kMsgSwitchRoom);
			}
		}
		break;
	case 0x1a:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x70);
			var(0x11).set(0x71);
			cmd(kSelf, kCmdPlay, 0x70);
			cmd(kSelf, kCmdSetPos, 0x70, 0xfa, 0x79, 0x92);
			cmd(kSelf, kCmdLoad, 0x71);
			cmd(kSelf, kCmdSetPos, 0x71, 0xfa, 0x79, 0x92);
			cmd(kSelf, kCmdLoad, 0x5fd);
			cmd(kSelf, kCmdLoad, 0x5ff);
			cmd(kSelf, kCmdLoad, 0x601);
			cmd(kSelf, kCmdLoad, 0x603);
			cmd(kSelf, kCmdLoad, 0x5fe);
			cmd(kSelf, kCmdLoad, 0x600);
			cmd(kSelf, kCmdLoad, 0x602);
			cmd(kSelf, kCmdLoad, 0x604);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x5fb) {
				send(kSelf, kMsgSay, 0x5fe05fd, 0xffff);
			} else if (msg.p2 == 0x5fd) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x1b:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0xf2);
			var(0x11).set(0xf4);
			cmd(kSelf, kCmdPlay, 0xf2);
			cmd(kSelf, kCmdSetPos, 0xf2, 2, 0x87, 0x92);
			cmd(kSelf, kCmdLoad, 0xf4);
			cmd(kSelf, kCmdSetPos, 0xf4, 2, 0x87, 0x92);
			cmd(kSelf, kCmdLoad, 0x627);
			cmd(kSelf, kCmdLoad, 0x62b);
			cmd(kSelf, kCmdLoad, 0x637);
			cmd(kSelf, kCmdLoad, 0x631);
			cmd(kSelf, kCmdLoad, 0x628);
			cmd(kSelf, kCmdLoad, 0x62c);
			cmd(kSelf, kCmdLoad, 0x638);
			cmd(kSelf, kCmdLoad, 0x632);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x625) {
				send(kSelf, kMsgSay, 0x6280627, 0xf);
			}
		}
		break;
	case 0x1c:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x108);
			var(0x11).set(0x109);
			cmd(kSelf, kCmdPlay, 0x108);
			cmd(kSelf, kCmdSetPos, 0x108, 0xa5, 0xac, 0x92);
			cmd(kSelf, kCmdLoad, 0x109);
			cmd(kSelf, kCmdSetPos, 0x109, 0xa5, 0xad, 0x92);
			cmd(kSelf, kCmdLoad, 0x641);
			cmd(kSelf, kCmdLoad, 0x66d);
			cmd(kSelf, kCmdLoad, 0x643);
			cmd(kSelf, kCmdLoad, 0x649);
			cmd(kSelf, kCmdLoad, 0x64d);
			cmd(kSelf, kCmdLoad, 0x653);
			cmd(kSelf, kCmdLoad, 0x659);
			cmd(kSelf, kCmdLoad, 0x65f);
			cmd(kSelf, kCmdLoad, 0x642);
			cmd(kSelf, kCmdLoad, 0x66e);
			cmd(kSelf, kCmdLoad, 0x644);
			cmd(kSelf, kCmdLoad, 0x64a);
			cmd(kSelf, kCmdLoad, 0x64e);
			cmd(kSelf, kCmdLoad, 0x654);
			cmd(kSelf, kCmdLoad, 0x65a);
			cmd(kSelf, kCmdLoad, 0x660);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x63f) {
				send(kSelf, kMsgSay, 0x6420641, 0xffff);
			} else if (msg.p2 == 0x641) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x647) {
				send(kSelf, kMsgSay, 0x64a0649);
			}
		}
		break;
	case 0x1d:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			cmd(kSelf, kCmdLoad, 0x12c);
			cmd(kSelf, kCmdSetPos, 0x12c, 0x10, 0xad, 0x92);
			cmd(kSelf, kCmdLoad, 0x12e);
			cmd(kSelf, kCmdSetPos, 0x12e, 0x10, 0xab, 0x92);
			cmd(kSelf, kCmdLoad, 0x12d);
			cmd(kSelf, kCmdSetPos, 0x12d, 0x65, 0xe9, 0xf6);
			if (var(0x89, 0x1d).get() == 0) {
				var(0x10).set(0x12c);
				var(0x11).set(0x12e);
				cmd(kSelf, kCmdPlay, 0x12c);
			} else {
				var(0x10).set(0x12d);
				var(0x11).set(0x12d);
				cmd(kSelf, kCmdPlay, 0x12d);
			}
			cmd(kSelf, kCmdLoad, 0x672);
			cmd(kSelf, kCmdLoad, 0x676);
			cmd(kSelf, kCmdLoad, 0x678);
			cmd(kSelf, kCmdLoad, 0x673);
			cmd(kSelf, kCmdLoad, 0x677);
			cmd(kSelf, kCmdLoad, 0x679);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x670) {
				send(kSelf, kMsgSay, 0x6730672, 0xffff);
			} else if (msg.p2 == 0x672) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x674) {
				send(kSelf, kMsgSay, 0x6770676);
			}
		}
		break;
	case 0x1f:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x18b);
			var(0x11).set(0x18c);
			cmd(kSelf, kCmdPlay, 0x18b);
			cmd(kSelf, kCmdSetPos, 0x18b, 0x27, 0x7b, 0x92);
			cmd(kSelf, kCmdLoad, 0x18c);
			cmd(kSelf, kCmdSetPos, 0x18c, 0x27, 0x7b, 0x92);
			cmd(kSelf, kCmdLoad, 0x6a2);
			cmd(kSelf, kCmdLoad, 0x69a);
			cmd(kSelf, kCmdLoad, 0x69c);
			cmd(kSelf, kCmdLoad, 0x6a3);
			cmd(kSelf, kCmdLoad, 0x69b);
			cmd(kSelf, kCmdLoad, 0x69d);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x6a0) {
				send(kSelf, kMsgSay, 0x6a306a2);
			}
		}
		break;
	case 0x20:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x196);
			var(0x11).set(0x197);
			cmd(kSelf, kCmdPlay, 0x196);
			cmd(kSelf, kCmdSetPos, 0x196, 0x187, 0x43, 0x92);
			cmd(kSelf, kCmdLoad, 0x197);
			cmd(kSelf, kCmdSetPos, 0x197, 0x187, 0x43, 0x92);
			cmd(kSelf, kCmdLoad, 0x6ae);
			cmd(kSelf, kCmdLoad, 0x6b0);
			cmd(kSelf, kCmdLoad, 0x6af);
			cmd(kSelf, kCmdLoad, 0x6b1);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x6ac) {
				send(kSelf, kMsgSay, 0x6af06ae, 0xffff);
			} else if (msg.p2 == 0x6ae) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x22:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x24c);
			var(0x11).set(0x24e);
			cmd(kSelf, kCmdLoad, 0x24e);
			cmd(kSelf, kCmdSetPos, 0x24e, 0x185, 0x9c, 0x92);
			cmd(kSelf, kCmdPlay, 0x24c);
			cmd(kSelf, kCmdSetPos, 0x24c, 0x185, 0x9c, 0x92);
			cmd(kSelf, kCmdLoad, 0x24d);
			cmd(kSelf, kCmdSetPos, 0x24d, 0x187, 0x9c, 0x92);
			cmd(kSelf, kCmdLoad, 0x72b);
			cmd(kSelf, kCmdLoad, 0x72f);
			cmd(kSelf, kCmdLoad, 0x731);
			cmd(kSelf, kCmdLoad, 0x735);
			cmd(kSelf, kCmdLoad, 0x737);
			cmd(kSelf, kCmdLoad, 0x72c);
			cmd(kSelf, kCmdLoad, 0x730);
			cmd(kSelf, kCmdLoad, 0x732);
			cmd(kSelf, kCmdLoad, 0x736);
			cmd(kSelf, kCmdLoad, 0x738);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x729) {
				var(0x11).set(0x24d);
				send(kSelf, kMsgSay, 0x72c072b, 0xf);
				var(0x11).set(0x24e);
			} else if (msg.p2 == 0x72d) {
				send(kSelf, kMsgSay, 0x730072f, 0xffff);
				send(0x22, 0x2f, 0x252, 0x251ffff, 0x8000c001);
			} else if (msg.p2 == 0x72f) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x733) {
				send(kSelf, kMsgSay, 0x7360735);
			}
		}
		break;
	case 0x23:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x261);
			var(0x11).set(0x262);
			cmd(kSelf, kCmdLoad, 0x262);
			cmd(kSelf, kCmdSetPos, 0x262, 0x69, 0xab, 0x92);
			cmd(kSelf, kCmdPlay, 0x261);
			cmd(kSelf, kCmdSetPos, 0x261, 0x68, 0xaa, 0x92);
			cmd(kSelf, kCmdLoad, 0x779);
			cmd(kSelf, kCmdLoad, 0x77b);
			cmd(kSelf, kCmdLoad, 0x77f);
			cmd(kSelf, kCmdLoad, 0x77a);
			cmd(kSelf, kCmdLoad, 0x77c);
			cmd(kSelf, kCmdLoad, 0x780);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x777) {
				send(kSelf, kMsgSay, 0x77a0779, 0xffff);
			} else if (msg.p2 == 0x779) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x24:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x27c);
			var(0x11).set(0x27d);
			cmd(kSelf, kCmdLoad, 0x27d);
			cmd(kSelf, kCmdSetPos, 0x27d, 0x12d, 0xcf, 0x92);
			cmd(kSelf, kCmdPlay, 0x27c);
			cmd(kSelf, kCmdSetPos, 0x27c, 0x12d, 0xcf, 0x92);
			cmd(kSelf, kCmdLoad, 0x282);
			cmd(kSelf, kCmdSetPos, 0x282, 0x12e, 0xc7, 0x92);
			cmd(kSelf, kCmdLoad, 0x27e);
			cmd(kSelf, kCmdSetPos, 0x27e, 0x12e, 0xc7, 0x92);
			cmd(kSelf, kCmdLoad, 0x27f);
			cmd(kSelf, kCmdSetPos, 0x27f, 0x117, 0xc0, 0x92);
			cmd(kSelf, kCmdLoad, 0x280);
			cmd(kSelf, kCmdSetPos, 0x280, 0xb8, 0xc9, 0x92);
			cmd(kSelf, kCmdLoad, 0x281);
			cmd(kSelf, kCmdSetPos, 0x281, 0x52, 0xca, 0x92);
			cmd(kSelf, kCmdLoad, 0x78f);
			cmd(kSelf, kCmdLoad, 0x793);
			cmd(kSelf, kCmdLoad, 0x795);
			cmd(kSelf, kCmdLoad, 0x790);
			cmd(kSelf, kCmdLoad, 0x794);
			cmd(kSelf, kCmdLoad, 0x796);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x78d) {
				send(kSelf, kMsgSay, 0x790078f, 0xf);
			} else if (msg.p2 == 0x791) {
				send(kSelf, kMsgSay, 0x7940793, 0xffff);
			} else if (msg.p2 == 0x793) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x25:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0);
			var(0x11).set(0);
			cmd(kSelf, kCmdLoad, 0xf3);
			cmd(kSelf, kCmdSetPos, 0xf3, 0xb, 0x4d, 0x92);
			cmd(kSelf, kCmdLoad, 0x7b2);
			cmd(kSelf, kCmdLoad, 0x7b6);
			cmd(kSelf, kCmdLoad, 0x7ba);
			cmd(kSelf, kCmdLoad, 0x7be);
			cmd(kSelf, kCmdLoad, 0x7c0);
			cmd(kSelf, kCmdLoad, 0x7b3);
			cmd(kSelf, kCmdLoad, 0x7b7);
			cmd(kSelf, kCmdLoad, 0x7bb);
			cmd(kSelf, kCmdLoad, 0x7bf);
			cmd(kSelf, kCmdLoad, 0x7c1);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x7b0) {
				send(kSelf, kMsgSay, 0x7b307b2, 0xf);
			} else if (msg.p2 == 0x7b4) {
				send(kSelf, kMsgSay, 0x7b707b6, 0xffff);
			} else if (msg.p2 == 0x7b6) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x7b8) {
				send(kSelf, kMsgSay, 0x7bb07ba);
			}
		}
		break;
	case 0x26:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x3a1);
			var(0x11).set(0x3a2);
			cmd(kSelf, kCmdPlay, 0x3a1);
			cmd(kSelf, kCmdSetPos, 0x3a1, 0x1dc, 0xc8, 0x92);
			cmd(kSelf, kCmdLoad, 0x3a2);
			cmd(kSelf, kCmdSetPos, 0x3a2, 0x1dc, 0xc8, 0x92);
			cmd(kSelf, kCmdLoad, 0x846);
			cmd(kSelf, kCmdLoad, 0x84c);
			cmd(kSelf, kCmdLoad, 0x850);
			cmd(kSelf, kCmdLoad, 0x854);
			cmd(kSelf, kCmdLoad, 0x856);
			cmd(kSelf, kCmdLoad, 0x858);
			cmd(kSelf, kCmdLoad, 0x85a);
			cmd(kSelf, kCmdLoad, 0x85c);
			cmd(kSelf, kCmdLoad, 0x840);
			cmd(kSelf, kCmdLoad, 0x842);
			cmd(kSelf, kCmdLoad, 0x844);
			cmd(kSelf, kCmdLoad, 0x847);
			cmd(kSelf, kCmdLoad, 0x84d);
			cmd(kSelf, kCmdLoad, 0x851);
			cmd(kSelf, kCmdLoad, 0x855);
			cmd(kSelf, kCmdLoad, 0x857);
			cmd(kSelf, kCmdLoad, 0x859);
			cmd(kSelf, kCmdLoad, 0x85b);
			cmd(kSelf, kCmdLoad, 0x85d);
			cmd(kSelf, kCmdLoad, 0x841);
			cmd(kSelf, kCmdLoad, 0x843);
			cmd(kSelf, kCmdLoad, 0x845);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x84a) {
				send(kSelf, kMsgSay, 0x84d084c, 0xffff);
			} else if (msg.p2 == 0x84c) {
				cmd(15, kCmdResume, 0x39d);
			} else if (msg.p2 == 0x84e) {
				send(kSelf, kMsgSay, 0x8510850, 0x12);
			}
		}
		break;
	case 0x27:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0);
			var(0x11).set(0);
			cmd(kSelf, kCmdLoad, 0x872);
			cmd(kSelf, kCmdLoad, 0x878);
			cmd(kSelf, kCmdLoad, 0x87c);
			cmd(kSelf, kCmdLoad, 0x880);
			cmd(kSelf, kCmdLoad, 0x873);
			cmd(kSelf, kCmdLoad, 0x879);
			cmd(kSelf, kCmdLoad, 0x87d);
			cmd(kSelf, kCmdLoad, 0x881);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x876) {
				send(kSelf, kMsgSay, 0x8790878, 0xf);
			} else if (msg.p2 == 0x87a) {
				send(kSelf, kMsgSay, 0x87d087c, 0xf);
			} else if (msg.p2 == 0x87e) {
				send(kSelf, kMsgSay, 0x8810880);
			}
		}
		break;
	case 0x28:
	case 0x29:
	case 0x2a:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			cmd(kSelf, kCmdLoad, 0x88e);
			cmd(kSelf, kCmdLoad, 0x890);
			cmd(kSelf, kCmdLoad, 0x892);
			cmd(kSelf, kCmdLoad, 0x894);
		}
		break;
	case 0x2b:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x455);
			var(0x11).set(0x457);
			cmd(kSelf, kCmdLoad, 0x457);
			cmd(kSelf, kCmdSetPos, 0x457, 0x1a4, 0xcd, 0x92);
			cmd(kSelf, kCmdPlay, 0x455);
			cmd(kSelf, kCmdSetPos, 0x455, 0x1a4, 0xcd, 0x92);
			cmd(kSelf, kCmdLoad, 0x458);
			cmd(kSelf, kCmdSetPos, 0x458, 0x194, 0xcf, 0x92);
			cmd(kSelf, kCmdLoad, 0x456);
			cmd(kSelf, kCmdSetPos, 0x456, 0x194, 0xcf, 0x92);
			cmd(kSelf, kCmdLoad, 0x459);
			cmd(kSelf, kCmdSetPos, 0x459, 0x180, 0xc7, 0x92);
			cmd(kSelf, kCmdLoad, 0x45a);
			cmd(kSelf, kCmdSetPos, 0x45a, 0x191, 0xd3, 0x92);
			cmd(kSelf, kCmdLoad, 0x8a9);
			cmd(kSelf, kCmdLoad, 0x8ab);
			cmd(kSelf, kCmdLoad, 0x8ad);
			cmd(kSelf, kCmdLoad, 0x8b1);
			cmd(kSelf, kCmdLoad, 0x8b5);
			cmd(kSelf, kCmdLoad, 0x8aa);
			cmd(kSelf, kCmdLoad, 0x8ac);
			cmd(kSelf, kCmdLoad, 0x8ae);
			cmd(kSelf, kCmdLoad, 0x8b2);
			cmd(kSelf, kCmdLoad, 0x8b6);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x8a7) {
				send(kSelf, kMsgSay, 0x8aa08a9, 0xffff);
			} else if (msg.p2 == 0x8a9) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x8b3) {
				send(kSelf, kMsgSay, 0x8b608b5);
			}
		}
		break;
	case 0x2c:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x465);
			var(0x11).set(0x467);
			cmd(kSelf, kCmdPlay, 0x465);
			cmd(kSelf, kCmdSetPos, 0x465, 0x77, 0xc3, 0x92);
			cmd(kSelf, kCmdLoad, 0x467);
			cmd(kSelf, kCmdSetPos, 0x467, 0x76, 0xc4, 0x92);
			cmd(kSelf, kCmdLoad, 0x466);
			cmd(kSelf, kCmdSetPos, 0x466, 0, 0xa9, 0x92);
			cmd(kSelf, kCmdLoad, 0x8d1);
			cmd(kSelf, kCmdLoad, 0x8d5);
			cmd(kSelf, kCmdLoad, 0x8d2);
			cmd(kSelf, kCmdLoad, 0x8d6);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x8cd) {
				send(kSelf, kMsgSay, 0x8d208d1, 0xf);
			}
		}
		break;
	case 0x2d:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0x47a);
			cmd(kSelf, kCmdUnload, 0x479);
			cmd(kSelf, kCmdUnload, 0x8de);
			cmd(kSelf, kCmdUnload, 0x8df);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x479);
			var(0x11).set(0x47a);
			cmd(kSelf, kCmdLoad, 0x47a);
			cmd(kSelf, kCmdSetPos, 0x47a, 0x151, 0x7f, 0x92);
			cmd(kSelf, kCmdPlay, 0x479);
			cmd(kSelf, kCmdSetPos, 0x479, 0x151, 0x7f, 0x92);
			cmd(kSelf, kCmdLoad, 0x8de);
			cmd(kSelf, kCmdLoad, 0x8df);
		}
		break;
	case 0x2e:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0x493);
			cmd(kSelf, kCmdUnload, 0x492);
			cmd(kSelf, kCmdUnload, 0x8f2);
			cmd(kSelf, kCmdUnload, 0x8f6);
			cmd(kSelf, kCmdUnload, 0x8fc);
			cmd(kSelf, kCmdUnload, 0x8f3);
			cmd(kSelf, kCmdUnload, 0x8f7);
			cmd(kSelf, kCmdUnload, 0x8fd);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x492);
			var(0x11).set(0x493);
			cmd(kSelf, kCmdLoad, 0x493);
			cmd(kSelf, kCmdSetPos, 0x493, 0xb, 0, 0x92);
			cmd(kSelf, kCmdPlay, 0x492);
			cmd(kSelf, kCmdSetPos, 0x492, 0xb, 0, 0x92);
			cmd(kSelf, kCmdLoad, 0x8f2);
			cmd(kSelf, kCmdLoad, 0x8f6);
			cmd(kSelf, kCmdLoad, 0x8fc);
			cmd(kSelf, kCmdLoad, 0x8f3);
			cmd(kSelf, kCmdLoad, 0x8f7);
			cmd(kSelf, kCmdLoad, 0x8fd);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x8f4) {
				send(kSelf, kMsgSay, 0x8f708f6, 0x10);
			} else if (msg.p2 == 0x8f6) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x8fa) {
				send(kSelf, kMsgSay, 0x8fd08fc, 0x2e);
			}
		}
		break;
	case 0x2f:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0x4c7);
			cmd(kSelf, kCmdUnload, 0x4c8);
			cmd(kSelf, kCmdUnload, 0x90a);
			cmd(kSelf, kCmdUnload, 0x90e);
			cmd(kSelf, kCmdUnload, 0x910);
			cmd(kSelf, kCmdUnload, 0x90b);
			cmd(kSelf, kCmdUnload, 0x90f);
			cmd(kSelf, kCmdUnload, 0x911);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x4c7);
			var(0x11).set(0x4c7);
			cmd(kSelf, kCmdPlay, 0x4c7);
			cmd(kSelf, kCmdSetPos, 0x4c7, 0x35, 0x151, 0x92);
			cmd(kSelf, kCmdLoad, 0x4c8);
			cmd(kSelf, kCmdSetPos, 0x4c8, 0x35, 0x151, 0x92);
			cmd(kSelf, kCmdLoad, 0x90a);
			cmd(kSelf, kCmdLoad, 0x90e);
			cmd(kSelf, kCmdLoad, 0x910);
			cmd(kSelf, kCmdLoad, 0x90b);
			cmd(kSelf, kCmdLoad, 0x90f);
			cmd(kSelf, kCmdLoad, 0x911);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x908) {
				send(kSelf, kMsgSay, 0x90b090a, 0xf);
			} else if (msg.p2 == 0x90c) {
				send(kSelf, kMsgSay, 0x90f090e, 0x2f);
			}
		}
		break;
	case 0x31:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x1ed);
			var(0x11).set(0x1ee);
			cmd(kSelf, kCmdLoad, 0x1ee);
			cmd(kSelf, kCmdSetPos, 0x1ee, 0x102, 0xc9, 0x92);
			cmd(kSelf, kCmdPlay, 0x1ed);
			cmd(kSelf, kCmdSetPos, 0x1ed, 0x102, 0xc9, 0x92);
			cmd(kSelf, kCmdLoad, 0x6c6);
			cmd(kSelf, kCmdLoad, 0x6c8);
			cmd(kSelf, kCmdLoad, 0x6ca);
			cmd(kSelf, kCmdLoad, 0x6cc);
			cmd(kSelf, kCmdLoad, 0x6ce);
			cmd(kSelf, kCmdLoad, 0x6d0);
			cmd(kSelf, kCmdLoad, 0x6d2);
			cmd(kSelf, kCmdLoad, 0x6c7);
			cmd(kSelf, kCmdLoad, 0x6c9);
			cmd(kSelf, kCmdLoad, 0x6cb);
			cmd(kSelf, kCmdLoad, 0x6cd);
			cmd(kSelf, kCmdLoad, 0x6cf);
			cmd(kSelf, kCmdLoad, 0x6d1);
			cmd(kSelf, kCmdLoad, 0x6d3);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x6c4) {
				send(kSelf, kMsgSay, 0x6c706c6, 0xffff);
			} else if (msg.p2 == 0x6c6) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x32:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x4ea);
			var(0x11).set(0x4eb);
			cmd(kSelf, kCmdLoad, 0x4eb);
			cmd(kSelf, kCmdSetPos, 0x4eb, 0x65, 0x92, 0x92);
			cmd(kSelf, kCmdLoad, 0x4ea);
			cmd(kSelf, kCmdSetPos, 0x4ea, 0x67, 0x92, 0x92);
			cmd(kSelf, kCmdUnload, 0x921);
			cmd(kSelf, kCmdUnload, 0x923);
			cmd(kSelf, kCmdUnload, 0x925);
			cmd(kSelf, kCmdUnload, 0x922);
			cmd(kSelf, kCmdUnload, 0x924);
			cmd(kSelf, kCmdUnload, 0x926);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x91f) {
				send(kSelf, kMsgSay, 0x9220921, 0xffff);
			} else if (msg.p2 == 0x921) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x92d) {
				send(kSelf, kMsgSay, 0x9260925, 0x32);
			}
		}
		break;
	case 0x33:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x4f7);
			var(0x11).set(0x4f8);
			cmd(kSelf, kCmdLoad, 0x4f8);
			cmd(kSelf, kCmdSetPos, 0x4f8, 0x1d5, 0xc2, 0x92);
			cmd(kSelf, kCmdLoad, 0x4f7);
			cmd(kSelf, kCmdSetPos, 0x4f7, 0x1d5, 0xc2, 0x92);
			cmd(kSelf, kCmdLoad, 0x4f9);
			cmd(kSelf, kCmdSetPos, 0x4f9, 0x1d5, 0x8a, 0x92);
			cmd(kSelf, kCmdUnload, 0x931);
			cmd(kSelf, kCmdUnload, 0x935);
			cmd(kSelf, kCmdUnload, 0x932);
			cmd(kSelf, kCmdUnload, 0x936);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x92f) {
				send(kSelf, kMsgSay, 0x9320931, 0xffff);
			} else if (msg.p2 == 0x931 || msg.p2 == 0x935) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x933) {
				send(kSelf, kMsgSay, 0x9360935, 0xffff);
			}
		}
		break;
	case 0x34:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x535);
			var(0x11).set(0x537);
			cmd(kSelf, kCmdPlay, 0x535);
			cmd(kSelf, kCmdSetPos, 0x535, 0x14b, 0x62, 0x92);
			cmd(kSelf, kCmdLoad, 0x537);
			cmd(kSelf, kCmdSetPos, 0x537, 0x14b, 0x62, 0x92);
			cmd(kSelf, kCmdLoad, 0x536);
			cmd(kSelf, kCmdSetPos, 0x536, 0xec, 0x87, 0x92);
			cmd(kSelf, kCmdLoad, 0xa1e);
			cmd(kSelf, kCmdLoad, 0xa24);
			cmd(kSelf, kCmdLoad, 0xa1f);
			cmd(kSelf, kCmdLoad, 0xa25);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa22) {
				send(kSelf, kMsgSay, 0xa250a24);
			}
		}
		break;
	case 0x35:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x50b);
			var(0x11).set(0x50d);
			cmd(kSelf, kCmdLoad, 0x50d);
			cmd(kSelf, kCmdSetPos, 0x50d, 0xb1, 0xaf, 0x92);
			cmd(kSelf, kCmdLoad, 0x50b);
			cmd(kSelf, kCmdSetPos, 0x50b, 0xb1, 0xaf, 0x92);
			cmd(kSelf, kCmdLoad, 0x50e);
			cmd(kSelf, kCmdSetPos, 0x50e, 0xbe, 0xb7, 0x92);
			cmd(kSelf, kCmdLoad, 0x50c);
			cmd(kSelf, kCmdSetPos, 0x50c, 0x99, 0, 0x97);
			cmd(kSelf, kCmdLoad, 0x9f0);
			cmd(kSelf, kCmdLoad, 0x9f4);
			cmd(kSelf, kCmdLoad, 0x9f1);
			cmd(kSelf, kCmdLoad, 0x9f5);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x9ee) {
				send(kSelf, kMsgSay, 0x9f109f0, 0xffff);
			} else if (msg.p2 == 0x9f0) {
				send(kGameState, 10);
			} else if (msg.p2 == 0x9f2) {
				send(kSelf, kMsgSay, 0x9f509f4);
			}
		}
		break;
	case 0x36:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x51f);
			var(0x11).set(0x522);
			cmd(kSelf, kCmdLoad, 0x51f);
			cmd(kSelf, kCmdSetPos, 0x51f, 0x62, 0x70, 0x92);
			cmd(kSelf, kCmdLoad, 0x522);
			cmd(kSelf, kCmdSetPos, 0x522, 0x62, 0x70, 0x92);
			cmd(kSelf, kCmdLoad, 0x520);
			cmd(kSelf, kCmdSetPos, 0x520, 0, 0x68, 0x92);
			cmd(kSelf, kCmdLoad, 0x523);
			cmd(kSelf, kCmdSetPos, 0x523, 0xae, 0, 0x92);
			cmd(kSelf, kCmdLoad, 0x521);
			cmd(kSelf, kCmdSetPos, 0x521, 0, 0x68, 0x9a);
			cmd(kSelf, kCmdLoad, 0xa08);
			cmd(kSelf, kCmdLoad, 0xa0c);
			cmd(kSelf, kCmdLoad, 0xa10);
			cmd(kSelf, kCmdLoad, 0xa09);
			cmd(kSelf, kCmdLoad, 0xa0d);
			cmd(kSelf, kCmdLoad, 0xa11);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa06) {
				send(kSelf, kMsgSay, 0xa090a08, 0xffff);
			} else if (msg.p2 == 0xa08) {
				send(kGameState, 10);
			} else if (msg.p2 == 0xa0a) {
				send(kSelf, kMsgSay, 0xa0d0a0c);
			} else if (msg.p2 == 0xa0e) {
				send(kSelf, kMsgSay, 0xa110a10);
			}
		}
		break;
	case 0x37:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0x236);
			cmd(kSelf, kCmdUnload, 0x235);
			cmd(kSelf, kCmdUnload, 0x6de);
			cmd(kSelf, kCmdUnload, 0x6e2);
			cmd(kSelf, kCmdUnload, 0x6df);
			cmd(kSelf, kCmdUnload, 0x6e3);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x235);
			var(0x11).set(0x236);
			cmd(kSelf, kCmdLoad, 0x236);
			cmd(kSelf, kCmdSetPos, 0x236, 0x1c3, 0x55, 0x92);
			cmd(kSelf, kCmdPlay, 0x235);
			cmd(kSelf, kCmdSetPos, 0x235, 0x1c3, 0x55, 0x92);
			cmd(kSelf, kCmdLoad, 0x6de);
			cmd(kSelf, kCmdLoad, 0x6e2);
			cmd(kSelf, kCmdLoad, 0x6df);
			cmd(kSelf, kCmdLoad, 0x6e3);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0x6dc) {
				send(kSelf, kMsgSay, 0x6df06de, 0xf);
			} else if (msg.p2 == 0x6e0) {
				send(kSelf, kMsgSay, 0x6e306e2, 0x10);
			} else if (msg.p2 == 0x6e2) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x38:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x561);
			var(0x11).set(0x562);
			cmd(kSelf, kCmdPlay, 0x561);
			cmd(kSelf, kCmdSetPos, 0x561, 0x2d, 0x9a, 0x92);
			cmd(kSelf, kCmdLoad, 0x562);
			cmd(kSelf, kCmdSetPos, 0x562, 0x2d, 0x98, 0x92);
			cmd(kSelf, kCmdLoad, 0xa53);
			cmd(kSelf, kCmdLoad, 0xa55);
			cmd(kSelf, kCmdLoad, 0xa57);
			cmd(kSelf, kCmdLoad, 0xa59);
			cmd(kSelf, kCmdLoad, 0xa5b);
			cmd(kSelf, kCmdLoad, 0xa4d);
			cmd(kSelf, kCmdLoad, 0xa54);
			cmd(kSelf, kCmdLoad, 0xa56);
			cmd(kSelf, kCmdLoad, 0xa58);
			cmd(kSelf, kCmdLoad, 0xa5a);
			cmd(kSelf, kCmdLoad, 0xa5c);
			cmd(kSelf, kCmdLoad, 0xa4e);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa51) {
				send(kSelf, kMsgSay, 0xa540a53, 0x38);
			} else if (msg.p2 == 0xa63) {
				send(kSelf, kMsgSay, 0xa4e0a4d, 0x38);
			}
		}
		break;
	case 0x39:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x579);
			var(0x11).set(0x57b);
			cmd(kSelf, kCmdPlay, 0x579);
			cmd(kSelf, kCmdSetPos, 0x579, 0xcd, 0x9a, 0x92);
			cmd(kSelf, kCmdLoad, 0x57b);
			cmd(kSelf, kCmdSetPos, 0x57b, 0xcd, 0x9c, 0x92);
			cmd(kSelf, kCmdLoad, 0x57a);
			cmd(kSelf, kCmdSetPos, 0x57a, 0, 0x75, 0x92);
			cmd(kSelf, kCmdLoad, 0xa65);
			cmd(kSelf, kCmdLoad, 0xa6b);
			cmd(kSelf, kCmdLoad, 0xa6d);
			cmd(kSelf, kCmdLoad, 0xa6f);
			cmd(kSelf, kCmdLoad, 0xa71);
			cmd(kSelf, kCmdLoad, 0xa73);
			cmd(kSelf, kCmdLoad, 0xa77);
			cmd(kSelf, kCmdLoad, 0xa66);
			cmd(kSelf, kCmdLoad, 0xa6c);
			cmd(kSelf, kCmdLoad, 0xa6e);
			cmd(kSelf, kCmdLoad, 0xa70);
			cmd(kSelf, kCmdLoad, 0xa72);
			cmd(kSelf, kCmdLoad, 0xa74);
			cmd(kSelf, kCmdLoad, 0xa78);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa69) {
				send(kSelf, kMsgSay, 0xa6c0a6b);
			} else if (msg.p2 == 0xa75) {
				send(kSelf, kMsgSay, 0xa780a77, 0x39);
			}
		}
		break;
	case 0x3a:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x542);
			var(0x11).set(0x543);
			cmd(kSelf, kCmdLoad, 0x543);
			cmd(kSelf, kCmdSetPos, 0x543, 4, 0xba, 0x92);
			cmd(kSelf, kCmdPlay, 0x542);
			cmd(kSelf, kCmdSetPos, 0x542, 4, 0xbb, 0x92);
			cmd(kSelf, kCmdLoad, 0xa3d);
			cmd(kSelf, kCmdLoad, 0xa3f);
			cmd(kSelf, kCmdLoad, 0xa43);
			cmd(kSelf, kCmdLoad, 0xa3e);
			cmd(kSelf, kCmdLoad, 0xa40);
			cmd(kSelf, kCmdLoad, 0xa44);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa3b) {
				send(kSelf, kMsgSay, 0xa3e0a3d, 0xffff);
			} else if (msg.p2 == 0xa3d) {
				send(kGameState, 10);
			}
		}
		break;
	case 0x3b:
		if (msg.msg == kMsgLeave) {
			cmd(kSelf, kCmdUnload, 0);
		} else if (msg.msg == kMsgEnter) {
			var(0x10).set(0x593);
			var(0x11).set(0x595);
			cmd(kSelf, kCmdLoad, 0x595);
			cmd(kSelf, kCmdSetPos, 0x595, 0x1a5, 0x1f, 0x92);
			cmd(kSelf, kCmdPlay, 0x593);
			cmd(kSelf, kCmdSetPos, 0x593, 0x1a5, 0x1f, 0x92);
			cmd(kSelf, kCmdLoad, 0x594);
			cmd(kSelf, kCmdSetPos, 0x594, 0x80, 0x88, 0x92);
			cmd(kSelf, kCmdLoad, 0x596);
			cmd(kSelf, kCmdSetPos, 0x596, 0x80, 0x86, 0x92);
			cmd(kSelf, kCmdLoad, 0x597);
			cmd(kSelf, kCmdSetPos, 0x597, 0x7d, 0xaf, 0x92);
			cmd(kSelf, kCmdLoad, 0xa89);
			cmd(kSelf, kCmdLoad, 0xa93);
			cmd(kSelf, kCmdLoad, 0xa97);
			cmd(kSelf, kCmdLoad, 0xa9b);
			cmd(kSelf, kCmdLoad, 0xa9d);
			cmd(kSelf, kCmdLoad, 0xa9f);
			cmd(kSelf, kCmdLoad, 0xa8a);
			cmd(kSelf, kCmdLoad, 0xa94);
			cmd(kSelf, kCmdLoad, 0xa98);
			cmd(kSelf, kCmdLoad, 0xa9c);
			cmd(kSelf, kCmdLoad, 0xa9e);
			cmd(kSelf, kCmdLoad, 0xaa0);
		} else if (msg.msg == kMsgSaid) {
			if (msg.p2 == 0xa87) {
				send(kSelf, kMsgSay, 0xa8a0a89, 0x10);
			} else if (msg.p2 == 0xa89) {
				send(kGameState, 10);
			} else if (msg.p2 == 0xa91) {
				send(kSelf, kMsgSay, 0xa940a93, 0xf);
			} else if (msg.p2 == 0xa99) {
				send(kSelf, kMsgSay, 0xa9c0a9b, 0x3b);
			} else if (msg.p2 == 0xaa1) {
				send(kSelf, kMsgSay, 0xa9e0a9d, 0x3b);
			}
		} else if (msg.msg == kMsgAnimDone) {
			if (msg.p1 == 0x597) {
				var(0x10).set(0x594);
				var(0x11).set(0x596);
				cmd(kSelf, kCmdPlay, 0x594);
				send(15, kMsgSay, 0xa920a91, 0x10);
			}
		}
		break;
	default:
		break;
	}

	// Switch between the idle and the talking animation at the same frame.
	if (msg.msg == kMsgSay) {
		if (var(0x11).get() != 0) {
			VarRef from = var(0x10);
			VarRef to = var(0x11);
			int32 frame = cmd(kSelf, kCmdGetFrame, from.get());
			cmd(kSelf, kCmdGotoFrame, to.get(), frame);
		}
	} else if (msg.msg == kMsgSoundDone) {
		if (msg.p1 == var(0x12).get() && var(0x11).get() != 0) {
			VarRef from = var(0x11);
			VarRef to = var(0x10);
			int32 frame = cmd(kSelf, kCmdGetFrame, from.get());
			cmd(kSelf, kCmdGotoFrame, to.get(), frame);
		}
	}

	if (msg.handled == 0)
		SpeakerScript::handleMessage(msg);
}

/**
 * Object 18 (vtable 0x4b4fe4, FUN_00469e00), the narrator [unverified]:
 * loads its sounds in each room. It has no animations.
 */
class Object18 : public SpeakerScript {
public:
	Object18(GameObject *obj) : SpeakerScript(obj) {}
	void handleMessage(Message &msg) override;

private:
	struct RoomSounds {
		int32 room;
		bool unloadEach; ///< on leaving, unload the sounds one by one (otherwise all children)
		uint16 sounds[32];  ///< 0-terminated
	};
	static const RoomSounds kRoomSounds[];
};

const Object18::RoomSounds Object18::kRoomSounds[] = {
	{ 0x16, true, { 0x5b2, 0x5b4, 0x5b6, 0x5b8, 0x5b3, 0x5b5, 0x5b7, 0x5b9, 0 } },
	{ 0x17, true, { 0x5c8, 0x5ca, 0x5cc, 0x5c9, 0x5cb, 0x5cd, 0 } },
	{ 0x18, false, { 0x5df, 0x5e1, 0x5e0, 0x5e2, 0 } },
	{ 0x19, false, { 0x5f5, 0x5f7, 0x5f9, 0x5f6, 0x5f8, 0x5fa, 0 } },
	{ 0x1a, false, { 0x613, 0x615, 0x617, 0x619, 0x61b, 0x61d, 0x61f, 0x621, 0x623, 0x605, 0x607, 0x609, 0x60b, 0x60d, 0x60f, 0x611, 0x614, 0x616, 0x618, 0x61a, 0x61c, 0x61e, 0x620, 0x622, 0x624, 0x606, 0x608, 0x60c, 0x60e, 0x610, 0x612, 0 } },
	{ 0x1b, false, { 0x633, 0x635, 0x639, 0x63b, 0x634, 0x636, 0x63a, 0x63c, 0 } },
	{ 0x1c, false, { 0x669, 0x66b, 0x64f, 0x655, 0x65b, 0x661, 0x663, 0x665, 0x667, 0x66a, 0x66c, 0x650, 0x656, 0x65c, 0x662, 0x664, 0x666, 0x668, 0 } },
	{ 0x1d, false, { 0x67a, 0x67c, 0x67e, 0x680, 0x682, 0x684, 0x686, 0x688, 0x67b, 0x67d, 0x67f, 0x681, 0x683, 0x685, 0x687, 0x689, 0 } },
	{ 0x1f, false, { 0x69e, 0x6a4, 0x6a6, 0x6a8, 0x6aa, 0x6f7, 0x69f, 0x6a5, 0x6a7, 0x6a9, 0x6ab, 0x6f8, 0 } },
	{ 0x20, false, { 0x6b2, 0x6b4, 0x6b6, 0x6b8, 0x6ba, 0x6f9, 0x6fb, 0x6b3, 0x6b5, 0x6b7, 0x6b9, 0x6bb, 0x6fa, 0x6fc, 0 } },
	{ 0x22, false, { 0x739, 0x73b, 0x73d, 0x73f, 0x741, 0x743, 0x745, 0x73a, 0x73c, 0x73e, 0x740, 0x742, 0x744, 0x746, 0 } },
	{ 0x23, false, { 0x783, 0x785, 0x787, 0x789, 0x78b, 0x6f9, 0x784, 0x786, 0x788, 0x78a, 0x78c, 0x6fa, 0 } },
	{ 0x24, false, { 0x797, 0x799, 0x79b, 0x79d, 0x79f, 0x7a1, 0x7a3, 0x7a5, 0x703, 0x70d, 0x711, 0x713, 0x715, 0x798, 0x79a, 0x79c, 0x79e, 0x7a0, 0x7a2, 0x7a4, 0x7a6, 0x704, 0x70e, 0x712, 0x714, 0x716, 0 } },
	{ 0x25, false, { 0x7c2, 0x7c4, 0x7c6, 0x7c3, 0x7c5, 0x7c7, 0 } },
	{ 0x26, false, { 0x85e, 0x860, 0x862, 0x864, 0x866, 0x868, 0x86a, 0x86c, 0x86e, 0x85f, 0x861, 0x863, 0x865, 0x867, 0x869, 0x86b, 0x86d, 0x86f, 0 } },
	{ 0x27, false, { 0x882, 0x884, 0x886, 0x888, 0x883, 0x885, 0x887, 0x889, 0 } },
	{ 0x28, false, { 0x89c, 0x89d, 0x89e, 0x89f, 0x8a0, 0x8a1, 0x8a2, 0x8a3, 0x8a4, 0x89a, 0x89b, 0 } },
	{ 0x29, false, { 0x89c, 0x89d, 0x89e, 0x89f, 0x8a0, 0x8a1, 0x8a2, 0x8a3, 0x8a4, 0x89a, 0x89b, 0 } },
	{ 0x2a, false, { 0x89c, 0x89d, 0x89e, 0x89f, 0x8a0, 0x8a1, 0x8a2, 0x8a3, 0x8a4, 0x89a, 0x89b, 0 } },
	{ 0x2b, false, { 0x8bb, 0x8bd, 0x8bf, 0x8c1, 0x8c3, 0x8c5, 0x8c7, 0x8c9, 0x8cb, 0x8b7, 0x8b9, 0x8bc, 0x8be, 0x8c0, 0x8c2, 0x8c4, 0x8c6, 0x8c8, 0x8ca, 0x8cc, 0x8b8, 0x8ba, 0 } },
	{ 0x2c, false, { 0x8d7, 0x8d9, 0x8db, 0x705, 0x6b4, 0x8d8, 0x8da, 0x8dc, 0x706, 0x6b5, 0 } },
	{ 0x2d, false, { 0x8e8, 0x8ea, 0x8ec, 0x8e9, 0x8eb, 0x8ed, 0 } },
	{ 0x2e, true, { 0x8fe, 0x900, 0x902, 0x8ff, 0x901, 0x903, 0 } },
	{ 0x2f, true, { 0x914, 0x916, 0x915, 0x917, 0 } },
	{ 0x31, false, { 0x6d4, 0x6d6, 0x6d8, 0x6da, 0x6d5, 0x6d7, 0x6d9, 0x6db, 0 } },
	{ 0x32, false, { 0x927, 0x929, 0x92b, 0x92d, 0x928, 0x92a, 0x92c, 0x92e, 0 } },
	{ 0x33, false, { 0x939, 0x93b, 0x93d, 0x93f, 0x941, 0x93a, 0x93c, 0x93e, 0x940, 0x942, 0 } },
	{ 0x34, true, { 0xa28, 0xa29, 0 } },
	{ 0x35, false, { 0x9f6, 0x9fa, 0x9fc, 0x9fe, 0xa00, 0xa02, 0xa04, 0x9f7, 0x9fb, 0x9fd, 0x9ff, 0xa01, 0xa03, 0xa05, 0 } },
	{ 0x36, false, { 0xa14, 0xa16, 0xa18, 0xa1a, 0xa1c, 0xa15, 0xa17, 0xa19, 0xa1b, 0xa1d, 0 } },
	{ 0x37, true, { 0x6e6, 0x6e8, 0x6e7, 0x6e9, 0 } },
	{ 0x39, false, { 0xa7b, 0xa7d, 0xa7f, 0xa81, 0xa83, 0xa85, 0xa7c, 0xa7e, 0xa80, 0xa82, 0xa84, 0xa86, 0 } },
	{ 0x3a, false, { 0xa45, 0xa47, 0xa4b, 0xa46, 0xa48, 0xa4c, 0 } },
	{ 0x3b, false, { 0xaa1, 0xaa3, 0xaa5, 0xaa7, 0xaa2, 0xaa4, 0xaa6, 0xaa8, 0 } },
	{ 0, false, { 0 } }
};

void Object18::handleMessage(Message &msg) {
	var(0x10).set(0);
	var(0x11).set(0);
	int32 room = var(1, kGameState).get();
	for (const RoomSounds *r = kRoomSounds; r->room; r++) {
		if (r->room != room)
			continue;
		if (msg.msg == kMsgLeave) {
			if (r->unloadEach) {
				for (const uint16 *s = r->sounds; *s; s++)
					cmd(kSelf, kCmdUnload, *s);
			} else {
				cmd(kSelf, kCmdUnload, 0);
			}
		} else if (msg.msg == kMsgEnter) {
			for (const uint16 *s = r->sounds; *s; s++)
				cmd(kSelf, kCmdLoad, *s);
		} else if (room == 0x26 && msg.msg == kMsgSaid) {
			if (msg.p2 == 0x850)
				send(kSelf, kMsgSay, 0x8670866, 0x12);
			else if (msg.p2 == 0x866)
				send(0x3d, 0x35);
		}
		break;
	}
	if (msg.handled == 0)
		SpeakerScript::handleMessage(msg);
}

/** Object 19 (vtable 0x4b4fc4, FUN_00406940), a character of room 0x18 [unverified]. */
class Object19 : public SpeakerScript {
public:
	Object19(GameObject *obj) : SpeakerScript(obj) {}
	void handleMessage(Message &msg) override;
};

void Object19::handleMessage(Message &msg) {
	if (var(1, kGameState).get() == 0x18) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgEnter:
			var(0x10).set(0x42);
			var(0x11).set(0x47);
			cmd(kSelf, kCmdLoad, 0x42);
			cmd(kSelf, kCmdSetPos, 0x42, 0x6b, 0x8e, 0x90);
			cmd(kSelf, kCmdLoad, 0x43);
			cmd(kSelf, kCmdSetPos, 0x43, 0x6b, 0x8e, 0x90);
			cmd(kSelf, kCmdLoad, 0x47);
			cmd(kSelf, kCmdSetPos, 0x47, 0x6b, 0x8e, 0x90);
			cmd(kSelf, kCmdLoad, 0x48);
			cmd(kSelf, kCmdSetPos, 0x48, 0x6b, 0x8e, 0x90);
			cmd(kSelf, kCmdPlay, 0x45, 0xffff, 0xc0003fff);
			cmd(kSelf, kCmdSetPos, 0x45, 0x6a, 0x8c, 0x90);
			if (var(0x99).get() == 2) {
				cmd(kSelf, kCmdSetCursor, 0x45, 0x80000003);
				cmd(kSelf, kCmdSetFlags, 0x45, 0x400);
			}
			cmd(kSelf, kCmdLoad, 0x46);
			cmd(kSelf, kCmdSetPos, 0x46, 0x6b, 0x8e, 0x90);
			cmd(kSelf, kCmdLoad, 0x44);
			cmd(kSelf, kCmdSetPos, 0x44, 0x67, 0x8e, 0x90);
			cmd(kSelf, kCmdLoad, 0x5d3);
			cmd(kSelf, kCmdLoad, 0x5db);
			cmd(kSelf, kCmdLoad, 0x5dd);
			cmd(kSelf, kCmdLoad, 0x5d4);
			cmd(kSelf, kCmdLoad, 0x5dc);
			cmd(kSelf, kCmdLoad, 0x5de);
			break;
		case kMsgSaid:
			if (msg.p2 == 0x5d9)
				send(kSelf, kMsgSay, 0x5dc05db);
			else if (msg.p2 == 0x5dd)
				send(kSelf, kMsgPlayAnim, 0x44, 0x430013, 0x80003fff);
			break;
		case 0x30:
			if (msg.p2 == 0x44) {
				send(10, 0x26, 0x48);
			} else if (msg.p2 == 0x46) {
				var(0x10).set(0x43);
				var(0x11).set(0x48);
				var(0x99).set(2);
				send(kSelf, kMsgSay, 0x5de05dd, 0xffff);
			} else if (msg.p2 == 0x4a) {
				cmd(kSelf, kCmdResume, 0x45);
			}
			break;
		case 0x3b:
			if (msg.p1 == 0x45)
				msg.result = 1;
			break;
		case 0x3c:
			if (msg.p1 == 0x45)
				send(kSpeech, kMsgSay, 0x5e005df);
			break;
		case kMsgClick:
			if (msg.p1 == 0x45 && var(0x99).get() != 2)
				send(16, kMsgPlayAnim, 0x4a, 0x490013, 0x80003fff);
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x45 && var(0x99).get() != 2) {
				var(0x99).set(1);
				cmd(kSelf, kCmdUnload, 0x45);
				send(kSelf, kMsgSay, 0x5d405d3, 0xf);
			}
			break;
		case kMsgDropTest:
		case kMsgDropTestAlt:
			if (var(kVarBusy, kGameState).get() == 0)
				msg.result = (uint32)msg.p1 >> 16 == 0x2e2 && msg.p2 == 0x42;
			break;
		case kMsgDrop:
			if ((uint32)msg.p1 >> 16 == 0x2e2 && msg.p2 == 0x42) {
				uint16 item = msg.p1;
				cmd(item, kCmdStop, var(0x30, item).get());
				setCursor(0);
				var(0x99).set(2);
				var(0x2d, item).set(0);
				var(0x31, item).set(1);
				cmd(kSelf, kCmdUnload, 0x42);
				send(kSelf, kMsgPlayAnim, 0x46, 0x430013, 0x80003fff);
			}
			break;
		default:
			break;
		}
	}
	if (msg.handled == 0)
		SpeakerScript::handleMessage(msg);
}

/** Object 20 (vtable 0x4b4fa4, FUN_00405510), a character of room 0x1f [unverified]. */
class Object20 : public SpeakerScript {
public:
	Object20(GameObject *obj) : SpeakerScript(obj) {}
	void handleMessage(Message &msg) override;
};

void Object20::handleMessage(Message &msg) {
	if (var(1, kGameState).get() == 0x1f) {
		switch (msg.msg) {
		case kMsgLeave:
			cmd(kSelf, kCmdUnload, 0);
			break;
		case kMsgEnter:
			var(0x10).set(0x17c);
			var(0x11).set(0x17f);
			cmd(kSelf, kCmdLoad, 0x17b, 0xffff);
			cmd(kSelf, kCmdSetPos, 0x17b, 0x16a, 0x6b, 0x90);
			cmd(kSelf, kCmdLoad, 0x17c);
			cmd(kSelf, kCmdSetPos, 0x17c, 0x183, 0x6b, 0x90);
			cmd(kSelf, kCmdLoad, 0x17e);
			cmd(kSelf, kCmdSetPos, 0x17e, 0xf8, 0x2b, 0x90);
			cmd(kSelf, kCmdLoad, 0x17d);
			cmd(kSelf, kCmdSetPos, 0x17d, 0x122, 0x55, 0x90);
			cmd(kSelf, kCmdLoad, 0x17f);
			cmd(kSelf, kCmdSetPos, 0x17f, 0x122, 0x5b, 0x90);
			cmd(kSelf, kCmdLoad, 0x696);
			cmd(kSelf, kCmdLoad, 0x697);
			break;
		case kMsgSaid:
			if (msg.p2 == 0x696) {
				send(kSelf, kMsgPlayAnim, 0x17d, 0x17c001f, 0x8000c001);
				cmd(15, kCmdUnload, 0x188);
				send(15, kMsgSay, 0x6990698, 0x14);
			} else if (msg.p2 == 0x698) {
				cmd(15, kCmdUnload, 0x182);
			}
			break;
		case 0x30:
			if (msg.p2 == 0x187) {
				var(0x11, 15).set(0x189);
				cmd(15, kCmdUnload, 0x182);
				cmd(15, kCmdUnload, 0x187);
				cmd(15, kCmdPlay, 0x188);
			}
			break;
		case kMsgAnimDone:
			if (msg.p1 == 0x17b) {
				cmd(kSelf, kCmdUnload, 0x17b);
				cmd(kSelf, kCmdPlay, 0x17e, 0xffff);
			} else if (msg.p1 == 0x17e) {
				cmd(kSelf, kCmdUnload, 0x17e);
				send(kSelf, kMsgSay, 0x6970696, 0xffff);
			}
			break;
		default:
			break;
		}
	}
	if (msg.handled == 0)
		SpeakerScript::handleMessage(msg);
}

/** Object 21 (vtable 0x4b4f84, FUN_0045da70), a character of rooms 0x38 and 0x3b [unverified]. */
class Object21 : public SpeakerScript {
public:
	Object21(GameObject *obj) : SpeakerScript(obj) {}
	void handleMessage(Message &msg) override;

private:
	void handleRoom38(Message &msg);
	void handleRoom3b(Message &msg);
	/** Object 16 comments the game of room 0x38 (var 0x19 of object 15: rounds played [unverified]). */
	void comment38(bool withDefault);
};

void Object21::handleMessage(Message &msg) {
	int32 room = var(1, kGameState).get();
	if (room == 0x38)
		handleRoom38(msg);
	else if (room == 0x3b)
		handleRoom3b(msg);
	if (msg.handled == 0)
		SpeakerScript::handleMessage(msg);
}

void Object21::comment38(bool withDefault) {
	if (withDefault) {
		if (var(0x19, 15).get() == 0)
			send(16, kMsgSay, 0xa5c0a5b, 0xf);
		else if (var(0x19, 15).get() == 1)
			send(16, kMsgSay, 0xa5a0a59, 0x15);
		else
			send(16, kMsgSay, 0xa580a57);
	} else {
		if (var(0x19, 15).get() < 1)
			send(16, kMsgSay, 0xa5c0a5b, 0xf);
		else if (var(0x19, 15).get() == 1)
			send(16, kMsgSay, 0xa5a0a59, 0x15);
	}
}

void Object21::handleRoom38(Message &msg) {
	switch (msg.msg) {
	case kMsgLeave:
		cmd(kSelf, kCmdUnload, 0);
		break;
	case kMsgEnter:
		var(0x10).set(0x566);
		var(0x11).set(0x56b);
		cmd(kSelf, kCmdPlay, 0x566);
		cmd(kSelf, kCmdSetPos, 0x566, 0x1b7, 0x7e, 0xa0);
		cmd(kSelf, kCmdLoad, 0x56b);
		cmd(kSelf, kCmdSetPos, 0x56b, 0x1b7, 0x7e, 0xa0);
		cmd(kSelf, kCmdLoad, 0x563);
		cmd(kSelf, kCmdSetPos, 0x563, 0x197, 0x26, 0xa0);
		cmd(kSelf, kCmdLoad, 0x564);
		cmd(kSelf, kCmdSetPos, 0x564, 0x199, 0x2d, 0xa0);
		cmd(kSelf, kCmdLoad, 0x565);
		cmd(kSelf, kCmdSetPos, 0x565, 0x17f, 0x42, 0xa0);
		cmd(kSelf, kCmdLoad, 0x568);
		cmd(kSelf, kCmdSetPos, 0x568, 0x173, 0x2c, 0xa0);
		cmd(kSelf, kCmdLoad, 0x569);
		cmd(kSelf, kCmdLoad, 0x56a);
		cmd(kSelf, kCmdLoad, 0xa4f);
		cmd(kSelf, kCmdLoad, 0xa5f);
		cmd(kSelf, kCmdLoad, 0xa61);
		cmd(kSelf, kCmdLoad, 0xa50);
		cmd(kSelf, kCmdLoad, 0xa60);
		cmd(kSelf, kCmdLoad, 0xa62);
		if (var(0x19, 15).get() == 0)
			var(0x19, 15).set(1);
		break;
	case kMsgSaid:
		if (msg.p2 == 0xa55) {
			var(0x1b).postIncrement();
			send(0x38, 0x50, var(0x1b).get());
			if (var(0x1b).get() == 5) {
				cmd(0x38, kCmdStop, 0x567);
				cmd(kSelf, kCmdPlay, 0x599);
				send(kSelf, kMsgSay, 0xa620a61, 0x15);
				cmd(kSelf, kCmdEnable, 0x568);
			} else {
				cmd(kSelf, kCmdStop, 0x568);
				cmd(kSelf, kCmdPlay, var(0x10).get());
				comment38(false);
			}
			var(0x9d, 0x38).set(0);
		} else if (msg.p2 == 0xa59) {
			send(kSelf, kMsgSay, 0xa600a5f);
		} else if (msg.p2 == 0xa61) {
			cmd(kSelf, kCmdStop, var(0x10).get());
			cmd(kSelf, kCmdEnable, 0x568, 1);
			cmd(kSelf, kCmdResume, 0x568);
		}
		break;
	case kMsgAnimDone: {
		// 0x563-0x565: the three cups [unverified]; var 0x9c of object 0x38 the right one.
		int32 cup = 0;
		switch (msg.p1) {
		case 0x563:
			cup = 1;
			break;
		case 0x564:
			cup = 2;
			break;
		case 0x565:
			cup = 3;
			break;
		case 0x568:
			if (var(0x1b).get() == 5)
				send(15, kMsgSay, 0xa640a63, 0x10);
			return;
		case 0x56e:
		case 0x56f:
		case 0x570:
			cmd(kSelf, kCmdPlay, 0x569);
			return;
		default:
			return;
		}
		if (var(0x9c, 0x38).get() != cup) {
			cmd(kSelf, kCmdPlay, 0x568, 0x15, 0x80000037);
			return;
		}
		cmd(kSelf, kCmdPlay, var(0x10).get());
		var(0x9d, 0x38).set(0);
		comment38(true);
		break;
	}
	default:
		break;
	}
}

void Object21::handleRoom3b(Message &msg) {
	switch (msg.msg) {
	case kMsgLeave:
		cmd(kSelf, kCmdUnload, 0);
		break;
	case kMsgEnter:
		var(0x10).set(0x599);
		var(0x11).set(0x59a);
		cmd(kSelf, kCmdLoad, 0x599);
		cmd(kSelf, kCmdSetPos, 0x599, 0x44, 0x87, 0xa0);
		cmd(kSelf, kCmdLoad, 0x59a);
		cmd(kSelf, kCmdSetPos, 0x59a, 0x2b, 0x87, 0xa0);
		cmd(kSelf, kCmdLoad, 0x59b);
		cmd(kSelf, kCmdSetPos, 0x59b, 0x2b, 5, 0xa0);
		cmd(kSelf, kCmdPlay, 0x59c, 0x15, 0xc000c001);
		cmd(kSelf, kCmdSetPos, 0x59c, 0x37, 0x87, 0xa0);
		cmd(kSelf, kCmdLoad, 0xa8b);
		cmd(kSelf, kCmdLoad, 0xa8d);
		cmd(kSelf, kCmdLoad, 0xa8c);
		cmd(kSelf, kCmdLoad, 0xa8e);
		cmd(kSelf, kCmdSetCursor, 0x59c, 0x80000006);
		break;
	case kMsgSaid:
		if (msg.p2 == 0xa8b) {
			send(kSelf, kMsgSay, 0xa8e0a8d, 0x15);
		} else if (msg.p2 == 0xa8d) {
			cmd(kSelf, kCmdStop, 0x599);
			cmd(kSelf, kCmdPlay, 0x59b);
		}
		break;
	case kMsgClick:
		if (msg.p1 == 0x59c && var(kVarDragMode, kGameState).get() == 0) {
			cmd(kSelf, kCmdSetFlags, 0x59c);
			cmd(kSelf, kCmdSetCursor, 0x59c, 0x80000003);
			cmd(kSelf, kCmdResume, 0x59c);
			var(0x1a).set(1);
			var(kVarDragMode, kGameState).set(1);
			setCursor(kCursorWait);
		}
		break;
	case kMsgAnimDone:
		if (msg.p1 == 0x59c) {
			send(kSelf, kMsgSay, 0xa8c0a8b, 0x15);
			cmd(15, kCmdStop, 0x58b);
			cmd(16, kCmdStop, 0x593);
			cmd(15, kCmdPlay, 0x590, 0, 0x8000c001);
		}
		break;
	default:
		break;
	}
}

/**
 * Object 60 (vtable 0x4b4aa4, FUN_0040cf70): the combination lock of room
 * 0x17 [unverified]. Six wheels with 26 positions; clicking hotspot 0x112
 * (p3 = the area: 1-6 down, 7-12 up) turns a wheel. The animations of the
 * wheels turning down/up are in the maps 0x76/0x77, the positions in 0x78.
 */
class Object60 : public BatchBScript {
public:
	Object60(GameObject *obj) : BatchBScript(obj) {}
	void handleMessage(Message &msg) override;

private:
	enum {
		kMsgShow = 0x44,
		kMsgHide = 0x45,
		kMsgTurn = 0x46 ///< p1 = wheel, p2 = 1 down / 0 up, p3 = the new position
	};

	static bool isPart(int32 id) {
		return id == 0x113 || id == 0x112 || (id >= 0xb43 && id <= 0xb4e);
	}
	VarRef wheel(uint16 map, int32 i) { return var(map).at(i); }
};

void Object60::handleMessage(Message &msg) {
	switch (msg.msg) {
	case kMsgLeave:
		if (msg.p1 == 0x17) {
			cmd(kSelf, kCmdUnload, 0x23e);
			cmd(kSelf, kCmdUnload, 0x113);
			cmd(kSelf, kCmdUnload, 0x112);
			for (int i = 1; i <= 6; i++) {
				cmd(kSelf, kCmdUnload, wheel(0x76, i).get());
				cmd(kSelf, kCmdUnload, wheel(0x77, i).get());
			}
		}
		break;
	case kMsgEnter:
		if (msg.p1 == 0x17) {
			cmd(kSelf, kCmdLoad, 0x23e);
			cmd(kSelf, kCmdLoad, 0x113);
			cmd(kSelf, kCmdLoad, 0x112);
			for (int i = 1; i <= 6; i++) {
				cmd(kSelf, kCmdLoad, wheel(0x76, i).get());
				cmd(kSelf, kCmdLoad, wheel(0x77, i).get());
			}
		}
		break;
	case 0xd: {
		static const uint16 kDown[6] = { 0xb44, 0xb46, 0xb48, 0xb4a, 0xb4c, 0xb4e };
		for (int i = 1; i <= 6; i++) {
			wheel(0x76, i).set(kDown[i - 1]);
			wheel(0x77, i).set(kDown[i - 1] - 1);
		}
		for (int i = 1; i <= 6; i++) {
			if (wheel(0x78, i).get() == 0)
				wheel(0x78, i).set(1);
		}
		break;
	}
	case 0x3b:
		msg.result = isPart(msg.p1);
		break;
	case 0x3c:
		if (isPart(msg.p1)) {
			int32 said = var(0x44).get();
			if (said == 0)
				send(kSpeech, kMsgSay, 0x5c905c8);
			else if (said == 1)
				send(kSpeech, kMsgSay, 0x5cb05ca);
		}
		break;
	case kMsgShow: {
		cmd(kSelf, kCmdPlay, 0x23e);
		cmd(kSelf, kCmdSetZ, 0x23e, 0x2710);
		cmd(kSelf, kCmdPlay, 0x113);
		cmd(kSelf, kCmdSetZ, 0x113, 0x2724);
		cmd(kSelf, kCmdPlay, 0x112);
		cmd(kSelf, kCmdSetZ, 0x112, 0x272e);
		for (int i = 1; i <= 6; i++) {
			cmd(kSelf, kCmdPlay, wheel(0x76, i).get(), 0x3c);
			cmd(kSelf, kCmdPause, wheel(0x76, i).get());
			cmd(kSelf, kCmdEnable, wheel(0x76, i).get(), 1);
			cmd(kSelf, kCmdPlay, wheel(0x77, i).get(), 0x3c);
			cmd(kSelf, kCmdPause, wheel(0x77, i).get());
			cmd(kSelf, kCmdEnable, wheel(0x77, i).get(), 0);
		}
		static const int16 kX[6] = { 0x5b, 0xa7, 0xf6, 0x144, 0x192, 0x1e2 };
		for (int i = 0; i < 6; i++) {
			cmd(kSelf, kCmdSetPos, 0xb44 + 2 * i, kX[i], 0xbc, 0x2729);
			cmd(kSelf, kCmdSetPos, 0xb43 + 2 * i, kX[i], 0xbc, 0x2729);
		}
		for (int i = 1; i <= 6; i++) {
			if (wheel(0x78, i).get() != 1) {
				int32 frame = wheel(0x78, i).get() * 3 - 3;
				cmd(kSelf, kCmdGotoFrame, wheel(0x76, i).get(), frame);
			}
		}
		send(16, kMsgSay, 0x5c105c0);
		break;
	}
	case kMsgHide:
		cmd(kSelf, kCmdStop, 0x23e);
		cmd(kSelf, kCmdStop, 0x113);
		cmd(kSelf, kCmdStop, 0x112);
		for (int i = 1; i <= 6; i++) {
			cmd(kSelf, kCmdStop, wheel(0x76, i).get());
			cmd(kSelf, kCmdStop, wheel(0x77, i).get());
		}
		break;
	case kMsgTurn: {
		// Play the wheel's animation from the old to the new position.
		uint16 i = msg.p1;
		uint16 map;
		int32 from, to;
		if (msg.p2 == 0) {
			from = wheel(0x78, i).get() * 3 - 3;
			to = msg.p3 * 3 - 3;
			cmd(kSelf, kCmdEnable, wheel(0x77, i).get(), 0);
			cmd(kSelf, kCmdEnable, wheel(0x76, i).get(), 1);
			map = 0x76;
		} else {
			from = (0x1b - wheel(0x78, i).get()) * 3;
			to = (0x1b - msg.p3) * 3;
			cmd(kSelf, kCmdEnable, wheel(0x76, i).get(), 0);
			cmd(kSelf, kCmdEnable, wheel(0x77, i).get(), 1);
			map = 0x77;
		}
		uint32 range = ((uint32)((to | 0x8000) & 0xffff) << 16) | (from & 0xffff);
		cmd(kSelf, kCmdResume, wheel(map, i).get(), range);
		wheel(0x78, i).set(msg.p3);
		break;
	}
	case kMsgClick:
		if (msg.p1 == 0x112) {
			if (msg.p3 > 6) {
				int32 pos = wheel(0x78, (uint16)(msg.p3 - 6)).get() + 1;
				if (pos > 0x1a)
					pos = 1;
				send(kSelf, kMsgTurn, msg.p3 - 6, 0, pos);
			} else {
				int32 pos = wheel(0x78, (uint16)msg.p3).get() - 1;
				if (pos < 1)
					pos = 0x1a;
				send(kSelf, kMsgTurn, msg.p3, 1, pos);
			}
		} else if (msg.p1 == 0x23e) {
			send(kSelf, kMsgHide);
		}
		break;
	case kMsgAnimDone: {
		static const int32 kCode[6] = { 0xd, 5, 0x12, 0xc, 0x19, 0xe };
		int i = 0;
		while (i < 6 && wheel(0x78, i + 1).get() == kCode[i])
			i++;
		if (i == 6) {
			send(kSelf, kMsgHide);
			send(0x17, kMsgHide);
		}
		cmd(kSelf, kCmdGetFrame, msg.p1);
		break;
	}
	default:
		break;
	}
	if (msg.handled == 0)
		ObjectScript::handle(msg);
}

/**
 * Object 61 (vtable 0x4b4a84, FUN_0041c2d0): the two-digit code panel of
 * room 0x26 [unverified]. The buttons 0xb14-0xb17 choose the first
 * value (var 0x5f), 0xb18-0xb1b the second (var 0x60); the text sprites
 * 0xb1c/0xb1d show them. The answer is 0x859 / 0x841.
 */
class Object61 : public BatchBScript {
public:
	Object61(GameObject *obj) : BatchBScript(obj) {}
	void handleMessage(Message &msg) override;

private:
	enum {
		kAddrEmptyString = 0x4caa4c ///< an empty string of the original's data
	};
	void sayValue(int32 value);
	void enableAll(int32 enable);
	static bool isPart(int32 id) {
		return id == 0x394 || id == 0x3a3 || id == 0xfe73 || (id >= 0xb14 && id <= 0xb1b);
	}
};

void Object61::sayValue(int32 value) {
	switch (value) {
	case 0x841:
	case 0x843:
	case 0x845:
	case 0x855:
	case 0x857:
	case 0x859:
	case 0x85b:
	case 0x85d:
		send(16, kMsgSay, (value << 16) | (value - 1), 0x3d);
		break;
	default:
		break;
	}
}

void Object61::enableAll(int32 enable) {
	cmd(kSelf, kCmdEnable, 0x394, enable);
	cmd(kSelf, kCmdEnable, 0x3a3, enable);
	cmd(kSelf, kCmdEnable, 0xfe73, enable);
	cmd(kSelf, kCmdEnable, 0xb1c, enable);
	cmd(kSelf, kCmdEnable, 0xb1d, enable);
}

void Object61::handleMessage(Message &msg) {
	if (var(1, kGameState).get() != 0x26) {
		if (msg.handled == 0)
			ObjectScript::handle(msg);
		return;
	}
	switch (msg.msg) {
	case kMsgLeave:
		cmd(kSelf, kCmdUnload, 0);
		break;
	case kMsgEnter:
		cmd(kSelf, kCmdLoad, 0x394);
		cmd(kSelf, kCmdLoad, 0x3a3);
		cmd(kSelf, kCmdLoad, 0x23e);
		break;
	case kMsgSaid:
		switch (msg.p2) {
		case 0x840:
		case 0x842:
		case 0x844:
		case 0x85c:
			if (var(0x5f).get() == 0x859 && var(0x60).get() == 0x841) {
				send(kSelf, 0x36);
				send(0x26, 0x36);
			} else {
				enableAll(1);
			}
			break;
		case 0x854:
		case 0x856:
		case 0x858:
		case 0x85a: {
			int32 second = var(0x60).get();
			if (second == 0)
				enableAll(1);
			else if (second == 0x841 || second == 0x843 || second == 0x845 || second == 0x85d)
				sayValue(second);
			break;
		}
		case 0x868:
			send(kSpeech, kMsgSay, 0x86b086a);
			break;
		case 0x86c:
			send(kSelf, 0x37);
			break;
		default:
			break;
		}
		break;
	case 0x35: {
		cmd(kSelf, kCmdPlay, 0x394);
		cmd(kSelf, kCmdSetPos, 0x394, 0xb5, 0x16, 0x2711);
		cmd(kSelf, kCmdPlay, 0x3a3);
		cmd(kSelf, kCmdSetPos, 0x3a3, 0x129, 0x152, 0x2712);
		cmd(kSelf, kCmdPlay, 0x23e);
		cmd(kSelf, kCmdSetZ, 0x23e, 0x2710);
		cmd(kSelf, kCmdPlay, 0xfe73);
		cmd(kSelf, kCmdSetView, 0xfe73, 0xe1, 0x18);
		static const int16 kY[8] = { 0x34, 0x4d, 0x66, 0x7f, 0xac, 0xc5, 0xde, 0xf7 };
		static const int16 kValue[8] = { 0x855, 0x857, 0x859, 0x85b, 0x85d, 0x841, 0x843, 0x845 };
		for (int i = 0; i < 8; i++) {
			cmd(kSelf, kCmdSetPos, 0xb14 + i, 0xd3, kY[i], 0x271a);
			cmd(kSelf, 0xe, 0xb14 + i, 0x10, kValue[i]);
		}
		var(0x5f).set(0);
		var(0x60).set(0);
		cmd(kSelf, kCmdPlay, 0xb1c);
		cmd(kSelf, kCmdSetPos, 0xb1c, 0xd3, 0x11f, 0x271a);
		cmd(kSelf, kCmdPlay, 0xb1d);
		cmd(kSelf, kCmdSetPos, 0xb1d, 0xd3, 0x133, 0x271a);
		// The original passes the address of a string of its data.
		cmdString(kSelf, kCmdSetText, 0xb1c, "");
		cmdString(kSelf, kCmdSetText, 0xb1d, "");
		enableAll(1);
		send(kSpeech, kMsgSay, 0x8690868, 0x3d);
		break;
	}
	case 0x36:
		cmd(kSelf, kCmdStop, 0);
		break;
	case 0x37: {
		int32 value = var(0x5f).get();
		if (value == 0) {
			value = var(0x60).get();
			if (value == 0)
				break;
		}
		enableAll(0);
		sayValue(value);
		break;
	}
	case 0x3b:
		msg.result = isPart(msg.p1);
		break;
	case 0x3c:
		if (isPart(msg.p1))
			send(kSpeech, kMsgSay, 0x86b086a);
		break;
	case kMsgClick:
		switch (msg.p1) {
		case 0xb14:
			var(0x5f).set(0x855);
			break;
		case 0xb15:
			var(0x5f).set(0x857);
			break;
		case 0xb16:
			var(0x5f).set(0x859);
			break;
		case 0xb17:
			var(0x5f).set(0x85b);
			break;
		case 0xb18:
			var(0x60).set(0x85d);
			break;
		case 0xb19:
			var(0x60).set(0x841);
			break;
		case 0xb1a:
			var(0x60).set(0x843);
			break;
		case 0xb1b:
			var(0x60).set(0x845);
			break;
		case 0x3a3:
			// The door: refused while something is chosen or busy (no base handler then).
			if (var(0x5f).get() != 0 || var(0x60).get() != 0 || var(kVarBusy, kGameState).get() != 0) {
				send(kSpeech, kMsgSay, 0x86d086c, 0x3d);
				return;
			}
			break;
		default:
			break;
		}
		cmd(kSelf, 0xe, 0xb1c, 0x10, var(0x5f).get());
		cmd(kSelf, 0xe, 0xb1d, 0x10, var(0x60).get());
		break;
	default:
		break;
	}
	if (msg.handled == 0)
		ObjectScript::handle(msg);
}

IVANOFF_SCRIPT(Object15) { return new Object15(obj); }
IVANOFF_SCRIPT(Object16) { return new Object16(obj); }
IVANOFF_SCRIPT(Object18) { return new Object18(obj); }
IVANOFF_SCRIPT(Object19) { return new Object19(obj); }
IVANOFF_SCRIPT(Object20) { return new Object20(obj); }
IVANOFF_SCRIPT(Object21) { return new Object21(obj); }
IVANOFF_SCRIPT(Object60) { return new Object60(obj); }
IVANOFF_SCRIPT(Object61) { return new Object61(obj); }

} // End of namespace Ivanoff
