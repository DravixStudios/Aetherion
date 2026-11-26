#!/usr/bin/env bash
set -euo pipefail

echo "======================================================"
echo " Building Aether Build Tool"
echo "======================================================"

if ! command -v dotnet >/dev/null 2>&1; then
    echo "Couldn't find dotnet."
    echo "Install .NET SDK from https://dotnet.microsoft.com/download"
    exit 1
fi

cd "$(dirname "$0")/../.."

dotnet build "Tools/Aether/Aether.csproj" -c Release -o "Bin/Aether" --nologo -v minimal

echo ""
echo "[SUCCESS] Aether built at:"
echo "  $(pwd)/Bin/Aether/Aether"
echo "======================================================"
