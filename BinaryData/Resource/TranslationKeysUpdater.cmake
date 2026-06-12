cmake_minimum_required(VERSION 3.10)

# Usage:
#   cmake -DKEYS_FILE=<keys_file> -DTRANS_FILE=<translation_file> -DOUT_FILE=<output_file> -P TranslationKeysUpdater.cmake
#
# Arguments:
#   KEYS_FILE   - File containing the reference list of translation keys (one per line)
#   TRANS_FILE  - Existing partial translation file
#   OUT_FILE    - Output file path (defaults to merged_translation.txt)

# --- Validate arguments ---
if(NOT DEFINED KEYS_FILE)
    message(FATAL_ERROR "KEYS_FILE is required. Usage: cmake -DKEYS_FILE=<keys_file> -DTRANS_FILE=<translation_file> [-DOUT_FILE=<output_file>] -P TranslationKeysUpdater.cmake")
endif()
if(NOT DEFINED TRANS_FILE)
    message(FATAL_ERROR "TRANS_FILE is required. Usage: cmake -DKEYS_FILE=<keys_file> -DTRANS_FILE=<translation_file> [-DOUT_FILE=<output_file>] -P TranslationKeysUpdater.cmake")
endif()
if(NOT DEFINED OUT_FILE)
    set(OUT_FILE "merged_translation.txt")
endif()

if(NOT EXISTS "${KEYS_FILE}")
    message(FATAL_ERROR "Keys file not found: ${KEYS_FILE}")
endif()
if(NOT EXISTS "${TRANS_FILE}")
    message(FATAL_ERROR "Translation file not found: ${TRANS_FILE}")
endif()

# --- Read key file ---
file(READ "${KEYS_FILE}" KEY_LINES)
string(REPLACE "\r\n" "\n" KEY_LINES  "${KEY_LINES}")
string(REPLACE "\n" ";" KEY_LINES  "${KEY_LINES}")
#message(STATUS "KEYS: ${KEY_LINES}")

# --- Read translation file ---
file(READ "${TRANS_FILE}" TRANS_RAW)
# Look for the line stating with "language:"
string(FIND "${TRANS_RAW}" "language:" _language_pos)
if(NOT _language_pos EQUAL 0)
    message(FATAL_ERROR "Translation file does not start with 'language:'.")
endif()
# Look for the line stating with "countries:"
string(FIND "${TRANS_RAW}" "countries:" _countries_pos)
if(_countries_pos EQUAL -1)
    message(FATAL_ERROR "Translation file does not contain 'countries:'.")
endif()

string(REPLACE "\n\n" "\n" TRANS_LINES "${TRANS_RAW}")
string(REPLACE "\r\n" "\n" TRANS_LINES "${TRANS_LINES}")
string(REPLACE "\n" ";" TRANS_LINES "${TRANS_LINES}")
string(REPLACE "\" = \"" "\";\"" TRANS_LINES "${TRANS_LINES}")
#message(STATUS "TRANSLATIONS: ${TRANS_LINES}")

set(OUTPUT_CONTENT "")
# Copy the two first lines (language: and countries:) as header
list(GET TRANS_LINES 0 _lang_line)
list(GET TRANS_LINES 1 _countries_line)
string(APPEND OUTPUT_CONTENT "${_lang_line}\n")
string(APPEND OUTPUT_CONTENT "${_countries_line}\n")
string(APPEND OUTPUT_CONTENT "\n")

foreach(KEY_LINE IN LISTS KEY_LINES)
    if(NOT "${KEY_LINE}" STREQUAL "")
        list(FIND TRANS_LINES "${KEY_LINE}" _found_idx)
        list(LENGTH TRANS_LINES TRANS_LINES_LENGTH)
        if(_found_idx GREATER -1)
            # We have a translation – emit:  <original LHS> = <translation RHS>
            math(EXPR _rhs_idx "${_found_idx} + 1")

            if(_rhs_idx GREATER_EQUAL ${TRANS_LINES_LENGTH})
                string(APPEND OUTPUT_CONTENT "${KEY_LINE} = \"\"\n")
                message(WARNING "Malformed translation file: no RHS for key '${KEY_LINE}'")
            else()
                list(GET TRANS_LINES ${_rhs_idx} _rhs_line)
                string(APPEND OUTPUT_CONTENT "${KEY_LINE} = ${_rhs_line}\n")
            endif()
        else()
            message(WARNING "Missing translation for key: ${KEY_LINE}")
            string(APPEND OUTPUT_CONTENT "${KEY_LINE} = \"\"\n")
        endif()
    endif()
endforeach()
    
get_filename_component(_out_dir "${OUT_FILE}" DIRECTORY)
if(NOT "${_out_dir}" STREQUAL "")
    file(MAKE_DIRECTORY "${_out_dir}")
endif()
file(WRITE "${OUT_FILE}" "${OUTPUT_CONTENT}")
message(STATUS "Merged translation written to: ${OUT_FILE}")
