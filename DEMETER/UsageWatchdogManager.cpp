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
* This file manages the static Watchdog instance and maps domain specific calls to the Watchdog
* object.
*
* Author : Lylian Siffre ( lylian.siffre@impakt.io )
*/

#include "UsageWatchdogManager.h"

static UsageWatchdog * watchdog = new UsageWatchdog();
static bool watchdog_enabled = false;

bool is_watchdog_under_lockdown() {
	if (!watchdog_enabled) return false;
	if (watchdog == nullptr) {
		spdlog::critical("Usage Watchdog is NULL.\n");
		return false;
	}
	return watchdog->is_lockdown();
}

void push_cpu_measurement(const float cpu_measurement) {
	if (!watchdog_enabled) return;
	if (watchdog == nullptr) {
		spdlog::critical("Usage Watchdog is NULL.\n");
		return;
	}
	watchdog->push_measurement(cpu_measurement);
}

void delete_usage_watchdog() {
	delete watchdog;
}

void enable_watchdog() {
	spdlog::info("Watchdog enabled");
	watchdog_enabled = true;
}