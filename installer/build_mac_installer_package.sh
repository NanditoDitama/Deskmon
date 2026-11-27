#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${WORKSPACE_DIR}/build"
DIST_DIR="${WORKSPACE_DIR}/dist"
APP_BUNDLE="${DIST_DIR}/Deskmon.app"

resolve_version() {
    local raw=""
    if [[ -n "${1:-}" ]]; then
        raw="$1"
    elif [[ -n "${DESKMON_VERSION:-}" ]]; then
        raw="${DESKMON_VERSION}"
    elif [[ -f "${WORKSPACE_DIR}/version.txt" ]]; then
        raw="$(tr -d '[:space:]' < "${WORKSPACE_DIR}/version.txt")"
    else
        raw="$(sed -n 's/^[[:space:]]*project([[:space:]]*Deskmon[[:space:]]*VERSION[[:space:]]*\([0-9.]*\).*/\1/p' "${WORKSPACE_DIR}/CMakeLists.txt" | head -n 1)"
    fi
    printf '%s' "${raw}" | grep -oE '[0-9]+(\.[0-9]+)+' | head -n 1 || true
}

find_qt_dir() {
    if [[ -n "${QT_ROOT_DIR:-}" && -d "${QT_ROOT_DIR}" ]]; then
        echo "${QT_ROOT_DIR}"
        return
    fi
    if [[ -n "${Qt6_DIR:-}" ]]; then
        local candidate
        candidate="$(cd "${Qt6_DIR}/../../.." 2>/dev/null && pwd || true)"
        if [[ -d "${candidate}/bin" ]]; then
            echo "${candidate}"
            return
        fi
    fi
    for d in "${HOME}"/Qt/6.*/macos /opt/homebrew/opt/qt /usr/local/opt/qt; do
        if [[ -d "$d" ]]; then
            echo "$d"
            return
        fi
    done
    echo ""
}

generate_icns_if_missing() {
    local target_icns="$1"
    local src_png="${WORKSPACE_DIR}/resources/images/icon.png"
    if [[ ! -f "${src_png}" ]] || ! command -v sips >/dev/null 2>&1 || ! command -v iconutil >/dev/null 2>&1; then
        return
    fi
    local iconset_dir="${BUILD_DIR}/Deskmon.iconset"
    rm -rf "${iconset_dir}"
    mkdir -p "${iconset_dir}"
    for sz in 16 32 128 256 512; do
        sips -z "${sz}" "${sz}" "${src_png}" --out "${iconset_dir}/icon_${sz}x${sz}.png" >/dev/null
        local dbl=$((sz * 2))
        sips -z "${dbl}" "${dbl}" "${src_png}" --out "${iconset_dir}/icon_${sz}x${sz}@2x.png" >/dev/null
    done
    iconutil -c icns "${iconset_dir}" -o "${target_icns}"
    rm -rf "${iconset_dir}"
}

VERSION="$(resolve_version "${1:-}")"
VERSION="${VERSION:-2.0.0.0}"
QT_DIR="$(find_qt_dir)"

echo "================================================================"
echo "  DESKMON macOS DMG & UPDATE PACKAGER"
echo "  Version: ${VERSION}"
echo "  Qt Dir:  ${QT_DIR:-PATH}"
echo "================================================================"

if [[ -n "${QT_DIR}" ]]; then
    export PATH="${QT_DIR}/bin:${PATH}"
fi

CMAKE_ARGS=(-B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF)
if [[ -n "${QT_DIR}" ]]; then
    CMAKE_ARGS+=("-DCMAKE_PREFIX_PATH=${QT_DIR}")
fi

cmake "${WORKSPACE_DIR}" "${CMAKE_ARGS[@]}"
cmake --build "${BUILD_DIR}" --parallel

rm -rf "${APP_BUNDLE}"
mkdir -p "${DIST_DIR}"
cp -R "${BUILD_DIR}/Deskmon.app" "${APP_BUNDLE}"

if [[ ! -f "${APP_BUNDLE}/Contents/Resources/icon.icns" ]]; then
    mkdir -p "${APP_BUNDLE}/Contents/Resources"
    generate_icns_if_missing "${APP_BUNDLE}/Contents/Resources/icon.icns"
fi

macdeployqt "${APP_BUNDLE}" -qmldir="${WORKSPACE_DIR}/qml" -always-overwrite
codesign --force --deep --sign - "${APP_BUNDLE}" || true

# 1. ZIP archive for auto-updater
ZIP_NAME="Deskmon-${VERSION}-macos.zip"
ZIP_PATH="${DIST_DIR}/${ZIP_NAME}"
rm -f "${ZIP_PATH}"
ditto -c -k --sequesterRsrc --keepParent "${APP_BUNDLE}" "${ZIP_PATH}"
shasum -a 256 "${ZIP_PATH}" | awk '{print $1}' > "${ZIP_PATH}.sha256"
echo "Created update archive: ${ZIP_PATH}"

