cmake_minimum_required(VERSION 3.14)

project(XKCP)

set(CMAKE_C_STANDARD 11)

set(COMMON_FILES
        XKCP/lib/high/KangarooTwelve/KangarooTwelve.c
        XKCP/lib/high/Keccak/FIPS202/KeccakHash.c
        XKCP/lib/high/Keccak/FIPS202/SimpleFIPS202.c
        XKCP/lib/high/Keccak/KeccakDuplex.c
        XKCP/lib/high/Keccak/KeccakSponge.c
        XKCP/lib/high/Keccak/PRG/KeccakPRG.c
        XKCP/lib/high/Keccak/SP800-185/SP800-185.c
        XKCP/lib/high/Ketje/Ketjev2.c
        XKCP/lib/high/Keyak/Keyakv2.c
        XKCP/lib/high/Kravatte/Kravatte.c
        XKCP/lib/high/Kravatte/KravatteModes.c
        XKCP/lib/high/TurboSHAKE/TurboSHAKE.c
        XKCP/lib/high/Xoodyak/Xoodyak.c
        XKCP/lib/high/Xoofff/Xoofff.c
        XKCP/lib/high/Xoofff/XoofffModes.c
        )

set(ARM64_FILES
        XKCP/lib/low/KeccakP-200/ref/KeccakP-200-reference.c
        XKCP/lib/low/KeccakP-400/ref/KeccakP-400-reference.c
        XKCP/lib/low/KeccakP-800/plain/KeccakP-800-opt32.c
        XKCP/lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c
        XKCP/lib/low/KeccakP-1600-times2/fallback-on1/KeccakP-1600-times2-on1.c
        XKCP/lib/low/KeccakP-1600-times4/fallback-on1/KeccakP-1600-times4-on1.c
        XKCP/lib/low/KeccakP-1600-times8/fallback-on1/KeccakP-1600-times8-on1.c
        XKCP/lib/low/Xoodoo/plain/Xoodoo-optimized.c
        XKCP/lib/low/Xoodoo/plain/Xoodyak-full-blocks.c
        XKCP/lib/low/Xoodoo-times4/fallback-on1/Xoodoo-times4-on1.c
        XKCP/lib/low/Xoodoo-times8/fallback-on1/Xoodoo-times8-on1.c
        XKCP/lib/low/Xoodoo-times16/fallback-on1/Xoodoo-times16-on1.c
        XKCP/lib/low/Ketje/OptimizedLE/Ket.c
        )

set(AVX_FILES
        XKCP/lib/low/KeccakP-200/ref/KeccakP-200-reference.c
        XKCP/lib/low/KeccakP-400/ref/KeccakP-400-reference.c
        XKCP/lib/low/KeccakP-800/plain/KeccakP-800-opt32.c
        XKCP/lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c
        XKCP/lib/low/KeccakP-1600-times2/SIMD128/KeccakP-1600-times2-SIMD128.c
        XKCP/lib/low/KeccakP-1600-times4/fallback-on2/KeccakP-1600-times4-on2.c
        XKCP/lib/low/KeccakP-1600-times8/fallback-on2/KeccakP-1600-times8-on2.c
        XKCP/lib/low/Xoodoo/SSE2/Xoodoo-SIMD128.c
        XKCP/lib/low/Xoodoo/SSE2/Xoodyak-full-block-SIMD128.c
        XKCP/lib/low/Xoodoo-times4/SSSE3/Xoodoo-times4-SIMD128.c
        XKCP/lib/low/Xoodoo-times8/fallback-on1/Xoodoo-times8-on1.c
        XKCP/lib/low/Xoodoo-times16/fallback-on1/Xoodoo-times16-on1.c
        XKCP/lib/low/Ketje/SnP-compliant/Ket.c
        )

