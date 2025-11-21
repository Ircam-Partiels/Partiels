# CPack configuration for Partiels
# This file configures CPack to generate installers for Windows, Linux, and macOS
#
# Usage:
#   Windows: cpack -C <Config> -G WIX        # Generates .msi installer (requires WiX Toolset)
#            cpack -C <Config> -G NSIS       # Generates .exe installer (fallback)
#   Linux:   cpack -G TGZ                    # Generates .tar.gz archive (default for CI)
#            cpack -G DEB                    # Generates .deb package
#            cpack -G RPM                    # Generates .rpm package
#   macOS:   DMG creation remains in CI workflow (uses appdmg)
#
# The configuration uses component-based installation to separate runtime files from
# development files, ensuring only necessary files are packaged.

# Common CPack settings
set(CPACK_PACKAGE_NAME "Partiels")
set(CPACK_PACKAGE_VENDOR "Ircam")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Partiels - Audio Analysis Software")
set(CPACK_PACKAGE_VERSION "${PARTIELS_VERSION_STRING}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PARTIELS_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PARTIELS_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PARTIELS_VERSION_PATCH}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.ircam.fr/")
set(CPACK_PACKAGE_CONTACT "Ircam <support@ircam.fr>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_BINARY_DIR}/Install.txt")

# Only install runtime components, skip development files
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};Partiels;Runtime;/")

# Platform-specific configurations
if(WIN32)
    # Windows Configuration - Try WiX for MSI, fallback to NSIS
    # Check if WiX is available for MSI generation
    find_program(WIX_CANDLE_EXECUTABLE candle)
    if(WIX_CANDLE_EXECUTABLE)
        set(CPACK_GENERATOR "WIX;ZIP")
        message(STATUS "WiX found - using MSI generator")
        
        # WiX specific settings
        set(CPACK_WIX_UPGRADE_GUID "2BE88D38-04D3-44AE-B6F6-2D78BD410D58")
        set(CPACK_WIX_PRODUCT_GUID "*") # Generate new GUID for each version
        set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/icon_square.png")
        set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/Ircam-logo-noir-RS.bmp")
        set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/Ircam-logo-noir-RS.bmp")
        set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
        set(CPACK_WIX_PROPERTY_ARPHELPLINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
        set(CPACK_WIX_PROGRAM_MENU_FOLDER "Partiels")
        # Create start menu shortcut
        set(CPACK_WIX_CMAKE_PACKAGE_REGISTRY "Partiels")
    else()
        set(CPACK_GENERATOR "NSIS;ZIP")
        message(STATUS "WiX not found - using NSIS generator")
        
        # NSIS specific settings
        set(CPACK_NSIS_DISPLAY_NAME "Partiels")
        set(CPACK_NSIS_PACKAGE_NAME "Partiels")
        set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
        set(CPACK_NSIS_MODIFY_PATH OFF)
        set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/icon_square.png")
        set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/icon_square.png")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "Partiels.exe")
        set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
        set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
        set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
        set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Partiels.lnk' '\$INSTDIR\\\\Partiels.exe'")
        set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Partiels.lnk'")
    endif()
    
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Partiels")
    set(CPACK_PACKAGE_FILE_NAME "Partiels-Windows")
    
    # Install components for Windows
    install(TARGETS Partiels RUNTIME DESTINATION . COMPONENT Runtime)
    install(TARGETS partiels-vamp-plugins LIBRARY DESTINATION PlugIns RUNTIME DESTINATION PlugIns COMPONENT Runtime)
    install(FILES "$<TARGET_FILE_DIR:partiels-vamp-plugins>/partiels-vamp-plugins.cat" DESTINATION PlugIns COMPONENT Runtime)
    install(FILES "${PARTIELS_BINARYDATA_DIRECTORY}/Preset/Partiels.trackpresets.settings" DESTINATION Preset COMPONENT Runtime)
    install(FILES "${PARTIELS_BINARYDATA_DIRECTORY}/Resource/FactoryTemplate.ptldoc" DESTINATION Templates COMPONENT Runtime)
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Scripts/" DESTINATION Scripts COMPONENT Runtime)
    install(DIRECTORY "${PARTIELS_BINARYDATA_DIRECTORY}/Translations/" DESTINATION Translations COMPONENT Runtime)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/About.txt" DESTINATION . COMPONENT Runtime)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/ChangeLog.txt" DESTINATION . COMPONENT Runtime)
    
    # Code signing support (if enabled)
    if(PARTIELS_NOTARIZE AND PARTIELS_CODESIGN_WINDOWS_KEYFILE)
        # Find signtool for Windows code signing
        file(GLOB_RECURSE SIGNTOOL_CANDIDATES
            "$ENV{ProgramFiles}/Windows Kits/*/bin/*/x64/signtool.exe"
            "$ENV{ProgramFiles\(x86\)}/Windows Kits/*/bin/*/x64/signtool.exe"
        )
        foreach(SIGNTOOL_CANDIDATE IN LISTS SIGNTOOL_CANDIDATES)
            get_filename_component(SIGNTOOL_CANDIDATE_DIR ${SIGNTOOL_CANDIDATE} DIRECTORY)
            list(APPEND SIGNTOOL_CANDIDATE_DIRS ${SIGNTOOL_CANDIDATE_DIR})
        endforeach()
        find_program(SIGNTOOL_EXE "signtool" HINTS ${SIGNTOOL_CANDIDATE_DIRS})
        if(SIGNTOOL_EXE)
            # Configure post-build signing for the installer
            set(CPACK_POST_BUILD_SCRIPTS "${CMAKE_CURRENT_BINARY_DIR}/sign_installer.cmake")
            file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/sign_installer.cmake" 
                "execute_process(COMMAND \"${SIGNTOOL_EXE}\" sign /f \"${PARTIELS_CODESIGN_WINDOWS_KEYFILE}\" /p \"${PARTIELS_CODESIGN_WINDOWS_KEYPASSWORD}\" /fd SHA256 /td SHA256 /tr \"${TIMESTAMP_SERVER_URL}\" \"\${CPACK_PACKAGE_FILES}\")\n"
                "execute_process(COMMAND \"${SIGNTOOL_EXE}\" verify /pa \"\${CPACK_PACKAGE_FILES}\")\n"
            )
        endif()
    endif()
    
