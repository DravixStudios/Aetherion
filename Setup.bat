@echo off

:: Build Aether if doesn't exist
call "%~dp0Scripts\Batch\BuildAether.bat"
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%