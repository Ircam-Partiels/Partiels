file(GLOB_RECURSE TranslationSourceFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Misc/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Misc/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_devices/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_devices/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_gui_basics/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_gui_basics/*.h
)

if(NOT DEFINED KEYS_FILE OR "${KEYS_FILE}" STREQUAL "")
    message(FATAL_ERROR "KEYS_FILE is required. Usage: cmake -DKEYS_FILE=<keys_file> -P TranslationKeysExtractor.cmake")
endif()

get_filename_component(_out_dir "${KEYS_FILE}" DIRECTORY)
if(NOT "${_out_dir}" STREQUAL "")
    file(MAKE_DIRECTORY "${_out_dir}")
endif()
file(WRITE "${KEYS_FILE}" "")
if(NOT DEFINED QUOTED_KEY_LIST)
    set(QUOTED_KEY_LIST OFF)
endif()

set(TranslationKeyList "")
foreach(TranslationSourceFile IN ITEMS ${TranslationSourceFiles})
    file(READ "${TranslationSourceFile}" TranslationSourceFileContent)
    # Match calls like juce::translate("...") while allowing escaped quotes inside the literal.
    string(REGEX MATCHALL "(juce::translate|TRANS|NEEDS_TRANS)[ \\t\\r\\n]*\\(\"([^\"\\\\]|\\\\.)*\"" JuceTranslationCalls "${TranslationSourceFileContent}")
    foreach(JuceTranslationCall IN ITEMS ${JuceTranslationCalls})
        string(FIND "${JuceTranslationCall}" "\"" TranslationKeyStart)
        string(FIND "${JuceTranslationCall}" "\"" TranslationKeyEnd REVERSE)
        math(EXPR TranslationKeyStart "${TranslationKeyStart}+1")
        math(EXPR TranslationKeyLength "${TranslationKeyEnd}-${TranslationKeyStart}")
        string(SUBSTRING "${JuceTranslationCall}" ${TranslationKeyStart} ${TranslationKeyLength} TranslationKey)
        list(FIND TranslationKeyList "${TranslationKey}" Index)
        if(Index EQUAL "-1")
            if(QUOTED_KEY_LIST)
                file(APPEND "${KEYS_FILE}" "\"${TranslationKey}\"\n")
            else()
                file(APPEND "${KEYS_FILE}" "${TranslationKey}\n")
            endif()
            list(APPEND TranslationKeyList "${TranslationKey}")
        endif()
    endforeach()
endforeach()
