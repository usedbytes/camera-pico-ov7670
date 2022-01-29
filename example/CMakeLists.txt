cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(camera_example C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(camera_example main.c)

pico_enable_stdio_usb(camera_example 1)

pico_add_extra_outputs(camera_example)

target_link_libraries(camera_example
	camera
	pico_stdlib
	hardware_i2c
	hardware_gpio
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/.. camera)
