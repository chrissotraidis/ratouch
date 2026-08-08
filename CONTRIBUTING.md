# Contributing to RAtouch

Focused issues and pull requests are welcome for reproducible gameplay defects,
Apple-platform build fixes, touch or pointer behavior, accessibility, and tests.

## Before opening a change

1. Keep engine changes narrow and prefer the existing Apple/platform boundary.
2. Do not change Red Alert simulation rules to make touch easier; route touch
   through the original input and command paths.
3. Do not commit MIX/ISO files, saves, imported artwork, or unreviewed gameplay
   captures. Never ask users to upload game data to an issue.
4. State the tested platform precisely. Simulator evidence is not
   physical-device acceptance.
5. Preserve existing saves and imported data during replacement or install work.

Run the maintained local gates:

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
python3 scripts/verify-public-repo.py
git diff --check
```

For gameplay changes, follow [the compatibility loop](docs/gameplay-compatibility.md)
and update [build status](docs/build-status.md) only with evidence actually
observed. Open gates belong in [remaining work](docs/remaining-work.md).

## Bug reports

Include the commit, platform and OS version, data layout without attaching the
data itself, the smallest reproduction sequence, expected and observed results,
and whether the result came from automated tests, Simulator, Mac runtime, or a
physical device. For audio defects, include the exact action and timestamp.

