cmake_minimum_required(VERSION 3.12)

project(Analyse VERSION 0.0.1 LANGUAGES C CXX)

add_subdirectory(JUCE)
include(vamp.cmake)

juce_add_gui_app(Analyse
    PRODUCT_NAME                        "Brioche"
    VERSION                             "0.0.1"
    BUNDLE_ID                           com.ircam.analyse
    COMPANY_COPYRIGHT                   "Copyright 2020 Ircam. All rights reserved"
    COMPANY_NAME                        "Ircam"
    COMPANY_WEBSITE                     "https://www.ircam.fr/"
    ICON_BIG                            "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Icons/icon.png"
    DOCUMENT_EXTENSIONS                 anl
    NEEDS_CURL                          FALSE
    NEEDS_WEB_BROWSER                   FALSE)
    #HARDENED_RUNTIME_ENABLED)

juce_generate_juce_header(Analyse)

set_target_properties(Analyse PROPERTIES CXX_STANDARD 20)
set(SOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source)

file(GLOB_RECURSE GlobalSources
    ${SOURCES_DIRECTORY}/main.cpp
    ${CMAKE_CACHEFILE_DIR}/Analyse_artefacts/JuceLibraryCode/JuceHeader.h)

file(GLOB ToolsSources
    ${SOURCES_DIRECTORY}/Tools/AnlMisc.h
    ${SOURCES_DIRECTORY}/Tools/AnlListenerList.h
    ${SOURCES_DIRECTORY}/Tools/AnlAtomicManager.h
    ${SOURCES_DIRECTORY}/Tools/AnlModel.h
    ${SOURCES_DIRECTORY}/Tools/AnlModel.cpp
    ${SOURCES_DIRECTORY}/Tools/AnlModelAccessor.h
    ${SOURCES_DIRECTORY}/Tools/AnlSignalBroadcaster.h
    ${SOURCES_DIRECTORY}/Tools/AnlStringParser.cpp
    ${SOURCES_DIRECTORY}/Tools/AnlStringParser.h
    ${SOURCES_DIRECTORY}/Tools/AnlColouredPanel.cpp
    ${SOURCES_DIRECTORY}/Tools/AnlColouredPanel.h
    ${SOURCES_DIRECTORY}/Tools/AnlPropertyPanel.cpp
    ${SOURCES_DIRECTORY}/Tools/AnlPropertyPanel.h
    ${SOURCES_DIRECTORY}/Tools/AnlPropertyLayout.cpp
    ${SOURCES_DIRECTORY}/Tools/AnlPropertyLayout.h
    ${SOURCES_DIRECTORY}/Tools/AnlTooltip.cpp
    ${SOURCES_DIRECTORY}/Tools/AnlTooltip.h)

file(GLOB ZoomSources
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomStateModel.cpp
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomStateModel.h
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomStateRuler.cpp
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomStateRuler.h
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomStateScrollBar.cpp
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomStateScrollBar.h
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomStateTimeRuler.cpp
    ${SOURCES_DIRECTORY}/Zoom/AnlZoomStateTimeRuler.h)

file(GLOB PluginSources
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListModel.cpp
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListModel.h
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListScanner.cpp
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListScanner.h
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListTable.cpp
    ${SOURCES_DIRECTORY}/Plugin/AnlPluginListTable.h)

file(GLOB AnalyzerSources
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerModel.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerModel.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerPluginInstance.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerPluginInstance.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerProcessor.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerProcessor.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerPropertyPanel.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerPropertyPanel.h
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerThumbnail.cpp
    ${SOURCES_DIRECTORY}/Analyzer/AnlAnalyzerThumbnail.h)

file(GLOB DocumentSources
    ${SOURCES_DIRECTORY}/Document/AnlDocumentModel.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentModel.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileBased.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileBased.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentAudioReader.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentAudioReader.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentControlPanel.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentControlPanel.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentTransport.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentTransport.h
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileInfoPanel.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentFileInfoPanel.h)

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
    ${SOURCES_DIRECTORY}/Application/AnlApplicationLookAndFeel.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationLookAndFeel.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInstance.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInstance.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationWindow.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationWindow.h)

source_group("Global" FILES ${GlobalSources})
source_group("Tools" FILES ${ToolsSources})
source_group("Zoom" FILES ${ZoomSources})
source_group("Application" FILES ${ApplicationSources})
source_group("Document" FILES ${DocumentSources})
source_group("Analyzer" FILES ${AnalyzerSources})
source_group("Plugin" FILES ${PluginSources})

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

create_module_group(Analyse "Modules\\juce_core" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_core")
create_module_group(Analyse "Modules\\juce_graphics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_graphics")
create_module_group(Analyse "Modules\\juce_events" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_events")
create_module_group(Analyse "Modules\\juce_gui_basics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_gui_basics")
create_module_group(Analyse "Modules\\juce_gui_extra" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_gui_extra")
create_module_group(Analyse "Modules\\juce_data_structures" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_data_structures")
create_module_group(Analyse "Modules\\juce_audio_formats" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_formats")
create_module_group(Analyse "Modules\\juce_audio_basics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_basics")
create_module_group(Analyse "Modules\\juce_audio_devices" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_audio_devices")

target_sources(Analyse PRIVATE ${ToolsSources} ${ZoomSources} ${PluginSources} ${AnalyzerSources} ${DocumentSources} ${ApplicationSources} ${GlobalSources})

target_compile_definitions(Analyse PRIVATE
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_ALLOW_STATIC_NULL_VARIABLES=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_LOG_ASSERTIONS=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

set(BINARYDATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData)
file(GLOB BinaryDataSources
    ${BINARYDATA_DIRECTORY}/Icons/icon.png
    ${BINARYDATA_DIRECTORY}/Translations/Fr.txt)

juce_add_binary_data(AnalyseData SOURCES ${BinaryDataSources})
# juce_add_bundle_resources_directory(Analyse ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Translations)

target_link_libraries(Analyse PRIVATE
    vamp
    AnalyseData
    juce::juce_gui_extra
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
