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

	// The application object starts the game by sending message 1 to
	// object 17 (FUN_0047b5d0: start object 0x11).
	Message init(kGameState, 0, kMsgInit);
	_world->deliver(nullptr, init);

	CursorMan.showMouse(true);
	applyCursor(0x80000003);

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

		uint32 now = _system->getMillis();
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
		_system->delayMillis(10);
	}

	return Common::kNoError;
}

// Debugging aid: "debug_events=ms:click:x,y ms:key:c ..." replays input at
// the given times (milliseconds since start).
void IvanoffEngine::runDebugEvents(uint32 now) {
	if (!ConfMan.hasKey("debug_events"))
		return;
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

	// Debugging aid: "dump_frames=<folder>" writes a screenshot every second.
	if (ConfMan.hasKey("dump_frames")) {
		uint32 now = _system->getMillis();
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
