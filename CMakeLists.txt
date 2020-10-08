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

file(GLOB GlobalSources
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp
    ${CMAKE_CACHEFILE_DIR}/Analyse_artefacts/JuceLibraryCode/JuceHeader.h)

file(GLOB ApplicationSources
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationInterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationLookAndFeel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationLookAndFeel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationHeader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationHeader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationInstance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationInstance.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Application/AnlApplicationWindow.h)

source_group("Global" FILES ${GlobalSources})
source_group("Application" FILES ${ApplicationSources})

create_module_group(Analyse "Modules\\ilf_core" "${CMAKE_CURRENT_SOURCE_DIR}/ILF/ilf_core")
create_module_group(Analyse "Modules\\ilf_audio_sequencer" "${CMAKE_CURRENT_SOURCE_DIR}/ILF/ilf_audio_sequencer")
create_module_group(Analyse "Modules\\juce_core" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_core")
create_module_group(Analyse "Modules\\juce_graphics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_graphics")
create_module_group(Analyse "Modules\\juce_events" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_events")
create_module_group(Analyse "Modules\\juce_gui_basics" "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/modules/juce_gui_basics")

target_sources(Analyse PRIVATE ${GlobalSources} ${ApplicationSources})

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
