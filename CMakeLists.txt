cmake_minimum_required(VERSION 3.18)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" CACHE STRING "")

set(CMAKE_XCODE_GENERATE_SCHEME true)
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")

set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

option(PARTIELS_MANUAL_ONLY "Only generate documentation targets (PartielsManual, PartielsPatchs) and release metadata" OFF)

if(NOT PARTIELS_MANUAL_ONLY)
    set(SDIF_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDIF_TESTS OFF CACHE BOOL "" FORCE)

    set(IFP_CLI ON CACHE STRING "" FORCE)
    set(IFP_TEST OFF CACHE STRING "" FORCE)
    set(IVE_PLUGIN_EXAMPLE ON CACHE STRING "" FORCE)

    set(VAMP_SDK_PATH "" CACHE PATH "The path to the Vamp plugin SDK")
    set(VAMP_SDK_TAG "d7ceb7d1d490674c93d334e5378108c4328e9e05" CACHE PATH "The tag of the Vamp plugin SDK")
    set(VAMP_SDK_PATCH "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/vamp-plugin-sdk-src.patch")

    set(TIMESTAMP_SERVER_URL "http://timestamp.sectigo.com" CACHE STRING "The timestamp server URL")
    set(PARTIELS_CODESIGN_WINDOWS_KEYFILE "" CACHE PATH "The Windows (.pfx) certificate file")
    set(PARTIELS_CODESIGN_WINDOWS_KEYPASSWORD "" CACHE STRING "The password of the Windows (.pfx) certificate file")
    option(PARTIELS_NOTARIZE "Code-sign and notarize the Windows package after building" OFF)
endif()

project(Partiels VERSION 2.3.0 LANGUAGES C CXX)

### Version ###
execute_process(COMMAND git log -1 --format=%h WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE PARTIELS_BUILD_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git status --porcelain WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE GIT_HAS_DIFF OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT GIT_HAS_DIFF STREQUAL "")
    set(PARTIELS_BUILD_ID "${PARTIELS_BUILD_ID}-head")
endif()

if(NOT PARTIELS_BUILD_TAG OR PARTIELS_BUILD_TAG STREQUAL "")
    set(PARTIELS_BUILD_TAG ${CMAKE_PROJECT_VERSION})
endif()
message(STATUS "Partiels: " ${PARTIELS_BUILD_TAG} " (" ${PARTIELS_BUILD_ID} ")")
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${PARTIELS_BUILD_TAG})
if(DEFINED CMAKE_MATCH_1 AND DEFINED CMAKE_MATCH_2 AND DEFINED CMAKE_MATCH_3)
    set(PARTIELS_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(PARTIELS_VERSION_MINOR ${CMAKE_MATCH_2})
    set(PARTIELS_VERSION_PATCH ${CMAKE_MATCH_3})
else()
    message(FATAL_ERROR "PARTIELS_BUILD_TAG is not in the format X.Y.Z")
endif()
set(PARTIELS_VERSION_STRING "${PARTIELS_VERSION_MAJOR}.${PARTIELS_VERSION_MINOR}.${PARTIELS_VERSION_PATCH}")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.txt ${PARTIELS_BUILD_TAG})

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.txt "v${PARTIELS_BUILD_TAG}")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.txt "\n\n")
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/ChangeLog.txt PARTIELS_CHANGE_LOG)
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.txt ${PARTIELS_CHANGE_LOG})

file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/About.txt ${CMAKE_CURRENT_BINARY_DIR}/Install.txt)
file(READ ${CMAKE_CURRENT_BINARY_DIR}/ChangeLog.txt CHANGELOG_CONTENT)
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Install.txt "\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Install.txt ${CHANGELOG_CONTENT})

