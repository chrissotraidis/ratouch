# RAtouch: the first 20-hour engineering record

This record summarizes the first end-to-end RAtouch implementation and validation session. It is the readable companion to the step-by-step [build status](build-status.md), not a replacement for it.

## Session boundary

- **Window:** July 21, 2026 at 06:47 CDT through July 22 at 03:04 CDT
- **Elapsed:** 20 hours, 16 minutes, 36 seconds
- **Published result:** 17 commits on `main`, ending at `b9cb4fb`
- **Change size:** 92 files changed, approximately 7,979 lines added and 237 removed across the 17 commits
- **Primary target:** native macOS and iPadOS play without changing Red Alert's simulation rules

Elapsed time includes source study, implementation, dependency and build work, automated tests, native Simulator automation, hands-on campaign and skirmish play, debugging, documentation, repository review, and publication. It is not presented as code-entry time.

## Result at a glance

| Area | Delivered result |
| --- | --- |
| macOS | Native Apple-silicon app bundle, resizable high-DPI window, fullscreen, Mac menus and settings, aligned pointer/cursor geometry, 25–200% pointer speed, data management, and quit autosave |
| iPadOS | Native arm64 Simulator app, Files-based import, lifecycle autosave, touch-first gameplay, hardware keyboard and pointer support, accessible command deck, control groups, and touch/display/audio settings |
| Gameplay fidelity | Original campaign, skirmish, sidebar, production, placement, Options, save/load, abort, restart, movies, audio, AI, hotkeys, and simulation behavior remain engine-owned |
| Data boundary | User-supplied data is validated and installed transactionally into Application Support; no commercial payload or screenshot is tracked by the repository |
| Quality evidence | 24 automated tests, 57 documented iPad Simulator checks, 15 Mac runtime checks, clean public-repository verification, and exact process cleanup after executable testing |
| Public project | Original RAtouch banner and concept art, platform and control diagrams, verified build instructions, legal/provenance guidance, contribution notes, and an honest alpha/release boundary |

## What was built

### 1. Native Apple applications

The existing portable engine remains the core. Thin Apple platform layers now provide the behavior the original desktop shell did not have:

- CMake produces `RAtouch.app` for macOS and a static-SDL2 iPad Simulator app.
- macOS owns its standard About, Settings, Quit, File, View, Window, and Help menus.
- iPadOS owns first-run setup, Files import/export, native command and settings surfaces, scene lifecycle handling, safe-area layout, and accessibility metadata.
- Both apps use sandbox-appropriate Application Support storage rather than reading commercial data from the bundle.
- Networking is disabled in the maintained Apple builds; the iPad privacy manifest declares no tracking or collected data.

### 2. Bring-your-own-data import

A shared importer was added for both Apple apps:

- accepts compatible MIX files, folders, and ISO images;
- validates archive structure and known Steam files by name, size, and SHA-256;
- stages changes before atomic activation;
- preserves configuration, saves, and hidden runtime state during replacement;
- records provenance without misclassifying unknown but structurally valid files;
- exports saves through native Apple pickers.

The local [`ref/`](../ref/README.md) directory is intentionally ignored. The public verifier rejects MIX, ISO, video, audio, shape, animation, and save payloads as well as unreviewed public images.

### 3. Touch grammar

`common/wwtouch.*` implements a platform-neutral recognizer and feeds the engine's existing mouse and scroll paths:

| Gesture | Engine meaning |
| --- | --- |
| Tap | Original left click: select, order, or activate UI |
| One-finger drag | Anchored left drag for classic box selection |
| Long press | Original right click for deselect/context behavior |
| Two-finger drag | Distance-proportional map pan after an intent dead zone |
| Pinch | Presentation-layer zoom without changing simulation scale |
| Double two-finger tap | Return presentation to fitted view |
| Cancellation/interruption | Release active input without a ghost click or order |

Touch-to-mouse synthesis is disabled at the SDL boundary. Presentation geometry is shared by drawing, cursor placement, pointer deltas, touch hit testing, and map pan so the visible cursor and internal engine point stay aligned.

### 4. Keyboard-free command access

The native command tab adds only actions that are materially difficult without a keyboard:

- immediate commands: Stop, Guard, Scatter, Next, Base, and View;
- one-shot modifiers: Attack+, Move+, Add+, and Queue+;
- assign/recall control groups 1–0;
- a Controls sheet for gesture reference and touch, display, and audio settings;
- left/right placement that remembers handedness and avoids the original sidebar.

