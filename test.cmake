set(phrases "")
file(GLOB_RECURSE SourceFiles
    ${CMAKE_CURRENT_LIST_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Source/*.h
    ${CMAKE_CURRENT_LIST_DIR}/Dependencies/Misc/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Dependencies/Misc/*.h
    ${CMAKE_CURRENT_LIST_DIR}/JUCE/modules/juce_audio_devices/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/JUCE/modules/juce_audio_devices/*.h
    ${CMAKE_CURRENT_LIST_DIR}/JUCE/modules/juce_gui_basics/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/JUCE/modules/juce_gui_basics/*.h
)

foreach(file IN ITEMS ${SourceFiles})
	file(READ ${file} content)
	set(strings "")
	string(REGEX MATCHALL "(juce::translate|TRANS|NEEDS_TRANS) ?\\(\"[^\"]+\"" calls ${content})
	foreach(call IN ITEMS ${calls})
		string(FIND ${call} "\"" start)
		string(FIND ${call} "\"" end REVERSE)
		math(EXPR start "${start}+1")
		math(EXPR end "${end}-${start}")
		string(SUBSTRING ${call} ${start} ${end} str)
		list(APPEND phrases ${str})
	endforeach()
endforeach()

list(JOIN phrases "\n" result)
file(WRITE "TestTranslation.txt" ${result})
message("${result}")