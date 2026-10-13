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
class GameFiles;
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

	/**
	 * The game's clock in milliseconds. With "virtual_clock=<ms>" it advances
	 * by that step per main loop iteration instead of following real time,
	 * which makes runs reproducible (for comparing traces).
	 */
	uint32 getMillis() const { return _virtualStep ? _virtualTime : _system->getMillis(); }
	bool hasVirtualClock() const { return _virtualStep != 0; }

	bool hasFeature(EngineFeature f) const override {
		return f == kSupportsReturnToLauncher || f == kSupportsLoadingDuringRuntime ||
		       f == kSupportsSavingDuringRuntime;
	}
	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override;
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override;
	Common::Error loadGameStream(Common::SeekableReadStream *stream) override;
	Common::Error saveGameStream(Common::WriteStream *stream, bool isAutosave = false) override;

	/** The save data for kMsgEngineLoad. */
	Common::Array<byte> _pendingLoad;

	Resources _resources;
	World *_world = nullptr;
	GameFiles *_files = nullptr;

	/** FUN_00477120: the cursor shown everywhere (0 = per sprite). */
	void setGlobalCursor(uint32 cursor);
	const byte *getScreenPalette() const { return _palette; }
	/** Changes whenever the palette changes. */
	uint32 getPaletteRevision() const { return _paletteRevision; }
	/** Command 10: entries 10-245 of the screen palette from an image's palette. */
	uint32 setGamePalette(const byte *palette, uint32 owner = 0);
	uint32 _paletteOwner = 0; ///< engine +0x58

	/** Command 0x14: the sprite follows the mouse, held at (x, y). */
	void attachToCursor(Sprite *sprite, int x, int y, int32 param);
	void detachFromCursor(Sprite *sprite);

	/** A font like the GDI one the game asks for (size in tenths of a point at 96 dpi). */
	const Graphics::Font *getFont(const Common::String &name, int size, int style);

	/**
	 * Debugging aid: with debug_events, a fault of the game code ends the
	 * run (traces of runs stay comparable) instead of stopping in the debugger.
	 */
	void debugFault(const char *what);

	/**
	 * Global variables of the original's game code (DAT_xxxxxxxx), by
	 * address; the initial values are those of mm2run.exe's data.
	 */
	int32 getGlobal(uint32 addr) const { return _globals.contains(addr) ? _globals[addr] : 0; }
	void setGlobal(uint32 addr, int32 value);
	/** The game's sound volume (DAT_004c6940, its low byte), for the mixer (0-255). */
	byte getGameVolume() const { return getGlobal(kGlobalVolume) & 0xff; }
	enum { kGlobalVolume = 0x4c6940 };

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
	void dumpFrame();

	Child *hitTest(int x, int y, uint32 mask);
	void onMouseMove(int x, int y);
	void onButton(int x, int y, bool down, bool right);
	void onKey(int keycode, int ascii, bool down);
	static int virtualKey(int keycode);

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
	void runDebugEvent(const char *kind, uint x, uint y, uint32 now);
	void pollDebugControl(uint32 now);
	uint32 _controlPolled = 0;
	uint _controlLines = 0;
	uint32 _debugStart = 0;
	int _debugEventIndex = 0;
	int _crawlLeft = 0, _crawlIndex = 0;
	// "fuzz:count,interval": messages to random objects (see runFuzz()).
	void loadFuzzDictionary();
	void runFuzz();
	struct FuzzEntry {
		uint16 object;
		Common::Array<int32> msgs, params;
		Common::Array<uint32> vars; ///< (object << 16) | id, object 0 = this one
	};
	Common::Array<FuzzEntry> _fuzz;
	int _fuzzLeft = 0;
	uint32 _fuzzNext = 0, _fuzzInterval = 500;
	uint32 _crawlNext = 0, _crawlInterval = 2500;

	uint32 _lastSignature = 0;
	bool _forceRedraw = true;


	Common::HashMap<uint32, int32> _globals;

	uint32 _virtualStep = 0;
	uint32 _virtualTime = 0;

	uint32 _lastDump = 0;
	int _dumpCount = 0;
};

extern IvanoffEngine *g_engine;

} // End of namespace Ivanoff

#endif // IVANOFF_H
