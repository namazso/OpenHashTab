cmake_minimum_required(VERSION 3.14)

project(mbedtls)

add_library(${PROJECT_NAME} STATIC
        mbedtls/library/md2.c
        mbedtls/library/md4.c
        mbedtls/library/md5.c
        mbedtls/library/platform_util.c
        mbedtls/library/ripemd160.c
        mbedtls/library/sha1.c
        mbedtls/library/sha256.c
        mbedtls/library/sha512.c
        )

target_include_directories(${PROJECT_NAME} PUBLIC mbedtls/include ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${PROJECT_NAME} PUBLIC MBEDTLS_CONFIG_FILE="mbedtls_config.h")
