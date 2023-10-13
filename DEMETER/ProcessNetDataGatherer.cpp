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
 * In this file you will find all the logic about network data gathering and processing.
 * Most of the logic is fragmented in functions to do specific tasks such as increasing the amount
 * of data received at a specific port, etc.
 * 
 * The logic lies on the library npcap ( https://npcap.com/ ) , 
 * which provides a way to capture incoming and outgoing packets.
 * Using this library, we capture packets, determine which port(s) is source and destination, and
 * then we increment the quantity of data received / sent through this port.
 * 
 * This way we can have the bandwith usage of each port, and then of each process.
 * 
 * Here, you will also find a way to get which port is bound to which process.
 * 
 * Author : Lylian Siffre ( lylian.siffre@impakt.io )
 */


#include "ProcessNetDataGatherer.h"

#include <ranges>

using namespace std;

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

// Port <-> PID mapping
static std::map<DWORD, vector<u_short>*> process_port_map;
static std::map<DWORD, SIZE_T> process_port_size;
static std::recursive_mutex pid_port_map_lock;
static std::recursive_mutex pid_port_size_map_lock;

// Port bandwidth
static UINT tx_port_packet_counts[65536];
static UINT rx_port_packet_counts[65536];
static std::recursive_mutex tx_port_packet_map_lock;
static std::recursive_mutex rx_port_packet_map_lock;

// ----- Utils data structures (used for npcap loop inter thread values)
typedef struct thread_data {
	pcap_t* handle;
	pcap_if_t* dev;
} thread_data, *pthread_data;

typedef struct loop_data {
	int linkType;
	char* addresses;
} loop_data, *ploop_data;

static int opened_handles_count;
static pcap_t** opened_handles = nullptr;

static bool loopback_capture = true;
// -----

void set_loopback_capture(const bool capture) {
	loopback_capture = capture;
}

void reset_port_map(const DWORD pid) {
	spdlog::info("resetPortMap");
	pid_port_map_lock.lock();
	pid_port_size_map_lock.lock();
	process_port_map.erase(pid);
	process_port_size[pid] = 0;
	pid_port_size_map_lock.unlock();
	pid_port_map_lock.unlock();
}

void alloc_port_map(const DWORD pid) {
	pid_port_map_lock.lock();
	spdlog::info("allocPortMap");
	pid_port_size_map_lock.lock();
	// process_port_map[pid] = new u_short[100];
	process_port_map[pid] = new vector<u_short>(1, 0);
	if (process_port_map[pid] == nullptr) {
		spdlog::info("Could not allocate memory for pid array");
		exit(1);
	}
	process_port_size[pid] = 0;
	pid_port_size_map_lock.unlock();
	pid_port_map_lock.unlock();
}

void add_port_to_map(const DWORD pid, const u_short port) {
	spdlog::info("addPortToMap");
	pid_port_map_lock.lock();
	if (!process_port_map.contains(pid)) {
		alloc_port_map(pid);
	}

	pid_port_size_map_lock.lock();
	const SIZE_T index = process_port_size[pid];
	const vector<u_short> ports = *process_port_map[pid];

	// Adding port only if it is not already present
	boolean found = false;
	for (unsigned int i = 0; i < index; i++) {
		if (!found && (ports[i] == port)) {
			found = true;
		}
	}

	if (!found) {
		// (*process_port_map[pid])[index] = port;
		process_port_map[pid]->push_back(port);
		process_port_size[pid] = index + 1;
	}
 	
	pid_port_size_map_lock.unlock();
	pid_port_map_lock.unlock();
}

void clear_pid_port_map() {
	spdlog::info("clearPidPortMap");
	pid_port_map_lock.lock();
	pid_port_size_map_lock.lock();

	for (const auto& snd : process_port_map | views::values) { delete snd; }

	spdlog::info("clearPidPortMap seconds deleted");

	process_port_size.clear();

	spdlog::info("clearPidPortMap process_port_size cleared");

	process_port_map.clear();

	spdlog::info("clearPidPortMap process_port_map cleared");
	
	pid_port_size_map_lock.unlock();
	pid_port_map_lock.unlock();
}

void update_tcp4() {
	// STATIC MALLOC
	static auto tcp_table = static_cast<MIB_TCPTABLE2*>(malloc(sizeof(MIB_TCPTABLE2))); 
	ULONG mib_tcptable_size;

	if (tcp_table == nullptr) {
		spdlog::info("Error allocating memory");
		return;
	}

	mib_tcptable_size = sizeof(MIB_TCPTABLE);
	// Make an initial call to GetTcpTable2 to
	// get the necessary size into the mib_tcptable_size variable
	DWORD return_code = GetTcpTable2(tcp_table, &mib_tcptable_size, FALSE);
	if (return_code == ERROR_INSUFFICIENT_BUFFER) {
		free(tcp_table); // FREE
		tcp_table = static_cast<MIB_TCPTABLE2*>(malloc(mib_tcptable_size)); // MALLOC
		if (tcp_table == nullptr) {
			spdlog::info("Error allocating memory");
			return;
		}
		return_code = GetTcpTable2(tcp_table, &mib_tcptable_size, FALSE);
	}

	// Loop over TCP table values
	if (return_code == NO_ERROR) {
		for (int i = 0; i < static_cast<int>(tcp_table->dwNumEntries); i++) {
			const DWORD pid = tcp_table->table[i].dwOwningPid;
			const u_short port = ntohs((u_short)tcp_table->table[i].dwLocalPort);
			//spdlog::info("pid {} __ {}", pid, port);
			add_port_to_map(pid, port);
		}
	}
	else {
		spdlog::info("\tGetTcpTable2 failed with {}", return_code);
	}
}

void update_tcp6() {
	// STATIC MALLOC
	static auto tcp_table = static_cast<MIB_TCP6TABLE2*>(malloc(sizeof(MIB_TCP6TABLE2))); 
	DWORD mib_tcptable_size;

	if (tcp_table == nullptr) {
		spdlog::info("Error allocating memory");
		return;
	}

	mib_tcptable_size = sizeof(MIB_TCP6TABLE2);
	// Make an initial call to GetTcpTable2 to
	// get the necessary size into the mib_tcptable_size variable
	DWORD return_code = GetTcp6Table2(tcp_table, &mib_tcptable_size, FALSE);
	if (return_code == ERROR_INSUFFICIENT_BUFFER) {
		free(tcp_table); // FREE
		tcp_table = static_cast<MIB_TCP6TABLE2*>(malloc(mib_tcptable_size)); // MALLOC
		if (tcp_table == nullptr) {
			spdlog::info("Error allocating memory");
			return;
		}
		return_code = GetTcp6Table2(tcp_table, &mib_tcptable_size, FALSE);
	}

	// Loop over tcp table values
	if (return_code == NO_ERROR) {
		for (int i = 0; i < static_cast<int>(tcp_table->dwNumEntries); i++) {
			const DWORD pid = tcp_table->table[i].dwOwningPid;
			const u_short port = ntohs((u_short)tcp_table->table[i].dwLocalPort);
			add_port_to_map(pid, port);
		}
	}
	else {
		spdlog::info("\tGetTcp6Table2 failed with {}", return_code);
	}
}

void update_tcp() {
	update_tcp4();
	update_tcp6();
}

void update_udp(const ULONG ul_af) {
	// STATIC MALLOC
	static auto udp_table = static_cast<MIB_UDPTABLE_OWNER_PID*>(malloc(sizeof(MIB_UDPTABLE_OWNER_PID))); 
	DWORD mib_udptable_size;

	if (udp_table == nullptr) {
		spdlog::info("Error allocating memory");
		return;
	}

	mib_udptable_size = sizeof(MIB_UDPTABLE_OWNER_PID);
	// Make an initial call to GetTcpTable2 to
	// get the necessary size into the mib_tcptable_size variable
	DWORD return_code = GetExtendedUdpTable(
		udp_table,
		&mib_udptable_size,
		FALSE,
		ul_af, 
		UDP_TABLE_OWNER_PID,
		0
	);
	if (return_code == ERROR_INSUFFICIENT_BUFFER) {
		free(udp_table); // FREE
		udp_table = static_cast<MIB_UDPTABLE_OWNER_PID*>(malloc(mib_udptable_size)); // MALLOC
		if (udp_table == nullptr) {
			spdlog::info("Error allocating memory");
			return;
		}
		return_code = GetExtendedUdpTable(
			udp_table, 
			&mib_udptable_size,
			FALSE,
			ul_af,
			UDP_TABLE_OWNER_PID,
			0);
	}

	// Loop over udp table values
	if (return_code == NO_ERROR) {
		for (int i = 0; i < static_cast<int>(udp_table->dwNumEntries); i++) {
			const DWORD pid = udp_table->table[i].dwOwningPid;
			const u_short port = ntohs((u_short)udp_table->table[i].dwLocalPort);
			//spdlog::info("pid {} __ {}", pid, port);
			add_port_to_map(pid, port);
		}
	}
	else {
		spdlog::info("\tGetExtendedUdpTable failed with {}", return_code);
	}
}

void update_udp() {
	update_udp(AF_INET);
	update_udp(AF_INET6);
}

void map_ports_to_pid() {
	spdlog::info("MapPortsToPID");
	// Useless to update two times (or more) at the same time the pid port map
	const bool map_locked = pid_port_map_lock.try_lock();
	if (!map_locked) {
		return;
	}
	if (!pid_port_size_map_lock.try_lock()) {
		if (map_locked) {
			pid_port_map_lock.unlock();
		}
		return;
	}
	clear_pid_port_map();
	update_udp();
	update_tcp();

	pid_port_size_map_lock.unlock();
	pid_port_map_lock.unlock();
}

bool get_connected_ports(const DWORD pid, vector<u_short>* vectret) {
	spdlog::info("GetConnectedPorts");
	pid_port_map_lock.lock();
	if (!process_port_map.contains(pid)) {
		pid_port_map_lock.unlock();
		return false;
	}
	*vectret = *process_port_map[pid];
	pid_port_map_lock.unlock();

	return true;
}

SIZE_T get_connected_ports_counts(const DWORD pid) {
	spdlog::info("GetConnectedPortsCounts");
	pid_port_map_lock.lock();
	if (!process_port_map.contains(pid)) {
		pid_port_map_lock.unlock();
		return 0;
	}

	pid_port_size_map_lock.lock();
	const SIZE_T size = process_port_size[pid];
	pid_port_size_map_lock.unlock();

	pid_port_map_lock.unlock();
	return size;
}

DWORD count_packets_for_pid(const DWORD pid, const UINT* port_packets) {
	spdlog::info("CountPacketsForPID");
	const SIZE_T ports_count = get_connected_ports_counts(pid);
	if (ports_count == 0) {
		return 0;
	}

	vector<u_short> ports;

	if (!get_connected_ports(pid, &ports)) {
		spdlog::info("Could not get ports.");
		return 0;
	}

	DWORD packet_sum = 0;
	for (unsigned int i = 0; i < ports_count; i++) {
		const u_short port = ports[i];
		packet_sum += port_packets[port];
	}

	return packet_sum;
}

DWORD get_packet_sent_weight(const DWORD pid) {
	tx_port_packet_map_lock.lock();
	const DWORD res = count_packets_for_pid(pid, tx_port_packet_counts);
	tx_port_packet_map_lock.unlock();
	return res;
}

DWORD get_packet_received_weight(const DWORD pid) {
	rx_port_packet_map_lock.lock();
	const DWORD res = count_packets_for_pid(pid, rx_port_packet_counts);
	rx_port_packet_map_lock.unlock();
	return res;
}

void increment_packet_sent_weight(const u_short port, const u_int packet_size) {
	// spdlog::info("IncrementPacketSent");
	tx_port_packet_map_lock.lock();
	tx_port_packet_counts[port] += packet_size;
	tx_port_packet_map_lock.unlock();
}

void increment_packet_received_weight(const u_short port, const u_int packet_size) {
	// spdlog::info("IncrementPacketReceived");
	rx_port_packet_map_lock.lock();
	rx_port_packet_counts[port] += packet_size;
	rx_port_packet_map_lock.unlock();
}

void reset_packet_bandwith() {
	spdlog::info("Resetting bandwidth");
	rx_port_packet_map_lock.lock();
	tx_port_packet_map_lock.lock();

	// Resetting counter arrays, optimized way according to:
	// https://stackoverflow.com/questions/9146395/reset-c-int-array-to-zero-the-fastest-way

	memset(rx_port_packet_counts, 0, sizeof(rx_port_packet_counts));
	memset(tx_port_packet_counts, 0, sizeof(tx_port_packet_counts));
	
	spdlog::info("Bandwidth reset");
	rx_port_packet_map_lock.unlock();
	tx_port_packet_map_lock.unlock();
}

pcap_t** get_pcap_handle(SIZE_T* size) {
	*size = opened_handles_count;
	return opened_handles;
}

/*
 * This function is called by the pcap library.
 * This is the main function of this logic, it will parse the packet and increment data sent or
 * received by the corresponding port, if any.
 */
static void got_packet(u_char* args, const struct pcap_pkthdr* header, const u_char* packet) {
	if (is_watchdog_under_lockdown()) return; // Do not handle packets if we are under lockdown.

	const auto p_loop_data = reinterpret_cast<ploop_data>(args);
	int data_link_type = p_loop_data->linkType;
	const string addresses = p_loop_data->addresses;

	// Here we'll parse the packet. because we are monitoring both loopback and normal interfaces,
	// Some packets will have an ethernet frame, some wont, so we have to parse them properly.
	// Procedure:
	// 
	// IP packet = None
	// 
	// If data_link_type == 1
	//		Extract IP packet from ether frame.
	// else
	//		Extract IP packet from loopback frame.
	// 
	// PDU = Extract PDU from IP packet.
	// 
	// 
	// 
	// In the PDU, only destination port and source port are relevant, 
	// they lie on the first 32 bits of the PDU. As:
	// 
	//			16 bits					16 bits
	//	+----------------------+----------------------+
	//	|		SRC PORT	   |		DST PORT	  |
	//	+----------------------+----------------------+

	int packet_head;

	if (data_link_type == 0) { // Loopback.
		packet_head = 4;
		if (!loopback_capture) return; // If we dont capture loopback packets, return.
	}
	else if (data_link_type == 1) { // Ethernet.
		packet_head = 14;
	}
	else {
		spdlog::info("Data link type unknown: {}", data_link_type);
		return;
	}

	int ip_header_len;
	string src_addr, dst_addr;
	src_addr = "";
	dst_addr = "";
	// Used to assert protocol ID, we are only interested in UDP and TCP.
	u_char protocol;
	// Used to parse ports, B means begin and E means end (of a 16bits value (u_char
	// are on 8 bits)).

	const u_char ip_version_byte = packet[packet_head];
	const int ip_version = ((ip_version_byte & 0xF0) >> 4);

	if (ip_version == 4) {
		u_char* bytes = const_cast<u_char*>(packet) + (packet_head + 12);
		src_addr = inet_ntoa(*reinterpret_cast<in_addr*>(bytes));

		bytes = const_cast<u_char*>(packet) + (packet_head + 16);
		dst_addr = inet_ntoa(*reinterpret_cast<in_addr*>(bytes));

		protocol = packet[packet_head + 9];
		// Value of the byte could be 0101 = 5, which means 5 times 4 bits.
		ip_header_len = (packet[packet_head] & 0x0F) * 4;
	} else if (ip_version == 6) {
		char src_addr_buffer[100];
		const u_char* bytes = const_cast<u_char*>(packet) + (packet_head + 8);
		inet_ntop(AF_INET6, bytes,
			(PSTR)src_addr_buffer, 100);

		char dst_addr_buffer[100];
		bytes = const_cast<u_char*>(packet) + (packet_head + 24);
		inet_ntop(AF_INET6, bytes,
			(PSTR)dst_addr_buffer, 100);

		src_addr = src_addr_buffer;
		dst_addr = dst_addr_buffer;


 		protocol = packet[packet_head + 6];
		ip_header_len = 40; // IPV6 header has a fixed size.
	} else {
		return; // unknown version.
	}

	if (protocol != 0x06 && protocol != 0x11) { // 0x06 for TCP 0x11 for UDP.
		return; // unknown protocol.
	}

	packet_head += ip_header_len;

	// Instead of using two variables, maybe directly using 
	// UINT16 port = (UINT16)packet[packet_head]; would work, but that is less clear.

	u_char B = packet[packet_head];
	u_char E = packet[packet_head + 1];

	// Port is on 16 bits, and we have 8 bits in B and 8 bits in E so we need to
	// shift B to the left of 8 and or the result with E to get the full BE.

	const UINT16 src_port = B << 8 | E;

	packet_head += 2;

	B = packet[packet_head];
	E = packet[packet_head + 1];

	const UINT16 dst_port = B << 8 | E;

	// Now we have the source and the destination port.
	// Next step is to increment packets for these ports if we are in loopback,
	// or to find which port is the local port and to increment only this one
	// (meaning this packets is either going to the internet, or coming from it).

	const u_int packet_length = header->len;

	if (data_link_type == 0) {
		increment_packet_sent_weight(src_port, packet_length);
		increment_packet_received_weight(dst_port, packet_length);
	}
	else { // Already filtered unknown data link types
		if (addresses.find(src_addr) != string::npos) {
			increment_packet_sent_weight(src_port, packet_length);
		}
		else if (addresses.find(dst_addr) != string::npos) {
			increment_packet_received_weight(dst_port, packet_length);
		}
	}
}

// https://gist.github.com/jkomyno/45bee6e79451453c7bbdc22d033a282e
char* get_ip_str(const sockaddr* sa, char* s, const size_t maxlen) {
	switch (sa->sa_family) {
	case AF_INET:
		inet_ntop(AF_INET, &((sockaddr_in*)sa)->sin_addr,
			s, maxlen);
		break;

	case AF_INET6:
		inet_ntop(AF_INET6, &(((sockaddr_in6*)sa)->sin6_addr),
			s, maxlen);
		break;

	default:
		strncpy(s, "Unknown AF", maxlen);
		return nullptr;
	}

	return s;
}

/*
* Returns space separated string with every addresses linked to this device (returns "" if no
* addresses found)
*/
char* extract_addresses(const pcap_if_t* device) {
	const pcap_addr* tmp_a = device->addresses;
	const auto addresses = new char[1000]; // passed to packet sniffer method
	addresses[0] = '\0'; // init str to ''
	const auto str_ip = new char[100];
	while (tmp_a != nullptr) {
		get_ip_str(tmp_a->addr, str_ip, 100);

		strcat(addresses, str_ip);
		strcat(addresses, " ");
		tmp_a = tmp_a->next;
	}

	delete[] str_ip;
	return addresses;
}

/* 
 * Tries to start the capturing loop.
 * This method is to be called in a thread, or else your thread will be blocked.
 */
DWORD WINAPI start_packet_sniffing_loop(const LPVOID passed_thread_parameters) {
	const auto data = static_cast<pthread_data>(passed_thread_parameters);
	pcap_t* handle = data->handle;
	const pcap_if_t* dev = data->dev;
	char* device_description = dev->description;

	spdlog::info("Starting sniffing loop on %s", device_description);

	// Copy DevDesc
	const SIZE_T device_description_length = strlen(device_description);
	const auto device_description_copy = static_cast<char*>(malloc(device_description_length * sizeof(char) + 1));

	if (device_description_copy == nullptr) {
		spdlog::info("Could not allocate memory for copy.");
		return 0;
	}

	device_description_copy[0] = '\0';

	strcpy(device_description_copy, device_description);

	// Get addresses (if any)
	char* addresses = extract_addresses(dev);

	// Get data link type
	const int link_type = pcap_datalink(handle);

	// Start loop

	const auto p_loop_data = static_cast<ploop_data>(malloc(sizeof(loop_data)));

	if (p_loop_data == nullptr) {
		spdlog::info("Could not allocate memory for pLoop_data");
		return 0;
	}

	p_loop_data->addresses = addresses;
	p_loop_data->linkType = link_type;

	pcap_loop(handle, -1, got_packet, reinterpret_cast<u_char*>(p_loop_data));
	return 1;
}

/*
* Will try to start a capturing loop over every devices, in order to capture every single packet,
* even local loop.
*/
int start_packet_sniffing() {
	// The device to sniff on.
	char errbuf[PCAP_ERRBUF_SIZE];// Error string.
	pcap_if_t* alldevsp;
	pcap_findalldevs(&alldevsp, errbuf);
	pcap_if_t* sniffed_device = alldevsp;

	int amount_of_devs = 0;

	pcap_if_t* dev_tmp = sniffed_device;
	while (dev_tmp != nullptr) {
		amount_of_devs++;
		dev_tmp = dev_tmp->next;
	}

	/*STATIC*/ opened_handles = new pcap_t * [amount_of_devs];

	dev_tmp = sniffed_device;
	while (dev_tmp != nullptr) {
		spdlog::info("------- %s -------", dev_tmp->description);

		if ((dev_tmp->flags & PCAP_IF_LOOPBACK)
			|| (
				(dev_tmp->flags & PCAP_IF_UP)
				&& (dev_tmp->flags & PCAP_IF_CONNECTION_STATUS_CONNECTED)
				&& (dev_tmp->flags & PCAP_IF_RUNNING)
			)) {

			pcap_t* opened_handle = sniff_device(dev_tmp);
			if (opened_handle != nullptr) {
				spdlog::info("Opened %s", dev_tmp->description);
				opened_handles[opened_handles_count] = opened_handle;
				opened_handles_count++;
			} else {
				spdlog::info("Could not open %s", dev_tmp->description);
			}
		}
		dev_tmp = dev_tmp->next;
	}

	// pcap_freealldevs(sniffed_device);

	if (amount_of_devs == 0) {
		spdlog::info("Could not find interface");
		return 2;
	}
	return 1;
}

/*Tries to start the capturing loop on the device device.*/
pcap_t* sniff_device(pcap_if_t* device) {
	// Session handle.
	char errbuf[PCAP_ERRBUF_SIZE]; // Error string.

	const auto data = static_cast<pthread_data>(malloc(sizeof(thread_data)));

	if (data == nullptr) {
		spdlog::info("Could not allocate memory for data.");
		return nullptr;
	}

	const char* device_name = device->name;
	pcap_t* handle = pcap_open(
		device_name, // Name of the device.
		65536, // Portion of the packet to capture.
		// 65536 guarantees that the whole packet will be captured on all the link layers.
		0, // Non promiscuous mode.
		1000, // Read timeout.
		nullptr, // Authentication on the remote machine.
		errbuf // Error buffer.
	);

	if (handle == nullptr) {
		spdlog::info("Couldn't open device %s: %s", device_name, errbuf);
		return nullptr;
	}

	data->handle = handle;
	data->dev = device;

	const LPDWORD packet_sniffer_handle_identifier = nullptr;
	CreateThread(
		nullptr, // Default security attributes
		0, // Use default stack size  
		start_packet_sniffing_loop, // Thread function name
		data, // Argument to thread function 
		0, // Use default creation flags 
		packet_sniffer_handle_identifier
	); // Returns the thread identifier

	return handle;
}