#!/usr/bin/env zsh
# ==============================================================================
# Deskmon macOS Build Script (Zsh / Bash Compatible)
# ==============================================================================
# Usage:
#   ./build.sh [OPTIONS]
#   zsh build.sh [OPTIONS]
#
# Options:
#   --debug        Build with Debug configuration (default: Release)
#   --clean        Clean build directory before configuring
#   --deploy       Bundle Qt frameworks and QML modules using macdeployqt
#   --dmg          Package into a standalone .dmg installer (implies --deploy)
#   --run          Launch Deskmon after a successful build
#   --help, -h     Show this help message
# ==============================================================================

# Error handling
set -eu
if [[ -n "${ZSH_VERSION:-}" ]]; then
    setopt pipefail 2>/dev/null || true
else
    set -o pipefail 2>/dev/null || true
fi

# ANSI Color codes
CLR_RESET="\033[0m"
CLR_BOLD="\033[1m"
CLR_GREEN="\033[32m"
CLR_YELLOW="\033[33m"
CLR_CYAN="\033[36m"
CLR_RED="\033[31m"

print_info()  { printf "%b\n" "${CLR_CYAN}$*${CLR_RESET}"; }
print_bold()  { printf "%b\n" "${CLR_BOLD}$*${CLR_RESET}"; }
print_succ()  { printf "%b\n" "${CLR_GREEN}$*${CLR_RESET}"; }
print_warn()  { printf "%b\n" "${CLR_YELLOW}$*${CLR_RESET}"; }
print_err()   { printf "%b\n" "${CLR_RED}$*${CLR_RESET}"; }

# Detect script directory (supports both Zsh and Bash)
if [[ -n "${ZSH_VERSION:-}" ]]; then
    SCRIPT_DIR="${0:A:h}"
elif [[ -n "${BASH_SOURCE[0]:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
else
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
fi
cd "$SCRIPT_DIR"

BUILD_TYPE="Release"
CLEAN_BUILD=false
DEPLOY_APP=false
CREATE_DMG=false
RUN_APP=false

# ------------------------------------------------------------------------------
# Parse Arguments
# ------------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --deploy)
            DEPLOY_APP=true
            shift
            ;;
        --dmg)
            DEPLOY_APP=true
            CREATE_DMG=true
            shift
            ;;
        --run)
            RUN_APP=true
            shift
            ;;
        -h|--help)
            print_bold "Deskmon Build Script for macOS (zsh)"
            echo ""
            echo "Usage: ./build.sh [OPTIONS]"
            echo "       zsh build.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --debug        Build Debug configuration (default: Release)"
            echo "  --clean        Clean build directory before compiling"
            echo "  --deploy       Run macdeployqt to bundle dependencies into Deskmon.app"
            echo "  --dmg          Create a distributable .dmg installer"
            echo "  --run          Launch Deskmon immediately after build"
            echo "  -h, --help     Show this help message"
            exit 0
            ;;
        *)
            print_warn "Unknown argument: $1"
            shift
            ;;
    esac
done

print_info "=================================================="
print_bold "Configuring environment for Deskmon build (macOS / zsh)"
print_info "=================================================="

# ------------------------------------------------------------------------------
# Auto-detect Qt 6 Installation
# ------------------------------------------------------------------------------
DETECTED_QT_DIR=""

if [[ -n "${QT_DIR:-}" ]] && [[ -d "${QT_DIR}" ]]; then
    DETECTED_QT_DIR="${QT_DIR}"
elif [[ -n "${CMAKE_PREFIX_PATH:-}" ]] && [[ -d "${CMAKE_PREFIX_PATH}" ]]; then
    DETECTED_QT_DIR="${CMAKE_PREFIX_PATH}"
fi

# Check standard Homebrew paths (Apple Silicon & Intel)
if [[ -z "$DETECTED_QT_DIR" ]]; then
    BREW_CANDIDATES=(
        "/opt/homebrew/opt/qt6"
        "/opt/homebrew/opt/qt"
        "/opt/homebrew/opt/qt@6"
        "/usr/local/opt/qt6"
        "/usr/local/opt/qt"
        "/usr/local/opt/qt@6"
    )
    for p in "${BREW_CANDIDATES[@]}"; do
        if [[ -d "$p" ]]; then
            DETECTED_QT_DIR="$p"
            break
        fi
    done
