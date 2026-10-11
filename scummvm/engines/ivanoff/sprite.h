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

#ifndef IVANOFF_SPRITE_H
#define IVANOFF_SPRITE_H

#include "audio/mixer.h"
#include "common/hashmap.h"
#include "common/array.h"
#include "common/rect.h"
#include "graphics/surface.h"

#include "ivanoff/world.h"

namespace Video {
class VideoDecoder;
}

namespace Ivanoff {

enum ChildType {
	kTypeAnim = 0xfdff,
	kTypeSound = 0xfe00,
	kTypeBitmap = 0xfe01,
	kTypeTextRes = 0xfe02,
	kTypeHotspot = 0xfe03,
	kTypeVideo = 0xfe04,
	kTypeText = 0xfe05
};

/** Flags at +0x30 of the original object (low 11 bits: input flags). */
enum {
	kFlagClick = 0x1,       ///< receives clicks (msg 5000)
	kFlagButtonUp = 0x2,
	kFlagRightClick = 0x4,
	kFlagDoubleClick = 0x8,
	kFlagHasCursor = 0x10,
	kFlagMouseMove = 0x20,
	kFlagEnterLeave = 0x40,
	kFlagDraggable = 0x80,
	kFlagDropTarget = 0x100,
	kFlagDropTargetAlt = 0x200,
	kFlagInputMask = 0x7ff
};

/** Cursor value meaning "no cursor of its own" (FUN_00477a90). */
const uint32 kNoCursor = 0x80000000;

/**
 * A child object of a GameObject (base FUN_00477560). Children are
 * addressed by their resource id, which is also their id here.
 */
class Child {
public:
	Child(uint16 id, uint16 type, uint32 flags);
	virtual ~Child() {}

	uint16 getId() const { return _id; }
	uint16 getType() const { return _type; }
	GameObject *getOwner() const { return _owner; }
	void setOwner(GameObject *owner) { _owner = owner; }

	/** vtable +0x34: dispatch a child command (FUN_004814a0 / FUN_00477950). */
	virtual void handleCommand(Message &msg);

	/** FUN_00477a90 */
	void setCursor(uint32 cursor);
	uint32 getCursor() const { return _cursor; }
	/** FUN_00477830 */
	void setFlags(uint32 flags);
	uint32 getFlags() const { return _flags; }

	bool isLoaded() const { return _loaded; }
	bool isVisible() const { return _visible; }
	bool isDisabled() const { return _disabled; }

	// The commands, as in the original vtable.
	virtual void load() { _loaded = true; }        // +0x38
	virtual void unload() {                        // +0x3c
		if (_visible)
			stop();
		_visible = false;
		_loaded = false;
	}
	virtual void play(int32 p1, int32 p2, int32 p3) {} // +0x40
	virtual void stop() {}                         // +0x44
	virtual void enable(int32 on) {}               // +0x48
	virtual void setPos(int32 x, int32 y, int32 z) {} // +0x4c
	virtual void setZ(int32 z) {}                  // +0x50
	virtual int32 getPaletteResult() { return 0; }

	/** Is (x, y) inside the object? (vtable +0xa8 bounding box, +0xac pixel test) */
	virtual bool hitTest(int x, int y) const { return false; }
	/** vtable +0x2c: object-relative coordinates for event messages */
	virtual int32 localCoords(int x, int y) const { return (y << 16) | (x & 0xffff); }

	/** Called every frame. */
	virtual void update(uint32 now) {}
	virtual bool isPaused() const { return false; }
	bool testBit(uint16 bit) const;

	/** The child's memory in the emulated game (see Machine), or 0. */
	uint32 _addr = 0;

	/** Disable input for a while (+0x14 "busy"). */
	bool _busy = false;
	/** +0x54 (command 0x2c) */
	bool _inputDisabled = false;

protected:
	uint16 _id;
	uint16 _type;
	GameObject *_owner = nullptr;
	uint32 _flags;
	uint32 _cursor = kNoCursor;
	bool _loaded = false;
	bool _visible = false;
	bool _disabled = false;
	uint16 _altResource = 0; ///< +0x50: load this resource instead of _id
	/** The resource to load: +0x50, which the game sets directly in the child's memory, or the id. */
	uint16 resourceId() const;
	Common::Array<uint16> _bits; ///< FUN_00477860 flag bits (commands 0x29-0x2b)

	/** Sends the "done" notification for play() parameter p1 (FUN_0047fab0). */
	void notifyDone(int32 notify, uint16 defaultMsg, int32 param);
};

/** A drawable child (FUN_004812d0). */
class Sprite : public Child {
public:
	Sprite(uint16 id, uint16 type, int32 transparent, uint32 flags, uint16 param);
	~Sprite() override;

	void handleCommand(Message &msg) override;

	void play(int32 p1, int32 p2, int32 p3) override;
	void stop() override;
	void enable(int32 on) override;
	void setPos(int32 x, int32 y, int32 z) override;
	void setZ(int32 z) override;

