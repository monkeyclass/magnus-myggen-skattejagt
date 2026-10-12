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

// For debugFault(), a debugging aid.
#define FORBIDDEN_SYMBOL_EXCEPTION_stdout
#define FORBIDDEN_SYMBOL_EXCEPTION_stderr
#define FORBIDDEN_SYMBOL_EXCEPTION_exit

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/events.h"
#include "common/tokenizer.h"
#include "common/file.h"
#include "common/formats/winexe.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/cursor.h"
#include "graphics/cursorman.h"
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/fonts/ttf.h"
#include "graphics/paletteman.h"
#include "graphics/wincursor.h"
#include "image/png.h"

#include "ivanoff/ivanoff.h"
#include "ivanoff/machine.h"
#include "ivanoff/sprite.h"
#include "ivanoff/world.h"

namespace Ivanoff {

IvanoffEngine *g_engine;

/** The Windows static colours, entries 0-9 and 246-255 of every palette. */
static const byte kStaticColors[20][3] = {
	{ 0, 0, 0 }, { 128, 0, 0 }, { 0, 128, 0 }, { 128, 128, 0 }, { 0, 0, 128 },
	{ 128, 0, 128 }, { 0, 128, 128 }, { 192, 192, 192 }, { 192, 220, 192 }, { 166, 202, 240 },
	{ 255, 251, 240 }, { 160, 160, 164 }, { 128, 128, 128 }, { 255, 0, 0 }, { 0, 255, 0 },
	{ 255, 255, 0 }, { 0, 0, 255 }, { 255, 0, 255 }, { 0, 255, 255 }, { 255, 255, 255 }
};

/** Cursor ids and the cursor groups in mm2run.exe (FUN_0047dd30). */
static const struct {
	uint32 id;
	uint16 resource;
} kCursorResources[] = {
	{ 0x80000004, 101 }, { 0x80000005, 102 }, { 0x80000006, 106 }, { 0x80000007, 108 },
	{ 0x80000008, 107 }, { 0x80000009, 103 }, { 0x8000000a, 104 }, { 0x8000000b, 111 }
};

IvanoffEngine::IvanoffEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Ivanoff") {
	g_engine = this;
	memset(_palette, 0, sizeof(_palette));
	// The globals of mm2run.exe that start with a value other than 0.
	_globals[0x4c6940] = 0x7fff; // the sound volume
	_globals[0x4c6970] = 22050;  // FUN_00477190's parameters [unverified: sound format]
	_globals[0x4c6974] = 2;
	_globals[0x4c6978] = 125;
}

IvanoffEngine::~IvanoffEngine() {
	delete _world;
	delete _machine;
	for (uint i = 0; i < _cursorGroups.size(); i++)
		delete _cursorGroups[i];
	delete _defaultCursor;
	delete _busyCursor;
	for (Common::HashMap<Common::String, Graphics::Font *>::iterator it = _fonts.begin(); it != _fonts.end(); ++it)
		delete it->_value;
	_screen.free();
}

Common::String IvanoffEngine::getGameId() const {
	return _gameDescription->gameId;
}

Common::String IvanoffEngine::getLanguageDir() const {
	switch (getLanguage()) {
	case Common::NB_NOR:
		return "N";
	case Common::SV_SWE:
		return "S";
	case Common::FI_FIN:
		return "SF";
	default:
		return "DK";
	}
}

