cmake_minimum_required(VERSION 3.18)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" CACHE STRING "" FORCE)

set(CMAKE_XCODE_GENERATE_SCHEME true)
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")

set(SDIF_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDIF_TESTS OFF CACHE BOOL "" FORCE)

set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

set(IFP_CLI ON CACHE STRING "" FORCE)
set(IFP_TEST OFF CACHE STRING "" FORCE)
set(IVE_PLUGIN_EXAMPLE ON CACHE STRING "" FORCE)

set(VAMP_SDK_PATH "" CACHE PATH "The path to the Vamp plugin SDK")
set(VAMP_SDK_TAG "d7ceb7d1d490674c93d334e5378108c4328e9e05" CACHE PATH "The tag of the Vamp plugin SDK")
set(VAMP_SDK_PATCH "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/vamp-plugin-sdk-src.patch")

set(TIMESTAMP_SERVER_URL "http://timestamp.sectigo.com" CACHE STRING "The timestamp server URL")

project(Partiels VERSION 2.0.4 LANGUAGES C CXX)

set(PARTIELS_CODESIGN_WINDOWS_KEYFILE "" CACHE PATH "The Windows (.pfx) certificate file")
set(PARTIELS_CODESIGN_WINDOWS_KEYPASSWORD "" CACHE STRING "The password of the Windows (.pfx) certificate file")

execute_process(COMMAND git log -1 --format=%h WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE PARTIELS_BUILD_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git status --porcelain WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE GIT_HAS_DIFF OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT GIT_HAS_DIFF STREQUAL "")
    set(PARTIELS_BUILD_ID "${PARTIELS_BUILD_ID}-head")
endif()

if(NOT PARTIELS_BUILD_TAG OR PARTIELS_BUILD_TAG STREQUAL "")
    set(PARTIELS_BUILD_TAG ${CMAKE_PROJECT_VERSION})
endif()
message(STATUS "Partiels: " ${PARTIELS_BUILD_TAG} " (" ${PARTIELS_BUILD_ID} ")")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.txt ${PARTIELS_BUILD_TAG})

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.txt "v${PARTIELS_BUILD_TAG}")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.txt "\n\n")
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/ChangeLog.txt PARTIELS_CHANGE_LOG)
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.txt ${PARTIELS_CHANGE_LOG})

file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/About.txt ${CMAKE_CURRENT_BINARY_DIR}/Install.txt)
file(READ ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.txt CHANGELOG_CONTENT)
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Install.txt "\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Install.txt ${CHANGELOG_CONTENT})

if(NOT EXISTS ${VAMP_SDK_PATH})
  include(FetchContent)
  find_package(Git REQUIRED)
  FetchContent_Declare(
    vamp-plugin-sdk
    GIT_REPOSITORY      https://github.com/vamp-plugins/vamp-plugin-sdk.git
    GIT_TAG             ${VAMP_SDK_TAG}
    PATCH_COMMAND       ${GIT_EXECUTABLE} reset --hard && ${GIT_EXECUTABLE} clean -fxd && ${GIT_EXECUTABLE} apply ${VAMP_SDK_PATCH}
    CONFIGURE_COMMAND   ""
    BUILD_COMMAND       ""
    INSTALL_COMMAND     ""
    UPDATE_DISCONNECTED TRUE
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(vamp-plugin-sdk)
  set(VAMP_SDK_PATH ${vamp-plugin-sdk_SOURCE_DIR} CACHE PATH "The path to the Vamp plugin SDK" FORCE)

  message(STATUS "Vamp Plugin SDK cloned in " ${VAMP_SDK_PATH})
endif()

add_subdirectory(JUCE)
add_subdirectory(Dependencies/ircam-vamp-extension)
add_subdirectory(Dependencies/sdif)
include(Dependencies/Misc/Misc.cmake)
add_subdirectory(VampPlugins)

juce_add_gui_app(Partiels
    PRODUCT_NAME                        "Partiels"
    VERSION                             ${CMAKE_PROJECT_VERSION}
    BUNDLE_ID                           fr.ircam.dev.partiels
    COMPANY_COPYRIGHT                   "Copyright 2024 Ircam. All rights reserved"
    COMPANY_NAME                        "Ircam"
    COMPANY_WEBSITE                     "https://www.ircam.fr/"
    ICON_BIG                            "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/icon.png"
    DOCUMENT_EXTENSIONS                 "ptldoc,aac,aiff,aif,flac,m4a,mp3,ogg,wav,wma"
    NEEDS_CURL                          FALSE
    NEEDS_WEB_BROWSER                   FALSE
    NEEDS_STORE_KIT                     FALSE
    HARDENED_RUNTIME_ENABLED            TRUE
    HARDENED_RUNTIME_OPTIONS            "com.apple.security.cs.disable-library-validation"
    PLIST_TO_MERGE                      "<plist><dict><key>LSApplicationCategoryType</key><string>public.app-category.music</string></dict></plist>"
    )

juce_generate_juce_header(Partiels)

set_target_properties(Partiels PROPERTIES CXX_STANDARD 20)
target_misc_setup(Partiels MISC_ZOOM_ENABLED MISC_TRANSPORT_ENABLED)

if(APPLE)
   file(GLOB_RECURSE PartielsSources
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.mm
        )
else()
   file(GLOB_RECURSE PartielsSources
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
        )
endif()
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${PartielsSources})