elseif(UNIX AND NOT APPLE)
    # Linux Configuration
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_PACKAGE_FILE_NAME "Partiels-Linux")
    
    # DEB specific settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libasound2, libjack0 | libjack-jackd2-0, libfreetype6, libfontconfig1, libx11-6, libxcomposite1, libxcursor1, libxext6, libxinerama1, libxrandr2, libxrender1, libgl1, libglu1-mesa")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
    
    # RPM specific settings
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0")
    set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_RPM_PACKAGE_REQUIRES "alsa-lib, jack-audio-connection-kit, freetype, fontconfig, libX11, libXcomposite, libXcursor, libXext, libXinerama, libXrandr, libXrender, mesa-libGL, mesa-libGLU")
    
    # Install components for Linux
    install(TARGETS Partiels RUNTIME DESTINATION opt/Partiels COMPONENT Runtime)
    install(TARGETS partiels-vamp-plugins LIBRARY DESTINATION opt/Partiels/PlugIns COMPONENT Runtime)
    install(FILES "$<TARGET_FILE_DIR:partiels-vamp-plugins>/partiels-vamp-plugins.cat" DESTINATION opt/Partiels/PlugIns COMPONENT Runtime)
    install(FILES "${PARTIELS_BINARYDATA_DIRECTORY}/Preset/Partiels.trackpresets.settings" DESTINATION opt/Partiels/Preset COMPONENT Runtime)
    install(FILES "${PARTIELS_BINARYDATA_DIRECTORY}/Resource/FactoryTemplate.ptldoc" DESTINATION opt/Partiels/Templates COMPONENT Runtime)
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Scripts/" DESTINATION opt/Partiels/Scripts COMPONENT Runtime)
    install(DIRECTORY "${PARTIELS_BINARYDATA_DIRECTORY}/Translations/" DESTINATION opt/Partiels/Translations COMPONENT Runtime)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/icon.png" DESTINATION opt/Partiels COMPONENT Runtime)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/Partiels.desktop" DESTINATION share/applications COMPONENT Runtime)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/About.txt" DESTINATION opt/Partiels COMPONENT Runtime)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/ChangeLog.txt" DESTINATION opt/Partiels COMPONENT Runtime)
    
    # For TGZ, create a self-contained archive with installer script
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/BinaryData/Resource/linux-install.sh" 
            DESTINATION opt/Partiels
            RENAME Partiels-install.sh
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
            COMPONENT Runtime)
    
elseif(APPLE)
    # macOS Configuration - Bundle resources only (DMG remains in CI)
    # Note: The app bundle is already created by JUCE, we just ensure resources are embedded
    
    # The Resources are already added to the bundle via juce_add_bundle_resources_directory
    # in the main CMakeLists.txt. This section is kept for potential future DMG generation
    # using CPack if desired.
    
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_FILE_NAME "Partiels-MacOS")
    set(CPACK_DMG_VOLUME_NAME "Partiels")
    set(CPACK_DMG_FORMAT "UDZO")
    
    # The Partiels.app is already configured to include all necessary resources
    # via the JUCE configuration in the main CMakeLists.txt
    
endif()

# Include CPack module to enable packaging
include(CPack)
