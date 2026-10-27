# Partiels Agent Guide

Project-specific notes for AI coding agents (Claude, Mistral, Copilot, Codex, ...). Read them before searching the tree or running ad-hoc commands.

## What It Is
- C++20 JUCE audio-analysis application (GUI and CLI) that hosts Vamp plugins and exports results (SDIF, CSV, JSON, CUE, images...).
- Application code in [Source](Source), split by module: Application, Document, Group, Track, Plugin, Misc.
- Vamp plugins distributed with Partiels in [VampPlugins](VampPlugins); resources, presets, translations and test data in [BinaryData](BinaryData).
- Git submodules: [JUCE](JUCE), [Dependencies/vamp-plugin-sdk](Dependencies/vamp-plugin-sdk), [Dependencies/ircam-vamp-extension](Dependencies/ircam-vamp-extension). These are Partiels forks: do not modify them unless explicitly asked.

## Build and Test
All commands below use `build-agent` as the build directory. Do not reuse or delete the developer's existing `build*` directories.
- Configure (Debug is faster to build and test):
  - macOS/Linux: `cmake -B build-agent -G Ninja -DCMAKE_BUILD_TYPE=Debug`
  - Windows: `cmake -B build-agent -G "Visual Studio 17 2022" -A x64`
  - Linux also needs the JUCE system dependencies (see [JUCE/docs/Linux Dependencies.md](JUCE/docs/Linux%20Dependencies.md)).
- Build: `cmake --build build-agent` (add `--config Debug` with multi-config generators such as Xcode or Visual Studio).
- Test: `ctest -C Debug -VV --test-dir build-agent --output-on-failure`.
  - Tests set `VAMP_PATH` themselves; no environment setup is required.
  - `UnitTests` runs `Partiels --unit-tests`; the other tests run CLI exports and compare their output with the expected files in [BinaryData/Tests](BinaryData/Tests).
  - New tests are declared with `add_test` in [CMakeLists.txt](CMakeLists.txt).

## Running the CLI
- Binary location under `build-agent/Partiels_artefacts/Debug/`:
  - macOS: `Partiels.app/Contents/MacOS/Partiels`
  - Linux: `Partiels`
  - Windows: `Partiels.exe`
- Useful sanity checks: `--version`, `--help`, `--plugin-list --format=json`.
- To find the Vamp plugins built with the project, set `VAMP_PATH` to the directories containing `partiels-vamp-plugins` and `vamp-example-plugins` (`build-agent/VampPlugins/Debug:build-agent/Debug`, also with Ninja; use `;` as separator on Windows).
- The GUI cannot be displayed in headless environments.

## Code Conventions
- Formatting is enforced by [.clang-format](.clang-format) (Allman braces, indented namespaces, no space before parentheses): run `cmake --build build-agent --target PartielsApplyFormat` and check with `PartielsCheckFormat`.
- Files are named `Anl<Module><Class>.h/.cpp` (e.g. `Source/Plugin/AnlPluginListScanner.cpp` declares `PluginList::Scanner`).
- Wrap file contents in `ANALYSE_FILE_BEGIN` / `ANALYSE_FILE_END` (see [Source/Misc/AnlBase.h](Source/Misc/AnlBase.h)) and put classes in their module namespace (`Track`, `Group`, `PluginList`, ...).
- Place `const` after the type (`Type const& value`) and prefix members with `m` (`mProcess`).
- Document public declarations with Doxygen comments (`//! @brief`, `//! @param`, `//! @return`).
- Match the style of the surrounding code rather than introducing new idioms.
- Code must compile without warnings. Strict warning flags are enabled for `Partiels` and `partiels-vamp-plugins` (`juce::juce_recommended_warning_flags`, plus [XcodeSetupWarnings.cmake](BinaryData/Resource/XcodeSetupWarnings.cmake) with Xcode); do not silence warnings with pragmas or flags.

## Translations
- Wrap every user-visible string in `juce::translate`.
- Translation keys are extracted automatically when building `Partiels` (target `PartielsExtractTranslationKeys`).
- Update the translation files with the targets `PartielsUpdateTranslationFrench` and `PartielsUpdateTranslationPortuguese`, then report the new keys in [BinaryData/Translations](BinaryData/Translations).
- Never modify [BinaryData/Translations/English.txt](BinaryData/Translations/English.txt): it must stay empty (English is the source locale).

## Architecture Pointers
- Entry point: [Source/Main.cpp](Source/Main.cpp); application setup, CLI ([AnlApplicationCommandLine.cpp](Source/Application/AnlApplicationCommandLine.cpp)) and main window in [Source/Application](Source/Application).
- Document, Group and Track modules model the analysis document, its groups and its tracks (models, directors, editors, exporters).
- Plugin hosting, scanning and the plugin list (including web download) live in [Source/Plugin](Source/Plugin); shared helpers in [Source/Misc](Source/Misc).
- Partiels' own Vamp plugins are implemented in [VampPlugins/AnlVampPlugins.cpp](VampPlugins/AnlVampPlugins.cpp); keep them in sync with the app-side hosting code.
- CI reference (build, test and packaging on Linux, Windows and macOS): [.github/workflows/ci.yml](.github/workflows/ci.yml).

## Guardrails
- Only add dependencies whose license is compatible with GPL-3.
- Keep audio-thread code allocation-free and non-blocking; heavy work belongs on worker threads.
- Do not move the test resources in [BinaryData/Tests](BinaryData/Tests): tests reference them by path.
- Do not commit, push or rewrite history unless explicitly asked. If asked to write a commit message, follow the existing convention `Module::Class: description` (e.g. `PluginList::Scanner: fix forceRescan in getPlugins()`).

## Checklist Before Handing Over
- Build completes without warnings in the modified files.
- Code formatted (`PartielsCheckFormat` passes).
- New user-visible strings translated in French and Portuguese.
- `ctest` passes.
