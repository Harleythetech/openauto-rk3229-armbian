# Use the official armhf Bookworm image for native binary compatibility
FROM --platform=linux/arm/v7 debian:bookworm

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# 1. Install prerequisites for adding secure repositories
RUN apt-get update && apt-get install -y \
    wget \
    gnupg \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# 2. Add the custom RK322x repository and GPG key
RUN wget -q http://apt.undo.it:7242/apt.undo.it.asc -O /etc/apt/trusted.gpg.d/apt.undo.it.asc && \
    echo "deb http://apt.undo.it:7242 bookworm main" | tee /etc/apt/sources.list.d/apt.undo.it.list

# 3. Set APT pinning to ensure the modified FFmpeg/MPV take precedence
RUN echo "Package: *\nPin: release o=apt.undo.it\nPin-Priority: 600" > /etc/apt/preferences.d/apt-undo-it

# 4. Install all dependencies, including the modified FFmpeg and MPV
# This also installs transitive dependencies like libdav1d6 and libx264 natively
RUN apt-get update && apt-get install -y \
    build-essential cmake pkg-config git \
    ffmpeg mpv \
    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libdrm-dev \
    qtbase5-dev qtmultimedia5-dev qtconnectivity5-dev \
    librtaudio-dev libtag1-dev libblkid-dev libgps-dev \
    libusb-1.0-0-dev libssl-dev libprotobuf-dev protobuf-compiler \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build