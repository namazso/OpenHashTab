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
