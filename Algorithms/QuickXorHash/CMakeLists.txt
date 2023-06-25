cmake_minimum_required(VERSION 3.14)

project(QuickXorHash)

add_library(${PROJECT_NAME} STATIC QuickXorHash/quickxorhash.c)

target_include_directories(${PROJECT_NAME} PUBLIC QuickXorHash)
