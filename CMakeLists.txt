cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Hook: Move sdkconfig to sdkconfig.old if it exists
if(EXISTS "${CMAKE_SOURCE_DIR}/sdkconfig")
    message(STATUS "Moving existing sdkconfig to sdkconfig.old")
    file(RENAME "${CMAKE_SOURCE_DIR}/sdkconfig" "${CMAKE_SOURCE_DIR}/sdkconfig.old")
endif()

project(fancy-board)
