# Partiels

<p align="center">
    <a href="https://github.com/Ircam-Partiels/partiels/actions/workflows/ci.yml"><img src="https://github.com/Ircam-Partiels/partiels/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
    <a href="https://github.com/Ircam-Partiels/partiels/actions/workflows/codacy.yml"><img src="https://github.com/Ircam-Partiels/partiels/actions/workflows/codacy.yml/badge.svg" alt="Codacy"></a>
</p>

<p align="center">
    <a href="https://github.com/Ircam-Partiels/partiels/releases/latest"><img src="https://img.shields.io/github/release/Ircam-Partiels/partiels.svg" alt="Release"></a>
    <a href="https://github.com/Ircam-Partiels/partiels/releases/latest"><img src="https://img.shields.io/github/downloads/Ircam-Partiels/partiels/total.svg?colorB=007ec6" alt="Downloads"></a>
    <a href="https://github.com/Ircam-Partiels/Partiels?tab=GPL-3.0-1-ov-file"><img src="https://img.shields.io/badge/license-GPL--v3-blue.svg" alt="License"></a>
</p>

**[Partiels](https://github.com/Ircam-Partiels/Partiels)** is a software suite for **analysing and exploring the content and characteristics** of sounds.

<p align="center"><img src="BinaryData/Resource/Partiels-v2.0.0-Sample-v2.gif"></p>

## Features

Partiels allows **analysis** of one or several audio files using Vamp plug-ins **loading** data files, **visualizing**, **editing**, **organizing**, and **exporting** the results as images or text files that can be used in other applications such as Max, Pure Data, Open Music, etc.

- Windows, Mac & Linux support
- Multiformat support
- Multichannel support
- Multiaudiofile support
- Analyzing audio with Vamp plug-ins
- Visualizing results as spectrogram, lines, and markers
- Textual and graphical editing of results 
- Organizing and ordering analyses in groups
- Exporting results to PNG, JPEG, CSV, LAB, JSON, CUE, REAPER, MAX, PUREDATA & SDIF formats
- Loading results from CSV, LAB, JSON, CUE, REAPER, MAX, PUREDATA & SDIF formats
- Batch processing
- Command line interface to analyze, export, and convert results
- Consolidating documents for sharing

Alongside Partiels, a wide range of analyses based on audio engines developed at IRCAM and outside are ported to Vamp plug-ins. These plug-ins allow you to perform FFT, LPC, transient, fundamental, formant, tempo, TTS and many other analyses. You can also find a large number of analysis plug-ins on the [Vamp plug-ins website](https://www.vamp-plugins.org/). 

<p align="center">
<strong><a href="https://github.com/Ircam-Partiels/partiels/releases/latest">👉 Download Partiels and the plugin suite</a></strong>
</p>

<p align="center"><img src="BinaryData/Resource/Partiels-Endorsements.png", width=520></p>

> For bug reports or feature requests, you can create an [issue](https://github.com/Ircam-Partiels/Partiels/issues) on this Github repository. For more general feedback and questions, you can create a [discussion](https://github.com/orgs/Ircam-Partiels/discussions) on this Github repository or on the [IRCAM forum](https://discussion.forum.ircam.fr/).

## Compatibility

Partiels is compatible on MacOS 10.13 and higher (Universal 2 - Intel/Silicon), Windows 10 & 11, and Linux.

<p align="center"><img src="BinaryData/Resource/Partiels-Compatibility.png"></p>

> ⚠️ Some Vamp plugins may not support the same configurations and may have other restrictions.

## Compilation

The compilation system is based on [CMake](https://cmake.org/). On macOS, you can use the XCode generator with the command `cmake -G Xcode`. On Windows, you can use the Visual Studio generator with the command `cmake -G "Visual Studio 17 2022"`. On Linux, you can use the Makefile or Ninja generators with the commands `cmake -G "Unix Makefiles"` or `cmake -G Ninja`, using Clang or GCC as the compiler.

Here is an example:

```
git clone --recursive https://github.com/Ircam-Partiels/Partiels.git
cd partiels
cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug # Example using Ninja generator; use XCode on macOS or Visual Studio on Windows.
cmake --build build
ctest -C Debug -VV --test-dir build # Note: The `-VV` flag is optional and provides verbose output for debugging purposes.
```

> ⚠️ For Linux, JUCE dependencies must be installed: Refer to the file [JUCE/docs/Linux Dependencies.md](<JUCE/docs/Linux Dependencies.md>).

> 💡 You can refer to the [GitHub Actions workflow](.github/workflows/ci.yml) for compilation instructions. 

## Releases

Partiels uses GitHub Actions for continuous integration and automated releases. Releases can be created in two ways:

### Automatic Release (Tag-based)

The release is created when a new tag is pushed (e.g., `2.1.0`). This is the preferred approach for final stable releases. The CI workflow automatically:
- Builds the project in Release mode
- Creates a draft release with the tag name
- Uploads all platform-specific installers and documentation

```bash
git tag 2.1.0
git push origin 2.1.0
```

### Manual Release (Workflow Dispatch)

You can also manually trigger a release build from the GitHub Actions UI. This is the preferred approach for testing releases, allowing you to create releases without pushing tags and providing flexibility for testing builds.

1. Go to the [Actions tab](https://github.com/Ircam-Partiels/Partiels/actions/workflows/ci.yml)
2. Click "Run workflow"
3. Configure the workflow inputs:
   - **Create a release**: Enable to create a GitHub release
   - **Version name**: Specify the version (e.g., `2.1.0-rc1`)
   - **Build type**: Choose `Release` or `Debug`

> 💡 The CI workflow also automatically detects `[debug]` in commit messages to build in Debug mode for regular commits.

## Credits

**[Partiels](https://github.com/Ircam-Partiels/partiels)**: Design, architecture and development by Pierre Guillot at IRCAM IMR department.  
Copyright 2026 IRCAM. All rights reserved.

Contributions by Thomas Barbé, Nolan Dupont, and Guilherme Zanchetta.  

### Dependencies:
- **[Vamp SDK](https://github.com/vamp-plugins/vamp-plugin-sdk)** by Chris Cannam, copyright (c) 2005-2024 Chris Cannam and Centre for Digital Music, Queen Mary, University of London.
- **[Ircam Vamp Extension](https://github.com/Ircam-Partiels/ircam-vamp-extension)** by Pierre Guillot at IRCAM IMR department.  
- **[JUCE](https://juce.com/)** by Raw Material Software Limited.
- **[tinycolormap](https://github.com/yuki-koyama/tinycolormap)** by Yuki Koyama.
- **[JSON C++](https://github.com/nlohmann/json)** by Niels Lohmann.
- **[Magic Enum](https://github.com/Neargye/magic_enum)** by Daniil Goncharov.
- **[ASIO SDK](https://www.steinberg.net/developers/)** by Steinberg Media Technologies GmbH.

## Publications and Video Presentations

Video recordings of Partiels presentations are available in this [discussion](https://github.com/orgs/Ircam-Partiels/discussions/213) on our GitHub organization page. These recordings include software demonstrations, feature walkthroughs, and academic presentations that complement the written publications.

The following academic publications present the Partiels software:

### RFIM 2025 Journal Article

**Partiels, une suite logicielle dédiée à l'analyse des fichiers audionumériques**  
Pierre Guillot  
*Revue francophone d'informatique musicale (RFIM), n° 11 — Frugalité, pérennité et création*  
December 2025  
[📄 PDF](https://revues.mshparisnord.fr/rfim/pdf/922.pdf) | [🔗 Article](https://revues.mshparisnord.fr/rfim/index.php?id=922)

```bibtex
@article{guillot_rfim_2025,
    author  = {Guillot, Pierre},
    title   = {Partiels, une suite logicielle dédiée à l'analyse des fichiers audionumériques},
    journal = {Revue Francophone d'Informatique et Musique},
    number  = {11},
    year    = {2025},
    month   = dec,
    doi     = {10.56698/rfim.922},
    url     = {https://revues.mshparisnord.fr/rfim/index.php?id=922},
    pdf     = {https://revues.mshparisnord.fr/rfim/pdf/922.pdf}
}
```

### DAFx 2025 Conference Paper

**Partiels : Exploring, Analyzing and Understanding Sounds**  
Pierre Guillot  
*Proceedings of the 28-th Int. Conf. on Digital Audio Effects (DAFx25)*  
Ancona, Italy, September 2025, pp. 298-303  
[📄 PDF](https://dafx.de/paper-archive/2025/DAFx25_paper_66.pdf)

```bibtex
@inproceedings{DAFx25_paper_66,
    author = "Guillot, Pierre",
    title = "{Partiels - Exploring, Analyzing and Understanding Sounds}",
    booktitle = "Proceedings of the 28-th Int. Conf. on Digital Audio Effects (DAFx25)",
    editor = "Gabrielli, L. and Cecchi, S.",
    location = "Ancona, Italy",
    eventdate = "2025-09-02/2025-09-05",
    year = "2025",
    month = "Sept",
    publisher = "",
    issn = "2413-6689",
    doi = "",
    pages = ""
}
```

### JIM 2025 Conference Paper

**Partiels : un logiciel pour analyser et explorer le son**  
Pierre Guillot  
*Proceedings of the 32nd Journées d'Informatique Musicale*  
Lyon, France, June 2025  
[📄 PDF](https://hal.science/hal-05102365v2/file/18.pdf) | [🔗 HAL](https://hal.science/hal-05102365)

```bibtex
@inproceedings{guillot:hal-05102365,
  TITLE = {{Partiels : un logiciel pour analyser et explorer le son}},
  AUTHOR = {Guillot, Pierre},
  URL = {https://hal.science/hal-05102365},
  BOOKTITLE = {{Proceedings of the 32nd Journ{\'e}es d'Informatique Musicale}},
  ADDRESS = {Lyon, France},
  ORGANIZATION = {{GRAME and Inria}},
  YEAR = {2025},
  MONTH = Jun,
  PDF = {https://hal.science/hal-05102365v2/file/18.pdf},
  HAL_ID = {hal-05102365},
  HAL_VERSION = {v2},
}
```
