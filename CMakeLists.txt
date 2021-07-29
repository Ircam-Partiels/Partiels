cmake_minimum_required(VERSION 3.12)

set(CMAKE_XCODE_GENERATE_SCHEME true)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" CACHE STRING "" FORCE)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)
project(Partiels VERSION 0.0.8 LANGUAGES C CXX)

add_subdirectory(JUCE)
include(vamp.cmake)
include(xcode-warnings.cmake)

juce_add_gui_app(Partiels
    PRODUCT_NAME                        "Partiels"
    VERSION                             "0.0.8"
    BUNDLE_ID                           fr.ircam.dev.partiels
    COMPANY_COPYRIGHT                   "Copyright 2021 Ircam. All rights reserved"
    COMPANY_NAME                        "Ircam"
    COMPANY_WEBSITE                     "https://www.ircam.fr/"
    ICON_BIG                            "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Icons/icon.png"
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

target_sources(Partiels PRIVATE ${GlobalSources} ${PartielsSources} ${TranslationsSources})

set(PartielsDefinitions
    APP_DOC_PREFIX="ptl"
    DONT_SET_USING_JUCE_NAMESPACE=1
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
    )

target_compile_definitions(Partiels PRIVATE ${PartielsDefinitions})

target_include_directories(Partiels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies)

if(WIN32)
target_include_directories(Partiels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/steinberg-asio-sdk)
endif()

file(GLOB IconsDataSources ${BINARYDATA_DIRECTORY}/Icons/*.png)
juce_add_binary_data(IconsData HEADER_NAME "IconsData.h" NAMESPACE IconsData SOURCES ${IconsDataSources})

file(GLOB CursorsDataSources ${BINARYDATA_DIRECTORY}/Cursors/*.png)
juce_add_binary_data(CursorsData HEADER_NAME "CursorsData.h" NAMESPACE CursorsData SOURCES ${CursorsDataSources})

file(GLOB TranslationsDataSources ${TranslationsSources})
juce_add_binary_data(TranslationsData HEADER_NAME "TranslationsData.h" NAMESPACE TranslationsData SOURCES ${TranslationsDataSources})

file(GLOB FontsDataSources ${BINARYDATA_DIRECTORY}/Fonts/*.ttf)
juce_add_binary_data(FontsData HEADER_NAME "FontsData.h" NAMESPACE FontsData SOURCES ${FontsDataSources})

file(GLOB MiscDataSources ${BINARYDATA_DIRECTORY}/Misc/About.txt)
juce_add_binary_data(MiscData HEADER_NAME "MiscData.h" NAMESPACE MiscData SOURCES ${MiscDataSources})

target_link_libraries(Partiels PRIVATE
    vamp
    IconsData
    CursorsData
    FontsData
    TranslationsData
    MiscData
    juce::juce_gui_extra
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if(CMAKE_GENERATOR STREQUAL Xcode)
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "3BD2P55TR2")
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "Application Partiels")
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