Common::Error IvanoffEngine::run() {
	initGraphics(640, 480);
	_screen.create(640, 480, Graphics::PixelFormat::createFormatCLUT8());
	initPalette();

	if (!_resources.open(getLanguageDir()))
		return Common::Error(Common::kNoGameDataFoundError, "mm2.idx/mm2.dat or the language pack");

	loadCursors("mm2run.exe");

	_world = new World(this);
	_machine = new Machine();
	if (!_machine->load("mm2run.exe"))
		return Common::Error(Common::kNoGameDataFoundError, "mm2run.exe");
	_machine->runInitializers();
	_machine->createObjects();
	_machine->attachScripts();

	// The application object starts the game by sending message 1 to
	// object 17 (FUN_0047b5d0: start object 0x11).
	Message init(kGameState, 0, kMsgInit);
	_world->deliver(nullptr, init);

	CursorMan.showMouse(true);
	applyCursor(0x80000003);

	if (ConfMan.hasKey("virtual_clock"))
		_virtualStep = MAX(ConfMan.getInt("virtual_clock"), 0);
	if (ConfMan.hasKey("coverage")) {
		_machine->cpu()._coverageBase = 0x401000;
		_machine->cpu()._coverage.resize(0x75000);
	}

	while (!shouldQuit()) {
		Common::Event e;
		while (_eventMan->pollEvent(e)) {
			switch (e.type) {
			case Common::EVENT_MOUSEMOVE:
				onMouseMove(e.mouse.x, e.mouse.y);
				break;
			case Common::EVENT_LBUTTONDOWN:
				onButton(e.mouse.x, e.mouse.y, true, false);
				break;
			case Common::EVENT_LBUTTONUP:
				onButton(e.mouse.x, e.mouse.y, false, false);
				break;
			case Common::EVENT_RBUTTONDOWN:
				onButton(e.mouse.x, e.mouse.y, true, true);
				break;
			case Common::EVENT_KEYDOWN:
				onKey(e.kbd.keycode, e.kbd.ascii, true);
				break;
			case Common::EVENT_KEYUP:
				onKey(e.kbd.keycode, e.kbd.ascii, false);
				break;
			default:
				break;
			}
		}

		if (_virtualStep)
			_virtualTime += _virtualStep;
		uint32 now = getMillis();
		if (now - _statsTime >= 5000) {
			uint64 n = _machine->cpu()._instructions;
			debug(1, "Interpreter: %u instructions/s", (uint)((n - _statsInstructions) * 1000 / MAX<uint32>(now - _statsTime, 1)));
			_statsInstructions = n;
			_statsTime = now;
		}
		runDebugEvents(now);
		_world->update();
		updateChildren(now);
		updateCursor();
		render();
		_system->delayMillis(_virtualStep ? 0 : 10);
	}

	writeCoverage();
	return Common::kNoError;
}

void IvanoffEngine::debugFault(const char *what) {
	if (!ConfMan.hasKey("debug_events"))
		return;
	debug("FAULT: %s", what);
	writeCoverage();
	fflush(stdout);
	fflush(stderr);
	exit(0);
}

// Debugging aid: "coverage=<file>" writes the addresses of the executed
// instructions of the game code, one per line.
void IvanoffEngine::writeCoverage() {
	Emu::CPU &cpu = _machine->cpu();
	if (!ConfMan.hasKey("coverage") || cpu._coverage.empty())
		return;
	Common::DumpFile f;
	if (!f.open(Common::Path(ConfMan.get("coverage"))))
		return;
	for (uint i = 0; i < cpu._coverage.size(); i++)
		if (cpu._coverage[i])
			f.writeString(Common::String::format("%x\n", cpu._coverageBase + i));
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
		EmuObject *o = (EmuObject *)objs[i];
		if (!o->_native)
			byVtable[o->getVtable()].push_back(o->getId());
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
		Emu::CPU &cpu = _machine->cpu();
		Emu::Memory &mem = _machine->mem();
		uint32 var = cpu.call(0x477050, ((EmuObject *)o)->getAddr(), v & 0xffff);
		debug(1, "fuzz: var %d.%d = %d", obj, v & 0xffff, value);
		cpu.call(mem.r32(mem.r32(var) + 0x2c), var, (uint32)value);
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
			_machine->saveWorld(Common::String::format("C:\\MM2\\appdata\\SavedGame%u.m2s", x), y);
		} else if (!strcmp(kind, "quit")) {
			quitGame();
		}
	}
}

