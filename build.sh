#!/bin/bash
# * Project: OpenAuto (RK3229/Armbian Custom Build)
# * This file is part of openauto project.
# * Copyright (C) 2025 OpenCarDev Team
# *
# *  openauto is free software: you can redistribute it and/or modify
# *  it under the terms of the GNU General Public License as published by
# *  the Free Software Foundation; either version 3 of the License, or
# *  (at your option) any later version.
# *
# *  openauto is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with openauto. If not, see <http://www.gnu.org/licenses/>.

set -e

# Script to build OpenAuto for RK3229/Armbian with KMS/DRM video output
# This custom build uses GStreamer kmssink for direct DRM plane rendering
# Usage: ./build.sh [release|debug] [--clean] [--package] [--output-dir DIR] [--no-kmssink]

# Default values
NOPI_FLAG="-DNOPI=ON"
USE_KMSSINK_FLAG="-DUSE_KMSSINK=ON"  # Enable KMS/DRM video output by default
AUTO_DEPS=false  # Auto-install dependencies without prompting
CLEAN_BUILD=false
PACKAGE=false
OUTPUT_DIR="/output"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="${SCRIPT_DIR}"

# Auto-detect build type based on git branch
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
if [ "$CURRENT_BRANCH" = "main" ] || [ "$CURRENT_BRANCH" = "master" ]; then
    BUILD_TYPE="release"
else
    BUILD_TYPE="debug"
fi

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        release|Release|RELEASE)
            BUILD_TYPE="release"
            shift
            ;;
        debug|Debug|DEBUG)
            BUILD_TYPE="debug"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --package)
            PACKAGE=true
            shift
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --no-kmssink)
            USE_KMSSINK_FLAG="-DUSE_KMSSINK=OFF"
            shift
            ;;
        --auto-deps)
            AUTO_DEPS=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [release|debug] [OPTIONS]"
            echo ""
            echo "OpenAuto build script for RK3229/Armbian with KMS/DRM video output"
            echo ""
            echo "Build types:"
            echo "  release        Build release version (default on main/master branch)"
            echo "  debug          Build debug version with symbols (default on other branches)"
            echo ""
            echo "Options:"
            echo "  --clean        Clean build directory before building"
            echo "  --package      Create DEB packages after building"
            echo "  --output-dir   Directory to copy packages (default: /output)"
            echo "  --no-kmssink   Disable KMS/DRM video output (use Qt video output instead)"
            echo "  --auto-deps    Automatically install missing dependencies without prompting"
            echo "  --help         Show this help message"
            echo ""
            echo "Target Platform: RK3229 (Rockchip) on Armbian"
            echo "Video Backend: GStreamer kmssink (KMS/DRM direct plane rendering)"
            echo ""
            echo "Required dependencies for KMS/DRM build:"
            echo "  - gstreamer1.0-plugins-base, gstreamer1.0-plugins-good"
            echo "  - gstreamer1.0-plugins-bad (for kmssink)"
            echo "  - gstreamer1.0-libav or gstreamer1.0-openh264 (for H.264 decoding)"
            echo "  - libdrm-dev (for DRM cursor support)"
            echo ""
            echo "Examples:"
            echo "  $0 release --package"
            echo "  $0 debug --clean"
            echo "  $0 release --package --output-dir ./packages"
            echo "  $0 release --no-kmssink    # Build without KMS/DRM (fallback to Qt)"
            echo "  $0 release --auto-deps     # Auto-install dependencies and build"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Determine build directory and CMake build type
if [ "$BUILD_TYPE" = "debug" ]; then
    BUILD_DIR="${SOURCE_DIR}/build-debug"
    CMAKE_BUILD_TYPE="Debug"
    CMAKE_CXX_FLAGS="-g3 -O0"
    echo "=== Building OpenAuto for RK3229/Armbian (Debug) ==="
else
    BUILD_DIR="${SOURCE_DIR}/build-release"
    CMAKE_BUILD_TYPE="Release"
    CMAKE_CXX_FLAGS=""
    echo "=== Building OpenAuto for RK3229/Armbian (Release) ==="
fi

echo "Source directory: ${SOURCE_DIR}"
echo "Build directory: ${BUILD_DIR}"
echo "Build type: ${CMAKE_BUILD_TYPE}"
echo "NOPI: ON (no Pi hardware dependencies)"
echo "USE_KMSSINK: $([ "$USE_KMSSINK_FLAG" = "-DUSE_KMSSINK=ON" ] && echo "ON (KMS/DRM video output)" || echo "OFF (Qt video output)")"
echo "Package: ${PACKAGE}"

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo ""
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
fi

