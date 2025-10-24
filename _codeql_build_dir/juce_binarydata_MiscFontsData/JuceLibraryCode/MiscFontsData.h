/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace MiscFontsData
{
    extern const char*   NunitoSansBlack_ttf;
    const int            NunitoSansBlack_ttfSize = 93732;

    extern const char*   NunitoSansBlackItalic_ttf;
    const int            NunitoSansBlackItalic_ttfSize = 95404;

    extern const char*   NunitoSansBold_ttf;
    const int            NunitoSansBold_ttfSize = 93000;

    extern const char*   NunitoSansBoldItalic_ttf;
    const int            NunitoSansBoldItalic_ttfSize = 94772;

    extern const char*   NunitoSansExtraBold_ttf;
    const int            NunitoSansExtraBold_ttfSize = 92228;

    extern const char*   NunitoSansExtraBoldItalic_ttf;
    const int            NunitoSansExtraBoldItalic_ttfSize = 94032;

    extern const char*   NunitoSansExtraLight_ttf;
    const int            NunitoSansExtraLight_ttfSize = 89988;

    extern const char*   NunitoSansExtraLightItalic_ttf;
    const int            NunitoSansExtraLightItalic_ttfSize = 91700;

    extern const char*   NunitoSansItalic_ttf;
    const int            NunitoSansItalic_ttfSize = 92852;

    extern const char*   NunitoSansLight_ttf;
    const int            NunitoSansLight_ttfSize = 89764;

    extern const char*   NunitoSansLightItalic_ttf;
    const int            NunitoSansLightItalic_ttfSize = 91496;

    extern const char*   NunitoSansRegular_ttf;
    const int            NunitoSansRegular_ttfSize = 91460;

    extern const char*   NunitoSansSemiBold_ttf;
    const int            NunitoSansSemiBold_ttfSize = 90708;

    extern const char*   NunitoSansSemiBoldItalic_ttf;
    const int            NunitoSansSemiBoldItalic_ttfSize = 92432;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 14;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
