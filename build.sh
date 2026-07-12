#!/usr/bin/env bash
# =============================================================================
# build.sh — Developer build script for Insurance Management System
#
# Usage (from MSYS2 bash):
#   ./build.sh            # Build using make (default)
#   ./build.sh cmake      # Build using CMake + Ninja/Make
#   ./build.sh clean      # Clean all build artefacts
#   ./build.sh run        # Build and run the application
#   ./build.sh help       # Show this help
#
# Environment:
#   Works in MSYS2 bash on Windows, and native bash on Linux/macOS.
#   Requires: gcc, make, sqlite3 dev headers (libsqlite3-dev or equivalent)
#
# Note:
#   On Windows, if bash fails with a "cygheap base mismatch" error, use
#   PowerShell instead: ./build.ps1
# =============================================================================

set -e          # Exit immediately on any error
set -u          # Treat unset variables as errors
set -o pipefail # Pipelines fail on first error

# ── Colour helpers ────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'   # No Colour

info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# ── Path setup for MSYS2 (harmless on Linux) ─────────────────────────────────
if [[ "$(uname -s)" == MSYS* ]] || [[ "$(uname -s)" == MINGW* ]]; then
    export PATH="/mingw64/bin:/usr/bin:${PATH}"
    IS_WINDOWS=1
else
    IS_WINDOWS=0
fi

# ── Project root = directory where this script lives ─────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

BUILD_DIR="build"
TARGET="insurance_system"

# ── Prerequisite checks ───────────────────────────────────────────────────────
check_prerequisites() {
    local missing=0

    info "Checking prerequisites..."

    for tool in gcc make; do
        if command -v "$tool" >/dev/null 2>&1; then
            success "$tool found: $(command -v "$tool")"
        else
            error "$tool not found. Please install it."
            missing=1
        fi
    done

    if pkg-config --libs sqlite3 >/dev/null 2>&1; then
        success "sqlite3 dev headers found: $(pkg-config --modversion sqlite3)"
    else
        error "sqlite3 dev headers not found. Install libsqlite3-dev (Linux) or mingw-w64-x86_64-sqlite3 (MSYS2)."
        missing=1
    fi

    if [[ $missing -ne 0 ]]; then
        error "One or more prerequisites are missing. Aborting."
        exit 1
    fi
    echo ""
}

# ── Make build ────────────────────────────────────────────────────────────────
build_make() {
    check_prerequisites
    info "Building with make..."
    echo ""
    make all
    echo ""
    success "Build complete → ${TARGET}"
}

# ── CMake build ───────────────────────────────────────────────────────────────
build_cmake() {
    check_prerequisites

    if ! command -v cmake >/dev/null 2>&1; then
        error "cmake not found."
        if [[ $IS_WINDOWS -eq 1 ]]; then
            warn "Install it with: pacman -S mingw-w64-x86_64-cmake"
        else
            warn "Install it with: sudo apt-get install cmake"
        fi
        exit 1
    fi

    info "Building with CMake..."
    info "Generator: $(cmake --version | head -1)"
    echo ""

    mkdir -p "${BUILD_DIR}"
    cmake -B "${BUILD_DIR}" -S . -DCMAKE_BUILD_TYPE=Debug
    cmake --build "${BUILD_DIR}"

    echo ""
    success "CMake build complete → ${BUILD_DIR}/${TARGET}"
}

# ── Clean ─────────────────────────────────────────────────────────────────────
clean() {
    info "Cleaning build artefacts..."
    make clean 2>/dev/null || true
    rm -rf "${BUILD_DIR}"
    rm -f src/*.o
    success "Clean complete."
}

# ── Run ───────────────────────────────────────────────────────────────────────
run_app() {
    build_make

    info "Running ${TARGET}..."
    echo ""
    echo "========================================"

    if [[ $IS_WINDOWS -eq 1 ]]; then
        ./${TARGET}.exe
    else
        ./${TARGET}
    fi
}

# ── Help ──────────────────────────────────────────────────────────────────────
show_help() {
    echo -e "${BOLD}Insurance Management System — Build Script${NC}"
    echo ""
    echo "Usage:"
    echo "  ./build.sh            Build with make (default)"
    echo "  ./build.sh cmake      Build with CMake"
    echo "  ./build.sh clean      Remove all build artefacts"
    echo "  ./build.sh run        Build and run the application"
    echo "  ./build.sh help       Show this help message"
    echo ""
    echo "Environment Requirements:"
    echo "  MSYS2/Windows : gcc, make, pkg-config — all in /mingw64/bin"
    echo "  Linux/macOS   : build-essential, libsqlite3-dev, cmake"
    echo ""
    echo "MSYS2 Quick Setup (if tools are missing):"
    echo "  pacman -S mingw-w64-x86_64-gcc"
    echo "  pacman -S mingw-w64-x86_64-make"
    echo "  pacman -S mingw-w64-x86_64-sqlite3"
    echo "  pacman -S mingw-w64-x86_64-cmake"
    echo ""
    echo "Linux Quick Setup:"
    echo "  sudo apt-get install build-essential libsqlite3-dev cmake"
}

# ── Entry point ───────────────────────────────────────────────────────────────
ACTION="${1:-make}"

echo -e "${BOLD}"
echo "╔══════════════════════════════════════════╗"
echo "║   Insurance Management System            ║"
echo "║   Build Script v1.0                      ║"
echo "╚══════════════════════════════════════════╝"
echo -e "${NC}"

case "${ACTION}" in
    make)   build_make   ;;
    cmake)  build_cmake  ;;
    clean)  clean        ;;
    run)    run_app      ;;
    help)   show_help    ;;
    *)
        error "Unknown action: '${ACTION}'"
        show_help
        exit 1
        ;;
esac
