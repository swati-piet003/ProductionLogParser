#!/usr/bin/env sh
set -eu
configuration="${1:-Release}"
root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cmake -S "$root" -B "$root/build" -DCMAKE_BUILD_TYPE="$configuration"
cmake --build "$root/build" --config "$configuration"
ctest --test-dir "$root/build" -C "$configuration" --output-on-failure

