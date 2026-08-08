#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${RATOUCH_IOS_DEVICE_BUILD_DIR:-$repo_root/build/ios-device-release}"
output_dir="${RATOUCH_RELEASE_DIR:-$repo_root/build/release}"
version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$repo_root/apple/ios/Info.plist.in")"
app="$build_dir/Release/vanillara.app"
artifact="$output_dir/RAtouch-v${version}-unsigned.ipa"
checksum="$output_dir/RAtouch-v${version}-SHA256.txt"
prefix_map="-ffile-prefix-map=$repo_root=."

cmake \
    -S "$repo_root" \
    -B "$build_dir" \
    -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT=iphoneos \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
    -DBUILD_VANILLATD=OFF \
    -DBUILD_VANILLARA=ON \
    -DBUILD_TESTS=OFF \
    -DBUILD_TOOLS=OFF \
    -DNETWORKING=OFF \
    -DSDL_AUDIO=ON \
    -DOPENAL=OFF \
    "-DCMAKE_C_FLAGS=$prefix_map" \
    "-DCMAKE_CXX_FLAGS=$prefix_map" \
    "-DCMAKE_OBJCXX_FLAGS=$prefix_map"

cmake --build "$build_dir" --config Release --target VanillaRA -- -quiet CODE_SIGNING_ALLOWED=NO

executable="$app/vanillara"
test -f "$executable"
test -f "$app/License.txt"
test -f "$app/PrivacyInfo.xcprivacy"
test ! -e "$app/embedded.mobileprovision"
test ! -e "$app/_CodeSignature"
test "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$app/Info.plist")" = "com.chrissotraidis.ratouch"
test "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$app/Info.plist")" = "$version"
file "$executable" | grep -q 'Mach-O 64-bit executable arm64'
xcrun vtool -show-build "$executable" | grep -q 'platform IOS'

stage="$(mktemp -d "${TMPDIR:-/tmp}/ratouch-ipa.XXXXXX")"
trap 'rm -rf "$stage"' EXIT
mkdir -p "$stage/Payload" "$output_dir"
cp -R "$app" "$stage/Payload/RAtouch.app"
/usr/bin/strip -Sx "$stage/Payload/RAtouch.app/vanillara"

if grep -R -a -F -q "$repo_root" "$stage/Payload/RAtouch.app"; then
    echo "Local checkout path escaped into the staged app." >&2
    exit 1
fi

rm -f "$artifact"
(
    cd "$stage"
    /usr/bin/zip -q -r -X "$artifact" Payload
)

/usr/bin/unzip -tq "$artifact"
contents="$stage/ipa-contents.txt"
/usr/bin/unzip -Z1 "$artifact" > "$contents"
grep -q '^Payload/RAtouch.app/PrivacyInfo.xcprivacy$' "$contents"
if grep -Eqi '\.(mix|iso|vqa|vqp|aud|shp|wsa|sav|save)$|/savegame\.' "$contents"; then
    echo "Commercial game data or a save escaped into the IPA." >&2
    exit 1
fi

(
    cd "$output_dir"
    shasum -a 256 "$(basename "$artifact")" > "$(basename "$checksum")"
)
cat "$checksum"
echo "$artifact"
