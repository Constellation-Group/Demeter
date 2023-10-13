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
* This file describes the methods callable to manipulate a static UsageWatchdog instance.
* 
* Author : Lylian Siffre ( lylian.siffre@impakt.io )
*/

#pragma once

#include "UsageWatchdog.h"

#include "DemeterLogger.h"

bool is_watchdog_under_lockdown();

void push_cpu_measurement(float);

void delete_usage_watchdog();

void enable_watchdog();