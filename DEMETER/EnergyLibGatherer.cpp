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
 * This file defines logic on power data gathering.
 *
 * This logic lies on the intel power gadget. 
 * ( https://www.intel.com/content/www/us/en/developer/articles/training/using-the-intel-power-gadget-api-on-mac-os-x.html )
 * ( https://www.intel.com/content/www/us/en/developer/articles/training/using-the-intel-power-gadget-30-api-on-windows.html )
 * Author : Lylian Siffre ( lylian.siffre@impakt.io )
 * Mainly based on Adel Nourredine's work. ( See git repository )
 */

#include "EnergyLibGatherer.h"

void get_power_elib(double* power, CIntelPowerGadgetLib* intel_pg_lib, const int num_nodes, const int num_msrs) {
	spdlog::info("energyLibGetPower");
	int n_data, func_id;

	for (int i = 0; i < num_nodes; i++) {
		for (int k = 0; k < num_msrs; k++) {
			double r_double[3];
			intel_pg_lib->GetMsrFunc(k, &func_id);
			// Demo using "Psys" (in fact Package)
			//wchar_t* msrName = (wchar_t*)malloc(50);
			//intel_pg_lib->GetMsrName(k, msrName);
			if (func_id != 1) {
			 	continue;
			}
			spdlog::info("getPowerData %d -- %d", i, k);
			intel_pg_lib->GetPowerData(i, k, r_double, &n_data);
			*power += r_double[2]; // power in mWh %6.2f
			// Still in demo
			/*double pr = -1;
			if (nData == 3) {
				pr = rDouble[0];
			}
			wprintf(L"%s nd: %d n:%d m:%d fid:%d \t p: %f\n", msrName, nData, i, k, funcId, pr);
			free(msrName);*/
		}
	}
}

void update_vars_elib(const double* power, double* co2, double* price) {
	spdlog::info("energyLibUpdateVars");
	constexpr double co2e = 35.0; // gram per kWh
	constexpr double kWh_price = 0.1525 * 100; // cents of euros per kWh

	*co2 = *power * co2e; // Get CO2 emissions
	*price = (*power * kWh_price) / 1000.0; // Get price in cents of euros
}
