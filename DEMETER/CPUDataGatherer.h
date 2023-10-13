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
#pragma once

#include <windows.h>
#include <psapi.h>
#include <map>
#include <mutex>

void init_cpu_getters();

void update_user_time(DWORD, ULARGE_INTEGER);

void update_time(DWORD, ULARGE_INTEGER);

void update_sys_time(DWORD, ULARGE_INTEGER);

ULARGE_INTEGER get_user_time(DWORD);

ULARGE_INTEGER get_time(DWORD);

ULARGE_INTEGER get_sys_time(DWORD);

float get_process_cpu_usage(DWORD, HANDLE);

float get_process_cpu_usage();