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

#### Credits

This project has been developed at [Ircam](https://www.ircam.fr/) by Pierre Guillot.  

This software is based on the [Vamp](https://www.vamp-plugins.org/) analysis plugin format, developed at the [Centre for Digital Music](http://c4dm.eecs.qmul.ac.uk/), Queen Mary, University of London.
