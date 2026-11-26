#!/usr/bin/env bash
set -euo pipefail

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

cd "$(dirname "$0")/../.."

dotnet build "Tools/Aether/Aether.csproj" -c Release -o "Bin/Aether" --nologo -v minimal
./Bin/Aether/Aether build "$TARGET" "$PRESET"
