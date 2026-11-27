# Deskmon

**Deskmon** adalah aplikasi desktop lintas platform (Windows, macOS, dan Linux) berbasis **Qt 6 / QML** dan **C++17** untuk memantau aktivitas kerja, penggunaan aplikasi dan situs web, manajemen tugas (*task tracking*), deteksi *idle*, serta produktivitas harian.

---

## Fitur Utama

- **Activity & Window Monitoring** — Mencatat jendela aktif dan domain situs web secara real-time untuk Windows, macOS, dan Linux (`WindowInfoProvider_*`), dilengkapi ekstraksi ikon aplikasi (`AppIconProvider`).
- **Task & Work Time Tracking** — Mendukung pemilihan/perpindahan tugas aktif, penghentian sementara (*pause/resume*), pengajuan *Need Review*, pengingat *Time Up*, serta pelacakan jam kerja harian dan *Early Leave*.
- **Productivity Analytics** — Klasifikasi aplikasi dan domain ke kategori *Productive*, *Non-Productive*, *Idle*, dan *Neutral*, termasuk fitur pengajuan aplikasi/domain baru untuk dipantau.
- **Multi-Bahasa & Tema Persisten** — Mendukung **English (`en`)** dan **Bahasa Indonesia (`id`)** (`qml/theme/Lang.qml` + auto-sync `scripts/sync_i18n.py`) serta **Dark / Light Mode** (`qml/theme/Theme.qml`) yang tersimpan otomatis via `QtCore.Settings`.
- **Auto-Updater Lintas OS** — Pemeriksaan versi otomatis melalui `version.json`, progress non-blocking di tombol header, verifikasi integritas **SHA-256**, dan skrip pembaruan otomatis untuk Windows, macOS, dan Linux (`UpdateManager`).
- **Single-Instance & System Tray** — Aplikasi berjalan di latar belakang melalui *system tray* dengan kontrol cepat (*Show*, *Pause/Resume*, *Quit*).

---

## Struktur Proyek

```text
Deskmon/
├── .github/workflows/
│   └── release.yml                     # CI/CD multi-OS build & generator version.json
├── installer/
│   ├── assets/                         # Aset visual wizard Inno Setup (.bmp)
│   ├── build_win_installer_package.bat # Packaging installer & update ZIP (Windows)
│   ├── build_linux_installer_package.sh# Packaging .tar.gz & .deb (Linux)
│   ├── build_mac_installer_package.sh  # Packaging .zip, .dmg & uninstaller (macOS)
│   ├── Deskmon_Setup.iss               # Konfigurasi Inno Setup (Windows)
│   ├── generate_installer_assets.py    # Generator gambar wizard installer
│   └── package_helper.py               # Helper build, windeployqt, & SHA-256 (Windows)
├── qml/
│   ├── application_submission/         # Dialog pengajuan aplikasi/domain pantauan
│   ├── components/                     # Komponen UI reusable & ToastNotification
│   ├── dashboard/                      # Halaman dashboard, produktivitas, tugas, log
│   ├── dialogs/                        # Dialog global (UpdateDialog, EarlyLeave, dll.)
│   ├── layouts/                        # HeaderLayout & BodyLayout
│   ├── login/                          # Halaman login
│   ├── profile/                        # Halaman profil & crop avatar
│   ├── theme/                          # Theme.qml & Lang.qml
│   └── Main.qml                        # Root window & router aplikasi
├── resources/                          # Ikon SVG, gambar, dan ikon aplikasi
├── scripts/
│   └── sync_i18n.py                    # Auto-scan & sinkronisasi kamus i18n QML
├── src/
│   ├── app/                            # main.cpp & AppController
│   ├── core/                           # ApiClient, DatabaseManager, repositories, platform
│   └── features/                       # auth, tasks, tracking, productivity, worktime, profile, updater
├── tests/                              # Unit tests (Qt Test) & E2E tests
├── CMakeLists.txt
├── version.txt                         # Single source of truth versi aplikasi
├── build.bat                           # Skrip build cepat (Windows)
├── build.sh                            # Skrip build cepat (Linux & macOS)
└── sync_i18n.bat                       # Shortcut sinkronisasi i18n (Windows)
```

---

## Prasyarat Build

