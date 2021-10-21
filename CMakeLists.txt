cmake_minimum_required(VERSION 3.12)

set(CMAKE_XCODE_GENERATE_SCHEME true)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" CACHE STRING "" FORCE)
set(SDIF_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDIF_TESTS OFF CACHE BOOL "" FORCE)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)
project(Partiels VERSION 1.0.0 LANGUAGES C CXX)

add_subdirectory(JUCE)
add_subdirectory(Dependencies/sdif)
include(vamp.cmake)
include(xcode-warnings.cmake)
include(Dependencies/Misc/Misc.cmake)
source_group("Misc" FILES ${MiscSource})

juce_add_gui_app(Partiels
    PRODUCT_NAME                        "Partiels"
    VERSION                             "1.0.0"
    BUNDLE_ID                           fr.ircam.dev.partiels
    COMPANY_COPYRIGHT                   "Copyright 2021 Ircam. All rights reserved"
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

set(SOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source)
set(BINARYDATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData)

SET(PartielsSources "")
file(GLOB SOURCES_SUBDIRECTORIES_NAMES RELATIVE ${SOURCES_DIRECTORY} ${SOURCES_DIRECTORY}/*)

foreach(SOURCES_SUBDIRECTORY_NAME ${SOURCES_SUBDIRECTORIES_NAMES})

  if(IS_DIRECTORY "${SOURCES_DIRECTORY}/${SOURCES_SUBDIRECTORY_NAME}")

    set(SUBDIRECTORY_PATH "${SOURCES_DIRECTORY}/${SOURCES_SUBDIRECTORY_NAME}")

    if(APPLE)
       file(GLOB SUBDIRECTORY_SOURCES ${SUBDIRECTORY_PATH}/*.cpp ${SUBDIRECTORY_PATH}/*.h ${SUBDIRECTORY_PATH}/*.mm)
    else()
       file(GLOB SUBDIRECTORY_SOURCES ${SUBDIRECTORY_PATH}/*.cpp ${SUBDIRECTORY_PATH}/*.h)
    endif()

    source_group("Source/${SOURCES_SUBDIRECTORY_NAME}" FILES ${SUBDIRECTORY_SOURCES})
    set(PartielsSources ${PartielsSources} ${SUBDIRECTORY_SOURCES})

  endif()

endforeach()

file(GLOB GlobalSources
    ${SOURCES_DIRECTORY}/Main.cpp
    ${CMAKE_CACHEFILE_DIR}/Partiels_artefacts/JuceLibraryCode/JuceHeader.h
    )
source_group("Source/Global" FILES ${GlobalSources})

file(GLOB TranslationsSources ${BINARYDATA_DIRECTORY}/Translations/*.txt)
source_group("Source/Translations" FILES ${TranslationsSources})

target_sources(Partiels PRIVATE ${GlobalSources} ${PartielsSources} ${MiscSource} ${TranslationsSources})

set(PartielsDefinitions
    APP_DOC_PREFIX="ptl"
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_MODAL_LOOPS_PERMITTED=1
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

target_compile_definitions(Partiels PRIVATE ${PartielsDefinitions})

target_include_directories(Partiels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies)

if(WIN32)
target_include_directories(Partiels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/steinberg-asio-sdk)
endif()

file(GLOB TranslationsDataSources ${TranslationsSources})
juce_add_binary_data(TranslationsData HEADER_NAME "TranslationsData.h" NAMESPACE TranslationsData SOURCES ${TranslationsDataSources})

file(GLOB ResourceDataSources ${BINARYDATA_DIRECTORY}/Resource/About.txt ${BINARYDATA_DIRECTORY}/Resource/icon.png)
juce_add_binary_data(ResourceData HEADER_NAME "ResourceData.h" NAMESPACE ResourceData SOURCES ${ResourceDataSources})

file(GLOB TestResultsDataSources ${BINARYDATA_DIRECTORY}/Tests/*.csv ${BINARYDATA_DIRECTORY}/Tests/*.json ${BINARYDATA_DIRECTORY}/Tests/*.dat ${BINARYDATA_DIRECTORY}/Tests/*.cue)
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
    juce::juce_recommended_warning_flags)

if(CMAKE_GENERATOR STREQUAL Xcode)
    if(DEFINED ENV{PARTIELS_PROVISIONING_PROFILE_SPECIFIER})
        set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "3BD2P55TR2")
        set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
        set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "$ENV{PARTIELS_PROVISIONING_PROFILE_SPECIFIER}")
    endif()
    set(VAMP_PATH_CMD "VAMP_PATH=\$VAMP_PATH:/Library/Audio/Plug-Ins/Vamp:$HOME/Library/Audio/Plug-Ins/Vamp:${CMAKE_CURRENT_BINARY_DIR}/Debug:${CMAKE_CURRENT_BINARY_DIR}/Release")
    set_target_properties(Partiels PROPERTIES XCODE_SCHEME_ENVIRONMENT ${VAMP_PATH_CMD})
endif()

target_enable_xcode_full_warnings(Partiels)

if(WIN32)
target_compile_options(Partiels PRIVATE /wd4244)
elseif(APPLE)
target_compile_options(Partiels PRIVATE -fno-aligned-allocation)
else()
target_link_libraries(Partiels PRIVATE atomic)
endif()

set(TESTS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Tests)

enable_testing()
add_test(NAME UnitTests COMMAND Partiels --unit-tests)
add_test(NAME Version COMMAND Partiels --version)
add_test(NAME Help COMMAND Partiels --help)
add_test(NAME ExportOptions COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/Xml/ --options=${TESTS_DIRECTORY}/exportOptions.xml)
add_test(NAME ExportJpeg COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JPEG/ --format=jpeg --width=800 --height=600)
add_test(NAME ExportPng COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/PNG/ --format=png --width=1200 --height=900 --groups)
add_test(NAME ExportCsv COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/CSV/ --format=csv --nogrids --header --separator=,)
add_test(NAME ExportJson COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JSON/ --format=json)
add_test(NAME ExportCue COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/CUE/ --format=cue)
add_test(NAME NewDocument COMMAND Partiels --new --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/Document/new.ptldoc)

set(EXPECTED_CSV_FILE_SV "${TESTS_DIRECTORY}/SpectralCentroid-SV.csv")
set(EXPECTED_CSV_FILE_FAILURE "${TESTS_DIRECTORY}/SpectralCentroid-failure.csv")
set(GENERATED_CSV_FILE "${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/CSV/Sound Group 1_Spectral Centroid.csv")
add_test(NAME CompareFilesSV COMMAND Partiels --compare-files ${EXPECTED_CSV_FILE_SV} ${GENERATED_CSV_FILE})
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