The overlay is deliberately absent while the original Options dialog, abort/restart confirmation, score flow, or main menu owns input. Resume restores it immediately; restart restores it only after the replacement scenario is live; abort keeps it hidden until another mission loads. Armed modifiers are canceled on settings entry, Options, backgrounding, and other ownership boundaries.

### 5. Mac pointer and window behavior

The Mac path received the same geometry discipline as touch:

- window-to-game mapping accounts for high-DPI drawable size and letterboxing;
- the software cursor and engine cursor share the mapped point;
- pointer speed is clamped to 25–200%, applies immediately, and persists;
- fullscreen enters SDL window grab and relative mouse mode, then releases both on return to windowed play;
- native Command-Q routes through the gameplay autosave path before normal process exit.

### 6. Audio, saves, and lifecycle

- SDL2-backed game and movie audio replaced unsupported Apple audio assumptions.
- Backgrounding cancels active input, writes the iOS autosave, and resumes the existing mission cleanly.
- Native quit saves a live Mac mission before shutdown.
- Save compatibility carries an explicit version contract.
- Import replacement leaves saves and preferences intact.

### 7. Bugs found by playing the real game

The session was not limited to menus or synthetic tests. Campaign and bases-on skirmish play exposed issues that source inspection did not:

- fixed Mac pointer/cursor disagreement and excessive pointer sensitivity;
- replaced fixed-step two-finger scrolling with dead-zoned, proportional, consume-once movement;
- prevented half of sequential SDL multi-touch events from becoming a false pan or pinch;
- made fast one-finger releases promote to a real anchored drag instead of an accidental tap;
- rejected touches in presentation letterbox bars while still releasing a drag that exits the frame;
- prevented a malformed score animation payload from writing before its allocation;
- kept the command overlay out of Options, confirmations, scenario exits, and the main menu;
- canceled stale one-shot commands across modal and lifecycle transitions;
- verified original production lanes, scrolling, construction, placement, and unit training remain directly touchable.

## How the 20 hours progressed

| Phase | Work and decision |
| --- | --- |
| Foundation | Read the PRD and feasibility report, audited the upstream engine, established the data/licensing boundary, and created native Apple build targets. |
| First playable | Implemented import, paths, audio, presentation geometry, lifecycle, saves, Mac shell, iPad shell, and the initial touch recognizer; reached real campaign and skirmish play. |
| Input refinement | Repeated selection, orders, drag, long press, pinch, reset, pointer, keyboard, and sidebar sessions; corrected coordinate mapping and sensitivity; introduced proportional two-finger pan. |
| Keyboard-free play | Added and exercised the command deck, one-shot modifiers, Groups, Controls, handedness, Dynamic Type, and sidebar avoidance. |
| Gameplay depth | Deployed an MCV, built and placed structures, scrolled populated production lanes, trained a unit, used Options, saved/loaded, and crossed abort/restart flows. |
| Stability | Reproduced and repaired score-animation memory corruption, then ran repeated lifecycle and scenario-boundary checks. |
| Mac proof | Validated live quit autosave, fullscreen pointer capture, relative input, window restoration, pointer-speed application, and persistence against isolated data fixtures. |
| Public release surface | Added original artwork and diagrams, rewrote the README, documented provenance and compatibility, enforced public asset/link checks, and published 17 focused commits. |

## Published commit ledger

| Commit | Outcome |
| --- | --- |
| `f3b1c72` | Built the native macOS/iPadOS foundation, shared importer, touch layer, tests, docs, artwork, and CI workflow |
| `e06c9d1` | Polished the public README and recorded input playtesting |
| `560364c` | Canceled stale touch modifiers before settings |
| `da9f2f9` | Clarified and proved Guard command semantics |
| `8d85341` | Prevented the skirmish score-animation crash |
| `36e8948` | Documented touch production playtesting |
| `40b7c55` | Documented direct native sidebar control proof |
| `03b3347` | Documented native structure-queue scrolling |
| `3ae4041` | Documented native unit-queue scrolling |
| `5e25713` | Documented the touch-only production chain |
| `db61108` | Clarified ownership of mission controls |
| `fb1a869` | Hid the command deck while game Options owns input |
| `efa639f` | Documented modal modifier cancellation |
| `403df88` | Canceled armed commands on app backgrounding |
| `47fb0b8` | Kept the command deck hidden through scenario exit |
| `c0417a2` | Documented the full abort-to-load overlay boundary |
| `b9cb4fb` | Documented live Mac fullscreen and pointer proof |

## Verification ledger

### Automated checks

