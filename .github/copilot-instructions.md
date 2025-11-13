# Partiels Audio Analysis Software

Partiels is a C++20 audio analysis application built with the JUCE framework and CMake. It provides both GUI and command-line interfaces for analyzing audio files using Vamp plugins.

**ALWAYS reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.**

## Repository Overview

- **Language**: C++20
- **Framework**: JUCE (cross-platform audio framework)
- **Build System**: CMake 3.18+
- **Supported Platforms**: Windows 10+, macOS 10.13+, Linux
- **Key Technologies**: Vamp plugins, SDIF format, audio analysis

### Architecture
- GUI application for interactive audio analysis and visualization
- Command-line interface for batch processing
- Plugin-based architecture using Vamp plugin standard
- Support for multiple audio formats and analysis algorithms

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

## Code Style and Standards

### C++ Coding Conventions
- **C++ Standard**: C++20 features are allowed and encouraged where appropriate
- **Code Formatting**: Use the `.clang-format` configuration file provided in the repository
- **Naming Conventions**:
  - Classes: PascalCase (e.g., `AudioAnalyzer`)
  - Functions/Methods: camelCase (e.g., `analyzeAudioFile()`)
  - Variables: camelCase (e.g., `sampleRate`)
  - Constants: UPPER_SNAKE_CASE or kPascalCase (e.g., `MAX_BUFFER_SIZE` or `kDefaultSampleRate`)
- **JUCE Conventions**: Follow JUCE framework naming and patterns for GUI components and audio processing
- **Header Guards**: Use `#pragma once` for header files

### Code Organization
- Keep classes focused and single-purpose
- Separate interface (headers) from implementation (source files)
- Use forward declarations where possible to minimize header dependencies
- Place inline functions in headers only when performance-critical

### Best Practices
- Use modern C++ features: smart pointers, range-based for loops, auto keyword when appropriate
- Avoid raw pointers; prefer `std::unique_ptr` and `std::shared_ptr`
- Use const-correctness throughout
- Prefer `nullptr` over `NULL` or `0`
- Handle errors appropriately with exceptions or error codes
- Document complex algorithms and non-obvious code with comments
- Keep functions small and focused on a single task

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

## Performance Considerations

### Audio Processing
- Audio processing must be real-time capable; avoid blocking operations in audio thread
- Use double-buffering for audio data to prevent glitches
- Minimize allocations in audio callback; pre-allocate buffers when possible
- Profile performance-critical code paths using appropriate tools

### GUI Responsiveness
- Keep GUI responsive; move heavy computations to background threads
- Use `std::thread` for parallel processing tasks
- Update UI components on the message thread only
- Implement progress feedback for long-running operations

### Memory Management
- Monitor memory usage, especially when processing large audio files
- Release resources promptly when no longer needed
- Be mindful of memory usage in plugin analysis chains
- Test with various file sizes and channel configurations

### Build Performance
- Debug builds are fast (~7 seconds); use for active development
- Release builds are slow (10+ minutes); only build when necessary
- Use Ninja generator for faster builds compared to Make
- Leverage ccache if available to speed up recompilation

## Documentation Requirements

### Code Documentation
- Document public APIs with clear comments explaining purpose and usage
- Use Doxygen-style comments for classes and functions intended for public use
- Document parameters, return values, and exceptions
- Explain non-obvious algorithms and design decisions
- Keep comments up to date when code changes

### User-Facing Documentation
- Update `Docs/Partiels-Manual.md` for user-visible feature changes
- Document new command-line options in help text
- Update example code and tutorials if API changes
- Maintain `ChangeLog.txt` for release notes

### Translation Documentation
- Wrap all user-visible strings with `juce::translate()`
- Extract translation keys after adding new strings
- Update all translation files in `BinaryData/Translations/`
- Test with different language settings when possible

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

## Security and Compliance

### Security Best Practices
- **Never commit sensitive data**: API keys, passwords, certificates, or private keys
- **Input Validation**: Always validate user input, especially file paths and audio data
- **Buffer Safety**: Use safe string operations and bounds checking for audio buffers
- **Plugin Security**: Be cautious when loading external Vamp plugins; validate plugin sources
- **File Operations**: Sanitize file paths to prevent directory traversal attacks
- **Memory Safety**: Use RAII and smart pointers to prevent memory leaks and use-after-free

### Code Review Guidelines
- All changes should be reviewed for security implications
- Check for potential buffer overflows, especially in audio processing code
- Verify proper error handling and resource cleanup
- Ensure user-facing strings are properly translated
- Validate that formatting rules are followed (use `PartielsCheckFormat`)

### License Compliance
- This project is licensed under GPL-3.0
- All contributions must be compatible with GPL-3.0
- Ensure proper attribution for third-party code
- Do not introduce dependencies with incompatible licenses

### Dependencies and Supply Chain
- Keep dependencies up to date (monitored via Dependabot)
- Verify checksums and signatures for external dependencies
- Review security advisories for JUCE, Vamp SDK, and other dependencies
- Test thoroughly after dependency updates

## Contribution Workflow

### Making Changes
1. Create a feature branch from `main`
2. Make minimal, focused changes that address a specific issue
3. Run formatters: `cmake --build build --target PartielsApplyFormat`
4. Build and test: `cmake --build build && ctest -C Debug -VV --test-dir build --output-on-failure`
5. Verify translations are updated if user-visible strings changed
6. Commit with clear, descriptive messages

### Pull Request Process
1. Ensure all tests pass locally before creating PR
2. Provide clear description of changes and motivation
3. Reference related issues with `Fixes #123` or `Closes #123`
4. Wait for CI checks to complete (all must pass)
5. Address code review feedback promptly
6. Keep PR scope focused; avoid mixing unrelated changes

### Code Review Expectations
- Code reviews focus on correctness, security, maintainability, and performance
- Reviewers will check formatting, test coverage, and documentation
- Be responsive to feedback and questions
- CI must pass before merging (format checks, tests, security scans)