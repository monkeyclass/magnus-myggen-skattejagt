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

// Debugging aids for checking the port against the original: scripted input
// (debug_events), random input (fuzz) and stopping on faults. Only active
// with the matching configuration keys.

// For debugFault().
#define FORBIDDEN_SYMBOL_EXCEPTION_stdout
#define FORBIDDEN_SYMBOL_EXCEPTION_stderr
#define FORBIDDEN_SYMBOL_EXCEPTION_exit

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/events.h"
#include "common/file.h"
#include "common/system.h"
#include "common/tokenizer.h"
#include "image/png.h"

#include "ivanoff/ivanoff.h"
#include "ivanoff/files.h"
#include "ivanoff/sprite.h"
#include "ivanoff/world.h"

namespace Ivanoff {

void IvanoffEngine::debugFault(const char *what) {
	if (!ConfMan.hasKey("debug_events"))
		return;
	debug("FAULT: %s", what);
	fflush(stdout);
	fflush(stderr);
	exit(0);
}

// Debugging aid: "fuzz_dict=<file>" lists, per class of object,
// "<vtable> <messages> <constants>" (comma separated hex). The "fuzz"
// debug event then sends random messages built from them to the objects,
// so that runs with and without the native logic can be compared.
void IvanoffEngine::loadFuzzDictionary() {
	_fuzz.clear();
	Common::File f;
	if (!ConfMan.hasKey("fuzz_dict") || !f.open(Common::FSNode(Common::Path(ConfMan.get("fuzz_dict"))))) {
		warning("fuzz: no fuzz_dict");
		return;
	}
	Common::HashMap<uint32, Common::Array<uint16> > byVtable;
	const Common::Array<GameObject *> &objs = _world->getObjects();
	for (uint i = 0; i < objs.size(); i++) {
		if (objs[i]->_script)
			byVtable[objs[i]->_class].push_back(objs[i]->getId());
	}
	while (!f.eos()) {
		Common::String line = f.readLine();
		Common::StringTokenizer tok(line, " ");
		uint32 vt = strtoul(tok.nextToken().c_str(), nullptr, 16);
		Common::StringTokenizer msgs(tok.nextToken(), ",");
		Common::StringTokenizer params(tok.nextToken(), ",");
		Common::StringTokenizer vars(tok.nextToken(), ",");
		FuzzEntry e;
		while (!vars.empty()) {
			Common::String v = vars.nextToken();
			uint id = 0, obj = 0;
			if (sscanf(v.c_str(), "%x:%x", &id, &obj) >= 1)
				e.vars.push_back((obj << 16) | id);
		}
		while (!msgs.empty())
			e.msgs.push_back(strtol(msgs.nextToken().c_str(), nullptr, 16));
		while (!params.empty())
			e.params.push_back(strtol(params.nextToken().c_str(), nullptr, 16));
		if (!byVtable.contains(vt) || e.msgs.empty())
			continue;
		// "fuzz_vtables": only these classes.
		if (ConfMan.hasKey("fuzz_vtables") && !ConfMan.get("fuzz_vtables").contains(Common::String::format("%x", vt)))
			continue;
		for (uint i = 0; i < byVtable[vt].size(); i++) {
			e.object = byVtable[vt][i];
			_fuzz.push_back(e);
		}
	}
	debug(1, "fuzz: %d objects", _fuzz.size());
}

void IvanoffEngine::runFuzz() {
	if (_fuzz.empty())
		return;
	const FuzzEntry &e = _fuzz[getRandomNumber(_fuzz.size() - 1)];
	if (!e.vars.empty() && ConfMan.hasKey("fuzz_vars") && ConfMan.getBool("fuzz_vars") && getRandomNumber(3) == 0) {
		// Set a variable, to reach the branches that depend on the game state.
		uint32 v = e.vars[getRandomNumber(e.vars.size() - 1)];
		uint16 obj = v >> 16 ? v >> 16 : e.object;
		int32 value = getRandomNumber(2);
		if (getRandomNumber(1) && !e.params.empty())
			value = e.params[getRandomNumber(e.params.size() - 1)];
		GameObject *o = _world->get(obj);
		if (!o)
			return;
		debug(1, "fuzz: var %d.%d = %d", obj, v & 0xffff, value);
		o->var(v & 0xffff)->set(value);
		return;
	}
	int32 p[3];
	for (int i = 0; i < 3; i++) {
		uint r = getRandomNumber(9);
		int32 c = e.params.empty() ? 0 : e.params[getRandomNumber(e.params.size() - 1)];
		int32 c2 = e.params.empty() ? 0 : e.params[getRandomNumber(e.params.size() - 1)];
		if (i > 0 && r < 4)
			p[i] = 0;
		else if (r < 6)
			p[i] = c;
		else if (r < 8)
			p[i] = (c << 16) | (c2 & 0xffff);
		else
			p[i] = getRandomNumber(3);
	}
	Message m(e.object, 0, e.msgs[getRandomNumber(e.msgs.size() - 1)], p[0], p[1], p[2]);
	debug(1, "fuzz: msg %d -> %d (%d, %d, %d)", m.msg, m.target, m.p1, m.p2, m.p3);
	_world->deliver(nullptr, m);
}

// Debugging aid: "debug_events=ms:click:x,y ms:key:c ..." replays input at
// the given times (milliseconds since start).
void IvanoffEngine::runDebugEvents(uint32 now) {
	if (!ConfMan.hasKey("debug_events"))
		return;
	if (_crawlLeft > 0 && now >= _crawlNext) {
		// Click the next clickable thing of the room (not the tool bar, objects 8-13).
		Common::Array<Sprite *> list;
		collectSprites(list);
		Common::Array<Common::Point> targets;
		for (uint i = 0; i < list.size(); i++) {
			Sprite *sp = list[i];
			uint16 owner = sp->getOwner()->getId();
			if (!(sp->getFlags() & kFlagClick) || (owner >= 8 && owner <= 13))
				continue;
			if (Hotspot *h = dynamic_cast<Hotspot *>(sp)) {
				Common::Array<Common::Point> pts = h->regionPoints();
				for (uint j = 0; j < pts.size(); j++)
					targets.push_back(pts[j]);
			} else {
				Common::Rect r = sp->getBounds();
				if (!r.isEmpty())
					targets.push_back(Common::Point((r.left + r.right) / 2, (r.top + r.bottom) / 2));
			}
		}
		if (targets.empty()) {
			debug(1, "Crawl: nothing to click");
		} else {
			// Random targets, so that runs wander through different rooms.
			Common::Point p = targets[_randomSource.getRandomNumber(targets.size() - 1)];
			debug(1, "Crawl: click %d,%d (%d targets)", p.x, p.y, targets.size());
			onMouseMove(p.x, p.y);
			onButton(p.x, p.y, true, false);
			onButton(p.x, p.y, false, false);
		}
		_crawlLeft--;
		_crawlNext = now + _crawlInterval;
	}
	if (_fuzzLeft > 0 && now >= _fuzzNext) {
		runFuzz();
		_fuzzLeft--;
		_fuzzNext = now + _fuzzInterval;
	}
	if (_debugStart == 0)
		_debugStart = now;
	Common::String all = ConfMan.get("debug_events");
	Common::StringTokenizer tok(all, " ");
	int index = 0;
	while (!tok.empty()) {
		Common::String ev = tok.nextToken();
		if (index++ < _debugEventIndex)
			continue;
		uint ms = 0, x = 0, y = 0;
		char kind[16] = { 0 };
		if (sscanf(ev.c_str(), "%u:%15[a-z]:%u,%u", &ms, kind, &x, &y) < 2)
			continue;
		if (now - _debugStart < ms)
			return;
		_debugEventIndex++;
		debug(1, "Debug event %s", ev.c_str());
		runDebugEvent(kind, x, y, now);
	}
}

void IvanoffEngine::runDebugEvent(const char *kind, uint x, uint y, uint32 now) {
	if (!strcmp(kind, "click")) {
		_system->warpMouse(x, y);
		onMouseMove(x, y);
		onButton(x, y, true, false);
		onButton(x, y, false, false);
	} else if (!strcmp(kind, "move")) {
		_system->warpMouse(x, y);
		onMouseMove(x, y);
	} else if (!strcmp(kind, "crawl")) {
		_crawlLeft = x;
		_crawlInterval = y ? y : 2500;
		_crawlNext = now;
	} else if (!strcmp(kind, "fuzz")) {
		loadFuzzDictionary();
		_fuzzLeft = x;
		_fuzzInterval = y ? y : 500;
		_fuzzNext = now;
	} else if (!strcmp(kind, "room")) {
		Message m(kGameState, 0, kMsgGotoRoom, x);
		_world->deliver(nullptr, m);
	} else if (!strcmp(kind, "dump")) {
		Common::Array<Sprite *> list;
		collectSprites(list);
		for (uint i = 0; i < list.size(); i++) {
			Sprite *sp = list[i];
			Common::Rect r = sp->getBounds();
			debug("visible %d.%d type %04x z %d flags %x box %d,%d-%d,%d", sp->getOwner()->getId(), sp->getId(),
			      sp->getType(), sp->getZ(), sp->getFlags(), r.left, r.top, r.right, r.bottom);
		}
	} else if (!strcmp(kind, "esc")) {
		onKey(Common::KEYCODE_ESCAPE, 27, true);
		onKey(Common::KEYCODE_ESCAPE, 27, false);
	} else if (!strcmp(kind, "save")) {
		// Saves to SavedGame<x>.m2s with version y.
		_files->saveWorld(Common::String::format("C:\\MM2\\appdata\\SavedGame%u.m2s", x), y);
	} else if (!strcmp(kind, "vk")) {
		// A key by its Windows virtual key code (x), down and up.
		Message d(kSelf, 0, kMsgKeyDown, x, 0);
		_world->deliverFiltered(d);
		Message u(kSelf, 0, kMsgKeyUp, x, 0);
		_world->deliverFiltered(u);
	} else if (!strcmp(kind, "gsave")) {
		// ScummVM's save to slot x.
		debug(1, "gsave %d: %s", x, canSaveGameStateCurrently() ? saveGameState(x, "test").getDesc().c_str() : "not now");
	} else if (!strcmp(kind, "gload")) {
		debug(1, "gload %d: %s", x, canLoadGameStateCurrently() ? loadGameState(x).getDesc().c_str() : "not now");
	} else if (!strcmp(kind, "pclick") || !strcmp(kind, "prclick") || !strcmp(kind, "pmove")) {
		// Through ScummVM's event queue, as a real mouse.
		bool right = kind[1] == 'r';
		Common::Event e;
		e.mouse = Common::Point(x, y);
		e.type = Common::EVENT_MOUSEMOVE;
		_system->getEventManager()->pushEvent(e);
		if (strcmp(kind, "pmove")) {
			e.type = right ? Common::EVENT_RBUTTONDOWN : Common::EVENT_LBUTTONDOWN;
			_system->getEventManager()->pushEvent(e);
			e.type = right ? Common::EVENT_RBUTTONUP : Common::EVENT_LBUTTONUP;
			_system->getEventManager()->pushEvent(e);
		}
	} else if (!strcmp(kind, "animdump")) {
		// The frames of child y of object x, next to the control file.
		GameObject *o = _world->get(x);
		Anim *a = o ? dynamic_cast<Anim *>(o->getChild(y)) : nullptr;
		if (a)
			a->dumpFrames(Common::Path(ConfMan.get("debug_control")).getParent().join(Common::String::format("anim%u_%u_", x, y)).toString());
	} else if (!strcmp(kind, "rclick")) {
		_system->warpMouse(x, y);
		onMouseMove(x, y);
		onButton(x, y, true, true);
		onButton(x, y, false, true);
	} else if (!strcmp(kind, "shot")) {
		// Writes a screenshot shot<x>.png next to the control file.
		Common::Path path = Common::Path(ConfMan.get("debug_control")).getParent().join(Common::String::format("shot%u.png", x));
		Common::DumpFile f;
		if (f.open(path))
			Image::writePNG(f, _screen, _palette);
	} else if (!strcmp(kind, "quit")) {
		quitGame();
	}
}

void IvanoffEngine::pollDebugControl(uint32 now) {
	// "debug_control=<file>": runs the lines added to the file while the
	// game runs (kind:x,y as in debug_events, without the time), so that a
	// tester can play by script and screenshots.
	if (now - _controlPolled < 100)
		return;
	_controlPolled = now;
	Common::File f;
	if (!f.open(Common::FSNode(Common::Path(ConfMan.get("debug_control")))))
		return;
	uint line = 0;
	while (!f.eos() && !f.err()) {
		Common::String l = f.readLine();
		if (l.empty())
			continue;
		if (line++ < _controlLines)
			continue;
		_controlLines++;
		uint x = 0, y = 0;
		char kind[16] = { 0 };
		if (sscanf(l.c_str(), "%15[a-z]:%u,%u", kind, &x, &y) < 1)
			continue;
		debug(1, "Control %s at %u", l.c_str(), now);
		runDebugEvent(kind, x, y, now);
	}
}

} // End of namespace Ivanoff
