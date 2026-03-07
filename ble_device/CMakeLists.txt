#
# These are the specific top level files and build additions for a BLE connected keyboard
# Note: CMake variables set in the parent dir are still in effect, etc
#

target_sources(${PROJECT} PRIVATE
  main.c
)

target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${PROJECT} PRIVATE
  pico_stdlib
  pico_btstack_ble
  pico_btstack_cyw43
  pico_cyw43_arch_none
)

pico_btstack_make_gatt_header(${PROJECT} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/hog_keyboard_demo.gatt")


