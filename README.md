# Demeter

Demeter (Desktop Energy METER) is a command-line tool for monitoring the energy consumption of Windows desktop components and processes. It logs detailed consumption metrics in a CSV file, enabling in-depth analysis of energy usage.

## Features

- Monitors CPU, RAM, disk, and network usage of all running applications
- Continously logs energy consumption per process
- Generates CSV files with detailed metrics
- Runs as a background process

## Requirements

- [Scaphandre](https://github.com/hubblo-org/windows-rapl-driver)
- [Npcap](https://npcap.com/#download)
- [spdlog](https://github.com/gabime/spdlog) (via vcpkg)
- [cxxopts](https://github.com/jarro2783/cxxopts) (via vcpkg)

## Installation

1. Clone the repository:
    ```sh
    git clone https://github.com/Constellation-Group/Demeter.git
    cd Demeter
    ```

2. Install dependencies:
    ```sh
    vcpkg install spdlog cxxopts
    ```

3. Open the project in Visual Studio and build in Release configuration (Ctrl+B). The binary will be located at `PROJECT_ROOT\x64\Release\DEMETER.exe`.

## Usage

To start Demeter, simply run the executable:
```sh
./Demeter.exe
```

For configuration options, run:
```sh
./Demeter.exe -h
```

## Output

Demeter generates a CSV file with the following columns:

- **TIME**: Timestamp (seconds)
- **NAME**: Process filename
- **CPU**: CPU usage (%)
- **CPUC**: CPU energy consumed (mWh)
- **NETUP**: Upstream bandwidth (MB/s)
- **NETUPC**: Upstream energy consumed (mWh)
- **NETDOWN**: Downstream bandwidth (MB/s)
- **NETDOWNC**: Downstream energy consumed (mWh)
- **DISKR**: Disk read speed (MB/s)
- **DISKW**: Disk write speed (MB/s)
- **DISKRC**: Disk read energy consumed (mWh)
- **DISKWC**: Disk write energy consumed (mWh)
- **RAM**: RAM usage (bytes)
- **SUMC**: Total energy consumption (mWh)

## Architecture

The project is organized into several key files:

- **CPUDataGatherer**: Gathers CPU usage per process
- **Demeter**: Main executable
- **DemeterLogger**: Handles logging
- **DiskDataGatherer**: Gathers disk usage per process
- **EnergyGatherer**: Retrieves energy data from Scaphandre
- **ProcessInfoGatherer**: Identifies if a process is a service
- **ProcessNetDataGatherer**: Monitors network traffic
- **RAMDataGatherer**: Gathers RAM usage per process
- **SystemInfoGatherer**: Collects system information
- **UsageWatchdog**: Describes watchdog behavior
- **UsageWatchdogManager**: Manages watchdog data and operational status

## Conversion Values

The following conversion formulas are used to compute energy consumption:

- **Network**: `B * 0.068` (B = Bandwidth in MB/s)
- **Disk Read**: `DR * 0.78` (DR = Disk Read bitrate in MB/s)
- **Disk Write**: `DW * 0.98` (DW = Disk Write bitrate in MB/s)

## Citation

If you use Demeter in your research, please cite our paper:

> Demeter: An Architecture for Long-Term Monitoring of Software Power Consumption. Lylian Siffre, Gabriel Breuil, Adel Noureddine, and Renaud Pawlak. In the 17th European Conference on Software Architecture (ECSA 2023). Istanbul, Turkey, September 2023.

BibTeX entry:
```bibtex
@inproceedings{demeter-ecsa-2023,
  title = {Demeter: An Architecture for Long-Term Monitoring of Software Power Consumption},
  author = {Siffre, Lylian and Breuil, Gabriel and Noureddine, Adel and Pawlak, Renaud},
  booktitle = {17th European Conference on Software Architecture (ECSA 2023)},
  address = {Istanbul, Turkey},
  year = {2023},
  month = {Sep},
  keywords = {Power Monitoring; Measurement; Energy Consumption; Long-term Monitoring; Distributed Architecture; Software Engineering}
}
```

## License

Demeter is licensed under the GNU GPL 3 license only (GPL-3.0-only).

Copyright (c) 2022-2024, Constellation, and University of Pau and Pays de l'Adour.
All rights reserved. This program and the accompanying materials are made available under the terms of the GNU General Public License v3.0 only (GPL-3.0-only) which accompanies this distribution, and is available at: https://www.gnu.org/licenses/gpl-3.0.en.html
