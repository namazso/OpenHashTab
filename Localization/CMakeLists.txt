cmake_minimum_required(VERSION 3.19)

function(sanitize_string IN_VAR OUT_VAR)
    set(OUT "")
    string(LENGTH "${IN_VAR}" LEN)
    math(EXPR MAX "${LEN} - 1")

    foreach (IDX RANGE ${MAX})
        string(SUBSTRING "${IN_VAR}" ${IDX} "1" CHAR)
        string(HEX "${CHAR}" HEXCHAR)
        if ("${HEXCHAR}" MATCHES "[0-7].")
            set(CHAR "\\x${HEXCHAR}")
        endif ()
        string(APPEND OUT "${CHAR}")
    endforeach ()

    set(${OUT_VAR} "${OUT}" PARENT_SCOPE)
endfunction()

function(lang_json_to_include JSON DEFAULT_JSON OUT_VAR)
    string(JSON LANGID GET "${JSON}" langid)
    string(JSON SUBLANGID GET "${JSON}" sublangid)
    string(JSON STRINGS GET "${JSON}" strings)
    string(JSON DEFAULT_STRINGS GET "${DEFAULT_JSON}" strings)
    string(JSON STRINGS_COUNT LENGTH "${DEFAULT_STRINGS}")

    set(RESULT "LANGUAGE ")
    string(APPEND RESULT "${LANGID}")
    string(APPEND RESULT ", ")
    string(APPEND RESULT "${SUBLANGID}")
    string(APPEND RESULT " \\\nSTRINGTABLE \\\nBEGIN \\\n")

    math(EXPR STRINGS_MAX "${STRINGS_COUNT} - 1")

    foreach (IDX RANGE ${STRINGS_MAX})
        string(JSON KEY MEMBER "${DEFAULT_STRINGS}" "${IDX}")
        string(JSON DEFAULT_VALUE GET "${DEFAULT_STRINGS}" "${KEY}")
        string(JSON VALUE ERROR_VARIABLE VALUE_ERROR GET "${STRINGS}" "${KEY}")
        if ((NOT "${VALUE_ERROR}" STREQUAL "NOTFOUND") OR ("${VALUE}" STREQUAL ""))
            set(VALUE "${DEFAULT_VALUE}")
        endif ()
        sanitize_string("${VALUE}" VALUE)
        string(APPEND RESULT "\tIDS_${KEY}\t\"${VALUE}\" \\\n")
    endforeach ()

    string(APPEND RESULT "END \\\n")
    set(${OUT_VAR} "${RESULT}" PARENT_SCOPE)
endfunction()

file(GLOB LOCALES CONFIGURE_DEPENDS "*.json")

# TODO: Since we're using the undocumented RtlGetString, maybe the defaulting could be moved into code.
file(READ "en-us.json" DEFAULT_LOCALE_JSON)

set(LOCALE_INCLUDE_CONTENT "#define LANGUAGES_IMPL \\\n")

foreach (LOCALE ${LOCALES})
    file(READ "${LOCALE}" LOCALE_JSON)
    lang_json_to_include("${LOCALE_JSON}" "${DEFAULT_LOCALE_JSON}" INCL)
    string(APPEND LOCALE_INCLUDE_CONTENT ${INCL})
endforeach ()

set(LOCALE_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/locale")

file(MAKE_DIRECTORY ${LOCALE_INCLUDE_DIR})

file(WRITE "${LOCALE_INCLUDE_DIR}/Localization.h" "${LOCALE_INCLUDE_CONTENT}")

add_library(Localization INTERFACE)

target_include_directories(Localization INTERFACE "${LOCALE_INCLUDE_DIR}")