set(AVX2_FILES
        XKCP/lib/low/KeccakP-200/ref/KeccakP-200-reference.c
        XKCP/lib/low/KeccakP-400/ref/KeccakP-400-reference.c
        XKCP/lib/low/KeccakP-800/plain/KeccakP-800-opt32.c
        XKCP/lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c
        XKCP/lib/low/KeccakP-1600-times2/SIMD128/KeccakP-1600-times2-SIMD128.c
        XKCP/lib/low/KeccakP-1600-times4/AVX2/KeccakP-1600-times4-SIMD256.c
        XKCP/lib/low/KeccakP-1600-times8/fallback-on4/KeccakP-1600-times8-on4.c
        XKCP/lib/low/Xoodoo/SSE2/Xoodoo-SIMD128.c
        XKCP/lib/low/Xoodoo/SSE2/Xoodyak-full-block-SIMD128.c
        XKCP/lib/low/Xoodoo-times4/SSSE3/Xoodoo-times4-SIMD128.c
        XKCP/lib/low/Xoodoo-times8/AVX2/Xoodoo-times8-SIMD256.c
        XKCP/lib/low/Xoodoo-times16/fallback-on1/Xoodoo-times16-on1.c
        XKCP/lib/low/Ketje/SnP-compliant/Ket.c
        )

set(AVX512_FILES
        XKCP/lib/low/KeccakP-200/ref/KeccakP-200-reference.c
        XKCP/lib/low/KeccakP-400/ref/KeccakP-400-reference.c
        XKCP/lib/low/KeccakP-800/plain/KeccakP-800-opt32.c
        XKCP/lib/low/KeccakP-1600/AVX512/C/KeccakP-1600-AVX512.c
        XKCP/lib/low/KeccakP-1600-times2/AVX512/KeccakP-1600-times2-SIMD512.c
        XKCP/lib/low/KeccakP-1600-times4/AVX512/KeccakP-1600-times4-SIMD512.c
        XKCP/lib/low/KeccakP-1600-times8/AVX512/KeccakP-1600-times8-SIMD512.c
        XKCP/lib/low/Xoodoo/AVX512/Xoodoo-SIMD512.c
        XKCP/lib/low/Xoodoo/AVX512/Xoodyak-full-block-SIMD512.c
        XKCP/lib/low/Xoodoo-times4/AVX512/Xoodoo-times4-SIMD512.c
        XKCP/lib/low/Xoodoo-times8/AVX512/Xoodoo-times8-SIMD512.c
        XKCP/lib/low/Xoodoo-times16/AVX512/Xoodoo-times16-SIMD512.c
        XKCP/lib/low/Ketje/SnP-compliant/Ket.c
        )

set(GENERIC_FILES
        XKCP/lib/low/KeccakP-200/ref/KeccakP-200-reference.c
        XKCP/lib/low/KeccakP-400/ref/KeccakP-400-reference.c
        XKCP/lib/low/KeccakP-800/plain/KeccakP-800-opt32.c
        XKCP/lib/low/KeccakP-1600/plain-64bits/KeccakP-1600-opt64.c
        XKCP/lib/low/KeccakP-1600-times2/fallback-on1/KeccakP-1600-times2-on1.c
        XKCP/lib/low/KeccakP-1600-times4/fallback-on1/KeccakP-1600-times4-on1.c
        XKCP/lib/low/KeccakP-1600-times8/fallback-on1/KeccakP-1600-times8-on1.c
        XKCP/lib/low/Xoodoo/plain/Xoodoo-optimized.c
        XKCP/lib/low/Xoodoo/plain/Xoodyak-full-blocks.c
        XKCP/lib/low/Xoodoo-times4/fallback-on1/Xoodoo-times4-on1.c
        XKCP/lib/low/Xoodoo-times8/fallback-on1/Xoodoo-times8-on1.c
        XKCP/lib/low/Xoodoo-times16/fallback-on1/Xoodoo-times16-on1.c
        XKCP/lib/low/Ketje/SnP-compliant/Ket.c
        )

set(COMMON_INCLUDES
        XKCP/lib/common
        XKCP/lib/high/common
        XKCP/lib/high/KangarooTwelve
        XKCP/lib/high/Keccak
        XKCP/lib/high/Keccak/FIPS202
        XKCP/lib/high/Keccak/PRG
        XKCP/lib/high/Keccak/SP800-185
        XKCP/lib/high/Ketje
        XKCP/lib/high/Keyak
        XKCP/lib/high/Kravatte
        XKCP/lib/high/TurboSHAKE
        XKCP/lib/high/Xoodyak
        XKCP/lib/high/Xoofff
        )

