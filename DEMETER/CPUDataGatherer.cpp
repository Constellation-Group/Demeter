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
 * This file defines logic on cpu data gathering.
 *
 * This logic lies on the api psapi. ( https://docs.microsoft.com/fr-fr/windows/win32/psapi/psapi-functions )
 *
 * Author : Lylian Siffre ( lylian.siffre@impakt.io )
 * Based on Adel Nourredine's work. ( See git repository )
 */

#include "CPUDataGatherer.h"

static int number_of_processors;
static std::map<DWORD, ULARGE_INTEGER> process_user_time_map;
static std::map<DWORD, ULARGE_INTEGER> process_sys_time_map;
static std::map<DWORD, ULARGE_INTEGER> process_time_map;

void init_cpu_getters() {
	SYSTEM_INFO sys_info;

	GetSystemInfo(&sys_info);
	number_of_processors = sys_info.dwNumberOfProcessors;
}

void update_user_time(const DWORD pid, const ULARGE_INTEGER time) {
	process_user_time_map[pid] = time;
}

void update_time(const DWORD pid, const ULARGE_INTEGER time) {
	process_time_map[pid] = time;
}

void update_sys_time(const DWORD pid, const ULARGE_INTEGER time) {
	process_sys_time_map[pid] = time;
}

ULARGE_INTEGER get_user_time(const DWORD pid) {
	if (process_user_time_map.contains(pid)) {
		return process_user_time_map[pid];
	}
	else {
		FILETIME ftime;
		GetSystemTimeAsFileTime(&ftime);
		ULARGE_INTEGER now;
		memcpy(&now, &ftime, sizeof(FILETIME));
		return now;
	}
}

ULARGE_INTEGER get_time(const DWORD pid) {
	if (process_time_map.contains(pid)) {
		return process_time_map[pid];
	}
	else {
		FILETIME ftime;
		GetSystemTimeAsFileTime(&ftime);
		ULARGE_INTEGER now;
		memcpy(&now, &ftime, sizeof(FILETIME));
		return now;
	}
}

ULARGE_INTEGER get_sys_time(const DWORD pid) {
	if (process_sys_time_map.contains(pid)) {
		return process_sys_time_map[pid];
	}
	else {
		FILETIME ftime;
		GetSystemTimeAsFileTime(&ftime);
		ULARGE_INTEGER now;
		memcpy(&now, &ftime, sizeof(FILETIME));
		return now;
	}
}

/* 
 * This method returns the percentage of CPU used by the process corresponding to the passed pid &
 * handle.
 * 
 * This is non trivial, "usage" of CPU is computed from the time the process occupied the cpu over a
 * period of time.
 * There are a total of 3 different times:
 *  - Total elapsed time since last call.
 *  - User time "used" since last call.
 *  - Kernel (sys) time "used" since last call.
 * 
 * The formula is simple:
 * CPU% = (user_time + kernel_time) / total_time
 * 
 * /!\ It is important to divide the result by the number of processors to get an accurate
 * percentage, otherwise, your percentage can be above 100%.
 */
float get_process_cpu_usage(const DWORD pid, const HANDLE process_handle) {

	FILETIME ftime, fsys, fuser;
	ZeroMemory(&ftime, sizeof(FILETIME));
	ZeroMemory(&fsys, sizeof(FILETIME));
	ZeroMemory(&fuser, sizeof(FILETIME));

	ULARGE_INTEGER now, sys, user, last_sys_cpu, last_user_cpu, last_cpu;
	ZeroMemory(&now, sizeof(ULARGE_INTEGER));
	ZeroMemory(&sys, sizeof(ULARGE_INTEGER));
	ZeroMemory(&user, sizeof(ULARGE_INTEGER));
	ZeroMemory(&last_sys_cpu, sizeof(ULARGE_INTEGER));
	ZeroMemory(&last_user_cpu, sizeof(ULARGE_INTEGER));
	ZeroMemory(&last_cpu, sizeof(ULARGE_INTEGER));

	float percent;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	GetProcessTimes(process_handle, &ftime, &ftime, &fsys, &fuser);

	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));

	last_sys_cpu = get_sys_time(pid);
	last_user_cpu = get_user_time(pid);
	last_cpu = get_time(pid);

	// PID re-attribution guard
	if ((sys.QuadPart < last_sys_cpu.QuadPart)
		|| (user.QuadPart < last_user_cpu.QuadPart)) {
		percent = 0;
	}
	else {
		percent = static_cast<float>((sys.QuadPart - last_sys_cpu.QuadPart) + (user.QuadPart - last_user_cpu.QuadPart));
		percent /= static_cast<float>(now.QuadPart - last_cpu.QuadPart);
		percent /= static_cast<float>(number_of_processors);
	}

	update_sys_time(pid, sys);
	update_user_time(pid, user);
	update_time(pid, now);

	return percent;
}

float get_process_cpu_usage() {
	return get_process_cpu_usage(-1, nullptr);
}