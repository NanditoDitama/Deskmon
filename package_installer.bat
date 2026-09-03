@echo off
setlocal

echo ================================================================
echo           DESKMON CUSTOM INSTALLER PACKAGING TOOL
echo ================================================================
if "%~1"=="" (
    echo [MODE] Default: Output HANYA installer (.exe).
    echo        (Gunakan "package_installer.bat folder_build" jika ingin output folder aplikasi)
) else (
    echo [MODE] Argumen terdeteksi: %*
)
echo.

python installer\package_helper.py %*
if errorlevel 1 (
    echo.
    echo [ERROR] Terjadi kesalahan saat membuat installer.
    pause
    exit /b 1
)

echo.
pause