set(ARM64_INCLUDES
        XKCP/lib/low/KeccakP-200/ref
        XKCP/lib/low/KeccakP-400/ref
        XKCP/lib/low/KeccakP-800/plain
        XKCP/lib/low/KeccakP-800/plain/ua
        XKCP/lib/low/KeccakP-1600/common
        XKCP/lib/low/KeccakP-1600/plain-64bits
        XKCP/lib/low/common
        XKCP/lib/low/KeccakP-1600/plain-64bits/ua
        XKCP/lib/low/KeccakP-1600-times2/fallback-on1
        XKCP/lib/low/KeccakP-1600-times4/fallback-on1
        XKCP/lib/low/KeccakP-1600-times8/fallback-on1
        XKCP/lib/low/Xoodoo
        XKCP/lib/low/Xoodoo/plain
        XKCP/lib/low/Xoodoo-times4/fallback-on1
        XKCP/lib/low/Xoodoo-times8/fallback-on1
        XKCP/lib/low/Xoodoo-times16/fallback-on1
        XKCP/lib/low/Ketje/OptimizedLE
        )

set(AVX_INCLUDES
        XKCP/lib/low/KeccakP-200/ref
        XKCP/lib/low/KeccakP-400/ref
        XKCP/lib/low/KeccakP-800/plain
        XKCP/lib/low/KeccakP-800/plain/lcua
        XKCP/lib/low/KeccakP-1600/common
        XKCP/lib/low/KeccakP-1600/plain-64bits
        XKCP/lib/low/common
        XKCP/lib/low/KeccakP-1600/plain-64bits/lcua-shld
        XKCP/lib/low/KeccakP-1600-times2/SIMD128
        XKCP/lib/low/KeccakP-1600-times2/SIMD128/SSSE3-ua
        XKCP/lib/low/KeccakP-1600-times4/fallback-on2
        XKCP/lib/low/KeccakP-1600-times8/fallback-on2
        XKCP/lib/low/Xoodoo
        XKCP/lib/low/Xoodoo/SSE2
        XKCP/lib/low/Xoodoo-times4/SSSE3
        XKCP/lib/low/Xoodoo-times8/fallback-on1
        XKCP/lib/low/Xoodoo-times16/fallback-on1
        XKCP/lib/low/Ketje/SnP-compliant
        )

set(AVX2_INCLUDES
        XKCP/lib/low/KeccakP-200/ref
        XKCP/lib/low/KeccakP-400/ref
        XKCP/lib/low/KeccakP-800/plain
        XKCP/lib/low/KeccakP-800/plain/ua
        XKCP/lib/low/KeccakP-1600/common
        XKCP/lib/low/KeccakP-1600/plain-64bits
        XKCP/lib/low/common
        XKCP/lib/low/KeccakP-1600/plain-64bits/ua
        XKCP/lib/low/KeccakP-1600-times2/SIMD128
        XKCP/lib/low/KeccakP-1600-times2/SIMD128/SSSE3-u2
        XKCP/lib/low/KeccakP-1600-times4/AVX2
        XKCP/lib/low/KeccakP-1600-times4/AVX2/u12
        XKCP/lib/low/KeccakP-1600-times8/fallback-on4
        XKCP/lib/low/Xoodoo
        XKCP/lib/low/Xoodoo/SSE2
        XKCP/lib/low/Xoodoo-times4/SSSE3
        XKCP/lib/low/Xoodoo-times8/AVX2
        XKCP/lib/low/Xoodoo-times16/fallback-on1
        XKCP/lib/low/Ketje/SnP-compliant
        )