# 2. Drag-and-drop DMG installer with /Applications symlink
DMG_STAGING="${DIST_DIR}/dmg_staging"
DMG_NAME="Deskmon-${VERSION}-macos.dmg"
DMG_PATH="${DIST_DIR}/${DMG_NAME}"
rm -rf "${DMG_STAGING}" "${DMG_PATH}"
mkdir -p "${DMG_STAGING}"
cp -R "${APP_BUNDLE}" "${DMG_STAGING}/Deskmon.app"
ln -s /Applications "${DMG_STAGING}/Applications"

cat > "${DMG_STAGING}/Uninstall Deskmon.command" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

show_error() {
    local msg="$1"
    echo "[ERROR] ${msg}" >&2
    osascript -e "display alert \"Deskmon Uninstaller Error\" message \"${msg}\" as critical" >/dev/null 2>&1 || true
}

APP_PATH="/Applications/Deskmon.app"
DATA_DIR="${HOME}/Library/Application Support/Pranala/Deskmon"
ORG_DATA_DIR="${HOME}/Library/Application Support/Pranala"
CACHE_DIR="${HOME}/Library/Caches/Pranala/Deskmon"
ORG_CACHE_DIR="${HOME}/Library/Caches/Pranala"
PREF_PLIST="${HOME}/Library/Preferences/com.pranala.Deskmon.plist"
DB_FILE="${DATA_DIR}/deskmon.db"

CHOICE="$(osascript <<'APPLESCRIPT'
try
    set resp to display dialog "Choose how you want to uninstall Deskmon:" & return & return & "• Keep Database: removes Deskmon.app and temporary caches/logs, keeping deskmon.db intact." & return & "• Delete All + DB: also removes deskmon.db, profile data, and saved preferences." with title "Uninstall Deskmon" buttons {"Cancel", "Keep Database", "Delete All + DB"} default button "Keep Database"
    return button returned of resp
on error number -128
    return "Cancel"
end try
APPLESCRIPT
)"

if [ "${CHOICE}" = "Cancel" ] || [ -z "${CHOICE}" ]; then
    exit 0
fi

pkill -x "Deskmon" >/dev/null 2>&1 || true
sleep 0.5

if [ "${CHOICE}" = "Delete All + DB" ] && [ -f "${DB_FILE}" ]; then
    while lsof "${DB_FILE}" >/dev/null 2>&1; do
        RETRY="$(osascript <<'APPLESCRIPT'
try
    set r to display dialog "Unable to delete deskmon.db because it is currently open in another application. Close the program using deskmon.db and click Retry." with title "Database Locked" buttons {"Cancel", "Retry"} default button "Retry" with icon stop
    return button returned of r
on error number -128
    return "Cancel"
end try
APPLESCRIPT
)"
        if [ "${RETRY}" != "Retry" ]; then
            show_error "Uninstall aborted because deskmon.db is still open in another application."
            exit 1
        fi
        pkill -x "Deskmon" >/dev/null 2>&1 || true
    done
fi

if [ -d "${APP_PATH}" ]; then
    if [ -w "$(dirname "${APP_PATH}")" ]; then
        if ! rm -rf "${APP_PATH}"; then
            show_error "Failed to remove ${APP_PATH}. Make sure Deskmon is closed."
            exit 1
        fi
    else
        if ! osascript -e "do shell script \"rm -rf '${APP_PATH}'\" with administrator privileges" >/dev/null 2>&1; then
            show_error "Failed to remove ${APP_PATH} (administrator permission canceled or application files are in use)."
            exit 1
        fi
    fi
fi

# Bersihkan cache, logs, icon_cache, updates
rm -rf "${DATA_DIR}/logs" "${DATA_DIR}/cache" "${DATA_DIR}/icon_cache" "${DATA_DIR}/updates" "${CACHE_DIR}" 2>/dev/null || true

if [ "${CHOICE}" = "Delete All + DB" ]; then
    if [ -f "${DB_FILE}" ] && ! rm -f "${DB_FILE}" "${DB_FILE}-wal" "${DB_FILE}-shm"; then
        show_error "Failed to delete ${DB_FILE}. Make sure no application is holding it open."
        exit 1
    fi
    rm -rf "${DATA_DIR}/profiles" "${PREF_PLIST}" 2>/dev/null || true
fi

rmdir "${DATA_DIR}" 2>/dev/null || true
rmdir "${ORG_DATA_DIR}" 2>/dev/null || true
rmdir "${CACHE_DIR}" 2>/dev/null || true
rmdir "${ORG_CACHE_DIR}" 2>/dev/null || true

osascript -e 'display notification "Deskmon has been uninstalled cleanly." with title "Uninstall Deskmon"' >/dev/null 2>&1 || true
echo "Deskmon uninstalled successfully (${CHOICE})."
EOF
chmod +x "${DMG_STAGING}/Uninstall Deskmon.command"

hdiutil create -volname "Deskmon ${VERSION}" \
    -srcfolder "${DMG_STAGING}" \
    -ov -format UDZO \
    "${DMG_PATH}"
rm -rf "${DMG_STAGING}"
echo "Created macOS installer: ${DMG_PATH}"
