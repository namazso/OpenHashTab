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

project(LegacyAlgorithms)

add_library(${PROJECT_NAME} STATIC LegacyHasher.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE AlgorithmsDlls)
