/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace AnlResourceData
{
    extern const char*   About_txt;
    const int            About_txtSize = 919;

    extern const char*   IrcamlogonoirRS_png;
    const int            IrcamlogonoirRS_pngSize = 3583;

    extern const char*   Ircamlogo_png;
    const int            Ircamlogo_pngSize = 66081;

    extern const char*   PompidoulogonoirRS_png;
    const int            PompidoulogonoirRS_pngSize = 1974;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 4;

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
