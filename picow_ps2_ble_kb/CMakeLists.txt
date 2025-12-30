# Modified CMakeLists.txt, copied from $/ble_usb_hid.git and modified

set(PROJECT hog_keyboard_demo)
cmake_minimum_required(VERSION 3.13)

# Uncomment next line if building for use with Microsoft Speedbump keyboard (i.e. Natural Keyboard Elite)
add_compile_definitions(MSFT_SPEEDBUMP_KB)


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

add_subdirectory(ps2kbd-lib)

add_executable(${PROJECT})
target_sources(${PROJECT} PRIVATE
	hog_keyboard_demo.c
	picow_bt_example_common.c
	picow_bt_example_background.c
)

# print memory usage, enable all warnings
target_compile_options(${PROJECT} PRIVATE ) #-Wall -Wextra
target_link_options(${PROJECT} PRIVATE -Xlinker --print-memory-usage)

# Add current directory to include path 
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${PROJECT} PRIVATE
	pico_btstack_ble
	pico_btstack_cyw43
	pico_cyw43_arch_none
	pico_stdlib
	ps2kbd-lib
)

pico_btstack_make_gatt_header(hog_keyboard_demo PRIVATE "${CMAKE_CURRENT_LIST_DIR}/hog_keyboard_demo.gatt")

set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME "${PROJECT}")

pico_add_extra_outputs(${PROJECT})       # Creates UF2 file

