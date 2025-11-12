# Partiels Audio Analysis Software

Partiels is a C++20 audio analysis application built with the JUCE framework and CMake. It provides both GUI and command-line interfaces for analyzing audio files using Vamp plugins.

**ALWAYS reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.**

## Working Effectively

### Bootstrap and Build Process
**CRITICAL: Debug builds are FAST (~7 seconds), Release builds are SLOW (10+ minutes). NEVER CANCEL long-running Release builds.**

#### Quick Development Workflow (Recommended)
- `sudo apt update`
- `sudo apt install libasound2-dev libjack-jackd2-dev ladspa-sdk libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev imagemagick ninja-build`
- `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER="/usr/bin/clang" -DCMAKE_CXX_COMPILER="/usr/bin/clang++"`
- `cmake --build build` -- FAST: Debug builds complete in ~7 seconds. NEVER CANCEL.
- `ctest -C Debug -VV --test-dir build --output-on-failure` -- takes ~20 seconds. NEVER CANCEL. Set timeout to 60+ seconds.

#### Production Build Workflow  
- `cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="/usr/bin/clang" -DCMAKE_CXX_COMPILER="/usr/bin/clang++"`
- `cmake --build build-release` -- SLOW: Release builds take 10+ minutes. NEVER CANCEL. Set timeout to 30+ minutes.
- `ctest -C Release -VV --test-dir build-release --output-on-failure` -- takes ~20 seconds. NEVER CANCEL.

### Running the Application

#### Command Line Interface
- **ALWAYS run the bootstrapping and build steps first.**
- Check version: `./build/Partiels_artefacts/Debug/Partiels --version`
- Get help: `./build/Partiels_artefacts/Debug/Partiels --help`
- List available plugins: `VAMP_PATH=build/VampPlugins/Debug:build/Debug ./build/Partiels_artefacts/Debug/Partiels --plugin-list --format=json`

#### GUI Application
- **Note: Cannot interact with GUI in this environment, but you can build and test CLI functionality.**
- Start GUI: `./build/Partiels_artefacts/Debug/Partiels` (will fail in headless environment)

## Validation

### Manual Testing Requirements
- **ALWAYS run the complete test suite after making changes**: `ctest -C Debug -VV --test-dir build --output-on-failure`
- **ALWAYS test CLI functionality with real commands**:
  - `./build/Partiels_artefacts/Debug/Partiels --version` (should show "Partiels v2.1.0")
  - `./build/Partiels_artefacts/Debug/Partiels --help` (should show detailed usage)
  - `VAMP_PATH=build/VampPlugins/Debug:build/Debug ./build/Partiels_artefacts/Debug/Partiels --plugin-list` (should list available plugins)

### Code Quality Validation
- **Format checking**: `cmake --build build --target PartielsCheckFormat` (must pass before committing; code must be properly formatted)
- **Run format fixing before committing**: `cmake --build build --target PartielsApplyFormat` (required pre-commit step)
- The CI pipeline (`.github/workflows/ci.yml`) will fail if code formatting issues are introduced.

### Testing Requirements
- All tests must pass before committing changes
- Test suite includes: unit tests, CLI functionality, export formats, image comparison, plugin validation
- Tests require both Partiels plugins and Vamp example plugins to be available
- **Set VAMP_PATH environment variable** for plugin-dependent tests: `VAMP_PATH=build/VampPlugins/Debug:build/Debug`

### Verifying Translations
- Check if `juce::translate` is used for all user-visible strings (must pass before committing)
- Extract translation keys: `cmake --build build --target PartielsExtractTranslationKeys`
- Update translation files in `BinaryData/Translations/` accordingly (must pass before committing)
- The English translation file `English.txt` should remain empty as it is the source language.
- All the other translation files should contain the keys generated and their respective translations.
- When reviewing code, ensure new strings are properly wrapped for translation and that translation files are updated accordingly.
- When committing changes, ensure that translation keys are extracted and translation files are up to date.

## Critical Build Information

### Timing Expectations
- **Configuration**: 1-40 seconds (longer on first run when fetching Vamp SDK)
- **Debug Build**: ~7 seconds - NEVER CANCEL  
- **Release Build**: 10+ minutes - NEVER CANCEL. Set timeout to 30+ minutes.
- **Complete Test Suite**: ~20 seconds - NEVER CANCEL. Set timeout to 60+ seconds.

