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
 * This file defines logic on process information gathering.
 * Informations are the process name, if it is a service or not.
 *
 * This logic lies on the api psapi. 
 * ( https://docs.microsoft.com/fr-fr/windows/win32/psapi/psapi-functions )
 *
 * Author : Lylian Siffre ( lylian.siffre@impakt.io )
 */

#include "ProcessInfoGatherer.h"


const static SC_HANDLE sc_handle = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
static BYTE* services = nullptr;
static DWORD services_length = 0;

/*
* This method exists because the call to EnumServicesStatusExA is slow for some reason.
* 
* This method refreshes the list of service processes and stores in the services
*/
void refresh_services() {
	if (services != nullptr) {
		free(services);
	}

	DWORD pcb_bytes_needed = 0;
	DWORD lp_resume_handle = 0;
	constexpr DWORD service_type = (SERVICE_DRIVER | SERVICE_FILE_SYSTEM_DRIVER | SERVICE_KERNEL_DRIVER
		| SERVICE_WIN32 | SERVICE_WIN32_OWN_PROCESS | SERVICE_WIN32_SHARE_PROCESS);

	EnumServicesStatusExA(
		sc_handle,
		SC_ENUM_PROCESS_INFO,
		service_type,
		SERVICE_STATE_ALL,
		nullptr,
		0,
		&pcb_bytes_needed,
		&services_length,
		&lp_resume_handle,
	nullptr);

	lp_resume_handle = 0;
	services = static_cast<BYTE*>(malloc(sizeof(BYTE) * pcb_bytes_needed));
	const DWORD cb_buff_size = pcb_bytes_needed;

	EnumServicesStatusExA(
		sc_handle,
		SC_ENUM_PROCESS_INFO,
		service_type,
		SERVICE_STATE_ALL,
		services,
		cb_buff_size,
		&pcb_bytes_needed,
		&services_length,
		&lp_resume_handle,
		nullptr
	);
}



bool is_process_service(const DWORD pid) {
	for (DWORD i = 0; i < services_length; i++) {
		const ENUM_SERVICE_STATUS_PROCESSA* service_status =
			reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSA*>((services + (i * sizeof(ENUM_SERVICE_STATUS_PROCESSA))));
		if (service_status->ServiceStatusProcess.dwProcessId == pid) {
			return true;
		}
	}
	return false;
}

string get_process_name(DWORD pid, const HANDLE process_handle) {
	TCHAR sz_process_name[MAX_PATH] = TEXT("<unknown>");

	// Get the process name.

	if (nullptr != process_handle) {
		HMODULE h_module;
		DWORD cb_needed;

		if (EnumProcessModulesEx(process_handle, &h_module, sizeof(h_module),
			&cb_needed, LIST_MODULES_ALL)) {
			GetModuleBaseName(process_handle, nullptr, sz_process_name,
				sizeof(sz_process_name) / sizeof(TCHAR));
		}
	}

	auto ws = std::wstring(sz_process_name);
	auto str = std::string(ws.begin(), ws.end());

	return str;
}