# PRD and Build Plan: RAtouch

**Product:** a native, ship-quality port of Command & Conquer: Red Alert (1996) for macOS, iPadOS, and iOS, built on Vanilla Conquer, with user-supplied game assets.
**Audience for this document:** autonomous coding agents executing the build, and the human lead directing them.
**Companion document:** `feasibility-report.md`. Read it first; every architectural claim below is evidenced there.
**Naming constraint (hard):** the shipped product may not be named or marketed with "Command & Conquer," "Red Alert," or any EA trademark (EA LICENSE.md, ADDITIONAL TERMS per GNU GPL Section 7). RAtouch is the working public identity and still requires human trademark review before distribution. Store copy may use nominative references such as "compatible with game data from the 1996 Westwood RTS."

---

## 1. Vision, goals, non-goals

**Vision.** The 1996 game, exactly as it played, feeling native on every Apple device: instant launch, trackpad-perfect on Mac, touch-first on iPad, respectful of the platform (sandbox, lifecycle, Files app, accessibility), with the user's own legally acquired game data.

**Goals.**

- G1: Faithful gameplay. The original simulation, campaigns (Allied and Soviet), skirmish, movies, music, and saves, unmodified in behavior.
- G2: Native feel per device. Mac: windowed and fullscreen, menu bar, Retina-crisp scaling, keyboard shortcuts. iPad: the primary touch platform, direct-manipulation gestures, keyboard and trackpad support, Files-based import. iPhone: supported with an adapted interaction mode.
- G3: Clean legality. No EA assets in any distributed artifact, full corresponding source published per GPLv3, neutral naming, documented asset provenance paths for users.
- G4: Agent-executable. Every phase below is scoped so autonomous agents can do the mechanical majority, with human gates marked.

**Non-goals for 1.0.**

- No gameplay modernization (no attack-move, no unit queues beyond the original, no rebalancing). Faithful means faithful.
- No online multiplayer service, no CnCNet integration (v2 candidate).
- No Counterstrike/Aftermath campaign UI beyond what the engine already exposes when expansion files are present.
- No Android, no Windows/Linux distribution work beyond keeping upstream CI green.
- No engine-level widescreen or UI re-layout (v2 investigation; see 9.4).

**Success metrics.**

- M1: Cold launch to main menu under 3 seconds on M1 Mac and A14 iPad.
- M2: Steady 60 fps render (120 where ProMotion) at all supported scale factors; simulation at original tick rate.
- M3: Asset import success rate above 90% in beta telemetry-free terms (measured by beta feedback form, since we ship no analytics).
- M4: Full Allied and Soviet campaign completion by testers on each platform without progression-blocking bugs.
- M5: Zero crashes across a 2-hour soak with repeated backgrounding on iPad and iPhone.
- M6: App Review outcome recorded (approval, or documented rejection reasons feeding the fallback channel decision).

**Target audience.** Primary: people who played RA in 1996-2000, now on Macs and iPads, willing to buy the $20 Steam release or dig out the freeware ISOs. Secondary: RTS-curious iPad players; retro gaming community; the C&C community (CnCNet, Reddit r/commandandconquer).

---

## 2. Functional requirements, v1

