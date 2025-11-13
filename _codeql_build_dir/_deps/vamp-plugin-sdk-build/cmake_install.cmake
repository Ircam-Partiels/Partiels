# Install script for directory: /home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-build/libvamp-sdk.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-build/libvamp-hostsdk.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vamp" TYPE FILE FILES "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp/vamp.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vamp-sdk" TYPE FILE FILES
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-sdk/FFT.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-sdk/Plugin.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-sdk/PluginAdapter.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-sdk/PluginBase.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-sdk/RealTime.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-sdk/plugguard.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-sdk/vamp-sdk.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vamp-hostsdk" TYPE FILE FILES
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/Plugin.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/PluginBase.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/PluginBufferingAdapter.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/PluginChannelAdapter.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/PluginHostAdapter.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/PluginInputDomainAdapter.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/PluginLoader.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/PluginSummarisingAdapter.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/PluginWrapper.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/RealTime.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/host-c.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/hostguard.h"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-src/vamp-hostsdk/vamp-hostsdk.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-build/vamp.pc"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-build/vamp-sdk.pc"
    "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-build/vamp-hostsdk.pc"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/runner/work/Partiels/Partiels/_codeql_build_dir/_deps/vamp-plugin-sdk-build/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
