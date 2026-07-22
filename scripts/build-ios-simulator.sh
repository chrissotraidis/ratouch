#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${RATOUCH_IOS_BUILD_DIR:-$repo_root/build/ios-simulator}"

cmake_args=(
    -S "$repo_root"
    -B "$build_dir"
    -G Xcode
    -DCMAKE_SYSTEM_NAME=iOS
    -DCMAKE_OSX_SYSROOT=iphonesimulator
    -DCMAKE_OSX_ARCHITECTURES=arm64
    -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
    -DBUILD_VANILLATD=OFF
    -DBUILD_VANILLARA=ON
    -DBUILD_TESTS=OFF
    -DBUILD_TOOLS=OFF
    -DNETWORKING=OFF
    -DSDL_AUDIO=ON
    -DOPENAL=OFF
)

if [[ -n "${RATOUCH_SDL2_SOURCE:-}" ]]; then
    cmake_args+=("-DFETCHCONTENT_SOURCE_DIR_SDL2=$RATOUCH_SDL2_SOURCE")
fi

cmake "${cmake_args[@]}"
cmake --build "$build_dir" --config Debug --target VanillaRA -- CODE_SIGNING_ALLOWED=NO

find "$build_dir" -type d -name vanillara.app -print -quit
