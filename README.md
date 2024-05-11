# Alarming System for LPC1768 using FreeRTOS
This project is an alarming system built for LPC1768 using FreeRTOS and developed using KeilStudioCloud IDE. It provides the following functionalities:

- Sensor monitoring service (working in a periodic way and on demand, and with the capability of storing collected data and with the possibility of handling alarm situations).
- Clock service (to provide user meaningful time (wall clock) and to allow clock alarm management).
- Processing of information.
- Device actuation (to be handled by a resource manager task or in a distributed way).
- User interface (console).

## Connecting to the board
You are free to use any terminal emulator of your choice. Despite that, here are some steps in order to connect to the board using Putty:
1. Select the device `Serial line` (on linux it is usually /dev/ttyACM0)
2. Match Baud in the `Speed` input box
3. Select Serial under `Connection Type`
4. (Optional) Go to `Terminal` settings and check `Force on` in the `Local echo`, this will allow you to verify your input integrity

### Command Line Functionalities

- `rc`: Read clock
- `sc h m s`: Set clock
- `rtl`: Read temperature and luminosity
- `rp`: Read parameters (pmon, tala, pproc)
- `mmp p`: Modify monitoring period (seconds - 0 deactivate)
- `mta t`: Modify time alarm (seconds)
- `mpp p`: Modify processing period (seconds - 0 deactivate)
- `rai`: Read alarm info (clock, temperature, luminosity, active/inactive-A/a)
- `dac h m s`: Define alarm clock
- `dtl T L`: Define alarm temperature and luminosity
- `aa a`: Activate/deactivate alarms (A/a)
- `cai`: Clear alarm info (letters CTL in LCD)
- `ir`: Information about records (NR, nr, wi, ri)
- `lr n i`: List n records from index i (0 - oldest)
- `dr`: Delete records
- `pr [h1 m1 s1 [h2 m2 s2]]`: Process records (max, min, mean) between instants t1 and t2 (h,m,s)

### FreeRTOS Documentation

For any information regarding FreeRTOS, please refer to their documentation: [FreeRTOS Documentation](https://www.freertos.org/)

## Requirements

- LPC1768 microcontroller
- FreeRTOS
- KeilStudioCloud IDE

## Installation and Usage

1. Clone this repository to your local machine.
2. Open the project in KeilStudioCloud IDE.
3. Build and flash the project to your LPC1768 microcontroller.
4. Connect to the microcontroller using a terminal emulator.
5. Use the provided command line functionalities to interact with the alarming system.
