cmake_minimum_required(VERSION 3.14)

project(LegacyAlgorithms)

add_library(${PROJECT_NAME} STATIC LegacyHasher.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE AlgorithmsDlls)
