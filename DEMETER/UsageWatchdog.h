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
* This file describes the structure of the UsageWatchdog, see UsageWatchdog.cpp for more details.
*
* Author : Lylian Siffre ( lylian.siffre@impakt.io )
*/

#pragma once

#include <stdio.h>
#include <time.h> 

#include "DemeterLogger.h"

class UsageWatchdog
{
private:
	float sum_first_hour_;
	float avg_first_hour_;
	time_t start_time_;
	bool calibration_ended_;
	int amount_of_measurements_;
	float treshold_;
	float last_stat_;


public:
	UsageWatchdog();
	bool is_lockdown();
	void push_measurement(float);
};

