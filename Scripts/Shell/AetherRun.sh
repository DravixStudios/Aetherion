#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

TARGET="${1:-Aetherion}"
if [ -n "${2:-}" ]; then
    PRESET="$2"
else
    case "$(uname -s)" in
        Darwin) PRESET="macos-debug-spdlog" ;;
        Linux)  PRESET="linux-debug-spdlog" ;;
        *)      PRESET="x64-debug-spdlog" ;;
    esac
fi

./Scripts/Shell/AetherBuild.sh "$TARGET" "$PRESET"

case "$PRESET" in
    *release*|*Release*) CONFIG="Release" ;;
    *) CONFIG="Debug" ;;
esac

cd "Bin/$TARGET/$CONFIG"
exec "./$TARGET"
