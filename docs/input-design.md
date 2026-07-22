# RAtouch input and gameplay refinement contract

This is the working contract for making the original simulation comfortable on iPad without changing its rules. The order is deliberate: preserve gameplay semantics, restore missing input, then tune from repeated play sessions.

## Gesture grammar

| Gesture | Engine action | Player intent | Current proof |
| --- | --- | --- | --- |
| Tap | Cursor move, left down, left up | Select, order, activate original UI | Unit selection, movement, MCV deployment, sidebar production, and completed-building placement verified in Simulator; invalid footprints retain placement mode as in original Red Alert |
| One-finger drag after 6, 8, or 12 pt | Left down at the original point, move, left up | Classic drag-box selection | A native XCTest finger drag over the live Allied mission restored four unit health bars after deselection; deterministic motion, reconfiguration, and fast-release tests cover the edges, while physical-device feel remains a gate |
| Long press for 450, 600, or 750 ms | Right down, right up | Deselect / original right-click | A native 800 ms Simulator finger hold removed all selected-unit health bars and exposed the exact terrain tooltip under the hold; optional light haptic wiring still requires physical-device feel proof |
| Two-finger drag after an 8 pt intent dead zone | Presentation-mapped finger travel through the engine's analog-scroll path, optionally inverted | Directly pan without touch edge-scroll or a fixed-speed jump | Live persisted direction setting plus deterministic dead-zone, proportional-distance, consume-once, and eight-direction compound-motion tests; neither Simulator's standard host UI nor public XCTest gesture APIs expose two-finger translation, so physical-device direction/velocity tuning remains |
| Pinch | Presentation zoom step | Increase target and text size without changing simulation | A native XCTest two-touch pinch visibly enlarged a live mission from fit through the real Simulator HID path; paired-finger sampling prevents half of a pan from becoming a zoom step, and deterministic tests cover step boundaries |
| Double two-finger tap | Reset presentation zoom | Recover the fitted whole-field view | A native XCTest gesture with two taps and two touches visibly returned the enlarged live mission to fit; deterministic timing, movement, pinch-exclusion, and timeout tests cover the recognition boundary |
| System cancellation | Release active drag or pan, never commit tap | Avoid ghost orders after interruptions | Deterministic cancellation tests |

Adding a second finger cancels a pending tap. If a selection drag is active, it is released before two-finger pan begins. SDL delivers the two moving fingers as separate events, so the recognizer samples pan and pinch only after both fingers have advanced; this prevents the first half of a translation from looking like a pinch and the first half of a symmetric pinch from nudging the map. The first eight points of paired travel establish intent without moving the camera. After that boundary, the paired center delta is converted through the same letterboxed presentation geometry as the game, accumulated within the current input poll, and consumed exactly once as map distance. The first lifted contact stops the camera immediately, while the surviving original contact is drained until release instead of becoming an accidental selection or allowing an intervening finger to start a new tap. Double-two-finger reset is recognized only after both contacts lift and final coalesced movement is included in its tap threshold. This makes short drags precise, longer drags faster, and release safe while leaving controller speed and pointer edge scrolling unchanged. Direct touch owns the virtual cursor and disables Red Alert's legacy mouse-at-edge camera path; two-finger pan remains active through this dedicated analog-scroll path. A genuine mouse or trackpad event returns cursor ownership to the pointer and restores edge scrolling. SDL touch-to-mouse synthesis is disabled and `SDL_TOUCH_MOUSEID` events are discarded, so this is a last-real-input-source decision rather than a timer or movement heuristic.

On macOS, absolute clicks and relative raw motion are both converted through the same presentation geometry used to draw the game. Window points are translated through Retina renderer scale, letterbox offsets, and presentation zoom before reaching the 640×400 simulation surface. Relative deltas are converted into game pixels before sensitivity is applied, preventing the accelerated, offset software cursor that results from treating display points as native game pixels. The native Settings panel exposes the engine's pointer speed from 25–200% and persists it immediately. The same native panel owns sharp/smooth filtering, preserve/fill aspect handling, and independent music and sound levels; display and audio requests cross to the engine thread before mutating the live renderer or original volume options.

On iPadOS, an attached hardware keyboard retains the original in-game shortcuts. Simulator mission proof covers `N` unit cycling and `E` select-view through the real SDL keyboard path. An indirect pointer uses the same presentation mapping as touch: live pointer clicks landed on the matching unit tooltip, right-click reached the intended terrain point, and captured pointer motion retained the original edge-scroll path. The Controls sheet advertises both paths without displacing the touch-first command deck. Physical-device testing still owns trackpad feel, acceleration, gestures, and ergonomics.

A fast or coalesced gesture that crosses the drag threshold only at finger-up is promoted to the same anchored drag sequence instead of being misread as a tap. This is deterministic-tested; the Simulator's host-mouse drag remains unsuitable as a sign-off for real finger feel.

## Keyboard coverage

The original sidebar already exposes Options, Sidebar, Repair, Sell, Map, build queues, and queue scrolling. RAtouch does not duplicate them.

