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
 * This file handles the logger of Demeter.
 *
 * Author : Lylian Siffre ( lylian.siffre@impakt.io )
 */

#include "DemeterLogger.h"

void initialize_logger() {
		// Create a file rotating logger with 5mb size max and 3 rotated files
		constexpr auto max_size = 1048576 * 5;
		constexpr auto max_files = 3;
		const auto loggerd = spdlog::rotating_logger_mt("PM", "logs/log.txt", max_size, max_files);

		// Default pattern 
		// [2014-10-31 23:46:59.678] [my_loggername] [info] Some message
		// Our pattern
		// [2014-10-31 23:46:59.678] Some message
		loggerd->set_pattern("[%c] %v");
		spdlog::set_default_logger(loggerd);
}