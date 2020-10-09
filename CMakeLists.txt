cmake_minimum_required(VERSION 3.12)

project(Analyse VERSION 0.0.1 LANGUAGES C CXX)

add_subdirectory(JUCE)
add_subdirectory(ILF)

juce_add_gui_app(Analyse
    PRODUCT_NAME                        "Analyse"
    VERSION                             "0.0.1"
    BUNDLE_ID                           com.ircam.analyse
    COMPANY_COPYRIGHT                   "Copyright 2020 Ircam. All rights reserved"
    COMPANY_NAME                        "Ircam"
    COMPANY_WEBSITE                     "https://www.ircam.fr/"
    ICON_BIG                            ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Icons/icon.png
    DOCUMENT_EXTENSIONS                 anl
    NEEDS_CURL                          FALSE
    NEEDS_WEB_BROWSER                   FALSE)
    #HARDENED_RUNTIME_ENABLED)

juce_generate_juce_header(Analyse)

set_target_properties(Analyse PROPERTIES CXX_STANDARD 17)
set(SOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source)

file(GLOB_RECURSE GlobalSources
    ${SOURCES_DIRECTORY}/main.cpp
    ${CMAKE_CACHEFILE_DIR}/Analyse_artefacts/JuceLibraryCode/JuceHeader.h)

file(GLOB MiscSources
    ${SOURCES_DIRECTORY}/Misc/AnlMisc.h)

file(GLOB ApplicationSources
    ${SOURCES_DIRECTORY}/Application/AnlApplicationModel.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationModel.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationController.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationController.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInterface.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInterface.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationLookAndFeel.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationLookAndFeel.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationHeader.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationHeader.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInstance.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationInstance.h
    ${SOURCES_DIRECTORY}/Application/AnlApplicationWindow.cpp
    ${SOURCES_DIRECTORY}/Application/AnlApplicationWindow.h)

file(GLOB DocumentSources
    ${SOURCES_DIRECTORY}/Document/AnlDocumentModel.cpp
    ${SOURCES_DIRECTORY}/Document/AnlDocumentModel.h)

source_group("Global" FILES ${GlobalSources})
source_group("Misc" FILES ${MiscSources})
source_group("Application" FILES ${ApplicationSources})
source_group("Document" FILES ${DocumentSources})

create_module_group(Analyse "Modules\\ilf_core" "${CMAKE_CURRENT_SOURCE_DIR}/ILF/ilf_core")
create_module_group(Analyse "Modules\\ilf_audio_sequencer" "${CMAKE_CURRENT_SOURCE_DIR}/ILF/ilf_audio_sequencer")
create_module_group(Analyse "Modules\\juce_core" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_core")
create_module_group(Analyse "Modules\\juce_graphics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_graphics")
create_module_group(Analyse "Modules\\juce_events" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_events")
create_module_group(Analyse "Modules\\juce_gui_basics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_gui_basics")

target_sources(Analyse PRIVATE ${GlobalSources} ${ApplicationSources} ${DocumentSources} ${MiscSources})

target_compile_definitions(Analyse PRIVATE
    JUCE_ALLOW_STATIC_NULL_VARIABLES=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_LOG_ASSERTIONS=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

juce_add_binary_data(AnalyseData SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Icons/icon.png)

target_link_libraries(Analyse PRIVATE
    AnalyseData
    juce::juce_gui_extra
    juce::juce_opengl
    ilf::ilf_core
    ilf::ilf_audio_sequencer
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