- **Qt 6** (`>= 6.8`, teruji dengan **Qt 6.12.0**) dengan modul: `Core`, `Gui`, `Qml`, `Quick`, `QuickControls2`, `Sql`, `Network`, `Widgets`, `Svg`, dan `Test` (jika `BUILD_TESTING=ON`).
- **CMake** `>= 3.16` dan **Ninja** (atau Make/MinGW Makefiles).
- **Compiler C++17**:
  - Windows: MinGW-w64 (`mingw1310_64` / GCC)
  - macOS: Clang + Xcode Command Line Tools
  - Linux: GCC / Clang + dependensi X11 (`libgl1-mesa-dev`, `libx11-dev`, `libxss-dev`, `libxcb-cursor0`, `libxkbcommon-dev`)
- **Python 3** (opsional namun disarankan; menjalankan `scripts/sync_i18n.py` saat build dan `package_helper.py` saat packaging Windows).
- **Inno Setup 6** (`ISCC.exe`) — khusus untuk membuat installer `.exe` di Windows.

---

## Cara Build & Menjalankan Lokal

### Windows
```bat
:: Debug build (dengan console log)
build.bat

:: Release build (tanpa jendela terminal / WIN32_EXECUTABLE)
build.bat release
```
Output executable berada di `build\Deskmon.exe`.

### Linux & macOS
```bash
# Debug build dan langsung jalankan
./build.sh

# Release build tanpa langsung menjalankan aplikasi
./build.sh release --no-run
```

---

## Menjalankan Unit Test

```bash
ctest --test-dir build --output-on-failure
```

Target unit test yang tersedia di `tests/unit/`:
- `test_database_and_repos`
- `test_task_management`
- `test_services_idle_auth`
- `test_updater`

---

## Packaging Installer & Rilis

Versi aplikasi dibaca secara terpusat dari [version.txt](version.txt).

### Windows (`.exe` Installer + `-win64.zip` + `.sha256`)
```bat
:: Membuat installer .exe dan arsip update .zip di folder dist\
installer\build_win_installer_package.bat

:: Mempertahankan folder staging dist\Deskmon
installer\build_win_installer_package.bat folder_build

:: Hanya membuat arsip update .zip dan .sha256
installer\build_win_installer_package.bat --zip-only
```
Uninstaller Windows juga menyediakan opsi checkbox untuk menghapus `deskmon.db`, profil tersimpan, dan preferensi (default: tidak terhapus), serta membersihkan cache (`logs`, `cache`, `icon_cache`, `updates`).

### Linux (`.tar.gz` + `.sha256` + `.deb`)
```bash
bash installer/build_linux_installer_package.sh
```
Output berada di `dist/`. Paket `.deb` memasang aplikasi ke `/opt/deskmon`, mendukung `apt remove` (membersihkan cache sementara) dan `apt purge` (menghapus database & konfigurasi pengguna setelah cek file lock).

### macOS (`.zip` + `.sha256` + `.dmg`)
```bash
bash installer/build_mac_installer_package.sh
```
Output `.dmg` di `dist/` sudah menyertakan `Deskmon.app`, tautan `/Applications`, serta skrip `Uninstall Deskmon.command` dengan pilihan *Keep Database* atau *Delete All + DB*.

### CI/CD Multi-OS (GitHub Actions)
Workflow [.github/workflows/release.yml](.github/workflows/release.yml) berjalan otomatis saat push tag `v*.*.*.*` atau manual lewat `workflow_dispatch` untuk membuild artifak Windows, macOS, dan Linux serta menghasilkan manifest `version.json`.

---

## Lokasi Data Lokal & Environment Variables

| OS | Database, Log, & Cache | Profil / Preferensi |
|---|---|---|
| **Windows** | `%LOCALAPPDATA%\Pranala\Deskmon\` | `%APPDATA%\Pranala\Deskmon\` & Registry `HKCU\Software\Pranala\Deskmon` |
| **Linux** | `~/.local/share/Pranala/Deskmon/` & `~/.cache/Pranala/Deskmon/` | `~/.config/Pranala/Deskmon.conf` |
| **macOS** | `~/Library/Application Support/Pranala/Deskmon/` & `~/Library/Caches/Pranala/Deskmon/` | `~/Library/Preferences/com.pranala.Deskmon.plist` |

Environment variable opsional untuk pengembangan/pengujian:
- `DESKMON_API_BASE_URL` — Override base URL API (default: `https://deskmon.pranala-dt.co.id/api`).
- `DESKMON_DB_PATH` — Override lokasi file SQLite `deskmon.db`.
- `DESKMON_DISABLE_STARTUP_UPDATE_CHECK=1` — Menonaktifkan pengecekan update otomatis saat aplikasi dimulai.