const Graphics::Font *IvanoffEngine::getFont(const Common::String &name, int size, int style) {
	// FUN_0048b9f0: CFont::CreatePointFont(size, name) with bold/italic bits.
	int px = MAX(1, (size * 96 + 360) / 720);
	Common::String key = Common::String::format("%s/%d/%d", name.c_str(), px, style & 3);
	if (_fonts.contains(key))
		return _fonts[key];

	// Liberation fonts have the metrics of Arial and Times New Roman.
	bool serif = name.contains("Times") || name.contains("Roman");
	Common::String file = Common::String("Liberation") + (serif ? "Serif" : "Sans") + "-";
	if ((style & 3) == 3)
		file += "BoldItalic";
	else if (style & 1)
		file += "Bold";
	else if (style & 2)
		file += "Italic";
	else
		file += "Regular";
	file += ".ttf";

	Graphics::Font *font = nullptr;
#ifdef USE_FREETYPE2
	font = Graphics::loadTTFFontFromArchive(file, px, Graphics::kTTFSizeModeCharacter, 0, 0,
	                                        Graphics::kTTFRenderModeMonochrome);
#endif
	if (!font) {
		warning("Can't load %s; using the GUI font", file.c_str());
		return FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);
	}
	_fonts[key] = font;
	return font;
}

// --- palette ---

void IvanoffEngine::initPalette() {
	for (int i = 0; i < 10; i++) {
		memcpy(_palette + i * 3, kStaticColors[i], 3);
		memcpy(_palette + (246 + i) * 3, kStaticColors[10 + i], 3);
	}
	_paletteDirty = true;
}

void IvanoffEngine::setGamePalette(const byte *palette) {
	// FUN_0047f4f0 copies 236 entries, starting at entry 10.
	if (memcmp(_palette + 10 * 3, palette + 10 * 3, 236 * 3)) {
		memcpy(_palette + 10 * 3, palette + 10 * 3, 236 * 3);
		_paletteDirty = true;
		_paletteRevision++;
	}
}

// --- cursors ---

void IvanoffEngine::loadCursors(const Common::Path &exe) {
	_defaultCursor = Graphics::makeDefaultWinCursor();
	_busyCursor = Graphics::makeBusyWinCursor();

	Common::WinResources *res = Common::WinResources::createFromEXE(exe);
	if (!res) {
		warning("Can't read the cursors from %s", exe.toString().c_str());
		return;
	}
	for (uint i = 0; i < ARRAYSIZE(kCursorResources); i++) {
		Graphics::WinCursorGroup *group = Graphics::WinCursorGroup::createCursorGroup(res, kCursorResources[i].resource);
		if (group && !group->cursors.empty()) {
			_cursorGroups.push_back(group);
			_cursors[kCursorResources[i].id] = group->cursors[0].cursor;
		} else {
			delete group;
		}
	}
	delete res;
}

void IvanoffEngine::setGlobalCursor(uint32 cursor) {
	_globalCursor = cursor;
	_shownCursor = 0;
	updateCursor();
}

void IvanoffEngine::updateCursor() {
	// FUN_0047dfa0
	uint32 want;
	if (_globalCursor == 0 && _hover) {
		want = _hover->getCursor();
		if (want == kNoCursor)
			return;
	} else if (_globalCursor == 0) {
		want = 0x80000003;
	} else {
		want = _globalCursor;
	}
	if (want != _shownCursor)
		applyCursor(want);
}

