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

# Script to build OpenAuto for RK3229/Armbian with FFmpeg DRM video output
# This custom build uses FFmpeg DRM hwaccel + DRM Prime for low-latency video
# Usage: ./build.sh [release|debug] [--clean] [--package] [--output-dir DIR]

# Default values
NOPI_FLAG="-DNOPI=ON"
USE_FFMPEG_DRM_FLAG="-DUSE_FFMPEG_DRM=ON"  # Enable FFmpeg DRM video output by default
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
        --auto-deps)
            AUTO_DEPS=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [release|debug] [OPTIONS]"
            echo ""
            echo "OpenAuto build script for RK3229/Armbian with FFmpeg DRM video output"
            echo ""
            echo "Build types:"
            echo "  release        Build release version (default on main/master branch)"
            echo "  debug          Build debug version with symbols (default on other branches)"
            echo ""
            echo "Options:"
            echo "  --clean        Clean build directory before building"
            echo "  --package      Create DEB packages after building"
            echo "  --output-dir   Directory to copy packages (default: /output)"
            echo "  --auto-deps    Automatically install missing dependencies without prompting"
            echo "  --help         Show this help message"
            echo ""
            echo "Target Platform: RK3229 (Rockchip) on Armbian"
            echo "Video Backend: FFmpeg DRM hwaccel + DRM Prime (lowest latency)"
            echo ""
            echo "Required dependencies (auto-installed with --auto-deps):"
            echo "  Qt5:       qtbase5-dev, qtmultimedia5-dev, qtconnectivity5-dev"
            echo "  Bluetooth: bluez, libbluetooth-dev"
            echo "  Audio:     librtaudio-dev"
            echo "  FFmpeg:    libavcodec-dev, libavformat-dev, libavutil-dev, libswscale-dev"
            echo "  DRM:       libdrm-dev"
            echo "  Other:     libtag1-dev, libblkid-dev, libgps-dev, libusb-1.0-0-dev,"
            echo "             libssl-dev, libprotobuf-dev, libboost-all-dev"
            echo ""
            echo "Examples:"
            echo "  $0 release --package"
            echo "  $0 debug --clean"
            echo "  $0 release --package --output-dir ./packages"
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
echo "USE_FFMPEG_DRM: ON (FFmpeg DRM hwaccel + DRM Prime output)"
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

# Check build dependencies
echo ""
echo "Checking build dependencies..."
MISSING_DEV_DEPS=""
MISSING_RUNTIME_DEPS=""

# Check for pkg-config
if ! command -v pkg-config &> /dev/null; then
    MISSING_DEV_DEPS="${MISSING_DEV_DEPS} pkg-config"
fi

# Check for Qt5 development libraries
if command -v pkg-config &> /dev/null; then
    if ! pkg-config --exists Qt5Core 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} qtbase5-dev qtmultimedia5-dev libqt5multimedia5-plugins"
    fi
    if ! pkg-config --exists Qt5Multimedia 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} qtmultimedia5-dev"
    fi
    if ! pkg-config --exists Qt5Bluetooth 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} qtconnectivity5-dev"
    fi
    # Check for bluez/bluetooth
    if ! pkg-config --exists bluez 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} bluez libbluetooth-dev"
    fi
    if ! pkg-config --exists Qt5DBus 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libqt5dbus5"
    fi
else
    # pkg-config not available, assume Qt5 packages needed
    MISSING_DEV_DEPS="${MISSING_DEV_DEPS} qtbase5-dev qtmultimedia5-dev libqt5multimedia5-plugins qtconnectivity5-dev bluez libbluetooth-dev"
fi

# Check for rtaudio
if command -v pkg-config &> /dev/null; then
    if ! pkg-config --exists rtaudio 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} librtaudio-dev"
    fi
else
    MISSING_DEV_DEPS="${MISSING_DEV_DEPS} librtaudio-dev"
fi

