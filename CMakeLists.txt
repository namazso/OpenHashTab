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

cmake_minimum_required(VERSION 3.31)

project(OpenHashTab)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_options(
        /guard:cf
        /Zi
        /utf-8
        /d1nodatetime
)

add_link_options(
        /BREPRO
        /PDBALTPATH:%_PDB%
        /FILEALIGN:0x1000
        /guard:cf
        /OPT:REF
        /OPT:ICF=10
        /NOVCFEATURE
        /NOCOFFGRPINFO
        /DEBUG
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# https://gitlab.kitware.com/cmake/cmake/-/issues/25012
# https://github.com/llvm/llvm-project/issues/63426
set(CMAKE_RC_FLAGS "-C65001")

set(CTRE_BUILD_TESTS OFF)
set(CTRE_BUILD_PACKAGE OFF)
add_subdirectory(compile-time-regular-expressions)

add_subdirectory(concurrentqueue)

add_library(tiny-json STATIC tiny-json/tiny-json.c)
target_include_directories(tiny-json PUBLIC tiny-json)

add_compile_options(
        /W4
)

add_subdirectory(AlgorithmsDlls)

add_subdirectory(Localization)

add_subdirectory(LegacyAlgorithms)

add_subdirectory(Benchmark)

add_subdirectory(OpenHashTab)

add_subdirectory(StandaloneStub)

if(DEFINED BUILD_AS_ARM64X)
    set(ARM64X_TARGETS Benchmark OpenHashTab StandaloneStub)
    include("arm64x.cmake")
endif()
