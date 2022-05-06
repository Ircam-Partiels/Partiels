cmake_minimum_required(VERSION 3.12)

set(CMAKE_XCODE_GENERATE_SCHEME true)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" CACHE STRING "" FORCE)
set(SDIF_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDIF_TESTS OFF CACHE BOOL "" FORCE)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

project(Partiels VERSION 1.0.5 LANGUAGES C CXX)

add_subdirectory(JUCE)
add_subdirectory(Dependencies/sdif)
add_subdirectory(VampPlugins)
include(Dependencies/Misc/Misc.cmake)

juce_add_gui_app(Partiels
    PRODUCT_NAME                        "Partiels"
    VERSION                             ${CMAKE_PROJECT_VERSION}
    BUNDLE_ID                           fr.ircam.dev.partiels
    COMPANY_COPYRIGHT                   "Copyright 2022 Ircam. All rights reserved"
    COMPANY_NAME                        "Ircam"
    COMPANY_WEBSITE                     "https://www.ircam.fr/"
    ICON_BIG                            "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/icon.png"
    DOCUMENT_EXTENSIONS                 "ptldoc,aac,aiff,aif,flac,m4a,mp3,ogg,wav,wma"
    NEEDS_CURL                          FALSE
    NEEDS_WEB_BROWSER                   FALSE
    NEEDS_STORE_KIT                     FALSE
    HARDENED_RUNTIME_ENABLED            true
    HARDENED_RUNTIME_OPTIONS            "com.apple.security.cs.disable-library-validation"
    PLIST_TO_MERGE                      "<plist><dict><key>LSApplicationCategoryType</key><string>public.app-category.music</string></dict></plist>"
    )

juce_generate_juce_header(Partiels)

set_target_properties(Partiels PROPERTIES CXX_STANDARD 20)
target_misc_setup(Partiels MISC_ZOOM_ENABLED MISC_TRANSPORT_ENABLED)

if(APPLE)
   file(GLOB_RECURSE PartielsSources
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.mm
        )
else()
   file(GLOB_RECURSE PartielsSources
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
        )
endif()
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${PartielsSources})

file(GLOB GlobalSources
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Main.cpp
    ${CMAKE_CACHEFILE_DIR}/Partiels_artefacts/JuceLibraryCode/JuceHeader.h
    )
source_group("Source/Global" FILES ${GlobalSources})
target_sources(Partiels PRIVATE ${GlobalSources} ${PartielsSources})

target_compile_definitions(Partiels PRIVATE
    APP_DOC_PREFIX="ptl"
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_ALLOW_STATIC_NULL_VARIABLES=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_LOG_ASSERTIONS=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_ENABLE_REPAINT_DEBUGGING=0
    JUCE_JACK
    JUCE_ALSA
    JUCE_WASAPI
    JUCE_DIRECTSOUND
    JUCE_ASIO
    MISC_SUPPORTS_SDIF=1
    )
target_include_directories(Partiels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies)
if(WIN32)
  target_include_directories(Partiels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/steinberg-asio-sdk)
endif()

