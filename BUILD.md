# Building OpenAuto for RK3229/Armbian (armhf)

This guide covers building OpenAuto for ARM hard-float (armhf) targets, specifically optimized for Rockchip RK3229 devices running Armbian.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Dependencies](#dependencies)
3. [Building AASDK](#building-aasdk)
4. [Building OpenAuto](#building-openauto)
5. [Cross-Compilation](#cross-compilation)
6. [Packaging](#packaging)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Target Hardware

- Rockchip RK3229 (or compatible: RK3328, RK3399)
- Armbian Linux (Bookworm recommended)
- HDMI display
- USB connection to Android phone

### Required Software

- CMake 3.14+
- GCC/G++ with C++17 support
- pkg-config
- Git

---

## Dependencies

### Install on Armbian (armhf)

```bash
# Update package lists
sudo apt update

# Core build tools
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    git

# Qt5 development libraries (including Qt Quick/QML for UI)
sudo apt install -y \
    qtbase5-dev \
    qtmultimedia5-dev \
    qtconnectivity5-dev \
    qtdeclarative5-dev \
    qtquickcontrols2-5-dev \
    libqt5multimedia5-plugins \
    qml-module-qtquick2 \
    qml-module-qtquick-controls2 \
    qml-module-qtquick-window2 \
    qml-module-qtquick-layouts \
    qml-module-qtgraphicaleffects

# FFmpeg development libraries (for video decoding)
sudo apt install -y \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev

# DRM/KMS libraries (for direct display output)
sudo apt install -y \
    libdrm-dev

# Audio libraries
sudo apt install -y \
    librtaudio-dev \
    libasound2-dev

# Bluetooth support
sudo apt install -y \
    bluez \
    libbluetooth-dev

# Other required libraries
sudo apt install -y \
    libusb-1.0-0-dev \
    libssl-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libboost-all-dev \
    libtag1-dev \
    libblkid-dev \
    libgps-dev

# Runtime: modetest for DRM debugging
sudo apt install -y libdrm-tests
```

### Verify FFmpeg DRM Support

```bash
# Check that FFmpeg has DRM hwaccel
ffmpeg -hwaccels 2>&1 | grep drm

# Should show: drm
```

### Verify DRM Device

```bash
# List DRM devices
ls -la /dev/dri/

# Check Rockchip driver
modetest -M rockchip -c
```

---

## Building AASDK

OpenAuto requires the Android Auto SDK (aasdk) library.

### Clone and Build aasdk

```bash
cd ~
git clone https://github.com/opencardev/aasdk.git
cd aasdk

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

### Clone and Build aap_protobuf

```bash
cd ~
git clone https://github.com/nicknakos/aap_protobuf.git
cd aap_protobuf

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

---

## Building OpenAuto

### Clone Repository

```bash
cd ~
git clone https://github.com/Harleythetech/openauto-rk3229-armbian.git
cd openauto-rk3229-armbian
```

### Build with build.sh (Recommended)

```bash
# Release build with DEB package
./build.sh release --package

# Debug build
./build.sh debug

# Clean build
./build.sh release --clean --package

# Auto-install missing dependencies
./build.sh release --auto-deps --package
```

### Manual CMake Build

```bash
mkdir build && cd build

cmake -DCMAKE_BUILD_TYPE=Release \
      -DNOPI=ON \
      -DUSE_FFMPEG_DRM=ON \
      ..

make -j$(nproc)
```

### CMake Options

| Option             | Default | Description                                      |
| ------------------ | ------- | ------------------------------------------------ |
| `NOPI`             | ON      | Build for non-Raspberry Pi (required for RK3229) |
| `USE_FFMPEG_DRM`   | ON      | Use FFmpeg with DRM hwaccel + DRM Prime output   |
| `CMAKE_BUILD_TYPE` | Release | Build type (Release/Debug)                       |

---

## Cross-Compilation

For building on x86_64 host for armhf target:

### Install ARM Toolchain

```bash
sudo apt install -y \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    crossbuild-essential-armhf
```

### Create Toolchain File

Create `armhf-toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

### Cross-Compile

```bash
mkdir build-armhf && cd build-armhf

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=../armhf-toolchain.cmake \
      -DNOPI=ON \
      -DUSE_FFMPEG_DRM=ON \
      ..

make -j$(nproc)
```

### Using Docker (Alternative)

```bash
# Build using Docker with Armbian rootfs
docker-compose -f buildenv/docker-compose.yml up
```

---

## Packaging

### Create DEB Package

```bash
cd build
cpack -G DEB
```

### Install Package

```bash
sudo dpkg -i openauto_*.deb
sudo ldconfig
```

### Package Contents

The DEB package installs:

- `/usr/local/bin/autoapp` - Main application
- `/usr/local/bin/btservice` - Bluetooth service
- `/opt/openauto/` - Configuration and scripts
- `/etc/systemd/system/openauto.service` - Systemd service

---

## Running

### Stop Display Manager

The DRM display requires exclusive access:

```bash
sudo systemctl stop lightdm
# or
sudo systemctl stop gdm3
```

### Run Manually

```bash
# Set environment
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0
export LD_LIBRARY_PATH=/usr/local/lib

# Run
/usr/local/bin/autoapp
```

### Run as Systemd Service

```bash
sudo systemctl enable openauto
sudo systemctl start openauto

# Check logs
journalctl -u openauto -f
```

---

## Troubleshooting

### Video Not Displaying

1. Check DRM plane availability:

   ```bash
   modetest -M rockchip -p
   ```

2. Verify connector:

   ```bash
   modetest -M rockchip -c
   ```

3. Check if display manager is holding DRM:
   ```bash
   sudo fuser /dev/dri/card0
   ```

### FFmpeg DRM Errors

1. Verify hwaccel support:

   ```bash
   ffmpeg -hwaccels
   ```

2. Test decoding:
   ```bash
   ffmpeg -hwaccel drm -hwaccel_output_format drm_prime \
          -i test.mp4 -f null -
   ```

### Audio Issues

1. List audio devices:

   ```bash
   aplay -l
   ```

2. Test playback:

   ```bash
   speaker-test -c2 -twav
   ```

3. Check ALSA config:
   ```bash
   cat /etc/asound.conf
   ```

### Build Errors

1. Missing dependencies:

   ```bash
   ./build.sh release --auto-deps
   ```

2. Clean rebuild:

   ```bash
   ./build.sh release --clean
   ```

3. Check pkg-config:
   ```bash
   pkg-config --libs libavcodec libdrm
   ```

### Permission Issues

```bash
# Add user to required groups
sudo usermod -aG video,audio,input,plugdev $USER

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

---

## Hardware-Specific Notes

### RK3229 VOP Configuration

- Primary plane: 31
- Connector: 46 (HDMI-A-1)
- Stride alignment: 64 bytes
- Color encoding: BT.709 (set automatically)

### Verify Color Encoding

```bash
modetest -M rockchip -p | grep -A 5 "COLOR_ENCODING"
# Should show: value: 1 (BT.709)
```

### Set Color Encoding Manually

```bash
modetest -M rockchip -w 31:COLOR_ENCODING:1
```

---

## Performance Optimization

### Disable Unused Services

```bash
sudo systemctl disable bluetooth  # If not using wireless AA
sudo systemctl disable cups
sudo systemctl disable avahi-daemon
```

### CPU Governor

```bash
# Set performance mode
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### Memory

Ensure at least 512MB RAM available. The video decoder uses CMA (Contiguous Memory Allocator).

---

## Version Information

Check build version:

```bash
autoapp --version
```

Version format: `YYYY.MM.DD+git.<commit>[.dirty][.debug]`

---

## License

GNU GPLv3 - See [LICENSE](LICENSE) for details.

## Credits

- Original [OpenAuto](https://github.com/opencardev/openauto) by f1x.studio
- [aasdk](https://github.com/opencardev/aasdk) library
- Armbian community for RK3229 support
