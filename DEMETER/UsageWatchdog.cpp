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
* This file describes the behaviour of the UsageWatchdog, this object is intended to trigger
* a lockdown mecanism when a treshold is triggered by a measurement.
* 
* Author : Lylian Siffre ( lylian.siffre@impakt.io )
*/

#include "UsageWatchdog.h"

UsageWatchdog::UsageWatchdog() {
	start_time_ = time(nullptr);
	calibration_ended_ = false;
	sum_first_hour_ = 0;
	last_stat_ = 0;
	avg_first_hour_ = 0;
	amount_of_measurements_ = 0;
	treshold_ = 0;
}

/*
* This method tells if the treshold was triggered during the last call to pushMeasurement.
*/
bool UsageWatchdog::is_lockdown() {
	if (calibration_ended_
		&& (last_stat_ > 3 * avg_first_hour_)) {
		return true;
	}
	return false;
}

/*
* Method to call in order to trigger the lockdown treshold and in order to calibrate
* the watchdog.
 */
void UsageWatchdog::push_measurement(const float cpu_percent) {
	last_stat_ = cpu_percent;

	if (calibration_ended_) return;

	sum_first_hour_ += cpu_percent;
	amount_of_measurements_++;

	const time_t now = time(nullptr);

	const auto seconds = static_cast<unsigned long>(difftime(now, start_time_));

	if (seconds >= 1200) { 
		avg_first_hour_ = sum_first_hour_ / static_cast<float>(amount_of_measurements_);
		spdlog::info("Watchdog calibration ended.");
	}
	calibration_ended_ = seconds >= 1200;
}