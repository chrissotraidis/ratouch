#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/ratouch-import-tests.XXXXXX")"
trap 'rm -rf "$test_dir"' EXIT
xcrun clang++ -std=c++17 -fobjc-arc -Wno-deprecated-declarations -I"$root" \
  "$root/tests/importer.mm" "$root/apple/shared/importer.mm" \
  "$root/apple/shared/iso9660.cpp" "$root/apple/shared/asset_manifest.cpp" \
  "$root/apple/shared/mix_validator.cpp" -framework Foundation -o "$test_dir/importer-tests"
"$test_dir/importer-tests"
