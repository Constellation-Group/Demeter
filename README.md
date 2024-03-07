## Presentation

DEMETER stands for Desktop Energy METER. This software aims to fill the gap in probing softwares for Windows desktops.
Indeed, the Windows probing softwares landscape is lacking one that can finely give energy consumed by the components of the computer, and per running process.
This program is a command line program or a process that can run in the background. It will log in a CSV file the consumption of each component for each running process.
The structure of the CSV is explained below. The conversion values can be found in the code (file Demeter.cpp) and below.

## Usage

To start Demeter, you simply need to launch the executable from your explorer.
If you want to tune Demeter, start demeter through the terminal with the -h flag like so:
`./Demeter.exe -h`
Help will be printed and you will be able to tune Demeter.

## Build

Requirements:
 - Scaphandre: https://github.com/hubblo-org/windows-rapl-driver
 - Npcap: https://npcap.com/#download
 - spdlog (install via vcpkg): https://github.com/gabime/spdlog
 - cxxopts (install via vcpkg): https://github.com/jarro2783/cxxopts

To build DEMETER you simply have to open the project in Visual Studio and build (Ctrl+B) in Release configuration.
The path to the generated binary should be in `PROJECT_ROOT\x64\Release\DEMETER.exe`.

## Architecture

The architecture does not exploit OOP. It is instead rather simple with method calls and small methods.
Files description:
| Filename               | Description                                                                                                                                |
|------------------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| CPUDataGatherer        | This file is used to get for any process it's CPU usage.                                                                                   |
| Demeter                | This is the main file.                                                                                                                     |
| DemeterLogger          | This file starts the logger.                                                                                                               |
| DiskDataGatherer       | This file is used to get for any process it's disk usage.                                                                                  |
| EnergyGatherer         | This file is used to get the energy data from Scaphandre.                                                                                  |
| ProcessInfoGatherer    | This file is used to know if a process is a service.                                                                                       |
| ProcessNetDataGatherer | This file handles the network traffic and gives the bandwidth usage for every port and process.                                            |
| RAMDataGatherer        | This file is used to get for any process it's RAM usage.                                                                                   |
| SystemInfoGatherer     | This file is used to get informations about the system, such as the name of the user.                                                      |
| UsageWatchdog          | This file describes the watchdog and it's behavior.                                                                                        |
| UsageWatchdogManager   | This file gives the watchdog data about Demeter's consumption and is used to tell if the watchdog is fully operational and under lockdown. |

## Output file

The output file is a CSV file created when Demeter starts.
Fields description:
| LABEL    | DESCRIPTION                                   | UNIT         |
|----------|-----------------------------------------------|--------------|
| TIME     | Record timestamp                              | Second       |
| NAME     | Process's filename                            | N/A          |
| CPU      | CPU usage                                     | % CPU        |
| CPUC     | Energy consumed                               | mWh          |
| NETUP    | Upstream bandwidth usage                      | Mo/s         |
| NETUPC   | Energy consumed by upstream bandwidth usage   | mWh          |
| NETDOWN  | Downstream bandwidth usage                    | Mo/s         |
| NETDOWNC | Energy consumed by downstream bandwidth usage | mWh          |
| DISKR    | Disk read speed                               | Mo/s         |
| DISKW    | Disk write speed                              | Mo/s         |
| DISKRC   | Energy consumed by disk read                  | mWh          |
| DISKWC   | Energy consumed by disk write                 | mWh          |
| RAM      | Private usage                                 | Byte         |
| SUMC     | Sum of energy consumption                     | mWh          |

If an output file for this day already existed when Demeter started, the line `----RESTARTLINE----\n` will be appended to the file to let the user know that this line is eventually incomplete.

## Conversion values

To compute some of our fields we use some conversion values.
Conversion values per field:
| Field      | Conversion formula | Variables                                     |
|------------|--------------------|-----------------------------------------------|
| Network    | B*0.068            | B stands for the Bandwidth in MB/s            |
| Disk Read  | DR*0.78            | DR stands for the Disk Read bitrate in MB/s   |
| Disk Write | DW*0.98            | DW stands for the Disk Write biterate in MB/s |

## Cite this work

To cite our work in a research paper, please cite our paper in the 17th European Conference on Software Architecture (ECSA 2023).

- **Demeter: An Architecture for Long-Term Monitoring of Software Power Consumption**. Lylian Siffre, Gabriel Breuil, Adel Noureddine, and Renaud Pawlak. In the 17th European Conference on Software Architecture (ECSA 2023). Istanbul, Turkey, September 2023.

```
@inproceedings{demeter-ecsa-2023,
  title = {Demeter: An Architecture for Long-Term Monitoring of Software Power Consumption},
  author = {Siffre, Lylian and Breuil, Gabriel and Noureddine, Adel and Pawlak, Renaud},
  booktitle = {17th European Conference on Software Architecture (ECSA 2023)},
  address = {Istanbul, Turkey},
  year = {2023},
  month = {Se},
  keywords = {Power Monitoring; Measurement; Energy Consumption; Long-term Monitoring; Distributed Architecture; Software Engineering}
}
```

## License

Demeter is licensed under the GNU GPL 3 license only (GPL-3.0-only).

Copyright (c) 2022-2024, Constellation.
All rights reserved. This program and the accompanying materials are made available under the terms of the GNU General Public License v3.0 only (GPL-3.0-only) which accompanies this distribution, and is available at: https://www.gnu.org/licenses/gpl-3.0.en.html