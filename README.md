# Partiels

[Partiels](https://forum.ircam.fr/projects/detail/partiels/) is software intended for the analysis of digital audio files. Partiels aims to offer a set of dynamic and ergonomic tools that meet the expectations of research in signal processing, musicology as well as compositional practices. Partiels allows to carry out, load, visualize, edit and export audio analyzes.

### Compilation

The compilation system is based on [CMake](https://cmake.org/).

```
git clone --recursive git@forge-2.ircam.fr:pierreguillot-group/partiels.git
cd partiels
cmake . -B build -G Xcode (specify the generator using -G such as -G"Unix Makefiles", -G"Xcode" or -G"Visual Studio 16 2019" -A x64)
cmake --build build (or use default approach of the selected generator)
```

### Release

The release system is based on the creation of tags in git. Windows and Linux binaries and the manual are generated on the GitLab CI and made available via the artifacts system. For macOS, in order to achieve notarization, we recommend compiling locally using the `./Scripts/macos-release.sh` script, defining the tag and the CI pipeline number. Once compiled locally and on the CI, the binaries and manual can be published on Nubo using the `./Scripts/publish-all.sh` script. 

```
./Scripts/macos-release.sh 1.3.1-rc1 18639
./Scripts/publish-all.sh
```

#### Credits

This project is designed and developed by Pierre Guillot at [IRCAM](https://www.ircam.fr/) IMR.  

This software is based on the [Vamp](https://www.vamp-plugins.org/) analysis plugin format, developed at the [Centre for Digital Music](http://c4dm.eecs.qmul.ac.uk/), Queen Mary, University of London.