set(PARTIELS_BINARYDATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData)
file(GLOB TranslationsDataSources
    ${PARTIELS_BINARYDATA_DIRECTORY}/Translations/*.txt
    )
juce_add_binary_data(TranslationsData HEADER_NAME "TranslationsData.h" NAMESPACE TranslationsData SOURCES ${TranslationsDataSources})

file(GLOB ResourceDataSources
    ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/About.txt
    ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/icon.png
    )
juce_add_binary_data(ResourceData HEADER_NAME "ResourceData.h" NAMESPACE ResourceData SOURCES ${ResourceDataSources})

file(GLOB TestResultsDataSources
    ${PARTIELS_BINARYDATA_DIRECTORY}/Tests/*.csv
    ${PARTIELS_BINARYDATA_DIRECTORY}/Tests/*.json
    ${PARTIELS_BINARYDATA_DIRECTORY}/Tests/*.dat
    ${PARTIELS_BINARYDATA_DIRECTORY}/Tests/*.cue
    )
juce_add_binary_data(TestResultsData HEADER_NAME "TestResultsData.h" NAMESPACE TestResultsData SOURCES ${TestResultsDataSources})

target_link_libraries(Partiels PRIVATE
    vamp
    sdif
    MiscData
    TranslationsData
    ResourceData
    TestResultsData
    juce::juce_gui_extra
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    )

if(DEFINED ENV{PARTIELS_PROVISIONING_PROFILE_SPECIFIER})
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "3BD2P55TR2")
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "$ENV{PARTIELS_PROVISIONING_PROFILE_SPECIFIER}")
endif()
set_target_properties(Partiels PROPERTIES XCODE_SCHEME_ENVIRONMENT "VAMP_PATH=\$VAMP_PATH:/Library/Audio/Plug-Ins/Vamp:$HOME/Library/Audio/Plug-Ins/Vamp:${CMAKE_CURRENT_BINARY_DIR}/Debug:${CMAKE_CURRENT_BINARY_DIR}/Release")
target_enable_xcode_full_warnings(Partiels)

if(WIN32)
    target_compile_options(Partiels PRIVATE /wd4244)
elseif(APPLE)
    target_compile_options(Partiels PRIVATE -fno-aligned-allocation)
endif()

file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/About.txt ${CMAKE_BINARY_DIR}/Install.txt)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/ChangeLog.txt CHANGELOG_CONTENT)
file(APPEND ${CMAKE_BINARY_DIR}/Install.txt "\n")
file(APPEND ${CMAKE_BINARY_DIR}/Install.txt ${CHANGELOG_CONTENT})

add_dependencies(Partiels partiels-vamp-plugins)
if(APPLE)
    set_target_properties(Partiels PROPERTIES 
        XCODE_EMBED_PLUGINS partiels-vamp-plugins
        XCODE_EMBED_PLUGINS_CODE_SIGN_ON_COPY TRUE
        )
else()
    add_custom_command(TARGET Partiels POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Partiels>/Plugins"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:partiels-vamp-plugins>" "$<TARGET_FILE_DIR:Partiels>/Plugins"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:partiels-vamp-plugins>/partiels-vamp-plugins.cat" "$<TARGET_FILE_DIR:Partiels>/Plugins"
    )
endif()

enable_testing()

set(TESTS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Tests)

set(VAMP_EXAMPLES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vamp-plugin-sdk/examples)
file(GLOB VAMP_EXAMPLE_PLUGINS_SOURCES ${VAMP_EXAMPLES_DIRECTORY}/*h  ${VAMP_EXAMPLES_DIRECTORY}/*cpp ${VAMP_EXAMPLES_DIRECTORY}/*c)
source_group("Vamp/Examples" FILES ${VAMP_EXAMPLE_PLUGINS_SOURCES})
add_library(vamp-example-plugins SHARED ${VAMP_EXAMPLE_PLUGINS_SOURCES})
target_link_libraries(vamp-example-plugins PRIVATE vamp-plugin)
set_target_properties(vamp-example-plugins PROPERTIES PREFIX "")
set_target_properties(vamp-example-plugins PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set_target_properties(vamp-example-plugins PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)
if(WIN32)
    set_target_properties(vamp-example-plugins PROPERTIES LINK_FLAGS "/export:vampGetPluginDescriptor")
    target_compile_options(vamp-example-plugins PRIVATE /wd4244 /wd4267 /wd4996)
endif()

add_test(NAME UnitTests COMMAND Partiels --unit-tests)
add_test(NAME Version COMMAND Partiels --version)
add_test(NAME Help COMMAND Partiels --help)
add_test(NAME ExportOptions COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/Xml/ --options=${TESTS_DIRECTORY}/exportOptions.xml)
set_tests_properties(ExportOptions PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
add_test(NAME ExportJpeg COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JPEG/ --format=jpeg --width=800 --height=600)
set_tests_properties(ExportJpeg PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
add_test(NAME ExportPng COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/PNG/ --format=png --width=1200 --height=900 --groups)
set_tests_properties(ExportPng PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
add_test(NAME ExportCsv COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/CSV/ --format=csv --nogrids --header --separator=,)
set_tests_properties(ExportCsv PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
add_test(NAME ExportJson COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JSON/ --format=json)
set_tests_properties(ExportJson PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
add_test(NAME ExportCue COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/CUE/ --format=cue)
set_tests_properties(ExportCue PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
add_test(NAME ExportSdif COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/ --format=sdif --frame=1TST --matrix=1TST --colname=Column)
set_tests_properties(ExportSdif PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
add_test(NAME NewDocument COMMAND Partiels --new --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/Document/new.ptldoc)
set_tests_properties(NewDocument PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

set(EXPECTED_CSV_FILE_SV "${TESTS_DIRECTORY}/SpectralCentroid-SV.csv")
set(EXPECTED_CSV_FILE_FAILURE "${TESTS_DIRECTORY}/SpectralCentroid-failure.csv")
set(GENERATED_CSV_FILE "${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/CSV/Sound Group 1_Spectral Centroid.csv")
add_test(NAME CompareFilesSV COMMAND Partiels --compare-files ${EXPECTED_CSV_FILE_SV} ${GENERATED_CSV_FILE})
set_tests_properties(CompareFilesSV PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
add_test(NAME CompareFilesFailure COMMAND Partiels --compare-files ${EXPECTED_CSV_FILE_FAILURE} ${GENERATED_CSV_FILE})
set_tests_properties(CompareFilesFailure PROPERTIES WILL_FAIL TRUE)

add_test(NAME ConvToSdifMarkers COMMAND Partiels --json2sdif --input=${TESTS_DIRECTORY}/Markers.json --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/Markers.sdif --frame=1MKR --matrix=1MKR)
add_test(NAME ConvFromSdifMarkers COMMAND Partiels --sdif2json --input=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/Markers.sdif --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JSON/Markers.json --frame=1MKR --matrix=1MKR)
set_tests_properties(ConvFromSdifMarkers PROPERTIES DEPENDS "ConvToSdifMarkers")
add_test(NAME ConvCompareMarkers1 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Markers.json ${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JSON/Markers.json)
set_tests_properties(ConvCompareMarkers1 PROPERTIES DEPENDS "ConvFromSdifMarkers")
add_test(NAME ConvCompareMarkers2 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Markers.json ${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/Markers.sdif ${TESTS_DIRECTORY}/MarkersArgs.xml)
set_tests_properties(ConvCompareMarkers2 PROPERTIES DEPENDS "ConvToSdifMarkers")

add_test(NAME ConvToSdifPoints COMMAND Partiels --json2sdif --input=${TESTS_DIRECTORY}/Points.json --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/Points.sdif --frame=1PNT --matrix=1PNT)
add_test(NAME ConvFromSdifPoints COMMAND Partiels --sdif2json --input=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/Points.sdif --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JSON/Points.json --frame=1PNT --matrix=1PNT)
set_tests_properties(ConvFromSdifPoints PROPERTIES DEPENDS "ConvToSdifPoints")
add_test(NAME ConvComparePoints1 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Points.json ${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JSON/Points.json)
set_tests_properties(ConvComparePoints1 PROPERTIES DEPENDS "ConvFromSdifPoints")
add_test(NAME ConvComparePoints2 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Points.json ${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/Points.sdif ${TESTS_DIRECTORY}/PointsArgs.xml)
set_tests_properties(ConvComparePoints2 PROPERTIES DEPENDS "ConvFromSdifPoints")

add_test(NAME ConvToSdifColumns COMMAND Partiels --json2sdif --input=${TESTS_DIRECTORY}/Columns.json --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/Columns.sdif --frame=1COL --matrix=1COL)
add_test(NAME ConvFromSdifColumns COMMAND Partiels --sdif2json --input=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/Columns.sdif --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JSON/Columns.json --frame=1COL --matrix=1COL)
set_tests_properties(ConvFromSdifColumns PROPERTIES DEPENDS "ConvToSdifColumns")
add_test(NAME ConvCompareColumns1 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Columns.json ${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JSON/Columns.json)
set_tests_properties(ConvCompareColumns1 PROPERTIES DEPENDS "ConvFromSdifColumns")
add_test(NAME ConvCompareColumns2 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Columns.json ${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/Columns.sdif ${TESTS_DIRECTORY}/ColumnsArgs.xml)
set_tests_properties(ConvCompareColumns2 PROPERTIES DEPENDS "ConvFromSdifColumns")
