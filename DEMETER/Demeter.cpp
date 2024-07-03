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
 * This is the main file.
 * This file starts the logging loop and starts the network gathering logic.
 * 
 * Author : Lylian Siffre ( lylian.siffre@impakt.io )
 */


#define _WIN32_WINNT 0x0A00
#define _WINSOCKAPI_
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <csignal>
#include <windows.h>
#include <strsafe.h>
#include <fstream>

#include "misc.h" /* LoadNpcapDlls */
#include "CPUDataGatherer.h"
#include "RAMDataGatherer.h"
#include "ProcessNetDataGatherer.h"
#include "ProcessInfoGatherer.h"
#include "DiskDataGatherer.h"
#include "SystemInfoGatherer.h"
#include <unordered_map>
#include <unordered_set>

#include <cxxopts.hpp>

#include <chrono>

#include "DemeterLogger.h"
#include "Demeter.h"

#include <ranges>

#include "EnergyGatherer.h"
#include "UsageWatchdogManager.h"

using namespace std;;

static ofstream printfile;

uint64_t timestamp_now() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void send_data_to_output(const string& data, const bool std_output) {
	if (std_output) {
		printf(data.c_str());
	}
	else {
		printfile << fixed << data;
	}
}

void open_csv_file() {
	const char* username = get_system_username();

	const time_t now = time(nullptr);
	const tm* ptm = localtime(&now);
	char buffer[32];
	// Format: 15_06_2009
	strftime(buffer, 32, "%d_%m_%Y", ptm);

	const string file_name = "log-" + string(buffer) + "-" + username + ".csv";

	const ifstream f(file_name);
	const bool exists = f.good();

	printfile.open(file_name, std::ofstream::out | std::ofstream::app);

	if (exists) {
		send_data_to_output("----RESTARTLINE----\n", false);
	}
	else {
		send_data_to_output(
			"TIME;NAME;CPU;CPUC;NetUP;NetUpC;NetDown;NetDownC;DiskR;DiskW;DirkRC;DiskWC;RAM;SumC\n",
			false
		);
	}
}

void sigtrap(int signo){
	spdlog::info("SIGTRAP! {}", signo);

	delete_usage_watchdog();
	CloseScaphandre();

	SIZE_T opened_handles_count;
	pcap_t** opened_handles = get_pcap_handle(&opened_handles_count);
	for (int i = 0; i < opened_handles_count; i++) {
		pcap_close(opened_handles[i]);
	}

	printfile.close();

	exit(0);
}

