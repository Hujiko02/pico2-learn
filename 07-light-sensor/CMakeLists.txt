# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(07-light-sensor C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(07-light-sensor main.c )

pico_set_program_name(07-light-sensor "07-light-sensor")
pico_set_program_version(07-light-sensor "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(07-light-sensor 0)
pico_enable_stdio_usb(07-light-sensor 1)

# Add the standard library to the build
target_link_libraries(07-light-sensor
        pico_stdlib
        hardware_adc)   # ★ adc_init / adc_read 在这个库里，pico_stdlib 不包含它

# Add the standard include files to the build
target_include_directories(07-light-sensor PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(07-light-sensor)

