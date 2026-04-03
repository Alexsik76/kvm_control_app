@echo off
setlocal enabledelayedexpansion

:: Attempt to find the latest Visual Studio installation
set "VS_WHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VS_WHERE!" (
    echo [ERROR] Visual Studio Installer not found.
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"!VS_WHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo [ERROR] Visual Studio with C++ Build Tools not found.
    exit /b 1
)

set "VSCMD_PATH=!VS_PATH!\Common7\Tools\VsDevCmd.bat"

if not exist "!VSCMD_PATH!" (
    echo [ERROR] VsDevCmd.bat not found at !VSCMD_PATH!
    exit /b 1
)

:: Initialize MSVC Environment for x64
echo [INFO] Initializing MSVC environment...
call "!VSCMD_PATH!" -arch=x64 > nul

:: Check if CMake is available
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found in PATH.
    exit /b 1
)

:: Run Configuration
echo [INFO] Running CMake Configuration...
cmake -B build -G "Ninja"

:: Run Build
echo [INFO] Running CMake Build...
cmake --build build

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build completed.
    echo [INFO] Running executable...
    .\build\KVMControlApp.exe
) else (
    echo [ERROR] Build failed.
)

endlocal
