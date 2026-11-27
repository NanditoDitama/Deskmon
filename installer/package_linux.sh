#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${WORKSPACE_DIR}/build"
DIST_DIR="${WORKSPACE_DIR}/dist"
STAGING_DIR="${DIST_DIR}/Deskmon"

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
        if [[ -d "${candidate}/lib" ]]; then
            echo "${candidate}"
            return
        fi
    fi
    for d in "${HOME}"/Qt/6.*/gcc_64 /opt/Qt/6.*/gcc_64 /usr/lib/x86_64-linux-gnu/qt6; do
        if [[ -d "$d" ]]; then
            echo "$d"
            return
        fi
    done
    if command -v qtpaths6 >/dev/null 2>&1; then
        qtpaths6 --install-prefix
        return
    fi
    echo ""
}

VERSION="$(resolve_version "${1:-}")"
VERSION="${VERSION:-2.0.0.0}"
QT_DIR="$(find_qt_dir)"
ARCH="$(uname -m)"
if [[ "${ARCH}" == "aarch64" || "${ARCH}" == "arm64" ]]; then
    DEB_ARCH="arm64"
    PKG_ARCH="linux-arm64"
else
    DEB_ARCH="amd64"
    PKG_ARCH="linux-x64"
fi

echo "================================================================"
echo "  DESKMON LINUX INSTALLER & UPDATE PACKAGER"
echo "  Version: ${VERSION} | Arch: ${PKG_ARCH} (${DEB_ARCH})"
echo "  Qt Dir:  ${QT_DIR:-system}"
echo "================================================================"

CMAKE_ARGS=(-B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF)
if [[ -n "${QT_DIR}" ]]; then
    CMAKE_ARGS+=("-DCMAKE_PREFIX_PATH=${QT_DIR}")
fi

cmake "${WORKSPACE_DIR}" "${CMAKE_ARGS[@]}"
cmake --build "${BUILD_DIR}" --parallel

rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}/lib" "${STAGING_DIR}/plugins" "${STAGING_DIR}/qml" "${DIST_DIR}"

cp "${BUILD_DIR}/Deskmon" "${STAGING_DIR}/Deskmon"
chmod +x "${STAGING_DIR}/Deskmon"
cp -R "${WORKSPACE_DIR}/resources" "${STAGING_DIR}/resources"

if [[ -n "${QT_DIR}" && -d "${QT_DIR}/lib" ]]; then
    cp -a "${QT_DIR}/lib"/libQt6*.so* "${STAGING_DIR}/lib/" 2>/dev/null || true
    cp -a "${QT_DIR}/lib"/libicu*.so* "${STAGING_DIR}/lib/" 2>/dev/null || true

    for plugin_cat in platforms sqldrivers imageformats iconengines tls networkinformation xcbglintegrations wayland-shell-integration wayland-decoration-client wayland-graphics-integration-client; do
        if [[ -d "${QT_DIR}/plugins/${plugin_cat}" ]]; then
            cp -a "${QT_DIR}/plugins/${plugin_cat}" "${STAGING_DIR}/plugins/"
        fi
    done

    for qml_mod in QtQuick QtQml QtCore Qt5Compat; do
        if [[ -d "${QT_DIR}/qml/${qml_mod}" ]]; then
            cp -a "${QT_DIR}/qml/${qml_mod}" "${STAGING_DIR}/qml/"
        fi
    done
fi

cat > "${STAGING_DIR}/qt.conf" <<'EOF'
[Paths]
Prefix = .
Libraries = lib
Plugins = plugins
QmlImports = qml
EOF

if command -v patchelf >/dev/null 2>&1; then
    patchelf --set-rpath '$ORIGIN:$ORIGIN/lib' "${STAGING_DIR}/Deskmon" || true
fi

cat > "${STAGING_DIR}/deskmon.sh" <<'EOF'
#!/usr/bin/env bash
HERE="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
export LD_LIBRARY_PATH="${HERE}/lib:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="${HERE}/plugins"
export QML2_IMPORT_PATH="${HERE}/qml"
export QML_IMPORT_PATH="${HERE}/qml"
exec "${HERE}/Deskmon" "$@"
EOF
chmod +x "${STAGING_DIR}/deskmon.sh"

# 1. Portable tar.gz update archive
TAR_NAME="Deskmon-${VERSION}-${PKG_ARCH}.tar.gz"
TAR_PATH="${DIST_DIR}/${TAR_NAME}"
tar -czf "${TAR_PATH}" -C "${STAGING_DIR}" .
sha256sum "${TAR_PATH}" | awk '{print $1}' > "${TAR_PATH}.sha256"
echo "Created update archive: ${TAR_PATH}"

# 2. Debian/Ubuntu .deb installer package
if command -v dpkg-deb >/dev/null 2>&1; then
    DEB_WORK="${DIST_DIR}/deb_build"
    DEB_NAME="Deskmon-${VERSION}-${DEB_ARCH}.deb"
    DEB_PATH="${DIST_DIR}/${DEB_NAME}"
    rm -rf "${DEB_WORK}" "${DEB_PATH}"

    mkdir -p "${DEB_WORK}/DEBIAN" \
             "${DEB_WORK}/opt/deskmon" \
             "${DEB_WORK}/usr/local/bin" \
             "${DEB_WORK}/usr/share/applications" \
             "${DEB_WORK}/usr/share/icons/hicolor/256x256/apps"

    cp -a "${STAGING_DIR}/." "${DEB_WORK}/opt/deskmon/"
    ln -sf "/opt/deskmon/deskmon.sh" "${DEB_WORK}/usr/local/bin/deskmon"

    if [[ -f "${WORKSPACE_DIR}/resources/images/icon.png" ]]; then
        cp "${WORKSPACE_DIR}/resources/images/icon.png" \
           "${DEB_WORK}/usr/share/icons/hicolor/256x256/apps/deskmon.png"
    fi

    cat > "${DEB_WORK}/usr/share/applications/deskmon.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Deskmon
Comment=Deskmon Desktop Workspace & Activity Monitor
Exec=/opt/deskmon/deskmon.sh
Icon=deskmon
Terminal=false
Categories=Office;Utility;
StartupWMClass=Deskmon
EOF

    cat > "${DEB_WORK}/DEBIAN/control" <<EOF
Package: deskmon
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: ${DEB_ARCH}
Maintainer: Deskmon <support@pranala-dt.co.id>
Depends: libc6, libstdc++6, libgl1, libx11-6, libxss1, libxcb-cursor0
Description: Deskmon Desktop Activity Monitor
 Cross-platform work tracking and productivity monitoring application.
EOF

    dpkg-deb --root-owner-group --build "${DEB_WORK}" "${DEB_PATH}"
    rm -rf "${DEB_WORK}"
    echo "Created .deb installer: ${DEB_PATH}"
else
    echo "Notice: dpkg-deb not installed, skipped .deb creation."
fi
