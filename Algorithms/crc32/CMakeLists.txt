cmake_minimum_required(VERSION 3.14)

project(crc32)

add_library(${PROJECT_NAME} STATIC crc32/Crc32.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC crc32)
