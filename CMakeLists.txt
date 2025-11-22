cmake_minimum_required(VERSION 4.0)

include_directories("C:\\SCEZTrade\\ACS_Source" "{CMAKE_SOURCE_DIR}")

message(STATUS "Source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary dir: ${CMAKE_BINARY_DIR}")

project(EZTrader)

set(CMAKE_CXX_STANDARD 20)

add_library(EZTrader SHARED studies.cpp
        fvg.cpp
)

set_target_properties(EZTrader PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "C:\\SCEZTrade\\Data" OUTPUT_NAME "eztraderMaster"
)