int start_demeter(const int loop_interval,
	bool console, 
	bool watchdog, 
	bool localloop,
	bool std_output, 
	float disk_r_cost, 
	float disk_w_cost,
	bool force_use_platform) 
{

	::ShowWindow(::GetConsoleWindow(), console ? SW_NORMAL : SW_HIDE);

	if (watchdog) {
		enable_watchdog();
	}

	set_loopback_capture(localloop);

	spdlog::info("Started with interval {} second(s).", loop_interval);

	/* Load Npcap and its functions. */
	spdlog::info("Loading Npcap");
	if (!load_npcap_dlls()) {
		spdlog::critical("Couldn't load Npcap");
		exit(1);
	}

	map_ports_to_pid();

	if (start_packet_sniffing() == 1) {
		spdlog::info("Successfully started sniffing");
	}
	else {
		spdlog::error("Failed to start sniffing");
	}

	if (!std_output) {
		open_csv_file();
	}

	if (LoadScaphandreDriver(force_use_platform))
	{
		spdlog::critical("Couldn't load Scaphandre Driver.");
		exit(1);
	}

	signal(SIGINT, sigtrap);
	signal(SIGTERM, sigtrap);
	signal(SIGSEGV, sigtrap);

	// Init for cpu usage formula
	init_cpu_getters();

	constexpr double infinity = std::numeric_limits<double>::infinity();

	spdlog::info("Loop interval: {}", loop_interval);

	int current_day = -1;
	const DWORD demeter_pid = _getpid();
	const string full_total_str("System Total");
	const string application_total_str("Application Total");
	const string not_recorded_total_str("Not recorded Total");
	const string cpu_energy_str("CPU Energy");

	while (true) {
		if (is_watchdog_under_lockdown()) {
			spdlog::warn("Watchdog triggered at {}, DEMETER under lockdown for 1 minute", time(nullptr));
			Sleep(60000);
			// After sleeping, PM will push it's CPU readings and should
			// update watchdog's lockdown state
		}

		time_t process_data_loop_start = time(nullptr);
		const tm* local_time = localtime(&process_data_loop_start);
		int today = local_time->tm_mday;

		if (current_day == -1) {
			current_day = today;
		}

		if (current_day != today) {
			if (!std_output) {
				printfile.close();
				open_csv_file();
			}
			current_day = today;
		}

		double energy_consumption = ReadEnergyConsumption();

		// Get the list of process identifiers.
		DWORD all_processes_pids[1024], returned_processes_count;

		if (EnumProcesses(all_processes_pids, sizeof(all_processes_pids), &returned_processes_count)) {
			DWORD processes_count;
			// Calculate how many process identifiers were returned.
			processes_count = returned_processes_count / sizeof(DWORD);

			// Print the name and process identifier for each process.
			unordered_map<string, float> CPU_usage_map;
			unordered_map<string, SIZE_T> RAM_usage_map;
			unordered_map<string, DWORD> net_up_usage_map;
			unordered_map<string, DWORD> net_down_usage_map;
			unordered_map<string, ULONGLONG> disk_write_usage_map;
			unordered_map<string, ULONGLONG> disk_read_usage_map;
			unordered_map<string, int> process_name_count_map;

			refresh_services();

			for (unsigned int i = 0; i < processes_count; i++) {
				DWORD pid = all_processes_pids[i];

				HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
					FALSE, pid);

				string process_name = get_process_name(pid, process_handle);

				SIZE_T process_ram = get_working_set(process_handle);
				float process_cpu_usage = get_process_cpu_usage(pid, process_handle);
				DWORD process_net_up = get_packet_sent_weight(pid);
				DWORD process_net_down = get_packet_received_weight(pid);
				ULONGLONG process_disk_read = 0;
				ULONGLONG process_disk_write = 0;

				get_disk_stats(pid, process_handle, &process_disk_read, &process_disk_write);

				if (infinity <= process_cpu_usage) {
					process_cpu_usage = 0;
				}

				record_measurements(
					CPU_usage_map,
					full_total_str,
					RAM_usage_map,
					net_up_usage_map,
					net_down_usage_map,
					disk_read_usage_map,
					disk_write_usage_map,
					process_name_count_map,
					process_cpu_usage,
					process_ram,
					process_net_up,
					process_net_down,
					process_disk_read,
					process_disk_write);

				if (!is_process_service(pid) && process_name != "<unknown>") {
					record_measurements(
						CPU_usage_map,
						process_name,
						RAM_usage_map,
						net_up_usage_map,
						net_down_usage_map,
						disk_read_usage_map,
						disk_write_usage_map,
						process_name_count_map,
						process_cpu_usage,
						process_ram,
						process_net_up,
						process_net_down,
						process_disk_read,
						process_disk_write);

					record_measurements(
						CPU_usage_map,
						application_total_str,
						RAM_usage_map,
						net_up_usage_map,
						net_down_usage_map,
						disk_read_usage_map,
						disk_write_usage_map,
						process_name_count_map,
						process_cpu_usage,
						process_ram,
						process_net_up,
						process_net_down,
						process_disk_read,
						process_disk_write);
				}
				else {
					record_measurements(
						CPU_usage_map,
						not_recorded_total_str,
						RAM_usage_map,
						net_up_usage_map,
						net_down_usage_map,
						disk_read_usage_map,
						disk_write_usage_map,
						process_name_count_map,
						process_cpu_usage,
						process_ram,
						process_net_up,
						process_net_down,
						process_disk_read,
						process_disk_write);
				}

				// Push measurement for watchdog to lockdown PM in case
				// of high cpu consumption
				if (pid == demeter_pid) {
					push_cpu_measurement(process_cpu_usage);
				}

				CloseHandle(process_handle);
			}

			record_measurements(
				CPU_usage_map,
				cpu_energy_str,
				RAM_usage_map,
				net_up_usage_map,
				net_down_usage_map,
				disk_read_usage_map,
				disk_write_usage_map,
				process_name_count_map,
				1,
				0,
				0,
				0,
				0,
				0);

			reset_packet_bandwith();

			time_t process_data_gathering_end = time(nullptr);
			time_t process_data_gathering_duration = (process_data_gathering_end - process_data_loop_start);
			process_data_gathering_duration = 
				process_data_gathering_duration < loop_interval 
				? 
				loop_interval
				: 
				process_data_gathering_duration;

			for (const auto& process_name : CPU_usage_map | views::keys) {
				float process_cpu_usage = CPU_usage_map[process_name];
				SIZE_T process_ram = RAM_usage_map[process_name];

				DWORD process_net_up = net_up_usage_map[process_name];
				DWORD process_net_down = net_down_usage_map[process_name];

				ULONGLONG process_disk_read = disk_read_usage_map[process_name];
				ULONGLONG process_disk_write = disk_write_usage_map[process_name];

				// qty of bytes => Mo/s
				float process_bandwidth_up = (process_net_up / 1000000.0f) / (process_data_gathering_duration * 1.0f);
				// qty of bytes => Mo/s
				float process_bandwidth_down = 
					(process_net_down / 1000000.0f)
					/
					(process_data_gathering_duration * 1.0f);


				// Value: 0.068 mWh/MB/s
				// Config 10Mb/s => 306 mW => 30.6 mW/Mb/s
				// => 244.8 mW/MB/s => 0.068 mWh/MB/s
				const float milli_wh_conversion = 0.068f;

				float process_net_up_consumption = milli_wh_conversion * process_bandwidth_up; // mWh
				float process_net_down_consumption = milli_wh_conversion * process_bandwidth_down; // mWh

				float process_cpu_consumption = static_cast<float>(energy_consumption)*process_cpu_usage; // mWh
				process_cpu_usage *= 100.0f;

				// qty of bytes => Mo/s
				float process_disk_read_speed = 
					(process_disk_read / 1000000.0f) 
					/
					(1.0f * process_data_gathering_duration);
				// qty of bytes => Mo/s
				float process_disk_write_speed = 
					(process_disk_write / 1000000.0f) 
					/ 
					(1.0f * process_data_gathering_duration);

				// 0.78 mW/(Mo/s) for read
				// 0.98 mW/(Mo/s) for write 

				float process_disk_read_consumption = (process_disk_read_speed * disk_r_cost) / 3600.0f; // mWh
				float process_disk_write_consumption = (process_disk_write_speed * disk_w_cost) / 3600.0f; // mWh

				float sum_consumption =
					process_disk_read_consumption + process_disk_write_consumption
					+ process_net_up_consumption + process_net_down_consumption
					+ process_cpu_consumption;

				string data = to_string(time(nullptr)) + ";"
					+ process_name + ";"
					+ to_string(process_cpu_usage)+";"
					+ to_string(process_cpu_consumption)+";"

					+ to_string(process_bandwidth_up)+";"
					+ to_string(process_net_up_consumption)+";"

					+ to_string(process_bandwidth_down)+";"
					+ to_string(process_net_down_consumption)+";"

					+ to_string(process_disk_read_speed)+";"
					+ to_string(process_disk_write_speed)+";"

					+ to_string(process_disk_read_consumption)+";"
					+ to_string(process_disk_write_consumption)+";"

					+ to_string(process_ram)+";"

					+ to_string(sum_consumption)
					
					+ "\n";

				send_data_to_output(data, std_output);
			}

			CPU_usage_map.clear();
			RAM_usage_map.clear();
			net_up_usage_map.clear();
			net_down_usage_map.clear();
			disk_write_usage_map.clear();
			disk_read_usage_map.clear();
			process_name_count_map.clear();
		} else {
			spdlog::critical("Can't enum processes !");
		}
		spdlog::debug("Logged");

		time_t process_data_loop_end = time(nullptr);
		time_t process_data_loop_exec_time = (process_data_loop_end - process_data_loop_start) * 1000;
		spdlog::debug("Took {} ms", process_data_loop_exec_time);

		time_t time_to_wait = (static_cast<long long>(loop_interval) * 1000) - (process_data_loop_exec_time);
		map_ports_to_pid();
		if (time_to_wait > 0) {
			Sleep(static_cast<DWORD>(time_to_wait));
		}
	}
	spdlog::critical("Probe crashed.");
	printfile.close();

	return 0;
}

