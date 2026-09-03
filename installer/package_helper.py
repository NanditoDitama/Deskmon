import os
import sys
import shutil
import subprocess

WORKSPACE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DIST_DIR = os.path.join(WORKSPACE_DIR, "dist")
APP_STAGING_DIR = os.path.join(DIST_DIR, "Deskmon")
INSTALLER_DIR = os.path.join(WORKSPACE_DIR, "installer")
BUILD_DIR = os.path.join(WORKSPACE_DIR, "build")

QT_DIRS = [
    r"C:\Qt692\6.9.2\mingw_64",
    r"C:\Qt\6.9.2\mingw_64",
]

ISCC_PATHS = [
    r"C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    r"C:\Program Files\Inno Setup 6\ISCC.exe",
]

def find_qt_dir():
    for d in QT_DIRS:
        if os.path.exists(d):
            return d
    raise RuntimeError("Qt 6 directory not found! Checked: " + ", ".join(QT_DIRS))

def find_tools():
    qt_dir = find_qt_dir()
    cmake_dir = r"C:\Qt692\Tools\CMake_64\bin" if os.path.exists(r"C:\Qt692\Tools\CMake_64\bin") else r"C:\Qt\Tools\CMake_64\bin"
    mingw_dir = r"C:\Qt692\Tools\mingw1310_64\bin" if os.path.exists(r"C:\Qt692\Tools\mingw1310_64\bin") else r"C:\Qt\Tools\mingw1310_64\bin"
    ninja_dir = r"C:\Qt692\Tools\Ninja" if os.path.exists(r"C:\Qt692\Tools\Ninja") else r"C:\Qt\Tools\Ninja"
    return qt_dir, cmake_dir, mingw_dir, ninja_dir

def find_iscc():
    for p in ISCC_PATHS:
        if os.path.exists(p):
            return p
    raise RuntimeError("Inno Setup 6 (ISCC.exe) not found! Checked: " + ", ".join(ISCC_PATHS))

def get_app_version():
    cmake_path = os.path.join(WORKSPACE_DIR, "CMakeLists.txt")
    if os.path.exists(cmake_path):
        with open(cmake_path, "r", encoding="utf-8") as f:
            for line in f:
                if "project" in line.lower() and "version" in line.lower():
                    parts = line.replace("(", " ").replace(")", " ").split()
                    if "VERSION" in parts:
                        idx = parts.index("VERSION")
                        if idx + 1 < len(parts):
                            return parts[idx + 1].strip('"\'')
    return "1.0.3.4"

def step_build_gui_executable(qt_dir, cmake_dir, mingw_dir, ninja_dir):
    print("\n[STEP 1/4] Compiling clean GUI binary (CONSOLE_LOGS=OFF - no terminal window)...")
    os.makedirs(BUILD_DIR, exist_ok=True)
    env = os.environ.copy()
    env["PATH"] = f"{qt_dir}\\bin;{cmake_dir};{mingw_dir};{ninja_dir};" + env.get("PATH", "")

    cmake_exe = os.path.join(cmake_dir, "cmake.exe")

    # Configure CMake with CONSOLE_LOGS=OFF so Windows links as WIN32_EXECUTABLE (no console window)
    configure_cmd = [
        cmake_exe,
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCONSOLE_LOGS=OFF",
        f"-DCMAKE_PREFIX_PATH={qt_dir}",
        ".."
    ]
    print("Configuring CMake:", " ".join(configure_cmd))
    res = subprocess.run(configure_cmd, cwd=BUILD_DIR, env=env)
    if res.returncode != 0:
        raise RuntimeError("CMake configuration failed.")

    # Build Deskmon.exe
    build_cmd = [cmake_exe, "--build", "."]
    print("Building Deskmon.exe...")
    res = subprocess.run(build_cmd, cwd=BUILD_DIR, env=env)
    if res.returncode != 0:
        raise RuntimeError("CMake build failed.")

def step_generate_assets():
    print("\n[STEP 2/4] Generating custom installer visual assets (no company branding)...")
    sys.path.insert(0, INSTALLER_DIR)
    from generate_installer_assets import create_wizard_large, create_wizard_small
    assets_dir = os.path.join(INSTALLER_DIR, "assets")
    os.makedirs(assets_dir, exist_ok=True)
    create_wizard_large(os.path.join(assets_dir, "wizard_large.bmp"))
    create_wizard_small(os.path.join(assets_dir, "wizard_small.bmp"))

def kill_running_instances():
    try:
        subprocess.run(["cmd.exe", "/c", "taskkill /IM Deskmon.exe /T /F"], 
                       capture_output=True, text=True)
    except Exception:
        pass
    import time
    time.sleep(0.5)

