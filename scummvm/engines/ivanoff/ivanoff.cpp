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
#include "common/file.h"
#include "common/formats/winexe.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/cursor.h"
#include "graphics/cursorman.h"
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
		_world->update();
		updateChildren(now);
		updateCursor();
		render();
		_system->delayMillis(10);
	}

	return Common::kNoError;
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
	memcpy(_palette + 10 * 3, palette + 10 * 3, 236 * 3);
	_paletteDirty = true;
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
		// [unverified] Other values name an animated sprite cursor (FUN_0047e040).
		debug(1, "Sprite cursor %08x not implemented", cursor);
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

void IvanoffEngine::onMouseMove(int x, int y) {
	// FUN_0047f570 / FUN_0047f2b0
	_mouseX = x;
	_mouseY = y;
	for (uint i = 0; i < _attached.size(); i++)
		_attached[i].sprite->setPos(x - _attached[i].x, y - _attached[i].y, -1);

	Child *c = hitTest(x, y, kFlagInputMask);
	if (c != _hover) {
		if (_hover && (_hover->getFlags() & kFlagEnterLeave))
			_hover->getOwner()->send(kSelf, kMsgMouseLeave, _hover->getId(), 0, _hover->localCoords(x, y));
		if (c && (c->getFlags() & kFlagEnterLeave))
			c->getOwner()->send(kSelf, kMsgMouseEnter, c->getId(), 0, c->localCoords(x, y));
		_hover = c;
	}
	if (_hover && (_hover->getFlags() & kFlagMouseMove))
		_hover->getOwner()->send(kSelf, kMsgMouseMove, _hover->getId(), (y << 16) | (x & 0xffff),
		                         _hover->localCoords(x, y));
	updateCursor();
}

void IvanoffEngine::onButton(int x, int y, bool down, bool right) {
	int32 pos = (y << 16) | (x & 0xffff);
	if (right) {
		// FUN_0047f800
		Child *c = hitTest(x, y, 0x404);
		if (c && (c->getFlags() & kFlagRightClick))
			c->getOwner()->send(kSelf, kMsgRightClick, c->getId(), pos, c->localCoords(x, y));
		return;
	}
	if (down) {
		// FUN_0047f630. [unverified] Dragging (flags 0x80/0x100) is not implemented yet.
		Child *c = hitTest(x, y, 0x581);
		if (c && (c->getFlags() & kFlagClick))
			c->getOwner()->send(kSelf, kMsgClick, c->getId(), pos, c->localCoords(x, y));
	} else {
		// FUN_0047f740
		Child *c = hitTest(x, y, 0x402);
		if (c && (c->getFlags() & kFlagButtonUp))
			c->getOwner()->send(kSelf, kMsgButtonUp, c->getId(), pos, c->localCoords(x, y));
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
