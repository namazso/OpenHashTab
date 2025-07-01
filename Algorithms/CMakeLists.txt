# Copyright 2019-2025 namazso <admin@namazso.eu>
# This file is part of OpenHashTab.
#
# OpenHashTab is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OpenHashTab is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with OpenHashTab.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.15)

project(AlgorithmsDll)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(
        -flto
        /GR-
        /GS-
        /EHs-c-
        /guard:cf,nochecks
        /Zi
)

add_compile_definitions(
        _CRTIMP=
        _NO_CRT_STDIO_INLINE=
)

if ("${OHT_FLAVOR}" STREQUAL "ARM64")
elseif ("${OHT_FLAVOR}" STREQUAL "x86")
elseif ("${OHT_FLAVOR}" STREQUAL "SSE2")
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

add_library(${PROJECT_NAME} SHARED
        Hasher2.cpp
        FakeCrt.cpp
)

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
        ntdllp
        )

target_link_options(${PROJECT_NAME} PRIVATE
        /BREPRO
        /PDBALTPATH:%_PDB%
        /FILEALIGN:0x1000
        /cetcompat
        /guard:cf
        "/EXPORT:get_algorithms_begin_${OHT_FLAVOR}=get_algorithms_begin"
        "/EXPORT:get_algorithms_end_${OHT_FLAVOR}=get_algorithms_end"
        /OPT:REF
        /OPT:ICF=10
        /NOENTRY
        /DEBUG
        )

install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION .)
