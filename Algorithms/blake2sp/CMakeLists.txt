cmake_minimum_required(VERSION 3.14)

project(blake2sp)

add_library(${PROJECT_NAME} STATIC blake2sp.c)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
