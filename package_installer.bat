@echo off
setlocal

echo ================================================================
echo           DESKMON CUSTOM INSTALLER PACKAGING TOOL
echo ================================================================
if "%~1"=="" (
    echo [MODE] Default: Output installer .exe dan paket update .zip.
    echo        Gunakan "package_installer.bat folder_build" jika ingin mempertahankan folder aplikasi.
) else (
    echo [MODE] Argumen terdeteksi: %*
)
echo.

python "%~dp0installer\package_helper.py" %*
if errorlevel 1 (
    echo.
    echo [ERROR] Terjadi kesalahan saat membuat installer.
    pause
    exit /b 1
)

echo.
pause