### Required Dependencies (Linux)
The following packages are required and should be installed with:
```bash
sudo apt install libasound2-dev libjack-jackd2-dev ladspa-sdk libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev imagemagick ninja-build
```

### Build System Details
- **Build System**: CMake 3.18+ with Ninja generator (fastest) or Unix Makefiles
- **Compiler**: Clang 18+ recommended (as used in CI), GCC also supported
- **C++ Standard**: C++20
- **Submodules**: JUCE framework, Vamp SDK (auto-fetched), IRCAM extensions

## Key Project Structure

### Source Code Organization
```
Source/              # Main application source code
├── Application/     # Application lifecycle and CLI
├── Document/        # Document model and file handling  
├── Group/           # Analysis group management
├── Main.cpp         # Application entry point
├── Misc/            # Utility classes
├── Plugin/          # Vamp plugin integration
└── Track/           # Audio track and analysis handling

VampPlugins/         # Audio analysis plugins
├── AnlVampPlugins.cpp # Plugin implementations
└── AnlVampPlugins.h

Dependencies/        # Third-party libraries
├── ircam-vamp-extension/ # IRCAM audio analysis extensions
├── sdif/            # SDIF format support
└── Misc/            # Utility libraries

BinaryData/          # Resources and test data
├── Tests/           # Test audio files and expected results
├── Translations/    # Localization files
└── Resource/        # Icons, templates, build scripts
```

### Important Files
- `CMakeLists.txt` - Main build configuration
- `.github/workflows/ci.yml` - CI pipeline with build instructions
- `BinaryData/Tests/` - Test data and expected results for validation
- `JUCE/docs/Linux Dependencies.md` - Complete Linux dependency list

### Build Outputs
- **Debug Executable**: `build/Partiels_artefacts/Debug/Partiels`
- **Release Executable**: `build-release/Partiels_artefacts/Release/Partiels`
- **Vamp Plugins**: `build/VampPlugins/Debug/partiels-vamp-plugins.so`
- **Package**: `build/Partiels-Linux.tar.gz` (created automatically)

## Common Tasks and Commands

### Development Workflow
```bash
# Set up fresh environment
sudo apt update && sudo apt install libasound2-dev libjack-jackd2-dev ladspa-sdk libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev imagemagick ninja-build

# Quick debug development cycle  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER="/usr/bin/clang" -DCMAKE_CXX_COMPILER="/usr/bin/clang++"
cmake --build build  # ~7 seconds
ctest -C Debug -VV --test-dir build --output-on-failure  # ~20 seconds

# Test basic functionality
./build/Partiels_artefacts/Debug/Partiels --version
./build/Partiels_artefacts/Debug/Partiels --help
VAMP_PATH=build/VampPlugins/Debug:build/Debug ./build/Partiels_artefacts/Debug/Partiels --plugin-list
```

### Release and Packaging
```bash
# Production build (slow)
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="/usr/bin/clang" -DCMAKE_CXX_COMPILER="/usr/bin/clang++"
cmake --build build-release  # 10+ minutes - NEVER CANCEL

# Package automatically created at: build-release/Partiels-Linux.tar.gz
```

### Code Quality Checks
```bash
# Check formatting (reports issues, does not fix)
cmake --build build --target PartielsCheckFormat

# Extract translation keys (required dependency)
cmake --build build --target PartielsExtractTranslationKeys
```

## Troubleshooting

### Common Issues
1. **"No such file or directory" for JUCE dependencies**: Install the required apt packages listed above
2. **"Vamp plugins not found" during tests**: Set `VAMP_PATH=build/VampPlugins/Debug:build/Debug`
3. **Release build appears to hang**: This is normal - Release builds take 10+ minutes. Wait for completion.
4. **Format check failures**: These may exist in the current codebase and are normal unless you introduce new formatting issues.

### Build Failures
- **Clean rebuild**: `rm -rf build && cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build`
- **Update submodules**: `git submodule update --init --recursive`
- **Check disk space**: Large Release builds require significant disk space

### Testing Issues
- **Image comparison test failures**: Requires ImageMagick - `sudo apt install imagemagick`
- **Plugin tests fail**: Ensure `VAMP_PATH` includes both `build/VampPlugins/Debug` and `build/Debug`

Always use Debug builds for development and testing unless specifically building for release distribution.