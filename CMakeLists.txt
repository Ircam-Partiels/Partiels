cmake_minimum_required(VERSION 3.12)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
project(Brioche VERSION 0.0.3 LANGUAGES C CXX)

add_subdirectory(JUCE)
include(vamp.cmake)

juce_add_gui_app(Brioche
    PRODUCT_NAME                        "Brioche"
    VERSION                             "0.0.3"
    BUNDLE_ID                           com.ircam.analyse
    COMPANY_COPYRIGHT                   "Copyright 2020 Ircam. All rights reserved"
    COMPANY_NAME                        "Ircam"
    COMPANY_WEBSITE                     "https://www.ircam.fr/"
    ICON_BIG                            "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Icons/icon.png"
    DOCUMENT_EXTENSIONS                 brioche
    NEEDS_CURL                          FALSE
    NEEDS_WEB_BROWSER                   FALSE
    NEEDS_STORE_KIT                     FALSE
    HARDENED_RUNTIME_ENABLED            true
    HARDENED_RUNTIME_OPTIONS           "com.apple.security.cs.disable-library-validation")

juce_add_console_app(BriocheTest
    PRODUCT_NAME                        "BriocheTest"
    VERSION                             "0.0.3")

juce_generate_juce_header(Brioche)
juce_generate_juce_header(BriocheTest)

set_target_properties(Brioche PROPERTIES CXX_STANDARD 20)
set_target_properties(BriocheTest PROPERTIES CXX_STANDARD 20)

set(SOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source)
set(BINARYDATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData)

file(GLOB_RECURSE GlobalSources
    ${SOURCES_DIRECTORY}/Main.cpp
    ${CMAKE_CACHEFILE_DIR}/Brioche_artefacts/JuceLibraryCode/JuceHeader.h)

file(GLOB MiscSources
    ${SOURCES_DIRECTORY}/Misc/AnlBasics.h
    ${SOURCES_DIRECTORY}/Misc/AnlBoundsListener.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlBoundsListener.h
    ${SOURCES_DIRECTORY}/Misc/AnlNotifier.h
    ${SOURCES_DIRECTORY}/Misc/AnlBroadcaster.h
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
    ${SOURCES_DIRECTORY}/Misc/AnlColouredPanel.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlColouredPanel.h
    ${SOURCES_DIRECTORY}/Misc/AnlColourSelector.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlColourSelector.h
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
    ${SOURCES_DIRECTORY}/Misc/AnlDraggableTable.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlDraggableTable.h
    ${SOURCES_DIRECTORY}/Misc/AnlLoadingCircle.cpp
    ${SOURCES_DIRECTORY}/Misc/AnlLoadingCircle.h
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
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomPlayhead.cpp
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomPlayhead.h)

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

file(GLOB AnalyzerSources
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerModel.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerModel.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerDirector.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerDirector.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerExporter.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerExporter.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerProcessor.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerProcessor.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerPropertyPanel.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerPropertyPanel.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerSnapshot.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerSnapshot.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerRenderer.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerRenderer.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerPlot.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerPlot.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerThumbnail.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerThumbnail.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerSection.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerSection.h)

file(GLOB GroupSources
    ${SOURCES_DIRECTORY}/Group/AnlGroupModel.h
    ${SOURCES_DIRECTORY}/Group/AnlGroupPlot.cpp
    ${SOURCES_DIRECTORY}/Group/AnlGroupPlot.h
    ${SOURCES_DIRECTORY}/Group/AnlGroupSection.cpp
    ${SOURCES_DIRECTORY}/Group/AnlGroupSection.h)

file(GLOB DocumentSources
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
    ${SOURCES_DIRECTORY}/Document/AnlDocumentPlot.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentPlot.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentSection.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentSection.h)

file(GLOB ApplicationSources
    ${SOURCES_DIRECTORY}/Application/AnlApplicationModel.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationModel.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationProperties.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationProperties.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationAudioReader.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationAudioReader.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationCommandTarget.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationCommandTarget.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInterface.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInterface.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationFontManager.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationFontManager.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationLookAndFeel.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationLookAndFeel.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationTranslationManager.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationTranslationManager.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInstance.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInstance.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationWindow.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationWindow.h)

file(GLOB TranslationsSources
    ${BINARYDATA_DIRECTORY}/Translations/Fr/Fr.txt
    ${BINARYDATA_DIRECTORY}/Translations/Fr/Plugin.txt
    ${BINARYDATA_DIRECTORY}/Translations/Fr/Analyzer.txt)

