# MSX PICOVERSE PROJECT
# (c) 2026 Cristiano Goncalves
# The Retro Hacker
#
# CMakeLists.txt - Raspberry Pico 2 (RP2350B) firmware of the PicoVerse 2350 USB FDD interface

# Set the minimum CMake version
cmake_minimum_required(VERSION 3.13)

# Set the C and C++ standards
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
# Core2350B uses an RP2350B (48 GPIOs). The 'pico2' board is RP2350A (30 GPIOs only)
# and ignores GPIO30..47. Use an RP2350B board definition so NUM_BANK0_GPIOS is 48.
set(PICO_BOARD pimoroni_pga2350 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(msxusbfdd C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(msxusbfdd
    ${PICO_SDK_PATH}/lib/tinyusb/src/tusb.c
    msxusbfdd.c
    usb_host.c
    fs_service.c
    diskio_usb.c
    fatfs/ff.c )

pico_generate_pio_header(msxusbfdd ${CMAKE_CURRENT_LIST_DIR}/msx_bus.pio)

pico_set_program_name(msxusbfdd "msxusbfdd")
pico_set_program_version(msxusbfdd "1.01")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(msxusbfdd 0)
pico_enable_stdio_usb(msxusbfdd 0)

# Add the standard library to the build
target_link_libraries(msxusbfdd
    pico_stdlib
    pico_multicore
    hardware_pio
    tinyusb_board
    tinyusb_host
    )

# Add the standard include files to the build
target_include_directories(msxusbfdd PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/fatfs
  ${PICO_SDK_PATH}/lib/tinyusb/src
)

target_link_options(msxusbfdd PRIVATE
    -Wl,--wrap=hw_endpoint_reset_transfer
    -Wl,--wrap=hcd_edpt_open
    -Wl,--wrap=hcd_setup_send
    -Wl,--wrap=hcd_edpt_xfer
    -Wl,--wrap=hcd_edpt_abort_xfer
    -Wl,--wrap=hcd_event_handler
    -Wl,--wrap=hcd_port_reset
    -Wl,--wrap=hcd_port_reset_end
)

pico_add_extra_outputs(msxusbfdd)

# Custom command to copy files to dist folder
add_custom_command(TARGET msxusbfdd POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/dist
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/msxusbfdd.bin ${CMAKE_SOURCE_DIR}/dist/msxusbfdd.bin
)
