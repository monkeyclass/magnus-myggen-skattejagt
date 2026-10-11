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

namespace Ivanoff {

const PlainGameDescriptor ivanoffGames[] = {
	{ "mm2", "Magnus & Myggen 2: Den store skattejagt" },
	{ 0, 0 }
};

// The Nordic retail CD (2002): one executable and data file, plus a language
// pack per language (DK, N, S, SF). The game runs the executable's logic, so
// mm2run.exe from an installation is required.
#define MM2_ENTRY(lang) \
	{ \
		"mm2", \
		nullptr, \
		AD_ENTRY2s("mm2.dat", "7b58496e0cced154ff83a3aa21aee05e", 80490872, \
		           "mm2run.exe", "07e76c5ccf57e2ad9b0e0d91944791b2", 839680), \
		lang, \
		Common::kPlatformWindows, \
		ADGF_UNSTABLE, \
		GUIO1(GUIO_NOMIDI) \
	}

const ADGameDescription gameDescriptions[] = {
	MM2_ENTRY(Common::DA_DNK),
	MM2_ENTRY(Common::NB_NOR),
	MM2_ENTRY(Common::SV_SWE),
	MM2_ENTRY(Common::FI_FIN),

	AD_TABLE_END_MARKER
};

} // End of namespace Ivanoff