def step_stage_files(qt_dir):
    print("\n[STEP 3/4] Staging Deskmon application files...")
    exe_src = os.path.join(BUILD_DIR, "Deskmon.exe")
    if not os.path.exists(exe_src):
        raise RuntimeError(f"Deskmon.exe not found at {exe_src}.")

    os.makedirs(APP_STAGING_DIR, exist_ok=True)
    exe_dst = os.path.join(APP_STAGING_DIR, "Deskmon.exe")

    # Tutup Deskmon jika sedang berjalan di folder staging agar file tidak terkunci
    kill_running_instances()

    import time
    for attempt in range(4):
        try:
            shutil.copy2(exe_src, exe_dst)
            break
        except PermissionError:
            kill_running_instances()
            time.sleep(0.5)
            if attempt == 3:
                raise

    print(f"Copied {exe_src} -> {exe_dst}")

    # Copy resources folder
    res_src = os.path.join(WORKSPACE_DIR, "resources")
    res_dst = os.path.join(APP_STAGING_DIR, "resources")
    if os.path.exists(res_src):
        if os.path.exists(res_dst):
            shutil.rmtree(res_dst)
        shutil.copytree(res_src, res_dst)
        print(f"Copied resources -> {res_dst}")

    # Run windeployqt
    windeployqt_path = os.path.join(qt_dir, "bin", "windeployqt.exe")
    qml_dir = os.path.join(WORKSPACE_DIR, "qml")
    cmd = [
        windeployqt_path,
        "--qmldir", qml_dir,
        "--no-translations",
        exe_dst
    ]
    print(f"Running windeployqt: {' '.join(cmd)}")
    env = os.environ.copy()
    env["PATH"] = os.path.join(qt_dir, "bin") + ";" + env.get("PATH", "")
    res = subprocess.run(cmd, env=env, capture_output=True, text=True)
    if res.returncode != 0:
        print("windeployqt warning/output:", res.stderr)

    # Ensure sqldrivers/qsqlite.dll is present
    sqldrivers_dst = os.path.join(APP_STAGING_DIR, "sqldrivers")
    os.makedirs(sqldrivers_dst, exist_ok=True)
    sqlite_src = os.path.join(qt_dir, "plugins", "sqldrivers", "qsqlite.dll")
    if os.path.exists(sqlite_src):
        shutil.copy2(sqlite_src, os.path.join(sqldrivers_dst, "qsqlite.dll"))
        print("Ensured sqldrivers/qsqlite.dll is present.")

    # Ensure imageformats/qsvg.dll is present
    imageformats_dst = os.path.join(APP_STAGING_DIR, "imageformats")
    os.makedirs(imageformats_dst, exist_ok=True)
    svg_src = os.path.join(qt_dir, "plugins", "imageformats", "qsvg.dll")
    if os.path.exists(svg_src):
        shutil.copy2(svg_src, os.path.join(imageformats_dst, "qsvg.dll"))
        print("Ensured imageformats/qsvg.dll is present.")

def step_compile_installer(iscc_path, version):
    print(f"\n[STEP 4/4] Compiling Inno Setup installer for v{version}...")
    iss_file = os.path.join(INSTALLER_DIR, "Deskmon_Setup.iss")
    cmd = [
        iscc_path,
        f"/DAppVersion={version}",
        iss_file
    ]
    print(f"Running ISCC: {' '.join(cmd)}")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("[ERROR] Inno Setup compilation failed!")
        print(res.stderr or res.stdout)
        sys.exit(1)
    
    output_exe = os.path.join(DIST_DIR, f"Deskmon-Setup-{version}.exe")
    if os.path.exists(output_exe):
        size_mb = os.path.getsize(output_exe) / (1024 * 1024)
        print("\n" + "=" * 64)
        print("SUCCESS: Installer created successfully!")
        print(f"Output File: {output_exe} ({size_mb:.2f} MB)")
        print("GUI Subsystem: Active (No console/terminal window)")
        print("=" * 64 + "\n")
    else:
        print(f"Warning: Installer output file not found at {output_exe}")

def main():
    keep_folder = False
    for arg in sys.argv[1:]:
        if arg.lower() in ["folder_build", "folder", "--folder", "-f", "keep_folder"]:
            keep_folder = True

    qt_dir, cmake_dir, mingw_dir, ninja_dir = find_tools()
    iscc_path = find_iscc()
    version = get_app_version()

    mode_label = "Installer + Folder (folder_build)" if keep_folder else "Installer Only (clean)"
    print(f"Deskmon Installer Builder")
    print(f"- Workspace:   {WORKSPACE_DIR}")
    print(f"- Version:     {version}")
    print(f"- Output Mode: {mode_label}")
    print(f"- Qt 6 Dir:    {qt_dir}")
    print(f"- ISCC Path:   {iscc_path}")

    step_build_gui_executable(qt_dir, cmake_dir, mingw_dir, ninja_dir)
    step_generate_assets()
    step_stage_files(qt_dir)
    step_compile_installer(iscc_path, version)

    # Bersihkan folder staging jika opsi folder_build tidak diberikan
    if not keep_folder:
        print(f"\n[CLEANUP] Cleaning up staging folder ({APP_STAGING_DIR})...")
        import time
        for attempt in range(4):
            try:
                if os.path.exists(APP_STAGING_DIR):
                    shutil.rmtree(APP_STAGING_DIR)
                break
            except Exception:
                kill_running_instances()
                time.sleep(0.5)
        print("[CLEANUP] Output folder 'dist/' now contains ONLY the installer (.exe).")
    else:
        print(f"\n[INFO] Build folder retained at: {APP_STAGING_DIR}")

if __name__ == "__main__":
    main()