### Application ###
if(NOT PARTIELS_MANUAL_ONLY)
    if(NOT EXISTS ${VAMP_SDK_PATH})
        include(FetchContent)
        find_package(Git REQUIRED)
        FetchContent_Declare(
            vamp-plugin-sdk
            GIT_REPOSITORY      https://github.com/vamp-plugins/vamp-plugin-sdk.git
            GIT_TAG             ${VAMP_SDK_TAG}
            PATCH_COMMAND       ${GIT_EXECUTABLE} reset --hard && ${GIT_EXECUTABLE} clean -fxd && ${GIT_EXECUTABLE} apply ${VAMP_SDK_PATCH}
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
        VERSION                             ${PARTIELS_VERSION_STRING}
        BUNDLE_ID                           fr.ircam.dev.partiels
        COMPANY_COPYRIGHT                   "Copyright 2024 Ircam. All rights reserved"
        COMPANY_NAME                        "Ircam"
        COMPANY_WEBSITE                     "https://www.ircam.fr/"
        ICON_BIG                            "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/icon_square.png"
        DOCUMENT_EXTENSIONS                 "ptldoc,aac,aiff,aif,flac,m4a,mp3,ogg,wav,wma"
        NEEDS_CURL                          FALSE
        NEEDS_WEB_BROWSER                   FALSE
        NEEDS_STORE_KIT                     FALSE
        HARDENED_RUNTIME_ENABLED            TRUE
        HARDENED_RUNTIME_OPTIONS            "com.apple.security.cs.disable-library-validation"
        PLIST_TO_MERGE                      "<plist><dict><key>LSApplicationCategoryType</key><string>public.app-category.music</string><key>NSAppTransportSecurity</key><dict><key>NSExceptionDomains</key><dict><key>vamp-plugins.org</key><dict><key>NSExceptionAllowsInsecureHTTPLoads</key><true/><key>NSIncludesSubdomains</key><true/><key>NSExceptionRequiresForwardSecrecy</key><false/><key>NSRequiresCertificateTransparency</key><false/></dict></dict></dict></dict></plist>"
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
        JUCE_LOG_ASSERTIONS=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_ENABLE_REPAINT_DEBUGGING=0
        JUCE_ALSA=1
        JUCE_WASAPI=1
        JUCE_DIRECTSOUND=1
        JUCE_ASIO=1
        MISC_IGNORE_MODEL_ACCESS_MESSAGE_MANAGER_WEAK_ASSERT=1
        PARTIELS_BUILD_TAG="${PARTIELS_BUILD_TAG}"
        PARTIELS_BUILD_ID="${PARTIELS_BUILD_ID}"
        )
    if(UNIX AND NOT APPLE)
        target_compile_definitions(Partiels PRIVATE 
        JUCE_JACK=1
        JUCE_USE_MP3AUDIOFORMAT=1
        )
    endif()

    target_include_directories(Partiels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies)
    if(WIN32)
    target_include_directories(Partiels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/steinberg-asio-sdk)
    endif()

    set(PARTIELS_BINARYDATA_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData)

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
        message(STATUS "Partiels Code-signing enabled")
        set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${PARTIELS_DEVELOPMENT_TEAM}")
        set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
        set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${PARTIELS_PROVISIONING_PROFILE_SPECIFIER}")
    elseif(DEFINED ENV{PARTIELS_PROVISIONING_PROFILE_SPECIFIER} AND DEFINED ENV{PARTIELS_DEVELOPMENT_TEAM})
        message(STATUS "Partiels Code-signing enabled")
        set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "$ENV{PARTIELS_DEVELOPMENT_TEAM}")
        set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
        set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "$ENV{PARTIELS_PROVISIONING_PROFILE_SPECIFIER}")
    endif()
    set_target_properties(Partiels PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
    set_target_properties(Partiels PROPERTIES XCODE_SCHEME_ENVIRONMENT "VAMP_PATH=\$VAMP_PATH:/Library/Audio/Plug-Ins/Vamp:$HOME/Library/Audio/Plug-Ins/Vamp:${CMAKE_CURRENT_BINARY_DIR}/Debug:${CMAKE_CURRENT_BINARY_DIR}/Release")

    include(${PARTIELS_BINARYDATA_DIRECTORY}/Resource/XcodeSetupWarnings.cmake)
    target_enable_xcode_full_warnings(Partiels)
    target_enable_xcode_full_warnings(partiels-vamp-plugins)

    if(WIN32)
        target_compile_options(Partiels PRIVATE /wd4244)
    elseif(APPLE)
        target_compile_options(Partiels PRIVATE -fno-aligned-allocation)
    endif()

    add_dependencies(Partiels partiels-vamp-plugins)

    if(WIN32)
        file(COPY ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/generate-translation-file.ps1 DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Scripts/)
    else()
        file(COPY ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/generate-translation-file.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Scripts/)
    endif()

    if(APPLE)
        set_target_properties(Partiels PROPERTIES XCODE_EMBED_PLUGINS partiels-vamp-plugins XCODE_EMBED_PLUGINS_CODE_SIGN_ON_COPY ON)
        file(COPY ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/FactoryTemplate.ptldoc DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Templates/)
        file(COPY ${PARTIELS_BINARYDATA_DIRECTORY}/Preset/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Preset/)
        file(COPY ${PARTIELS_BINARYDATA_DIRECTORY}/Translations/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Translations/)
        juce_add_bundle_resources_directory(Partiels ${CMAKE_CURRENT_BINARY_DIR}/Templates/)
        juce_add_bundle_resources_directory(Partiels ${CMAKE_CURRENT_BINARY_DIR}/Preset/)
        juce_add_bundle_resources_directory(Partiels ${CMAKE_CURRENT_BINARY_DIR}/Translations/)
        juce_add_bundle_resources_directory(Partiels ${CMAKE_CURRENT_BINARY_DIR}/Scripts/)
    else()
        add_custom_command(TARGET Partiels POST_BUILD 
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Partiels>/PlugIns"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:partiels-vamp-plugins>" "$<TARGET_FILE_DIR:Partiels>/PlugIns"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:partiels-vamp-plugins>/partiels-vamp-plugins.cat" "$<TARGET_FILE_DIR:Partiels>/PlugIns"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Partiels>/Preset"
            COMMAND ${CMAKE_COMMAND} -E copy ${PARTIELS_BINARYDATA_DIRECTORY}/Preset/Partiels.trackpresets.settings "$<TARGET_FILE_DIR:Partiels>/Preset"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Partiels>/Templates"
            COMMAND ${CMAKE_COMMAND} -E copy ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/FactoryTemplate.ptldoc "$<TARGET_FILE_DIR:Partiels>/Templates"
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${PARTIELS_BINARYDATA_DIRECTORY}/Translations/ "$<TARGET_FILE_DIR:Partiels>/Translations/"
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/Scripts/ "$<TARGET_FILE_DIR:Partiels>/Scripts/"
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
        file(MAKE_DIRECTORY ${PARTIELS_PACKAGE_TEMPDIR}/Preset)
        file(MAKE_DIRECTORY ${PARTIELS_PACKAGE_TEMPDIR}/Templates)
        file(MAKE_DIRECTORY ${PARTIELS_PACKAGE_TEMPDIR}/Translations)
        file(COPY ${CMAKE_CURRENT_BINARY_DIR}/Install.txt DESTINATION ${PARTIELS_PACKAGE_TEMPDIR})
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/Ircam-logo-noir-RS.bmp DESTINATION ${PARTIELS_PACKAGE_TEMPDIR})
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/About.txt DESTINATION ${PARTIELS_PACKAGE_TEMPDIR})
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/ChangeLog.txt DESTINATION ${PARTIELS_PACKAGE_TEMPDIR})
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Translations/ DESTINATION ${PARTIELS_PACKAGE_TEMPDIR}/Translations/)

        find_program(ISCC_EXE "iscc" HINTS "C:/Program Files (x86)/Inno Setup 6" REQUIRED)
        cmake_path(NATIVE_PATH ISCC_EXE ISCC_EXE_NAT)
        
        add_custom_command(TARGET Partiels POST_BUILD 
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Partiels>" "${PARTIELS_PACKAGE_TEMPDIR}"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:partiels-vamp-plugins>" "${PARTIELS_PACKAGE_TEMPDIR}/PlugIns"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:partiels-vamp-plugins>/partiels-vamp-plugins.cat" "${PARTIELS_PACKAGE_TEMPDIR}/PlugIns"
            COMMAND ${CMAKE_COMMAND} -E copy "${PARTIELS_BINARYDATA_DIRECTORY}/Preset/Partiels.trackpresets.settings" "${PARTIELS_PACKAGE_TEMPDIR}/Preset"
            COMMAND ${CMAKE_COMMAND} -E copy "${PARTIELS_BINARYDATA_DIRECTORY}/Resource/FactoryTemplate.ptldoc" "${PARTIELS_PACKAGE_TEMPDIR}/Templates"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/Scripts/" "${PARTIELS_PACKAGE_TEMPDIR}/Scripts/"
        )
        file(WRITE ${PARTIELS_PACKAGE_SCRIPT} "\"${ISCC_EXE_NAT}\" /DMyAppVerName=\"${PARTIELS_BUILD_TAG}\" /DMyBinaryDir=\"${PARTIELS_PACKAGE_TEMPDIR_NAT}\" /O\"${PARTIELS_PACKAGE_DIR_NAT}\" \"${PARTIELS_ISS_FILE_NAT}\"\n")

        if(PARTIELS_NOTARIZE)
            file(GLOB_RECURSE SIGNTOOL_CANDIDATES
            "$ENV{ProgramFiles}/Windows Kits/10/bin/*/x64/signtool.exe"
            "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/*/x64/signtool.exe"
            )
            foreach(SIGNTOOL_CANDIDATE IN LISTS SIGNTOOL_CANDIDATES)
                get_filename_component(SIGNTOOL_CANDIDATE_DIR ${SIGNTOOL_CANDIDATE} DIRECTORY)
                list(APPEND SIGNTOOL_CANDIDATE_DIRS ${SIGNTOOL_CANDIDATE_DIR})
            endforeach()
            find_program(SIGNTOOL_EXE "signtool" HINTS ${SIGNTOOL_CANDIDATE_DIRS} REQUIRED)
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
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp -r $PARTIELS_BINARY_PATH/Preset ${PARTIELS_PACKAGE_DIR}/Preset\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp -r $PARTIELS_BINARY_PATH/Templates ${PARTIELS_PACKAGE_DIR}/Templates\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp -r $PARTIELS_BINARY_PATH/Translations ${PARTIELS_PACKAGE_DIR}/Translations\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp -r $PARTIELS_BINARY_PATH/Scripts ${PARTIELS_PACKAGE_DIR}/Scripts\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/icon.png ${PARTIELS_PACKAGE_DIR}\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/Partiels.desktop ${PARTIELS_PACKAGE_DIR}\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/About.txt ${PARTIELS_PACKAGE_DIR}\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/ChangeLog.txt ${PARTIELS_PACKAGE_DIR}\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cp ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/linux-install.sh ${PARTIELS_PACKAGE_DIR}/Partiels-install.sh\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "cd ${CMAKE_CURRENT_BINARY_DIR}\n")
        file(APPEND ${PARTIELS_PACKAGE_SCRIPT} "tar zcvf Partiels-Linux.tar.gz Partiels\n")
        file(CHMOD ${PARTIELS_PACKAGE_SCRIPT} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

        add_custom_target(PartielsPackage ALL COMMAND ${PARTIELS_PACKAGE_SCRIPT} $<TARGET_FILE_DIR:Partiels>)
        add_dependencies(PartielsPackage Partiels)
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

    ### Translation Key Extraction ###
    add_custom_target(PartielsExtractTranslationKeys COMMAND ${CMAKE_COMMAND} -DTRANSLATION_KEY_FILE="${CMAKE_CURRENT_BINARY_DIR}/Scripts/TranslationKeys.txt" -P ${PARTIELS_BINARYDATA_DIRECTORY}/Resource/TranslationKeysExtractor.cmake WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_dependencies(Partiels PartielsExtractTranslationKeys)
    
endif() # NOT PARTIELS_MANUAL_ONLY

### Manual ###
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/Docs/Partiels-Manual.md MANUAL_CONTENT)
string(REPLACE "APPVERSION" "${PARTIELS_BUILD_TAG} (${PARTIELS_BUILD_ID})" MANUAL_CONTENT ${MANUAL_CONTENT})
file(RELATIVE_PATH PARTIELS_DOCS_REL ${CMAKE_CURRENT_BINARY_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/Docs")
string(REPLACE "src=\"Images/" "src=\"${PARTIELS_DOCS_REL}/Images/" MANUAL_CONTENT ${MANUAL_CONTENT})
file(RELATIVE_PATH PARTIELS_BIN_REL ${CMAKE_CURRENT_BINARY_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData")
string(REPLACE "src=\"../BinaryData/" "src=\"${PARTIELS_BIN_REL}/" MANUAL_CONTENT ${MANUAL_CONTENT})
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Manual.md ${MANUAL_CONTENT})
find_program(MDPDF_EXE "mdpdf")
if(MDPDF_EXE)
    add_custom_target(PartielsManual COMMAND ${MDPDF_EXE} Partiels-Manual.md Partiels-Manual.pdf --style=${CMAKE_CURRENT_SOURCE_DIR}/Docs/style.css --gh-style VERBATIM WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
else()
    message(STATUS "Manual target cannot be generated because mdpdf is not found")
endif()
add_custom_target(PartielsPatchs COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_CURRENT_BINARY_DIR}/Partiels-Patchs.zip" --format=zip "*" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Patchs/.")

### Publishing ###
set(PROJECT_URL "https://github.com/Ircam-Partiels/Partiels")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "## Downloads\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "### Partiels: [Linux](${PROJECT_URL}/releases/download/${PARTIELS_BUILD_TAG}/Partiels-Linux.tar.gz) / [MacOS](${PROJECT_URL}/releases/download/${PARTIELS_BUILD_TAG}/Partiels-MacOS.dmg) / [Windows](${PROJECT_URL}/releases/download/${PARTIELS_BUILD_TAG}/Partiels-Windows.exe) / [Manual](${PROJECT_URL}/releases/download/${PARTIELS_BUILD_TAG}/Partiels-Manual.pdf) / [Patchs](${PROJECT_URL}/releases/download/${PARTIELS_BUILD_TAG}/Partiels-Patchs.zip)\n") 

file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "### Vamp Plugins:\n")

set(IVP_URL "https://github.com/Ircam-Partiels/ircam-vamp-plugins")
set(IVP_TAG "2.1.0")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "- Ircam Vamp Plugins: [Linux](${IVP_URL}/releases/download/${IVP_TAG}/Ircam-Vamp-Plugins-Linux.zip) / [MacOS](${IVP_URL}/releases/download/${IVP_TAG}/Ircam-Vamp-Plugins-MacOS.zip) / [Windows](${IVP_URL}/releases/download/${IVP_TAG}/Ircam-Vamp-Plugins-Windows.zip) / [Manual](${IVP_URL}/releases/download/${IVP_TAG}/Ircam-Vamp-Plugins-Manuals.zip)\n")

set(VAX_TAG "1.0.0")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "- VAX Vamp Plugins: [Linux](${IVP_URL}/releases/download/${IVP_TAG}/VAX-Vamp-Plugin-v${VAX_TAG}-Linux.zip) / [MacOS](${IVP_URL}/releases/download/${IVP_TAG}/VAX-Vamp-Plugin-v${VAX_TAG}-MacOS.zip) / [Windows](${IVP_URL}/releases/download/${IVP_TAG}/VAX-Vamp-Plugin-v${VAX_TAG}-Windows.zip) / [Manual](${IVP_URL}/releases/download/${IVP_TAG}/VAX-Vamp-Plugin-v${VAX_TAG}-Manual.zip)\n")

set(CREPE_URL "https://github.com/Ircam-Partiels/crepe-vamp-plugin")
set(CREPE_TAG "3.0.0")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "- Crepe Vamp Plugin: [Linux](${CREPE_URL}/releases/download/${CREPE_TAG}/Crepe-Linux.tar.gz) / [MacOS](${CREPE_URL}/releases/download/${CREPE_TAG}/Crepe-MacOS.pkg) / [Windows](${CREPE_URL}/releases/download/${CREPE_TAG}/Crepe-Windows.exe) / [Manual](${CREPE_URL}/releases/download/${CREPE_TAG}/Crepe-Manual.zip)\n") 

