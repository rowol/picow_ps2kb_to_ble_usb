# Modified CMakeLists.txt, copied from $/ble_usb_hid.git and modified

set(PROJECT ps2_ble_kb)
cmake_minimum_required(VERSION 3.13)

# Uncomment next line for debug version
set(CMAKE_BUILD_TYPE "Debug")


if (NOT DEFINED ENV{PICO_SDK_PATH})
	message(FATAL_ERROR "Must set PICO_SDK_PATH environment variable")
endif()
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})

if (NOT DEFINED PICO_BOARD)
	set(PICO_BOARD pico_w)
endif()

include (${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(${PROJECT} C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT})

add_subdirectory(ps2kbd-lib)
add_subdirectory(ble_device)


# print memory usage, enable all warnings
target_compile_options(${PROJECT} PRIVATE ) #-Wall -Wextra
target_link_options(${PROJECT} PRIVATE -Xlinker --print-memory-usage)

# Add current directory to include path 
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${PROJECT} PRIVATE
	pico_stdlib
	ps2kbd-lib
)

set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME "${PROJECT}")

pico_add_extra_outputs(${PROJECT})       # Creates UF2 file

