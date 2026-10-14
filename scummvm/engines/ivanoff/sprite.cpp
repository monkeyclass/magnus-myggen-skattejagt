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

#include "audio/audiostream.h"
#include "audio/decoders/wave.h"
#include "common/debug.h"
#include "common/memstream.h"
#include "common/system.h"
#include "image/bmp.h"
#include "graphics/font.h"
#include "graphics/fonts/ttf.h"
#include "graphics/fontman.h"
#include "video/avi_decoder.h"
#include "common/file.h"
#include "image/png.h"
#include "video/flic_decoder.h"

#include "ivanoff/ivanoff.h"
#include "ivanoff/files.h"
#include "ivanoff/sprite.h"

namespace Ivanoff {

// --- Child (FUN_00477560) ---

Child::Child(uint16 id, uint16 type, uint32 flags) : _id(id), _type(type) {
	setFlags(flags);
}

void Child::setCursor(uint32 cursor) {
	// FUN_00477a90: "no cursor" also clears the flags; a cursor makes the
	// child one the mouse finds (0x10). The cursor changes at once if the
	// mouse is over the child.
	_cursor = cursor;
	if (cursor == kNoCursor)
		_flags = 0;
	else
		_flags |= kFlagHasCursor;
	if (g_engine)
		g_engine->childCursorChanged(this);
}

void Child::setFlags(uint32 flags) {
	// FUN_00477830: input children get the default cursor.
	_flags = flags;
	setCursor((flags & kFlagInputMask) ? 0x80000003 : kNoCursor);
}

void Child::handleCommand(Message &msg) {
	// FUN_00477950
	uint16 bit = (uint16)msg.p1;
	switch (msg.cmd) {
	case 0:
		msg.result = 0;
		break;
	case 0x1a:
		msg.result = _loaded;
		break;
	case kCmdIsPlaying:
		msg.result = _visible;
		break;
	case 0x1d:
		msg.result = isPaused();
		break;
	case 0x21:
		msg.result = _visible && !isPaused();
		break;
	case kCmdSetCursor:
		setCursor(msg.p1);
		break;
	case kCmdSetFlags:
		setFlags(msg.p1);
		break;
	case kCmdGetFlags:
		msg.result = _flags;
		break;
	case 0x29: {
		bool found = false;
		for (uint i = 0; i < _bits.size(); i++)
			found = found || _bits[i] == bit;
		if (!found)
			_bits.push_back(bit);
		break;
	}
	case 0x2a:
		for (uint i = 0; i < _bits.size(); i++) {
			if (_bits[i] == bit) {
				_bits.remove_at(i);
				break;
			}
		}
		break;
	case 0x2b:
		msg.result = testBit(bit);
		break;
	case 0x2c:
		_inputDisabled = (msg.p1 == 0);
		break;
	default:
		debug(3, "Child %d: unhandled command %d", _id, msg.cmd);
		break;
	}
}

Child *Child::setAltResource(uint16 res) {
	_altResource = res;
	return this;
}

uint16 Child::resourceId() const {
	uint16 alt = _altResource;
	return alt ? alt : _id;
}

bool Child::testBit(uint16 bit) const {
	// FUN_00477910: the type counts as a tag.
	if (bit == _type)
		return true;
	for (uint i = 0; i < _bits.size(); i++)
		if (_bits[i] == bit)
			return true;
	return false;
}

void Child::notifyDone(int32 notify, uint16 defaultMsg, int32 param) {
	// FUN_0047fab0 (animations) / FUN_0047fb90 (sounds)
	if (!_owner || notify == 0)
		return;
	uint16 target = notify & 0xffff;
	uint16 msg = (uint32)notify >> 16;
	if (msg == 0) {
		_owner->send(target, defaultMsg, _id, param);
	} else if (!(notify & 0x80000000)) {
		_owner->send(target, msg, _id, param);
	} else {
		_owner->send(target, defaultMsg, _id, param);
		if (g_engine->groupDone(notify) == 0)
			_owner->send(target, msg, _id, param);
	}
}

// --- Sprite (FUN_004812d0) ---

Sprite::Sprite(uint16 id, uint16 type, int32 transparent, uint32 flags, uint16 param) :
	Child(id, type, flags), _transparent(transparent), _param(param) {
}

Sprite::~Sprite() {
	g_engine->detachFromCursor(this);
}

void Sprite::handleCommand(Message &msg) {
	// FUN_004814a0
	switch (msg.cmd) {
	case kCmdLoad:
		load();
		break;
	case kCmdUnload:
		unload();
		break;
	case kCmdPlay:
		play(msg.p1, msg.p2, msg.p3);
		break;
	case kCmdStop:
		stop();
		break;
	case kCmdEnable:
		enable(msg.p1);
		break;
	case kCmdSetPos:
		setPos(msg.p1, msg.p2, msg.p3 == 0 ? -1 : msg.p3);
		break;
	case kCmdSetZ:
		setZ(msg.p1);
		break;
	case kCmdSetOffset:
		// FUN_00481ab0: move by (p1, p2); walking characters use it from
		// animation frame messages.
		if (!_loaded)
			load();
		_x += msg.p1;
		_y += msg.p2;
		// FUN_00489ab6: and z by p3.
		_z += msg.p3;
		break;
	case kCmdSetClip:
		debug(2, "Sprite %d: command %d (%d, %d, %d) not implemented", _id, msg.cmd, msg.p1, msg.p2, msg.p3);
		break;
	case 0x11:
		// FUN_00481c60
		setView(msg.p1, msg.p2);
		break;
	case 0x12:
		// FUN_00481c80 returns nothing.
		scrollTo(msg.p1, msg.p2, msg.p3 == 0);
		msg.result = 0;
		break;
	case 0x13:
		// FUN_00481d00
		msg.result = scrollBy(msg.p1, msg.p2, msg.p3 == 0);
		break;
	case 0x17:
		// FUN_004780a0
		msg.result = (_y << 16) | (_x & 0xffff);
		break;
	case 0x18:
		// FUN_00478090 [unverified]: the image's +0x20.
		msg.result = _z;
		break;
	case 0x19: {
		// FUN_004780d0: the image size.
		const Graphics::Surface *surf = getSurface();
		msg.result = surf ? ((surf->h << 16) | (surf->w & 0xffff)) : 0;
		break;
	}
	case kCmdUsePalette:
		if (!_loaded)
			load();
		// FUN_00481b10: the result is the previous palette's owner.
		msg.result = getPalette() ? g_engine->setGamePalette(getPalette(), ((uint32)_id << 16) | (_owner ? _owner->getId() : 0)) : 0;
		break;
	case kCmdMove:
		// FUN_0047e180: attach to the mouse cursor (dragging), grabbed at (x, y).
		if (!_loaded)
			load();
		g_engine->attachToCursor(this, (int16)(msg.p1 & 0xffff), (int16)(msg.p1 >> 16), msg.p2);
		break;
	case 0x15:
		g_engine->detachFromCursor(this);
		break;
	case 0x16:
		msg.result = ((((int16)(msg.p1 >> 16)) - _y) << 16) | ((((int16)(msg.p1 & 0xffff)) - _x) & 0xffff);
		break;
	case 0x20:
		msg.result = _busy;
		break;
	case 0x23:
		msg.result = (_y << 16) | (_x & 0xffff);
		break;
	default:
		Child::handleCommand(msg);
		break;
	}
}

void Sprite::play(int32 p1, int32 p2, int32 p3) {
	// FUN_00477f80 (bitmaps)
	if (!_loaded)
		load();
	_visible = true;
}

void Sprite::stop() {
	// FUN_00481950
	if (!_visible)
		return;
	g_engine->detachFromCursor(this);
	_visible = false;
}

void Sprite::enable(int32 on) {
	// FUN_004819b0
	_shown = (on != 0);
}

void Sprite::setPos(int32 x, int32 y, int32 z) {
	// FUN_004819e0
	if (!_loaded)
		load();
	_x = x;
	_y = y;
	if (z > 0)
		_z = z;
}

void Sprite::setZ(int32 z) {
	// FUN_00481a40 -> frame player FUN_00489570: z only counts if > 0.
	if (!_loaded)
		load();
	if (z > 0)
		_z = z;
}

Common::Rect Sprite::getBounds() const {
	const Graphics::Surface *s = getSurface();
	if (!s)
		return Common::Rect();
	int w = _viewW ? MIN<int>(_viewW, s->w - _srcX) : s->w - _srcX;
	int h = _viewH ? MIN<int>(_viewH, s->h - _srcY) : s->h - _srcY;
	return Common::Rect(left(), top(), left() + MAX(w, 0), top() + MAX(h, 0));
}

void Sprite::setView(int32 w, int32 h) {
	_viewW = MAX(w, 0);
	_viewH = MAX(h, 0);
}

int32 Sprite::scrollTo(int32 x, int32 y, bool clamp) {
	return scrollBy(x - _srcX, y - _srcY, clamp);
}

int32 Sprite::scrollBy(int32 dx, int32 dy, bool clamp) {
	const Graphics::Surface *s = getSurface();
	if (clamp && s) {
		int maxX = MAX<int>(0, s->w - (_viewW ? _viewW : s->w));
		int maxY = MAX<int>(0, s->h - (_viewH ? _viewH : s->h));
		dx = CLIP<int32>(_srcX + dx, 0, maxX) - _srcX;
		dy = CLIP<int32>(_srcY + dy, 0, maxY) - _srcY;
	}
	_srcX += dx;
	_srcY += dy;
	return (dy << 16) | (dx & 0xffff);
}

bool Sprite::hitTest(int x, int y) const {
	// FUN_00481e40 (bounding box) and FUN_00481e70 (mask)
	if (!_visible || !_shown)
		return false;
	const Graphics::Surface *s = getSurface();
	if (!s || !getBounds().contains(x, y))
		return false;
	// Command 0x2c makes the whole bounding box count (+0x54).
	if (_transparent < 0 || _inputDisabled)
		return true;
	return *(const byte *)s->getBasePtr(x - left() + _srcX, y - top() + _srcY) != _transparent;
}

void Sprite::draw(Graphics::Surface &dst) const {
	if (!_visible || !_shown)
		return;
	const Graphics::Surface *s = getSurface();
	if (!s)
		return;

	Common::Rect r = getBounds();
	r.clip(Common::Rect(dst.w, dst.h));
	if (r.isEmpty())
		return;

	for (int y = r.top; y < r.bottom; y++) {
		const byte *src = (const byte *)s->getBasePtr(r.left - left() + _srcX, y - top() + _srcY);
		byte *d = (byte *)dst.getBasePtr(r.left, y);
		if (_transparent < 0) {
			memcpy(d, src, r.width());
		} else {
			for (int x = 0; x < r.width(); x++)
				if (src[x] != _transparent)
					d[x] = src[x];
		}
	}
}

// --- Bitmap (FUN_00477b20) ---

Bitmap::Bitmap(uint16 id, int32 transparent, uint32 flags, int32 mode) :
	Sprite(id, kTypeBitmap, transparent, flags, 0) {
	_mode = mode;
	memset(_palette, 0, sizeof(_palette));
}

Bitmap::~Bitmap() {
	unload();
}

void Bitmap::handleCommand(Message &msg) {
	// FUN_00477bd0
	Sprite::handleCommand(msg);
}

bool Bitmap::decode(Common::SeekableReadStream &stream) {
	Image::BitmapDecoder decoder;
	if (!decoder.loadStream(stream))
		return false;
	if (_surface) {
		_surface->free();
		delete _surface;
	}
	_surface = new Graphics::Surface();
	_surface->copyFrom(*decoder.getSurface());
	const Graphics::Palette &pal = decoder.getPalette();
	memset(_palette, 0, sizeof(_palette));
	memcpy(_palette, pal.data(), MIN<uint>(pal.size(), 256) * 3);
	_revision++;
	return true;
}

void Bitmap::loadFromMemory(Common::SeekableReadStream &stream) {
	if (_loaded)
		return;
	if (decode(stream))
		_loaded = true;
	else
		warning("Bitmap %d: can't decode the image in memory", _id);
}

void Bitmap::load() {
	// FUN_00477c00
	if (_loaded)
		return;
	uint16 res = resourceId();
	Common::SeekableReadStream *stream = g_engine->_resources.get(res);
	if (!stream) {
		warning("Bitmap %d: no resource", res);
		return;
	}
	if (decode(*stream))
		_loaded = true;
	else
		warning("Bitmap %d: can't decode", res);
	delete stream;
}

void Bitmap::unload() {
	// FUN_00477f30
	if (_visible)
		stop();
	if (_surface) {
		_surface->free();
		delete _surface;
		_surface = nullptr;
	}
	_loaded = false;
}

// --- Anim (FUN_00478140) ---

Anim::Anim(uint16 id, int32 transparent, uint32 flags, uint16 param) :
	Sprite(id, kTypeAnim, transparent, flags, param) {
	memset(_palette, 0, sizeof(_palette));
}

Anim::~Anim() {
	unload();
}

void Anim::handleCommand(Message &msg) {
	// FUN_004781f0
	switch (msg.cmd) {
	case kCmdPause:
		pause();
		break;
	case kCmdResume:
		if (msg.p1 != 0)
			resume((int16)(msg.p1 & 0xffff), ((uint32)msg.p1 >> 16) & 0x7fff);
		else
			resume(kNoFrame, kNoFrame);
		break;
	case kCmdGetFrame:
		msg.result = _loaded ? _frame : 0;
		break;
	case kCmdSetClip:
		// FUN_00481b50: p2 is a message the game allocated for us.
		if (msg.trigger) {
			setFrameTrigger(msg.p1, msg.trigger);
		} else {
			setFrameTrigger(msg.p1, nullptr);
		}
		break;
	case kCmdGotoFrame:
		if (_loaded && _visible)
			gotoFrame(msg.p1);
		else
			_pendingFrame = msg.p1;
		break;
	default:
		Sprite::handleCommand(msg);
		break;
	}
}

Video::VideoDecoder *Anim::openDecoder() {
	uint16 res = resourceId();
	Common::SeekableReadStream *stream = g_engine->_resources.get(res, true);
	if (!stream) {
		warning("Anim %d: no resource", res);
		return nullptr;
	}

	// The FLC header: size, magic, frames, width, height, depth, flags, speed.
	stream->seek(4);
	uint16 magic = stream->readUint16LE();
	stream->seek(16);
	uint32 speed = stream->readUint32LE();
	if (magic == 0xaf11)
		speed = speed * 100 / 7;
	_period = speed ? speed : 50;
	stream->seek(0);

	Video::FlicDecoder *dec = new Video::FlicDecoder();
	if (!dec->loadStream(stream)) {
		warning("Anim %d: can't decode", res);
		delete dec;
		return nullptr;
	}
	_flic = dec;
	return dec;
}

const Graphics::Surface *Anim::nextFrame() {
	const Graphics::Surface *s = _decoder->decodeNextFrame();
	// The FLC decoder collects the changed areas of every frame until they
	// are cleared; nothing here uses them.
	if (_flic)
		_flic->clearDirtyRects();
	return s;
}

void Anim::load() {
	// FUN_004782e0
	if (_loaded)
		return;
	loadFrames();
	// A new frame player (FUN_004841b0): end 0xffff, loop count 0.
	_endSpec = 0xffff;
	_hideAtEnd = false;
	_loops = 0;
}

void Anim::loadFrames() {
	_decoder = openDecoder();
	if (!_decoder)
		return;

	_frameCount = _decoder->getFrameCount();
	_width = _decoder->getWidth();
	_height = _decoder->getHeight();

	// Small animations are decoded once, so that frames can be shown in any
	// order. Large ones (full-screen cut-scenes) are decoded as they play to
	// save memory on small devices.
	if ((uint32)_width * _height * _frameCount <= kCacheLimit) {
		for (int i = 0; i < _frameCount; i++) {
			const Graphics::Surface *s = nextFrame();
			if (_decoder->hasDirtyPalette() || i == 0)
				memcpy(_palette, _decoder->getPalette(), sizeof(_palette));
			Graphics::Surface *copy = new Graphics::Surface();
			convertFrame(*s, *copy);
			_frames.push_back(copy);
		}
		delete _decoder;
		_decoder = nullptr;
		_flic = nullptr;
	} else {
		_current = new Graphics::Surface();
		_current->create(_width, _height, Graphics::PixelFormat::createFormatCLUT8());
		decodeTo(0);
	}

	_frame = 0;
	_loaded = true;
}

void Anim::unload() {
	// FUN_0047b3b0
	if (_visible)
		stop();
	releaseFrames();
	_loaded = false;
}

void Anim::releaseFrames() {
	for (uint i = 0; i < _frames.size(); i++) {
		_frames[i]->free();
		delete _frames[i];
	}
	_frames.clear();
	if (_current) {
		_current->free();
		delete _current;
		_current = nullptr;
	}
	delete _decoder;
	_decoder = nullptr;
	_flic = nullptr;
	_decodedFrame = -1;
}

void Anim::decodeTo(int frame) {
	if (!_decoder || frame == _decodedFrame)
		return;
	if (frame < _decodedFrame || _decodedFrame < 0) {
		_decoder->rewind();
		_decodedFrame = -1;
	}
	const Graphics::Surface *s = nullptr;
	while (_decodedFrame < frame) {
		s = nextFrame();
		if (!s)
			break;
		_decodedFrame++;
		if (_decoder->hasDirtyPalette())
			memcpy(_palette, _decoder->getPalette(), sizeof(_palette));
	}
	if (s)
		convertFrame(*s, *_current);
}

void Anim::convertFrame(const Graphics::Surface &src, Graphics::Surface &dst) {
	dst.copyFrom(src);
}

const Graphics::Surface *Anim::getSurface() const {
	if (!_loaded)
		return nullptr;
	if (!_frames.empty())
		return _frames[CLIP<int>(_frame, 0, _frames.size() - 1)];
	return _current;
}

void Anim::gotoFrame(int frame) {
	// FUN_00484460
	if (_frameCount == 0)
		return;
	_frame = CLIP<int>(frame, 0, _frameCount - 1);
	if (_decoder)
		decodeTo(_frame);
}

void Anim::dumpFrames(const Common::String &prefix) {
	if (!_loaded)
		load();
	int saved = _frame;
	for (int i = 0; i < _frameCount; i++) {
		gotoFrame(i);
		const Graphics::Surface *f = (uint)i < _frames.size() ? _frames[i] : _current;
		Common::DumpFile out;
		if (f && out.open(Common::Path(Common::String::format("%s%d.png", prefix.c_str(), i))))
			Image::writePNG(out, *f, _palette);
	}
	gotoFrame(saved);
}

void Anim::setFrameTrigger(int frame, const Message *msg) {
	if (msg)
		_triggers[frame].push_back(*msg);
	else
		_triggers.erase(frame);
}

void Anim::fireTriggers() {
	if (!_triggers.contains(_frame))
		return;
	// Copy: a handler may change the triggers.
	Common::Array<Message> msgs = _triggers[_frame];
	for (uint i = 0; i < msgs.size(); i++) {
		Message m = msgs[i];
		if (m.cmd)
			g_engine->_world->deliverCommand(nullptr, m);
		else
			g_engine->_world->deliver(nullptr, m);
	}
}

void Anim::play(int32 p1, int32 p2, int32 p3) {
	// FUN_004817f0. See docs/ENGINE.md for the meaning of p1 and p2.
	if (!_loaded)
		load();
	if (!_loaded || _visible)
		return;

	if (_pendingFrame != 0) {
		gotoFrame(_pendingFrame);
		_pendingFrame = 0;
	}

	_visible = true;
	_paused = false;
	_notify = p1;
	_notifyParam = p3;
	if (p1 & 0x80000000)
		g_engine->groupAdd(p1);

	// FUN_004817f0: the frames part of p2 is only written when bit 31 is set;
	// otherwise the end and hide flags of the last play stay (+0xcc, +0xd4).
	int last = MAX(_frameCount - 1, 0);
	if (p2 & 0x80000000) {
		int start = ((uint32)p2 >> 16) & 0x3fff;
		if (start == 0x3fff)
			start = last;
		gotoFrame(start);
		// +0xd0: the first tick pauses the sprite on its start frame (a
		// resume plays on: closed doors, gauges).
		_holdPending = (p2 & 0x40000000) != 0;
		// +0xcc |end|, +0xd4 end < 0: stop the sprite at the end.
		int16 end = (int16)(p2 & 0xffff);
		_endSpec = ABS<int>(end);
		_hideAtEnd = end < 0;
	}
	_playing = true;
	debug(5, "Anim %d: play p1 %x p2 %x: frame %d, end %d, hold %d", _id, p1, p2, _frame, _endSpec, _holdPending);
	_nextFrameTime = g_engine->getMillis() + _period;
}

void Anim::stop() {
	if (!_visible)
		return;
	_playing = false;
	_paused = false;
	Sprite::stop();
}

void Anim::pause() {
	// FUN_0047b430
	if (_visible && !_paused)
		_paused = true;
}

void Anim::resume(int32 start, int32 end) {
	// FUN_0047b460
	debug(5, "Anim %d: resume %d %d at frame %d (visible %d paused %d)", _id, start, end, _frame, _visible, _paused);
	if (!_visible || !_paused)
		return;
	_paused = false;
	_holdPending = false;
	if (start != kNoFrame)
		gotoFrame(start);
	if (end != kNoFrame) {
		if (start == end) {
			_paused = true;
			gotoFrame(end);
			return;
		}
		_endSpec = ABS<int>(end);
		_hideAtEnd = end < 0;
	}
	_playing = true;
	_nextFrameTime = g_engine->getMillis() + _period;
}

bool Anim::loopsRemain() const {
	// FUN_00478060: the constructor's parameter is the loop count, 0 = forever.
	return _param == 0 || _loops < (int)_param;
}

void Anim::step() {
	// FUN_00489290: the next frame; past the last one, the loop count goes up
	// and the player starts over, or stays on the last frame and notifies.
	if (_frame + 1 < _frameCount) {
		gotoFrame(_frame + 1);
		return;
	}
	_loops++;
	if (!loopsRemain()) {
		gotoFrame(_frameCount - 1);
		notify();
		return;
	}
	gotoFrame(0);
}

void Anim::update(uint32 now) {
	// FUN_00484290, once per frame period.
	if (!_visible || !_playing || _paused || _frameCount == 0)
		return;
	if (now < _nextFrameTime)
		return;

	// Don't try to catch up after long stalls.
	_nextFrameTime = MAX(_nextFrameTime + _period, now);

	if (_holdPending) {
		_holdPending = false;
		_paused = true;
		fireTriggers();
		return;
	}
	// The frame's triggers, then the step.
	fireTriggers();
	int last = _frameCount - 1;
	if (_frame == _endSpec - 1 || (_endSpec == 0 && _frame == last)) {
		// The end: one more step, then pause (or stop) and notify.
		step();
		if (!_hideAtEnd)
			_paused = true;
		notify();
		if (_hideAtEnd)
			stop();
		return;
	}
	if (_frame != last) {
		step();
		return;
	}
	step();
	if (!loopsRemain() && !_hideAtEnd)
		_paused = true;
	if (_hideAtEnd)
		stop(); // message 0x8002 (FUN_0047fc70): child command 4
}

void Anim::notify() {
	// The notification set by play stays for later ends, until a play
	// without one or the unload (FUN_00484490).
	debug(5, "Anim %d: notify at frame %d", _id, _frame);
	notifyDone(_notify, kMsgAnimDone, _notifyParam);
}

// --- Hotspot (FUN_004786a0) ---

Hotspot::Hotspot(uint16 id, uint32 flags) : Sprite(id, kTypeHotspot, -1, flags, 0) {
}

void Hotspot::load() {
	// FUN_00478ba0 / FUN_004785c0
	if (_loaded)
		return;
	uint16 res = resourceId();
	Common::SeekableReadStream *s = g_engine->_resources.get(res);
	if (!s) {
		warning("Hotspot %d: no resource", res);
		return;
	}
	if (s->size() < 0x24 || s->readUint32BE() != MKTAG('.', 'H', 'S', 'P')) {
		warning("Hotspot %d: not a .HSP", res);
		delete s;
		return;
	}
	uint32 count = s->readUint32LE();
	for (uint32 i = 0; i < count && !s->eos(); i++) {
		HotspotRegion r;
		r.id = s->readUint32LE();
		r.type = s->readUint32LE();
		uint32 n = s->readUint32LE();
		for (uint32 j = 0; j < n; j++) {
			int16 x = s->readSint32LE();
			int16 y = s->readSint32LE();
			r.points.push_back(Common::Point(x, y));
		}
		if (r.points.empty())
			continue;
		if (r.type <= 1) {
			// Rectangles and ellipses are given by two corners.
			if (r.points.size() < 2)
				continue;
			r.bounds = Common::Rect(MIN(r.points[0].x, r.points[1].x), MIN(r.points[0].y, r.points[1].y),
			                        MAX(r.points[0].x, r.points[1].x), MAX(r.points[0].y, r.points[1].y));
		} else {
			r.bounds = Common::Rect(r.points[0].x, r.points[0].y, r.points[0].x, r.points[0].y);
			for (uint j = 1; j < r.points.size(); j++)
				r.bounds.extend(Common::Rect(r.points[j].x, r.points[j].y, r.points[j].x, r.points[j].y));
		}
		_regions.push_back(r);
	}
	delete s;
	_loaded = true;
}

void Hotspot::handleCommand(Message &msg) {
	// FUN_004787f0
	switch (msg.cmd) {
	case 0x16:
		msg.result = msg.p1;
		break;
	case kCmdHitTest:
		msg.result = regionCenter(msg.p1, msg.p2);
		break;
	case kCmdSetPos:
	case kCmdSetOffset:
	case 0x14:
	case 0x15:
		// No-ops for hotspots (FUN_004789a0, FUN_0041b900): regions are absolute.
		break;
	case kCmdHotspotHit:
		// FUN_00478b40: the enable flag and the regions, played or not.
		if (_shown && anyRegionContains(msg.p1, msg.p2))
			msg.result = (regionAt(msg.p1, msg.p2) << 16) | 1;
		else
			msg.result = 0;
		break;
	default:
		Sprite::handleCommand(msg);
		break;
	}
}

void Hotspot::play(int32 p1, int32 p2, int32 p3) {
	// FUN_00478880
	if (!_loaded)
		load();
	_visible = true;
}

static bool pointInPolygon(const Common::Array<Common::Point> &pts, int x, int y) {
	// Alternate fill mode, like CreatePolygonRgn(..., ALTERNATE).
	bool inside = false;
	for (uint i = 0, j = pts.size() - 1; i < pts.size(); j = i++) {
		if (((pts[i].y > y) != (pts[j].y > y)) &&
		    (x < (pts[j].x - pts[i].x) * (y - pts[i].y) / (double)(pts[j].y - pts[i].y) + pts[i].x))
			inside = !inside;
	}
	return inside;
}

bool HotspotRegion::contains(int x, int y) const {
	if (!bounds.contains(x, y))
		return false;
	switch (type) {
	case 0:
		return true;
	case 1: {
		// Ellipse inscribed in the bounds.
		double rx = bounds.width() / 2.0, ry = bounds.height() / 2.0;
		if (rx <= 0 || ry <= 0)
			return false;
		double dx = (x + 0.5 - bounds.left - rx) / rx, dy = (y + 0.5 - bounds.top - ry) / ry;
		return dx * dx + dy * dy <= 1.0;
	}
	default:
		return pointInPolygon(points, x, y);
	}
}

uint32 Hotspot::regionAt(int x, int y) const {
	// FUN_00478790
	for (uint i = 0; i < _regions.size(); i++)
		if (_regions[i].contains(x, y))
			return _regions[i].id;
	return 0;
}

int32 Hotspot::regionCenter(int32 all, int32 id) const {
	// FUN_004789b0
	Common::Rect u;
	bool any = false;
	for (uint i = 0; i < _regions.size(); i++) {
		if (all != 0 && _regions[i].id != (uint32)id)
			continue;
		if (_regions[i].bounds.isEmpty())
			continue;
		if (!any)
			u = _regions[i].bounds;
		else
			u.extend(_regions[i].bounds);
		any = true;
	}
	if (!any)
		return 0;
	int cx = (u.left + u.right) / 2, cy = (u.top + u.bottom) / 2;
	return (cy << 16) | (cx & 0xffff);
}

bool Hotspot::hitTest(int x, int y) const {
	// FUN_00478ae0 + FUN_00478b40
	if (!_visible || !_shown)
		return false;
	return anyRegionContains(x, y);
}

Common::Array<Common::Point> Hotspot::regionPoints() const {
	Common::Array<Common::Point> pts;
	for (uint i = 0; i < _regions.size(); i++) {
		const Common::Rect &b = _regions[i].bounds;
		// The centre, or the first vertex area for odd shapes.
		Common::Point c((b.left + b.right) / 2, (b.top + b.bottom) / 2);
		if (!_regions[i].contains(c.x, c.y) && !_regions[i].points.empty())
			c = _regions[i].points[0];
		pts.push_back(c);
	}
	return pts;
}

bool Hotspot::anyRegionContains(int x, int y) const {
	for (uint i = 0; i < _regions.size(); i++)
		if (_regions[i].contains(x, y))
			return true;
	return false;
}

// --- Sound (FUN_00478e40) ---

Sound::Sound(uint16 id, uint16 param) : Child(id, kTypeSound, 0), _param(param) {
}

Sound::~Sound() {
	stop();
}

void Sound::handleCommand(Message &msg) {
	// FUN_00478ef0
	switch (msg.cmd) {
	case kCmdLoad:
		_loaded = true;
		break;
	case kCmdUnload:
		stop();
		_loaded = false;
		break;
	case kCmdPlay:
		play(msg.p1, msg.p2, msg.p3);
		break;
	case kCmdStop:
		stop();
		break;
	default:
		Child::handleCommand(msg);
		break;
	}
}

void Sound::play(int32 p1, int32 p2, int32 p3) {
	// FUN_004792e0: does nothing if the sound is already playing.
	if (isPlaying())
		return;
	uint16 res = resourceId();
	Common::SeekableReadStream *s = g_engine->_resources.get(res);
	if (!s) {
		warning("Sound %d: no resource", res);
		return;
	}
	Audio::SeekableAudioStream *audio = Audio::makeWAVStream(s, DisposeAfterUse::YES);
	if (!audio) {
		// Happens in the original too: some sound children name text resources.
		debug(1, "Sound %d: can't decode", res);
		return;
	}
	_loaded = true;
	_visible = true;
	_notify = p1;
	_notifyParam = p3;
	if (p1 & 0x80000000)
		g_engine->groupAdd(p1);

	// The parameter given at construction is the loop count (FUN_00486c97,
	// +0x98): 1 plays once, 0 loops forever.
	if (g_engine->hasVirtualClock()) {
		uint32 len = audio->getLength().msecs();
		_endTime = _param == 0 ? 0xffffffff : g_engine->getMillis() + MAX<uint32>(len, 1) * _param;
		delete audio;
		_playing = true;
		return;
	}
	Audio::AudioStream *stream = audio;
	if (_param != 1)
		stream = Audio::makeLoopingAudioStream(audio, _param);
	g_engine->_mixer->playStream(Audio::Mixer::kSFXSoundType, &_handle, stream, -1,
	                             g_engine->getGameVolume(), 0, DisposeAfterUse::YES);
	_playing = true;
}

void Sound::stop() {
	// FUN_004793a0: stopping doesn't notify.
	if (_playing && !g_engine->hasVirtualClock())
		g_engine->_mixer->stopHandle(_handle);
	_playing = false;
	_visible = false;
	_notify = 0;
}

void Sound::setVolume(byte volume) {
	if (_playing && !g_engine->hasVirtualClock())
		g_engine->_mixer->setChannelVolume(_handle, volume);
}

bool Sound::isActive() const {
	if (g_engine->hasVirtualClock())
		return g_engine->getMillis() < _endTime;
	return g_engine->_mixer->isSoundHandleActive(_handle);
}

bool Sound::isPlaying() const {
	return _playing && isActive();
}

void Sound::update(uint32 now) {
	if (_playing && !isActive()) {
		_playing = false;
		_visible = false;
		int32 notify = _notify;
		_notify = 0;
		notifyDone(notify, kMsgSoundDone, _notifyParam);
	}
}

// --- TextResource (FUN_00479400) ---

void TextResource::handleCommand(Message &msg) {
	// FUN_00479500
	switch (msg.cmd) {
	case kCmdLoad:
		load();
		break;
	case kCmdUnload:
		_text.clear();
		_loaded = false;
		break;
	case kCmdPlay:
		play(msg.p1, 0, 0);
		break;
	case kCmdStop:
		stop();
		break;
	default:
		Child::handleCommand(msg);
		break;
	}
}

void TextResource::load() {
	if (_loaded)
		return;
	_text = g_engine->_resources.getText(resourceId());
	_loaded = true;
}

/** A child named by (child << 16) | object, as text sources are. */
static Child *childByKey(uint32 key) {
	GameObject *obj = g_engine->_world->get(key & 0xffff);
	return obj ? obj->getChild(key >> 16) : nullptr;
}

void TextResource::play(int32 p1, int32 p2, int32 p3) {
	// FUN_004796c0
	load();
	if (p1 == 0)
		return;
	if (_target)
		stop();
	TextSprite *t = dynamic_cast<TextSprite *>(childByKey(p1));
	if (!t) {
		warning("TextResource %d: no text sprite %08x", _id, p1);
		return;
	}
	t->load();
	t->setSource(((uint32)_id << 16) | _owner->getId());
	_target = p1;
	_visible = true;
}

void TextResource::stop() {
	// FUN_00479740
	if (_target) {
		TextSprite *t = dynamic_cast<TextSprite *>(childByKey(_target));
		if (t)
			t->setSource(0);
	}
	_target = 0;
	_visible = false;
}

// --- TextSprite (FUN_004797f0) ---

TextSprite::TextSprite(uint16 id, int32 fg, int32 bg, const Common::String &font, int32 size, int32 wrap,
                       int32 mode, int32 style, uint32 source, uint32 flags) :
	Sprite(id, kTypeText, -1, flags, 1), _font(font), _fg(fg), _bg(bg), _size(size), _wrap(wrap),
	_mode(mode), _style(style), _source(source) {
	if (_bg < 0)
		_transparent = 0xfa;
}

TextSprite::~TextSprite() {
	if (_surface) {
		_surface->free();
		delete _surface;
	}
}

void TextSprite::handleCommand(Message &msg) {
	// FUN_0047a660
	switch (msg.cmd) {
	case 0xd: {
		// Font: name, size (tenths of a point), style; 0 / -1 keep the current one.
		if (msg.text)
			_font = msg.text;
		if (msg.p2)
			_size = msg.p2;
		if (msg.p3 != -1)
			_style = msg.p3;
		render();
		break;
	}
	case 0xe:
		setSource(((uint32)(msg.p2 & 0xffff) << 16) | (msg.p1 & 0xffff));
		break;
	case 0xf:
		if (msg.text)
			setText(msg.text);
		break;
	case 0x10:
		// FUN_0047a4b0: colours, palette entries 10-245 (background < 0: none).
		if (msg.p1 > 9 && msg.p1 < 0xf6)
			_fg = msg.p1;
		if (msg.p2 < 0 || (msg.p2 > 9 && msg.p2 < 0xf6))
			_bg = msg.p2;
		_transparent = _bg < 0 ? 0xfa : -1;
		render();
		break;
	case 0x11:
		setBox(msg.p1, msg.p2);
		break;
	default:
		if (msg.cmd >= 0xd && msg.cmd <= 0x34 && msg.cmd != 0x16 && msg.cmd != 0x1a && msg.cmd != 0x1b &&
		    msg.cmd < 0x24)
			debug(1, "TextSprite %d: command %x (%d, %d, %d) not implemented", _id, msg.cmd, msg.p1, msg.p2, msg.p3);
		Sprite::handleCommand(msg);
		break;
	}
}

void TextSprite::load() {
	if (!_loaded) {
		_loaded = true;
		render();
	}
}

void TextSprite::play(int32 p1, int32 p2, int32 p3) {
	load();
	_visible = true;
}

void TextSprite::setSource(uint32 source) {
	_source = source;
	render();
}

void TextSprite::setText(const Common::String &text) {
	_literal = text;
	_source = 0xffffffff;
	render();
}

void TextSprite::setBox(int32 w, int32 h) {
	// FUN_0047a920: the box is also the shown part of the text.
	_boxW = MAX(w, 0);
	_boxH = MAX(h, 0);
	setView(_boxW, _boxH);
	render();
}

Common::String TextSprite::currentText() const {
	if (_source == 0xffffffff)
		return _literal;
	if (_source == 0)
		return Common::String();
	TextResource *r = dynamic_cast<TextResource *>(childByKey(_source));
	if (!r)
		return Common::String();
	r->load();
	return r->getText();
}

void TextSprite::refresh() {
	if (currentText() != _shown)
		render();
}

void TextSprite::render() {
	// FUN_00479cb0
	_shown = currentText();
	_revision++;
	if (_surface) {
		_surface->free();
		delete _surface;
		_surface = nullptr;
	}
	if (_shown.empty() && _boxW == 0)
		return;

	const Graphics::Font *font = g_engine->getFont(_font, _size, _style);
	Common::Array<Common::String> lines;
	if (_wrap && _boxW > 0) {
		font->wordWrapText(_shown, _boxW, lines);
	} else {
		// Lines end at CR/LF only.
		Common::String line;
		for (uint i = 0; i < _shown.size(); i++) {
			if (_shown[i] == '\r')
				continue;
			if (_shown[i] == '\n') {
				lines.push_back(line);
				line.clear();
			} else {
				line += _shown[i];
			}
		}
		lines.push_back(line);
	}
	int lh = font->getFontHeight();
	int w = _boxW;
	if (w == 0)
		for (uint i = 0; i < lines.size(); i++)
			w = MAX(w, font->getStringWidth(lines[i]));
	int textH = lh * lines.size();
	// The whole text; the box shows part of it (scrolled with command 0x13).
	int h = MAX(textH, _boxH);
	_srcX = 0;
	_srcY = 0;

	_surface = new Graphics::Surface();
	_surface->create(MAX(w, 1), MAX(h, 1), Graphics::PixelFormat::createFormatCLUT8());
	_surface->fillRect(Common::Rect(_surface->w, _surface->h), _bg < 0 ? 0xfa : _bg);
	// [unverified] The mode (+0xc8) aligns the lines: 0 left, 1 centred,
	// 2 right; text is placed at the top of the box.
	Graphics::TextAlign align = _mode == 1 ? Graphics::kTextAlignCenter :
	                            _mode == 2 ? Graphics::kTextAlignRight : Graphics::kTextAlignLeft;
	for (uint i = 0; i < lines.size(); i++)
		font->drawString(_surface, lines[i], 0, i * lh, _surface->w, _fg, align);
}

// --- VideoSprite (FUN_0047af30) ---

void VideoSprite::convertFrame(const Graphics::Surface &src, Graphics::Surface &dst) {
	if (src.format.bytesPerPixel == 1) {
		dst.copyFrom(src);
		return;
	}
	if (dst.w != src.w || dst.h != src.h || dst.format.bytesPerPixel != 1) {
		dst.free();
		dst.create(src.w, src.h, Graphics::PixelFormat::createFormatCLUT8());
	}
	if (_lutRevision != g_engine->getPaletteRevision() || _lut.empty()) {
		// Filled on demand: RGB565 -> nearest of the game's colours (10-245).
		_lut.resize(65536);
		for (uint i = 0; i < 65536; i++)
			_lut[i] = -1;
		_lutRevision = g_engine->getPaletteRevision();
	}
	const byte *pal = g_engine->getScreenPalette();
	const Graphics::PixelFormat &f = src.format;
	for (int y = 0; y < src.h; y++) {
		const byte *s = (const byte *)src.getBasePtr(0, y);
		byte *d = (byte *)dst.getBasePtr(0, y);
		for (int x = 0; x < src.w; x++, s += f.bytesPerPixel) {
			uint32 p = f.bytesPerPixel == 4 ? READ_UINT32(s) : f.bytesPerPixel == 2 ? READ_UINT16(s) : 0;
			uint8 r, g, b;
			f.colorToRGB(p, r, g, b);
			uint16 key = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
			int16 &e = _lut[key];
			if (e < 0) {
				int best = 10, bestDist = 0x7fffffff;
				for (int k = 10; k < 246; k++) {
					int dr = pal[k * 3] - r, dg = pal[k * 3 + 1] - g, db = pal[k * 3 + 2] - b;
					int dist = dr * dr * 3 + dg * dg * 4 + db * db * 2;
					if (dist < bestDist) {
						bestDist = dist;
						best = k;
					}
				}
				e = best;
			}
			d[x] = e;
		}
	}
}

VideoSprite::VideoSprite(uint16 id, int32 transparent, const Common::String &file, uint32 flags, int32 param) :
	Anim(id, transparent, flags, param), _file(file) {
	_type = kTypeVideo;
}

void VideoSprite::load() {
	// Rooms load many videos; they are only opened while they play.
	_loaded = true;
}

void VideoSprite::play(int32 p1, int32 p2, int32 p3) {
	if (!_decoder && _frames.empty())
		loadFrames();
	Anim::play(p1, p2, p3);
}

void VideoSprite::stop() {
	Anim::stop();
	releaseFrames();
}

Video::VideoDecoder *VideoSprite::openDecoder() {
	// FUN_0047b100: a Cinepak AVI, dithered to the screen palette like the
	// original's 8-bit ICDecompress output.
	Common::String path = g_engine->_files->gamePath(_file);
	Common::File *f = new Common::File();
	if (path.empty() || !f->open(Common::Path(path, '/'))) {
		warning("VideoSprite %d: can't open %s", _id, _file.c_str());
		delete f;
		return nullptr;
	}
	Video::AVIDecoder *dec = new Video::AVIDecoder();
	if (!dec->loadStream(f)) {
		warning("VideoSprite %d: can't decode %s", _id, _file.c_str());
		delete dec;
		return nullptr;
	}
	// The original dithers to the palette set when the video starts; here
	// frames are mapped to the palette of the moment (convertFrame), since
	// rooms set their palette after loading the videos.
	uint32 frames = dec->getFrameCount();
	_period = frames ? dec->getDuration().msecs() / frames : 66;
	if (_period == 0)
		_period = 66;
	return dec;
}

} // End of namespace Ivanoff
