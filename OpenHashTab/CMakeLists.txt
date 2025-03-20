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

project(ShellExtension)

file(GLOB FILES CONFIGURE_DEPENDS *.cpp *.rc *.manifest)

add_library(OpenHashTab SHARED ${FILES})

target_link_libraries(OpenHashTab PRIVATE
        Localization
        LegacyAlgorithms
        ctre
        concurrentqueue
        tiny-json
        ntdll.lib
        winhttp.lib
        )

target_compile_definitions(OpenHashTab PRIVATE
        UNICODE
        _UNICODE
        _WINDLL
        "CI_VERSION=\"${CI_VERSION}\""
        "CI_VERSION_MAJOR=${CI_VERSION_MAJOR}"
        "CI_VERSION_MINOR=${CI_VERSION_MINOR}"
        "CI_VERSION_PATCH=${CI_VERSION_PATCH}"
        )

target_compile_options(OpenHashTab PRIVATE
        /EHsc
        )

target_link_options(OpenHashTab PRIVATE
        /EXPORT:DllCanUnloadNow
        /EXPORT:DllGetClassObject
        /DELAYLOAD:winhttp.dll
        )

target_precompile_headers(OpenHashTab PRIVATE stdafx.h)

add_custom_command(TARGET OpenHashTab POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:OpenHashTab> $<TARGET_FILE_DIR:OpenHashTab>
        COMMAND_EXPAND_LISTS
        )
