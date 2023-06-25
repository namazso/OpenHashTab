cmake_minimum_required(VERSION 3.14)

project(crc64)

add_library(${PROJECT_NAME} STATIC crc64.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