set(AVX512_INCLUDES
        XKCP/lib/low/KeccakP-200/ref
        XKCP/lib/low/KeccakP-400/ref
        XKCP/lib/low/KeccakP-800/plain
        XKCP/lib/low/KeccakP-800/plain/ua
        XKCP/lib/low/KeccakP-1600/AVX512/C/u12
        XKCP/lib/low/KeccakP-1600/AVX512/C
        XKCP/lib/low/KeccakP-1600-times2/AVX512
        XKCP/lib/low/KeccakP-1600-times2/AVX512/AVX512u12
        XKCP/lib/low/KeccakP-1600-times4/AVX512
        XKCP/lib/low/KeccakP-1600-times4/AVX512/AVX512u12
        XKCP/lib/low/KeccakP-1600-times8/AVX512
        XKCP/lib/low/KeccakP-1600-times8/AVX512/u12
        XKCP/lib/low/Xoodoo
        XKCP/lib/low/Xoodoo/AVX512
        XKCP/lib/low/Xoodoo-times4/AVX512
        XKCP/lib/low/Xoodoo-times8/AVX512
        XKCP/lib/low/Xoodoo-times16/AVX512
        XKCP/lib/low/Ketje/SnP-compliant
        )

set(GENERIC_INCLUDES
        XKCP/lib/low/KeccakP-200/ref
        XKCP/lib/low/KeccakP-400/ref
        XKCP/lib/low/KeccakP-800/plain
        XKCP/lib/low/KeccakP-800/plain/ua
        XKCP/lib/low/KeccakP-1600/common
        XKCP/lib/low/KeccakP-1600/plain-64bits
        XKCP/lib/low/common
        XKCP/lib/low/KeccakP-1600/plain-64bits/ua
        XKCP/lib/low/KeccakP-1600-times2/fallback-on1
        XKCP/lib/low/KeccakP-1600-times4/fallback-on1
        XKCP/lib/low/KeccakP-1600-times8/fallback-on1
        XKCP/lib/low/Xoodoo
        XKCP/lib/low/Xoodoo/plain
        XKCP/lib/low/Xoodoo-times4/fallback-on1
        XKCP/lib/low/Xoodoo-times8/fallback-on1
        XKCP/lib/low/Xoodoo-times16/fallback-on1
        XKCP/lib/low/Ketje/SnP-compliant
        )

if ("${OHT_FLAVOR}" STREQUAL "ARM64")
    set(FILES ${COMMON_FILES} ${ARM64_FILES})
    set(INCLUDES ${COMMON_INCLUDES} ${ARM64_INCLUDES})
elseif ("${OHT_FLAVOR}" STREQUAL "SSE2")
    set(FILES ${COMMON_FILES} ${GENERIC_FILES})
    set(INCLUDES ${COMMON_INCLUDES} ${GENERIC_INCLUDES})
elseif ("${OHT_FLAVOR}" STREQUAL "AVX")
    set(FILES ${COMMON_FILES} ${AVX_FILES})
    set(INCLUDES ${COMMON_INCLUDES} ${AVX_INCLUDES})
elseif ("${OHT_FLAVOR}" STREQUAL "AVX2")
    set(FILES ${COMMON_FILES} ${AVX2_FILES})
    set(INCLUDES ${COMMON_INCLUDES} ${AVX2_INCLUDES})
elseif ("${OHT_FLAVOR}" STREQUAL "AVX512")
    set(FILES ${COMMON_FILES} ${AVX512_FILES})
    set(INCLUDES ${COMMON_INCLUDES} ${AVX512_INCLUDES})
else ()
    message(FATAL_ERROR "OHT_FLAVOR not set.")
endif ()

add_library(${PROJECT_NAME} STATIC ${FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PUBLIC
        XKCP_has_Duplex_Keccak
        XKCP_has_FIPS202
        XKCP_has_KangarooTwelve
        XKCP_has_KeccakP1600
        XKCP_has_KeccakP1600times2
        XKCP_has_KeccakP1600times4
        XKCP_has_KeccakP1600times8
        XKCP_has_KeccakP200
        XKCP_has_KeccakP400
        XKCP_has_KeccakP800
        XKCP_has_Ketje
        XKCP_has_Keyak
        XKCP_has_Kravatte
        XKCP_has_PRG_Keccak
        XKCP_has_SP800_185
        XKCP_has_Sponge_Keccak
        XKCP_has_TurboSHAKE
        XKCP_has_Xoodoo
        XKCP_has_Xoodootimes16
        XKCP_has_Xoodootimes4
        XKCP_has_Xoodootimes8
        XKCP_has_Xoodyak
        XKCP_has_Xoofff
        )
