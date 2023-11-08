cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
set(ENV{PICO_SDK_PATH} "/home/ai-team/pico/pico-sdk")

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(camera_example C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()


add_library(camera INTERFACE)

target_include_directories(camera INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(camera INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/camera.c
	${CMAKE_CURRENT_LIST_DIR}/format.c
	${CMAKE_CURRENT_LIST_DIR}/ov7670.c
)

pico_generate_pio_header(camera ${CMAKE_CURRENT_LIST_DIR}/camera.pio)

target_link_libraries(camera INTERFACE
	hardware_clocks
	hardware_dma
	hardware_irq
	hardware_pio
)

add_subdirectory("example")