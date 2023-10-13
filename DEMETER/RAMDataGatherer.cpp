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
 * This file defines logic on memory data gathering.
 * 
 * This logic lies on the api psapi. 
 * ( https://docs.microsoft.com/fr-fr/windows/win32/psapi/psapi-functions ) 
 * 
 * Author : Lylian Siffre ( lylian.siffre@impakt.io )
 */

#include "RAMDataGatherer.h"

/*Returns the amount of space occupied by the process corresponding to the passed handle.
 * https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/aa965225(v=vs.85)
 * https://stackoverflow.com/questions/1984186/what-is-private-bytes-virtual-bytes-working-set
 */
SIZE_T get_working_set(const HANDLE process_handle) {
	PROCESS_MEMORY_COUNTERS_EX pmc;

	if (process_handle == nullptr)
		return 0;

	SIZE_T used_memory = 0;

	if (GetProcessMemoryInfo(process_handle, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
		used_memory = pmc.PrivateUsage;
	}

	return used_memory;
}