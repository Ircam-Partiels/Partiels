cmake_minimum_required(VERSION 3.12)

set(IGNORE_VAMP_PLUGIN_TESTER false CACHE STRING "Disables the tests with vamp plugin tester")

project(PartielsVampPlugins VERSION 1.0.0 LANGUAGES C CXX)

include(../xcode-warnings.cmake)

file(GLOB PARTIELS_VAMP_PLUGINS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/AnlVampPlugins.h ${CMAKE_CURRENT_SOURCE_DIR}/AnlVampPlugins.cpp)
source_group("sources" FILES ${PARTIELS_VAMP_PLUGINS_SOURCES})
add_library(partiels-vamp-plugins SHARED ${PARTIELS_VAMP_PLUGINS_SOURCES})
target_compile_definitions(partiels-vamp-plugins PRIVATE PARTIELS_VAMP_PLUGINS_VERSION=${PROJECT_VERSION_MAJOR})
ive_prepare_plugin_target(partiels-vamp-plugins)
set_target_properties(partiels-vamp-plugins PROPERTIES XCODE_ATTRIBUTE_SKIP_INSTALL "YES")

add_custom_command(TARGET partiels-vamp-plugins POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/partiels-vamp-plugins.cat "$<IF:$<CONFIG:Debug>,${CMAKE_CURRENT_BINARY_DIR}/Debug/partiels-vamp-plugins.cat,${CMAKE_CURRENT_BINARY_DIR}/Release/partiels-vamp-plugins.cat>")

if(NOT IGNORE_VAMP_PLUGIN_TESTER)
  enable_testing()
  if(APPLE)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester)
      file(DOWNLOAD "https://nubo.ircam.fr/index.php/s/Pe6A2nf5eDELqBZ/download/vamp-plugin-tester-1.1-osx.tar.gz" "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz")
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  elseif(UNIX)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester)
      file(DOWNLOAD "https://nubo.ircam.fr/index.php/s/66TzgHfHTf69DFz/download/vamp-plugin-tester-1.1-linux64.tar.gz" "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz")
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  elseif(WIN32)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester.exe)
      file(DOWNLOAD "https://nubo.ircam.fr/index.php/s/5s2rM5qwLQ7aBZ8/download/vamp-plugin-tester-1.1a-win64.zip" "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.zip")
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.zip" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  endif()

  add_test(NAME VampPluginTester COMMAND ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester -a)
  set_tests_properties(VampPluginTester PROPERTIES ENVIRONMENT "$<IF:$<CONFIG:Debug>,VAMP_PATH=${CMAKE_CURRENT_BINARY_DIR}/Debug,VAMP_PATH=${CMAKE_CURRENT_BINARY_DIR}/Release>")
endif()