# Create build directory
mkdir -p "${BUILD_DIR}"

# Detect architecture
TARGET_ARCH=$(dpkg-architecture -qDEB_HOST_ARCH 2>/dev/null || echo "amd64")
echo "Target architecture: ${TARGET_ARCH}"

# Compute distro-specific release suffix
if [ -f "${SOURCE_DIR}/scripts/distro_release.sh" ]; then
    DISTRO_DEB_RELEASE=$(bash "${SOURCE_DIR}/scripts/distro_release.sh")
    echo "Distro release suffix: ${DISTRO_DEB_RELEASE}"
else
    DISTRO_DEB_RELEASE=""
    echo "Warning: distro_release.sh not found, using default release suffix"
fi

# Check dependencies for KMS/DRM build
if [ "$USE_KMSSINK_FLAG" = "-DUSE_KMSSINK=ON" ]; then
    echo ""
    echo "Checking KMS/DRM dependencies..."
    MISSING_DEV_DEPS=""
    MISSING_RUNTIME_DEPS=""
    
    # Check for pkg-config
    if ! command -v pkg-config &> /dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} pkg-config"
    fi
    
    # Check for GStreamer development libraries
    if command -v pkg-config &> /dev/null; then
        if ! pkg-config --exists gstreamer-1.0 2>/dev/null; then
            MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libgstreamer1.0-dev"
        fi
        if ! pkg-config --exists gstreamer-app-1.0 2>/dev/null; then
            MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libgstreamer-plugins-base1.0-dev"
        fi
        # Check for libdrm
        if ! pkg-config --exists libdrm 2>/dev/null; then
            MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libdrm-dev"
        fi
    fi
    
    # Check for GStreamer runtime plugins (needed for kmssink and H.264 decoding)
    if command -v gst-inspect-1.0 &> /dev/null; then
        if ! gst-inspect-1.0 kmssink &>/dev/null; then
            MISSING_RUNTIME_DEPS="${MISSING_RUNTIME_DEPS} gstreamer1.0-plugins-bad"
        fi
        if ! gst-inspect-1.0 h264parse &>/dev/null; then
            MISSING_RUNTIME_DEPS="${MISSING_RUNTIME_DEPS} gstreamer1.0-plugins-bad"
        fi
        # Check for at least one H.264 decoder
        if ! gst-inspect-1.0 avdec_h264 &>/dev/null && ! gst-inspect-1.0 openh264dec &>/dev/null; then
            MISSING_RUNTIME_DEPS="${MISSING_RUNTIME_DEPS} gstreamer1.0-libav"
        fi
    else
        # gst-inspect not available, assume we need base plugins
        MISSING_RUNTIME_DEPS="${MISSING_RUNTIME_DEPS} gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-libav"
    fi
    
    # Remove duplicates from runtime deps
    MISSING_RUNTIME_DEPS=$(echo "$MISSING_RUNTIME_DEPS" | tr ' ' '\n' | sort -u | tr '\n' ' ')
    
    ALL_MISSING_DEPS="${MISSING_DEV_DEPS}${MISSING_RUNTIME_DEPS}"
    
    if [ -n "$ALL_MISSING_DEPS" ]; then
        echo "Missing dependencies detected:${ALL_MISSING_DEPS}"
        echo ""
        
        # Determine how to run apt-get (as root or with sudo)
        if [ "$(id -u)" -eq 0 ]; then
            # Running as root, no sudo needed
            APT_CMD="apt-get"
            echo "Running as root - will install directly."
        elif command -v sudo &> /dev/null; then
            APT_CMD="sudo apt-get"
            echo "Will use sudo to install packages."
        else
            APT_CMD=""
        fi
        
        if [ -n "$APT_CMD" ]; then
            INSTALL_DEPS=false
            if [ "$AUTO_DEPS" = true ]; then
                echo "Auto-installing dependencies (--auto-deps enabled)..."
                INSTALL_DEPS=true
            else
                echo "Would you like to install missing dependencies automatically?"
                read -p "Install dependencies? [Y/n] " -n 1 -r
                echo
                if [[ ! $REPLY =~ ^[Nn]$ ]]; then
                    INSTALL_DEPS=true
                fi
            fi
            
            if [ "$INSTALL_DEPS" = true ]; then
                echo "Installing dependencies with: $APT_CMD"
                $APT_CMD update
                $APT_CMD install -y ${ALL_MISSING_DEPS}
                
                # Also install additional recommended packages
                echo ""
                echo "Installing additional recommended GStreamer packages..."
                $APT_CMD install -y gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
                    gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav \
                    gstreamer1.0-tools libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
                    libdrm-dev 2>/dev/null || true
                
                echo ""
                echo "Dependencies installed successfully."
            else
                echo "Skipping dependency installation."
                echo "You may encounter build errors without these packages."
                read -p "Continue anyway? [y/N] " -n 1 -r
                echo
                if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                    exit 1
                fi
            fi
        else
            echo "sudo not available and not running as root. Please install manually:"
            echo "  sudo apt-get install${ALL_MISSING_DEPS}"
            echo ""
            echo "Also install GStreamer plugins:"
            echo "  sudo apt-get install gstreamer1.0-plugins-base gstreamer1.0-plugins-good \\"
            echo "                       gstreamer1.0-plugins-bad gstreamer1.0-libav"
            echo ""
            if [ "$AUTO_DEPS" = true ]; then
                echo "Cannot auto-install without sudo or root. Exiting."
                exit 1
            fi
            read -p "Continue anyway? [y/N] " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
        fi
    else
        echo "All KMS/DRM dependencies found."
    fi
