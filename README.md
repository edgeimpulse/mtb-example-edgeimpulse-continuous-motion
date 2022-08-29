# Firmware Infineon/Cypress PSoC6 Internal

Edge Impulse enables developers to create the next generation of intelligent device solutions with embedded machine learning. This repository contains the Edge Impulse firmware for Infineon/Cypress PSoC62 43012 Pioneer Kit with IoT SENSE. This device supports all of Edge Impulse's device features, including ingestion, remote management and inferencing.

## Introduction

This project supports:
* Data ingestion using the [EdgeImpulse Studio](https://studio.edgeimpulse.com)
* Sensor Fusion for Inertial and Environment sensor
* Live inference, including of a Sensor Fusion model
* Data sampling from the BMX160 IMU and DPS310 Barometer+Temperature sensor on the IoT SENSE board
* Storing samples on the external NOR Flash using the QSPI inteface

Project dependencies (all libraries are provided in this code repository):
* PSoC6 io_retarget library for debug UART functionality
* PSoC6 serial-flash library for accessing NOR Flash over QSPI interface
* PSoC6 SPI library for hardware-driven Master SPI interface needed by the BMX160 driver
* BMX160 driver with Bosch BMI160 and BMM150 library for IMU functionality
* Fix for BMX160 CHIPID (see more in Troubleshooting)
* Buffix for DPS310 configuration

## Requirements

### Software
- Install ModusToolbox SDK and IDE
- Toolchain in the SDK is GNU Arm® embedded compiler v9.3.1

### Hardware

- Development board: [PSoC 62S2 Wi-Fi Bluetooth pioneer kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s2-43012/) (`CY8CKIT-062S2-43012`)
- Evaluation board: [Infineon IoT Sense kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-028-sense/) (`CY8CKIT-028-SENSE`)

## Building

### ModusToolbox IDE

### Comammand Line

1. Install [ModusToolbox](https://www.raspberrypi.com/products/compute-module-4-io-board/)
1. Clone this repository.
1. Open terminal and go to the directory with cloned project
1. Run the following commands

    ```
    make getlibs
    make build
    ```

### Docker

1. Build docker image

    ```
    docker build -t edge-impulse-infineon .
    ```

1. Build firmware

    ```
    docker run --rm -v $PWD:/app edge-impulse-infineon
    ```

## Flashing

### ModusToolbox IDE

Infineon provides extensive documentation, with screenshots, about how to use the ModusToolbox IDE. Topics covered include:
- Importing a project
- Building the project
- Flashing the project to the board

Please visit this link for the [Infineon ModusToolbox IDE guide](https://www.infineon.com/dgdl/Infineon-Eclipse_IDE_for_ModusToolbox_User_Guide_1-UserManual-v01_00-EN.pdf?fileId=8ac78c8c7d718a49017d99bcb86331e8)

### Command Line

1. After building the firmware (see steps above) connect the board and run

    ```
    make program
    ```

### Standalone

1. Install [CyProgrammer](https://softwaretools.infineon.com/tools/com.ifx.tb.tool.cypressprogrammer)
1. Connect the board and run `CyProgrammer`
1. Select a probe/kit

    ![](docs/flash1.png)

    ![](docs/flash2.png)

    ![](docs/flash3.png)

1. Select compiled hex file with compiled firmware

    ![](docs/flash4.png)

1. Connect to the board

    ![](docs/flash5.png)

1. Program the firmware

    ![](docs/flash6.png)

1. After successful flashing, you should see a LED blinking patter

    ![](docs/flash6.png)

    ![](docs/blink.gif)


## Troubleshooting / Known issues

### BMX160 issues

This project includes a modified version of the BMI160 library in order to modify the expect CHIPID from 0xD1(BMI160) to the correct 0xD8(BMX160). In case ModusToolbox updates the BMI160 library, please make sure to check the BMI160_CHIP_ID definition in `bmi160_defs.h` file.

The project also addresses the following BMX160 specifics for out-of-the-box experience:
- BMX160 needs a rising edge on Chip Select early after power-on.
- Do not assign Chip select pin to the SPI driver, instead assign it to the BMX160 driver.
- Do not work at the recommended 10MHz SPI or expect scrambled bits, 8MHz provides stable output.