FR1. Launch, without game data present, into a Setup experience that explains what data is needed and imports it (see UX, section 10).
FR2. Import accepts: bare MIX files, a folder (Steam install layout or Remastered `CNCDATA/RED_ALERT` layout), or a CD ISO image (freeware Allied/Soviet ISOs), extracting the needed MIX files in-app. Validation by file name, size, and known-good hashes, with clear per-file status.
FR3. Full singleplayer: both campaigns, all difficulties, mission briefings, VQA movies with audio, score music, saves and loads.
FR4. Skirmish vs original AI with the original options.
FR5. Saves: autosave on app background (iOS) and on quit (macOS); manual save/load; save directory excluded from asset area; saves survive app updates that do not change gameplay structs (release gate; see Testing T6).
FR6. Settings: display scale filter (sharp/smooth), aspect handling, music/SFX volume, touch tuning (long-press duration, drag threshold presets), control scheme reference, licenses/credits screen with the full EA notice and GPL text, "Get the source" link.
FR7. macOS: resizable window, fullscreen toggle, standard menu bar (About, Preferences, Quit), Retina scaling, pointer confinement in fullscreen, standard keyboard shortcuts matching the original plus Cmd equivalents.
FR8. iPadOS: full touch scheme (section 9), Apple Pencil treated as touch, hardware keyboard shortcuts, trackpad/mouse as first-class pointer, Split View not supported in v1 (fullscreen only, declared).
FR9. iOS (iPhone): same touch scheme with the phone adaptations in 9.3.
FR10. Lifecycle: on iOS, backgrounding pauses simulation and rendering, autosaves within the platform's ~5 second budget, and resumes cleanly; audio session handles interruptions (calls, Siri) without killing music.
FR11. LAN multiplayer: compiled in but gated behind Settings > Experimental on macOS only, using Vanilla Conquer's existing UDP transport. Absent from iOS builds in v1.
FR12. Accessibility baseline: respects system text-size for our native (non-game-buffer) UI, VoiceOver labels on all native chrome (setup, settings, import), Reduce Motion honored in our transitions, color-blind-safe status colors in the importer.
FR13. Public repository: an exceptional screenshot-led README with an original RAtouch banner, concise platform comparison, touch-control diagrams, honest build/test status, verified setup instructions, contributor links, and prominent bring-your-own-data/legal boundaries. It must contain no EA asset, screenshot, logo, or derived commercial art.

---

## 3. Technical architecture

### 3.1 Repo structure

Fork `TheAssemblyArmada/Vanilla-Conquer` to a new org repo (keep full history for GPL provenance; do not squash). Layout additions, following the conventions the codebase already uses:

```
common/            existing portable engine + backends (upstream-owned; minimize diffs)
  soundio_sdl2.cpp     NEW audio backend implementing common/soundio_imp.h
  vqaaudio_sdl2.cpp    NEW VQA audio path for the SDL backend
  wwtouch.cpp/.h       NEW touch gesture state machine (platform-agnostic, unit-tested)
  paths_posix.cpp      MODIFIED: TARGET_OS_IPHONE branches (bundle resources, App Support, Caches)
  video_sdl2.cpp       MODIFIED: expose Get_Render_Rect/Get_Game_Resolution/Set_Video_Mouse
                       hooks if not already sufficient (Vita fork shows the shape)
redalert/          game logic; goal: zero diffs vs upstream except hook consumption
apple/             NEW: everything Apple-specific and non-engine
  macos/               menu bar glue, Sparkle-free update check stub, DMG/notarize scripts
  ios/                 project.yml (XcodeGen), Stub/main.m + Info.plist, entitlements,
                       import UI (Swift, UIDocumentPicker), launch screen, asset staging
  shared/              ISO9660 reader, MIX validator (known-hash table), first-run logic
cmake/             iOS toolchain wiring, bundle resources
.github/workflows/ macos.yml (exists upstream), ios.yml (NEW: build + simulator smoke)
docs/              PORTING notes, provenance, third-party licenses
```

Policy: engine changes go upstream as PRs where OmniBlade will take them (hooks, backends); product code stays in `apple/`. This keeps the fork rebaseable against a single-maintainer upstream.

### 3.2 Renderer

Keep Vanilla Conquer's pipeline exactly as is: 8-bit palettized game surface at 640x400, CPU palette conversion, streaming `SDL_Texture`, `SDL_Renderer` present. On Apple platforms SDL selects Metal automatically; verify with `SDL_GetRendererInfo` at startup and log it. Rendering work items are limited to: scale-filter setting (nearest for integer factors, sharp-bilinear via `SDL_ScaleModeLinear` on a pre-integer-scaled target for fractional factors), letterbox color, and honoring `SDL_WINDOW_ALLOW_HIGHDPI` so the drawable is native-resolution. Do not adopt ra-port's renderer; do not add shaders in v1.

### 3.3 Platform abstraction

