<p align="center">
  <img src="docs/images/ratouch-banner.png" alt="RAtouch — an original touch, pointer, and radar-inspired banner" width="100%">
</p>

<p align="center">
  <strong>The original 1996 real-time strategy simulation, rebuilt for the way Macs and iPads are actually used.</strong>
</p>

<p align="center">
  <a href="https://github.com/chrissotraidis/ratouch/actions/workflows/ratouch.yml"><img alt="Apple build and tests" src="https://github.com/chrissotraidis/ratouch/actions/workflows/ratouch.yml/badge.svg"></a>
  <img alt="Status: active alpha" src="https://img.shields.io/badge/status-active_alpha-ff3b30">
  <img alt="Platforms: macOS and iPadOS" src="https://img.shields.io/badge/platforms-macOS_%7C_iPadOS-f4ead7">
  <a href="License.txt"><img alt="License: GPL-3.0 with additional terms" src="https://img.shields.io/badge/license-GPL--3.0_with_terms-292c31"></a>
</p>

RAtouch preserves the campaigns, skirmish AI, movies, music, saves, build queues, and rules of the original game. It changes the platform layer around them: native Apple builds, sandbox-safe data import, lifecycle handling, modern audio, precise pointer input on Mac, and a deliberate touch grammar on iPad.

This repository contains engine and platform code only. **It does not contain commercial game data.** You provide legally acquired compatible data on your own device.

<p align="center">
  <img src="docs/images/ratouch-gameplay-concept.png" alt="Original RAtouch concept art showing an abstract touch-driven tactical field on an iPad" width="100%">
  <br>
  <sub>Original RAtouch concept art — not a game screenshot and not built from commercial assets.</sub>
</p>

## Native where it matters

<p align="center">
  <img src="docs/images/platforms.svg" alt="macOS uses pointer precision and original hotkeys; iPadOS uses direct touch, control groups, and an accessible command deck" width="100%">
</p>

| | macOS | iPadOS |
| --- | --- | --- |
| Primary input | Mouse or trackpad + original hotkeys | Direct touch; hardware keyboard and pointer supported |
| Selection | Click or pointer drag | Tap or one-finger drag |
| Map movement | Edge scroll, keys, pointer | Distance-proportional two-finger drag after an intent dead zone; direct touch never triggers edge scroll |
| Context / deselect | Right-click | Long press |
| Display zoom | Window and presentation controls | Pinch through fit, 1.5×, and 2×; double two-finger tap returns to fit |
| Missing keyboard actions | Full keyboard available | Accessible command palette, one-shot Attack+, Move+, Add+, and Queue+ modifiers, ten assignable control-group slots, and an in-game Controls sheet |
| Game data | Native first-run picker into Application Support | Native Files picker into Application Support |
| Window model | Resizable native window, macOS menus, and fullscreen toggle | Fullscreen landscape in v1 |

The simulation is shared. The control surface is not forced to pretend that a finger is a mouse.

## Touch without rewriting the game

<p align="center">
  <img src="docs/images/touch-controls.svg" alt="RAtouch gesture map: tap and one-finger drag; two-finger pan and pinch; keyboard-free command deck and control groups" width="100%">
</p>

The gesture layer feeds the engine's existing virtual mouse and scroll paths. One finger keeps the original left-button semantics without activating mouse-at-edge camera movement. Two fingers own navigation, with a small intent dead zone followed by distance-proportional map travel, so hand jitter stays still and a pan cannot accidentally become a selection box; lifting one finger stops the camera while the remaining contact drains safely instead of turning into a stray selection. A real mouse or trackpad immediately restores the original pointer edge scroll. The native command tab passes every touch outside its controls straight through to the game, can move to either safe-area edge for handedness, remembers that choice, and shifts to the map boundary when Red Alert's right sidebar opens. Its two-column command deck keeps every keyboard substitute wide and readable with Dynamic Type. A compact Groups sheet recalls or replaces slots 1–0 through the original number and Control-number paths, making formation switching practical without a keyboard. The native Controls sheet exposes the complete gesture reference plus live, persisted presets for hold duration, drag threshold, pan direction, haptics, sharp or smooth scaling, preserved or filled aspect, and independent music and sound volume. A nested Game Data route identifies the imported source, stages validated replacements for the next cold launch, and exports saves through Files. Pinned sheet headers keep every Done action available while the scroll-safe bodies scale through accessibility text sizes, without adding another permanent gameplay overlay.

The complete hotkey coverage matrix, overlay rules, and repeatable playtest loop live in [the input and gameplay refinement contract](docs/input-design.md).

## What works today

RAtouch is an active alpha, not a packaged public release. The current vertical slice has been exercised with legally supplied Steam 2229840 data:

