# Save compatibility contract

RAtouch keeps Red Alert's original save format. The header contains an ABI fingerprint derived from the sizes of the game-state classes that are serialized as raw objects. A layout change can therefore make an older campaign save unreadable even when the source change looks unrelated to saving.

## Apple 1.x canary

The current 64-bit Apple values are:

- base engine ABI: `0x01007E46`;
- Aftermath-enabled header written by RAtouch: `0x01007E47`.

`redalert/savegame_version.h` owns the calculation, the accepted-version rule, and compile-time assertions for both values. The macOS test target exercises the same function, and the iPad Simulator build evaluates the assertions while compiling the actual app. A retained live Simulator autosave also reports `0x01007E47` in its local header. No save file is committed or used by CI.

## Intentional changes

An Apple build must not update the canary merely to make CI pass. If a game-state layout genuinely must change:

1. determine whether the field can be added without altering a serialized class;
2. if not, implement and test an old-to-new migration before changing the canary;
3. add a release note that names the oldest compatible build and the migration behavior;
4. load the campaign save corpus on macOS and iPadOS before release;
5. update both expected values only after that evidence exists.

The target for RAtouch 1.x is no save break.