	/** FUN_00481c30: set the position without loading. */
	void setInitialPos(int32 x, int32 y) {
		_x = x;
		_y = y;
	}

	Common::Rect getBounds() const;
	bool isShown() const { return _visible && _shown; }

	/** +0x7c: drawing mode (vtable +0x28). */
	int32 _mode = 0;

	/** Changes when the image changes without a new surface (e.g. a large animation's frame). */
	virtual uint32 getRevision() const { return 0; }

	int32 getX() const { return _x; }
	int32 getY() const { return _y; }
	int32 getZ() const { return _z; }

	/** The current image, or nullptr. */
	virtual const Graphics::Surface *getSurface() const { return nullptr; }
	/** The palette of the image (256 RGB triplets), or nullptr. */
	virtual const byte *getPalette() const { return nullptr; }
	int32 getTransparent() const { return _transparent; }

	bool hitTest(int x, int y) const override;
	int32 localCoords(int x, int y) const override { return ((y - _y) << 16) | ((x - _x) & 0xffff); }

	/** Draw onto an 8-bit screen surface. */
	virtual void draw(Graphics::Surface &dst) const;

	/** Command 0x11: show a w x h part of the image (0 = all). */
	virtual void setView(int32 w, int32 h);
	/** Commands 0x12 / 0x13: scroll the shown part; returns (dy << 16) | dx actually moved. */
	int32 scrollTo(int32 x, int32 y, bool clamp);
	int32 scrollBy(int32 dx, int32 dy, bool clamp);

protected:
	int32 _x = 0, _y = 0, _z = 0;
	int32 _viewW = 0, _viewH = 0; ///< image +0x70/+0x74 [unverified]
	int32 _srcX = 0, _srcY = 0;
	int32 _transparent; ///< +0x5c: palette index drawn transparent, -1 = opaque
	uint16 _param;      ///< +0x60
	bool _shown = true; ///< command 5
};

/** FUN_00477b20: a still image (BMP). */
class Bitmap : public Sprite {
public:
	Bitmap(uint16 id, int32 transparent, uint32 flags, int32 mode);
	~Bitmap() override;

	void handleCommand(Message &msg) override;
	/** Command 0x35 (FUN_00477de0): load a BMP from memory instead of the resources. */
	void loadFromMemory(Common::SeekableReadStream &stream);

	void load() override;
	void unload() override;

	const Graphics::Surface *getSurface() const override { return _surface; }
	const byte *getPalette() const override { return _palette; }
	uint32 getRevision() const override { return _revision; }

private:
	bool decode(Common::SeekableReadStream &stream);
	Graphics::Surface *_surface = nullptr;
	byte _palette[256 * 3];
	uint32 _revision = 0;
};

/** FUN_00478140: an animation (FLC). */
class Anim : public Sprite {
public:
	Anim(uint16 id, int32 transparent, uint32 flags, uint16 param);
	~Anim() override;

	void handleCommand(Message &msg) override;
	void load() override;
	void unload() override;
	void play(int32 p1, int32 p2, int32 p3) override;
	void stop() override;
	void update(uint32 now) override;

	const Graphics::Surface *getSurface() const override;
	const byte *getPalette() const override { return _palette; }

	int frameCount() const { return _frameCount; }
	uint32 getRevision() const override { return _frame; }
	bool isPaused() const override { return _paused; }
	int currentFrame() const { return _frame; }
	void gotoFrame(int frame);

	/** Commands 0xb / 0xc */
	void pause();
	void resume(int32 start, int32 end);

	/** Command 9 (FUN_00481b50): deliver msg whenever frame is shown; no message removes the frame's. */
	void setFrameTrigger(int frame, const Message *msg);

private:
	void decodeTo(int frame);
	void finish();

	static const int32 kNoFrame = -0x8000;
	static const uint32 kCacheLimit = 1024 * 1024;

protected:
	/** Opens the frames' source (an FLC from the resources). */
	virtual Video::VideoDecoder *openDecoder();
	/** Opens the decoder and decodes or caches the frames; sets _loaded. */
	void loadFrames();
	void releaseFrames();
	/** Converts a decoded frame to the 8-bit screen format. */
	virtual void convertFrame(const Graphics::Surface &src, Graphics::Surface &dst);

	Video::VideoDecoder *_decoder = nullptr; ///< kept for large animations only
	Graphics::Surface *_current = nullptr;  ///< current frame of a large animation
	int _decodedFrame = -1;
	int _frameCount = 0;
	uint16 _width = 0, _height = 0;
	int _startFrame = 0;
	int32 _pendingFrame = 0; ///< +0x6c: command 0x26 before play()
	Common::HashMap<int, Common::Array<Message> > _triggers;
	void fireTriggers();
	Common::Array<Graphics::Surface *> _frames;
	byte _palette[256 * 3];
	int _frame = 0;
	int _endFrame = 0;
	bool _loop = false;
	bool _reverse = false;
	bool _playing = false;
	bool _paused = false;
	uint32 _period = 50; ///< ms per frame; FLC speed 0 means 50 (FUN_00484xxx)
	uint32 _nextFrameTime = 0;
	int32 _notify = 0;   ///< play() p1: target object (low 16) and message (high 16)
	int32 _notifyParam = 0; ///< +0x40
};

/** One polygon of a .HSP hotspot. */
struct HotspotRegion {
	uint32 id;
	uint32 type; ///< 0 rectangle, 1 ellipse, 2 polygon
	Common::Array<Common::Point> points;
	Common::Rect bounds;