- shared native Apple first-run import with a scroll-safe Dynamic Type setup screen, Files/folder/ISO selection, structural validation, exact known-file hashes, atomic install, nested disc mapping, and a local provenance receipt; native data management shows that receipt, exports saves to Files on iPad, and stages replacement archives separately until the next cold launch while preserving saves and settings;
- intro, main menu, campaign selection, briefing, a playable Allied mission, manual save/load, skirmish, both expansion menus, and background autosave through one hundred cumulative live Simulator mission cycles;
- direct touch selection and movement without accidental edge scrolling, distance-proportional two-finger pan with an intent dead zone and consume-once delivery, native Simulator finger-drag group selection and long-press deselection, live two-touch pinch and double-two-finger reset, iPad hardware-keyboard shortcuts and aligned pointer click/right-click with preserved pointer edge scroll, safe touch and pointer movie skipping with first-menu-frame isolation, deterministic gesture-edge coverage, fast-release drag recovery, ten immediate or one-shot keyboard-free commands, ten assignable control-group slots, and persisted native controls for touch tuning, display filtering, aspect handling, music/SFX volume, and a sidebar-aware command-tab side over a running mission;
- Apple SDL2 audio for simultaneous game sound and movie audio;
- branded Apple Silicon `RAtouch.app` first playable with native menus, live display/audio controls, imported-source and safe replacement controls, a persisted 25–200% pointer-speed control with percentage feedback, resizable/fullscreen presentation, the same mapped data, and original full-bleed app icons on macOS and iPadOS;
- Retina-aware pointer mapping that keeps the physical pointer, software game cursor, clicks, and raw-motion sensitivity in the same letterboxed presentation space;
- native Mac Quit and window-close events routed through one engine-thread shutdown path, with an explicit active-scenario gate preventing menu or partial-load autosaves; a live Command-Q mission pass changed the isolated autosave and completed cleanup with exit code 0;
- full, scrollable GPL and EA terms bundled in both Apple apps, with a source link in macOS About and iPadOS Controls → About & license;
- 23 automated tests plus a successful arm64 iPad Simulator build.

See the exact session evidence and remaining hardware/release gates in [build status](docs/build-status.md).

## Build

### macOS

Install CMake and SDL2, then build the focused Apple target:

```sh
brew install cmake sdl2

cmake -S . -B build/ratouch-macos \
  -DBUILD_VANILLATD=OFF \
  -DBUILD_VANILLARA=ON \
  -DBUILD_TESTS=ON \
  -DSDL_AUDIO=ON \
  -DOPENAL=OFF \
  -DNETWORKING=OFF

cmake --build build/ratouch-macos --parallel
ctest --test-dir build/ratouch-macos --output-on-failure
python3 scripts/verify-public-repo.py
```

The app is produced at `build/ratouch-macos/RAtouch.app` and stores imported data, settings, and saves under `~/Library/Application Support/Ratouch/vanillara`. On a clean launch, its native picker accepts legally acquired MIX files, an ISO, or a folder and installs validated data through the same transactional importer used on iPadOS.

### iPad Simulator

Xcode and an installed iOS Simulator runtime are required:

```sh
./scripts/build-ios-simulator.sh
```

The script prints the generated `.app` path. Install it with `xcrun simctl install`, launch RAtouch, and choose legally acquired MIX files, a folder containing them, or a supported ISO in the native setup screen. Nothing is copied into the app bundle at build time.

## Bring your own data

RAtouch intentionally ships empty. Imported files stay in the app's Application Support container and are excluded from source, CI, and release artifacts. Known Steam 2229840 files are accepted only when filename, byte size, and SHA-256 agree; unknown structurally valid archives remain identifiable as user-supplied data rather than being silently misclassified.

- [Asset provenance and deterministic mapping](docs/asset-provenance.md)
- [Save compatibility contract](docs/save-compatibility.md)
- [Local `ref/` policy](ref/README.md)
- [Product requirements and phased build plan](docs/prd-build-plan.md)
- [Technical and legal feasibility report](docs/feasibility-report.md)

Never commit MIX files, ISO images, save files, imported artwork, or screenshots containing commercial game assets.

## Direction

The next refinements are proof-gated:

1. repeat campaign and skirmish playtests around selection, orders, scrolling, zoom, sidebar recovery, save/load, and lifecycle;
2. tune gesture thresholds and pan direction on physical iPads, then verify Pencil, trackpad feel, haptics, accessibility, thermals, and audio interruption;
3. keep refining the proven one-shot modifiers and control-group workflow from real campaign and skirmish sessions;
4. refine native Mac settings, packaging, signing, notarization, and release compliance now that live in-mission quit autosave is proven;
5. keep the public README visual, current, reproducible, and free of commercial assets.

The full sequence and acceptance gates are in the [PRD and build plan](docs/prd-build-plan.md).

## Foundation and license

RAtouch is built on [Vanilla Conquer](https://github.com/TheAssemblyArmada/Vanilla-Conquer) and keeps its portable engine architecture intact wherever possible. The source is licensed under GPL-3.0 with the additional terms carried in [License.txt](License.txt). Those terms grant no trademark or game-asset rights. The complete text and source link are also available inside each build through **RAtouch → About RAtouch** on macOS and **Commands → Controls → About & license** on iPadOS.

RAtouch is an independent, non-commercial project. It is not affiliated with, endorsed by, or supported by Electronic Arts. Product names may be referenced only to explain compatibility with data the user already owns.
