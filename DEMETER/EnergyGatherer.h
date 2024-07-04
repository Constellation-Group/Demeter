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

#include <Windows.h>


// Intel MSRs
constexpr UINT64 MSR_RAPL_POWER_UNIT = 0x606;
constexpr UINT64 MSR_PKG_ENERGY_STATUS = 0x611;
constexpr UINT64 MSR_DRAM_ENERGY_STATUS = 0x00000619;
constexpr UINT64 MSR_PP0_ENERGY_STATUS = 0x00000639;
constexpr UINT64 MSR_PP1_ENERGY_STATUS = 0x00000641;
constexpr UINT64 MSR_PLATFORM_ENERGY_STATUS = 0x0000064d;

int LoadScaphandreDriver(BOOL = false);

float ReadEnergyConsumption(UINT64, BOOL = true);

float ReadEnergyConsumption(BOOL = true);

void CloseScaphandre();