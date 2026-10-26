@echo off
setlocal

echo ======================================================
echo  Building Aether Build Tool
echo ======================================================

where dotnet >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo "Couldn't find dotnet".
    echo "Install dotnet from https://dotnet.microsoft.com/download"
    exit /b 1
)

set ROOT_DIR=%~dp0\..\..\
set AETHER_CSPROJ=%ROOT_DIR%Tools\Aether\Aether.csproj
set OUTPUT_DIR=%ROOT_DIR%bin\Aether

:: Compilar hacia bin\Aether
dotnet build "%AETHER_CSPROJ%" -c Release -o "%OUTPUT_DIR%" --nologo -v minimal

if %ERRORLEVEL% neq 0 (
    echo.
    echo "[ERROR] Failed building Aether"
    exit /b %ERRORLEVEL%
)

echo.
echo "[SUCCESS] Aether built at:"
echo   %OUTPUT_DIR%\Aether.exe
echo ======================================================