- Audio: implement `common/soundio_imp.h` (13 functions) on SDL2 audio; port the queue/mixer semantics from `soundio_openal.cpp` (247 lines) and ra-port's proven SDL mixer (`PORT/MAC/mac_audio_stub.cpp`) as references. This removes OpenAL from Apple builds entirely (OpenAL.framework is deprecated since iOS 15; openal-soft is LGPL and would complicate static linking). Keep the OpenAL backend selectable for other platforms.
- Input: touch enters through a new `wwtouch` module translating fingers into the existing virtual-mouse hooks (`Put_Mouse_Message`, `Move_Video_Mouse`, `Set_Video_Mouse`), the exact pattern proven by the Vita fork (+138 lines in `wwkeyboard.cpp`) and refined by the Generals port. Set `SDL_HINT_TOUCH_MOUSE_EVENTS=0` and additionally drop `SDL_TOUCH_MOUSEID` mouse events.
- Paths (iOS): read-only data from the bundle is not applicable (we bundle no assets); imported assets live in Application Support (backed up), caches in Caches, saves in Application Support/saves, logs capped in Documents for user-visible retrieval (Generals-port pattern: current + previous session, 8 MB cap).
- Lifecycle (iOS): `SDL_SetEventFilter` for immediate `SDL_APP_*` delivery; two atomics (backgrounded, inactive) gating both sim and render, 50 ms idle sleep while gated; autosave on WILLENTERBACKGROUND.

### 3.4 Build system

