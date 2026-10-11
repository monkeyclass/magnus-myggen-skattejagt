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

#include "ivanoff/machine.h"
#include "ivanoff/scripts/scripts.h"

namespace Ivanoff {

#define SCRIPT(vtable, name) \
	IVANOFF_SCRIPT(name);
#include "ivanoff/scripts/registry.h"
#undef SCRIPT

static const ScriptEntry kScripts[] = {
#define SCRIPT(vtable, name) { vtable, create##name },
#include "ivanoff/scripts/registry.h"
#undef SCRIPT
	{ 0, nullptr }
};

Script *createScript(EmuObject *obj) {
	uint32 vt = obj->getVtable();
	for (const ScriptEntry *e = kScripts; e->create; e++)
		if (e->vtable == vt)
			return e->create(obj);
	return nullptr;
}

} // End of namespace Ivanoff
