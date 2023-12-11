cmake_minimum_required(VERSION 3.12)
project(universal_serializing CXX)

include(third_party/userver/cmake/SetupEnvironment.cmake)
include(GNUInstallDirs)

add_subdirectory(third_party/userver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common sources
add_library(${PROJECT_NAME}_objs OBJECT
    string.hpp
    universal_serializing.hpp
    basic_checks.hpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver-core)




# Unit Tests
add_executable(${PROJECT_NAME}_unittest
    tests.cpp
)
target_link_libraries(${PROJECT_NAME}_unittest PRIVATE ${PROJECT_NAME}_objs userver-utest)
add_google_tests(${PROJECT_NAME}_unittest)