CMake stays the single source of truth for the engine. macOS: existing `MAKE_BUNDLE` path, universal2 (`arm64;x86_64`). iOS: `-DCMAKE_SYSTEM_NAME=iOS` producing a static-SDL2 `MACOSX_BUNDLE` (ra-port's `ios/CMakeLists.txt` is the working reference), combined with the Generals port's XcodeGen shell-app pattern for signing: a stub app target whose signed shell receives the CMake-built binary and re-signs (`package-ios-zh.sh` is the template; adapt, it is GPL-compatible). Pin SDL2 by tag with checksums. CI: GitHub Actions macos-14 for both platforms; iOS job boots the Simulator and runs a headless menu smoke test (adapt ra-port's `smoke_linux_menu.sh` concept).

### 3.5 Asset pipeline (in-app, not build-time)

Components in `apple/shared/`: ISO9660 reader (small, well-specified; agent task with test fixtures), MIX table of known files (name, size, SHA-256 for: freeware Allied/Soviet ISO contents, Steam 2229840 layout, Remastered CNCDATA layout, official demo), staging into Application Support with atomic rename, and a provenance record (which source the user imported, shown in Settings). The engine then finds files via the existing `PathsClass` search order; on iOS add the staged directory to the search path in `startup.cpp` hook (one-line change upstream may accept).

### 3.6 Save system

Do not change the save format in v1. Add: version-stamped save directory per app-build-ABI (the engine's `SAVEGAME_VERSION` already hard-rejects mismatches; surface that as a human-readable message instead of a silent fail), autosave slot, and a Testing gate (T6) that fails release if a gameplay-struct change alters `SAVEGAME_VERSION` unintentionally. iCloud sync of the saves directory is v1.1 (NSUbiquitousContainer, no engine changes).

### 3.7 Packaging

- macOS: `.app` from CMake, dylibbundler for SDL2 (or static SDL2 to simplify notarization), Developer ID signing, notarization via `notarytool`, stapled DMG. Scripts in `apple/macos/`, run in CI on tags.
- iOS/iPadOS: single binary, `TARGETED_DEVICE_FAMILY "1,2"`, deployment target iOS 15 (raises floor above ra-port's 13 for modern APIs; ~99% device coverage), landscape-only, `UIRequiresFullScreen`, `UIFileSharingEnabled`, `LSSupportsOpeningDocumentsInPlace`, `UIApplicationSupportsIndirectInputEvents`, privacy manifest declaring zero data collection.

---

## 4. Distribution decision

**Recommendation: three-channel strategy, committed now.**

1. **Open repo, always.** GPLv3 makes this mandatory for any distribution, and it is also the community-trust play. Tagged releases with full corresponding source.
2. **macOS 1.0: notarized direct download (DMG on GitHub Releases), not the Mac App Store.** Defense: Developer ID distribution has zero GPL tension (no store terms interposed on conveyance), notarization is a technical gate we control, the target audience is comfortable with DMGs, and it ships weeks earlier. The Mac App Store adds sandbox entitlement work and the same GPLv3 ambiguity as iOS for negligible reach among this audience. Revisit for v2 only if iOS review succeeds.
3. **iOS/iPadOS: TestFlight beta, then App Store submission, with eyes open.** Defense: Apple does not screen for GPL; every historical removal followed a rights-holder complaint; ScummVM (GPL, App Store since December 2023) and iDOS 3 (GPLv2, approved August 2024) are live precedents; EA has taken no action against any 2025-2026 C&C port including one covered by mainstream press. The residual risks are (a) EA complaint leading to removal, and (b) an App Review objection to bring-your-own-data, mitigated by shipping a functional-at-review experience (the Setup flow itself, plus, if the demo-content legal check clears, the official demo mission as reviewable content) and review notes citing the ScummVM pattern. If rejected or removed: AltStore PAL (EU) requires only notarization; worldwide sideload instructions remain in the repo. TestFlight precedes the store because a rejection there is quiet and informative.

Accompanying actions: send the EA outreach letter before first TestFlight (asking for written comfort or a GPLv3 section 7 additional permission for App Store conveyance; low probability, high value, one hour of work); pre-draft the takedown response plan; never use EA marks in the app name, icon, bundle ID, or screenshots.

**Pricing: free, no monetization.** EA's modding guidelines and the asset posture both point one way; charging money multiplies every legal risk and invites the complaint we are trying not to receive. Costs are one Apple developer account.

---

## 5. Multiplayer decision

**Recommendation: defer beyond experimental macOS LAN.** v1 ships singleplayer + skirmish. Vanilla Conquer's UDP LAN (`wsproto.cpp`/`wspudp.cpp`) stays compiled in and exposed behind Settings > Experimental on macOS only. iOS LAN is deferred (local-network permission, sandbox socket behavior, and cross-device lockstep desync testing are each real work), and internet play is out of scope (it is a relay service commitment, and CnCNet already serves that need for desktop players). Defense: the lockstep model makes cross-platform determinism the single most expensive class of bug to chase (any struct padding or float divergence desyncs); no reference port has it working on iOS; and the audience value of v1 is overwhelmingly the campaigns and skirmish. Revisit in v2 with a dedicated desync-hunting phase and a relay decision.

---

## 6. macOS roadmap

1. **First playable (week 1).** Build upstream `vanillara` with `MAKE_BUNDLE=ON` on Apple Silicon; verify campaigns, skirmish, movies, audio with Steam-sourced assets. Almost no code: upstream CI already produces universal2 bundles. Exit: 2-hour play session, no crashes, issues logged.
2. **SDL-audio backend (weeks 1-2).** `soundio_sdl2.cpp` + `vqaaudio_sdl2.cpp`; A/B against OpenAL on desktop; upstream PR.
3. **App polish (weeks 2-4).** Menu bar, Preferences window mapping to the existing INI settings, Retina scale-filter behavior, app icon, first-run Setup flow with the shared importer, crash-safe quit with save prompt.
4. **Sign, notarize, ship beta (week 4).** Developer ID + notarytool in CI; DMG; a public beta tag on the repo.
5. **Stabilization to 1.0 (weeks 5-8, overlapping iOS work).** Beta feedback burn-down; Intel Mac validation (CI builds it; test on one Intel machine); T-series gates in section 12.

## 7. iPadOS and iOS roadmap

1. **Platform bring-up (weeks 3-5).** CMake iOS toolchain + static SDL2 + stub audio first, then the SDL-audio backend from macOS work; XcodeGen shell app; boots to menu in Simulator, then on device with dev signing. Reference files: ra-port `ios/CMakeLists.txt`, `PORT/IOS/src/ios_main.cpp`, Generals `ios/project.yml` + `package-ios-zh.sh`.
2. **Lifecycle + paths (week 5).** Event-filter lifecycle gate, autosave on background, Application Support layout, capped session logs.
3. **Touch layer (weeks 5-8).** `common/wwtouch.cpp` per section 9, unit tests ported from ra-port's `mobile_touch_gesture_test.cpp` style, then human-led device tuning rounds on iPad, then iPhone adaptations.
4. **Import flow (weeks 6-8).** Document picker UI in Swift, ISO reader, hash validation, provenance screen. Files-app drop support as the secondary path.
5. **Keyboard, trackpad, pencil (week 8).** Verify indirect input arrives as mouse; map hardware keys; treat Pencil as touch (no hover behaviors in v1).
6. **Performance and battery (week 9).** Frame pacing at 60 (cap by default; 120 optional), thermal soak, energy log review; the workload is small, so this is verification, not optimization.
7. **TestFlight beta (week 9-10).** Internal, then external with the review-notes package. iPhone enters external beta only after the 9.3 adaptations pass on-device evaluation.
8. **App Store submission (week 12+).** After beta stability and the EA-letter waiting period.

---

## 8. Dependencies and third-party components

Pinned and vendored-or-fetched-with-checksums, nothing floating: SDL2 (zlib license; latest 2.30.x tag; static on iOS, bundled dylib or static on macOS), Vanilla Conquer upstream (pin SHA per release; ce83b59 at time of writing), XcodeGen (build-time tool only, not shipped), and nothing else in v1. Explicitly excluded: OpenAL/openal-soft on Apple builds (replaced by the SDL audio backend, removing a deprecated framework and an LGPL question), DXVK/MoltenVK (Generals-only problems; RA needs neither), and any analytics or crash-reporting SDK (zero-data-collection posture; crash evidence comes from the capped session logs and user-initiated sharing).

---

## 9. Touch control design

Foundation: the Generals port's deferred-commit state machine, adapted to RA's classic control scheme. RA95 semantics: left click selects; left click on ground with selection issues move; left drag = selection box; right click deselects; edge/keyboard scrolls the map; sidebar is click-driven with build queue clicks and repeat-clicks.

### 9.1 Core gesture grammar (iPad and iPhone)

- **Tap:** synthesize cursor motion at the point, wait one frame (hover states), then left down + up at the same point. Selects units, issues orders, presses sidebar buttons.
- **Drag (threshold 8 px):** left down anchored at the initial touch point, then motion stream; finger up = left up. This is drag-box selection and works because the anchor preserves where the finger first landed. If a fast/coalesced touch crosses the threshold only at finger-up, promote it to the same anchored sequence instead of committing a tap.
- **Long press (600 ms, stationary within dead zone, timer polled from the frame loop):** right click. Deselects. Haptic tick (UIImpactFeedbackGenerator, light) confirms it.
- **Two-finger drag:** map pan. Implement as direct viewport scroll (RA exposes keyboard/edge scrolling; drive `redalert/scroll.cpp`'s path the way mainline's controller right-stick already does, which is cleaner than ra-port's arrow-key pulses). Second finger during a pending tap cancels the tap; during an active drag, the drag is completed first (Generals rule).
- **Pinch:** display zoom of the rendered frame (2x steps between 1x, 1.5x, 2x), implemented at the presentation layer (scale the destination rect), not in the engine. RA has no engine zoom; presentation zoom plus pan gives phone users target sizes that meet touch standards. Double-two-finger-tap resets to fit.
- **Finger canceled (system interruption):** never commits a tap.
- **Edge behaviors:** no edge scroll on touch (two-finger pan replaces it); pointer users keep edge scroll.

### 9.2 iPad layout

Direct interaction with the original 640x400 frame, letterboxed. The original sidebar at iPad scale yields roughly 9-12 mm buttons (640x400 stretched to a 10-13 inch panel), which clears the 44 pt bar without redesign. Additions are rendered as a native pass-through overlay outside the game buffer: a compact 44-point command tab opens a temporary two-column deck. It exposes Stop, Guard, Scatter, Next Unit, Base, and Select View plus one-shot Attack+, Move+, Add+, and Queue+ modifiers after repeated skirmish sessions showed that keyboard-free modifier access materially improves play. It collapses after an immediate command; an armed modifier is shown on an amber, relabeled tab until the next tactical action consumes it. A later live Allied mission proved the remaining control-group need: assigning a visible formation, switching to a single unit, and instantly recovering the formation was valuable and otherwise unavailable without a keyboard. The resulting Groups sheet exposes Recall and Assign selected modes over slots 1–0 while sending only the original number and Control-number inputs. All controls are VoiceOver-labeled and drive the original engine key semantics. Do not duplicate Options, Repair, Sell, Map, build queues, or sidebar scrolling because the original UI already exposes them. The command tab can switch sides for left-handed play.

### 9.3 iPhone adaptations

640x400 mapped to a 6.1-inch screen fails the 44 pt test for sidebar and small units, so iPhone mode adds: default presentation zoom 1.5x with two-finger pan always available; a sidebar drawer: the game's sidebar area is covered by a native, enlarged rendition of the build queue (tapping a native tile synthesizes the click into the corresponding game-sidebar coordinates), toggled by a persistent 44 pt tab on the right edge; radar taps open a native fullscreen map overlay rather than relying on the tiny in-frame radar. These are overlays and synthesized clicks only; the engine is untouched. iPhone ships only if the 9.3 prototype passes human playtest; otherwise iPhone slips to 1.1 and the store listing is iPad-first. This is an honest fallback, stated now.

### 9.4 Explicit v2 investigation (not v1)

Engine-level UI scaling or widescreen (reworking `display.cpp`/`sidebar.cpp` coordinate assumptions). High cost, high reward for iPhone; do not let it leak into v1.

---

## 10. UX

**First launch.** Three screens: (1) what this is (engine, no game data, GPL, not affiliated with or endorsed by EA); (2) "Get your game files" with three cards: Steam ($20, cleanest, both expansions), freeware ISOs (link-out to a docs page in the repo, not in-app hosting), Remastered Collection (partial, Soviet movies missing, say so); (3) Import: document picker, drag-in on iPad, or "copy into the app's folder in Files." Progress per file with checkmarks; on success, straight into the main menu with a subtle "imported from: Steam layout" provenance note in Settings.

**Error handling.** Import errors are specific and recoverable: unknown file (show expected names), hash mismatch (likely a patched/modded file; allow "use anyway" with a warning), ISO missing MAIN.MIX (explain Allied vs Soviet disc contents), insufficient space. Engine-side missing-file failures at runtime route back into Setup rather than crashing. Save-version mismatch after an update produces a plain-language dialog naming the incompatible saves.

**Settings.** Display (filter, letterbox color, fps cap), Audio (music/SFX sliders mapped to existing INI), Controls (long-press duration 400-800 ms, drag threshold, strip on/off, left-handed strip side, invert two-finger pan), Data (imported sources, re-run Setup, open saves in Files), About (versions, GPL text, EA notice verbatim, source link, third-party licenses).

**Accessibility.** Native chrome fully VoiceOver-labeled; game buffer itself is out of VoiceOver scope in v1 (declared honestly in the accessibility statement). System text size respected in native UI. Haptics can be disabled. Color-blind-safe importer status. Full keyboard operability of native chrome on iPad.

---

## 11. Phase plan

Effort unit: agent-days (ad) = one autonomous agent working with review; human gates marked H. Risk: L/M/H.

| # | Phase | Objective | Depends | Files likely affected | Acceptance criteria | Risk | Effort | Agent-autonomous? |
|---|---|---|---|---|---|---|---|---|
| 0 | Experiments | The four disconfirming experiments from the feasibility report | none | scratch | All four answered and written up | L | 4 ad + H review | Mostly (EA letter is H) |
| 1 | Fork + CI | Org repo, upstream remote, macOS CI green, Linux CI green | 0 | .github/workflows | CI badges green on fork | L | 1 ad | Yes |
| 2 | macOS first playable | Signed local build plays campaigns | 1 | none (build only) | 2-hour session log, issue list | L | 2 ad + H playtest | Playtest is H |
| 3 | SDL audio backend | Remove OpenAL on Apple builds | 1 | common/soundio_sdl2.cpp, vqaaudio_sdl2.cpp, common/CMakeLists.txt | A/B audio parity checklist vs OpenAL; upstream PR opened | M | 5 ad | Yes, with H listening test |
| 4 | iOS bring-up | Menu on Simulator + device | 3 | cmake/, apple/ios/, paths_posix.cpp | Boots, renders Metal, menu clickable via touch-as-mouse | M | 5 ad | Yes |
| 5 | Lifecycle + paths | Background-safe, sandbox-correct | 4 | apple/ios, startup hooks | 50 background/resume cycles, zero crashes, autosave present | M | 3 ad | Yes |
| 6 | Touch layer | Full 9.1 grammar + tests | 4 | common/wwtouch.*, wwkeyboard hooks, video hooks | Unit tests green; scripted gesture sim; H device sign-off | M | 8 ad + H tuning rounds | Grammar yes; tuning H |
| 7 | Import flow | FR1/FR2 complete | 4 | apple/shared, apple/ios import UI | All three source layouts import on device; error matrix passes | M | 6 ad | Yes, H copy review |
| 8 | macOS ship polish | Menu bar, prefs, DMG, notarize | 2,3 | apple/macos | Notarized DMG installs clean on Intel + AS | L | 4 ad | Yes |
| 9 | iPad polish | Strip overlay, keyboard/trackpad, haptics | 6,7 | apple/ios | FR8 checklist; H playtest sign-off | M | 6 ad + H | Split |
| 9a | Public repository | Banner, control visuals, platform story, verified README | 6,8,9 | README.md, docs/images | FR13 checklist; zero asset leakage; every command/link verified | L | 2 ad + H copy review | Split |
| 10 | iPhone mode | 9.3 drawer + zoom | 9 | apple/ios | H go/no-go playtest vs 44 pt audit | H | 6 ad + H | Split; go/no-go is H |
| 11 | Beta | TestFlight external + macOS public beta | 8,9 | metadata | Betas live; feedback channel; review notes pack | M | 2 ad + H | Submission is H |
| 12 | Hardening | Burn-down, soak, save-gate, perf | 11 | everywhere | Section 12 gates all green | M | 8 ad + H triage | Split |
| 13 | Store submission | iOS App Store attempt | 12 + EA letter window | metadata | Approved, or rejection documented and fallback executed | H | 1 ad + H | H decision-making |
| 14 | 1.0 + upstreaming | Tag, publish source, upstream PRs | 13 | all | GPL compliance checklist; upstream PRs for hooks/backends | L | 2 ad | Yes |

Critical path: 0 → 1 → 3 → 4 → 6 → 9 → 9a → 11 → 12 → 13. Total: roughly 60 agent-days plus 15-20 human sessions over 3-6 calendar months.

---

## 12. Testing plan

- T1 **Matrix:** macOS on Apple Silicon (M1 minimum) and one Intel Mac (last-supported OS); Simulator for smoke only; physical iPad (one A-series, one M-series), physical iPhone (one current, one iOS-15-floor device). Rationale: input timing, Metal behavior, and thermals do not reproduce in Simulator.
- T2 **Regression:** upstream's unit tests stay green; add wwtouch unit tests (gesture grammar as table-driven cases including FINGER_CANCELED, two-finger interrupt, dead-zone edges); headless menu smoke test in CI per platform.
- T3 **Campaign sweep:** scripted save-file corpus at key missions; each release candidate loads all corpus saves and plays 5 minutes unattended (agent-driven) on each platform.
- T4 **Asset compatibility:** import fixtures for all supported layouts (freeware ISO pair, Steam 2229840, Remastered CNCDATA, demo), plus adversarial fixtures (truncated MIX, renamed files, Aftermath-only).
- T5 **Performance:** frame-time capture at 1x/1.5x/2x zoom on lowest-spec devices; battery soak 60 minutes; thermal throttle check; launch-time measurement (M1, M2 metrics).
- T6 **Save gate:** CI computes the engine's `SAVEGAME_VERSION`; any change between release tags fails the build unless release notes declare a save break (target: never in 1.x).
- T7 **Lifecycle torture (iOS):** automated 100-cycle background/foreground with random dwell, call-interruption simulation, low-memory warnings.
- T8 **GPL compliance check:** release job diffs the shipped binary's build inputs against the published source tag.
- T9 **Public-repo gate:** README images contain only original project artwork; an asset-leak scan rejects MIX/ISO/save payloads and commercial screenshots; every documented build command and local link is verified from a clean checkout.

---

## 13. Risk register

| Risk | Prob. | Impact | Mitigation |
|---|---|---|---|
| EA complaint removes the app post-launch | Low-Med | High | Neutral naming, no assets, no revenue; EA outreach letter; fallback channels ready (direct macOS, AltStore PAL, sideload docs); takedown response plan pre-drafted |
| App Review rejects bring-your-own-data | Med | Med | Functional Setup at review; demo-content option if cleared; review notes citing ScummVM/iDOS precedent; TestFlight first to surface objections quietly |
| GPLv3/App Store theory invoked (FSF-style complaint) | Low | Med | Only holders have standing; EA is the holder; outreach; open repo makes compliance visible |
| iPhone UX fails the quality bar | Med | Med | 9.3 prototype gate; ship iPad-first, iPhone in 1.1 if needed |
| Upstream single-maintainer stalls or diverges | Med | Low | Fork-and-maintain policy; upstream only hooks/backends; pin upstream SHA per release |
| Save-format break in an update angers users | Low | Med | T6 gate; struct-change review checklist |
| Touch heuristics feel wrong for RA specifically (differs from Generals' LMB-command scheme) | Med | Med | RA is select-then-order with the same LMB grammar; tuning rounds budgeted; settings exposure of thresholds |
| Cross-platform LAN desyncs (if experimental flag is used) | High (for that feature) | Low (flagged experimental) | macOS-only, labeled experimental, desync log capture |
| Freeware-ISO guidance draws criticism | Low | Low | Link-out only, provenance honesty, Steam as the promoted path |
| OpenAL removal introduces audio regressions | Low | Med | A/B parity checklist; keep OpenAL backend compiled for desktop until parity is signed off |

---

## 14. Release strategy

1. **Internal prototype** (end Phase 4): sideload, core team only.
2. **Friends beta** (end Phase 7): dev-signed iPad builds + macOS zip; 5-10 RA veterans; structured feedback form.
3. **Public macOS beta** (Phase 11): notarized DMG on GitHub Releases; announce in r/commandandconquer and CnCNet Discord with careful non-affiliation language.
4. **TestFlight external beta** (Phase 11): 200-500 testers; two-week minimum soak; this is also the quiet App Review probe.
5. **1.0**: macOS DMG + iOS App Store submission in the same week; AltStore PAL prepared in advance for EU fallback; source tag published simultaneously (GPL requirement, and the credibility moment).
6. **1.x cadence**: monthly patch train; 1.1 candidates: iPhone mode (if gated out), iCloud saves, Mac App Store evaluation; 2.0 candidates: engine UI scaling investigation, iOS LAN, CnCNet conversation.

---

## Appendix A: Reference implementations to keep open while coding

- Touch machine: `Generals-Mac-iOS-iPad/GeneralsMD/Code/GameEngineDevice/Source/SDL3GameEngine.cpp:136-369`; constants at :160-162.
- Vita touch hooks: Northfear/Vanilla-Conquer-vita, `common/wwkeyboard.cpp` (Handle_Touch_Event), video hook additions.
- iOS CMake: `ra-port/ios/CMakeLists.txt`; sandbox asset staging: `ra-port/PORT/IOS/src/ios_main.cpp`; landscape delegate: `ios_app_delegate.m`.
- Packaging: `Generals-Mac-iOS-iPad/scripts/build/ios/package-ios-zh.sh`; `ios/project.yml`; Info.plist keys at `ios/Stub/Info.plist`.
- Audio shim to implement: `Vanilla-Conquer/common/soundio_imp.h`; reference backends: `soundio_openal.cpp`, `ra-port/PORT/MAC/mac_audio_stub.cpp`.
- Renderer (do not change, just read): `Vanilla-Conquer/common/video_sdl2.cpp` (RenderSurface at ~797).
- Playbooks: `Generals-Mac-iOS-iPad/docs/port/PORTING_PLAYBOOK.md` and `PORTING_PATTERNS.md`.

## Appendix B: License obligations checklist (every release)

1. Full corresponding source tagged and public before binaries go live.
2. EA's LICENSE.md (GPL v3 + Additional Terms) reproduced in-app and in the repo, unmodified.
3. Modified-version marking: app About screen states this is a modified, unofficial port, not the original program.
4. No EA trademarks in name, icon, bundle ID, screenshots, or keywords; nominative references only, with "EA has not endorsed and does not support this product."
5. No EA assets in any distributed artifact, including screenshots for the store (use screenshots only after legal review of the screenshot question, or compose store art from original engine UI with imported user assets, which store rules and EA terms both complicate: default to stylized original artwork).
6. Third-party notices: SDL (zlib), any Swift packages.
