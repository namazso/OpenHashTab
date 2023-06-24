cmake_minimum_required(VERSION 3.14)

project(ShellExtension)

file(GLOB FILES CONFIGURE_DEPENDS *.h *.cpp *.rc *.manifest)

add_library(OpenHashTab SHARED ${FILES})

target_link_libraries(OpenHashTab PRIVATE
        Localization
        LegacyAlgorithms
        ctre
        concurrentqueue
        tiny-json
        )

target_compile_definitions(OpenHashTab PRIVATE
        UNICODE
        _UNICODE
        _WINDLL
        CI_VERSION=\"blah\"
        CI_VERSION_MAJOR=1
        CI_VERSION_MINOR=1
        CI_VERSION_PATCH=1
        )

target_compile_options(OpenHashTab PRIVATE
        /EHsc
        )

target_precompile_headers(OpenHashTab PRIVATE stdafx.h)
