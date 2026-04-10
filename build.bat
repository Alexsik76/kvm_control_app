@echo off
setlocal enabledelayedexpansion

:: --- Configuration ---
set "CS_BIN_DIR=..\kvm_desktop\src\KvmDesktop\bin\Debug\net10.0"

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
cmake --build build --config Debug

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build completed.
    
    if exist "!CS_BIN_DIR!" (
        echo [INFO] Copying DLLs to C# project bin directory...
        copy /Y "build\Debug\KVMVideoCodec.dll" "!CS_BIN_DIR!\"
        
        :: Also copy FFmpeg DLLs if they are in the build/Debug folder
        if exist "build\Debug\avcodec-*.dll" copy /Y "build\Debug\av*.dll" "!CS_BIN_DIR!\"
        if exist "build\Debug\swscale-*.dll" copy /Y "build\Debug\swscale-*.dll" "!CS_BIN_DIR!\"
        if exist "build\Debug\avutil-*.dll" copy /Y "build\Debug\avutil-*.dll" "!CS_BIN_DIR!\"
        
        :: Copy other potential dependencies from vcpkg
        if exist "build\Debug\datachannel.dll" copy /Y "build\Debug\datachannel.dll" "!CS_BIN_DIR!\"
        if exist "build\Debug\libcrypto-*.dll" copy /Y "build\Debug\libcrypto-*.dll" "!CS_BIN_DIR!\"
        if exist "build\Debug\libssl-*.dll" copy /Y "build\Debug\libssl-*.dll" "!CS_BIN_DIR!\"
        if exist "build\Debug\juice.dll" copy /Y "build\Debug\juice.dll" "!CS_BIN_DIR!\"
        if exist "build\Debug\srtp2.dll" copy /Y "build\Debug\srtp2.dll" "!CS_BIN_DIR!\"
        if exist "build\Debug\swresample-*.dll" copy /Y "build\Debug\swresample-*.dll" "!CS_BIN_DIR!\"
        
        echo [INFO] Deployment to C# bin folder finished.
    ) else (
        echo [WARNING] C# bin directory not found at !CS_BIN_DIR!. Skipping copy.
    )
) else (
    echo [ERROR] Build failed.
)

endlocal
