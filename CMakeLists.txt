cmake_minimum_required(VERSION 4.0)

include_directories("D:\\SierraChartCME\\ACS_Source" "{CMAKE_SOURCE_DIR}")

message(STATUS "Source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary dir: ${CMAKE_BINARY_DIR}")

project(EZTrader)

set(CMAKE_CXX_STANDARD 20)

add_library(EZTrader SHARED studies.cpp
)

set_target_properties(EZTrader PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "D:\\SierraChartCME\\Data" OUTPUT_NAME "eztraderMaster"
)