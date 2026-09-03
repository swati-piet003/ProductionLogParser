param([ValidateSet("Debug", "Release")][string]$Configuration = "Release")
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
cmake -S $Root -B "$Root/build" -DCMAKE_BUILD_TYPE=$Configuration
cmake --build "$Root/build" --config $Configuration
ctest --test-dir "$Root/build" -C $Configuration --output-on-failure