# Check for other common build dependencies
if command -v pkg-config &> /dev/null; then
    if ! pkg-config --exists taglib 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libtag1-dev"
    fi
    if ! pkg-config --exists blkid 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libblkid-dev"
    fi
    if ! pkg-config --exists libgps 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libgps-dev"
    fi
    if ! pkg-config --exists libusb-1.0 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libusb-1.0-0-dev"
    fi
    if ! pkg-config --exists openssl 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libssl-dev"
    fi
    if ! pkg-config --exists protobuf 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libprotobuf-dev protobuf-compiler"
    fi
fi

# Check for FFmpeg/DRM dependencies (needed for FFmpeg DRM build)
if command -v pkg-config &> /dev/null; then
    if ! pkg-config --exists libavcodec 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libavcodec-dev"
    fi
    if ! pkg-config --exists libavformat 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libavformat-dev"
    fi
    if ! pkg-config --exists libavutil 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libavutil-dev"
    fi
    if ! pkg-config --exists libswscale 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libswscale-dev"
    fi
    # Check for libdrm
    if ! pkg-config --exists libdrm 2>/dev/null; then
        MISSING_DEV_DEPS="${MISSING_DEV_DEPS} libdrm-dev"
    fi
fi

# Remove duplicates from deps
MISSING_DEV_DEPS=$(echo "$MISSING_DEV_DEPS" | tr ' ' '\n' | sort -u | tr '\n' ' ')
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
            echo "Installing additional recommended packages..."
            # Qt5 packages
            $APT_CMD install -y qtbase5-dev qtmultimedia5-dev libqt5multimedia5-plugins \
                qtconnectivity5-dev 2>/dev/null || true
            # Bluetooth packages
            $APT_CMD install -y bluez libbluetooth-dev 2>/dev/null || true
            # GStreamer packages
            $APT_CMD install -y gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
                gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav \
                gstreamer1.0-tools libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
                libdrm-dev 2>/dev/null || true
            # Other build dependencies
            $APT_CMD install -y librtaudio-dev libtag1-dev libblkid-dev libgps-dev \
                libusb-1.0-0-dev libssl-dev libprotobuf-dev protobuf-compiler \
                libboost-all-dev cmake build-essential 2>/dev/null || true
            
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
        echo "Full dependency list:"
        echo "  sudo apt-get install qtbase5-dev qtmultimedia5-dev libqt5multimedia5-plugins \\"
        echo "      qtconnectivity5-dev bluez libbluetooth-dev librtaudio-dev libtag1-dev libblkid-dev libgps-dev \\"
        echo "      libusb-1.0-0-dev libssl-dev libprotobuf-dev protobuf-compiler \\"
        echo "      libboost-all-dev cmake build-essential \\"
        echo "      gstreamer1.0-plugins-base gstreamer1.0-plugins-good \\"
        echo "      gstreamer1.0-plugins-bad gstreamer1.0-libav \\"
        echo "      libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libdrm-dev"
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
    echo "All build dependencies found."
fi

# Copy launcher script and dependencies if they exist (Docker build environment)
# The script at /root/openauto.sh configures linuxfb, kmssink, and RTAudio
if [ -f "/root/openauto.sh" ]; then
    echo ""
    echo "Copying launcher script from /root/openauto.sh..."
    mkdir -p "${SOURCE_DIR}/packaging/opt/openauto"
    cp -v "/root/openauto.sh" "${SOURCE_DIR}/packaging/opt/openauto/openauto.sh"
    chmod +x "${SOURCE_DIR}/packaging/opt/openauto/openauto.sh"
fi