The maintained macOS test target contains 24 passing tests. RAtouch-specific coverage includes touch recognition, one-shot modifiers, presentation geometry, lifecycle state, gameplay policy, settings, save versioning, ISO parsing, MIX validation, manifest selection, SDL2 audio, and the score-animation sizing regression. Existing engine rendering and compression tests remain in the same run.

The public-repository verifier checks:

- forbidden commercial/runtime file types and `savegame.*` names;
- the `ref/` tracking boundary;
- the complete reviewed image allowlist;
- banner/concept minimum dimensions;
- SVG title and description metadata;
- opaque 1024×1024 iPad app icon requirements;
- every local target in tracked Markdown.

### Runtime checks

The detailed log records 57 iPad Simulator steps and 15 Mac steps. Important native evidence includes:

- tap selection and movement, one-finger drag selection, long-press right click, two-touch pinch, and double-two-finger reset;
- hardware keyboard commands, indirect pointer click/right click, pointer edge scroll, and disabled touch edge scroll;
- all command-deck families, group assign/recall, handedness, Dynamic Type, and sidebar clearance;
- Options, Resume, Restart, Abort, main-menu absence, and restoration after mission load;
- MCV deployment, structure placement, populated structure/unit queue scrolling, and a complete Attack Dog training cycle;
- 100 live background/foreground cycles with autosave updates and no new crash report;
- Mac live mission quit autosave, window/fullscreen transitions, pointer capture, relative input, and sensitivity persistence.

Commercial gameplay frames, disposable XCTest harnesses, isolated data fixtures, and result bundles stayed under `/private/tmp` and were never added to Git.

### Completion audit

At the end of the session:

- the macOS app built successfully;
- all 24 tests passed;
- the arm64 iPad Simulator app built, installed, and launched successfully;
- the public verifier passed across 924 files and 8 Markdown documents;
- `git diff --check` passed and the worktree was clean;
- local `main`, `origin/main`, and GitHub `main` matched at `b9cb4fb`;
- the tested Simulator executable was terminated and exact-name process checks found no RAtouch or `vanillara` process.

GitHub Actions jobs were configured and triggered, but the account stopped every job before checkout because of a billing/spending-limit restriction. That is recorded as a hosted-runner/account gate, not a passing or failing source result. The equivalent Apple build, test, and repository checks above passed locally.

### Post-completion sanity pass

After the recorded 20-hour window, a focused native iPad UI audit exercised the complete command palette, all immediate and one-shot commands, both control-group modes and all ten slots, the Controls, Game Data, and About/license sheets, both handedness positions, and palette open/close behavior. The iPad Pro 11-inch (M4) / iOS 18.5 Simulator test passed in 188.095 seconds with zero failures. Five current-run screenshots were inspected, the app was terminated, and the result became Simulator check 58 in the maintained [build status](build-status.md). This follow-up is intentionally separate from the 57 checks and commit totals inside the original session boundary above.

## Reproduce the maintained gates

```sh
cmake -S . -B build/ratouch-macos \
  -DBUILD_VANILLATD=OFF \
  -DBUILD_VANILLARA=ON \
  -DBUILD_TESTS=ON \
  -DSDL_AUDIO=ON \
  -DOPENAL=OFF \
  -DNETWORKING=OFF

cmake --build build/ratouch-macos --parallel
ctest --test-dir build/ratouch-macos --output-on-failure
./scripts/build-ios-simulator.sh
python3 scripts/verify-public-repo.py
git diff --check
```

The iPad build requires full Xcode and an installed Simulator runtime. Neither build downloads nor bundles game data.

## What remains outside this proof

This session completed the native Mac/iPad Simulator product objective. It did not claim release certification. Open gates remain:

- physical-iPad two-finger pan direction/velocity feel, long-press haptics, Pencil, and trackpad ergonomics;
- VoiceOver traversal against the running game and physical-device Dynamic Type review;
- device thermals, battery, memory pressure, audio interruptions, and long hardware soak;
- signed device builds, TestFlight, App Store review, and release compliance;
- universal2/Intel Mac build, signing, notarization, and DMG packaging;
- real-file validation for every declared non-Steam layout;
- iPhone interaction prototype and its explicit go/no-go quality gate.

These are tracked as release work rather than quietly folded into Simulator claims.

## Source of truth

- [Build status](build-status.md): chronological runtime and playtest evidence
- [Input design](input-design.md): gesture grammar, overlay contract, and validation matrix
- [PRD and build plan](prd-build-plan.md): product scope, architecture, phases, and release gates
- [Asset provenance](asset-provenance.md): compatible data mapping and legal boundary
- [Save compatibility](save-compatibility.md): save and version contract
- [Public README](../README.md): installation, project story, and contributor entry point
