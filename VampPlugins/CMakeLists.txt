cmake_minimum_required(VERSION 3.12)

set(CMAKE_XCODE_GENERATE_SCHEME true)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" CACHE STRING "" FORCE)
set(SDIF_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDIF_TESTS OFF CACHE BOOL "" FORCE)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

set(IGNORE_VAMP_PLUGIN_TESTER false CACHE STRING "Disables the tests with vamp plugin tester")

project(PartielsVampPlugins VERSION 1.0.0 LANGUAGES C CXX)

include(../vamp.cmake)
include(../xcode-warnings.cmake)

file(GLOB PARTIELS_VAMP_PLUGINS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/AnlVampPlugins.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnlVampPlugins.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../vamp-plugin-sdk/src/vamp-sdk/PluginAdapter.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/../vamp-plugin-sdk/src/vamp-sdk/RealTime.cpp  
    ${CMAKE_CURRENT_SOURCE_DIR}/../vamp-plugin-sdk/src/vamp-sdk/FFT.cpp
)
source_group("sources" FILES ${PARTIELS_VAMP_PLUGINS_SOURCES})
add_library(partiels-vamp-plugins SHARED ${PARTIELS_VAMP_PLUGINS_SOURCES})
set_target_properties(partiels-vamp-plugins PROPERTIES CXX_STANDARD 17)
set_target_properties(partiels-vamp-plugins PROPERTIES PREFIX "")
set_target_properties(partiels-vamp-plugins PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(partiels-vamp-plugins PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set_target_properties(partiels-vamp-plugins PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)
target_compile_definitions(partiels-vamp-plugins PRIVATE PARTIELS_VAMP_PLUGINS_VERSION=${PROJECT_VERSION_MAJOR})
target_include_directories(partiels-vamp-plugins PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../vamp-plugin-sdk)
if(WIN32)
    target_compile_definitions(partiels-vamp-plugins PRIVATE _USE_MATH_DEFINES)
    target_compile_options(partiels-vamp-plugins PRIVATE /wd4996 /wd4267)
    set_target_properties(partiels-vamp-plugins PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set_target_properties(partiels-vamp-plugins PROPERTIES LINK_FLAGS "/export:vampGetPluginDescriptor")
elseif(APPLE)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../vamp-plugin-sdk/src/vamp-sdk/PluginAdapter.cpp  PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../vamp-plugin-sdk/src/vamp-sdk/RealTime.cpp  PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../vamp-plugin-sdk/src/vamp-sdk/FFT.cpp  PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
elseif(UNIX)
    set_property(TARGET partiels-vamp-plugins PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
set_target_properties(partiels-vamp-plugins PROPERTIES XCODE_ATTRIBUTE_SKIP_INSTALL "YES")

add_custom_command(TARGET partiels-vamp-plugins POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/partiels-vamp-plugins.cat "$<IF:$<CONFIG:Debug>,${CMAKE_CURRENT_BINARY_DIR}/Debug/partiels-vamp-plugins.cat,${CMAKE_CURRENT_BINARY_DIR}/Release/partiels-vamp-plugins.cat>")

if(NOT IGNORE_VAMP_PLUGIN_TESTER)
  enable_testing()
  if(APPLE)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester)
      file(DOWNLOAD "https://code.soundsoftware.ac.uk/attachments/download/1590/vamp-plugin-tester-1.1-osx.tar.gz" "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz")
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  elseif(UNIX)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester)
      file(DOWNLOAD "https://code.soundsoftware.ac.uk/attachments/download/1589/vamp-plugin-tester-1.1-linux64.tar.gz" "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz")
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  elseif(WIN32)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester.exe)
      file(DOWNLOAD "https://code.soundsoftware.ac.uk/attachments/download/2331/vamp-plugin-tester-1.1a-win64.zip" "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.zip")
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.zip" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  endif()

  add_test(NAME VampPluginTester COMMAND ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester -a)
  set_tests_properties(VampPluginTester PROPERTIES ENVIRONMENT "$<IF:$<CONFIG:Debug>,VAMP_PATH=${CMAKE_CURRENT_BINARY_DIR}/Debug,VAMP_PATH=${CMAKE_CURRENT_BINARY_DIR}/Release>")
endif()
