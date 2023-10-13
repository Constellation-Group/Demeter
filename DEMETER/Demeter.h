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

void record_measurements(
	std::unordered_map<std::string, float>& cpu_usage_map,
	const std::string& name,
	cxxopts::NameHashMap& ram_usage_map,
	std::unordered_map<std::string, DWORD>& net_up_usage_map,
	std::unordered_map<std::string, DWORD>& net_down_usage_map,
	cxxopts::NameHashMap& disk_read_usage_map,
	cxxopts::NameHashMap& disk_write_usage_map,
	std::unordered_map<std::string, int>& process_name_count_map,
	float process_cpu_usage,
	const SIZE_T& process_ram,
	const DWORD& process_net_up,
	const DWORD& process_net_down,
	const ULONGLONG& process_disk_read,
	const ULONGLONG& process_disk_write);