set(WHISPER_URL "https://github.com/Ircam-Partiels/whisper-vamp-plugin")
set(WHISPER_TAG "3.0.0")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Partiels-Release.md "- Whisper Vamp Plugin: [Linux](${WHISPER_URL}/releases/download/${WHISPER_TAG}/Whisper-Linux.tar.gz) / [MacOS](${WHISPER_URL}/releases/download/${WHISPER_TAG}/Whisper-MacOS.pkg) / [Windows](${WHISPER_URL}/releases/download/${WHISPER_TAG}/Whisper-Windows.exe) / [Manual](${WHISPER_URL}/releases/download/${WHISPER_TAG}/Whisper-Manual.zip)\n") 

### Testing ###
if(NOT PARTIELS_MANUAL_ONLY)
    enable_testing()

    set(TESTS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Tests)
    set(TESTS_EXPECTED_DIRECTORY ${TESTS_DIRECTORY}/Expected)
    set(TESTS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/TestsOutput)
    set(TESTS_SPECTRUM_FILE_NAME "Group 1_Simple Power Spectrum")
    set(TESTS_CENTROID_FILE_NAME "Group 1_Spectral Centroid")
    set(TESTS_FOLLOWER_FILE_NAME "Group 2_Amplitude Follower")
    set(TESTS_ONSET_FILE_NAME "Group 2_Simple Percussion Onset Detector")

    find_package(ImageMagick COMPONENTS compare QUIET)
    if(NOT ImageMagick_compare_FOUND)
        find_program(ImageMagick_EXECUTABLE "magick" HINTS ${ImageMagick_DIR})
        if(ImageMagick_EXECUTABLE)
            set(ImageMagick_ARGS "compare")
        else()
            message(NOTICE "ImageMagick compare tool not found. Please install ImageMagick with the compare component to compare images during tests.")
        endif()
    else()
        set(ImageMagick_EXECUTABLE ${ImageMagick_compare_EXECUTABLE})
        set(ImageMagick_ARGS "")
    endif()

    if(APPLE)
        set(ImageMagick_THRESHOLD "0.001")
    elseif(WIN32)
        set(ImageMagick_THRESHOLD "0.05")
    else()
        set(ImageMagick_THRESHOLD "0.05")
    endif()

    function(add_compare_image_test test_name test_name_ext filename format extension)
        if(ImageMagick_EXECUTABLE)
            add_test(NAME ${test_name}${test_name_ext} COMMAND ${CMAKE_COMMAND} -DMAGICKEXE=${ImageMagick_EXECUTABLE} -DMAGICKARGS=${ImageMagick_ARGS} -DIMAGE1=${TESTS_EXPECTED_DIRECTORY}/${format}/${filename}.${extension} -DIMAGE2=${TESTS_OUTPUT_DIRECTORY}/${format}/${filename}.${extension} -DTHRESHOLD=${ImageMagick_THRESHOLD} -P ${TESTS_DIRECTORY}/CompareImages.cmake)
            set_tests_properties(${test_name}${test_name_ext} PROPERTIES DEPENDS "${test_name}")
        endif()
    endfunction(add_compare_image_test)

    function(add_compare_image_tests test_name format extension)
        add_compare_image_test(${test_name} "CompareSpectrum" "${TESTS_SPECTRUM_FILE_NAME}" ${format} ${extension})
        add_compare_image_test(${test_name} "CompareCentroid" "${TESTS_CENTROID_FILE_NAME}" ${format} ${extension})
        add_compare_image_test(${test_name} "CompareFollower" "${TESTS_FOLLOWER_FILE_NAME}" ${format} ${extension})
        add_compare_image_test(${test_name} "CompareOnset" "${TESTS_ONSET_FILE_NAME}" ${format} ${extension})
        add_compare_image_test("${test_name}Group" "Compare1" "Group 1" ${format} ${extension})
        add_compare_image_test("${test_name}Group" "Compare2" "Group 2" ${format} ${extension})
    endfunction(add_compare_image_tests) 

    function(add_compare_text_test test_name test_name_ext filename format extension)
        add_test(NAME ${test_name}${test_name_ext} COMMAND ${CMAKE_COMMAND} -E compare_files "${TESTS_EXPECTED_DIRECTORY}/${format}/${filename}.${extension}" "${TESTS_OUTPUT_DIRECTORY}/${format}/${filename}.${extension}")
        set_tests_properties(${test_name}${test_name_ext} PROPERTIES DEPENDS "${test_name}")
    endfunction(add_compare_text_test)

    function(add_compare_text_tests test_name format extension)
        add_compare_text_test(${test_name} "CompareSpectrum" "${TESTS_SPECTRUM_FILE_NAME}" ${format} ${extension})
        add_compare_text_test(${test_name} "CompareCentroid" "${TESTS_CENTROID_FILE_NAME}" ${format} ${extension})
        add_compare_text_test(${test_name} "CompareOnset" "${TESTS_ONSET_FILE_NAME}" ${format} ${extension})
    endfunction(add_compare_text_tests)

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
    set_tests_properties(Version PROPERTIES PASS_REGULAR_EXPRESSION "${PARTIELS_BUILD_TAG}")

    add_test(NAME Help COMMAND Partiels --help)

    add_test(NAME CleanDirectory COMMAND ${CMAKE_COMMAND} -E remove_directory ${TESTS_OUTPUT_DIRECTORY})

    add_test(NAME ExportEmbeddedPlugins COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/FactoryTemplate.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/JPEG/ --format=jpeg --width=800 --height=600)

    add_test(NAME ExportDocument COMMAND Partiels --export --document=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/Document/ --format=jpeg --width=800 --height=600)
    set_tests_properties(ExportDocument PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportDocumentWithFileUnix COMMAND Partiels --export --document=${TESTS_DIRECTORY}/TemplateWithFileUnix.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/Document/ --format=csv --nogrids --header --separator=,)
    set_tests_properties(ExportDocumentWithFileUnix PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportDocumentWithFileWindows COMMAND Partiels --export --document=${TESTS_DIRECTORY}/TemplateWithFileWindows.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CSV/ --format=csv --nogrids --header --separator=,)
    set_tests_properties(ExportDocumentWithFileWindows PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportDocumentWithInvalidFile COMMAND Partiels --export --document=${TESTS_DIRECTORY}/TemplateWithInvalidFile.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CSV/ --format=csv --nogrids --header --separator=,)
    set_tests_properties(ExportDocumentWithInvalidFile PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    set_tests_properties(ExportDocumentWithInvalidFile PROPERTIES WILL_FAIL TRUE)

    add_test(NAME ExportDocumentWithFileExtraCsv COMMAND Partiels --export --document=${TESTS_DIRECTORY}/TemplateWithFileExtra.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CSV/ --format=csv --nogrids --header --separator=,)
    set_tests_properties(ExportDocumentWithFileExtraCsv PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_test(NAME CompareFilesExtraCsvCsv COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/PointsWithExtra.csv "${TESTS_OUTPUT_DIRECTORY}/CSV/Group 1_PointsWithExtraCSV.csv")
    set_tests_properties(CompareFilesExtraCsvCsv PROPERTIES DEPENDS "ExportDocumentWithFileExtraCsv")
    add_test(NAME CompareFilesExtraCsvJson COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/PointsWithExtra.csv "${TESTS_OUTPUT_DIRECTORY}/CSV/Group 1_PointsWithExtraJSON.csv")
    set_tests_properties(CompareFilesExtraCsvJson PROPERTIES DEPENDS "ExportDocumentWithFileExtraCsv")

    add_test(NAME ExportDocumentWithFileExtraThresholdsCsv COMMAND Partiels --export --document=${TESTS_DIRECTORY}/TemplateWithFileExtra.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CSVFiltered/ --format=csv --nogrids --header --separator=, --thresholds)
    set_tests_properties(ExportDocumentWithFileExtraThresholdsCsv PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_test(NAME CompareFilesExtraThresholdsCsvCsv COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/PointsWithExtraFiltered.csv "${TESTS_OUTPUT_DIRECTORY}/CSVFiltered/Group 1_PointsWithExtraCSV.csv")
    set_tests_properties(CompareFilesExtraThresholdsCsvCsv PROPERTIES DEPENDS "ExportDocumentWithFileExtraThresholdsCsv")
    add_test(NAME CompareFilesExtraThresholdsCsvJson COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/PointsWithExtraFiltered.csv "${TESTS_OUTPUT_DIRECTORY}/CSVFiltered/Group 1_PointsWithExtraJSON.csv")
    set_tests_properties(CompareFilesExtraThresholdsCsvJson PROPERTIES DEPENDS "ExportDocumentWithFileExtraThresholdsCsv")

    add_test(NAME ExportDocumentWithFileExtraJson COMMAND Partiels --export --document=${TESTS_DIRECTORY}/TemplateWithFileExtra.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/JSON/ --format=json --nogrids --header --separator=,)
    set_tests_properties(ExportDocumentWithFileExtraJson PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_test(NAME CompareFilesExtraJsonCsv COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/PointsWithExtra.json "${TESTS_OUTPUT_DIRECTORY}/JSON/Group 1_PointsWithExtraCSV.json")
    set_tests_properties(CompareFilesExtraJsonCsv PROPERTIES DEPENDS "ExportDocumentWithFileExtraJson")
    add_test(NAME CompareFilesExtraJsonJson COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/PointsWithExtra.json "${TESTS_OUTPUT_DIRECTORY}/JSON/Group 1_PointsWithExtraJSON.json")
    set_tests_properties(CompareFilesExtraJsonJson PROPERTIES DEPENDS "ExportDocumentWithFileExtraJson")

    add_test(NAME ExportDocumentWithFileExtraThresholdsJson COMMAND Partiels --export --document=${TESTS_DIRECTORY}/TemplateWithFileExtra.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/JSONFiltered/ --format=json --nogrids --header --separator=, --thresholds)
    set_tests_properties(ExportDocumentWithFileExtraThresholdsJson PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_test(NAME CompareFilesExtraThresholdsJsonCsv COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/PointsWithExtraFiltered.json "${TESTS_OUTPUT_DIRECTORY}/JSONFiltered/Group 1_PointsWithExtraCSV.json")
    set_tests_properties(CompareFilesExtraThresholdsJsonCsv PROPERTIES DEPENDS "ExportDocumentWithFileExtraThresholdsJson")
    add_test(NAME CompareFilesExtraThresholdsJsonJson COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/PointsWithExtraFiltered.json "${TESTS_OUTPUT_DIRECTORY}/JSONFiltered/Group 1_PointsWithExtraJSON.json")
    set_tests_properties(CompareFilesExtraThresholdsJsonJson PROPERTIES DEPENDS "ExportDocumentWithFileExtraThresholdsJson")

    add_test(NAME ExportOptions COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/Xml/ --options=${TESTS_DIRECTORY}/exportOptions.xml)
    set_tests_properties(ExportOptions PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportJpeg COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/JPEG/ --format=jpeg --width=800 --height=600)
    set_tests_properties(ExportJpeg PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_test(NAME ExportJpegGroup COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/JPEG/ --format=jpeg --width=800 --height=600  --groups)
    set_tests_properties(ExportJpegGroup PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_compare_image_tests(ExportJpeg "JPEG" "jpeg")

    add_test(NAME ExportPng COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/PNG/ --format=png --width=1200 --height=900 )
    set_tests_properties(ExportPng PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_test(NAME ExportPngGroup COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/PNG/ --format=png --width=1200 --height=900 --groups)
    set_tests_properties(ExportPngGroup PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_compare_image_tests(ExportPng "PNG" "png")

    add_test(NAME ExportCsv COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CSV/ --format=csv --nogrids --header --separator=,)
    set_tests_properties(ExportCsv PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportCsvWithFileUnix COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/TemplateWithFileUnix.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CSV/ --format=csv --nogrids --header --separator=,)
    set_tests_properties(ExportCsvWithFileUnix PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportCsvWithFileWindows COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/TemplateWithFileWindows.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CSV/ --format=csv --nogrids --header --separator=,)
    set_tests_properties(ExportCsvWithFileWindows PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportCsvWithFileNoColour COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/TemplateWithFileNoColour.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CSV/ --format=csv --nogrids --header --separator=,)
    set_tests_properties(ExportCsvWithFileNoColour PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportCsvWithInvalidFile COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/TemplateWithInvalidFile.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CSV/ --format=csv --nogrids --header --separator=,)
    set_tests_properties(ExportCsvWithInvalidFile PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    set_tests_properties(ExportCsvWithInvalidFile PROPERTIES WILL_FAIL TRUE)

    add_test(NAME ExportJson COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/JSON/ --format=json)
    set_tests_properties(ExportJson PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_compare_text_tests(ExportJson "JSON" "json")

    add_test(NAME ExportPureData COMMAND Partiels --export --document=${TESTS_DIRECTORY}/TemplateWithFileExtra.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/PUREDATA/ --format=puredata)
    set_tests_properties(ExportPureData PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_compare_text_test(ExportPureData "CompareCSV" "Group 1_PointsWithExtraCSV" "PUREDATA" "txt")
    add_compare_text_test(ExportPureData "CompareJSON" "Group 1_PointsWithExtraJSON" "PUREDATA" "txt")

    add_test(NAME ExportMax COMMAND Partiels --export --document=${TESTS_DIRECTORY}/TemplateWithFileExtra.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/MAX/ --format=max)
    set_tests_properties(ExportMax PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_compare_text_test(ExportMax "CompareCSV" "Group 1_PointsWithExtraCSV" "MAX" "txt")
    add_compare_text_test(ExportMax "CompareJSON" "Group 1_PointsWithExtraJSON" "MAX" "txt")

    add_test(NAME ExportCue COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/CUE/ --format=cue)
    set_tests_properties(ExportCue PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportSdif COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/SDIF/ --format=sdif --frame=1TST --matrix=1TST --colname=Column)
    set_tests_properties(ExportSdif PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    add_test(NAME ExportFail COMMAND Partiels --export --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template-Failure.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/Xml/ --options=${TESTS_DIRECTORY}/exportOptions.xml)
    set_tests_properties(ExportFail PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    set_tests_properties(ExportFail PROPERTIES WILL_FAIL TRUE)

    add_test(NAME NewDocument COMMAND Partiels --new --input=${TESTS_DIRECTORY}/Sound.wav --template=${TESTS_DIRECTORY}/Template.ptldoc --output=${TESTS_OUTPUT_DIRECTORY}/Document/new.ptldoc)
    set_tests_properties(NewDocument PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")

    set(EXPECTED_CSV_FILE_SV "${TESTS_DIRECTORY}/SpectralCentroid-SV.csv")
    set(EXPECTED_CSV_FILE_FAILURE "${TESTS_DIRECTORY}/SpectralCentroid-failure.csv")
    set(GENERATED_CSV_FILE "${TESTS_OUTPUT_DIRECTORY}/CSV/Group 1_Spectral Centroid.csv")
    add_test(NAME CompareFilesSV COMMAND Partiels --compare-files ${EXPECTED_CSV_FILE_SV} ${GENERATED_CSV_FILE})
    set_tests_properties(CompareFilesSV PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    add_test(NAME CompareFilesFailure COMMAND Partiels --compare-files ${EXPECTED_CSV_FILE_FAILURE} ${GENERATED_CSV_FILE})
    set_tests_properties(CompareFilesFailure PROPERTIES WILL_FAIL TRUE)

    function(parse_expected_results expected_file output_content)
        file(READ "${expected_file}" output_content)
        string(REPLACE "\[" "\\[" output_content "${output_content}")
        string(REPLACE "\]" "\\]" output_content "${output_content}")
        string(REPLACE "\<" "\\<" output_content "${output_content}")
        string(REPLACE "\>" "\\>" output_content "${output_content}")
        string(REPLACE "\:" "\\:" output_content "${output_content}")
        string(REPLACE "\?" "\\?" output_content "${output_content}")
        string(REGEX REPLACE "\"details\":.*" "\"details\":.*" output_content "${output_content}")
        string(REGEX REPLACE "\"details\"=.*" "\"details\"=.*" output_content "${output_content}")
    endfunction()

    parse_expected_results("${TESTS_DIRECTORY}/PluginList.json" PLUGIN_LIST_JSON_CONTENT)
    add_test(NAME PluginListJson COMMAND Partiels --plugin-list --format=json)
    set_tests_properties(PluginListJson PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    set_tests_properties(PluginListJson PROPERTIES PASS_REGULAR_EXPRESSION "${PLUGIN_LIST_JSON_CONTENT}")

    parse_expected_results("${TESTS_DIRECTORY}/PluginList.xml" PLUGIN_LIST_XML_CONTENT)
    add_test(NAME PluginListXml COMMAND Partiels --plugin-list --format=xml)
    set_tests_properties(PluginListXml PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    set_tests_properties(PluginListXml PROPERTIES PASS_REGULAR_EXPRESSION "${PLUGIN_LIST_XML_CONTENT}")

    parse_expected_results("${TESTS_DIRECTORY}/PluginListDescription.json" PLUGIN_LIST_DESCRIPTION_JSON_CONTENT)
    add_test(NAME PluginListDescriptionJson COMMAND Partiels --plugin-list --description --format=json)
    set_tests_properties(PluginListDescriptionJson PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    set_tests_properties(PluginListDescriptionJson PROPERTIES PASS_REGULAR_EXPRESSION "${PLUGIN_LIST_DESCRIPTION_JSON_CONTENT}")

    parse_expected_results("${TESTS_DIRECTORY}/PluginListDescription.xml" PLUGIN_LIST_DESCRIPTION_XML_CONTENT)
    add_test(NAME PluginListDescriptionXml COMMAND Partiels --plugin-list --description --format=xml)
    set_tests_properties(PluginListDescriptionXml PROPERTIES ENVIRONMENT "VAMP_PATH=$<TARGET_FILE_DIR:vamp-example-plugins>")
    set_tests_properties(PluginListDescriptionXml PROPERTIES PASS_REGULAR_EXPRESSION "${PLUGIN_LIST_DESCRIPTION_XML_CONTENT}")

    add_test(NAME ConvToSdifMarkers COMMAND Partiels --json2sdif --input=${TESTS_DIRECTORY}/Markers.json --output=${TESTS_OUTPUT_DIRECTORY}/SDIF/Markers.sdif --frame=1MKR --matrix=1MKR)
    add_test(NAME ConvFromSdifMarkers COMMAND Partiels --sdif2json --input=${TESTS_OUTPUT_DIRECTORY}/SDIF/Markers.sdif --output=${TESTS_OUTPUT_DIRECTORY}/JSON/Markers.json --frame=1MKR --matrix=1MKR)
    set_tests_properties(ConvFromSdifMarkers PROPERTIES DEPENDS "ConvToSdifMarkers")
    add_test(NAME ConvCompareMarkers1 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Markers.json ${TESTS_OUTPUT_DIRECTORY}/JSON/Markers.json)
    set_tests_properties(ConvCompareMarkers1 PROPERTIES DEPENDS "ConvFromSdifMarkers")
    add_test(NAME ConvCompareMarkers2 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Markers.json ${TESTS_OUTPUT_DIRECTORY}/SDIF/Markers.sdif ${TESTS_DIRECTORY}/MarkersArgs.xml)
    set_tests_properties(ConvCompareMarkers2 PROPERTIES DEPENDS "ConvToSdifMarkers")

    add_test(NAME ConvToSdifPoints COMMAND Partiels --json2sdif --input=${TESTS_DIRECTORY}/Points.json --output=${TESTS_OUTPUT_DIRECTORY}/SDIF/Points.sdif --frame=1PNT --matrix=1PNT)
    add_test(NAME ConvFromSdifPoints COMMAND Partiels --sdif2json --input=${TESTS_OUTPUT_DIRECTORY}/SDIF/Points.sdif --output=${TESTS_OUTPUT_DIRECTORY}/JSON/Points.json --frame=1PNT --matrix=1PNT)
    set_tests_properties(ConvFromSdifPoints PROPERTIES DEPENDS "ConvToSdifPoints")
    add_test(NAME ConvComparePoints1 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Points.json ${TESTS_OUTPUT_DIRECTORY}/JSON/Points.json)
    set_tests_properties(ConvComparePoints1 PROPERTIES DEPENDS "ConvFromSdifPoints")
    add_test(NAME ConvComparePoints2 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Points.json ${TESTS_OUTPUT_DIRECTORY}/SDIF/Points.sdif ${TESTS_DIRECTORY}/PointsArgs.xml)
    set_tests_properties(ConvComparePoints2 PROPERTIES DEPENDS "ConvFromSdifPoints")

    add_test(NAME ConvToSdifColumns COMMAND Partiels --json2sdif --input=${TESTS_DIRECTORY}/Columns.json --output=${TESTS_OUTPUT_DIRECTORY}/SDIF/Columns.sdif --frame=1COL --matrix=1COL)
    add_test(NAME ConvFromSdifColumns COMMAND Partiels --sdif2json --input=${TESTS_OUTPUT_DIRECTORY}/SDIF/Columns.sdif --output=${TESTS_OUTPUT_DIRECTORY}/JSON/Columns.json --frame=1COL --matrix=1COL)
    set_tests_properties(ConvFromSdifColumns PROPERTIES DEPENDS "ConvToSdifColumns")
    add_test(NAME ConvCompareColumns1 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Columns.json ${TESTS_OUTPUT_DIRECTORY}/JSON/Columns.json)
    set_tests_properties(ConvCompareColumns1 PROPERTIES DEPENDS "ConvFromSdifColumns")
    add_test(NAME ConvCompareColumns2 COMMAND Partiels --compare-files ${TESTS_DIRECTORY}/Columns.json ${TESTS_OUTPUT_DIRECTORY}/SDIF/Columns.sdif ${TESTS_DIRECTORY}/ColumnsArgs.xml)
    set_tests_properties(ConvCompareColumns2 PROPERTIES DEPENDS "ConvFromSdifColumns")

endif() # NOT PARTIELS_MANUAL_ONLY
