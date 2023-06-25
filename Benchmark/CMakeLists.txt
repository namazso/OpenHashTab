cmake_minimum_required(VERSION 3.14)

project(Benchmark)

add_executable(${PROJECT_NAME} Benchmark.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE LegacyAlgorithms)
