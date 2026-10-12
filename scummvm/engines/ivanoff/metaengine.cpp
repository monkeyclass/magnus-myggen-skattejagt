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

#include "backends/keymapper/action.h"
#include "backends/keymapper/keymap.h"
#include "backends/keymapper/standard-actions.h"
#include "common/translation.h"

#include "ivanoff/metaengine.h"
#include "ivanoff/detection.h"
#include "ivanoff/ivanoff.h"

namespace Ivanoff {

static const ADExtraGuiOptionsMap optionsList[] = {
	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

} // End of namespace Ivanoff

const char *IvanoffMetaEngine::getName() const {
	return "ivanoff";
}

const ADExtraGuiOptionsMap *IvanoffMetaEngine::getAdvancedExtraGuiOptions() const {
	return Ivanoff::optionsList;
}

Common::Error IvanoffMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	*engine = new Ivanoff::IvanoffEngine(syst, desc);
	return Common::kNoError;
}

bool IvanoffMetaEngine::hasFeature(MetaEngineFeature f) const {
	return checkExtendedSaves(f) ||
		(f == kSupportsLoadingDuringStartup);
}

Common::KeymapArray IvanoffMetaEngine::initKeymaps(const char *target) const {
	using namespace Common;

	Keymap *keymap = new Keymap(Keymap::kKeymapTypeGame, "ivanoff-default", _("Default keymappings"));
	Action *act;

	act = new Action(kStandardActionLeftClick, _("Interact"));
	act->setLeftClickEvent();
	act->addDefaultInputMapping("MOUSE_LEFT");
	act->addDefaultInputMapping("JOY_A");
	keymap->addAction(act);

	act = new Action(kStandardActionRightClick, _("Right click"));
	act->setRightClickEvent();
	act->addDefaultInputMapping("MOUSE_RIGHT");
	act->addDefaultInputMapping("JOY_B");
	keymap->addAction(act);

	// The custom events are the Windows virtual key codes the game reads.
	static const struct {
		const char *id;
		const char *label;
		int vk;
		const char *key, *joy;
	} kKeys[] = {
		{ "MENU", _s("Menu / skip"), 0x1b, "ESCAPE", "JOY_X" },
		{ "UP", _s("Up"), 0x26, "UP", "JOY_UP" },
		{ "DOWN", _s("Down"), 0x28, "DOWN", "JOY_DOWN" },
		{ "LEFT", _s("Left"), 0x25, "LEFT", "JOY_LEFT" },
		{ "RIGHT", _s("Right"), 0x27, "RIGHT", "JOY_RIGHT" },
		{ "SPACE", _s("Space"), 0x20, "SPACE", "JOY_Y" },
		{ "LOAD", _s("Load screen"), 0x74, "F5", nullptr },
		{ "SAVE", _s("Save screen"), 0x75, "F6", nullptr }
	};
	for (uint i = 0; i < ARRAYSIZE(kKeys); i++) {
		act = new Action(kKeys[i].id, _(kKeys[i].label));
		act->setCustomEngineActionEvent(kKeys[i].vk);
		act->addDefaultInputMapping(kKeys[i].key);
		if (kKeys[i].joy)
			act->addDefaultInputMapping(kKeys[i].joy);
		keymap->addAction(act);
	}

	return Keymap::arrayOf(keymap);
}

#if PLUGIN_ENABLED_DYNAMIC(IVANOFF)
REGISTER_PLUGIN_DYNAMIC(IVANOFF, PLUGIN_TYPE_ENGINE, IvanoffMetaEngine);
#else
REGISTER_PLUGIN_STATIC(IVANOFF, PLUGIN_TYPE_ENGINE, IvanoffMetaEngine);
#endif
