cmake_minimum_required(VERSION 3.15.0)

project(flir-mre)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake;${CMAKE_MODULE_PATH})
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(CMAKE_CXX_STANDARD 14)

find_package(Spinnaker 1.24.0.60 REQUIRED)
message(STATUS "Spinnaker headers at ${Spinnaker_INCLUDE_DIR}")
message(STATUS "Spinnaker library: ${Spinnaker_LIBRARIES}")

include_directories(${Spinnaker_INCLUDE_DIR})
link_directories(/usr/local/lib)
add_executable(flir-mre main.cpp)
target_link_libraries(flir-mre ${Spinnaker_LIBRARIES})
