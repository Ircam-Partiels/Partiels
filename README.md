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
- Exporting results to PNG, JPEG, CSV, LAB, JSON, CUE, REAPER & SDIF formats
- Loading results from CSV, LAB, JSON, CUE & SDIF formats
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

The compilation system is based on [CMake](https://cmake.org/), for example:

```
git clone --recursive https://github.com/Ircam-Partiels/Partiels.git
cd partiels
cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest -C Debug -VV --test-dir build
```

> ⚠️ For Linux, JUCE dependencies must be installed: Refer to the file `JUCE/docs/Linux Dependencies.md`.
## Credits

**[Partiels](https://github.com/Ircam-Partiels/partiels)**: Design, architecture and development by Pierre Guillot at IRCAM IMR department.  
Contributions to development by Thomas Barbé and Nolan Dupont.  
Copyright 2025 IRCAM. All rights reserved.

### Dependencies:
- **[Vamp SDK](https://github.com/vamp-plugins/vamp-plugin-sdk)** by Chris Cannam, copyright (c) 2005-2024 Chris Cannam and Centre for Digital Music, Queen Mary, University of London.
- **[Ircam Vamp Extension](https://github.com/Ircam-Partiels/ircam-vamp-extension)** by Pierre Guillot at IRCAM IMR department.  
- **[JUCE](https://juce.com/)** by Raw Material Software Limited.
- **[tinycolormap](https://github.com/yuki-koyama/tinycolormap)** by Yuki Koyama.
- **[JSON C++](https://github.com/nlohmann/json)** by Niels Lohmann.
- **[Magic Enum](https://github.com/Neargye/magic_enum)** by Daniil Goncharov.
- **[ASIO SDK](https://www.steinberg.net/developers/)** by Steinberg Media Technologies GmbH.