source_group("Global" FILES ${GlobalSources})
source_group("Misc" FILES ${MiscSources})
source_group("Zoom" FILES ${ZoomSources})
source_group("Plugin" FILES ${PluginSources})
source_group("Analyzer" FILES ${AnalyzerSources})
source_group("Group" FILES ${GroupSources})
source_group("Document" FILES ${DocumentSources})
source_group("Application" FILES ${ApplicationSources})
source_group("Translations" FILES ${TranslationsSources})

function(create_module_group TARGET GROUP_PATH MODULE_PATH)

if(APPLE)
    file(GLOB MAIN_CPP_SOURCES ${MODULE_PATH}/*.cpp)
    set_source_files_properties(${MAIN_CPP_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)
    source_group(${GROUP_PATH} FILES ${MAIN_CPP_SOURCES})
    target_sources(${TARGET} PRIVATE ${MAIN_CPP_SOURCES})

    file(GLOB MAIN_OTHER_SOURCES ${MODULE_PATH}/*.h ${MODULE_PATH}/*.mm)
    source_group(${GROUP_PATH} FILES ${MAIN_OTHER_SOURCES})
else()
    file(GLOB MAIN_OTHER_SOURCES ${MODULE_PATH}/*.h ${MODULE_PATH}/*.cpp)
    source_group(${GROUP_PATH} FILES ${MAIN_OTHER_SOURCES})
endif()

    file(GLOB_RECURSE SUB_SOURCES ${MODULE_PATH}/**/*.cpp ${MODULE_PATH}/**/*.h)
    set_source_files_properties(${SUB_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

    foreach(SOURCE ${SUB_SOURCES})
        file(RELATIVE_PATH FILE_PATH_REL "${MODULE_PATH}" "${SOURCE}")
        get_filename_component(DIR_PATH_REL ${FILE_PATH_REL} DIRECTORY)

        string(REPLACE "/" "\\\\" SUBGROUP ${DIR_PATH_REL})
        source_group(${GROUP_PATH}\\${SUBGROUP} FILES ${SOURCE})
    endforeach()

    target_sources(${TARGET} PRIVATE ${SUB_SOURCES})
endfunction()

create_module_group(Brioche "Modules\\juce_core" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_core")
create_module_group(Brioche "Modules\\juce_graphics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_graphics")
create_module_group(Brioche "Modules\\juce_events" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_events")
create_module_group(Brioche "Modules\\juce_gui_basics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_gui_basics")
create_module_group(Brioche "Modules\\juce_gui_extra" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_gui_extra")
create_module_group(Brioche "Modules\\juce_data_structures" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_data_structures")
create_module_group(Brioche "Modules\\juce_audio_formats" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_formats")
create_module_group(Brioche "Modules\\juce_audio_basics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_basics")
create_module_group(Brioche "Modules\\juce_audio_devices" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_devices")

set(BriochesSources ${MiscSources} ${ZoomSources} ${PluginSources} ${AnalyzerSources} ${GroupSources} ${DocumentSources} ${ApplicationSources})
target_sources(Brioche PRIVATE ${GlobalSources} ${BriochesSources} ${TranslationsSources})
target_sources(BriocheTest PRIVATE ${BriochesSources} ${SOURCES_DIRECTORY}/Test.cpp)

set(BriochesDefinitions
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_ALLOW_STATIC_NULL_VARIABLES=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_LOG_ASSERTIONS=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0)

target_compile_definitions(Brioche PRIVATE ${BriochesDefinitions})
target_compile_definitions(BriocheTest PRIVATE ${BriochesDefinitions})

file(GLOB ImagesDataSources ${BINARYDATA_DIRECTORY}/Images/MagicWord.jpeg)
juce_add_binary_data(ImagesData HEADER_NAME "ImagesData.h" NAMESPACE ImagesData SOURCES ${ImagesDataSources})

file(GLOB IconsDataSources ${BINARYDATA_DIRECTORY}/Icons/*.png)
juce_add_binary_data(IconsData HEADER_NAME "IconsData.h" NAMESPACE IconsData SOURCES ${IconsDataSources})

file(GLOB TranslationsDataSources ${TranslationsSources})
juce_add_binary_data(TranslationsData HEADER_NAME "TranslationsData.h" NAMESPACE TranslationsData SOURCES ${TranslationsDataSources})

file(GLOB FontsDataSources ${BINARYDATA_DIRECTORY}/Fonts/*.otf)
juce_add_binary_data(FontsData HEADER_NAME "FontsData.h" NAMESPACE FontsData SOURCES ${FontsDataSources})

target_link_libraries(Brioche PRIVATE
    vamp
    ImagesData
    IconsData
    FontsData
    TranslationsData
    juce::juce_gui_extra
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

target_link_libraries(BriocheTest PRIVATE
    vamp
    ImagesData
    IconsData
    FontsData
    TranslationsData
    juce::juce_gui_extra
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

enable_testing()
add_test(NAME AllTest COMMAND BriocheTest)
