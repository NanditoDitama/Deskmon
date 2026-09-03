@echo off
setlocal

echo ==================================================
echo Configuring environment for Deskmon build...
echo ==================================================

:: Set Qt 6.9.2 and Tools directories
if exist "C:\Qt692\6.9.2\mingw_64" (
    set "QT_DIR=C:\Qt692\6.9.2\mingw_64"
    set "CMAKE_DIR=C:\Qt692\Tools\CMake_64\bin"
    set "MINGW_DIR=C:\Qt692\Tools\mingw1310_64\bin"
    set "NINJA_DIR=C:\Qt692\Tools\Ninja"
) else (
    set "QT_DIR=C:\Qt\6.9.2\mingw_64"
    set "CMAKE_DIR=C:\Qt\Tools\CMake_64\bin"
    set "MINGW_DIR=C:\Qt\Tools\mingw1310_64\bin"
    set "NINJA_DIR=C:\Qt\Tools\Ninja"
)

:: Update PATH for the build process
set "PATH=%QT_DIR%\bin;%CMAKE_DIR%;%MINGW_DIR%;%NINJA_DIR%;%PATH%"

echo - Qt Path: %QT_DIR%
echo - CMake Path: %CMAKE_DIR%
echo - MinGW Path: %MINGW_DIR%
echo - Ninja Path: %NINJA_DIR%
echo.

:: Check build folder
set "BUILD_DIR=build"
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
)

cd "%BUILD_DIR%"

set "CONSOLE_OPTION=OFF"
if /i "%1"=="debug" set "CONSOLE_OPTION=ON"
if /i "%2"=="debug" set "CONSOLE_OPTION=ON"

echo ==================================================
echo Running CMake Configuration (CONSOLE_LOGS=%CONSOLE_OPTION%)...
echo ==================================================
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCONSOLE_LOGS=%CONSOLE_OPTION% -DCMAKE_PREFIX_PATH="%QT_DIR%" ..
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

echo ==================================================
echo Compiling Deskmon...
echo ==================================================
cmake --build .
if errorlevel 1 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo ==================================================
echo Build successful! Launching Deskmon...
echo Logs will be printed below. Close Deskmon window to exit.
echo ==================================================

:: Run the executable directly. This blocks the script and shows output.
Deskmon.exe

echo ==================================================
echo Deskmon has closed.
echo ==================================================
pause