fi

# Check official Qt installer paths in ~/Qt
if [[ -z "$DETECTED_QT_DIR" ]]; then
    QT_INSTALL_ROOT="$HOME/Qt"
    if [[ -d "$QT_INSTALL_ROOT" ]]; then
        LATEST_MACOS_QT=$(find "$QT_INSTALL_ROOT" -maxdepth 3 -type d -path "*/6.*/macos" 2>/dev/null | sort -V | tail -n 1 || true)
        if [[ -n "$LATEST_MACOS_QT" ]] && [[ -d "$LATEST_MACOS_QT" ]]; then
            DETECTED_QT_DIR="$LATEST_MACOS_QT"
        fi
    fi
fi

# Check system PATH for qmake/qtpaths
if [[ -z "$DETECTED_QT_DIR" ]]; then
    if command -v qtpaths6 &>/dev/null; then
        DETECTED_QT_DIR="$(qtpaths6 --install-prefix 2>/dev/null || true)"
    elif command -v qtpaths &>/dev/null; then
        DETECTED_QT_DIR="$(qtpaths --install-prefix 2>/dev/null || true)"
    elif command -v qmake6 &>/dev/null; then
        DETECTED_QT_DIR="$(qmake6 -query QT_INSTALL_PREFIX 2>/dev/null || true)"
    fi
fi

if [[ -n "$DETECTED_QT_DIR" ]] && [[ -d "$DETECTED_QT_DIR" ]]; then
    export QT_DIR="$DETECTED_QT_DIR"
    export PATH="$QT_DIR/bin:$PATH"
    print_succ "✓ Qt Path detected: $QT_DIR"
else
    print_err "[ERROR] Qt 6 installation not found!"
    echo "Please ensure Qt 6 is installed via Homebrew or the official installer."
    echo "  - Via Homebrew: brew install qt6"
    echo "  - Or set environment variable: export QT_DIR=/path/to/Qt/6.x.x/macos"
    exit 1
fi

# ------------------------------------------------------------------------------
# Verify CMake
# ------------------------------------------------------------------------------
if ! command -v cmake &>/dev/null; then
    if [[ -x "/Applications/CMake.app/Contents/bin/cmake" ]]; then
        export PATH="/Applications/CMake.app/Contents/bin:$PATH"
    else
        print_err "[ERROR] CMake is not installed or not in PATH."
        echo "Install it via: brew install cmake"
        exit 1
    fi
fi
print_succ "✓ CMake Path: $(command -v cmake)"

# Generator check: prefer Ninja if available
CMAKE_GEN_ARGS=()
if command -v ninja &>/dev/null; then
    CMAKE_GEN_ARGS=(-G "Ninja")
    print_succ "✓ Generator: Ninja ($(command -v ninja))"
else
    print_warn "- Generator: Default Unix Makefiles (Ninja not found, recommended: brew install ninja)"
fi

print_succ "✓ Build Type: $BUILD_TYPE"
echo ""

