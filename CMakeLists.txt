cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(x68kzkbd-pico)

pico_sdk_init()

add_executable(x68kzkbd-pico)

target_sources(x68kzkbd-pico PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/hid_app.c
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        )

target_include_directories(x68kzkbd-pico PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(x68kzkbd-pico PUBLIC pico_stdlib tinyusb_host tinyusb_board)

pico_add_extra_outputs(x68kzkbd-pico)
