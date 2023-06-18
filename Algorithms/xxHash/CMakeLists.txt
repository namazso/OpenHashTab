cmake_minimum_required(VERSION 3.14)

project(xxHash)

add_library(${PROJECT_NAME} STATIC xxhash.c)

target_include_directories(${PROJECT_NAME} PUBLIC xxHash)