void IvanoffEngine::applyCursor(uint32 cursor) {
	_shownCursor = cursor;
	if (cursor == 0x80000001) {
		CursorMan.showMouse(false);
		return;
	}
	Graphics::Cursor *c = nullptr;
	if (cursor == 0x80000002)
		c = _busyCursor;
	else if (_cursors.contains(cursor))
		c = _cursors[cursor];
	else if (cursor == 0x80000003 || (cursor & 0x80000000))
		c = _defaultCursor;
	if (!c) {
		// Other values name a child of the engine's input object, drawn
		// with its top-left corner at the mouse position (FUN_0047e040).
		GameObject *k = _machine->kernel();
		Sprite *sp = k ? dynamic_cast<Sprite *>(k->getChild(cursor & 0xffff)) : nullptr;
		if (sp) {
			sp->load();
			const Graphics::Surface *surf = sp->getSurface();
			if (surf) {
				int key = sp->getTransparent() >= 0 ? sp->getTransparent() : 250;
				CursorMan.replaceCursor(*surf, 0, 0, key);
				CursorMan.showMouse(true);
				return;
			}
		}
		warning("Cursor %08x not found", cursor);
		c = _defaultCursor;
	}

	// Map the cursor's colours to the game palette, keeping index 250 free
	// for the transparent colour.
	uint w = c->getWidth(), h = c->getHeight();
	const byte *src = c->getSurface();
	const byte *pal = c->getPalette();
	byte *buf = new byte[w * h];
	for (uint i = 0; i < w * h; i++) {
		byte p = src[i];
		if (p == c->getKeyColor()) {
			buf[i] = 250;
			continue;
		}
		int idx = p - c->getPaletteStartIndex();
		byte r = 0, g = 0, b = 0;
		if (pal && idx >= 0 && idx < c->getPaletteCount()) {
			r = pal[idx * 3];
			g = pal[idx * 3 + 1];
			b = pal[idx * 3 + 2];
		}
		int best = 0, bestDist = 0x7fffffff;
		for (int k = 0; k < 256; k++) {
			if (k == 250)
				continue;
			int dr = _palette[k * 3] - r, dg = _palette[k * 3 + 1] - g, db = _palette[k * 3 + 2] - b;
			int d = dr * dr + dg * dg + db * db;
			if (d < bestDist) {
				bestDist = d;
				best = k;
			}
		}
		buf[i] = best;
	}
	CursorMan.replaceCursor(buf, w, h, c->getHotspotX(), c->getHotspotY(), 250);
	CursorMan.showMouse(true);
	delete[] buf;
}

// --- children and drawing ---

void IvanoffEngine::updateChildren(uint32 now) {
	const Common::Array<GameObject *> &objs = _world->getObjects();
	for (uint i = 0; i < objs.size(); i++) {
		const Common::Array<Child *> &children = objs[i]->getChildren();
		for (uint j = 0; j < children.size(); j++)
			children[j]->update(now);
	}
}

void IvanoffEngine::collectSprites(Common::Array<Sprite *> &list) {
	// All visible sprites, sorted by z (stable: later children on top).
	const Common::Array<GameObject *> &objs = _world->getObjects();
	for (uint i = 0; i < objs.size(); i++) {
		const Common::Array<Child *> &children = objs[i]->getChildren();
		for (uint j = 0; j < children.size(); j++) {
			Sprite *s = dynamic_cast<Sprite *>(children[j]);
			if (s && s->isShown())
				list.push_back(s);
		}
	}
	for (uint i = 1; i < list.size(); i++) {
		Sprite *s = list[i];
		int j = i - 1;
		while (j >= 0 && list[j]->getZ() > s->getZ()) {
			list[j + 1] = list[j];
			j--;
		}
		list[j + 1] = s;
	}
}

void IvanoffEngine::render() {
	Common::Array<Sprite *> list;
	collectSprites(list);

	// Skip the composition when nothing visible changed (small devices).
	uint32 sig = list.size();
	for (uint i = 0; i < list.size(); i++) {
		Sprite *sp = list[i];
		uint32 v = (uint32)(uintptr)sp ^ ((uint32)(uintptr)sp->getSurface() << 7) ^ (sp->getX() << 3) ^
		           (sp->getY() << 17) ^ (sp->getZ() << 11) ^ sp->getRevision();
		sig = (sig ^ v) * 16777619;
	}
	if (sig == _lastSignature && !_paletteDirty && !_forceRedraw) {
		_system->updateScreen();
		dumpFrame();
		return;
	}
	_lastSignature = sig;
	_forceRedraw = false;

	_screen.fillRect(Common::Rect(640, 480), 0);
	for (uint i = 0; i < list.size(); i++)
		list[i]->draw(_screen);

	if (_paletteDirty) {
		_system->getPaletteManager()->setPalette(_palette, 0, 256);
		_paletteDirty = false;
		uint32 c = _shownCursor;
		if (c)
			applyCursor(c);
	}
	_system->copyRectToScreen(_screen.getPixels(), _screen.pitch, 0, 0, 640, 480);
	_system->updateScreen();
	dumpFrame();
}

