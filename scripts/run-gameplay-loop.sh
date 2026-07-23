#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${RATOUCH_MACOS_BUILD_DIR:-$repo_root/build/ratouch-macos}"
mode="${1:-quick}"

usage() {
    echo "Usage: $0 [quick|full]"
    echo "  quick  Build macOS, run focused gameplay-boundary tests, and verify public files."
    echo "  full   Run quick plus every macOS test and an arm64 iPad Simulator build."
}

if [[ "$mode" != "quick" && "$mode" != "full" ]]; then
    usage
    exit 2
fi

if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
    cmake \
        -S "$repo_root" \
        -B "$build_dir" \
        -DBUILD_VANILLATD=OFF \
        -DBUILD_VANILLARA=ON \
        -DBUILD_TESTS=ON \
        -DBUILD_TOOLS=OFF \
        -DNETWORKING=OFF \
        -DSDL_AUDIO=ON \
        -DOPENAL=OFF
fi

echo "Gameplay loop: build current macOS app and tests"
cmake --build "$build_dir" --parallel "${RATOUCH_BUILD_JOBS:-8}"

echo "Gameplay loop: run focused asset-free regression gates"
ctest \
    --test-dir "$build_dir" \
    --output-on-failure \
    -L ratouch_gameplay_loop

echo "Gameplay loop: reject commercial assets and broken public documentation"
python3 "$repo_root/scripts/verify-public-repo.py"

if [[ "$mode" == "full" ]]; then
    echo "Gameplay loop: run the complete macOS test suite"
    ctest --test-dir "$build_dir" --output-on-failure

    echo "Gameplay loop: build the arm64 iPad Simulator app"
    "$repo_root/scripts/build-ios-simulator.sh"
fi

echo
echo "Automated gameplay loop passed ($mode)."
echo "Next: replay one live behavior; fix only its narrow cause if it fails."
echo "Do not add a framework or duplicate Red Alert's rules."
echo "The loop does not launch RAtouch, so it cannot leave the game running."
