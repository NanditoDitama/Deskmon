#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
OS_NAME="$(uname -s)"

case "${OS_NAME}" in
    Darwin|Linux) ;;
    *)
        echo "[ERROR] OS '${OS_NAME}' tidak didukung oleh build.sh. Gunakan build.bat untuk Windows."
        exit 1
        ;;
esac

BUILD_TYPE="Debug"
RUN_APP=1
for arg in ${1:+"$@"}; do
    arg_lower="$(printf '%s' "$arg" | tr '[:upper:]' '[:lower:]')"
    case "${arg_lower}" in
        release) BUILD_TYPE="Release" ;;
        --no-run|build-only) RUN_APP=0 ;;
    esac
done

find_qt_dir() {
    if [[ -n "${QT_ROOT_DIR:-}" && -d "${QT_ROOT_DIR}" ]]; then
        echo "${QT_ROOT_DIR}"
        return
    fi
    if [[ -n "${Qt6_DIR:-}" ]]; then
        local base
        base="$(cd "${Qt6_DIR}/../../.." 2>/dev/null && pwd || true)"
        if [[ -d "${base}" ]]; then
            echo "${base}"
            return
        fi
    fi

    local found=""
    if [[ "${OS_NAME}" == "Darwin" ]]; then
        for d in /usr/local/opt/qt /opt/homebrew/opt/qt "${HOME}"/Qt/6.*/macos; do
            [[ -d "$d" ]] && found="$d"
        done
        if [[ -n "${found}" ]]; then
            echo "${found}"
            return
        fi
    else
        for d in /usr/lib/aarch64-linux-gnu/qt6 /usr/lib/x86_64-linux-gnu/qt6 /opt/Qt/6.*/gcc_64 "${HOME}"/Qt/6.*/gcc_64; do
            [[ -d "$d" ]] && found="$d"
        done
        if [[ -n "${found}" ]]; then
            echo "${found}"
            return
        fi
        if command -v qtpaths6 >/dev/null 2>&1; then
            qtpaths6 --install-prefix
            return
        fi
    fi
    echo ""
}

echo "=================================================="
echo "Configuring environment for Deskmon build (${OS_NAME})..."
echo "=================================================="

QT_DIR="$(find_qt_dir)"

# Add Qt / CMake / Ninja tools to PATH if available
if [[ "${OS_NAME}" == "Darwin" ]]; then
    for tool_path in "${HOME}/Qt/Tools/CMake/CMake.app/Contents/bin" "${HOME}/Qt/Tools/Ninja" "/opt/homebrew/bin" "/usr/local/bin"; do
        [[ -d "${tool_path}" ]] && export PATH="${tool_path}:${PATH}"
    done
else
    for tool_path in "${HOME}/Qt/Tools/CMake/bin" "${HOME}/Qt/Tools/Ninja"; do
        [[ -d "${tool_path}" ]] && export PATH="${tool_path}:${PATH}"
    done
fi

if [[ -n "${QT_DIR}" && -d "${QT_DIR}/bin" ]]; then
    export PATH="${QT_DIR}/bin:${PATH}"
fi

echo "- OS:         ${OS_NAME}"
echo "- Build Type: ${BUILD_TYPE}"
echo "- Qt Path:    ${QT_DIR:-system default}"
echo "- CMake:      $(command -v cmake || echo 'not found')"
echo "- Generator:  $(command -v ninja >/dev/null 2>&1 && echo 'Ninja' || echo 'Unix Makefiles')"
echo ""

if ! command -v cmake >/dev/null 2>&1; then
    echo "[ERROR] cmake tidak ditemukan di PATH. Silakan install cmake atau Qt CMake Tools."
    exit 1
fi

CMAKE_ARGS=(-S "${SCRIPT_DIR}" -B "${BUILD_DIR}" "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}")
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]] && command -v ninja >/dev/null 2>&1; then
    CMAKE_ARGS+=(-G Ninja)
fi
if [[ -n "${QT_DIR}" ]]; then
    CMAKE_ARGS+=("-DCMAKE_PREFIX_PATH=${QT_DIR}")
fi

echo "=================================================="
echo "Running CMake Configuration..."
echo "=================================================="
cmake "${CMAKE_ARGS[@]}"

echo "=================================================="
echo "Compiling Deskmon..."
echo "=================================================="
pkill -x Deskmon >/dev/null 2>&1 || true

if [[ "${OS_NAME}" == "Darwin" ]]; then
    JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
else
    JOBS="$(nproc 2>/dev/null || echo 4)"
fi
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

if [[ "${RUN_APP}" -eq 0 ]]; then
    echo "Build successful! (--no-run aktif)"
    exit 0
fi

echo "=================================================="
echo "Build successful! Launching Deskmon..."
echo "Logs will be printed below. Close Deskmon to exit."
echo "=================================================="

set +e
if [[ "${OS_NAME}" == "Darwin" ]]; then
    if [[ -n "${QT_DIR}" && -d "${QT_DIR}/lib" ]]; then
        export DYLD_FRAMEWORK_PATH="${QT_DIR}/lib:${DYLD_FRAMEWORK_PATH:-}"
    fi
    if [[ -n "${QT_DIR}" && -d "${QT_DIR}/plugins" ]]; then
        export QT_PLUGIN_PATH="${QT_DIR}/plugins:${QT_PLUGIN_PATH:-}"
    fi
    "${BUILD_DIR}/Deskmon.app/Contents/MacOS/Deskmon"
    APP_EXIT=$?
else
    if [[ -n "${QT_DIR}" && -d "${QT_DIR}/lib" ]]; then
        export LD_LIBRARY_PATH="${QT_DIR}/lib:${LD_LIBRARY_PATH:-}"
    fi
    if [[ -n "${QT_DIR}" && -d "${QT_DIR}/plugins" ]]; then
        export QT_PLUGIN_PATH="${QT_DIR}/plugins:${QT_PLUGIN_PATH:-}"
    fi
    "${BUILD_DIR}/Deskmon"
    APP_EXIT=$?
fi
set -e

echo "=================================================="
echo "Deskmon has closed (exit code: ${APP_EXIT})."
echo "=================================================="
