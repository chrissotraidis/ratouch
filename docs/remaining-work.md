# Remaining work

This is the source of truth for unfinished RAtouch verification and release
work. It lists only open gates. Completed implementation and evidence belong in
[build status](build-status.md); repeatable gameplay scenarios belong in the
[gameplay compatibility loop](gameplay-compatibility.md).

Last reviewed: July 23, 2026.

## Current baseline

- Apple-silicon macOS and arm64 iPad Simulator builds succeed.
- All 24 macOS tests pass; the focused gameplay loop contains 12 tests.
- Steam 2229840 data imports and runs through the shared Apple importer.
- Core build placement and unit-order scenarios pass on macOS and/or iPad
  Simulator as recorded in the gameplay compatibility matrix.
- No signed DMG, physical-iPad build, IPA, or TestFlight release is published.
- Simulator evidence is not physical-device certification.

## Work loop

Work on one row at a time:

1. Choose the first open row that can run in the available environment.
2. Reproduce its exact starting condition once.
3. If it fails, fix only the narrow cause.
4. Run `./scripts/run-gameplay-loop.sh quick` and replay that same case.
5. Stop RAtouch, confirm no related process remains, and record the evidence.
6. Mark the row complete only when every exit condition is observed.

Do not add a framework or duplicate Red Alert's rules to close one case. Use
the original engine state as the gameplay authority and keep private game data,
saves, and commercial screenshots out of Git.

## Next: locally executable gameplay

These are the next cases that can advance without release credentials or new
hardware. Run them in order and stop at the first failure.

| Order | Gate | Exit condition | Status |
| --- | --- | --- | --- |
| 1 | `AUDIO-01` real-speaker session | Complete at least 30 continuous minutes covering music, EVA speech, construction, dense combat, Options, hide/restore, and save/load. Record the triggering action and timestamp for any clipping, repetition, gap, or random fragment. | **NEXT**; automated underrun and 30-second signal checks pass |
| 2 | `MISSION-01` game-flow outcomes | Win and lose one skirmish, then complete one representative Allied and Soviet campaign mission. Verify briefing, mission triggers, victory/defeat, score screen, and return path. | Open; abort/loss score path passes |

After these two cases pass, extend the same loop to both full campaigns,
expansion missions, and a two-hour gameplay/audio soak. Those longer sessions
are release gates, not reasons to redesign the short loop.

## Data compatibility

| Gate | Exit condition | Environment |
| --- | --- | --- |
| Remastered-layout import | Legally supplied CNCDATA imports, provenance is correct, and a campaign or skirmish reaches live play with expected movies/audio limitations documented. | Local Mac, then physical iPad |
| Bare-MIX import | A supported loose archive set imports, relaunches, and reaches live play without misclassification or asset corruption. | Local Mac, then physical iPad |
| Allied and Soviet ISO import | Each legally supplied disc imports independently; expected side-specific content is identified; the combined installed data reaches live play. | Local Mac, then physical iPad |
| Adversarial import pass | Truncated, renamed, mismatched, incomplete, and insufficient-space fixtures fail safely without replacing a working install or losing saves/settings. | Automated fixtures plus one Apple-app UI pass |

Steam 2229840 is the only real-file layout currently signed off. Fixture tests
do not replace a legally supplied real-file launch.

## Physical Apple hardware

These gates cannot be closed by Simulator results.

| Gate | Exit condition | Required environment |
| --- | --- | --- |
| Signed iPad build | Install and launch a development-signed build on a physical iPad without copying game data into the app bundle. | Apple signing identity and physical iPad |
| Touch feel | Tune and sign off tap accuracy, 6/8/12-point drag thresholds, two-finger pan direction and speed, pinch, long-press timing, accidental-command rate, and command-deck reachability during real play. | At least one A-series and one M-series iPad |
| Alternate input | Verify Apple Pencil, trackpad/mouse, hardware keyboard, edge scrolling, right-click, hotkeys, and transitions between touch and pointer input. | Physical iPad plus accessories |
| Accessibility | Exercise VoiceOver, Switch/keyboard navigation for native UI, maximum supported Dynamic Type, left-handed placement, contrast, and 44-point targets. Document that the game buffer itself is outside VoiceOver scope. | Physical iPad |
| Lifecycle and interruption | Repeat the 100-cycle mission soak with randomized background dwell, audio interruption, low-memory pressure, autosave checks, and no stale touches or modifiers. | Physical iPad |
| Performance and battery | Run a 60-minute mission at fit, 1.5×, and 2× presentation states; record frame pacing, thermal state, battery impact, and any audio degradation. | Lowest practical supported iPad |
| Intel/universal Mac | Build universal2, install and play on an Intel Mac, then repeat pointer, window/fullscreen, audio, import, save/load, and quit checks. | Intel Mac |

## Packaging and public beta

| Order | Gate | Exit condition | Dependency |
| --- | --- | --- | --- |
| 1 | Mac release candidate | Produce a universal2 app with version metadata, complete licenses, privacy posture, and no commercial data. Verify a clean first-run import. | Intel and Apple-silicon sign-off |
| 2 | Signed and notarized DMG | Developer ID sign, notarize, staple, install from the DMG on clean Macs, and publish matching source and checksums with a prerelease tag. | Apple Developer credentials |
| 3 | iPad beta archive | Produce a signed device archive with correct bundle metadata, privacy manifest, orientations, icons, and zero bundled game data. | Physical-iPad gates |
| 4 | TestFlight | Upload an internal build, complete review notes for bring-your-own-data, run a focused external beta, and burn down reproducible feedback. | Apple Developer credentials and human submission |
| 5 | App Store decision | Attempt submission only after beta stability and legal review; document any rejection and the approved fallback distribution path. | Human release decision |
| 6 | Versioned release | Tag the exact source, publish corresponding-source and license materials, checksums, installation instructions, known limitations, and supported-data matrix. | Completed platform release candidate |

## External and human-owned gates

These need a person or external service and must not be reported as engineering
failures:

- restore GitHub Actions execution after the account billing/spending-limit
  restriction, then require green hosted checks on the release commit;
- review the project name, icon, store art, screenshots, trademark posture, and
  bring-your-own-data wording;
- finalize the GPL corresponding-source/source-offer mechanism for every
  distributed binary;
- decide whether and when to contact Electronic Arts before TestFlight or an
  App Store submission;
- own Apple Developer enrollment, certificates, notarization, TestFlight, and
  store submissions.

## Explicitly deferred

These are not blockers for the iPad-first and direct-download Mac release:

- iPhone support, until a physical-device 44-point and gameplay-quality
  prototype earns an explicit go decision;
- internet multiplayer and iPad LAN play;
- engine-level widescreen or scalable game UI;
- Mac App Store distribution;
- App Store approval itself, which is an external outcome rather than a code
  acceptance test.

## Updating this document

- Keep only unfinished work here.
- When a row passes, move its evidence to
  [build status](build-status.md), update the corresponding case in
  [gameplay compatibility](gameplay-compatibility.md), and remove the row.
- If a failure creates new work, add the narrowest observable gate rather than
  a speculative subsystem project.
- Keep the first locally executable row marked `NEXT`.
