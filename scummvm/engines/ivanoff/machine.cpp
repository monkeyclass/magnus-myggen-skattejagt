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

#include "common/debug.h"
#include "common/file.h"
#include "common/system.h"

#include "ivanoff/enginefuncs.h"
#include "ivanoff/ivanoff.h"
#include "ivanoff/machine.h"
#include "ivanoff/sprite.h"

namespace Ivanoff {

using Emu::CPU;
using Emu::EAX;
using Emu::ECX;
using Emu::EDX;
using Emu::ESP;

// Memory layout of the emulated process.
static const uint32 kImportThunks = 0x00020000;
static const uint32 kTib = 0x00030000;
static const uint32 kStackBottom = 0x00100000;
static const uint32 kStackTop = 0x00200000;

// Addresses in the 2002 mm2run.exe (see docs/ENGINE.md).
enum {
	kSetupObjects = 0x43f760,
	kVarContainerCtor = 0x47c0a0,
	kCStringRelease = 0x49fec5,
	kAfxGetModuleState = 0x4a881a,
	kMessageVtable = 0x4b5fa0
};

// Original class vtables of the children.
enum {
	kVtBitmap = 0x4b55a8,
	kVtAnim = 0x4b56f8,
	kVtHotspot = 0x4b5848,
	kVtSound = 0x4b5900,
	kVtTextRes = 0x4b5a34,
	kVtText = 0x4b5ab4,
	kVtVideo = 0x4b5d18
};

// Trapped engine functions.
enum EngineFunc {
	kObjectCtor,      // 476910
	kPost,            // 476a60
	kSend,            // 476ad0
	kCommand,         // 476b40
	kAddChild,        // 4770a0
	kGetChild,        // 4770c0
	kGetChildOf,      // 4770d0
	kSetGlobalCursor, // 477120
	kStartTimer,      // 477140
	kStopTimer,       // 477170
	kChildSetCursor,  // 477a90
	kNewBitmap,       // 477b20
	kNewAnim,         // 478140
	kNewHotspot,      // 4786a0
	kNewSound,        // 478e40
	kNewTextRes,      // 479400
	kNewText,         // 4797f0
	kNewVideo,        // 47af30
	kMapLookup,       // 47bb10
	kRegister,        // 47bb40
	kMalloc,          // 492ecd _malloc
	kFree,            // 490cb4 free
	kNew,             // 49fc06 operator new
	kDelete,          // 49fc2f operator delete
	kRand,            // 491807 _rand
	kAtexit,          // 490c73 atexit
	kAppCtor,         // 47b5d0 application object (the engine itself)
	kSetKeyFocus,     // 477240
	kRedrawNow,       // 476a40
	kUnloadChildren   // 476a30 (and its thunk 4771b0)
};

static const struct {
	uint32 addr;
	uint16 cleanup;
	EngineFunc id;
} kEngineTraps[] = {
	{ 0x476910, 12, kObjectCtor },
	{ 0x476a60, 20, kPost },
	{ 0x476ad0, 20, kSend },
	{ 0x476b40, 24, kCommand },
	{ 0x4770a0, 4, kAddChild },
	{ 0x4770c0, 4, kGetChild },
	{ 0x4770d0, 8, kGetChildOf },
	{ 0x477120, 4, kSetGlobalCursor },
	{ 0x477140, 12, kStartTimer },
	{ 0x477170, 4, kStopTimer },
	{ 0x477a90, 4, kChildSetCursor },
	{ 0x477b20, 16, kNewBitmap },
	{ 0x478140, 16, kNewAnim },
	{ 0x4786a0, 8, kNewHotspot },
	{ 0x478e40, 8, kNewSound },
	{ 0x479400, 4, kNewTextRes },
	{ 0x4797f0, 40, kNewText },
	{ 0x47af30, 20, kNewVideo },
	{ 0x47bb10, 4, kMapLookup },
	{ 0x47bb40, 4, kRegister },
	{ 0x492ecd, 0, kMalloc },
	{ 0x490cb4, 0, kFree },
	{ 0x49fc06, 0, kNew },
	{ 0x49fc2f, 0, kDelete },
	{ 0x491807, 0, kRand },
	{ 0x490c73, 0, kAtexit },
	{ 0x47b5d0, 28, kAppCtor },
	{ 0x477240, 4, kSetKeyFocus },
	{ 0x476a40, 0, kRedrawNow },
	{ 0x476a30, 0, kUnloadChildren },
	{ 0x4771b0, 0, kUnloadChildren }
};

// Engine code the interpreter may run: variables, variable containers and
// messages are plain data structures without platform dependencies.
static const struct {
	uint32 start, end;
} kEmulatedEngineCode[] = {
	{ 0x476060, 0x4766d0 }, // string helpers, static initializers
	{ 0x4766d0, 0x4766f0 }, // add variable
	{ 0x476bc0, 0x477070 }, // heap messages, variable creation and lookup
	{ 0x477190, 0x4771a0 },
	{ 0x477260, 0x477560 }, // variable base class, trivial methods
	{ 0x477ae0, 0x477b20 }, // trivial methods
	{ 0x47c0a0, 0x47c4b0 }, // variable container, message
	{ 0x47c790, 0x47da30 }  // variable classes
};

// Win32 functions the game code may reach, with their argument counts
// (all stdcall except wsprintfA).
static const struct {
	const char *name;
	int nargs;
} kImportArgs[] = {
	{ "KERNEL32.DLL!InterlockedIncrement", 1 },
	{ "KERNEL32.DLL!InterlockedDecrement", 1 },
	{ "KERNEL32.DLL!InterlockedExchange", 2 },
	{ "KERNEL32.DLL!lstrlenA", 1 },
	{ "KERNEL32.DLL!lstrcpynA", 3 },
	{ "KERNEL32.DLL!lstrcmpA", 2 },
	{ "KERNEL32.DLL!lstrcmpiA", 2 },
	{ "KERNEL32.DLL!GetLastError", 0 },
	{ "KERNEL32.DLL!SetLastError", 1 },
	{ "KERNEL32.DLL!EnterCriticalSection", 1 },
	{ "KERNEL32.DLL!LeaveCriticalSection", 1 },
	{ "KERNEL32.DLL!GetCurrentThreadId", 0 },
	{ "KERNEL32.DLL!Sleep", 1 },
	{ "WINMM.DLL!timeGetTime", 0 },
	{ "KERNEL32.DLL!GetTickCount", 0 },
	{ "KERNEL32.DLL!InitializeCriticalSection", 1 },
	{ "KERNEL32.DLL!DeleteCriticalSection", 1 },
	{ "KERNEL32.DLL!TlsAlloc", 0 },
	{ "KERNEL32.DLL!TlsFree", 1 },
	{ "KERNEL32.DLL!TlsGetValue", 1 },
	{ "KERNEL32.DLL!TlsSetValue", 2 },
	{ "KERNEL32.DLL!LocalAlloc", 2 },
	{ "KERNEL32.DLL!LocalFree", 1 },
	{ "KERNEL32.DLL!LocalReAlloc", 3 },
	{ "KERNEL32.DLL!GlobalAlloc", 2 },
	{ "KERNEL32.DLL!GlobalFree", 1 },
	{ "KERNEL32.DLL!GlobalLock", 1 },
	{ "KERNEL32.DLL!GlobalUnlock", 1 },
	{ "KERNEL32.DLL!GlobalHandle", 1 },
	{ "KERNEL32.DLL!GlobalReAlloc", 3 },
	{ "KERNEL32.DLL!GlobalFlags", 1 },
	{ "KERNEL32.DLL!GetModuleHandleA", 1 },
	{ "KERNEL32.DLL!GetVersion", 0 },
	{ "KERNEL32.DLL!GetProcessVersion", 1 },
	{ "KERNEL32.DLL!GetCurrentThread", 0 },
	{ "KERNEL32.DLL!GetCurrentProcess", 0 },
	{ "KERNEL32.DLL!GetACP", 0 },
	{ "KERNEL32.DLL!GetOEMCP", 0 },
	{ "KERNEL32.DLL!GetCPInfo", 2 },
	{ "KERNEL32.DLL!MultiByteToWideChar", 6 },
	{ "KERNEL32.DLL!WideCharToMultiByte", 8 },
	{ "KERNEL32.DLL!GetStringTypeA", 5 },
	{ "KERNEL32.DLL!GetStringTypeW", 4 },
	{ "KERNEL32.DLL!LCMapStringA", 6 },
	{ "KERNEL32.DLL!LCMapStringW", 6 },
	{ "KERNEL32.DLL!HeapCreate", 3 },
	{ "KERNEL32.DLL!HeapAlloc", 3 },
	{ "KERNEL32.DLL!HeapFree", 3 },
	{ "KERNEL32.DLL!HeapReAlloc", 4 },
	{ "KERNEL32.DLL!VirtualAlloc", 4 },
	{ "KERNEL32.DLL!VirtualFree", 3 },
	{ "USER32.DLL!RegisterWindowMessageA", 1 },
	{ "USER32.DLL!GetSysColor", 1 },
	{ "USER32.DLL!GetSysColorBrush", 1 },
	{ "USER32.DLL!GetSystemMetrics", 1 },
	{ "USER32.DLL!GetDC", 1 },
	{ "USER32.DLL!ReleaseDC", 2 },
	{ "USER32.DLL!LoadCursorA", 2 },
	{ "USER32.DLL!LoadIconA", 2 },
	{ "GDI32.DLL!GetDeviceCaps", 2 },
	{ "GDI32.DLL!GetStockObject", 1 },
	{ "KERNEL32.DLL!GetVersionExA", 1 },
	{ "ADVAPI32.DLL!RegOpenKeyExA", 5 },
	{ "ADVAPI32.DLL!RegCreateKeyExA", 9 },
	{ "ADVAPI32.DLL!RegQueryValueExA", 6 },
	{ "ADVAPI32.DLL!RegSetValueExA", 6 },
	{ "ADVAPI32.DLL!RegCloseKey", 1 },
	{ "KERNEL32.DLL!LoadLibraryA", 1 },
	{ "KERNEL32.DLL!FreeLibrary", 1 },
	{ "KERNEL32.DLL!GetProcAddress", 2 },
	{ "KERNEL32.DLL!GetModuleFileNameA", 3 },
	{ "KERNEL32.DLL!CreateDirectoryA", 2 },
	{ "KERNEL32.DLL!CreateFileA", 7 },
	{ "KERNEL32.DLL!ReadFile", 5 },
	{ "KERNEL32.DLL!WriteFile", 5 },
	{ "KERNEL32.DLL!SetFilePointer", 4 },
	{ "KERNEL32.DLL!GetFileSize", 2 },
	{ "KERNEL32.DLL!CloseHandle", 1 },
	{ "KERNEL32.DLL!GetFileAttributesA", 1 },
	{ "KERNEL32.DLL!DeleteFileA", 1 },
	{ "KERNEL32.DLL!FlushFileBuffers", 1 },
	{ "KERNEL32.DLL!SetEndOfFile", 1 },
	{ "KERNEL32.DLL!GetFileType", 1 },
	{ "KERNEL32.DLL!GetPrivateProfileStringA", 6 },
	{ "KERNEL32.DLL!GetPrivateProfileIntA", 4 },
	{ "KERNEL32.DLL!WritePrivateProfileStringA", 4 },
	{ "KERNEL32.DLL!GetLocalTime", 1 },
	{ "KERNEL32.DLL!GetSystemTime", 1 },
	{ "KERNEL32.DLL!GetTimeZoneInformation", 1 },
	{ "USER32.DLL!wsprintfA", 0 },
	{ "USER32.DLL!CharUpperA", 1 }
};

/** A minimal wsprintfA: %d %i %u %x %X %c %s %ld %lu %lx, with flags, width and precision. */
static Common::String formatPrintf(Emu::Memory &mem, const Common::String &fmt, uint32 argp) {
	Common::String out;
	for (uint i = 0; i < fmt.size(); i++) {
		char c = fmt[i];
		if (c != '%') {
			out += c;
			continue;
		}
		Common::String spec("%");
		i++;
		while (i < fmt.size() && strchr("-+ #0123456789.", fmt[i]))
			spec += fmt[i++];
		while (i < fmt.size() && (fmt[i] == 'l' || fmt[i] == 'h'))
			i++;
		if (i >= fmt.size())
			break;
		char conv = fmt[i];
		switch (conv) {
		case '%':
			out += '%';
			break;
		case 's':
			out += Common::String::format((spec + "s").c_str(), mem.readString(mem.r32(argp)).c_str());
			argp += 4;
			break;
		case 'c':
			out += (char)mem.r32(argp);
			argp += 4;
			break;
		case 'd':
		case 'i':
			out += Common::String::format((spec + "d").c_str(), (int)mem.r32(argp));
			argp += 4;
			break;
		default:
			out += Common::String::format((spec + conv).c_str(), (unsigned)mem.r32(argp));
			argp += 4;
			break;
		}
	}
	return out;
}

void Machine::importCall(CPU &cpu, const Common::String &name) {
	uint32 result = 0;
	if (name == "KERNEL32.DLL!InterlockedIncrement") {
		result = _mem.r32(cpu.arg(0)) + 1;
		_mem.w32(cpu.arg(0), result);
	} else if (name == "KERNEL32.DLL!InterlockedDecrement") {
		result = _mem.r32(cpu.arg(0)) - 1;
		_mem.w32(cpu.arg(0), result);
	} else if (name == "KERNEL32.DLL!InterlockedExchange") {
		result = _mem.r32(cpu.arg(0));
		_mem.w32(cpu.arg(0), cpu.arg(1));
	} else if (name == "KERNEL32.DLL!lstrlenA") {
		result = cpu.arg(0) ? _mem.readString(cpu.arg(0)).size() : 0;
	} else if (name == "KERNEL32.DLL!lstrcpynA") {
		Common::String str = _mem.readString(cpu.arg(1));
		uint32 n = cpu.arg(2);
		if (n > 0) {
			if (str.size() > n - 1)
				str = Common::String(str.c_str(), n - 1);
			_mem.writeString(cpu.arg(0), str);
		}
		result = cpu.arg(0);
	} else if (name == "KERNEL32.DLL!lstrcmpA") {
		result = (uint32)_mem.readString(cpu.arg(0)).compareTo(_mem.readString(cpu.arg(1)));
	} else if (name == "KERNEL32.DLL!lstrcmpiA") {
		result = (uint32)_mem.readString(cpu.arg(0)).compareToIgnoreCase(_mem.readString(cpu.arg(1)));
	} else if (name == "KERNEL32.DLL!GetLastError") {
		result = _lastError;
	} else if (name == "KERNEL32.DLL!SetLastError") {
		_lastError = cpu.arg(0);
	} else if (name == "KERNEL32.DLL!EnterCriticalSection" || name == "KERNEL32.DLL!LeaveCriticalSection" ||
	           name == "KERNEL32.DLL!Sleep") {
		// Single-threaded.
	} else if (name == "KERNEL32.DLL!InitializeCriticalSection" || name == "KERNEL32.DLL!DeleteCriticalSection") {
		// Single-threaded.
	} else if (name == "KERNEL32.DLL!TlsAlloc") {
		result = _tls.size();
		_tls.push_back(0);
	} else if (name == "KERNEL32.DLL!TlsFree") {
		result = 1;
	} else if (name == "KERNEL32.DLL!TlsGetValue") {
		result = cpu.arg(0) < _tls.size() ? _tls[cpu.arg(0)] : 0;
		_lastError = 0;
	} else if (name == "KERNEL32.DLL!TlsSetValue") {
		if (cpu.arg(0) < _tls.size())
			_tls[cpu.arg(0)] = cpu.arg(1);
		result = 1;
	} else if (name == "KERNEL32.DLL!LocalAlloc" || name == "KERNEL32.DLL!GlobalAlloc") {
		// Handles are plain pointers (fixed memory); memory is always zeroed.
		result = _mem.alloc(cpu.arg(1));
	} else if (name == "KERNEL32.DLL!LocalFree" || name == "KERNEL32.DLL!GlobalFree") {
		_mem.free(cpu.arg(0));
	} else if (name == "KERNEL32.DLL!LocalReAlloc" || name == "KERNEL32.DLL!GlobalReAlloc") {
		result = _mem.realloc(cpu.arg(0), cpu.arg(1));
	} else if (name == "KERNEL32.DLL!GlobalLock" || name == "KERNEL32.DLL!GlobalHandle") {
		result = cpu.arg(0);
	} else if (name == "KERNEL32.DLL!GlobalUnlock" || name == "KERNEL32.DLL!GlobalFlags") {
		result = 0;
	} else if (name == "KERNEL32.DLL!GetModuleHandleA") {
		result = _imageBase;
	} else if (name == "KERNEL32.DLL!GetVersion" || name == "KERNEL32.DLL!GetProcessVersion") {
		// Windows 98 (4.10)
		result = name.hasSuffix("GetVersion") ? 0xc0000a04 : 0x00040000;
	} else if (name == "KERNEL32.DLL!GetCurrentThread") {
		result = 0xfffffffe;
	} else if (name == "KERNEL32.DLL!GetCurrentProcess") {
		result = 0xffffffff;
	} else if (name == "KERNEL32.DLL!GetACP") {
		result = 1252;
	} else if (name == "KERNEL32.DLL!GetOEMCP") {
		result = 850;
	} else if (name == "KERNEL32.DLL!GetCPInfo") {
		// A single-byte code page.
		uint32 info = cpu.arg(1);
		_mem.fill(info, 0, 20);
		_mem.w32(info, 1);
		_mem.w8(info + 4, '?');
		result = 1;
	} else if (name == "KERNEL32.DLL!MultiByteToWideChar") {
		// cp1252 bytes become the same code units (enough for this game's texts).
		uint32 src = cpu.arg(2), dst = cpu.arg(4);
		int32 n = (int32)cpu.arg(3), cap = (int32)cpu.arg(5);
		if (n < 0)
			n = _mem.readString(src).size() + 1;
		if (cap == 0) {
			result = n;
		} else {
			int32 m = MIN(n, cap);
			for (int32 i = 0; i < m; i++)
				_mem.w16(dst + 2 * i, _mem.r8(src + i));
			result = m;
		}
	} else if (name == "KERNEL32.DLL!WideCharToMultiByte") {
		uint32 src = cpu.arg(2), dst = cpu.arg(4);
		int32 n = (int32)cpu.arg(3), cap = (int32)cpu.arg(5);
		if (n < 0) {
			n = 0;
			while (_mem.r16(src + 2 * n))
				n++;
			n++;
		}
		if (cap == 0) {
			result = n;
		} else {
			int32 m = MIN(n, cap);
			for (int32 i = 0; i < m; i++) {
				uint16 ch = _mem.r16(src + 2 * i);
				_mem.w8(dst + i, ch < 0x100 ? ch : '?');
			}
			result = m;
		}
	} else if (name == "KERNEL32.DLL!GetStringTypeA" || name == "KERNEL32.DLL!GetStringTypeW") {
		// CT_CTYPE1 for Latin-1.
		bool wide = name.hasSuffix("W");
		uint32 src = cpu.arg(wide ? 1 : 2), out = cpu.arg(wide ? 3 : 4);
		int32 n = (int32)cpu.arg(wide ? 2 : 3);
		for (int32 i = 0; i < n; i++) {
			int c = wide ? _mem.r16(src + 2 * i) : _mem.r8(src + i);
			uint16 t = 0;
			if (c >= 'A' && c <= 'Z')
				t |= 0x101;
			else if (c >= 'a' && c <= 'z')
				t |= 0x102;
			else if (c >= '0' && c <= '9')
				t |= 0x84;
			else if (c == ' ' || (c >= 9 && c <= 13))
				t |= 0x48;
			else if (c < 32)
				t |= 0x20;
			else if (c < 128)
				t |= 0x10;
			else if (c >= 0xc0)
				t |= (c >= 0xe0 && c != 0xf7) ? 0x102 : (c != 0xd7 ? 0x101 : 0x10);
			_mem.w16(out + 2 * i, t);
		}
		result = 1;
	} else if (name == "KERNEL32.DLL!LCMapStringA" || name == "KERNEL32.DLL!LCMapStringW") {
		bool wide = name.hasSuffix("W");
		uint32 flags = cpu.arg(1), src = cpu.arg(2), dst = cpu.arg(4);
		int32 n = (int32)cpu.arg(3), cap = (int32)cpu.arg(5);
		int csize = wide ? 2 : 1;
		if (n < 0) {
			n = 0;
			while (wide ? _mem.r16(src + 2 * n) : _mem.r8(src + n))
				n++;
			n++;
		}
		if (cap == 0) {
			result = n;
		} else {
			int32 m = MIN(n, cap);
			for (int32 i = 0; i < m; i++) {
				uint32 c = wide ? _mem.r16(src + 2 * i) : _mem.r8(src + i);
				if ((flags & 0x100) && ((c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7)))
					c += 32;
				else if ((flags & 0x200) && ((c >= 'a' && c <= 'z') || (c >= 0xe0 && c <= 0xfe && c != 0xf7)))
					c -= 32;
				if (csize == 2)
					_mem.w16(dst + 2 * i, c);
				else
					_mem.w8(dst + i, c);
			}
			result = m;
		}
	} else if (name == "KERNEL32.DLL!HeapCreate") {
		result = 0x7000;
	} else if (name == "KERNEL32.DLL!HeapAlloc") {
		result = _mem.alloc(cpu.arg(2));
	} else if (name == "KERNEL32.DLL!HeapFree") {
		_mem.free(cpu.arg(2));
		result = 1;
	} else if (name == "KERNEL32.DLL!HeapReAlloc") {
		result = _mem.realloc(cpu.arg(2), cpu.arg(3));
	} else if (name == "KERNEL32.DLL!VirtualAlloc") {
		result = cpu.arg(0) ? cpu.arg(0) : _mem.alloc(cpu.arg(1));
		_mem.map(result, cpu.arg(1));
	} else if (name == "KERNEL32.DLL!VirtualFree") {
		result = 1;
	} else if (name == "USER32.DLL!RegisterWindowMessageA") {
		result = 0xc000 + _nextHandle++ % 0x3fff;
	} else if (name == "USER32.DLL!GetSysColor") {
		result = 0xc0c0c0;
	} else if (name == "USER32.DLL!GetSystemMetrics") {
		// SM_CXSCREEN, SM_CYSCREEN; small values otherwise.
		result = cpu.arg(0) == 0 ? 640 : cpu.arg(0) == 1 ? 480 : 16;
	} else if (name == "GDI32.DLL!GetDeviceCaps") {
		// BITSPIXEL 8, PLANES 1, LOGPIXELSX/Y 96, SIZEPALETTE 256, NUMRESERVED 20, RASTERCAPS with RC_PALETTE.
		switch (cpu.arg(1)) {
		case 12: result = 8; break;
		case 14: result = 1; break;
		case 38: result = 0x100; break;
		case 88: case 90: result = 96; break;
		case 104: result = 256; break;
		case 106: result = 20; break;
		case 8: result = 640; break;
		case 10: result = 480; break;
		default: result = 0; break;
		}
	} else if (name == "USER32.DLL!GetSysColorBrush" || name == "USER32.DLL!GetDC" || name == "USER32.DLL!LoadCursorA" ||
	           name == "USER32.DLL!LoadIconA" || name == "GDI32.DLL!GetStockObject") {
		// Opaque handles; the game draws through the native engine.
		result = 0x5000 + _nextHandle++;
	} else if (name == "USER32.DLL!ReleaseDC") {
		result = 1;
	} else if (name == "KERNEL32.DLL!GetVersionExA") {
		// Windows 98
		uint32 info = cpu.arg(0);
		_mem.w32(info + 4, 4);
		_mem.w32(info + 8, 10);
		_mem.w32(info + 12, 2222);
		_mem.w32(info + 16, 1);
		_mem.w8(info + 20, 0);
		result = 1;
	} else if (name == "ADVAPI32.DLL!RegOpenKeyExA" || name == "ADVAPI32.DLL!RegCreateKeyExA") {
		bool create = name.hasSuffix("CreateKeyExA");
		Common::String key = _mem.readString(cpu.arg(1));
		if (_regKeys.contains(cpu.arg(0)))
			key = _regKeys[cpu.arg(0)] + "\\" + key;
		else
			key = Common::String::format("%08x\\", cpu.arg(0)) + key;
		key.toLowercase();
		if (!create && !_registryKeys.contains(key)) {
			result = 2; // ERROR_FILE_NOT_FOUND
		} else {
			_registryKeys[key] = true;
			uint32 h = 0x80001000 + _nextHandle++;
			_regKeys[h] = key;
			_mem.w32(cpu.arg(create ? 7 : 4), h);
			if (create && cpu.arg(8))
				_mem.w32(cpu.arg(8), 2); // REG_OPENED_EXISTING_KEY
			result = 0;
		}
	} else if (name == "ADVAPI32.DLL!RegQueryValueExA") {
		Common::String v = _mem.readString(cpu.arg(1));
		v.toLowercase();
		Common::String k = _regKeys.contains(cpu.arg(0)) ? _regKeys[cpu.arg(0)] + "\\" + v : v;
		if (!_registry.contains(k)) {
			result = 2;
		} else {
			const Common::String &val = _registry[k];
			uint32 size = val.size() + 1;
			if (cpu.arg(3))
				_mem.w32(cpu.arg(3), 1); // REG_SZ
			uint32 cb = cpu.arg(5) ? _mem.r32(cpu.arg(5)) : 0;
			if (cpu.arg(4) && cb < size) {
				result = 234; // ERROR_MORE_DATA
			} else {
				if (cpu.arg(4))
					_mem.write(cpu.arg(4), val.c_str(), size);
				result = 0;
			}
			if (cpu.arg(5))
				_mem.w32(cpu.arg(5), size);
		}
	} else if (name == "ADVAPI32.DLL!RegSetValueExA") {
		Common::String v = _mem.readString(cpu.arg(1));
		v.toLowercase();
		Common::String k = _regKeys.contains(cpu.arg(0)) ? _regKeys[cpu.arg(0)] + "\\" + v : v;
		uint32 type = cpu.arg(3), data = cpu.arg(4), size = cpu.arg(5);
		if (type == 1)
			_registry[k] = _mem.readString(data, size);
		else
			debug(1, "Registry: %s set (type %d)", k.c_str(), type);
		result = 0;
	} else if (name == "ADVAPI32.DLL!RegCloseKey") {
		_regKeys.erase(cpu.arg(0));
		result = 0;
	} else if (name == "KERNEL32.DLL!LoadLibraryA") {
		debug(1, "Machine: LoadLibraryA(%s)", _mem.readString(cpu.arg(0)).c_str());
		result = 0x6000 + _nextHandle++;
	} else if (name == "KERNEL32.DLL!FreeLibrary") {
		result = 1;
	} else if (name == "KERNEL32.DLL!GetProcAddress") {
		debug(1, "Machine: GetProcAddress(%s): not available", cpu.arg(1) < 0x10000 ? "#" : _mem.readString(cpu.arg(1)).c_str());
		result = 0;
	} else if (name == "KERNEL32.DLL!GetModuleFileNameA") {
		Common::String path("C:\\MM2\\mm2run.exe");
		uint32 n = cpu.arg(2);
		if (n > 0) {
			if (path.size() >= n)
				path = Common::String(path.c_str(), n - 1);
			_mem.writeString(cpu.arg(1), path);
		}
		result = path.size();
	} else if (name == "KERNEL32.DLL!CreateDirectoryA") {
		debug(1, "Machine: CreateDirectoryA(%s)", _mem.readString(cpu.arg(0)).c_str());
		result = 1;
	} else if (name == "KERNEL32.DLL!GetLocalTime" || name == "KERNEL32.DLL!GetSystemTime") {
		TimeDate t;
		g_system->getTimeAndDate(t);
		uint32 st = cpu.arg(0);
		_mem.w16(st, t.tm_year + 1900);
		_mem.w16(st + 2, t.tm_mon + 1);
		_mem.w16(st + 4, t.tm_wday);
		_mem.w16(st + 6, t.tm_mday);
		_mem.w16(st + 8, t.tm_hour);
		_mem.w16(st + 10, t.tm_min);
		_mem.w16(st + 12, t.tm_sec);
		_mem.w16(st + 14, 0);
	} else if (name == "KERNEL32.DLL!GetTimeZoneInformation") {
		// Local time is what ScummVM reports, so no offset (TIME_ZONE_ID_UNKNOWN).
		_mem.fill(cpu.arg(0), 0, 172);
		result = 0;
	} else if (name == "KERNEL32.DLL!GetCurrentThreadId") {
		result = 1;
	} else if (name == "WINMM.DLL!timeGetTime" || name == "KERNEL32.DLL!GetTickCount") {
		result = g_system->getMillis();
	} else if (name == "USER32.DLL!wsprintfA") {
		Common::String out = formatPrintf(_mem, _mem.readString(cpu.arg(1)), cpu.r[ESP] + 12);
		_mem.writeString(cpu.arg(0), out);
		result = out.size();
	} else if (name == "USER32.DLL!CharUpperA") {
		uint32 a = cpu.arg(0);
		if (a < 0x10000) {
			result = toupper(a & 0xff);
		} else {
			Common::String str = _mem.readString(a);
			str.toUppercase();
			_mem.writeString(a, str);
			result = a;
		}
	} else if (fileCall(cpu, name, result)) {
		// done
	} else {
		cpu.dumpState();
		error("Machine: Win32 function %s not implemented", name.c_str());
	}
	cpu.r[EAX] = result;
}

// --- EmuObject ---

bool EmuObject::accepts(Message &msg) {
	if (_native)
		return false;
	// Slot 5 (+0x14)
	uint32 vt = _machine->mem().r32(_addr);
	uint32 m = _machine->allocMessage(msg);
	bool ok = _machine->cpu().call(_machine->mem().r32(vt + 0x14), _addr, m) != 0;
	_machine->readMessage(m, msg);
	_machine->mem().free(m);
	return ok;
}

uint32 EmuObject::eventMask() {
	return _native ? 0 : _machine->mem().r32(_addr + 0x18);
}

void EmuObject::handle(Message &msg) {
	// Slot 6 (+0x18)
	uint32 vt = _machine->mem().r32(_addr);
	uint32 m = _machine->allocMessage(msg);
	_machine->cpu().call(_machine->mem().r32(vt + 0x18), _addr, m);
	_machine->readMessage(m, msg);
	_machine->mem().free(m);
}

// --- Machine ---

Machine::Machine() : _cpu(_mem) {
	_cpu.setTrapHandler(this);
}

Machine::~Machine() {
}

bool Machine::loadPE(const Common::Path &exePath) {
	Common::File f;
	if (!f.open(exePath))
		return false;

	f.seek(0x3c);
	uint32 pe = f.readUint32LE();
	f.seek(pe);
	if (f.readUint32BE() != MKTAG('P', 'E', 0, 0))
		return false;
	f.skip(2);
	uint16 numSections = f.readUint16LE();
	f.skip(12);
	uint16 optSize = f.readUint16LE();
	f.skip(2);
	uint32 opt = f.pos();
	f.seek(opt + 28);
	_imageBase = f.readUint32LE();
	f.seek(opt + 56);
	_imageSize = f.readUint32LE();
	uint32 headerSize = f.readUint32LE();
	f.seek(opt + 96 + 8); // data directory 1: imports
	uint32 importRva = f.readUint32LE();

	_mem.map(_imageBase, _imageSize);
	byte *buf = new byte[headerSize];
	f.seek(0);
	f.read(buf, headerSize);
	_mem.write(_imageBase, buf, headerSize);
	delete[] buf;

	uint32 sec = opt + optSize;
	for (uint i = 0; i < numSections; i++) {
		f.seek(sec + i * 40 + 8);
		uint32 vsize = f.readUint32LE();
		uint32 va = f.readUint32LE();
		uint32 rawSize = f.readUint32LE();
		uint32 rawPtr = f.readUint32LE();
		uint32 n = MIN(vsize ? vsize : rawSize, rawSize);
		if (n) {
			buf = new byte[n];
			f.seek(rawPtr);
			f.read(buf, n);
			_mem.write(_imageBase + va, buf, n);
			delete[] buf;
		}
	}

	// Imports: every IAT slot points to a trap of its own.
	_mem.map(kImportThunks, 0x10000);
	uint32 thunk = kImportThunks;
	for (uint32 d = _imageBase + importRva; _mem.r32(d + 12); d += 20) {
		Common::String dll = _mem.readString(_imageBase + _mem.r32(d + 12));
		dll.toUppercase();
		uint32 lookup = _mem.r32(d) ? _mem.r32(d) : _mem.r32(d + 16);
		uint32 iat = _imageBase + _mem.r32(d + 16);
		for (uint32 k = 0;; k++) {
			uint32 e = _mem.r32(_imageBase + lookup + k * 4);
			if (!e)
				break;
			Common::String name;
			if (e & 0x80000000)
				name = Common::String::format("%s!#%d", dll.c_str(), e & 0xffff);
			else
				name = dll + "!" + _mem.readString(_imageBase + e + 2);
			_imports[thunk] = name;
			_mem.w32(iat + k * 4, thunk);
			int nargs = 0;
			for (uint j = 0; j < ARRAYSIZE(kImportArgs); j++)
				if (name == kImportArgs[j].name)
					nargs = kImportArgs[j].nargs;
			_cpu.addTrap(thunk, nargs * 4);
			thunk += 4;
		}
	}
	debug(1, "Machine: loaded %s, %d imports", exePath.toString().c_str(), _imports.size());
	return true;
}

void Machine::addTrap(uint32 addr, uint16 cleanup, int id) {
	_cpu.addTrap(addr, cleanup);
	_engineTraps[addr] = id;
}

void Machine::installEngineTraps() {
	for (uint i = 0; i < ARRAYSIZE(kEngineTraps); i++)
		addTrap(kEngineTraps[i].addr, kEngineTraps[i].cleanup, kEngineTraps[i].id);

	installChildTraps(kVtBitmap, 44);
	installChildTraps(kVtAnim, 44);
	installChildTraps(kVtHotspot, 44);
	installChildTraps(kVtSound, 44);
	installChildTraps(kVtTextRes, 32);
	installChildTraps(kVtText, 44);
	installChildTraps(kVtVideo, 44);

	_cpu._guardStart = 0x476000;
	_cpu._guardEnd = 0x490000;
	for (uint i = 0; i < ARRAYSIZE(kEmulatedEngineCode); i++)
		_cpu.allowRange(kEmulatedEngineCode[i].start, kEmulatedEngineCode[i].end);
}

void Machine::installChildTraps(uint32 vtable, int slots) {
	// Methods of the children are native; the trap's cleanup comes from
	// the original function's "ret n", found by scanning for it.
	for (int s = 1; s < slots; s++) {
		uint32 fn = _mem.r32(vtable + s * 4);
		if (fn < 0x476000 || fn >= 0x490000 || _childTraps.contains(fn) || _engineTraps.contains(fn))
			continue;
		if (_cpu.isAllowed(fn))
			continue;
		int cleanup = -1;
		for (uint i = 0; i < ARRAYSIZE(kEngineFunctions); i++)
			if (kEngineFunctions[i].addr == fn)
				cleanup = kEngineFunctions[i].cleanup;
		if (cleanup < 0) {
			warning("Machine: no ret found in %08x", fn);
			continue;
		}
		_cpu.addTrap(fn, cleanup);
		_childTraps[fn] = true;
	}
}

bool Machine::load(const Common::Path &exePath) {
	if (!loadPE(exePath))
		return false;

	_mem.map(kStackBottom, kStackTop - kStackBottom);
	_cpu.r[ESP] = kStackTop - 16;
	_mem.map(kTib, 0x1000);
	_mem.w32(kTib, 0xffffffff);     // SEH chain
	_mem.w32(kTib + 4, kStackTop);  // stack base
	_mem.w32(kTib + 8, kStackBottom);
	_mem.w32(kTib + 0x18, kTib);    // self
	_cpu.fsBase = kTib;

	installEngineTraps();
	_mem._faultHandler = [](uint32) { g_engine->_machine->cpu().dumpState(); };

	// The registry values the installer writes. The game folder is C:\MM2.
	Common::String lang = g_engine->getLanguageDir();
	Common::String key = "80000002\\software\\ivanoff interactive\\mm2";
	_registryKeys[key] = true;
	_registry[key + "\\labpath"] = "C:\\MM2\\Lab\\";
	_registry[key + "\\language"] = lang;
	_registry[key + "\\programpath"] = "C:\\MM2\\";
	_registry[key + "\\resource file"] = "C:\\MM2\\mm2";
	_registry[key + "\\resource local file"] = "C:\\MM2\\" + lang + "\\mm2lng";

	// The world object is opaque to the game code: only engine functions use it.
	_world = _mem.alloc(0x100);
	return true;
}

void Machine::runInitializers() {
	// __xc_a to __xc_z (FUN_00493d3f)
	for (uint32 a = 0x4c6000; a < 0x4c612c; a += 4) {
		uint32 fn = _mem.r32(a);
		if (fn)
			_cpu.call(fn, 0);
	}

	// The MFC application object (CWinApp, built by FUN_0047b5d0 in the
	// original). Game code reads only its command line (+0x70).
	uint32 moduleState = _cpu.call(kAfxGetModuleState, 0);
	_app = _mem.alloc(0x200);
	_mem.w32(_app + 0x70, _mem.allocString(""));
	_mem.w32(moduleState + 4, _app);
}

void Machine::createObjects() {
	// The second argument is the engine's input object, which owns global
	// sprites (cursors, frames). [unverified] Its id in the original.
	uint32 k = _mem.alloc(0x40);
	_mem.w16(k + 4, 0xfffe);
	_kernel = new EmuObject(this, 0xfffe, k);
	_kernel->_native = true;
	_kernel->_childContainer = _mem.alloc(0x14);
	_mem.w32(k + 0xc, _world);
	_mem.w32(k + 0x14, _kernel->_childContainer);
	_objects[k] = _kernel;
	_containers[_kernel->_childContainer] = _kernel;
	g_engine->_world->add(_kernel);

	uint32 args[2] = { _world, k };
	_cpu.call(kSetupObjects, 0, 2, args);
	debug(1, "Machine: created %d objects, %d children", _objects.size(), _children.size());
}

EmuObject *Machine::objectAt(uint32 addr) const {
	return _objects.contains(addr) ? _objects[addr] : nullptr;
}

Child *Machine::childAt(uint32 addr) const {
	return _children.contains(addr) ? _children[addr] : nullptr;
}

void Machine::writeMessage(uint32 a, const Message &msg) {
	_mem.w32(a, kMessageVtable);
	_mem.w16(a + 4, msg.target);
	_mem.w16(a + 6, 0);
	_mem.w32(a + 8, msg.mode);
	_mem.w32(a + 0xc, msg.cmd);
	_mem.w16(a + 0x10, msg.msg);
	_mem.w16(a + 0x12, 0);
	_mem.w32(a + 0x14, msg.handled);
	_mem.w32(a + 0x18, msg.p1);
	_mem.w32(a + 0x1c, msg.p2);
	_mem.w32(a + 0x20, msg.p3);
	_mem.w32(a + 0x24, msg.result);
	_mem.w32(a + 0x28, 0);
}

void Machine::readMessage(uint32 a, Message &msg) {
	msg.target = _mem.r16(a + 4);
	msg.mode = _mem.r32(a + 8);
	msg.cmd = _mem.r32(a + 0xc);
	msg.msg = _mem.r16(a + 0x10);
	msg.handled = _mem.r32(a + 0x14);
	msg.p1 = _mem.r32(a + 0x18);
	msg.p2 = _mem.r32(a + 0x1c);
	msg.p3 = _mem.r32(a + 0x20);
	msg.result = _mem.r32(a + 0x24);
}

uint32 Machine::allocMessage(const Message &msg) {
	uint32 a = _mem.alloc(0x2c);
	writeMessage(a, msg);
	return a;
}

Child *Machine::createChild(uint32 addr, Child *child, uint32 vtable) {
	// The child's memory was allocated by the game; keep the fields it may
	// read directly (+4 id, +0x28 type, +0x30 flags) in sync.
	_mem.w32(addr, vtable);
	child->_addr = addr;
	_children[addr] = child;
	syncChild(child);
	return child;
}

void Machine::syncChild(Child *c) {
	uint32 a = c->_addr;
	if (!a)
		return;
	_mem.w16(a + 4, c->getId());
	_mem.w32(a + 0xc, c->isLoaded());
	_mem.w32(a + 0x10, c->isVisible());
	_mem.w16(a + 0x28, c->getType());
	_mem.w32(a + 0x30, c->getFlags());
	_mem.w32(a + 0x34, c->getCursor());
}

void Machine::trap(CPU &cpu, uint32 addr) {
	if (_engineTraps.contains(addr))
		engineCall(cpu, _engineTraps[addr]);
	else if (_imports.contains(addr))
		importCall(cpu, _imports[addr]);
	else if (_childTraps.contains(addr))
		childMethod(cpu, addr);
	else
		error("Machine: unknown trap %08x", addr);
}

/** Releases a CString passed by value (the callee owns it). */
static void releaseCString(CPU &cpu, int argIndex) {
	cpu.call(kCStringRelease, cpu.r[ESP] + 4 + 4 * argIndex);
}

void Machine::engineCall(CPU &cpu, int id) {
	uint32 self = cpu.r[ECX];
	World *world = g_engine->_world;
	uint32 result = 0;

	switch (id) {
	case kObjectCtor: {
		// FUN_00476910(id, world, param)
		uint16 objId = cpu.arg(0);
		_mem.w32(self, 0x4b5430);
		_mem.w16(self + 4, objId);
		_mem.w32(self + 8, cpu.arg(2));
		_mem.w32(self + 0xc, cpu.arg(1));
		_mem.w32(self + 0x18, 0);
		uint32 vars = _mem.alloc(0xc);
		cpu.call(kVarContainerCtor, vars);
		_mem.w32(self + 0x10, vars);
		EmuObject *obj = new EmuObject(this, objId, self);
		obj->_childContainer = _mem.alloc(0x14);
		_mem.w32(self + 0x14, obj->_childContainer);
		_objects[self] = obj;
		_containers[obj->_childContainer] = obj;
		result = self;
		break;
	}
	case kRegister: {
		// FUN_0047bb40(object)
		uint32 a = cpu.arg(0);
		EmuObject *obj = objectAt(a);
		if (!obj)
			error("Machine: register of unknown object %08x", a);
		_mem.w32(a + 0xc, _world);
		world->add(obj);
		result = a;
		break;
	}
	case kMapLookup: {
		// FUN_0047bb10(id) on the object map or on a child container
		uint16 key = cpu.arg(0);
		if (self == _world) {
			EmuObject *obj = (EmuObject *)world->get(key);
			result = obj ? obj->getAddr() : 0;
		} else if (_containers.contains(self)) {
			Child *c = _containers[self]->getChild(key);
			result = c ? c->_addr : 0;
		} else {
			error("Machine: lookup in unknown map %08x", self);
		}
		break;
	}
	case kSend:
	case kPost: {
		Message m(cpu.arg(0), 0, cpu.arg(1), cpu.arg(2), cpu.arg(3), cpu.arg(4));
		if (m.target == kSelf)
			m.target = objectId(self);
		if (id == kPost) {
			world->post(m);
		} else {
			world->deliver(objectAt(self), m);
			result = m.result;
		}
		break;
	}
	case kCommand: {
		if (cpu.arg(1) == 0)
			break;
		Message m(cpu.arg(0), cpu.arg(1), cpu.arg(2), cpu.arg(3), cpu.arg(4), cpu.arg(5));
		if (m.target == kSelf)
			m.target = objectId(self);
		world->deliverCommand(objectAt(self), m);
		result = m.result;
		break;
	}
	case kAddChild: {
		EmuObject *obj = objectAt(self);
		Child *c = childAt(cpu.arg(0));
		if (!obj || !c) {
			cpu.dumpState();
			error("Machine: addChild(%08x, %08x)", self, cpu.arg(0));
		}
		obj->addChild(c);
		_mem.w32(c->_addr + 8, self);
		result = cpu.arg(0);
		break;
	}
	case kGetChild: {
		EmuObject *obj = objectAt(self);
		Child *c = obj ? obj->getChild(cpu.arg(0)) : nullptr;
		result = c ? c->_addr : 0;
		break;
	}
	case kGetChildOf: {
		// FUN_004770d0(childId, objectId): a child of another object, or of this one
		GameObject *obj = world->get(cpu.arg(1));
		if (!obj)
			obj = objectAt(self);
		Child *c = obj ? obj->getChild(cpu.arg(0)) : nullptr;
		result = c ? c->_addr : 0;
		break;
	}
	case kSetGlobalCursor:
		g_engine->setGlobalCursor(cpu.arg(0));
		break;
	case kStartTimer: {
		uint16 timerId = cpu.arg(0);
		int32 ms = cpu.arg(1);
		uint32 msgAddr = cpu.arg(2);
		if (timerId && ms > 0 && msgAddr) {
			Message m;
			readMessage(msgAddr, m);
			_mem.free(msgAddr);
			EmuObject *obj = objectAt(self);
			if (obj)
				obj->startTimer(timerId, ms, m);
		}
		break;
	}
	case kStopTimer: {
		EmuObject *obj = objectAt(self);
		if (obj)
			obj->stopTimer(cpu.arg(0));
		break;
	}
	case kChildSetCursor: {
		Child *c = childAt(self);
		if (c) {
			c->setCursor(cpu.arg(0));
			syncChild(c);
		}
		break;
	}
	case kNewBitmap:
		createChild(self, new Bitmap(cpu.arg(0), cpu.arg(1), cpu.arg(2), cpu.arg(3)), kVtBitmap);
		result = self;
		break;
	case kNewAnim:
		createChild(self, new Anim(cpu.arg(0), cpu.arg(1), cpu.arg(2), cpu.arg(3)), kVtAnim);
		result = self;
		break;
	case kNewHotspot:
		createChild(self, new Hotspot(cpu.arg(0), cpu.arg(1)), kVtHotspot);
		result = self;
		break;
	case kNewSound:
		createChild(self, new Sound(cpu.arg(0), cpu.arg(1)), kVtSound);
		result = self;
		break;
	case kNewTextRes:
		createChild(self, new TextResource(cpu.arg(0)), kVtTextRes);
		result = self;
		break;
	case kNewText: {
		// FUN_004797f0(id, fg, bg, CString font, size, wrap, align, charset, source, flags)
		debug(2, "TextSprite %d: fg %d bg %d font %s size %d wrap %d mode %d style %d source %08x flags %x",
		      cpu.arg(0), cpu.arg(1), cpu.arg(2), _mem.readString(cpu.arg(3)).c_str(), cpu.arg(4), cpu.arg(5),
		      cpu.arg(6), cpu.arg(7), cpu.arg(8), cpu.arg(9));
		TextSprite *t = new TextSprite(cpu.arg(0), cpu.arg(1), cpu.arg(2),
		                               _mem.readString(cpu.arg(3)), cpu.arg(4), cpu.arg(5),
		                               cpu.arg(6), cpu.arg(7), cpu.arg(8), cpu.arg(9));
		createChild(self, t, kVtText);
		releaseCString(cpu, 3);
		result = self;
		break;
	}
	case kNewVideo: {
		// FUN_0047af30(id, transparent, CString file, flags, param)
		VideoSprite *v = new VideoSprite(cpu.arg(0), cpu.arg(1), _mem.readString(cpu.arg(2)),
		                                 cpu.arg(3), cpu.arg(4));
		createChild(self, v, kVtVideo);
		releaseCString(cpu, 2);
		result = self;
		break;
	}
	case kMalloc:
	case kNew:
		result = _mem.alloc(cpu.arg(0));
		break;
	case kFree:
	case kDelete:
		_mem.free(cpu.arg(0));
		break;
	case kRand:
		result = g_engine->getRandomNumber(0x7fff);
		break;
	case kAtexit:
		// Nothing is cleaned up at exit.
		break;
	case kRedrawNow:
		// The window's vtable +0xb8: repaint now.
		g_engine->redrawNow();
		break;
	case kUnloadChildren: {
		// FUN_0047c630: command 2 to every child.
		EmuObject *obj = objectAt(self);
		if (obj) {
			const Common::Array<Child *> &ch = obj->getChildren();
			for (uint i = 0; i < ch.size(); i++) {
				Message m(obj->getId(), kCmdUnload, ch[i]->getId());
				ch[i]->handleCommand(m);
				syncChild(ch[i]);
			}
		}
		break;
	}
	case kSetKeyFocus:
		result = world->_keyFocus;
		world->_keyFocus = cpu.arg(0);
		break;
	case kAppCtor:
		// FUN_0047b5d0(startObject 0x11, 1, 640, 480, title, setup, ...):
		// IvanoffEngine plays this part.
		result = self;
		break;
	default:
		error("Machine: engine function %d not implemented", id);
	}
	cpu.r[EAX] = result;
}

void Machine::childMethod(CPU &cpu, uint32 addr) {
	uint32 self = cpu.r[ECX];
	Child *c = childAt(self);
	if (!c) {
		cpu.dumpState();
		error("Machine: child method %08x called on unknown object %08x", addr, self);
	}

	// Find which slot of the child's vtable was called.
	uint32 vt = _mem.r32(self);
	int slot = -1;
	for (int s = 1; s < 64; s++) {
		if (_mem.r32(vt + s * 4) == addr) {
			slot = s * 4;
			break;
		}
	}

	Message m(kSelf, 0, c->getId(), cpu.arg(0), cpu.arg(1), cpu.arg(2));
	uint32 result = 0;
	bool textRes = c->getType() == kTypeTextRes;

	switch (slot) {
	case 0x1c: m.cmd = 0x29; break;
	case 0x20: m.cmd = 0x2a; break;
	case 0x24: m.cmd = 0x2b; break;
	case 0x34: {
		// The command dispatcher, called with a message.
		Message in;
		readMessage(cpu.arg(0), in);
		c->handleCommand(in);
		writeMessage(cpu.arg(0), in);
		syncChild(c);
		cpu.r[EAX] = 0;
		return;
	}
	case 0x28:
		// +0x7c: the bitmap/animation drawing mode.
		if (Sprite *s = dynamic_cast<Sprite *>(c))
			s->_mode = m.p1;
		break;
	case 0x38: m.cmd = kCmdLoad; break;
	case 0x3c: m.cmd = kCmdUnload; break;
	case 0x40: m.cmd = kCmdPlay; break;
	case 0x44: m.cmd = kCmdStop; break;
	case 0x48: m.cmd = kCmdEnable; break;
	case 0x4c:
		// A direct call doesn't map z = 0 to -1 like command 6.
		if (Sprite *s = dynamic_cast<Sprite *>(c))
			s->setPos(m.p1, m.p2, m.p3);
		break;
	case 0x50: m.cmd = kCmdSetZ; break;
	case 0x54: m.cmd = kCmdSetOffset; break;
	case 0x58: {
		// FUN_00481b50(frame, message): per-frame messages of an animation.
		Anim *a = dynamic_cast<Anim *>(c);
		if (a) {
			if (m.p2) {
				Message t;
				readMessage(m.p2, t);
				_mem.free(m.p2);
				a->setFrameTrigger(m.p1, &t);
			} else {
				a->setFrameTrigger(m.p1, nullptr);
			}
		}
		break;
	}
	case 0x5c: m.cmd = kCmdUsePalette; break;
	case 0x60: m.cmd = kCmdPause; break;
	case 0x64:
		if (Anim *a = dynamic_cast<Anim *>(c))
			a->resume(m.p1, m.p2);
		break;
	case 0x90:
		// FUN_0047a920: a text box's size.
		if (TextSprite *t = dynamic_cast<TextSprite *>(c))
			t->setBox(m.p1, m.p2);
		else
			goto unknown;
		break;
	case 0x78:
		// FUN_00481c30: the position, also before loading.
		if (Sprite *s = dynamic_cast<Sprite *>(c))
			s->setInitialPos(m.p1, m.p2);
		break;
	case 0x7c:
		if (textRes) {
			c->play(m.p1, m.p2, 0);
			break;
		}
		// fall through
	default:
	unknown:
		cpu.dumpState();
		error("Machine: child %d (type %04x) method slot %x (%08x) not implemented", c->getId(), c->getType(), slot, addr);
	}

	if (m.cmd) {
		c->handleCommand(m);
		result = m.result;
	}
	syncChild(c);
	cpu.r[EAX] = result;
}

} // End of namespace Ivanoff
