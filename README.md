# OpenAuto for RK3229/Rockchip

Fork of OpenAuto optimized for Rockchip RK3229 (and similar) ARM devices running Armbian.

### Description

OpenAuto is an AndroidAuto(tm) headunit emulator based on aasdk library and Qt libraries. This fork is specifically optimized for Rockchip SoCs with hardware video decoding via V4L2/KMS.

### Features

- **Hardware H.264 decoding** via V4L2 stateless decoder (rkvdec)
- **KMS/DRM video output** - direct rendering without compositor overhead
- **ALSA audio** - no PulseAudio dependency, lower latency (requires modification of /etc/asound.conf)
- 480p, 720p 30 FPS (720p 60, 1080p 30/60 has too much latency)
- Audio playback from all audio channels (Media, System and Speech)
- Touchscreen input
- Bluetooth support
- Automatic launch after device hotplug
- Automatic detection of connected Android devices
- Wireless (WiFi) mode via head unit server

### Supported Hardware

- **Rockchip RK3229** (primary target)
- Other Rockchip SoCs with rkvdec (RK3328, RK3399, etc.)
- Any ARM device with V4L2 stateless H.264 decoder and KMS/DRM support

### Requirements

- Armbian or similar Linux distribution
- GStreamer 1.14+ with:
  - `gstreamer1.0-plugins-bad` (for kmssink)
  - V4L2 stateless decoder support
- Qt5 with Multimedia widgets
- ALSA (libasound2)
- libdrm

### Getting Started

#### 1. Download Armbian

Download the Armbian image for RK322x-box:

- [Armbian RK322x-box Archive](https://armbian.hosthatch.com/archive/rk322x-box/archive/)

#### 2. Enable V4L2 Hardware Decoding

The V4L2 stateless decoder requires additional setup. Follow this guide to enable hardware video decoding:

- [V4L2-request Hardware Video Decoding for Rockchip/Allwinner](https://forum.armbian.com/topic/32449-repository-for-v4l2request-hardware-video-decoding-rockchip-allwinner/)

This is required for hardware video decoding to work properly.

### Building

For detailed build instructions including dependencies and cross-compilation, see **[BUILD.md](BUILD.md)**.

#### Quick Start (Native Build on Device)

```bash
# Install dependencies and build
./build.sh release --auto-deps --package
```

#### Cross-Compilation

```bash
# Set up ARM toolchain
export CROSS_COMPILE=arm-linux-gnueabihf-

# Build with FFmpeg DRM video output (default)
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DNOPI=ON \
      -DUSE_FFMPEG_DRM=ON \
      ..
make -j$(nproc)
make package
```

### ALSA Configuration

Create `/etc/asound.conf` for audio output (HDMI example):

```
pcm.!default {
    type asym
    playback.pcm "plug_dmix_hdmi"
    capture.pcm "plug_null"
}

ctl.!default {
    type hw
    card 0
}

pcm.plug_dmix_hdmi {
    type plug
    slave.pcm "dmix_hdmi"
}

pcm.plug_null {
    type plug
    slave.pcm "null"
}

pcm.dmix_hdmi {
    type dmix
    ipc_key 1024
    ipc_perm 0666
    slave {
        pcm {
            type hw
            card 0
            device 0
        }
        format S16_LE
        rate 48000
        channels 2
        period_size 512
        buffer_size 4096
    }
    bindings {
        0 0
        1 1
    }
}
```

### Running

```bash
# Stop display manager to free DRM resources
sudo systemctl stop lightdm

# Run autoapp
./autoapp
```

### DRM/KMS Configuration

The default configuration uses:

- Connector 46 (HDMI-A-1)
- Plane 31 (Primary plane)

To find your device's configuration:

```bash
modetest -M rockchip -c  # List connectors
modetest -M rockchip -p  # List planes
```

### License

GNU GPLv3

_AndroidAuto is registered trademark of Google Inc._

### Credits

- Original [OpenAuto](https://github.com/opencardev/openauto) by f1x.studio
- [aasdk](https://github.com/opencardev/aasdk) library

### Disclaimer

**This software is not certified by Google Inc. It is created for R&D purposes and may not work as expected. Do not use while driving. You use this software at your own risk.**
