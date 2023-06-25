cmake_minimum_required(VERSION 3.14)

project(streebog)

add_library(${PROJECT_NAME} STATIC streebog/gost3411-2012-core.c)

target_include_directories(${PROJECT_NAME} PUBLIC streebog ${CMAKE_CURRENT_SOURCE_DIR})
