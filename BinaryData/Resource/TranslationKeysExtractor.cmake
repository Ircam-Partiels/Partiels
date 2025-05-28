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

file(WRITE ${TRANSLATION_KEY_FILE} "")

set(TranslationKeyList "")
foreach(TranslationSourceFile IN ITEMS ${TranslationSourceFiles})
    file(READ ${TranslationSourceFile} TranslationSourceFileContent)
    string(REGEX MATCHALL "(juce::translate|TRANS|NEEDS_TRANS) ?\\(\"[^\"]+\"" JuceTranslationCalls ${TranslationSourceFileContent})
    foreach(JuceTranslationCall IN ITEMS ${JuceTranslationCalls})
        string(FIND ${JuceTranslationCall} "\"" TranslationKeyStart)
        string(FIND ${JuceTranslationCall} "\"" TranslationKeyEnd REVERSE)
        math(EXPR TranslationKeyStart "${TranslationKeyStart}+1")
        math(EXPR TranslationKeyLength "${TranslationKeyEnd}-${TranslationKeyStart}")
        string(SUBSTRING ${JuceTranslationCall} ${TranslationKeyStart} ${TranslationKeyLength} TranslationKey)
        list(FIND TranslationKeyList "${TranslationKey}" Index)
        if(${Index} EQUAL "-1")
            file(APPEND ${TRANSLATION_KEY_FILE} "${TranslationKey}\n")
            list(APPEND TranslationKeyList "${TranslationKey}")
        endif()
    endforeach()
endforeach()
