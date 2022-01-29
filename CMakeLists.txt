add_library(camera INTERFACE)

target_include_directories(camera INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(camera INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/camera.c
	${CMAKE_CURRENT_LIST_DIR}/format.c
)

pico_generate_pio_header(camera ${CMAKE_CURRENT_LIST_DIR}/camera.pio)

target_link_libraries(camera INTERFACE hardware_clocks hardware_dma hardware_irq)