file(GLOB GlobalSources
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Main.cpp
    ${CMAKE_CACHEFILE_DIR}/Partiels_artefacts/JuceLibraryCode/JuceHeader.h
    )
source_group("Source/Global" FILES ${GlobalSources})
target_sources(Partiels PRIVATE ${GlobalSources} ${PartielsSources})

target_compile_definitions(Partiels PRIVATE
    APP_DOC_PREFIX="ptl"
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_MODAL_LOOPS_PERMITTED=0
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
    MISC_IGNORE_MODEL_ACCESS_MESSAGE_MANAGER_WEAK_ASSERT=1
    PARTIELS_BUILD_TAG="${PARTIELS_BUILD_TAG}"
    PARTIELS_BUILD_ID="${PARTIELS_BUILD_ID}"
    )

target_include_directories(Partiels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies)
if(WIN32)
  target_include_directories(Partiels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/steinberg-asio-sdk)
endif()

set(PARTIELS_BINARYDATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData)
file(GLOB TranslationsDataSources
    ${PARTIELS_BINARYDATA_DIRECTORY}/Translations/*.txt
    )
juce_add_binary_data(TranslationsData HEADER_NAME "TranslationsData.h" NAMESPACE TranslationsData SOURCES ${TranslationsDataSources})

file(GLOB PartielsResourceDataSources
    ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/About.txt
    ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/Ircam-logo.png
    ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/Ircam-logo-noir-RS.png
    ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/Pompidou-logo-noir-RS.png
    )
juce_add_binary_data(AnlResourceData HEADER_NAME "AnlResourceData.h" NAMESPACE AnlResourceData SOURCES ${PartielsResourceDataSources})

file(GLOB PartielsIconsDataSources ${PARTIELS_BINARYDATA_DIRECTORY}/Icons/*.png)
juce_add_binary_data(AnlIconsData HEADER_NAME "AnlIconsData.h" NAMESPACE AnlIconsData SOURCES ${PartielsIconsDataSources})

file(GLOB PartielsCursorsDataSources ${PARTIELS_BINARYDATA_DIRECTORY}/Cursors/*.png)
juce_add_binary_data(AnlCursorsData HEADER_NAME "AnlCursorsData.h" NAMESPACE AnlCursorsData SOURCES ${PartielsCursorsDataSources})

file(GLOB TestResultsDataSources
    ${PARTIELS_BINARYDATA_DIRECTORY}/Tests/*.csv
    ${PARTIELS_BINARYDATA_DIRECTORY}/Tests/*.lab
    ${PARTIELS_BINARYDATA_DIRECTORY}/Tests/*.json
    ${PARTIELS_BINARYDATA_DIRECTORY}/Tests/*.dat
    ${PARTIELS_BINARYDATA_DIRECTORY}/Tests/*.cue
    )
juce_add_binary_data(TestResultsData HEADER_NAME "TestResultsData.h" NAMESPACE TestResultsData SOURCES ${TestResultsDataSources})

target_link_libraries(Partiels PRIVATE
    ive_host_static
    sdif
    MiscData
    TranslationsData
    TestResultsData
    AnlResourceData
    AnlIconsData
    AnlCursorsData
    juce::juce_gui_extra
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_osc
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    )

if(DEFINED PARTIELS_PROVISIONING_PROFILE_SPECIFIER AND DEFINED PARTIELS_DEVELOPMENT_TEAM)
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${PARTIELS_DEVELOPMENT_TEAM}")
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${PARTIELS_PROVISIONING_PROFILE_SPECIFIER}")
elseif(DEFINED ENV{PARTIELS_PROVISIONING_PROFILE_SPECIFIER} AND DEFINED ENV{PARTIELS_DEVELOPMENT_TEAM})
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "$ENV{PARTIELS_DEVELOPMENT_TEAM}")
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "$ENV{PARTIELS_PROVISIONING_PROFILE_SPECIFIER}")
endif()
set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(Partiels PROPERTIES XCODE_SCHEME_ENVIRONMENT "VAMP_PATH=\$VAMP_PATH:/Library/Audio/Plug-Ins/Vamp:$HOME/Library/Audio/Plug-Ins/Vamp:${CMAKE_CURRENT_BINARY_DIR}/Debug:${CMAKE_CURRENT_BINARY_DIR}/Release")
target_enable_xcode_full_warnings(Partiels)

if(WIN32)
    target_compile_options(Partiels PRIVATE /wd4244)
elseif(APPLE)
    target_compile_options(Partiels PRIVATE -fno-aligned-allocation)
endif()

add_dependencies(Partiels partiels-vamp-plugins)

if(APPLE)
    set_target_properties(Partiels PROPERTIES XCODE_EMBED_PLUGINS partiels-vamp-plugins XCODE_EMBED_PLUGINS_CODE_SIGN_ON_COPY ON)
    file(COPY ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/FactoryTemplate.ptldoc DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Templates/)
    juce_add_bundle_resources_directory(Partiels ${CMAKE_CURRENT_BINARY_DIR}/Templates/)
else()
    add_custom_command(TARGET Partiels POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Partiels>/PlugIns"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:partiels-vamp-plugins>" "$<TARGET_FILE_DIR:Partiels>/PlugIns"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:partiels-vamp-plugins>/partiels-vamp-plugins.cat" "$<TARGET_FILE_DIR:Partiels>/PlugIns"
    )
    add_custom_command(TARGET Partiels POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Partiels>/Templates"
        COMMAND ${CMAKE_COMMAND} -E copy ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/FactoryTemplate.ptldoc "$<TARGET_FILE_DIR:Partiels>/Templates"
    )
endif()

### Packaging ###
if(WIN32)
    set(PARTIELS_PACKAGE_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}")
    file(MAKE_DIRECTORY ${PARTIELS_PACKAGE_OUTDIR})
    cmake_path(NATIVE_PATH PARTIELS_PACKAGE_OUTDIR PARTIELS_PACKAGE_DIR_NAT)
    set(PARTIELS_PACKAGE_OUTEXE "${PARTIELS_PACKAGE_OUTDIR}/Partiels-Windows.exe")
    cmake_path(NATIVE_PATH PARTIELS_PACKAGE_OUTEXE PARTIELS_PACKAGE_OUTEXE_NAT)
    set(PARTIELS_PACKAGE_TEMPDIR "${CMAKE_CURRENT_BINARY_DIR}/Package")
    cmake_path(NATIVE_PATH PARTIELS_PACKAGE_TEMPDIR PARTIELS_PACKAGE_TEMPDIR_NAT)
    set(PARTIELS_ISS_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/windows-package.iss")
    cmake_path(NATIVE_PATH PARTIELS_ISS_FILE PARTIELS_ISS_FILE_NAT)  
    set(PARTIELS_PACKAGE_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/PartielsPackage.bat")

    file(MAKE_DIRECTORY ${PARTIELS_PACKAGE_TEMPDIR})
    file(MAKE_DIRECTORY ${PARTIELS_PACKAGE_TEMPDIR}/PlugIns)
    file(MAKE_DIRECTORY ${PARTIELS_PACKAGE_TEMPDIR}/Templates)
    file(COPY ${CMAKE_CURRENT_BINARY_DIR}/Install.txt DESTINATION ${PARTIELS_PACKAGE_TEMPDIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/Ircam-logo-noir-RS.bmp DESTINATION ${PARTIELS_PACKAGE_TEMPDIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/About.txt DESTINATION ${PARTIELS_PACKAGE_TEMPDIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/ChangeLog.txt DESTINATION ${PARTIELS_PACKAGE_TEMPDIR})

    find_program(ISCC_EXE "iscc" HINTS "C:/Program Files (x86)/Inno Setup 6" REQUIRED)
    cmake_path(NATIVE_PATH ISCC_EXE ISCC_EXE_NAT)
    
    add_custom_command(TARGET Partiels POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Partiels>" "${PARTIELS_PACKAGE_TEMPDIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:partiels-vamp-plugins>" "${PARTIELS_PACKAGE_TEMPDIR}/PlugIns"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:partiels-vamp-plugins>/partiels-vamp-plugins.cat" "${PARTIELS_PACKAGE_TEMPDIR}/PlugIns"
        COMMAND ${CMAKE_COMMAND} -E copy ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/FactoryTemplate.ptldoc "${PARTIELS_PACKAGE_TEMPDIR}/Templates"
    )
    file(WRITE ${PARTIELS_PACKAGE_SCRIPT} "\"${ISCC_EXE_NAT}\" /DMyAppVerName=\"${PARTIELS_BUILD_TAG}\" /DMyBinaryDir=\"${PARTIELS_PACKAGE_TEMPDIR_NAT}\" /O\"${PARTIELS_PACKAGE_DIR_NAT}\" \"${PARTIELS_ISS_FILE_NAT}\"\n")

    if(PARTIELS_NOTARIZE)
        find_program(SIGNTOOL_EXE "signtool" HINTS "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64" REQUIRED)
        cmake_path(NATIVE_PATH SIGNTOOL_EXE SIGNTOOL_EXE_NAT)
        cmake_path(NATIVE_PATH PARTIELS_CODESIGN_WINDOWS_KEYFILE PARTIELS_CODESIGN_WINDOWS_KEYFILE_NAT)
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "\"${SIGNTOOL_EXE_NAT}\" sign /f \"${PARTIELS_CODESIGN_WINDOWS_KEYFILE_NAT}\" /p \"${PARTIELS_CODESIGN_WINDOWS_KEYPASSWORD}\" /fd SHA256 /td SHA256 /tr http://timestamp.sectigo.com \"${PARTIELS_PACKAGE_OUTEXE_NAT}\"\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "\"${SIGNTOOL_EXE_NAT}\" verify /pa \"${PARTIELS_PACKAGE_OUTEXE_NAT}\"\n")
    endif()

    add_custom_target(PartielsPackage ALL COMMAND ${PARTIELS_PACKAGE_SCRIPT})
    add_dependencies(PartielsPackage Partiels)
elseif(UNIX AND NOT APPLE)
    set(PARTIELS_PACKAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/Partiels)
    set(PARTIELS_PACKAGE_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/PartielsPackage.sh)
    file(WRITE ${PARTIELS_PACKAGE_SCRIPT} "#!/bin/sh\n\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "PARTIELS_BINARY_PATH=$1\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "rm -rf ${PARTIELS_PACKAGE_DIR}\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "mkdir -p ${PARTIELS_PACKAGE_DIR}\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp $PARTIELS_BINARY_PATH/Partiels ${PARTIELS_PACKAGE_DIR}\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp -r $PARTIELS_BINARY_PATH/PlugIns ${PARTIELS_PACKAGE_DIR}/PlugIns\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp -r $PARTIELS_BINARY_PATH/Templates ${PARTIELS_PACKAGE_DIR}/Templates\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/icon.png ${PARTIELS_PACKAGE_DIR}\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/Partiels.desktop ${PARTIELS_PACKAGE_DIR}\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/About.txt ${PARTIELS_PACKAGE_DIR}\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/ChangeLog.txt ${PARTIELS_PACKAGE_DIR}\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/linux-install.sh ${PARTIELS_PACKAGE_DIR}/install.sh\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/linux-uninstall.sh ${PARTIELS_PACKAGE_DIR}/uninstall.sh\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cd ${CMAKE_CURRENT_BINARY_DIR}\n")
    file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "tar zcvf Partiels-Linux.tar.gz Partiels\n")
    file(CHMOD ${PARTIELS_PACKAGE_SCRIPT} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    add_custom_target(PartielsPackage ALL COMMAND ${PARTIELS_PACKAGE_SCRIPT} $<TARGET_FILE_DIR:Partiels>)
    add_dependencies(PartielsPackage Partiels)
endif()

### Manual ###
set(PARTIELS_MANUAL_DIR ${CMAKE_CURRENT_BINARY_DIR}/Manual)
file(MAKE_DIRECTORY ${PARTIELS_MANUAL_DIR})

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/Docs/Partiels-Manual.md MANUAL_CONTENT)
string(REPLACE "APPVERSION" "${PARTIELS_BUILD_TAG} (${PARTIELS_BUILD_ID})" MANUAL_CONTENT ${MANUAL_CONTENT})
string(REPLACE "src=\"Images/" "src=\"${CMAKE_CURRENT_SOURCE_DIR}/Docs/Images/" MANUAL_CONTENT ${MANUAL_CONTENT})
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Manual.md ${MANUAL_CONTENT})
find_program(MDPDF_EXE "mdpdf")
if(MDPDF_EXE)
  add_custom_target(PartielsManual COMMAND ${MDPDF_EXE} ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Manual.md ${PARTIELS_MANUAL_DIR}/Partiels-Manual.pdf --debug VERBATIM)
else()
  message(STATUS "Manual target cannot be generated because mdpdf is not found")
endif()

### Format ###
find_program(CLANG_FORMAT_EXE "clang-format" HINTS "C:/Program Files/LLVM/bin")
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ClangFormatSources
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/VampPlugins/*.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/VampPlugins/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Misc/Source/*.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Misc/Source/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/ircam-vamp-extension/Source/*.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/ircam-vamp-extension/Source/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/ircam-vamp-extension/Example/*.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/ircam-vamp-extension/Example/*.h
    )
    add_custom_target(PartielsCheckFormat ${CLANG_FORMAT_EXE} --Werror --dry-run --verbose -style=file ${ClangFormatSources})
    add_custom_target(PartielsApplyFormat ${CLANG_FORMAT_EXE} -i -style=file ${ClangFormatSources})
else()
    message(WARNING "Clang Format targets cannot be generated because clang-format is not found")
endif()

### Publishing ###
set(PROJECT_URL "https://github.com/Ircam-Partiels/Partiels")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "## Downloads\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "- [Partiels-Linux.tar.gz](${PROJECT_URL}/releases/download/${PARTIELS_BUILD_TAG}/Partiels-Linux.tar.gz)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "- [Partiels-MacOS.dmg](${PROJECT_URL}/releases/download/${PARTIELS_BUILD_TAG}/Partiels-MacOS.dmg)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "- [Partiels-Windows.exe](${PROJECT_URL}/releases/download/${PARTIELS_BUILD_TAG}/Partiels-Windows.exe)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "- [Partiels-Manual.pdf](${PROJECT_URL}/releases/download/${PARTIELS_BUILD_TAG}/Partiels-Manual.pdf)\n") 

### Testing ###
enable_testing()

set(TESTS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Tests)

set(VAMP_EXAMPLES_DIRECTORY ${VAMP_SDK_PATH}/examples)
file(GLOB VAMP_EXAMPLE_PLUGINS_SOURCES ${VAMP_EXAMPLES_DIRECTORY}/*h  ${VAMP_EXAMPLES_DIRECTORY}/*cpp ${VAMP_EXAMPLES_DIRECTORY}/*c)
source_group("Vamp/Examples" FILES ${VAMP_EXAMPLE_PLUGINS_SOURCES})
add_library(vamp-example-plugins SHARED ${VAMP_EXAMPLE_PLUGINS_SOURCES})
vamp_prepare_plugin_target(vamp-example-plugins)
if(WIN32)
    target_compile_options(vamp-example-plugins PRIVATE /wd4244)
elseif(UNIX)
    target_compile_options(vamp-example-plugins PRIVATE -Wno-deprecated-declarations)
    target_compile_options(vamp-example-plugins PRIVATE -Wno-shorten-64-to-32)
endif()

add_test(NAME UnitTests COMMAND Partiels --unit-tests)

add_test(NAME Version COMMAND Partiels --version)

add_test(NAME Help COMMAND Partiels --help)

add_test(NAME ExportOptions COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/Xml/ --options=${TESTS_DIRECTORY}/exportOptions.xml)
set_tests_properties(ExportOptions PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

add_test(NAME ExportJpeg COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JPEG/ --format=jpeg --width=800 --height=600)
set_tests_properties(ExportJpeg PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

add_test(NAME ExportPng COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/PNG/ --format=png --width=1200 --height=900 --groups)
set_tests_properties(ExportPng PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

add_test(NAME ExportCsv COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/CSV/ --format=csv --nogrids --header --separator=,)
set_tests_properties(ExportCsv PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

add_test(NAME ExportJson COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/JSON/ --format=json)
set_tests_properties(ExportJson PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

add_test(NAME ExportCue COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/CUE/ --format=cue)
set_tests_properties(ExportCue PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

add_test(NAME ExportSdif COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/SDIF/ --format=sdif --frame=1TST --matrix=1TST --colname=Column)
set_tests_properties(ExportSdif PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

add_test(NAME NewDocument COMMAND Partiels --new --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/Document/new.ptldoc)
set_tests_properties(NewDocument PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

set(EXPECTED_CSV_FILE_SV "${TESTS_DIRECTORY}/SpectralCentroid-SV.csv")
set(EXPECTED_CSV_FILE_FAILURE "${TESTS_DIRECTORY}/SpectralCentroid-failure.csv")
set(GENERATED_CSV_FILE "${CMAKE_CURRENT_BINARY_DIR}/TestsOutput/CSV/Sound Group 1_Spectral Centroid.csv")
add_test(NAME CompareFilesSV COMMAND Partiels --compare-files ${EXPECTED_CSV_FILE_SV} ${GENERATED_CSV_FILE})
set_tests_properties(CompareFilesSV PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
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