fi

# Configure CMake
echo ""
echo "Configuring with CMake..."
CMAKE_ARGS=(
    -S "${SOURCE_DIR}"
    -B "${BUILD_DIR}"
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
)

if [ -n "$CMAKE_CXX_FLAGS" ]; then
    CMAKE_ARGS+=(-DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS}")
fi

if [ -n "$NOPI_FLAG" ]; then
    CMAKE_ARGS+=("${NOPI_FLAG}")
fi

if [ -n "$USE_KMSSINK_FLAG" ]; then
    CMAKE_ARGS+=("${USE_KMSSINK_FLAG}")
fi

if [ -n "$DISTRO_DEB_RELEASE" ]; then
    CMAKE_ARGS+=(-DCPACK_DEBIAN_PACKAGE_RELEASE="${DISTRO_DEB_RELEASE}")
fi

CMAKE_ARGS+=(-DCPACK_PROJECT_CONFIG_FILE="${SOURCE_DIR}/cmake_modules/CPackProjectConfig.cmake")

# Run CMake configuration
env DISTRO_DEB_RELEASE="${DISTRO_DEB_RELEASE}" cmake "${CMAKE_ARGS[@]}"

# Build
echo ""
echo "Building..."
NUM_CORES=$(nproc 2>/dev/null || echo 4)
cmake --build "${BUILD_DIR}" -j"${NUM_CORES}"

echo ""
echo "✓ Build completed successfully"

# Package if requested
if [ "$PACKAGE" = true ]; then
    echo ""
    echo "Creating packages..."
    cd "${BUILD_DIR}"
    cpack -G DEB
    cd "${SOURCE_DIR}"
    
    # Copy packages to output directory
    if [ -n "$OUTPUT_DIR" ] && [ "$OUTPUT_DIR" != "${BUILD_DIR}" ]; then
        echo ""
        echo "Copying packages to ${OUTPUT_DIR}..."
        mkdir -p "${OUTPUT_DIR}"
        find "${BUILD_DIR}" -name "*.deb" -exec cp -v {} "${OUTPUT_DIR}/" \;
        echo ""
        echo "Packages in ${OUTPUT_DIR}:"
        ls -lh "${OUTPUT_DIR}"/*.deb 2>/dev/null || echo "No packages found"
    else
        echo ""
        echo "Packages in ${BUILD_DIR}:"
        find "${BUILD_DIR}" -name "*.deb" -ls
    fi
fi

echo ""
echo "=== Build Summary ==="
echo "Target platform: RK3229/Armbian"
echo "Build type: ${CMAKE_BUILD_TYPE}"
echo "Build directory: ${BUILD_DIR}"
if [ "$USE_KMSSINK_FLAG" = "-DUSE_KMSSINK=ON" ]; then
    echo "Video output: KMS/DRM (GStreamer kmssink)"
    echo "  - Uses hardware H.264 decoding (v4l2slh264dec/v4l2h264dec)"
    echo "  - Direct DRM plane rendering (no compositor needed)"
else
    echo "Video output: Qt Multimedia"
fi
if [ -f "${BUILD_DIR}/bin/autoapp" ]; then
    echo "Binary: ${BUILD_DIR}/bin/autoapp"
elif [ -f "${BUILD_DIR}/autoapp" ]; then
    echo "Binary: ${BUILD_DIR}/autoapp"
fi
if [ -f "${BUILD_DIR}/bin/btservice" ]; then
    echo "Binary: ${BUILD_DIR}/bin/btservice"
elif [ -f "${BUILD_DIR}/btservice" ]; then
    echo "Binary: ${BUILD_DIR}/btservice"
fi

echo ""
echo "Done!"