| Original action | macOS / hardware keyboard | iPad touch | Decision |
| --- | --- | --- | --- |
| Select, order, build | Pointer + original keys | Direct tap | Complete |
| Drag-select | Pointer drag | One-finger drag | Implemented; tune on device |
| Deselect / right-click | Right-click | Long press | Implemented with native haptic wiring; physical feel remains a device gate |
| Scroll map | Edge / arrows / trackpad | Two-finger drag with an 8 pt intent dead zone and distance-proportional travel | Implemented and deterministic-tested; tune direction and velocity on device |
| Zoom presentation | App control | Pinch; double two-finger tap resets to fit | Implemented and deterministic-tested |
| Stop, Guard, Scatter | S, G, X | Command palette | Implemented and Simulator-verified; Guard interrupts movement and enters Red Alert's area-defense mission |
| Next unit, Base, Select View | N, H, E | Command palette | Implemented and Simulator-verified |
| Repair, Sell, Map | T, Y, U or sidebar | Original sidebar | Already touchable |
| Force move / force attack | Option / Control | Move+ / Attack+ for the next tactical action | Original force-move and force-attack gameplay paths verified in Simulator |
| Add to selection / queued move | Shift / Q | Add+ / Queue+ for the next tactical action | Additive selection and ordered two-waypoint movement verified in Simulator |
| Control groups 1–0 | Number recalls; Control-number assigns | Groups sheet with Recall / Assign selected modes | Implemented through original key semantics; formation assignment, context switch, recall, background autosave, cold reload, and post-load recall verified in a live Allied mission |
| Formation, bookmarks, alliance, resign | Original keys / menus | Original menus or hardware keyboard | No overlay until play evidence shows a blocking need |

## Overlay placement

The collapsed 92×44-point `COMMANDS` tab sits at the chosen safe-area edge, centered vertically. It opens inward so it never pushes controls under the system edge. On the right, it follows the map/sidebar boundary while Red Alert's original sidebar is open, keeping both the tab and expanded palette clear of build queues, then returns to the safe-area edge when the sidebar closes. The command deck uses seven rows: Stop/Guard, Scatter/Next, Base/View, Attack+/Move+, Add+/Queue+, Groups/Controls, then a full-width handedness control. Its preferred 320×392-point frame remains bounded by the safe area, gives every action a wide target, and scales labels with Dynamic Type up to a gameplay-safe maximum. The side choice persists across launches. `Groups` opens a native sheet with Recall and Assign selected modes plus slots 1–0. Recall sends the original number key; assignment sends the original Control-number sequence, so Red Alert remains the sole owner of group membership and selection behavior. The mode-specific VoiceOver hints explain whether a slot replaces or recalls the current selection. The essential selector, grid, and Done control remain visible at maximum Dynamic Type; supporting explanation can scroll below them. `Controls` opens a separate Dynamic Type-compatible sheet with the full gesture grammar, including the otherwise undiscoverable two-finger zoom reset, plus live persisted presets for hold duration, drag threshold, inverted pan, long-press haptics, display filtering, aspect handling, and music/SFX volume. Its header keeps Done reachable while the body scrolls within the safe area instead of compressing or clipping controls at accessibility text sizes. A bottom `About & license` route presents the complete bundled terms and source link in another scroll-safe native sheet; settings and legal information therefore add no new persistent gameplay control. Preference changes cross onto the engine event thread through private SDL events instead of mutating active engine or gesture state from UIKit. Immediate commands collapse the palette after firing. A modifier collapses it into an amber, relabeled armed tab until the next tactical action consumes it; reopening and tapping the same modifier cancels it. The rest of the transparent native window passes touches through to SDL.

Controls also links to a non-gameplay `Game data` sheet. It reports the installed provenance receipt, offers save export through Files, and stages archive replacement for the next cold launch rather than changing data under an active scenario. Controls, Game Data, and Groups pin uniquely labeled title/Done headers above their scrolling bodies, so maximum Dynamic Type cannot move the recovery action offscreen. These routes remain nested behind Controls and therefore add no extra map overlay.

Movie input remains on the movie side of the transition. The shared touch/pointer guard consumes a skip gesture through release and also blocks input observed during the first non-movie poll. Main-menu initialization force-presents its first completed frame and clears transition input before the menu becomes interactive; a quiet poll or completed release then clears the boundary so the next deliberate menu gesture is accepted immediately.

These are not timed fake key taps. The overlay presses the original SDL modifier key, the engine evaluates the unmodified Red Alert action path, and the key is released at the corresponding engine mouse release. A right-click, overlay hide, replacement modifier, explicit second tap, or transition into the non-tactical Controls or Groups sheets also releases it, preventing a stale tactical action after a UI detour.

This placement is intentionally low-risk: it avoids the original top Options/money/sidebar chrome, does not permanently cover the map, meets the 44-point target baseline, and gives every control a VoiceOver label and hint. A later Settings surface may add a hide toggle, but the handedness requirement is complete without creating a second persistent overlay.

## Playtest loop

Each refinement pass must exercise a real campaign or skirmish state, not only menus:

1. Select one unit, select a group, issue move and attack orders, and deselect.
2. Drag-select near the dead-zone boundary and immediately transition into two-finger pan.
3. Pan in all eight directions, pinch through every zoom step, then recover the sidebar and map.
4. Use every immediate command and one-shot modifier with a valid selection; verify the palette never steals nearby map taps and every armed state releases after one action.
5. Save, load, background, foreground, and repeat the input pass. Lifecycle regression runs 100 deterministic cycles; Simulator batches exercise real Home/foreground transitions over a live mission.
6. Log the symptom and change only one threshold, gesture, or overlay behavior per iteration.

Simulator is the continuous smoke environment. Physical iPad testing owns gesture feel, Pencil, trackpad, haptics, thermals, audio interruption, and accessibility sign-off.
