cmake_minimum_required(VERSION 3.12)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)
project(Partiels VERSION 0.0.5 LANGUAGES C CXX)

add_subdirectory(JUCE)
include(vamp.cmake)

juce_add_gui_app(Partiels
    PRODUCT_NAME                        "Partiels"
    VERSION                             "0.0.5"
    BUNDLE_ID                           com.ircam.analyse
    COMPANY_COPYRIGHT                   "Copyright 2020 Ircam. All rights reserved"
    COMPANY_NAME                        "Ircam"
    COMPANY_WEBSITE                     "https://www.ircam.fr/"
    ICON_BIG                            "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Icons/icon.png"
    DOCUMENT_EXTENSIONS                 "ptldoc,aac,aiff,aif,flac,m4a,mp3,ogg,wav,wma"
    NEEDS_CURL                          FALSE
    NEEDS_WEB_BROWSER                   FALSE
    NEEDS_STORE_KIT                     FALSE
    HARDENED_RUNTIME_ENABLED            true
    HARDENED_RUNTIME_OPTIONS           "com.apple.security.cs.disable-library-validation")

juce_add_console_app(PartielsTest
    PRODUCT_NAME                        "PartielsTest"
    VERSION                             "0.0.5")

juce_generate_juce_header(Partiels)
juce_generate_juce_header(PartielsTest)

set_target_properties(Partiels PROPERTIES CXX_STANDARD 20)
set_target_properties(PartielsTest PROPERTIES CXX_STANDARD 20)

set(SOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source)
set(BINARYDATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData)

file(GLOB_RECURSE GlobalSources
    ${SOURCES_DIRECTORY}/Main.cpp
    ${CMAKE_CACHEFILE_DIR}/Partiels_artefacts/JuceLibraryCode/JuceHeader.h)

file(GLOB MiscSources
    ${SOURCES_DIRECTORY}/Misc/AnlBasics.h
    ${SOURCES_DIRECTORY}/Misc/AnlBoundsListener.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlBoundsListener.h
    ${SOURCES_DIRECTORY}/Misc/AnlNotifier.h
    ${SOURCES_DIRECTORY}/Misc/AnlAtomicManager.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlAtomicManager.h
    ${SOURCES_DIRECTORY}/Misc/AnlXmlParser.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlXmlParser.h
    ${SOURCES_DIRECTORY}/Misc/AnlModel.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlModel.h
    ${SOURCES_DIRECTORY}/Misc/AnlIconManager.h
    ${SOURCES_DIRECTORY}/Misc/AnlIconManager.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlFloatingWindow.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlFloatingWindow.h
    ${SOURCES_DIRECTORY}/Misc/AnlHMSmsField.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlHMSmsField.h
    ${SOURCES_DIRECTORY}/Misc/AnlChrono.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlChrono.h
    ${SOURCES_DIRECTORY}/Misc/AnlColouredPanel.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlColouredPanel.h
    ${SOURCES_DIRECTORY}/Misc/AnlColourSelector.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlColourSelector.h
    ${SOURCES_DIRECTORY}/Misc/AnlDraggableTable.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlDraggableTable.h
    ${SOURCES_DIRECTORY}/Misc/AnlDecorator.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlDecorator.h
    ${SOURCES_DIRECTORY}/Misc/AnlTooltip.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlTooltip.h
    ${SOURCES_DIRECTORY}/Misc/AnlNumberField.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlNumberField.h
    ${SOURCES_DIRECTORY}/Misc/AnlPropertyComponent.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlPropertyComponent.h
    ${SOURCES_DIRECTORY}/Misc/AnlResizerBar.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlResizerBar.h
    ${SOURCES_DIRECTORY}/Misc/AnlConcertinaTable.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlConcertinaTable.h
    ${SOURCES_DIRECTORY}/Misc/AnlLoadingCircle.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlLoadingCircle.h
    ${SOURCES_DIRECTORY}/Misc/AnlMessageWindow.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlMessageWindow.h
    ${SOURCES_DIRECTORY}/Misc/AnlMisc.h
    )

if(APPLE)
file(GLOB MiscSources ${MiscSources} ${SOURCES_DIRECTORY}/Misc/AnlFloatingWindow.mm)
endif()

