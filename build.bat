@echo off
echo [INFO] build.bat is the canonical native build entry point.
echo MSBuild (dotnet build from kvm_desktop) invokes this script automatically.
echo You can also run it directly for standalone native builds or troubleshooting.
echo.
setlocal enabledelayedexpansion

set "BUILD_CONFIG=%~1"
if "%BUILD_CONFIG%"=="" set "BUILD_CONFIG=Debug"
echo [INFO] Build configuration: %BUILD_CONFIG%

:: --- Configuration ---
set "CS_BIN_DIR=..\kvm_desktop\src\KvmDesktop\bin\%BUILD_CONFIG%\net10.0"

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

echo [INFO] Running CMake Configuration...
cmake --preset default
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Configuration failed.
    exit /b 1
)

:: Run Build
echo [INFO] Running CMake Build...
cmake --build build --config %BUILD_CONFIG%

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build completed.
    
    if exist "!CS_BIN_DIR!" (
        echo [INFO] Copying DLLs to C# project bin directory...
        copy /Y "build\%BUILD_CONFIG%\KVMVideoCodec.dll" "!CS_BIN_DIR!\"
        
        :: Also copy FFmpeg DLLs if they are in the build/%BUILD_CONFIG% folder
        if exist "build\%BUILD_CONFIG%\avcodec-*.dll" copy /Y "build\%BUILD_CONFIG%\av*.dll" "!CS_BIN_DIR!\"
        if exist "build\%BUILD_CONFIG%\swscale-*.dll" copy /Y "build\%BUILD_CONFIG%\swscale-*.dll" "!CS_BIN_DIR!\"
        if exist "build\%BUILD_CONFIG%\avutil-*.dll" copy /Y "build\%BUILD_CONFIG%\avutil-*.dll" "!CS_BIN_DIR!\"
        
        :: Copy other potential dependencies from vcpkg
        if exist "build\%BUILD_CONFIG%\datachannel.dll" copy /Y "build\%BUILD_CONFIG%\datachannel.dll" "!CS_BIN_DIR!\"
        if exist "build\%BUILD_CONFIG%\libcrypto-*.dll" copy /Y "build\%BUILD_CONFIG%\libcrypto-*.dll" "!CS_BIN_DIR!\"
        if exist "build\%BUILD_CONFIG%\libssl-*.dll" copy /Y "build\%BUILD_CONFIG%\libssl-*.dll" "!CS_BIN_DIR!\"
        if exist "build\%BUILD_CONFIG%\juice.dll" copy /Y "build\%BUILD_CONFIG%\juice.dll" "!CS_BIN_DIR!\"
        if exist "build\%BUILD_CONFIG%\srtp2.dll" copy /Y "build\%BUILD_CONFIG%\srtp2.dll" "!CS_BIN_DIR!\"
        if exist "build\%BUILD_CONFIG%\swresample-*.dll" copy /Y "build\%BUILD_CONFIG%\swresample-*.dll" "!CS_BIN_DIR!\"
        
        echo [INFO] Deployment to C# bin folder finished.
    ) else (
        echo [WARNING] C# bin directory not found at !CS_BIN_DIR!. Skipping copy.
    )
) else (
    echo [ERROR] Build failed.
)

endlocal
