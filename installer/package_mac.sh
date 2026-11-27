#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${WORKSPACE_DIR}/build"
DIST_DIR="${WORKSPACE_DIR}/dist"
APP_BUNDLE="${DIST_DIR}/Deskmon.app"

resolve_version() {
    if [[ -n "${1:-}" ]]; then
        echo "$1"
        return
    fi
    if [[ -n "${DESKMON_VERSION:-}" ]]; then
        echo "${DESKMON_VERSION}"
        return
    fi
    if [[ -f "${WORKSPACE_DIR}/version.txt" ]]; then
        tr -d '[:space:]' < "${WORKSPACE_DIR}/version.txt"
        return
    fi
    sed -n 's/^[[:space:]]*project([[:space:]]*Deskmon[[:space:]]*VERSION[[:space:]]*\([0-9.]*\).*/\1/p' "${WORKSPACE_DIR}/CMakeLists.txt" | head -n 1
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

hdiutil create -volname "Deskmon ${VERSION}" \
    -srcfolder "${DMG_STAGING}" \
    -ov -format UDZO \
    "${DMG_PATH}"
rm -rf "${DMG_STAGING}"
echo "Created macOS installer: ${DMG_PATH}"
