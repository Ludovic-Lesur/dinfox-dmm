# Description

The **DMM** is the master board of the DINFox project. It embeds the following features:

* **RS485** communication to monitor and control slaves on the bus.
* **HMI** for local monitoring and control.
* Analog **measurements** such as USB, RS485 bus, and HMI voltages.

# Hardware

The board was designed on **Circuit Maker V2.0**. Below is the list of hardware revisions:

| Hardware revision | Description | `cmake_hw_version` | Status |
|:---:|:---:|:---:|:---:|
| [DMM HW1.0](https://365.altium.com/files/ED83B6F3-90FC-4C58-A588-77DC635C6A63) | Initial version. | `HW1_0` | :white_check_mark: |

# Embedded software

## Environment

The embedded software is developed under **Eclipse IDE** version 2024-09 (4.33.0) and **GNU MCU** plugin. The `script` folder contains Eclipse run/debug configuration files and **JLink** scripts to flash the MCU.

> [!WARNING]
> To compile any version under `sw5.0`, the `git_version.sh` script must be patched when `sscanf` function is called: the `SW` prefix must be replaced by `sw` since Git tags have been renamed in this way.

## Target

The board is based on the **STM32L081CBT6** microcontroller of the STMicroelectronics L0 family. Each hardware revision has a corresponding **build configuration** in the Eclipse project, which sets up the code for the selected board version.

## Architecture

<p align="center">
<img src="https://github.com/Ludovic-Lesur/dinfox-doc/blob/master/images/dmm-sw-architecture.drawio.png" width="600"/>
</p>

## Structure

The project is organized as follow:

* `drivers` :
    * `device` : MCU **startup** code and **linker** script.
    * `registers` : MCU **registers** address definition.
    * `peripherals` : internal MCU **peripherals** drivers.
    * `mac` : **medium access control** driver.
    * `components` : external **components** drivers.
    * `utils` : **utility** functions.
* `middleware` :
    * `analog` : High level **analog measurements** driver.
    * `hmi` : Nodes access through HMI.
    * `node` : **UNA** nodes interface implementation.
    * `power` : Board **power tree** manager.
    * `radio` : Nodes access through radio.
* `application` : Main **application**.

## Build

The project can be compiled by command line with `cmake`.

```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="script/cmake-arm-none-eabi/toolchain.cmake" \
      -DTOOLCHAIN_PATH="<arm_none_eabi_gcc_path>" \
      -DDMM_HW_VERSION="<cmake_hw_version>" \
      -DDMM_NVM_FACTORY_RESET=OFF \
      -DDMM_NODE_SCAN_PERIOD_SECONDS=86400 \
      -DDMM_SIGFOX_UL_PERIOD_SECONDS=300 \
      -DDMM_SIGFOX_DL_PERIOD_SECONDS=21600 \
      -G "Unix Makefiles" ..
make all
```
