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

#define _WINSOCKAPI_
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <psapi.h>

#include <iostream>
#include <fstream>

#include <string>
#include <tchar.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

// Need to link with Iphlpapi.lib and Ws2_32.lib
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")


#include <Pdh.h>

#include "pcap.h"
#include <stdlib.h>
#include <map>

#include <stdio.h>

#include <mutex>

#include "DemeterLogger.h"

#include "UsageWatchdogManager.h"

using namespace std;

void set_loopback_capture(bool);

void reset_port_map(DWORD);

void alloc_port_map(DWORD);

void add_port_to_map(DWORD, u_short);

void map_ports_to_pid();

void get_connected_ports(DWORD, u_short**);

SIZE_T get_connected_ports_counts(DWORD);

DWORD get_packets(DWORD, std::map<u_short, DWORD>);

DWORD get_packet_sent_weight(DWORD);

DWORD get_packet_received_weight(DWORD);

void increment_packet_sent_weight(u_short, u_int);

void increment_packet_received_weight(u_short, u_int);

void reset_packet_bandwith();

static void got_packet(u_char*, const struct pcap_pkthdr*, const u_char*);

char* get_ip_str(const struct sockaddr*, char*, size_t);

DWORD WINAPI start_packet_sniffing_loop(LPVOID);

int start_packet_sniffing();

pcap_t** get_pcap_handle(SIZE_T*);

pcap_t* sniff_device(pcap_if_t*);