	bool contains(int x, int y) const;
};

/** FUN_004786a0: a clickable region (.HSP). */
class Hotspot : public Sprite {
public:
	Hotspot(uint16 id, uint32 flags);

	void handleCommand(Message &msg) override;
	void load() override;
	/** FUN_00478d60 */
	void unload() override {
		Sprite::unload();
		_regions.clear();
	}
	void play(int32 p1, int32 p2, int32 p3) override;
	bool hitTest(int x, int y) const override;
	int32 localCoords(int x, int y) const override { return regionAt(x, y); }
	void draw(Graphics::Surface &dst) const override {}

	/** The id of the region containing (x, y), 0 if none (FUN_00478790). */
	uint32 regionAt(int x, int y) const;
	/** Command 0x17: the centre of region id (or all regions if all == 0), as y << 16 | x. */
	int32 regionCenter(int32 all, int32 id) const;
	bool anyRegionContains(int x, int y) const;
	/** For testing: a point inside each region. */
	Common::Array<Common::Point> regionPoints() const;

private:
	Common::Array<HotspotRegion> _regions;
};

/** FUN_00478e40: a sound (WAV). */
class Sound : public Child {
public:
	Sound(uint16 id, uint16 param);
	~Sound() override;

	void handleCommand(Message &msg) override;
	void play(int32 p1, int32 p2, int32 p3) override;
	void stop() override;
	void update(uint32 now) override;
	bool isPlaying() const;

private:
	Audio::SoundHandle _handle;
	uint16 _param;
	bool _playing = false;
	int32 _notify = 0;
	int32 _notifyParam = 0;
	/** With the virtual clock, sounds aren't played and end at this time. */
	uint32 _endTime = 0;
	bool isActive() const;
};

/** FUN_00479400: a text string from the language pack (0xfe02). */
class TextResource : public Child {
public:
	TextResource(uint16 id) : Child(id, kTypeTextRes, 0) {}

	void handleCommand(Message &msg) override;
	void load() override;
	/** FUN_004796c0: show the text in text sprite p1 = (object << 16) | child. */
	void play(int32 p1, int32 p2, int32 p3) override;
	/** FUN_00479740 */
	void stop() override;

	const Common::String &getText() const { return _text; }

private:
	Common::String _text;
	uint32 _target = 0;
};

/** FUN_004797f0: a text box (0xfe05), drawn like GDI's DrawText without antialiasing. */
class TextSprite : public Sprite {
public:
	TextSprite(uint16 id, int32 fg, int32 bg, const Common::String &font, int32 size, int32 wrap,
	           int32 mode, int32 style, uint32 source, uint32 flags);
	~TextSprite() override;

	void handleCommand(Message &msg) override;
	void load() override;
	void play(int32 p1, int32 p2, int32 p3) override;
	const Graphics::Surface *getSurface() const override { return _surface; }
	uint32 getRevision() const override { return _revision; }
	/** FUN_0047ab30/0047ab60: text images have no mask, so the whole box counts. */
	bool hitTest(int x, int y) const override { return isShown() && getBounds().contains(x, y); }

	/** FUN_0047a2f0: show the text of a string child, (child << 16) | object; 0 = none. */
	void setSource(uint32 source);
	/** FUN_0047a340: show a literal text. */
	void setText(const Common::String &text);
	/** FUN_0047a920 (command 0x11): the box size; 0 = fit the text. */
	void setBox(int32 w, int32 h);
	/** Redraws if the text of the source changed. */
	void refresh();

private:
	Common::String currentText() const;
	void render();

	Graphics::Surface *_surface = nullptr;
	uint32 _revision = 0;
	Common::String _literal;
	Common::String _shown;
	Common::String _font;
	int32 _fg, _bg, _size, _wrap, _mode, _style;
	uint32 _source; ///< +0x88: (child << 16) | object, 0xffffffff = literal
	int _boxW = 0, _boxH = 0;
};

/** FUN_0047af30: an AVI video (0xfe04), used in the labyrinths; otherwise an animation. */
class VideoSprite : public Anim {
public:
	VideoSprite(uint16 id, int32 transparent, const Common::String &file, uint32 flags, int32 param);

	void load() override;
	void play(int32 p1, int32 p2, int32 p3) override;
	void stop() override;

protected:
	Video::VideoDecoder *openDecoder() override;
	void convertFrame(const Graphics::Surface &src, Graphics::Surface &dst) override;

private:
	Common::String _file;
	Common::Array<int16> _lut;
	uint32 _lutRevision = 0;
};

} // End of namespace Ivanoff

#endif
