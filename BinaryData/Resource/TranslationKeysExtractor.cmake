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

file(WRITE ${TRANSLATION_FILE} "")

set(TranslationKeyList "")
foreach(TranslationSourceFile IN ITEMS ${TranslationSourceFiles})
    file(READ ${TranslationSourceFile} TranslationSourceFileContent)
    string(REGEX MATCHALL "(juce::translate|TRANS|NEEDS_TRANS) ?\\(\"[^\"]+\"" JuceTranslationCalls ${TranslationSourceFileContent})
    foreach(JuceTranslationCall IN ITEMS ${JuceTranslationCalls})
        string(FIND ${JuceTranslationCall} "\"" TranslationKeyStartIndex)
        string(FIND ${JuceTranslationCall} "\"" TranslationKeyEndIndex REVERSE)
        math(EXPR TranslationKeyStartIndex "${TranslationKeyStartIndex}+1")
        math(EXPR TranslationKeyEndIndex "${TranslationKeyEndIndex}-${TranslationKeyStartIndex}")
        string(SUBSTRING ${JuceTranslationCall} ${TranslationKeyStartIndex} ${TranslationKeyEndIndex} TranslationKey)
        list(FIND TranslationKeyList "${TranslationKey}" Index)
        if(${Index} EQUAL "-1")
        file(APPEND ${TRANSLATION_FILE} "${TranslationKey}\n")
        list(APPEND TranslationKeyList "${TranslationKey}")
        endif()
    endforeach()
endforeach()
