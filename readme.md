# Summary
The DMM is the master board of the DINFox project. It embeds the following features:
* **RS485** communication to monitor and control slaves on the bus.
* **HMI** for local monitoring and control.
* Analog **measurements** such as USB, RS485 bus, and HMI voltages.

# Hardware
The boards was designed on **Circuit Maker V2.0**. Hardware documentation and design files are available @ https://circuitmaker.com/Projects/Details/Ludovic-Lesur/DMMHW1-0

# Embedded software

## Environment
The embedded software was developed under **Eclipse IDE** version 2019-06 (4.12.0) and **GNU MCU** plugin. The `script` folder contains Eclipse run/debug configuration files and **JLink** scripts to flash the MCU.

## Target
The boards are based on the **STM32L081CBT6** of the STMicroelectronics L0 family microcontrollers. Each hardware revision has a corresponding **build configuration** in the Eclipse project, which sets up the code for the selected target.

## Structure
The project is organized as follow:
* `inc` and `src`: **source code** split in 6 layers:
    * `registers`: MCU **registers** adress definition.
    * `peripherals`: internal MCU **peripherals** drivers.
    * `utils`: **utility** functions.
    * `components`: external **components** drivers.
    * `nodes`: **Node interfaces** layer.
    * `applicative`: high-level **application** layers.
* `startup`: MCU **startup** code (from ARM).
* `linker`: MCU **linker** script (from ARM).
