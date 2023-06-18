cmake_minimum_required(VERSION 3.14)

project(AlgorithmsDll)

set(CMAKE_CXX_STANDARD 20)

string(REGEX REPLACE "/M[DT]d?" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
string(REGEX REPLACE "/M[DT]d?" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REGEX REPLACE "/M[DT]d?" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
string(REGEX REPLACE "/M[DT]d?" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
string(REGEX REPLACE "/M[DT]d?" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
string(REGEX REPLACE "/M[DT]d?" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REGEX REPLACE "/M[DT]d?" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
string(REGEX REPLACE "/M[DT]d?" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")

string(REGEX REPLACE "/Ob1" "/Ob2" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
string(REGEX REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")

add_compile_options(-flto /GR- /GS- /guard:cf,nochecks /MT)

if ("${OHT_FLAVOR}" STREQUAL "ARM64")
elseif ("${OHT_FLAVOR}" STREQUAL "SSE2")
elseif ("${OHT_FLAVOR}" STREQUAL "AVX")
    add_compile_options(/arch:AVX)
elseif ("${OHT_FLAVOR}" STREQUAL "AVX2")
    add_compile_options(/arch:AVX2)
elseif ("${OHT_FLAVOR}" STREQUAL "AVX512")
    add_compile_options(/arch:AVX512)
else ()
    message(FATAL_ERROR "OHT_FLAVOR not set.")
endif ()

add_subdirectory(XKCP)
add_subdirectory(QuickXorHash)
add_subdirectory(streebog)
add_subdirectory(xxHash)
add_subdirectory(crc32)
add_subdirectory(crc64)
add_subdirectory(mbedtls)
add_subdirectory(blake2sp)
add_subdirectory(BLAKE3)

add_library(${PROJECT_NAME} SHARED Hasher2.cpp DllMain.c)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_${OHT_FLAVOR}")

target_link_libraries(${PROJECT_NAME}
        XKCP
        QuickXorHash
        streebog
        xxHash
        crc32
        crc64
        mbedtls
        blake2sp
        BLAKE3
        )

install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION .)
