# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
set(PICO_BOARD waveshare_rp2350_zero CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(media-controller C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 1.0

add_executable(media-controller
    src/main.cpp
    src/debounced_input.cpp
    src/latching_button.cpp
    src/media_hid.cpp
    src/quadrature_input.cpp
    src/rotation_sensor.cpp
)

pico_set_program_name(media-controller "media-controller")
pico_set_program_version(media-controller "1.0")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(media-controller 0)
pico_enable_stdio_usb(media-controller 0)

# Add the standard library to the build
target_link_libraries(media-controller
    pico_unique_id
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

# Add the standard include files to the build
target_include_directories(media-controller PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_options(media-controller PRIVATE -Wall -Wextra)

# -Wconversion only on our own sources: third-party conversion warnings are
# reported against code we cannot fix, and an SDK bump would dirty the build.
set_source_files_properties(
    src/main.cpp
    src/debounced_input.cpp
    src/latching_button.cpp
    src/media_hid.cpp
    src/quadrature_input.cpp
    src/rotation_sensor.cpp
    PROPERTIES COMPILE_OPTIONS "-Wconversion"
)

pico_add_extra_outputs(media-controller)