# ------------------------------------------------------------------------------
# Icon Generation (if icon.icns is missing)
# ------------------------------------------------------------------------------
if [[ ! -f "icon.icns" ]] && [[ -f "icon.png" ]]; then
    print_info "Generating icon.icns from icon.png..."
    ICONSET_DIR="Deskmon.iconset"
    mkdir -p "$ICONSET_DIR"
    sips -z 16 16     icon.png --out "${ICONSET_DIR}/icon_16x16.png" >/dev/null 2>&1 || true
    sips -z 32 32     icon.png --out "${ICONSET_DIR}/icon_16x16@2x.png" >/dev/null 2>&1 || true
    sips -z 32 32     icon.png --out "${ICONSET_DIR}/icon_32x32.png" >/dev/null 2>&1 || true
    sips -z 64 64     icon.png --out "${ICONSET_DIR}/icon_32x32@2x.png" >/dev/null 2>&1 || true
    sips -z 128 128   icon.png --out "${ICONSET_DIR}/icon_128x128.png" >/dev/null 2>&1 || true
    sips -z 256 256   icon.png --out "${ICONSET_DIR}/icon_128x128@2x.png" >/dev/null 2>&1 || true
    sips -z 256 256   icon.png --out "${ICONSET_DIR}/icon_256x256.png" >/dev/null 2>&1 || true
    sips -z 512 512   icon.png --out "${ICONSET_DIR}/icon_256x256@2x.png" >/dev/null 2>&1 || true
    sips -z 512 512   icon.png --out "${ICONSET_DIR}/icon_512x512.png" >/dev/null 2>&1 || true
    sips -z 1024 1024 icon.png --out "${ICONSET_DIR}/icon_512x512@2x.png" >/dev/null 2>&1 || true
    if command -v iconutil &>/dev/null; then
        iconutil -c icns "$ICONSET_DIR" -o "icon.icns" || true
    fi
    rm -rf "$ICONSET_DIR"
    if [[ -f "icon.icns" ]]; then
        print_succ "✓ icon.icns successfully generated."
    fi
fi

# ------------------------------------------------------------------------------
# Build Directory Setup
# ------------------------------------------------------------------------------
BUILD_DIR="build"

if [[ "$CLEAN_BUILD" = true ]] && [[ -d "$BUILD_DIR" ]]; then
    print_warn "Cleaning build directory: $BUILD_DIR..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

# ------------------------------------------------------------------------------
# CMake Configuration
# ------------------------------------------------------------------------------
print_info "=================================================="
print_bold "Running CMake Configuration..."
print_info "=================================================="

cmake -B "$BUILD_DIR" \
    "${CMAKE_GEN_ARGS[@]}" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCONSOLE_LOGS=ON

# ------------------------------------------------------------------------------
# Compilation
# ------------------------------------------------------------------------------
print_info "=================================================="
print_bold "Compiling Deskmon..."
print_info "=================================================="

CPU_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -- -j"$CPU_CORES"

print_succ "=================================================="
print_succ "Build successful!"
print_succ "=================================================="
echo "App bundle: $BUILD_DIR/Deskmon.app"
echo ""

# ------------------------------------------------------------------------------
# Deployment / macdeployqt (Optional)
# ------------------------------------------------------------------------------
APP_PATH="$BUILD_DIR/Deskmon.app"

if [[ "$DEPLOY_APP" = true ]]; then
    if command -v macdeployqt &>/dev/null; then
        print_info "=================================================="
        print_bold "Deploying Qt libraries to Deskmon.app with macdeployqt..."
        print_info "=================================================="

        DEPLOY_FLAGS=("-qmldir=.")
        if [[ "$CREATE_DMG" = true ]]; then
            DEPLOY_FLAGS+=("-dmg")
        fi

        macdeployqt "$APP_PATH" "${DEPLOY_FLAGS[@]}"
        print_succ "✓ Deployment completed."
        if [[ "$CREATE_DMG" = true ]]; then
            print_succ "✓ DMG installer created in $BUILD_DIR/"
        fi
    else
        print_warn "[WARNING] macdeployqt not found. Skipping deployment."
    fi
fi

# ------------------------------------------------------------------------------
# Run Application
# ------------------------------------------------------------------------------
if [[ "$RUN_APP" = true ]]; then
    print_info "=================================================="
    print_bold "Launching Deskmon on macOS..."
    print_info "=================================================="
    "$APP_PATH/Contents/MacOS/Deskmon"
else
    echo "To run Deskmon:"
    printf "  %bopen %s%b\n" "$CLR_BOLD" "$APP_PATH" "$CLR_RESET"
    printf "  or directly: %b%s/Contents/MacOS/Deskmon%b\n" "$CLR_BOLD" "$APP_PATH" "$CLR_RESET"
    echo ""
    echo "To create a standalone DMG installer:"
    printf "  %b./build.sh --dmg%b\n" "$CLR_BOLD" "$CLR_RESET"
fi
