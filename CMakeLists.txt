#
# CMakeLists.txt
#
#  Created on: 03 mar. 2026
#      Author: Ludo
#

# Minimum CMake version.
cmake_minimum_required(VERSION 3.23)

# Project creation.
project(dinfox-dmm)
add_executable(${PROJECT_NAME})

# Use object build mode in all sub-directories.
set(BUILD_MODE "OBJECT")

# Macro to convert options to variables.
macro(add_compilation_flag FLAG_NAME FLAG_DESCRIPTION FLAG_DEFAULT_VALUE)
    option(FLAG_NAME FLAG_DESCRIPTION FLAG_DEFAULT_VALUE)
    set(${FLAG_NAME} ${FLAG_DEFAULT_VALUE} CACHE STRING ${FLAG_DESCRIPTION})
    list(APPEND COMPILATION_FLAGS_LIST ${FLAG_NAME})
endmacro()

# Hardware compilation flags.
set(DMM_HW_VERSION "NONE" CACHE STRING "Hardware version")

# Software compilation flags.
add_compilation_flag(DMM_NVM_FACTORY_RESET "Enable NVM factory reset." OFF)
add_compilation_flag(DMM_NODE_SCAN_PERIOD_SECONDS "Automatic node scanning period in seconds." 86400)
add_compilation_flag(DMM_SIGFOX_UL_PERIOD_SECONDS "Sigfox uplink period in seconds." 300)
add_compilation_flag(DMM_SIGFOX_DL_PERIOD_SECONDS "Sigfox downlink period in seconds" 21600)

# Hardware specific settings.
# DMM HW1.0.
if(DMM_HW_VERSION STREQUAL "HW1_0")   
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m0plus -O0")
    set(DMM_MCU "stm32l0xx")
    set(PROJECT_LINKER_SCRIPT "stm32l081xbxx.ld")
# Unknown version.
else()
    message(FATAL_ERROR "Invalid hardware version.")
endif()
set(PROJECT_LINKER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/drivers/device/${DMM_MCU}-device/linker")

# Add hardware compilation flags.
add_compile_definitions(
    __DMM_FLAGS_H__
    ${DMM_HW_VERSION}
)

# Add software compilation flags.
foreach(FLAG ${COMPILATION_FLAGS_LIST})
    # Macros only defined.
    if(${FLAG} STREQUAL ON)
        add_compile_definitions(${FLAG})
    endif()
    # Macros with value.
    if((NOT ${FLAG} STREQUAL OFF) AND (NOT ${FLAG} STREQUAL ON))
        set(${FLAG} "(${${FLAG}})")
        add_compile_definitions(${FLAG}=${${FLAG}}) 
    endif()
endforeach()

# Add project sources files.
target_sources(${PROJECT_NAME}
    PRIVATE
        drivers/peripherals/src/mcu_mapping.c
        drivers/components/src/led.c
        drivers/components/src/r4s8cr_hw.c
        drivers/components/src/sh1106_hw.c
        drivers/mac/src/lmac_hw.c
        drivers/utils/src/logo.c
        drivers/utils/src/terminal_hw.c
        middleware/analog/src/analog.c
        middleware/hmi/src/hmi_bcm.c
        middleware/hmi/src/hmi_bpsm.c
        middleware/hmi/src/hmi_ddrm.c
        middleware/hmi/src/hmi_dmm.c
        middleware/hmi/src/hmi_gpsm.c
        middleware/hmi/src/hmi_lvrm.c
        middleware/hmi/src/hmi_mpmcm.c
        middleware/hmi/src/hmi_node.c
        middleware/hmi/src/hmi_r4s8cr.c
        middleware/hmi/src/hmi_sm.c
        middleware/hmi/src/hmi_uhfm.c
        middleware/hmi/src/hmi.c
        middleware/node/src/node.c
        middleware/node/src/una_at_hw.c
        middleware/node/src/una_dmm.c
        middleware/power/src/power.c
        middleware/radio/src/radio_bcm.c
        middleware/radio/src/radio_bpsm.c
        middleware/radio/src/radio_common.c
        middleware/radio/src/radio_ddrm.c
        middleware/radio/src/radio_dmm.c
        middleware/radio/src/radio_gpsm.c
        middleware/radio/src/radio_lvrm.c
        middleware/radio/src/radio_mpmcm.c
        middleware/radio/src/radio_r4s8cr.c
        middleware/radio/src/radio_sm.c
        middleware/radio/src/radio_uhfm.c
        middleware/radio/src/radio.c
        application/src/main.c
)

# Project include folders.
include_directories(${PROJECT_NAME}
    PRIVATE
        drivers/device/inc
        drivers/registers/inc
        drivers/registers/${DMM_MCU}-registers/inc
        drivers/peripherals/inc
        drivers/peripherals/${DMM_MCU}-drivers/inc
        drivers/utils/inc
        drivers/utils/embedded-utils/inc
        drivers/components/inc
        drivers/components/r4s8cr-driver/inc
        drivers/components/sh1106-driver/inc
        drivers/mac/inc
        drivers/mac/lmac-driver/inc
        middleware/analog/inc
        middleware/hmi/inc
        middleware/node/inc
        middleware/node/dinfox-registers/inc
        middleware/node/una-at/inc
        middleware/node/una-lib/inc
        middleware/node/una-r4s8cr/inc
        middleware/power/inc
        middleware/radio/inc
        application/inc
)

# Add submodules sources files.
add_subdirectory(drivers/device/${DMM_MCU}-device EXCLUDE_FROM_ALL)
add_subdirectory(drivers/peripherals/${DMM_MCU}-drivers EXCLUDE_FROM_ALL)
add_subdirectory(drivers/utils/embedded-utils EXCLUDE_FROM_ALL)
add_subdirectory(drivers/components/r4s8cr-driver EXCLUDE_FROM_ALL)
add_subdirectory(drivers/components/sh1106-driver EXCLUDE_FROM_ALL)
add_subdirectory(drivers/mac/lmac-driver EXCLUDE_FROM_ALL)
add_subdirectory(middleware/node/dinfox-registers EXCLUDE_FROM_ALL)
add_subdirectory(middleware/node/una-at EXCLUDE_FROM_ALL)
add_subdirectory(middleware/node/una-lib EXCLUDE_FROM_ALL)
add_subdirectory(middleware/node/una-r4s8cr EXCLUDE_FROM_ALL)

# Link libraries.
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${DMM_MCU}-device
        ${DMM_MCU}-drivers
        embedded-utils
        r4s8cr-driver
        sh1106-driver
        lmac-driver
        dinfox-registers
        una-at
        una-lib
        una-r4s8cr
        c_nano
        nosys
        gcc
)

# Linker and artifact.
include(script/cmake-arm-none-eabi/linker.cmake)
include(script/cmake-arm-none-eabi/artifact.cmake)
