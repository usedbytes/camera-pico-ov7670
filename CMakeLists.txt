cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
# set(ENV{PICO_SDK_PATH} "/home/ai-team/pico/pico-sdk")
set(ENV{PICO_SDK_PATH} "/home/ttb/raspberry-pi-pico/pico/pico-sdk")
# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(camera_example C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Define ARM_CPU, CMSIS ROOT and DSP to use CMSIS-DSP
set(ARM_CPU "cortex-m0plus")
set(ROOT ${CMAKE_CURRENT_LIST_DIR}/lib/CMSIS_5)
set(DSP ${ROOT}/CMSIS/DSP)

set(CONFIGTABLE ON)
set(RFFT_Q15_256 ON)
set(ALLFAST ON)

# include CMSIS-DSP .cmake for GCC Toolchain
include(${DSP}/Toolchain/GCC.cmake)

# add CMSIS-DSP Source directory as subdirectory
add_subdirectory(${DSP}/Source EXCLUDE_FROM_ALL)

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
	pico-tflmicro
)
add_subdirectory("lib/pico-tflmicro" EXCLUDE_FROM_ALL)
add_subdirectory("example")