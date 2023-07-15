cmake_minimum_required(VERSION 3.14)

project(AlgorithmsDlls)

if (MSVC_C_ARCHITECTURE_ID STREQUAL "x64")
    set(FLAVORS "SSE2" "AVX" "AVX2" "AVX512")
elseif (MSVC_C_ARCHITECTURE_ID STREQUAL "ARM64")
    set(FLAVORS "ARM64")
else ()
    message(FATAL_ERROR "Unsupported architecture")
endif ()

add_library(AlgorithmsDlls INTERFACE)
target_include_directories(AlgorithmsDlls INTERFACE ../Algorithms)
target_link_libraries(AlgorithmsDlls INTERFACE delayimp)

foreach (FLAVOR ${FLAVORS})
    add_library("AlgorithmsDll_${FLAVOR}" SHARED IMPORTED)
    set_property(TARGET "AlgorithmsDll_${FLAVOR}"
            PROPERTY IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/AlgorithmsDll_${FLAVOR}.lib")
    set_property(TARGET "AlgorithmsDll_${FLAVOR}"
            PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/AlgorithmsDll_${FLAVOR}.dll")
    set_property(TARGET "AlgorithmsDll_${FLAVOR}"
            PROPERTY PDB_NAME "${CMAKE_CURRENT_SOURCE_DIR}/AlgorithmsDll_${FLAVOR}.pdb")
    target_link_options("AlgorithmsDll_${FLAVOR}" INTERFACE "/DELAYLOAD:AlgorithmsDll_${FLAVOR}.dll")

    target_link_libraries(AlgorithmsDlls INTERFACE "AlgorithmsDll_${FLAVOR}")
endforeach ()
