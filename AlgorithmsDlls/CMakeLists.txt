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
