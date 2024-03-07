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
#include "EnergyGatherer.h"
#include <iostream>
#include <map>
#include "DemeterLogger.h"

const DWORD RAPL_CTL_CODE = CTL_CODE(
    FILE_DEVICE_UNKNOWN, 
    static_cast<UINT16>(MSR_RAPL_POWER_UNIT),
    METHOD_BUFFERED,
    FILE_READ_DATA | FILE_WRITE_DATA
);

HANDLE driver_handle;

float power_unit, energy_unit, time_unit;
static std::map<UINT64, UINT32> total_energy_consumed;
time_t last_sample_time;
bool use_platform_register = false;

/**
 * @brief Reads data from the driver.
 *
 * This function reads data from the driver using DeviceIoControl function.
 *
 * @param inputData Input data to be sent to the driver.
 * @param outputData Pointer to the buffer that receives the data from the driver.
 * @param bytesReturned Pointer to a variable that receives the size of the data returned by the
 * driver.
 * @return Returns TRUE if the function succeeds, otherwise FALSE.
 */
BOOL readData(UINT64 inputData, UINT64* outputData, DWORD* bytesReturned)
{
	if (!DeviceIoControl(
        driver_handle,
        RAPL_CTL_CODE,
        &inputData,
        sizeof(inputData),
        outputData,
        sizeof(outputData),
        bytesReturned,
        NULL
    )) {
        std::cerr << "DeviceIoControl failed. Error code: " << GetLastError() << std::endl;
        return 0;
    }

    return 1;
}

/**
 * @brief Loads energy conversion units.
 *
 * This function loads energy conversion units from the driver.
 */
void loadEnergyConversionUnits()
{
	if(driver_handle == NULL)
	{
        std::cerr << "Scaphandre must be loaded." << std::endl;
        return;
	}

    constexpr UINT64 energy_unit_msr = MSR_RAPL_POWER_UNIT;
    UINT64 res;
    DWORD bytes_returned;

    if (!readData(energy_unit_msr, &res, &bytes_returned))
    {
        std::cerr << "Couldn't read data from MSR. See previous error." << std::endl;
        return;
    }

    // Time Units
    constexpr UINT64 time_mask = 0xF0000;
    const UINT64 time_val = res & time_mask;
    time_unit = 1.0f / (pow(2.0f, static_cast<float>(time_val >> 16)));

	// Energy Units
    constexpr UINT64 energy_mask = 0x1F00;
    const UINT64 energy_val = res & energy_mask;
	energy_unit = 1.0f / (pow(2.0f, static_cast<float>(energy_val >> 8)));

    // Power Units
    constexpr UINT64 power_mask = 0xF;
    const UINT64 power_val = res & power_mask;
    power_unit = 1.0f / (pow(2.0f, static_cast<float>(power_val)));
}

/**
 * @brief Loads the Scaphandre driver.
 *
 * This function loads the Scaphandre driver and initializes energy conversion units.
 *
 * @param force_use_platform Boolean flag indicating whether to force using the platform register.
 * @return Returns 0 if successful, otherwise returns 1.
 */
int LoadScaphandreDriver(BOOL force_use_platform)
{
    if(driver_handle != NULL)
    {
        std::cerr << "Scaphandre already loaded." << std::endl;
        return 1;
    }

    driver_handle = CreateFile(
        L"\\\\.\\ScaphandreDriver",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (driver_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open device. Error code: " << GetLastError() << std::endl;
        return 1;
    }

    loadEnergyConversionUnits();

    if(force_use_platform)
    {
        spdlog::info("Forced MSR_PLATFORM_ENERGY_COUNTER usage");
        use_platform_register = true; 
    }
    
    return 0;
}

/**
 * @brief Reads energy consumption from the specified MSR.
 *
 * This function reads energy consumption from the specified Model Specific Register (MSR).
 *
 * @param msr MSR to read energy consumption from.
 * @return Returns the energy consumption in Joules.
 */
float readEnergyConsumption(UINT64 msr)
{
    if (driver_handle == NULL)
    {
        std::cerr << "Scaphandre must be loaded." << std::endl;
        return -1.0f;
    }

    UINT64 res;
    DWORD bytes_returned;

    if (!readData(msr, &res, &bytes_returned))
    {
        std::cerr << "Couldn't read data from MSR. See previous error." << std::endl;
        return-1.0f;
    }

    if(total_energy_consumed.find(msr) == total_energy_consumed.end())
    {
        total_energy_consumed[msr] = 0;
    }
    const UINT32 old_consumption = total_energy_consumed[msr];

    // Get new cumulated energy consumption
    const UINT32 new_consumption = res & 0xFFFFFFFF;
    const UINT32 delta_consumption = new_consumption - old_consumption;
    total_energy_consumed[msr] = new_consumption;

    const float energy_consumption = static_cast<float>(delta_consumption) * energy_unit;

    return energy_consumption;
}

/**
 * @brief Reads energy consumption from the specified MSR and updates time if required.
 *
 * This function reads energy consumption from the specified MSR and updates the last sample time
 * if required.
 *
 * @param msr MSR to read energy consumption from.
 * @param update_time Boolean flag indicating whether to update the last sample time.
 * @return Returns the energy consumption in Joules.
 */
float ReadEnergyConsumption(UINT64 msr, BOOL update_time)
{
    const float energy_consumption = readEnergyConsumption(msr);

    if (energy_consumption <= -1.0f)
    {
        std::cerr << "Couldn't read energy consumption. See previous error." << std::endl;
        return-1.0f;
    }

    if (update_time)
    {
        const time_t actual_time = time(nullptr);
        last_sample_time = actual_time;
    }

    return energy_consumption / 3.6f;
}

/**
 * @brief Reads energy consumption.
 *
 * This function reads energy consumption using the platform register if required, otherwise using
 * the package register.
 *
 * @param update_time Boolean flag indicating whether to update the last sample time.
 * @return Returns the energy consumption in Watt hours.
 */
float ReadEnergyConsumption(BOOL update_time)
{
    UINT64 msr = use_platform_register ? MSR_PLATFORM_ENERGY_STATUS : MSR_PKG_ENERGY_STATUS;
    const float energy_consumption = readEnergyConsumption(msr);

    if (energy_consumption <= -1.0f)
    {
        std::cerr << "Couldn't read energy consumption. See previous error." << std::endl;
        return-1.0f;
    }

    if (update_time)
    {
        const time_t actual_time = time(nullptr);
        last_sample_time = actual_time;
    }

    return energy_consumption / 3.6f;
}

/**
 * @brief Closes the Scaphandre driver.
 *
 * This function closes the Scaphandre driver.
 */
void CloseScaphandre()
{
	if(driver_handle != NULL)
	{
        CloseHandle(driver_handle);
        driver_handle = NULL;
	}
}