# Copy custom libraries from /root/deps/ if present
# These include aasdk, aap_protobuf, and other dependencies
if [ -d "/root/deps" ]; then
    echo ""
    echo "Copying custom libraries from /root/deps/..."
    mkdir -p "${SOURCE_DIR}/deps/lib"
    if [ -d "/root/deps/lib" ]; then
        cp -rv /root/deps/lib/* "${SOURCE_DIR}/deps/lib/" 2>/dev/null || true
    fi
    # Also copy any .so files directly in /root/deps/
    find /root/deps -maxdepth 1 -name "*.so*" -exec cp -v {} "${SOURCE_DIR}/deps/lib/" \; 2>/dev/null || true
fi

# Copy libaasdk and libaap_protobuf from /usr/local/lib if present
if [ -d "/usr/local/lib" ]; then
    echo ""
    echo "Copying aasdk and aap_protobuf libraries..."
    mkdir -p "${SOURCE_DIR}/deps/lib"
    cp -v /usr/local/lib/libaasdk*.so* "${SOURCE_DIR}/deps/lib/" 2>/dev/null || true
    cp -v /usr/local/lib/libaap_protobuf*.so* "${SOURCE_DIR}/deps/lib/" 2>/dev/null || true
fi

# =============================================================================
# RK322x Sysroot Setup - FFmpeg 5.1 with V4L2 Request API
# =============================================================================
# This section downloads and extracts the high-performance FFmpeg 5.1 libraries
# with V4L2 Request API support for RK3229 hardware video decoding.
# The libraries are installed into an isolated sysroot to avoid breaking the
# host system's FFmpeg installation.

RK322X_SYSROOT="/opt/rk322x-sysroot"
FFMPEG_DEB_URL="http://apt.undo.it:7242/pool/main/f/ffmpeg"

# List of FFmpeg 5.1 packages to download
FFMPEG_DEBS=(
    "libavcodec-dev_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libavcodec59_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libavdevice-dev_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libavdevice59_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libavfilter-dev_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libavfilter8_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libavformat-dev_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libavformat59_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libavutil-dev_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libavutil57_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libswresample-dev_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libswresample4_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libswscale-dev_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libswscale6_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libpostproc-dev_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
    "libpostproc56_5.1.8-0+deb12u1-v4l2request1_armhf.deb"
)

setup_rk322x_sysroot() {
    echo ""
    echo "=== Setting up RK322x FFmpeg 5.1 Sysroot ==="
    echo "Sysroot directory: ${RK322X_SYSROOT}"
    
    # Check if sysroot already exists and has the libraries
    if [ -d "${RK322X_SYSROOT}/usr/lib/arm-linux-gnueabihf" ] && \
       [ -f "${RK322X_SYSROOT}/usr/lib/arm-linux-gnueabihf/libavcodec.so" ]; then
        echo "RK322x sysroot already exists, skipping download."
        return 0
    fi
    
    # Determine how to create the sysroot directory (may need sudo)
    if [ "$(id -u)" -eq 0 ]; then
        SYSROOT_CMD=""
    elif command -v sudo &> /dev/null; then
        SYSROOT_CMD="sudo"
    else
        echo "Warning: Cannot create ${RK322X_SYSROOT} without sudo or root."
        echo "Please create it manually: sudo mkdir -p ${RK322X_SYSROOT}"
        return 1
    fi
    
    # Create sysroot directory
    echo "Creating sysroot directory..."
    ${SYSROOT_CMD} mkdir -p "${RK322X_SYSROOT}"
    
    # Create temporary directory for downloads
    TEMP_DEB_DIR=$(mktemp -d)
    echo "Downloading FFmpeg 5.1 packages to ${TEMP_DEB_DIR}..."
    
    # Download all FFmpeg packages
    for deb in "${FFMPEG_DEBS[@]}"; do
        echo "  Downloading ${deb}..."
        if ! wget -q --show-progress -P "${TEMP_DEB_DIR}" "${FFMPEG_DEB_URL}/${deb}"; then
            echo "Warning: Failed to download ${deb}"
            # Try without --show-progress for older wget versions
            wget -P "${TEMP_DEB_DIR}" "${FFMPEG_DEB_URL}/${deb}" || {
                echo "Error: Failed to download ${deb}"
                rm -rf "${TEMP_DEB_DIR}"
                return 1
            }
        fi
    done
    
    # Extract all packages into the sysroot
    echo ""
    echo "Extracting packages into ${RK322X_SYSROOT}..."
    for deb in "${FFMPEG_DEBS[@]}"; do
        echo "  Extracting ${deb}..."
        ${SYSROOT_CMD} dpkg-deb -x "${TEMP_DEB_DIR}/${deb}" "${RK322X_SYSROOT}"
    done
    
    # Clean up temporary directory
    rm -rf "${TEMP_DEB_DIR}"
    
    echo ""
    echo "RK322x sysroot setup complete."
    echo "  Include path: ${RK322X_SYSROOT}/usr/include/arm-linux-gnueabihf"
    echo "  Library path: ${RK322X_SYSROOT}/usr/lib/arm-linux-gnueabihf"
    
    return 0
}

# Set up the sysroot
if ! setup_rk322x_sysroot; then
    echo ""
    echo "Warning: Failed to set up RK322x sysroot. Build may fail or use system FFmpeg."
    echo "You can manually set up the sysroot by running:"
    echo "  sudo mkdir -p ${RK322X_SYSROOT}"
    echo "  # Download and extract FFmpeg debs from ${FFMPEG_DEB_URL}"
    RK322X_SYSROOT_AVAILABLE=false
else
    RK322X_SYSROOT_AVAILABLE=true
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

if [ -n "$USE_FFMPEG_DRM_FLAG" ]; then
    CMAKE_ARGS+=("${USE_FFMPEG_DRM_FLAG}")
fi

if [ -n "$DISTRO_DEB_RELEASE" ]; then
    CMAKE_ARGS+=(-DCPACK_DEBIAN_PACKAGE_RELEASE="${DISTRO_DEB_RELEASE}")
fi

CMAKE_ARGS+=(-DCPACK_PROJECT_CONFIG_FILE="${SOURCE_DIR}/cmake_modules/CPackProjectConfig.cmake")

# Add RK322x sysroot paths if available
if [ "$RK322X_SYSROOT_AVAILABLE" = true ] && [ -d "${RK322X_SYSROOT}/usr" ]; then
    echo ""
    echo "Using RK322x sysroot for FFmpeg 5.1 libraries"
    CMAKE_ARGS+=(
        -DCMAKE_PREFIX_PATH="${RK322X_SYSROOT}/usr"
        -DCMAKE_INCLUDE_PATH="${RK322X_SYSROOT}/usr/include/arm-linux-gnueabihf"
        -DCMAKE_LIBRARY_PATH="${RK322X_SYSROOT}/usr/lib/arm-linux-gnueabihf"
    )
    
    # Set package dependencies for FFmpeg 5.1 runtime libraries
    # Disable automatic shlibdeps to prevent Ubuntu's dpkg-shlibdeps from
    # trying to find the old system FFmpeg libraries
    CMAKE_ARGS+=(
        -DCPACK_DEBIAN_PACKAGE_DEPENDS="libavcodec59, libavutil57, libswscale6, libdrm2, libqt5gui5"
        -DCPACK_DEBIAN_PACKAGE_SHLIBDEPS=OFF
    )
    
    # Also set PKG_CONFIG_PATH to find the sysroot's .pc files
    export PKG_CONFIG_PATH="${RK322X_SYSROOT}/usr/lib/arm-linux-gnueabihf/pkgconfig:${PKG_CONFIG_PATH}"
fi

# Run CMake configuration
# Run CMake configuration with RK322X_SYSROOT environment variable
export RK322X_SYSROOT="${RK322X_SYSROOT}"
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
echo "Video output: FFmpeg DRM hwaccel + DRM Prime (lowest latency)"
echo "  - Uses hardware H.264 decoding via DRM hwaccel"
echo "  - Zero-copy DRM Prime output (no compositor needed)"
if [ "$RK322X_SYSROOT_AVAILABLE" = true ]; then
    echo "FFmpeg sysroot: ${RK322X_SYSROOT} (FFmpeg 5.1 with V4L2 Request API)"
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