file(GLOB ZoomSources
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomModel.cpp
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomModel.h
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomRuler.cpp
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomRuler.h
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomScrollBar.cpp
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomScrollBar.h
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomTools.cpp
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomTools.h)

file(GLOB TransportSources
    ${SOURCES_DIRECTORY}/Transport/AnlTransportAudioReader.cpp
    ${SOURCES_DIRECTORY}/Transport/AnlTransportAudioReader.h
    ${SOURCES_DIRECTORY}/Transport/AnlTransportDisplay.cpp
    ${SOURCES_DIRECTORY}/Transport/AnlTransportDisplay.h
    ${SOURCES_DIRECTORY}/Transport/AnlTransportLoopBar.cpp
    ${SOURCES_DIRECTORY}/Transport/AnlTransportLoopBar.h
    ${SOURCES_DIRECTORY}/Transport/AnlTransportModel.h
    ${SOURCES_DIRECTORY}/Transport/AnlTransportPlayheadBar.cpp
    ${SOURCES_DIRECTORY}/Transport/AnlTransportPlayheadBar.h)

file(GLOB PluginSources
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginModel.cpp
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginModel.h
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginProcessor.cpp
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginProcessor.h
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListModel.cpp
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListModel.h
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListScanner.cpp
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListScanner.h
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListTable.cpp
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListTable.h)

file(GLOB TrackSources
    ${SOURCES_DIRECTORY}/Track/AnlTrackDirector.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackDirector.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackExporter.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackExporter.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackGraphics.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackGraphics.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackModel.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackModel.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackPlot.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackPlot.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackProcessor.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackProcessor.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackProgressBar.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackProgressBar.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackPropertyPanel.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackPropertyPanel.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackSection.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackSection.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackSnapshot.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackSnapshot.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackStateButton.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackStateButton.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackThumbnail.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackThumbnail.h
    ${SOURCES_DIRECTORY}/Track/AnlTrackTools.cpp
    ${SOURCES_DIRECTORY}/Track/AnlTrackTools.h)

file(GLOB DocumentSources
    ${SOURCES_DIRECTORY}/Document/AnlDocumentModel.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentModel.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentDirector.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentDirector.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileBased.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileBased.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileWatcher.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileWatcher.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentAudioReader.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentAudioReader.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentTransport.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentTransport.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentPlayhead.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentPlayhead.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileInfoPanel.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileInfoPanel.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupButtonState.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupButtonState.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupContainer.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupPlot.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupPlot.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupSection.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupSection.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupSnapshot.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupSnapshot.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupThumbnail.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentGroupThumbnail.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentSection.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentSection.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentTools.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentTools.h)

file(GLOB ApplicationSources
    ${SOURCES_DIRECTORY}/Application/AnlApplicationAbout.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationAbout.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationAudioReader.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationAudioReader.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationAudioSettings.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationAudioSettings.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationProperties.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationProperties.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationCommandTarget.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationCommandTarget.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInterface.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInterface.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationFontManager.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationFontManager.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationLookAndFeel.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationLookAndFeel.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationMainMenuModel.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationMainMenuModel.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationModel.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationModel.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationTranslationManager.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationTranslationManager.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInstance.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInstance.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationWindow.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationWindow.h)

file(GLOB TranslationsSources
    ${BINARYDATA_DIRECTORY}/Translations/Fr/Fr.txt
    ${BINARYDATA_DIRECTORY}/Translations/Fr/Plugin.txt
    ${BINARYDATA_DIRECTORY}/Translations/Fr/Track.txt)

source_group("Source/Global" FILES ${GlobalSources})
source_group("Source/Misc" FILES ${MiscSources})
source_group("Source/Zoom" FILES ${ZoomSources})
source_group("Source/Transport" FILES ${TransportSources})
source_group("Source/Plugin" FILES ${PluginSources})
source_group("Source/Track" FILES ${TrackSources})
source_group("Source/Group" FILES ${GroupSources})
source_group("Source/Document" FILES ${DocumentSources})
source_group("Source/Application" FILES ${ApplicationSources})
source_group("Source/Translations" FILES ${TranslationsSources})

