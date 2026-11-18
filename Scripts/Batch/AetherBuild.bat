@echo off
setlocal

set "ROOT_DIR=%~dp0..\..\"
pushd "%ROOT_DIR%" || exit /b 1

dotnet build "%ROOT_DIR%Tools\Aether\Aether.csproj" -c Release --nologo -v minimal
if errorlevel 1 (
    popd
    exit /b 1
)

"%ROOT_DIR%bin\Aether\Aether.exe" build Aetherion x64-debug-spdlog
set "EXIT_CODE=%ERRORLEVEL%"

popd
exit /b %EXIT_CODE%