void IvanoffEngine::dumpFrame() {
	// Debugging aid: "dump_frames=<folder>" writes a screenshot every second.
	if (ConfMan.hasKey("dump_frames")) {
		uint32 now = getMillis();
		if (now - _lastDump >= 1000) {
			_lastDump = now;
			Common::Path path(ConfMan.get("dump_frames"));
			path = path.join(Common::String::format("frame%05d.png", _dumpCount++));
			Common::DumpFile f;
			if (f.open(path))
				Image::writePNG(f, _screen, _palette);
		}
	}

}

// --- input ---

Child *IvanoffEngine::hitTest(int x, int y, uint32 mask) {
	// FUN_0047bb70: the topmost child with one of the input flags in mask.
	Common::Array<Sprite *> list;
	collectSprites(list);
	for (int i = list.size() - 1; i >= 0; i--) {
		Sprite *s = list[i];
		if (s->_busy || !(s->getFlags() & mask))
			continue;
		if (s->hitTest(x, y))
			return s;
	}
	return nullptr;
}

/** The original's key for a child: (child id << 16) | owner id. */
static int32 childKey(Child *c) {
	return c ? (int32)(((uint32)c->getId() << 16) | c->getOwner()->getId()) : 0;
}

static int32 sendTo(Child *c, uint16 msg, int32 p1, int32 p2, int32 p3) {
	return c->getOwner()->send(kSelf, msg, p1, p2, p3);
}

void IvanoffEngine::onMouseMove(int x, int y) {
	// FUN_0047f570 / FUN_0047f2b0
	_mouseX = x;
	_mouseY = y;
	for (uint i = 0; i < _attached.size(); i++)
		_attached[i].sprite->setPos(x - _attached[i].x, y - _attached[i].y, -1);

	Child *c = hitTest(x, y, kFlagInputMask);
	if (c != _hover) {
		int32 oldKey = childKey(_hover), newKey = childKey(c);
		if (_hover && (_hover->getFlags() & kFlagEnterLeave))
			sendTo(_hover, kMsgMouseLeave, _hover->getId(), newKey, _hover->localCoords(x, y));
		if (c && (c->getFlags() & kFlagEnterLeave))
			sendTo(c, kMsgMouseEnter, c->getId(), oldKey, c->localCoords(x, y));
		_hover = c;
	}
	dragOver(x, y);
	if (_hover && (_hover->getFlags() & kFlagMouseMove))
		sendTo(_hover, kMsgMouseMove, _hover->getId(), (y << 16) | (x & 0xffff), _hover->localCoords(x, y));
	updateCursor();
}

void IvanoffEngine::dragOver(int x, int y) {
	// FUN_0047e480: tell the dragged child's owner whether it is over a target.
	if (!_dragged)
		return;
	if (_hover && sendTo(_hover, kMsgDropTest, childKey(_dragged), _hover->getId(), _hover->localCoords(x, y)))
		sendTo(_dragged, kMsgDragEnd, _dragged->getId(), childKey(_hover), _hover->localCoords(x, y));
	else
		sendTo(_dragged, kMsgDragEnd, _dragged->getId(), 0, 0);
}

void IvanoffEngine::endDrag() {
	// FUN_004817d0
	_dragged = nullptr;
}

bool IvanoffEngine::dropOnClick(int x, int y) {
	// FUN_0047e590: flag 0x100 children are picked up by one click and
	// dropped by the next. Returns whether the click was used.
	if (!_dragged || !(_dragged->getFlags() & kFlagDropTarget))
		return false;
	bool alt = (_dragged->getFlags() & 0x300) == 0x300;
	Child *d = _dragged;
	if (_hover) {
		int32 local = _hover->localCoords(x, y);
		bool ok = true;
		if (alt)
			ok = sendTo(_hover, kMsgDropTestAlt, childKey(d), _hover->getId(), local) != 0;
		if (ok && sendTo(_hover, kMsgDropTest, childKey(d), _hover->getId(), local)) {
			sendTo(d, kMsgDropped, d->getId(), childKey(_hover), local);
			if (sendTo(_hover, kMsgDrop, childKey(d), _hover->getId(), local) == 0)
				endDrag();
			return true;
		}
	}
	if (!alt && sendTo(d, kMsgClickNoDrag, d->getId(), 0, 0) == 0)
		endDrag();
	return true;
}