void record_measurements(
	std::unordered_map<std::string, float>& cpu_usage_map,
	const std::string& name, 
	cxxopts::NameHashMap& ram_usage_map, 
	std::unordered_map<std::string, DWORD>& net_up_usage_map,
	std::unordered_map<std::string, DWORD>& net_down_usage_map,
	cxxopts::NameHashMap& disk_read_usage_map, 
	cxxopts::NameHashMap& disk_write_usage_map, 
	std::unordered_map<std::string, int>& process_name_count_map,
	const float process_cpu_usage,
	const SIZE_T& process_ram, 
	const DWORD& process_net_up,
	const DWORD& process_net_down,
	const ULONGLONG& process_disk_read, 
	const ULONGLONG& process_disk_write)
{
	if (!cpu_usage_map.contains(name)) {
		cpu_usage_map[name] = 0;
		ram_usage_map[name] = 0;
		net_up_usage_map[name] = 0;
		net_down_usage_map[name] = 0;
		disk_read_usage_map[name] = 0;
		disk_write_usage_map[name] = 0;
		process_name_count_map[name] = 0;
	}

	cpu_usage_map[name] += process_cpu_usage;
	ram_usage_map[name] += process_ram;
	net_up_usage_map[name] += process_net_up;
	net_down_usage_map[name] += process_net_down;
	disk_read_usage_map[name] += process_disk_read;
	disk_write_usage_map[name] += process_disk_write;
	process_name_count_map[name] += 1;
}

