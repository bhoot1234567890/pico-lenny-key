cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(lenny_keyboard)
set(PICO_BOARD pico)

pico_sdk_init()

# Debug version with CDC serial output
add_executable(lenny_debug lenny_debug.c)
target_include_directories(lenny_debug PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(lenny_debug
    pico_stdlib
    tinyusb_device
    tinyusb_board
    hardware_gpio
)
pico_enable_stdio_usb(lenny_debug 0)
pico_enable_stdio_uart(lenny_debug 0)
pico_add_extra_outputs(lenny_debug)
