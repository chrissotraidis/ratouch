# Gameplay compatibility loop

RAtouch does not replace Red Alert's rules. The inherited engine remains the
authority for production, placement, orders, economy, combat, AI, and mission
scripts. This loop checks that RAtouch's data, input, presentation, audio, and
lifecycle layers do not prevent those rules from being played.

The ordered open queue, hardware gates, and release work live in
[Remaining work](remaining-work.md). This file defines the gameplay cases and
their evidence.

## Smallest useful loop

1. Pick one unverified behavior.
2. Reproduce it once on the current build.
3. If it fails, fix only the narrow cause.
4. Run `quick` and replay that same behavior.
5. Stop the app, record the result, and move to the next case.

Do not add a framework, abstraction, or second rules engine just to test one
behavior. Add a deterministic regression only when it protects a real fix or a
boundary that has already failed. The goal is a game that runs and works, not a
larger testing system.

## One-command gates

Run the fast loop after an input, audio, renderer, importer, or lifecycle change:

```sh
./scripts/run-gameplay-loop.sh quick
```

Run the full loop before merging:

```sh
./scripts/run-gameplay-loop.sh full
```

`quick` builds the macOS app, runs the focused `ratouch_gameplay_loop` CTest
label, and checks the public repository. `full` additionally runs every macOS
test and builds the arm64 iPad Simulator app. Neither command launches the game,
uses private game data, or leaves an executable running.

## Fixed live fixture

Use legally acquired data from the ignored `ref/` directory or an existing
private app container. Never copy screenshots, saves, MIX files, or extracted
art into Git.

- Game: Red Alert skirmish
- Map: `A Path Beyond (Lg)`
- Side: Soviet
- Difficulty: Easy
- Bases: On
- Unit count: 0
- Starting credits: 10000
- Speed: default
- Opponents: one Easy AI when the case requires combat; otherwise the smallest
  setup the original menu permits

If the fixture cannot be reproduced exactly, record the difference before
interpreting the result. A different map, faction, starting unit, or asset set
can change placement space, prerequisites, art, and audio.

## Targeted scenario order

Stop at the first failure. Reproduce it once, collect the narrowest useful
evidence, terminate RAtouch, and fix that failure before continuing.

| ID | Scenario and checkpoints | Current evidence | Automation |
| --- | --- | --- | --- |
| `BUILD-01` | Deploy the MCV. Obstructed terrain must reject deployment; clear terrain must create a Construction Yard and expose the sidebar. | Passed on macOS and iPad Simulator. | Touch and pointer translation are covered; the engine transition remains live. |
| `BUILD-02` | Tap Power Plant once. Credits must fall, production must advance, and the cameo must reach `READY` without entering placement early. | Passed on macOS and iPad Simulator. | Save/input boundaries are covered; production timing remains live. |
| `BUILD-03` | Tap the completed cameo. A pending footprint must appear; an overlapping target must remain pending; a legal adjacent target must place the building and consume `READY`. | Passed on macOS and iPad Simulator. | Geometry is covered; placement state remains live. |
| `BUILD-04` | While building, cancel once to suspend; cancel again to abandon. Confirm the refund and cleared factory. Repeat from placement and confirm it returns to `READY` before abandoning. | Passed on iPad Simulator with native 0.8-second holds: active production froze at `HOLD` and 9850 credits before refunding to 10000; placement returned to `READY` at 9730 before abandoning and refunding to 10000. | Gesture generation is covered; factory/refund state remains a live-data case. |
| `BUILD-05` | Place Power Plant, Barracks, and Kennel, scroll both lanes, train one Attack Dog, and reconcile the exact credit change. | Passed on iPad Simulator. | Sidebar coordinates and credits remain live. |
| `ORDER-01` | Select one unit, move, stop, guard, and scatter. Confirm selection, destination/action cursor, issued mission, and visible response after each action. | Passed on iPad Simulator. A native sequence issued a long group move, interrupted it partway with Guard, and visibly spread the stopped formation with Scatter; Move and Stop were already verified. | Command dispatch is covered; engine missions remain live. |
| `ORDER-02` | Box-select multiple units, queue two moves, force-move, force-attack an enemy, and recall a saved control group. Confirm no modifier remains armed. | Passed on iPad Simulator. The selected formation traversed both queued waypoints in order; `MOVE+` moved it onto an enemy structure's location; each one-shot modifier returned to neutral. Force-attack, Stop, group assign/recall, and save/load recall had already passed. | Drag, modifier, lifecycle cancellation, and groups are covered. |
| `ECON-01` | Build a Refinery, let a Harvester unload, and reconcile credits. Trigger low power, then restore it; production speed must recover. Repair and sell one structure. | Passed on iPad Simulator. The Harvester visibly docked and raised credits from 8044 to 8744. Selling the Power Plant created zero power; its replacement was still building at 35.7 seconds and reached `READY` by 77.4 seconds, while the same item reached `READY` by 25.8 seconds after power was restored. A controlled Rifle Infantry force-fire replay damaged the Construction Yard; Repair restored its full green health bar and deducted 6 credits. | Live engine case; retained evidence stayed outside Git. |
| `SAVE-01` | Save/load once during production, once with a completed item at `READY`, and once during combat. State must resume without a stale touch modifier. | Passed on iPad Simulator. A background autosave restored a Power Plant at `READY` with credits unchanged at 9730, a neutral command tab, a valid placement preview, and successful placement. A second autosave restored an in-progress Refinery with its base and credits, continued to `READY`, placed successfully, and produced its Harvester. A retained Allied combat save restored the same zero-credit formation and continued its issued advance toward the live target with the command tab neutral. | Autosave eligibility, ABI, and lifecycle dispatch are covered. |
| `AUDIO-01` | Listen for 30 minutes across music, EVA speech, construction, combat, Options, hide/restore, and save/load. Record any clip with the triggering action and timestamp. Music must remain continuous and effects may overlap without random fragments. | Objective 30-second signal and underrun regression passed; real-speaker soak is open. | Dummy-device buffering and simultaneous samples are covered. |
| `MISSION-01` | Win and lose one skirmish, then complete one representative Allied and Soviet campaign mission. Verify briefing, triggers, victory/defeat, score screen, and return path. | Abort/loss score path passed; complete mission outcomes remain required. | Corrupt score-animation bounds are covered. |

## Evidence record

For each live case, capture only:

- commit hash and platform/build;
- exact fixture differences;
- starting and ending credits where relevant;
- the expected and observed engine state;
- whether the system pointer, software cursor, and touch target agreed;
- the first failing action and whether it reproduced;
- app exit result and a final process sweep.

Screenshots are supporting evidence, not the assertion. For production and
placement, use the factory state, pending placement object, credits, and placed
object. For orders, use selection and the resulting mission or target. For
audio, use the triggering action plus an objective capture when possible.

## Fix rule

Classify a failure before changing code:

- If pristine Vanilla Conquer behaves the same with the same data and fixture,
  it is inherited behavior or an upstream bug, not automatically a RAtouch
  regression.
- If only RAtouch fails, fix the narrow platform boundary first: imported data,
  coordinate conversion, gesture-to-original input, audio streaming, focus, or
  lifecycle.
- Do not change core simulation rules merely to make touch easier. Add visible
  controls that issue the original command.
- Add a deterministic regression for the discovered cause, rerun `quick`, then
  rerun only the failed live case. Run `full` after the scenario passes.

Always exit the game after a live pass. Confirm there is no remaining RAtouch,
VanillaRA, XCTest, `xctest`, `xcodebuild`, or debugger process before reporting
the result.
