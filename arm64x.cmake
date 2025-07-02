# directory where the link.rsp file generated during arm64 build will be stored
set(arm64ReproDir "${CMAKE_CURRENT_SOURCE_DIR}/repros")

# This function reads in the content of the rsp file outputted from arm64 build for a target. Then passes the arm64 libs, objs and def file to the linker using /machine:arm64x to combine them with the arm64ec counterparts and create an arm64x binary.

function(set_arm64_dependencies n)
    set(REPRO_FILE "${arm64ReproDir}/${n}.rsp")
    file(STRINGS "${REPRO_FILE}" ARM64_OBJS REGEX obj\"$)
    file(STRINGS "${REPRO_FILE}" ARM64_DEF REGEX def\"$)
    file(STRINGS "${REPRO_FILE}" ARM64_LIBS REGEX lib\"$)
    string(REPLACE "\"" ";" ARM64_OBJS "${ARM64_OBJS}")
    string(REPLACE "\"" ";" ARM64_LIBS "${ARM64_LIBS}")
    string(REPLACE "\"" ";" ARM64_DEF "${ARM64_DEF}")
    string(REPLACE "/def:" "/defArm64Native:" ARM64_DEF "${ARM64_DEF}")

    target_sources(${n} PRIVATE ${ARM64_OBJS})
    target_link_options(${n} PRIVATE /machine:arm64x "${ARM64_DEF}" "${ARM64_LIBS}")
endfunction()

# During the arm64 build, create link.rsp files that containes the absolute path to the inputs passed to the linker (objs, def files, libs).

if("${BUILD_AS_ARM64X}" STREQUAL "ARM64")
    foreach (n ${ARM64X_TARGETS})
        # tell the linker to produce this special rsp file that has absolute paths to its inputs
        target_link_options(${n} PRIVATE "/LINKREPROFULLPATHRSP:${arm64ReproDir}/${n}.rsp")
    endforeach()

    # During the ARM64EC build, modify the link step appropriately to produce an arm64x binary
elseif("${BUILD_AS_ARM64X}" STREQUAL "ARM64EC")
    foreach (n ${ARM64X_TARGETS})
        set_arm64_dependencies(${n})
    endforeach()
endif()