void IvanoffEngine::onButton(int x, int y, bool down, bool right) {
	int32 pos = (y << 16) | (x & 0xffff);
	if (right) {
		// FUN_0047f800
		Child *c = hitTest(x, y, 0x404);
		if (c && (c->getFlags() & kFlagRightClick))
			sendTo(c, kMsgRightClick, c->getId(), pos, c->localCoords(x, y));
		return;
	}
	if (down) {
		// FUN_0047f630
		Child *c = hitTest(x, y, 0x581);
		if (!c) {
			dropOnClick(x, y);
			return;
		}
		if (c->getId() == 0 || c->_busy || dropOnClick(x, y))
			return;
		if ((c->getFlags() & (kFlagDraggable | kFlagDropTarget)) && !_dragged &&
		    sendTo(c, kMsgCanDrag, c->getId(), 0, 0) == 0) {
			// FUN_00481710: start dragging.
			uint16 msg = (c->getFlags() & kFlagDraggable) ? kMsgDragStart : kMsgDragStartAlt;
			sendTo(c, msg, c->getId(), pos, c->localCoords(x, y));
			_dragged = c;
			dragOver(x, y);
		}
		if (c->getFlags() & kFlagClick)
			sendTo(c, kMsgClick, c->getId(), pos, c->localCoords(x, y));
	} else {
		// FUN_0047f740
		Child *c = hitTest(x, y, 0x402);
		if (c && (c->getFlags() & kFlagButtonUp))
			sendTo(c, kMsgButtonUp, c->getId(), pos, c->localCoords(x, y));

		// FUN_0047e7e0: release a press-and-drag child.
		if (_dragged && (_dragged->getFlags() & kFlagDraggable)) {
			Child *d = _dragged;
			if (_hover && sendTo(_hover, kMsgDropTest, childKey(d), _hover->getId(), _hover->localCoords(x, y))) {
				int32 local = _hover->localCoords(x, y);
				sendTo(d, kMsgDropped, d->getId(), childKey(_hover), local);
				sendTo(_hover, kMsgDrop, childKey(d), _hover->getId(), local);
			} else {
				sendTo(d, kMsgClickNoDrag, d->getId(), 0, 0);
			}
			endDrag();
		}
	}
}

void IvanoffEngine::onKey(int keycode, int ascii, bool down) {
	// FUN_0047f940 / FUN_0047f9c0: p1 is the Windows virtual key code.
	int vk = keycode;
	if (keycode >= Common::KEYCODE_a && keycode <= Common::KEYCODE_z)
		vk = keycode - Common::KEYCODE_a + 'A';
	else if (keycode == Common::KEYCODE_ESCAPE)
		vk = 0x1b;
	else if (keycode == Common::KEYCODE_RETURN)
		vk = 0x0d;
	else if (keycode == Common::KEYCODE_SPACE)
		vk = 0x20;
	Message m(kSelf, 0, down ? kMsgKeyDown : kMsgKeyUp, vk, ascii);
	_world->deliverFiltered(m);
}

// --- dragging and groups ---

void IvanoffEngine::attachToCursor(Sprite *sprite, int x, int y, int32 param) {
	detachFromCursor(sprite);
	Attached a;
	a.sprite = sprite;
	a.x = x;
	a.y = y;
	a.savedZ = sprite->getZ();
	_attached.push_back(a);
	sprite->setPos(_mouseX - x, _mouseY - y, 0xffffffe);
	if (!sprite->isVisible())
		sprite->play(0, 0, 0);
	sprite->_busy = true;
}

void IvanoffEngine::detachFromCursor(Sprite *sprite) {
	for (uint i = 0; i < _attached.size(); i++) {
		if (_attached[i].sprite == sprite) {
			sprite->setZ(_attached[i].savedZ);
			sprite->_busy = false;
			_attached.remove_at(i);
			return;
		}
	}
}

void IvanoffEngine::groupAdd(int32 key) {
	_groups[key]++;
}

int IvanoffEngine::groupDone(int32 key) {
	if (!_groups.contains(key))
		return 0;
	int n = --_groups[key];
	if (n <= 0)
		_groups.erase(key);
	return MAX(n, 0);
}

} // End of namespace Ivanoff
