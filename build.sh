#!/usr/bin/env bash
# ==============================================================================
# Deskmon macOS Build Script
# ==============================================================================
# Usage:
#   ./build.sh [OPTIONS]
#
# Options:
#   --debug        Build with Debug configuration (default: Release)
#   --clean        Clean build directory before configuring
#   --deploy       Bundle Qt frameworks and QML modules using macdeployqt
#   --dmg          Package into a standalone .dmg installer (implies --deploy)
#   --run          Launch Deskmon after a successful build
#   --help, -h     Show this help message
# ==============================================================================

set -euo pipefail

# ANSI Color codes for output
CLR_RESET="\033[0m"
CLR_BOLD="\033[1m"
CLR_GREEN="\033[32m"
CLR_YELLOW="\033[33m"
CLR_CYAN="\033[36m"
CLR_RED="\033[31m"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
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
            echo -e "${CLR_BOLD}Deskmon Build Script for macOS${CLR_RESET}"
            echo ""
            echo "Usage: ./build.sh [OPTIONS]"
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
            echo -e "${CLR_YELLOW}Unknown argument: $1${CLR_RESET}"
            shift
            ;;
    esac
done

echo -e "${CLR_CYAN}==================================================${CLR_RESET}"
echo -e "${CLR_BOLD}Configuring environment for Deskmon build (macOS)${CLR_RESET}"
echo -e "${CLR_CYAN}==================================================${CLR_RESET}"

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
    for path in "${BREW_CANDIDATES[@]}"; do
        if [[ -d "$path" ]]; then
            DETECTED_QT_DIR="$path"
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
    echo -e "${CLR_GREEN}✓ Qt Path detected:${CLR_RESET} $QT_DIR"
else
    echo -e "${CLR_RED}[ERROR] Qt 6 installation not found!${CLR_RESET}"
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
        echo -e "${CLR_RED}[ERROR] CMake is not installed or not in PATH.${CLR_RESET}"
        echo "Install it via: brew install cmake"
        exit 1
    fi
fi
echo -e "${CLR_GREEN}✓ CMake Path:${CLR_RESET} $(command -v cmake)"

# Generator check: prefer Ninja if available
CMAKE_GEN=""
if command -v ninja &>/dev/null; then
    CMAKE_GEN="-G Ninja"
    echo -e "${CLR_GREEN}✓ Generator:${CLR_RESET} Ninja ($(command -v ninja))"
else
    echo -e "${CLR_YELLOW}- Generator: Default Unix Makefiles (Ninja not found, recommended: brew install ninja)${CLR_RESET}"
fi

echo -e "${CLR_GREEN}✓ Build Type:${CLR_RESET} $BUILD_TYPE"
echo ""

# ------------------------------------------------------------------------------
# Icon Generation (if icon.icns is missing)
# ------------------------------------------------------------------------------
if [[ ! -f "icon.icns" ]] && [[ -f "icon.png" ]]; then
    echo -e "${CLR_CYAN}Generating icon.icns from icon.png...${CLR_RESET}"
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
        echo -e "${CLR_GREEN}✓ icon.icns successfully generated.${CLR_RESET}"
    fi
fi

# ------------------------------------------------------------------------------
# Build Directory Setup
# ------------------------------------------------------------------------------
BUILD_DIR="build"

if [[ "$CLEAN_BUILD" = true ]] && [[ -d "$BUILD_DIR" ]]; then
    echo -e "${CLR_YELLOW}Cleaning build directory: $BUILD_DIR...${CLR_RESET}"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

# ------------------------------------------------------------------------------
# CMake Configuration
# ------------------------------------------------------------------------------
echo -e "${CLR_CYAN}==================================================${CLR_RESET}"
echo -e "${CLR_BOLD}Running CMake Configuration...${CLR_RESET}"
echo -e "${CLR_CYAN}==================================================${CLR_RESET}"

cmake -B "$BUILD_DIR" \
    $CMAKE_GEN \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCONSOLE_LOGS=ON

# ------------------------------------------------------------------------------
# Compilation
# ------------------------------------------------------------------------------
echo -e "${CLR_CYAN}==================================================${CLR_RESET}"
echo -e "${CLR_BOLD}Compiling Deskmon...${CLR_RESET}"
echo -e "${CLR_CYAN}==================================================${CLR_RESET}"

CPU_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -- -j"$CPU_CORES"

echo -e "${CLR_GREEN}==================================================${CLR_RESET}"
echo -e "${CLR_BOLD}${CLR_GREEN}Build successful!${CLR_RESET}"
echo -e "${CLR_GREEN}==================================================${CLR_RESET}"
echo "App bundle: $BUILD_DIR/Deskmon.app"
echo ""

# ------------------------------------------------------------------------------
# Deployment / macdeployqt (Optional)
# ------------------------------------------------------------------------------
APP_PATH="$BUILD_DIR/Deskmon.app"

if [[ "$DEPLOY_APP" = true ]]; then
    if command -v macdeployqt &>/dev/null; then
        echo -e "${CLR_CYAN}==================================================${CLR_RESET}"
        echo -e "${CLR_BOLD}Deploying Qt libraries to Deskmon.app with macdeployqt...${CLR_RESET}"
        echo -e "${CLR_CYAN}==================================================${CLR_RESET}"

        DEPLOY_FLAGS=("-qmldir=.")
        if [[ "$CREATE_DMG" = true ]]; then
            DEPLOY_FLAGS+=("-dmg")
        fi

        macdeployqt "$APP_PATH" "${DEPLOY_FLAGS[@]}"
        echo -e "${CLR_GREEN}✓ Deployment completed.${CLR_RESET}"
        if [[ "$CREATE_DMG" = true ]]; then
            echo -e "${CLR_GREEN}✓ DMG installer created in $BUILD_DIR/${CLR_RESET}"
        fi
    else
        echo -e "${CLR_YELLOW}[WARNING] macdeployqt not found. Skipping deployment.${CLR_RESET}"
    fi
fi

# ------------------------------------------------------------------------------
# Run Application
# ------------------------------------------------------------------------------
if [[ "$RUN_APP" = true ]]; then
    echo -e "${CLR_CYAN}==================================================${CLR_RESET}"
    echo -e "${CLR_BOLD}Launching Deskmon on macOS...${CLR_RESET}"
    echo -e "${CLR_CYAN}==================================================${CLR_RESET}"
    "$APP_PATH/Contents/MacOS/Deskmon"
else
    echo -e "To run Deskmon:"
    echo -e "  ${CLR_BOLD}open $APP_PATH${CLR_RESET}"
    echo -e "  or directly in terminal: ${CLR_BOLD}$APP_PATH/Contents/MacOS/Deskmon${CLR_RESET}"
    echo ""
    echo -e "To create a standalone DMG installer:"
    echo -e "  ${CLR_BOLD}./build.sh --dmg${CLR_RESET}"
fi