set(PartielsSources ${MiscSources} ${ZoomSources} ${TransportSources} ${PluginSources} ${TrackSources} ${GroupSources} ${DocumentSources} ${ApplicationSources})
target_sources(Partiels PRIVATE ${GlobalSources} ${PartielsSources} ${TranslationsSources})
target_sources(PartielsTest PRIVATE ${PartielsSources} ${SOURCES_DIRECTORY}/Test.cpp)

set(PartielsDefinitions
    APP_DOC_PREFIX="ptl"
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_ALLOW_STATIC_NULL_VARIABLES=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_LOG_ASSERTIONS=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_ENABLE_REPAINT_DEBUGGING=0
    JUCE_JACK
    JUCE_ALSA
    JUCE_WASAPI
    JUCE_DIRECTSOUND
    JUCE_ASIO)

target_compile_definitions(Partiels PRIVATE ${PartielsDefinitions})
target_compile_definitions(PartielsTest PRIVATE ${PartielsDefinitions})

if(WIN32)
target_include_directories(Partiels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/steinberg-asio-sdk)
target_include_directories(PartielsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/steinberg-asio-sdk)
endif()

file(GLOB ImagesDataSources ${BINARYDATA_DIRECTORY}/Images/MagicWord.jpeg)
juce_add_binary_data(ImagesData HEADER_NAME "ImagesData.h" NAMESPACE ImagesData SOURCES ${ImagesDataSources})

file(GLOB IconsDataSources ${BINARYDATA_DIRECTORY}/Icons/*.png)
juce_add_binary_data(IconsData HEADER_NAME "IconsData.h" NAMESPACE IconsData SOURCES ${IconsDataSources})

file(GLOB TranslationsDataSources ${TranslationsSources})
juce_add_binary_data(TranslationsData HEADER_NAME "TranslationsData.h" NAMESPACE TranslationsData SOURCES ${TranslationsDataSources})

file(GLOB FontsDataSources ${BINARYDATA_DIRECTORY}/Fonts/*.otf)
juce_add_binary_data(FontsData HEADER_NAME "FontsData.h" NAMESPACE FontsData SOURCES ${FontsDataSources})

file(GLOB MiscDataSources ${BINARYDATA_DIRECTORY}/Misc/About.txt)
juce_add_binary_data(MiscData HEADER_NAME "MiscData.h" NAMESPACE MiscData SOURCES ${MiscDataSources})

target_link_libraries(Partiels PRIVATE
    vamp
    ImagesData
    IconsData
    FontsData
    TranslationsData
    MiscData
    juce::juce_gui_extra
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

target_link_libraries(PartielsTest PRIVATE
    vamp
    ImagesData
    IconsData
    FontsData
    TranslationsData
    MiscData
    juce::juce_gui_extra
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if(CMAKE_GENERATOR STREQUAL Xcode)
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_CHECK_SWITCH_STATEMENTS "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_EMPTY_BODY "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_BOOL_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_CONSTANT_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_DIRECT_OBJC_ISA_USAGE  "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_EMPTY_BODY "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_ENUM_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_FLOAT_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_INT_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_NON_LITERAL_NULL_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_IMPLICIT_SIGN_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_OBJC_ROOT_CLASS  "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_UNREACHABLE_CODE "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN__DUPLICATE_METHOD_MATCH "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_64_TO_32_BIT_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE  "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNDECLARED_SELECTOR "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNINITIALIZED_AUTOS  "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNUSED_FUNCTION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VARIABLE "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_BOOL_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_CONSTANT_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_DIRECT_OBJC_ISA_USAGE  "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_EMPTY_BODY "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_ENUM_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_INT_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_OBJC_ROOT_CLASS  "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN_UNREACHABLE_CODE "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CLANG_WARN__DUPLICATE_METHOD_MATCH "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_64_TO_32_BIT_CONVERSION "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE  "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNDECLARED_SELECTOR "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNINITIALIZED_AUTOS  "YES")
   set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_UNUSED_FUNCTION "YES")
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(Partiels PRIVATE atomic)
    target_link_libraries(PartielsTest PRIVATE atomic)
endif()

enable_testing()
add_test(NAME AllTest COMMAND PartielsTest)
