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

#ifndef IVANOFF_H
#define IVANOFF_H

#include "common/array.h"
#include "common/error.h"
#include "common/hashmap.h"
#include "common/random.h"
#include "common/system.h"
#include "engines/engine.h"
#include "graphics/surface.h"

#include "ivanoff/detection.h"
#include "ivanoff/resources.h"

namespace Graphics {
class Cursor;
class Font;
struct WinCursorGroup;
}

namespace Ivanoff {

class Child;
class Machine;
class Sprite;
class World;

class IvanoffEngine : public Engine {
public:
	IvanoffEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~IvanoffEngine() override;

	Common::String getGameId() const;
	Common::Language getLanguage() const { return _gameDescription->language; }
	const Common::String &getTarget() const { return _targetName; }
	/** The language pack's folder: DK, N, S or SF. */
	Common::String getLanguageDir() const;

	uint32 getRandomNumber(uint maxNum) { return _randomSource.getRandomNumber(maxNum); }

	bool hasFeature(EngineFeature f) const override {
		return f == kSupportsReturnToLauncher;
	}

	Resources _resources;
	World *_world = nullptr;
	Machine *_machine = nullptr;

	/** FUN_00477120: the cursor shown everywhere (0 = per sprite). */
	void setGlobalCursor(uint32 cursor);
	const byte *getScreenPalette() const { return _palette; }
	/** Changes whenever the palette changes. */
	uint32 getPaletteRevision() const { return _paletteRevision; }
	/** Command 10: entries 10-245 of the screen palette from an image's palette. */
	void setGamePalette(const byte *palette);

	/** Command 0x14: the sprite follows the mouse, held at (x, y). */
	void attachToCursor(Sprite *sprite, int x, int y, int32 param);
	void detachFromCursor(Sprite *sprite);

	/** A font like the GDI one the game asks for (size in tenths of a point at 96 dpi). */
	const Graphics::Font *getFont(const Common::String &name, int size, int style);

	/** Draws the screen now (the original's synchronous repaint). */
	void redrawNow() { render(); }

	/** Notification groups (FUN_004875b0 / FUN_00487670). */
	void groupAdd(int32 key);
	/** Returns the number of members still running. */
	int groupDone(int32 key);

protected:
	Common::Error run() override;

private:
	void initPalette();
	void loadCursors(const Common::Path &exe);
	void updateCursor();
	void applyCursor(uint32 cursor);

	void updateChildren(uint32 now);
	void collectSprites(Common::Array<Sprite *> &list);
	void render();

	Child *hitTest(int x, int y, uint32 mask);
	void onMouseMove(int x, int y);
	void onButton(int x, int y, bool down, bool right);
	void onKey(int keycode, int ascii, bool down);

	const ADGameDescription *_gameDescription;
	Common::RandomSource _randomSource;

	Graphics::Surface _screen;
	byte _palette[256 * 3];
	bool _paletteDirty = true;
	uint32 _paletteRevision = 1;

	Common::HashMap<uint32, Graphics::Cursor *> _cursors;
	Common::Array<Graphics::WinCursorGroup *> _cursorGroups;
	Graphics::Cursor *_defaultCursor = nullptr;
	Graphics::Cursor *_busyCursor = nullptr;
	uint32 _globalCursor = 0;
	uint32 _shownCursor = 0;
	int _mouseX = 0, _mouseY = 0;

	Child *_hover = nullptr;
	Child *_dragged = nullptr; ///< input manager +0x34
	void dragOver(int x, int y);
	bool dropOnClick(int x, int y);
	void endDrag();

	struct Attached {
		Sprite *sprite;
		int x, y;
		int32 savedZ;
	};
	Common::Array<Attached> _attached;

	Common::HashMap<int32, int> _groups;
	Common::HashMap<Common::String, Graphics::Font *> _fonts;

	void runDebugEvents(uint32 now);
	uint32 _debugStart = 0;
	int _debugEventIndex = 0;
	int _crawlLeft = 0, _crawlIndex = 0;
	uint32 _crawlNext = 0, _crawlInterval = 2500;

	uint32 _lastSignature = 0;
	bool _forceRedraw = true;

	uint32 _statsTime = 0;
	uint64 _statsInstructions = 0;

	uint32 _lastDump = 0;
	int _dumpCount = 0;
};

extern IvanoffEngine *g_engine;

} // End of namespace Ivanoff

#endif // IVANOFF_H