int main(const int argc, char* argv[]) {

	initialize_logger();

	cxxopts::Options options("DEMETER", "Desktop Energy Meter");

	options.add_options()
		("c,hide-console", "Hides the console")
		("w,no-watchdog", "Disables the watchdog")
		("i,interval", "Defines the measurement interval in seconds", 
			cxxopts::value<int>()->default_value("10"))
		("drcost", "Sets the disc reading cost (mW/MB)", 
			cxxopts::value<float>()->default_value("0.78f"))
		("dwcost", "Sets disk write cost (mW/MB)", 
			cxxopts::value<float>()->default_value("0.98f"))
		("l,no-loopbackcap", "Disables local loop packet capture")
		("stdoutput", "Redirects data writing to the console")
		("use-platform", "Force the use of the MSR_PLATFORM_ENERGY_COUNTER register")
		("h,help", "Displays help");

	const auto result = options.parse(argc, argv);

	if (result.count("help")) {
		printf("%s\n", options.help().c_str());
		exit(0);
	}

	const bool console = !result["hide-console"].as<bool>();
	const bool watchdog = !result["no-watchdog"].as<bool>();
	const int interval = result["interval"].as<int>();
	const bool localloop = !result["no-loopbackcap"].as<bool>();
	const bool force_use_platform = result["use-platform"].as<bool>();
	const bool std_output =  result["stdoutput"].as<bool>();
	const float disk_r_cost = result["drcost"].as<float>();
	const float disk_w_cost = result["dwcost"].as<float>();

	return start_demeter(interval,
		console, 
		watchdog, 
		localloop,
		std_output,
		disk_r_cost, 
		disk_w_cost, 
		force_use_platform);
}