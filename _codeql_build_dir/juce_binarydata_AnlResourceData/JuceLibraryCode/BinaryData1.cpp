/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace AnlResourceData
{

//================== About.txt ==================
static const unsigned char temp_binary_data_0[] =
"Partiels is software intended for the analysis of digital audio files. Partiels aims to offer a set of dynamic and ergonomic tools that meet the expectations of research in signal processing, musicology as well as compositional practices. Partiels al"
"lows to carry out, load, visualize, edit and export audio analyzes.\n"
"\n"
"Partiels: Design, architecture and development by Pierre Guillot at IRCAM IMR department. \n"
"Contributions to development by Thomas Barb\xc3\xa9 and Nolan Dupont.\n"
"Copyright 2025 IRCAM. All rights reserved.\n"
"https://www.ircam.fr/\n"
"\n"
"Dependencies:\n"
"Vamp SDK by Chris Cannam, copyright (c) 2005-2024 Chris Cannam and Centre for Digital Music, Queen Mary, University of London.\n"
"Ircam Vamp Extension by Pierre Guillot at IRCAM IMR department.\n"
"JUCE by Raw Material Software Limited.\n"
"tinycolormap by Yuki Koyama.\n"
"JSON C++ by Niels Lohmann.\n"
"Magic Enum by Daniil Goncharov.\n"
"ASIO SDK by Steinberg Media Technologies GmbH.\n";

const char* About_txt = (const char*) temp_binary_data_0;

}

#include "AnlResourceData.h"

namespace AnlResourceData
{

const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0x279da69e:  numBytes = 919; return About_txt;
        case 0x72bb11a6:  numBytes = 3583; return IrcamlogonoirRS_png;
        case 0x2ebf997b:  numBytes = 66081; return Ircamlogo_png;
        case 0x74136043:  numBytes = 1974; return PompidoulogonoirRS_png;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "About_txt",
    "IrcamlogonoirRS_png",
    "Ircamlogo_png",
    "PompidoulogonoirRS_png"
};

const char* originalFilenames[] =
{
    "About.txt",
    "Ircam-logo-noir-RS.png",
    "Ircam-logo.png",
    "Pompidou-logo-noir-RS.png"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}
