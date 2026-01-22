# Partiels AI Guide

Use these project-specific notes before searching the tree or running ad-hoc commands.

## What It Is
- C++20 JUCE audio-analysis suite; GUI plus CLI; hosts Vamp plugins and exports SDIF/CSV/JSON/graphics.
- Core code in [Source](../Source) (Application/Document/Group/Track/Plugin/Misc) with Vamp plugins in [VampPlugins](../VampPlugins) and packaged resources/tests/translations under [BinaryData](../BinaryData).

## Build and Test Fast Path
- Prefer Debug: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug` then `cmake --build build` (≈7s). Release takes 10+ minutes—do not cancel.
- macOS is fine with `-G Xcode`, Linux needs JUCE deps (see [JUCE/docs/Linux Dependencies.md](../JUCE/docs/Linux%20Dependencies.md)).
- Run tests: `ctest -C Debug -VV --test-dir build --output-on-failure` (≈20s). Set `VAMP_PATH=build/VampPlugins/Debug:build/Debug` for plugin-dependent tests.

## Running the App
- CLI artifacts: [build/Partiels_artefacts/Debug/Partiels](../build/Partiels_artefacts/Debug/Partiels) (use `--version`, `--help`, `--plugin-list --format=json`).
- GUI binary is the same path; headless CI cannot display it.

## Conventions That Matter
- Format with `cmake --build build --target PartielsApplyFormat` (check with `PartielsCheckFormat`).
- Wrap every user-visible string in `juce::translate`; extract keys via `cmake --build build --target PartielsExtractTranslationKeys` and update [BinaryData/Translations](../BinaryData/Translations). Keep [BinaryData/Translations/English.txt](../BinaryData/Translations/English.txt) empty (source locale).
- Plugin hosting: Vamp plugins live under [build/VampPlugins](../build/VampPlugins); tests/CLI require `VAMP_PATH` to include both `build/VampPlugins/Debug` and `build/Debug`.
- Resources and canned data for tests live in [BinaryData/Tests](../BinaryData/Tests); avoid moving them.

## Architectural Pointers
- Application entry at [Source/Main.cpp](../Source/Main.cpp); JUCE app setup and CLI wiring sit in [Source/Application](../Source/Application).
- Document/Track/Group directories model analysis documents, tracks, and grouped results; analyses rely on Vamp adapters in [Source/Plugin](../Source/Plugin) and shared helpers in [Source/Misc](../Source/Misc).
- Vamp plugin implementations for distribution are in [VampPlugins/AnlVampPlugins.cpp](../VampPlugins/AnlVampPlugins.cpp); keep interfaces in sync with app-side hosts.

## Release/Packaging
- Release build: `cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release` then `cmake --build build-release` (slow). Artifacts land under [build-release/Partiels_artefacts/Release](../build-release/Partiels_artefacts/Release) with tarball at [build-release/Partiels-Linux.tar.gz](../build-release/Partiels-Linux.tar.gz).
- CI workflow reference: [.github/workflows/ci.yml](workflows/ci.yml).

## Guardrails
- Avoid adding dependencies without GPL-3 compatibility.
- Keep audio-thread code allocation-free and non-blocking; heavy work belongs on worker threads.
- Do not modify [BinaryData/Translations/English.txt](../BinaryData/Translations/English.txt).

## Quick Checklist Before PR
- Format + translation key extraction done; tests pass with `VAMP_PATH` set; CLI sanity checks (`--version`, `--plugin-list`) succeed.

If anything above is unclear or missing, ask for the specific area and we will refine this file.