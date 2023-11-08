cmake_minimum_required(VERSION 3.12)
# rest of your project
add_executable(camera_example
    ../model/ml_model.cpp
    main.cpp
)
pico_enable_stdio_usb(camera_example 1)
# target_include_directories(camera_example PRIVATE ${CMAKE_CURRENT_LIST_DIR}/.. include/camera)
# add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/.. camera)
target_link_libraries(camera_example PRIVATE camera pico_stdlib hardware_i2c hardware_gpio)
# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(camera_example)
