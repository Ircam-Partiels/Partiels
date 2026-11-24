# Generates the file gathering the copyright notices and the license terms of the third-party software distributed with Partiels.
# The license texts are read from the dependencies so that the file follows their updates.

# Appends a dependency and its license texts to the output file.
# NAME: the name of the dependency.
# LICENSE: the SPDX identifier (or a short description) of the license.
# URL: the website of the dependency (optional).
# FILES: the files containing the license texts, relative to the source directory (optional).
# BEGIN/END: the markers delimiting the license text in the first file (optional).
function(partiels_append_third_party_license OUTPUT_FILE)
    cmake_parse_arguments(ARG "" "NAME;LICENSE;URL;BEGIN;END" "FILES" ${ARGN})
    string(REPEAT "=" 80 PARTIELS_SEPARATOR)
    file(APPEND ${OUTPUT_FILE} "${PARTIELS_SEPARATOR}\n${ARG_NAME} (${ARG_LICENSE})\n")
    if(ARG_URL)
        file(APPEND ${OUTPUT_FILE} "${ARG_URL}\n")
    endif()
    file(APPEND ${OUTPUT_FILE} "${PARTIELS_SEPARATOR}\n\n")
    foreach(LICENSE_FILE IN LISTS ARG_FILES)
        set(LICENSE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${LICENSE_FILE}")
        if(NOT EXISTS "${LICENSE_PATH}")
            message(FATAL_ERROR "Third-party license file not found: ${LICENSE_PATH}")
        endif()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${LICENSE_PATH}")
        file(READ "${LICENSE_PATH}" LICENSE_CONTENT)
        if(ARG_BEGIN)
            string(FIND "${LICENSE_CONTENT}" "${ARG_BEGIN}" BEGIN_POSITION)
            string(FIND "${LICENSE_CONTENT}" "${ARG_END}" END_POSITION)
            if(BEGIN_POSITION EQUAL -1 OR END_POSITION LESS_EQUAL BEGIN_POSITION)
                message(FATAL_ERROR "Third-party license markers not found in: ${LICENSE_PATH}")
            endif()
            math(EXPR LENGTH "${END_POSITION} - ${BEGIN_POSITION}")
            string(SUBSTRING "${LICENSE_CONTENT}" ${BEGIN_POSITION} ${LENGTH} LICENSE_CONTENT)
            unset(ARG_BEGIN)
        endif()
        string(STRIP "${LICENSE_CONTENT}" LICENSE_CONTENT)
        file(APPEND ${OUTPUT_FILE} "${LICENSE_CONTENT}\n\n")
    endforeach()
endfunction()

# Generates the file gathering the licenses of the third-party software distributed with Partiels.
function(partiels_generate_third_party_licenses OUTPUT_FILE)
    file(WRITE ${OUTPUT_FILE} "Partiels is free software distributed under the GNU Affero General Public License v3 (AGPL-3.0), see LICENSE.txt.\n")
    file(APPEND ${OUTPUT_FILE} "Its source code is available at https://github.com/Ircam-Partiels/Partiels.\n\n")
    file(APPEND ${OUTPUT_FILE} "Partiels includes the following third-party software. Their copyright notices and license terms are reproduced below.\n\n")

    set(JUCE_MODULES "JUCE/modules")
    set(LLAMA_VENDOR "Dependencies/llama.cpp/vendor")

    partiels_append_third_party_license(${OUTPUT_FILE} NAME "Vamp Plugin SDK" LICENSE "MIT-style" URL "https://github.com/vamp-plugins/vamp-plugin-sdk"
        FILES "Dependencies/vamp-plugin-sdk/COPYING")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "Ircam Vamp Extension" LICENSE "MIT" URL "https://github.com/Ircam-Partiels/ircam-vamp-extension"
        FILES "Dependencies/ircam-vamp-extension/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "JUCE" LICENSE "AGPL-3.0-only, used under the AGPL-3.0" URL "https://juce.com"
        FILES "JUCE/LICENSE.md")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "zlib (included in JUCE)" LICENSE "Zlib" URL "https://zlib.net"
        FILES "${JUCE_MODULES}/juce_core/zip/zlib/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "HarfBuzz (included in JUCE)" LICENSE "MIT" URL "https://harfbuzz.github.io"
        FILES "${JUCE_MODULES}/juce_graphics/fonts/harfbuzz/COPYING")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "SheenBidi (included in JUCE)" LICENSE "Apache-2.0" URL "https://github.com/Tehreer/SheenBidi"
        FILES "${JUCE_MODULES}/juce_graphics/unicode/sheenbidi/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "LunaSVG (included in JUCE)" LICENSE "MIT" URL "https://github.com/sammycage/lunasvg"
        FILES "${JUCE_MODULES}/juce_graphics/drawables/lunasvg/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "PlutoVG (included in JUCE)" LICENSE "MIT" URL "https://github.com/sammycage/plutovg"
        FILES "${JUCE_MODULES}/juce_graphics/drawables/lunasvg/plutovg/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "libpng (included in JUCE)" LICENSE "libpng-2.0" URL "http://www.libpng.org"
        FILES "${JUCE_MODULES}/juce_graphics/image_formats/pnglib/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "Independent JPEG Group's JPEG software (included in JUCE) - This software is based in part on the work of the Independent JPEG Group" LICENSE "IJG" URL "https://www.ijg.org"
        FILES "${JUCE_MODULES}/juce_graphics/image_formats/jpglib/README" BEGIN "LEGAL ISSUES\n============" END "REFERENCES\n==========")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "libwebp (included in JUCE)" LICENSE "BSD-3-Clause" URL "https://chromium.googlesource.com/webm/libwebp"
        FILES "${JUCE_MODULES}/juce_graphics/image_formats/libwebp/COPYING" "${JUCE_MODULES}/juce_graphics/image_formats/libwebp/PATENTS")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "FLAC (included in JUCE)" LICENSE "BSD-3-Clause" URL "https://xiph.org/flac"
        FILES "${JUCE_MODULES}/juce_audio_formats/codecs/flac/Flac Licence.txt")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "libogg (included in JUCE)" LICENSE "BSD-3-Clause" URL "https://xiph.org/ogg"
        FILES "${JUCE_MODULES}/juce_audio_formats/codecs/ogg/COPYING")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "libvorbis (included in JUCE)" LICENSE "BSD-3-Clause" URL "https://xiph.org/vorbis"
        FILES "${JUCE_MODULES}/juce_audio_formats/codecs/vorbis/COPYING")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "Opus (included in JUCE)" LICENSE "BSD-3-Clause" URL "https://opus-codec.org"
        FILES "${JUCE_MODULES}/juce_audio_formats/codecs/opus/opus/COPYING")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "opusfile (included in JUCE)" LICENSE "BSD-3-Clause" URL "https://opus-codec.org"
        FILES "${JUCE_MODULES}/juce_audio_formats/codecs/opus/opusfile/COPYING")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "libopusenc (included in JUCE)" LICENSE "BSD-3-Clause" URL "https://opus-codec.org"
        FILES "${JUCE_MODULES}/juce_audio_formats/codecs/opus/libopusenc/COPYING")
    if(WIN32)
        partiels_append_third_party_license(${OUTPUT_FILE} NAME "ASIO SDK (included in JUCE)" LICENSE "GPL-3.0-only, used under the GPL-3.0" URL "https://www.steinberg.net/developers"
            FILES "${JUCE_MODULES}/juce_audio_devices/native/asio/LICENSE.txt" "BinaryData/Licenses/GPL-3.0.txt")
        if(WebView2_root_dir)
            file(GLOB WEBVIEW2_LICENSE_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${WebView2_root_dir}/LICENSE*")
        endif()
        if(WEBVIEW2_LICENSE_FILES)
            partiels_append_third_party_license(${OUTPUT_FILE} NAME "Microsoft Edge WebView2 SDK" LICENSE "BSD-3-Clause" URL "https://developer.microsoft.com/microsoft-edge/webview2"
                FILES ${WEBVIEW2_LICENSE_FILES})
        else()
            message(WARNING "The license of the WebView2 SDK has not been found and is not included in the third-party licenses")
        endif()
    endif()
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "llama.cpp" LICENSE "MIT" URL "https://github.com/ggml-org/llama.cpp"
        FILES "Dependencies/llama.cpp/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "rotate-bits (included in llama.cpp)" LICENSE "MIT" URL "https://github.com/jb55/rotate-bits.h"
        FILES "${LLAMA_VENDOR}/hash/rotate-bits/LICENSE.md")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "xxHash (included in llama.cpp)" LICENSE "BSD-2-Clause" URL "https://github.com/Cyan4973/xxHash"
        FILES "${LLAMA_VENDOR}/hash/xxhash/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "JSON for Modern C++ (included in Partiels and llama.cpp)" LICENSE "MIT" URL "https://github.com/nlohmann/json"
        FILES "Dependencies/Misc/Dependencies/json/LICENSE.MIT")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "md4c" LICENSE "MIT" URL "https://github.com/mity/md4c"
        FILES "Dependencies/md4c/LICENSE.md")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "hnswlib" LICENSE "Apache-2.0" URL "https://github.com/nmslib/hnswlib"
        FILES "Dependencies/hnswlib/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "SDIF library by IRCAM (the source code is included in the source code of Partiels)" LICENSE "LGPL-2.1-or-later" URL "https://sourceforge.net/projects/sdif"
        FILES "Dependencies/sdif/COPYING")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "tinycolormap" LICENSE "MIT" URL "https://github.com/yuki-koyama/tinycolormap"
        FILES "Dependencies/tinycolormap/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "Magic Enum C++" LICENSE "MIT" URL "https://github.com/Neargye/magic_enum"
        FILES "Dependencies/Misc/Dependencies/magic_enum/LICENSE")
    partiels_append_third_party_license(${OUTPUT_FILE} NAME "Mermaid and its bundled dependencies" LICENSE "MIT, ISC, BSD-3-Clause, Apache-2.0, MPL-2.0 OR Apache-2.0, Unlicense" URL "https://mermaid.js.org"
        FILES "BinaryData/Neuralyzer/Mermaid/ThirdPartyLicenses.txt")

    string(REPEAT "=" 80 PARTIELS_SEPARATOR)
    file(APPEND ${OUTPUT_FILE} "${PARTIELS_SEPARATOR}\nPublic domain software\n${PARTIELS_SEPARATOR}\n\n")
    file(APPEND ${OUTPUT_FILE} "The following software included in llama.cpp is released into the public domain (for stb_image and miniaudio, the public domain alternative is used):\n")
    file(APPEND ${OUTPUT_FILE} "- stb_image by Sean Barrett (https://github.com/nothings/stb)\n")
    file(APPEND ${OUTPUT_FILE} "- miniaudio by David Reid (https://miniaud.io)\n")
    file(APPEND ${OUTPUT_FILE} "- subprocess.h by Neil Henning (https://github.com/sheredom/subprocess.h)\n")
    file(APPEND ${OUTPUT_FILE} "- SHA-1 in C by Steve Reid\n")
    file(APPEND ${OUTPUT_FILE} "- SHA-256 by Igor Pavlov\n")
endfunction()
