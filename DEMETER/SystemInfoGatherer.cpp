/*
 * Demeter - Desktop Energy Meter
 * Copyright (C) 2023  Constellation
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
/*
 * This file defines methods for getting details on the host system.
 *
 * Author : Lylian Siffre ( lylian.siffre@impakt.io )
 */

#include "SystemInfoGatherer.h"

char* get_system_username() {
	constexpr EXTENDED_NAME_FORMAT format = NameDisplay;
	ULONG size = 0;

	GetUserNameEx(format, nullptr, &size);

	const auto name_buffer = static_cast<LPWSTR>(malloc(size * sizeof(WCHAR)));

	if (name_buffer == nullptr) {
		spdlog::critical("Could not allocate memory for name_buffer\n");
		return nullptr;
	}

	const auto name = static_cast<char*>(malloc(size * sizeof(char)));

	if (name == nullptr) {
		spdlog::critical("Could not allocate memory for name\n");
		return nullptr;
	}

	GetUserNameEx(format, name_buffer, &size);

	wcstombs(name, name_buffer, size + 1); // size, on success does not include '\0' char.

	free(name_buffer);

	return name;
}