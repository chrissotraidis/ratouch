# Feasibility Report: Command & Conquer: Red Alert (1996) on macOS, iPadOS, and iOS

**Date:** July 21, 2026
**Status:** Research complete. Verdict at the end of this document: **Conditional Go.**
**Companion document:** `prd-build-plan.md` (PRD and step-by-step build plan)

---

## 1. Executive summary

Red Alert on Apple platforms is technically feasible with modest engineering effort, because almost all of the hard engine work has already been done in public, GPL-licensed code. The recommended foundation is **Vanilla Conquer** (TheAssemblyArmada/Vanilla-Conquer): a six-year-old, 973-commit, 37-contributor SDL2 + OpenAL port of Red Alert and Tiberian Dawn with a clean compile-time platform abstraction, working campaigns, skirmish, VQA video, LAN multiplayer, and a macOS universal2 `.app` CI pipeline that exists today. I compiled it in this study's Linux environment: 401 build targets, 2 compiler warnings, clean link, binary launches.

No public Red Alert port has shipped on iOS, but two 2026 projects prove every individual piece: **dk8827/ra-port** (a two-week-old, AI-assisted port of EA's Red Alert source) runs on iPhone and iPad as a debug build with a real touch gesture layer, and **ammaarreshi/Generals-Mac-iOS-iPad** (July 2026) shipped a far more complex C&C title on iOS with a refined ~230-line touch-to-mouse state machine, a signing and packaging pipeline, and a written porting playbook. Both are GPL v3 and license-compatible with Vanilla Conquer, so their mobile work can be adapted directly.

The two real risks are not engineering:

1. **Assets.** EA's GPL release covers code only. Every asset path (2008 freeware ISOs, Steam re-releases, Remastered Collection) requires the user to supply files, or requires distribution choices that rest on community tolerance rather than a written license. This is solvable with a Files-app import flow, exactly as ScummVM and iDOS 3 do on the App Store today.
2. **GPLv3 on the App Store.** EA is effectively the sole copyright holder and granted no App Store exception. Apple does not screen for GPL; removals happen only when a rights holder complains, and EA has not moved against any of the 2025-2026 community ports. This is a calculated risk, not a cleared one.

Bottom line: a ship-quality macOS build is weeks away, mostly polish. A ship-quality iPad/iPhone build is a 2 to 4 month project dominated by touch UX and UI scale work, not engine work. App Store distribution is plausible but revocable; the plan should treat the App Store as an attempt with fallbacks (notarized direct macOS distribution, TestFlight, AltStore PAL in the EU, open repo), not as a guaranteed endpoint.

---

## 2. Method and evidence base

Everything in this report is grounded in one of:

- **Local clones** of `TheAssemblyArmada/Vanilla-Conquer` (HEAD ce83b59, 2026-07-11), `dk8827/ra-port` (HEAD b270206, 2026-07-12), `ammaarreshi/Generals-Mac-iOS-iPad` (HEAD c5c8c4d3e, 2026-07-05), and `electronicarts/CnC_Red_Alert` (HEAD 0dc09bb, 2025-02-27), analyzed file by file with full git history.
- **Test compilation** performed for this study on Ubuntu 24.04 x86_64, SDL2 2.30.0, OpenAL Soft 1.23.1, CMake 3.28, Ninja: both Vanilla Conquer (`vanillara`, `vanillatd`) and ra-port (`redalert_linux`) configured and built to completion.
- **Primary web sources** fetched during the study: EA's LICENSE.md files (verbatim), Apple App Store Review Guidelines, SDL documentation and source, Steam/SteamDB listings, press coverage, and project wikis. Cited inline.

**Known evidence gap:** this environment's network policy blocked the GitHub issues/PR API for the two Red Alert repos, so live open-issue counts and issue text for Vanilla-Conquer and ra-port could not be retrieved. Where the brief asks for issue numbers, I cite merge-commit PR numbers, commit hashes, and wiki text instead, and I flag tracker-dependent claims. Re-running the issue queries from an unrestricted machine is a 15-minute task listed in the experiments section.

---

## 3. Verified background

**EA's February 27, 2025 release.** Confirmed: EA published full source for four titles under GPL v3: `CnC_Tiberian_Dawn`, `CnC_Red_Alert`, `CnC_Renegade`, and `CnC_Generals_Zero_Hour`. Each LICENSE.md carries the same "ADDITIONAL TERMS per GNU GPL Section 7" block. The material clauses, quoted from `electronicarts/CnC_Red_Alert/LICENSE.md`:

> "No trademark or publicity rights are granted. This license does NOT give you any right, title or interest in 'Command & Conquer' or any other Electronic Arts trademark. You may not distribute any modification of this program using any Electronic Arts trademark or claim any affiliation or association with Electronic Arts Inc. or its affiliates or their employees."

Plus a marking requirement for modified versions, an indemnification clause, and an expanded warranty disclaimer. All four are categories permitted by GPLv3 section 7, so the license remains valid GPLv3. **There is no asset grant and no additional permission for app stores.** The repo README states the code "does not fully compile" as released (it depends on the DirectX 5 SDK, DirectX Media 5.1, Greenleaf Communications Library, and HMI SOS, none of which EA could publish), is archived "for preservation purposes," and that "to use the compiled binaries, you must own the game." The EA repo itself has exactly 3 commits, all on 2025-02-27, by one EA employee (LFeenanEA), and has been static since; it is a source drop, not a project.

**The 2020 release.** Confirmed: `electronicarts/CnC_Remastered_Collection` (May 2020) released `TiberianDawn.dll`, `RedAlert.dll`, and the map editor sources under the same GPLv3 plus additional terms. This is the codebase Vanilla Conquer grew from.

**The background claims in the brief that did not fully survive verification:**

- ra-port's README does claim an iOS target, and the claim is real but narrow: `ios/` contains a 90-line CMakeLists and an Info.plist template, no Xcode project; `scripts/build_ios_debug.sh` generates one. It produces a working signed debug app for iPhone and iPad (deployment target iOS 13, `TARGETED_DEVICE_FAMILY "1,2"`), with the user's own game assets copied into the bundle at build time. The README explicitly says the iOS and Android builds are "for local development and testing only. Do not distribute."
- Vanilla Conquer's community ports to PS Vita and Nintendo DSi exist as third-party forks (for example Northfear/Vanilla-Conquer-vita, last commit 2023-12-23), not as mainline platforms.

---

## 4. Which base repo (Question 1)

### The candidates, measured

| | Vanilla Conquer | ra-port | EA source directly | Generals-Mac-iOS-iPad |
|---|---|---|---|---|
| First commit | 2020-03-30 | 2026-07-07 | 2025-02-27 (drop) | fork lineage to 2025-02-27; iOS work 2026-07-03 to 07-05 |
| Last commit at study time | 2026-07-11 | 2026-07-12 | 2025-02-27 | 2026-07-05 |
| Total commits | 973 | 40 | 3 | 2,003 (mostly upstream GeneralsX) |
| Contributors | 37 | 5 (one primary) | 1 (EA uploader) | 64 (upstream); ~1 for iOS layer |
| Commits, last 12 months | 12 | 40 | 0 | 1,384 (upstream velocity) |
| Linux test build (this study) | Clean, 2 warnings | Clean, 1,074 warnings | Does not compile (by design) | Not attempted (needs DXVK/MoltenVK) |
| Platform abstraction | Compile-time backend selection in `common/` | Win32 emulation shim layer (`PORT/MAC/`) | None (Win32/DirectX/asm) | SDL3 + DXVK/MoltenVK (3D, not relevant as a base) |
| macOS today | CI universal2 `.app` bundles | Apple Silicon build, no `.app` bundle | No | Yes (signed, sideload) |
| iOS today | None | Debug app, iPhone+iPad, touch | No | Yes (signed, sideload, refined touch) |
| Multiplayer | UDP LAN working | Stripped entirely | WOLAPI-era, dead deps | LAN (GameSpy replacement, upstream) |

### Recommendation: Vanilla Conquer as the engine base, with the mobile layer ported in from the other two

**Why Vanilla Conquer wins on architecture.** Its platform layer is a genuine abstraction, not an emulation. Backends are swapped at CMake time in `common/CMakeLists.txt:105-151`: `video_sdl2.cpp` vs `video_ddraw.cpp` vs `video_null.cpp`; `soundio_openal.cpp` vs `soundio_dsound.cpp`; `wwkeyboard_sdl2.cpp` vs `wwkeyboard_win32.cpp`; `paths_posix.cpp` vs `paths_win.cpp`. Game logic in `redalert/` (281 files) contains essentially no SDL (a grep matches only a dialog enum). The audio interface is a deliberately minimal 13-function C shim, `common/soundio_imp.h` (`SoundImp_Init`, `SoundImp_Buffer_Sample_Data`, ...), split out explicitly to ease new backends (Mark Olsen, commit of 2023-02-16). An iOS audio backend is therefore a ~250-line file, and the OpenAL dependency (deprecated by Apple since iOS 15, though still functional) can be dropped rather than fought.

ra-port took the opposite approach: it keeps EA's code shaped as Windows code and supplies fake Windows underneath: a 1,521-line `windows.h`, a 452-line `ddraw.h`, `dsound.h`, a 512-entry Win32 message queue in `PORT/MAC/mac_sdl_runtime.cpp` (888 lines) dispatching into the original `Windows_Procedure`. This is a legitimate fast-porting technique, and its correctness work is real (endianness header `CODE/RA_ENDIAN.H`, Watcom enum-width normalization in `TRIGGERWIDTH.H` from PRs #3/#5, LP64 fixes with regression tests, commits 1bb6137, e04dd07, a022f03). But as a foundation for a multi-year, multi-platform product it means maintaining a Win32 emulator forever, and the 1,074 compiler warnings in this study's build (vs Vanilla Conquer's 2) measure the difference. It is also six days of history by effectively one person, with multiplayer deleted from the build (`cmake/ra95_common.cmake:5-20` strips NETDLG, TCPIP, MPMGR and friends; `INTERNET_OFF` defined), while Vanilla Conquer's UDP LAN works.

**Why not EA's source directly.** It does not compile as released, requires four proprietary SDKs to be replaced, and both candidate ports above are exactly that replacement work already done. Starting there re-does ra-port's first week for no benefit.

**Why not OpenRA.** OpenRA is a from-scratch reimplementation with modernized gameplay, not the original engine; its project explicitly keeps mobile off the roadmap (OpenRA issue #21761), and it is C#/.NET, which complicates iOS. It is the right project for "modern RA multiplayer" and the wrong one for "the 1996 game, faithful, native on Apple platforms."

**What to take from the other repos.** From ammaarreshi/Generals-Mac-iOS-iPad: the touch gesture state machine (`GeneralsMD/Code/GameEngineDevice/Source/SDL3GameEngine.cpp:136-369`, constants `LONG_PRESS_MS = 600`, `TAP_DEAD_ZONE_PX = 8.0f`, `PINCH_STEP_RATIO = 0.06f`, verified in the clone), the XcodeGen "provisioning shell app" packaging pattern (`ios/project.yml`, `scripts/build/ios/package-ios-zh.sh`), the Info.plist recipe (`UIFileSharingEnabled`, `LSSupportsOpeningDocumentsInPlace`, `UIApplicationSupportsIndirectInputEvents`), the lifecycle pause pattern, and `docs/port/PORTING_PLAYBOOK.md`. From ra-port: the iOS CMake toolchain arrangement (`ios/CMakeLists.txt`, `-DCMAKE_SYSTEM_NAME=iOS`, generated Xcode project), the Android project as a fourth-platform reference, the mobile touch heuristics that agree with the Generals numbers (6 px drag slop, 650 ms long press in `PORT/MAC/include/mobile_touch_gesture.h:47,167`), and its test discipline for legacy-code correctness fixes. All three codebases are GPLv3 with EA's section 7 terms, so code can move between them.

One caveat to own: Vanilla Conquer today is effectively maintained by one person (OmniBlade authored nearly all 2024-2026 commits; 12 commits in the last 12 months). The plan should assume fork-and-maintain, contributing back where upstream wants it.

---

## 5. Existing iOS work on any Red Alert port (Question 2)

Exact state as of July 21, 2026:

- **dk8827/ra-port** is the only Red Alert codebase with any iOS code. What exists: `ios/CMakeLists.txt` (90 lines) + `ios/Info.plist.in`; `PORT/IOS/src/ios_main.cpp` (266 lines, copies bundled assets to the writable sandbox on first launch so saves work) and `ios_app_delegate.m` (103 lines, subclasses SDL's `SDLUIKitDelegate` to force landscape on iOS 16+). Built via `scripts/build_ios_debug.sh`, which downloads SDL2, generates an Xcode project, builds `redalert_ios` as a bundle with automatic signing (`com.raport.redalert`), simulator by default, device with `RA_IOS_DEVELOPMENT_TEAM=... --device`. Touch is real (shared `RA_MOBILE_TOUCH` gesture layer, not OS mouse emulation): tap = left click, drag past 6 px = drag select, 650 ms long press = right click, two-finger drag = map pan converted to arrow-key pulses, tap during movie = skip. iOS commits 67f4a7d and 57fb8df on 2026-07-09; the entire platform was brought up in about a day. There is no App Store packaging, no entitlements work, no document picker, no UI scaling, no pinch zoom, and the README forbids distributing the builds because they bundle the user's assets.
- **Vanilla Conquer** mainline has zero iOS mentions in code, CMake, or CI. macOS is first-class (`.github/workflows/macos.yml`: macos-14 runner, `CMAKE_OSX_ARCHITECTURES: "arm64;x86_64"`, MacPorts SDL2 + OpenAL Soft, dylibbundler, `.app` bundles with generated `.icns`, rolling `latest` release). `common/paths_posix.cpp` already includes `<TargetConditionals.h>` and Apple branches (`_NSGetExecutablePath`, `~/Library/Application Support/Vanilla-Conquer`) but no `TARGET_OS_IPHONE` branch yet.
- **No Red Alert port has been submitted to the App Store or TestFlight** as far as any public evidence shows. Searches found no submission, approval, rejection, or EA takedown for any C&C GPL port. The nearest precedent, the Generals iOS port, is deliberately sideload-only.

---

## 6. What SDL2 gives you on iOS, and what it does not (Question 3)

Free (verified against SDL's `docs/README-ios.md` and `src/render/SDL_render.c`):

- **Metal rendering.** SDL_Render has a Metal backend since 2.0.8, and on iOS/macOS the driver table orders Metal first, so the standard `SDL_Renderer` path used by Vanilla Conquer lands on Metal with no code change. This resolves the "Metal-only platforms" question: no OpenGL dependency exists in the recommended path.
- **App lifecycle.** SDL's internal `SDLUIKitDelegate` delivers `SDL_APP_WILLENTERBACKGROUND`, `DIDENTERBACKGROUND`, `LOWMEMORY`, etc. The documented pattern is to handle these via `SDL_SetEventFilter` for immediate delivery, with about 5 seconds to save state on backgrounding.
- **Touch.** Multi-touch finger events with IDs (`SDL_FINGERDOWN/MOTION/UP`), and the critical hint `SDL_HINT_TOUCH_MOUSE_EVENTS=0` to disable naive touch-as-mouse synthesis. On-screen keyboard via `SDL_StartTextInput`. Game controllers, and iPad trackpad/mouse as real mouse events.
- **Audio** via CoreAudio, **high-DPI** drawable support, an Xcode project template, and a zlib license with zero App Store friction.

Not free (all confirmed by omission in SDL docs and by what both 2026 ports had to build themselves):

- Touch-first UX semantics (the entire gesture state machine), UI scaling, document picker / Files integration for asset import (native UIKit code), iCloud, safe-area handling, background-audio session policy, privacy manifests, packaging, signing, notarization, TestFlight, and App Review posture.

SDL covers roughly the bottom half of the port. The top half is product work.

---

## 7. Renderer path (Question 4)

Red Alert is an 8-bit palettized 2D engine, and both ports render it the same proven way:

- **Vanilla Conquer** (`common/video_sdl2.cpp`, `VideoSurfaceSDL2::RenderSurface`, verified lines 797-844): the game draws into an 8-bit `SDL_Surface` with an attached palette; `SDL_BlitSurface` converts to a 32-bit intermediate on CPU; the result is uploaded to a `SDL_TEXTUREACCESS_STREAMING` texture and drawn with `SDL_RenderCopy` + `SDL_RenderPresent`, GPU-scaled with nearest or linear filtering and 16:10 aspect letterboxing (INI-configurable `Scaler`, `Boxing`, `BoxingAspectRatio`).
- **ra-port** (`PORT/MAC/mac_sdl_runtime.cpp:768-814`, `MacSDL_Present8`): CPU palette-to-ARGB8888 conversion into a streaming texture, letterboxed copy, `opengles2` driver with nearest filtering on mobile.

On iOS this exact pipeline runs on SDL's Metal backend unmodified. The bandwidth arithmetic is trivial: the game buffer is 640x400 (RA's hi-res mode; 320x200 in DOS mode), so the per-frame upload is about 1 MB at 32-bit, roughly 60 MB/s at 60 fps, which is noise against an A-series chip's tens of GB/s. The GPU then scales one quad. Palette animation (RA cycles palette entries) is handled in the existing convert step. What is required on Metal-only platforms is therefore: nothing new. The one renderer-adjacent product decision is the scale filter (nearest integer-ish scaling vs a sharp-bilinear shader for non-integer factors on Retina panels), which SDL's renderer can do with logical-size presentation.

The fixed 640x400 internal resolution is a UI constraint, not a renderer constraint; see section 9.

---

## 8. Input and touch (Question 5)

What exists, by codebase:

- **Vanilla Conquer mainline:** full SDL2 mouse/keyboard (`common/wwkeyboard_sdl2.cpp`, 338 lines), relative and absolute mouse modes with render-scale mapping (`Get_Video_Mouse` in `video_sdl2.cpp:448-460`), hardware color cursors, and **gamepad support in mainline since 2021-11-19** (SDL_GameController, axis-driven virtual mouse, right-stick map scroll consumed by `redalert/scroll.cpp`). No touch code.
- **Northfear's Vita fork** (diff vs merge-base 59efc894: 36 files, +770/-17): added `Handle_Touch_Event` to `wwkeyboard.cpp` (+138 lines), mapping front-screen fingers to game coordinates through three small hooks it added to the video layer (`Get_Game_Resolution`, `Get_Render_Rect`, `Set_Video_Mouse`), synthesizing clicks through the existing `Put_Mouse_Message`. This is the template proving mainline's hooks are where an iOS touch layer plugs in, in a few hundred lines.
- **ra-port mobile:** the header-only gesture machine `PORT/MAC/include/mobile_touch_gesture.h`, unit-tested in `tests/mobile_touch_gesture_test.cpp`: 6 px drag slop, 650 ms long press to right click, two-finger pan emitting arrow-key pulses every 24 logical px, movie tap = ESC, Android back = ESC. No pinch zoom, no virtual buttons.
- **Generals-Mac-iOS-iPad:** the most refined scheme, ~230 self-contained lines (`SDL3GameEngine.cpp:136-369`) with hard-won heuristics documented in comments: send nothing but a cursor motion on finger-down (a premature click sets rally points); deliver tap down/up at the original press position (dense UI buttons miss otherwise); anchor drag-select at first touch; poll the long-press timer from the frame loop because a stationary finger produces no SDL events; treat `FINGER_CANCELED` (incoming call) as "not a tap"; drop mouse events with `which == SDL_TOUCH_MOUSEID` as a second line of defense; synthetic events must carry a valid windowID or coordinate scaling silently fails. Two-finger drag pans the camera, pinch maps to wheel ticks at 6% distance change per tick.

Reusability verdict: the Generals state machine is the design to adapt (it has no Generals dependencies beyond "inject an SDL mouse event"), the Vita fork shows exactly which Vanilla Conquer files to touch, and ra-port independently converged on nearly identical constants, which is good evidence the numbers are right. Touch input is a solved design problem; the remaining work is adaptation and tuning on hardware.

---

## 9. The UI problem (Question 6)

This is the largest genuine product problem. Facts:

- Vanilla Conquer renders RA's original fixed layout at 640x400 (`GBUFF_INIT_WIDTH/HEIGHT` in `redalert/externs.h:61-65`), GPU-upscaled whole-frame. There is no widescreen, no virtual resolution, and no per-element UI scaling in either RA port. (The 3072x3072 buffer path exists only for `REMASTER_BUILD`, where the Remastered client owns the UI.) The sidebar, radar, menus, and fonts assume 640x400 mouse-precision hit targets.
- ra-port is the same: 640x400 letterboxed, nearest-filtered; its README lists "mobile input/UI polish" as future work. On an iPhone, 640x400 upscaled means sidebar buttons around 5-7 mm, marginal against Apple's 44 pt guidance, and text that is legible but small.
- The Generals port dodged this because its 2003 engine has resolution-aware UI scaling; it injects the panel's native pixel size as `-xres/-yres`. RA's 1996 engine has no such facility.

What an iPad-native experience requires, in increasing order of cost: (a) whole-frame scale with letterboxing and a touch layer, which is what exists and is acceptable on iPad at 640x400 into a ~10-13 inch panel (this is the v1 iPad answer); (b) enlarged touch-side affordances rendered outside the game buffer (native overlay for critical actions, larger hit-slop mapping in the touch layer so taps near a sidebar button snap to it); (c) true engine-level UI scale or widescreen, which means reworking coordinate assumptions across `redalert/display.cpp`, `sidebar.cpp`, and dialog code, a known large job that community hi-res patches for RA95 historically found painful. The PRD scopes (a)+(b) for v1, treats (c) as a v2 investigation, and treats iPhone as a supported-but-secondary form factor with a zoomed sidebar interaction mode, because 640x400 direct mapping on a 6-inch panel cannot meet the 44 pt bar honestly.

---

## 10. Asset legality and acquisition (Question 7)

What the code needs (canonical, from Vanilla Conquer's wiki and OpenRA's manual-install docs): `REDALERT.MIX` plus the Allied and Soviet `MAIN.MIX` files for the full game with movies and music; `EXPAND.MIX`/`EXPAND2.MIX`/`HIRES1.MIX` and the expansion `MAIN.MIX` files for Counterstrike/Aftermath. Vanilla Conquer locates them by search path (`Paths.Init(... "REDALERT.MIX" ...)` at `redalert/startup.cpp:292`: executable dir, then data path, then per-user path, macOS `~/Library/Application Support/Vanilla-Conquer/vanillara`).

Legal sources a user can supply:

1. **The 2008 freeware release.** EA released the full Allied and Soviet CD ISOs as freeware on August 31, 2008 (13th anniversary promotion for RA3). Expansions were not included. EA's own downloads disappeared quickly; ModDB and archive.org have mirrored the ISOs for 18 years without takedown, and CnCNet redistributes repackaged freeware titles, stating plainly that it is legal to do so. Caveat found in this study: the exact text of EA's 2008 announcement could not be retrieved (archive fetch failed), so the precise redistribution grant is unverified. Community practice plus EA's sustained non-enforcement is strong but is not a license you can hand to Apple.
2. **Steam, since March 7, 2024.** "Command & Conquer Red Alert, Counterstrike and The Aftermath" is sold standalone (Steam app 2229840, about $20), shipping the Windows 95 game with original MIX files, both discs' movies, and both expansions. This is the cleanest "user owns it" source. Not on GOG (Dreamlist only).
3. **C&C Remastered Collection (2020).** Contains original classic MIX files under `CnCRemastered/Data/CNCDATA/RED_ALERT/`, but the Soviet disc's movies are absent (documented in Vanilla Conquer's install wiki), so it is a partial source.
4. **The official RA demo** (one mission per side, fully playable per Vanilla Conquer's README) exists and could serve as a review-time and first-run content path, with the same informally-freeware caveat as the ISOs.

**In-app import under the iOS sandbox** is a solved pattern with App Store precedent: ScummVM (on the App Store since December 2023) documents Files-app copy, iTunes/Finder file sharing, iCloud Drive, and cloud-provider import; Delta, PPSSPP, RetroArch, and iDOS 3 (approved August 12, 2024 after Apple's guideline 4.7 revisions) all ship empty and let users bring content through the document picker. The plan: `UIFileSharingEnabled` + `LSSupportsOpeningDocumentsInPlace` + a `UIDocumentPickerViewController` flow that accepts the Steam folder, the freeware ISOs (mount/parse the ISO9660 in-app and extract the MIX files), or bare MIX files, validates them by hash/size, and copies them into Application Support. Bundling assets in a distributed binary is ruled out; ra-port's README says the same about its own debug builds.

**Never do:** OpenRA-style self-hosted asset mirrors for an App Store app. OpenRA's own FAQ frames its mirror as an assumption of individual-use tolerance and rules out commercial contexts.

---

## 11. Licensing and distribution (Question 8)

The full analysis is in section 3 (license terms) plus the following distribution facts:

- Apple does not reject apps for being GPL. Removals (GNU Go 2010, VLC 2011) happened when a copyright holder complained. VLC returned in 2013 only after a rewrite and relicense (MPLv2 + GPLv2 dual), which demonstrates both the risk and the cure: the copyright holder's consent is what matters.
- Live precedent favors the attempt: ScummVM (GPL) has been on the App Store since December 2023, published by the project itself; iDOS 3 (GPLv2 DOSBox) approved August 2024; Wesnoth has had an App Store presence for years. The theoretical GPLv3 objections (anti-tivoization, Apple's usage rules as "further restrictions") remain unlitigated, and the FSF-side reading has never been enforced against a party other than by holder complaint.
- For this project, EA is the sole copyright holder of the engine, granted no App Store exception, and also holds the trademarks. Two obligations are hard and clear regardless of venue: the app cannot be named or marketed with "Command & Conquer" or "Red Alert" (EA's section 7 terms; use a neutral name with nominative-use description text), and full corresponding source for the exact shipped build must be published.
- Fallback channels are real: notarized Developer ID distribution on macOS has none of the App Store's GPL tension; TestFlight is Apple-mediated and shares App Store risk; AltStore PAL (EU, since April 2024) removes App Review content rules but still requires notarization (the UTM SE episode shows notarization is itself a chokepoint Apple has used, though Apple reversed under pressure in July 2024).
- EA's posture since February 2025 has been permissive in practice: the Generals iOS port got mainstream press in July 2026 (PC Gamer and others) with no documented EA reaction, and CnCNet has operated for a decade. Silence is not consent, but the base rate of enforcement is low and the first enforcement step would almost certainly be a takedown, not damages.

The distribution recommendation and its defense live in the PRD; summary: open repo always, notarized direct download for macOS at v1, TestFlight then App Store attempt for iOS under a neutral name with user-supplied assets, AltStore PAL as the EU fallback, and a pre-drafted response plan for a takedown. A parallel, polite outreach to EA's community team asking for a written blessing (or even a section 7 additional permission) is cheap and has asymmetric upside.

---

## 12. Multiplayer (Question 9)

- **Vanilla Conquer:** original IPX connection classes retained but framed over UDP (`common/wsproto.cpp`, `common/wspudp.cpp`, `UDPInterfaceClass` instantiated at `redalert/init.cpp:954`); broadcast LAN discovery; lobby code in `redalert/netdlg.cpp` and `session.cpp`; gated by the `NETWORKING` CMake option (default ON). Working state: UDP LAN between Vanilla clients cross-platform. No Westwood Online (never compiled in), no modem, no CnCNet integration (only an INI-compat comment at `common/settings.cpp:50`). Internet play would need a relay/tunnel layer.
- **ra-port:** multiplayer deleted from the build entirely.
- **Realism for v1:** LAN on macOS is nearly free (it exists). On iOS it needs the local-network permission prompt, socket behavior in the sandbox, and cross-device desync soak testing; the RTS lockstep model means one divergent float or struct layout desyncs the match, and Vanilla Conquer's own 2024 commit stream still fixed sim bugs. Verdict: ship v1 with singleplayer + skirmish; include LAN as an "experimental" flagged feature on macOS only; defer iOS LAN and any online service to v2. Online (CnCNet-style relay) is a service commitment, not a feature, and should not gate v1.

---

## 13. Save/load, audio, video, lifecycle (Question 10)

- **Saves:** Vanilla Conquer uses the original Westwood machinery (`redalert/saveload.cpp`): Blowfish + LCW over raw struct dumps, with `SAVEGAME_VERSION` computed from the sizeof of 30+ classes and a hard reject on mismatch. Consequence: saves are compatible only between builds with identical class layout; not with original RA95, and potentially not across app updates that touch gameplay structs. The PRD treats save-format stability as a release-gate check and adds iCloud Drive sync of the save directory as a later feature. ra-port's saves work but its own README lists "save/load hardening" as open work.
- **Audio:** Vanilla Conquer's mixer sits above the 13-function `soundio_imp` shim; the OpenAL backend is 247 lines. Apple deprecated OpenAL.framework in iOS 15 (still functional); options are static openal-soft (LGPL, needs care) or a new SDL-audio backend, which the shim makes straightforward and which removes a dependency class entirely. ra-port already proved SDL-audio-only works (5-voice software mixer with Westwood ADPCM in `mac_audio_stub.cpp`, 788 lines).
- **Video:** Vanilla Conquer has the full VQA player (`common/vqa*.cpp`) with OpenAL audio and 2x interpolation for hi-res playback; working. ra-port rewrote a VQA player behind the original interface (`PORT/MAC/mac_vqa.cpp`, 1,221 lines); its intro plays with sound. No codec risk on Apple platforms since decoding is pure CPU code.
- **Lifecycle:** the Generals port documents the sharp edge: iOS resign-active (app switcher) is distinct from backgrounding, and touching the Metal drawable in either state eventually kills the process; the fix is an event-watch that sets atomics gating both simulation and rendering, with a 50 ms idle sleep. The same pattern applies verbatim to SDL2. Backgrounding must also trigger an autosave inside the ~5 second budget, which RA's fast save supports. Memory is a non-issue for RA (tens of MB working set vs the Generals port's ~3 GB).

---

## 14. Autonomous agents vs human judgment

Well suited to autonomous coding agents (evidence: ra-port was substantially built by agents in six days, and the Generals port's README credits the C++, cross-builds, and device debugging to Claude Code):

- The iOS/macOS platform backend work in Vanilla Conquer (`TARGET_OS_IPHONE` path branches, SDL-audio `soundio_imp` backend, CMake iOS toolchain, XcodeGen shell-app packaging scripts adapted from the Generals port).
- Porting the touch state machine and writing unit tests for it (both reference implementations have tests to crib).
- Asset importer plumbing (ISO9660 parsing, MIX validation by known hashes, file staging), settings UI wiring, CI pipelines, notarization scripts.
- Regression testing harnesses, headless smoke tests (ra-port's `smoke_linux_menu.sh` is a template), save-format canary tests.

Requires human judgment, design, or hands-on hardware:

- Touch UX tuning on physical devices (every constant in the gesture machine was tuned by a human playtesting; the Generals README is explicit that the human "described symptoms").
- iPad/iPhone UI layout decisions, sidebar ergonomics, accessibility choices, visual identity and naming under the trademark constraints.
- All licensing/distribution decisions, EA outreach, App Review conversations and appeal letters.
- Multiplayer desync forensics if LAN is pursued (lockstep divergence debugging is slow, stateful, and device-bound).
- Final performance and battery validation on hardware; App Store screenshots/metadata taste.

---

## 15. Blockers, unknowns, experiments, effort, verdict

**What works today:** Vanilla Conquer RA on macOS (universal2 `.app` from CI, campaigns, skirmish, VQA, audio, LAN); ra-port RA on iPhone/iPad as a debug sideload with touch; a complete, documented iOS packaging and touch pattern from the Generals port; clean Linux builds of both RA candidates reproduced in this study.

**What is missing for ship-quality:** an iOS platform layer in Vanilla Conquer (does not exist, but is a well-scoped few hundred lines plus packaging); touch UX at product polish level; a native asset-import flow; UI scale answers for iPhone; signing/notarization/review pipeline; a name and identity that satisfies EA's section 7 terms; save-stability policy across updates.

**Biggest blockers, ranked:** 1) asset acquisition UX under legal constraints (product risk, solvable); 2) GPLv3-on-App-Store acceptance being revocable at EA's word (external risk, not fully mitigable); 3) fixed 640x400 UI on iPhone (design risk, scoped down in v1); 4) bus factor of upstream (mitigated by fork-and-maintain).

**Key unknowns and the cheap experiments that resolve them:**

| Unknown | Experiment | Cost |
|---|---|---|
| Does Vanilla Conquer actually build and run on iOS? | CMake `-DCMAKE_SYSTEM_NAME=iOS` + SDL2 iOS + stub audio backend, run in Simulator | 1-2 days, agent-heavy |
| Is 640x400 upscaled acceptable on iPad, and playable at all on iPhone? | TestFlight-free sideload of the above on real devices, structured playtest | 1 day after the build exists |
| Will Apple review accept a bring-your-own-assets RTS engine app? | Submit a TestFlight external beta; reviewer response is signal at low blast radius | days of calendar time, hours of work |
| Will EA object? | Email EA community team describing the project and asking for written comfort or a section 7 permission | 1 hour to send; weeks to hear |
| Exact 2008 freeware grant text | Retrieve archived EA announcement from an unrestricted network; ask CnCNet maintainers | 1 hour |
| Vanilla Conquer open-issue landscape (RA bugs, hi-res discussion) | Run the GitHub API queries from an unrestricted machine | 15 minutes |

**Effort estimate** (one senior engineer directing autonomous agents, per the PRD's phase plan): macOS ship-quality: 2-4 weeks. iPadOS ship-quality: +6-10 weeks including device tuning. iPhone support: +2-4 weeks on top of iPad. LAN-on-macOS hardening, App Store process, and polish overlap within a 3-6 month total to a public 1.0.

**Verdict: Conditional Go.** The engineering is low-risk and largely proven by three independent codebases; this study's own builds confirm the foundation compiles clean. The conditions: (1) accept that App Store presence is revocable and ship with fallback channels from day one; (2) never bundle assets and invest properly in the import flow; (3) run the four cheap experiments above before committing to the full phase plan, since any one of them failing changes scope, and none of them costs more than days.
