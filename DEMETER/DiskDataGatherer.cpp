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
 * This file describes the logic of how disk data is gathered.
 *
 * Author : Lylian Siffre ( lylian.siffre@impakt.io )
 */

#include "DiskDataGatherer.h"

using namespace std;

static std::unordered_map<DWORD, ULONGLONG> bytes_read_map;
static std::unordered_map<DWORD, ULONGLONG> bytes_write_map;


int get_disk_stats(const DWORD pid, const HANDLE process_handle, ULONGLONG* bytes_read, ULONGLONG* bytes_write) {
	const auto pioc = static_cast<PIO_COUNTERS>(malloc(sizeof(IO_COUNTERS)));
	
	if (pioc == nullptr) {
		spdlog::critical("Could not allocate memory for IO_COUNTERS\n");
		return 1;
	}

	if (process_handle == nullptr) {
		free(pioc);
		return 1;
	}

	if (GetProcessIoCounters(process_handle, pioc)) {
		ULONGLONG old_bytes_read;
		ULONGLONG old_bytes_write;

		const ULONGLONG new_bytes_read = pioc->ReadTransferCount;
		const ULONGLONG new_bytes_write = pioc->WriteTransferCount;

		if (bytes_read_map.contains(pid)) {
			old_bytes_read = bytes_read_map[pid];
			old_bytes_write = bytes_write_map[pid];
		} else {
			// No data is set for the old value, so if we try to get the delta, we will just get the
			// whole number of bytes read and written by this pid so far. That is useless. So, by
			// setting old = new, we'll have a result of 0.
			old_bytes_read = new_bytes_read;
			old_bytes_write = new_bytes_write;
		}

		// PID re-attribution guard
		if (new_bytes_read < old_bytes_read || new_bytes_write < old_bytes_write) {
			old_bytes_read = 0;
			old_bytes_write = 0;
		}

		const ULONGLONG delta_bytes_read = new_bytes_read - old_bytes_read;
		const ULONGLONG delta_bytes_write = new_bytes_write - old_bytes_write;

		bytes_read_map[pid] = new_bytes_read;
		bytes_write_map[pid] = new_bytes_write;

		*bytes_read = delta_bytes_read;
		*bytes_write = delta_bytes_write;
	}
	free(pioc);
	return 0;
}