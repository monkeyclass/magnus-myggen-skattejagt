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

#ifndef IVANOFF_DETECTION_H
#define IVANOFF_DETECTION_H

#include "engines/advancedDetector.h"

namespace Ivanoff {

enum IvanoffDebugChannels {
	kDebugGraphics = 1,
	kDebugPath,
	kDebugScan,
	kDebugFilePath,
	kDebugScript,
};

extern const PlainGameDescriptor ivanoffGames[];

extern const ADGameDescription gameDescriptions[];

#define GAMEOPTION_ORIGINAL_SAVELOAD GUIO_GAMEOPTIONS1

} // End of namespace Ivanoff

class IvanoffMetaEngineDetection : public AdvancedMetaEngineDetection<ADGameDescription> {
	static const DebugChannelDef debugFlagList[];

public:
	IvanoffMetaEngineDetection();
	~IvanoffMetaEngineDetection() override {}

	const char *getName() const override {
		return "ivanoff";
	}

	const char *getEngineName() const override {
		return "Ivanoff";
	}

	const char *getOriginalCopyright() const override {
		return "Magnus & Myggen (C) IVANOFF Interactive";
	}

	const DebugChannelDef *getDebugChannels() const override {
		return debugFlagList;
	}
};

#endif // IVANOFF_DETECTION_H
