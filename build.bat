@echo off
setlocal
echo ==============================================
echo Building Deskmon (window_logger)
echo ==============================================

:: Setup paths for Qt, CMake, and Ninja
set "QT_ROOT=C:\Qt\6.9.2\mingw_64"
set "CMAKE_BIN=C:\Qt\Tools\CMake_64\bin"
set "NINJA_BIN=C:\Qt\Tools\Ninja"
set "MINGW_BIN=C:\Qt\Tools\mingw1310_64\bin"

:: Add tools to PATH
set "PATH=%CMAKE_BIN%;%NINJA_BIN%;%MINGW_BIN%;%QT_ROOT%\bin;%PATH%"

if not exist "build" (
    mkdir build
)

cd build
echo [1/3] Configuring CMake...
cmake -G Ninja -DCMAKE_PREFIX_PATH="%QT_ROOT%" ..
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    cd ..
    pause
    exit /b %errorlevel%
)

echo.
echo [2/3] Building Project...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    cd ..
    pause
    exit /b %errorlevel%
)

echo.
echo [3/3] Build Successful! Running Application...
echo ==============================================
if exist "Deskmon.exe" (
    Deskmon.exe
) else if exist "Release\Deskmon.exe" (
    cd Release
    Deskmon.exe
    cd ..
) else (
    echo [ERROR] Could not find executable Deskmon.exe
)
cd ..
pause
endlocal
