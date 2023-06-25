cmake_minimum_required(VERSION 3.14)

project(BLAKE3 C ASM)

set(CMAKE_C_STANDARD 11)

set(COMMON_FILES
        blake3_dispatch.c
        BLAKE3/c/blake3.c
        BLAKE3/c/blake3_portable.c
        )
set(ARM64_FILES BLAKE3/c/blake3_neon.c)
set(SSE2_FILES BLAKE3/c/blake3_sse2_x86-64_windows_gnu.S)
set(AVX_FILES BLAKE3/c/blake3_sse41_x86-64_windows_gnu.S)
set(AVX2_FILES BLAKE3/c/blake3_avx2_x86-64_windows_gnu.S BLAKE3/c/blake3_sse41_x86-64_windows_gnu.S)
set(AVX512_FILES BLAKE3/c/blake3_avx512_x86-64_windows_gnu.S)

if ("${OHT_FLAVOR}" STREQUAL "ARM64")
    set(FILES ${COMMON_FILES} ${ARM64_FILES})
elseif ("${OHT_FLAVOR}" STREQUAL "SSE2")
    set(FILES ${COMMON_FILES} ${SSE2_FILES})
elseif ("${OHT_FLAVOR}" STREQUAL "AVX")
    set(FILES ${COMMON_FILES} ${AVX_FILES})
elseif ("${OHT_FLAVOR}" STREQUAL "AVX2")
    set(FILES ${COMMON_FILES} ${AVX2_FILES})
elseif ("${OHT_FLAVOR}" STREQUAL "AVX512")
    set(FILES ${COMMON_FILES} ${AVX512_FILES})
else ()
    message(FATAL_ERROR "OHT_FLAVOR not set.")
endif ()

add_library(${PROJECT_NAME} STATIC ${FILES})

target_include_directories(${PROJECT_NAME} PUBLIC BLAKE3/c)

target_compile_definitions(${PROJECT_NAME} PUBLIC BLAKE3_USE_NEON)
