/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * FFmpegDrmVideoOutput.hpp
 *
 * Ultra-low latency video output using FFmpeg with DRM hardware
 * acceleration and DRM/KMS direct display output via drmprime.
 *
 * This backend is specifically designed for Rockchip RK3229 running
 * jock's Armbian build with DRM hwaccel support.
 *
 * IMPORTANT: On RK3229 with jock's FFmpeg:
 * - Supported hwaccels: drm, vaapi, vdpau, opencl, vulkan (NOT v4l2m2m)
 * - Use "-hwaccel drm -hwaccel_output_format drm_prime"
 * - v4l2_request probe errors during init are benign
 * - FFmpeg's DRM framework negotiates with rkvdec VPU
 *
 * Pipeline: H.264 data -> FFmpeg h264 (DRM hwaccel) -> DRM Prime -> KMS display
 *
 * Key Features:
 * - Uses native h264 decoder with DRM hardware context
 * - Hardware accelerated decoding via DRM hwaccel framework
 * - Zero-copy display path using DRM Prime (DMABUF)
 * - Direct KMS/DRM plane output without compositor overhead
 * - Minimal buffering for lowest possible latency
 *
 * Requirements:
 * - FFmpeg with DRM hwaccel support (jock's APT repo for Armbian)
 * - Linux kernel with rkvdec decoder support
 * - libdrm for DRM/KMS display
 *
 * Tested: 248 FPS (4.18x speed) on 720p60 H.264 stream
 *
 * Compile with: -DUSE_FFMPEG_DRM to enable this backend
 */

#pragma once

#ifdef USE_FFMPEG_DRM

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <f1x/openauto/autoapp/Projection/VideoOutput.hpp>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace f1x {
namespace openauto {
namespace autoapp {
namespace projection {

/**
 * @class FFmpegDrmVideoOutput
 * @brief Ultra-low latency video output using FFmpeg DRM hwaccel + DRM Prime.
 *
 * This class provides the lowest possible latency video decoding and display
 * for embedded Linux systems using FFmpeg's DRM hardware acceleration
 * framework.
 *
 * Features:
 * - Hardware accelerated H.264 decoding via DRM hwaccel (rkvdec on RK3229)
 * - Zero-copy DRM Prime path to KMS display
 * - Minimal internal buffering (decode-on-demand)
 * - DRM hardware cursor support
 * - Thread-safe frame submission
 */
class FFmpegDrmVideoOutput : public VideoOutput {
public:
  /**
   * @brief Constructs the FFmpegDrmVideoOutput with the given configuration.
   * @param configuration Pointer to the OpenAuto configuration interface.
   */
  explicit FFmpegDrmVideoOutput(
      configuration::IConfiguration::Pointer configuration);

  /**
   * @brief Destructor - ensures proper cleanup of FFmpeg and DRM resources.
   */
  ~FFmpegDrmVideoOutput() override;

  // Prevent copying
  FFmpegDrmVideoOutput(const FFmpegDrmVideoOutput &) = delete;
  FFmpegDrmVideoOutput &operator=(const FFmpegDrmVideoOutput &) = delete;

  /**
   * @brief Initializes FFmpeg decoder and DRM display.
   * @return true if initialization succeeded, false otherwise.
   */
  bool open() override;

  /**
   * @brief Starts the video output pipeline.
   * @return true if pipeline started successfully, false otherwise.
   */
  bool init() override;

  /**
   * @brief Writes an H.264 video frame for decoding and display.
   * @param timestamp The presentation timestamp in nanoseconds.
   * @param buffer The raw H.264 frame data to be decoded and displayed.
   */
  void write(uint64_t timestamp,
             const aasdk::common::DataConstBuffer &buffer) override;

  /**
   * @brief Stops the pipeline and releases all resources.
   */
  void stop() override;

  /**
   * @brief Updates the hardware cursor position.
   * @param x X coordinate in screen pixels.
   * @param y Y coordinate in screen pixels.
   */
  static void updateCursorPosition(int x, int y);

  /**
   * @brief Shows or hides the hardware cursor.
   * @param visible true to show cursor, false to hide.
   */
  static void setCursorVisible(bool visible);

  /**
   * @brief Emergency cleanup for signal handlers.
   * Called on SIGINT/SIGTERM to release DRM resources and prevent CMA leaks.
   * This method is safe to call from a signal handler context.
   */
  void emergencyCleanup();

private:
  /**
   * @brief Initializes the FFmpeg decoder with DRM hwaccel.
   * @return true if decoder initialized successfully.
   */
  bool initDecoder();

  /**
   * @brief Initializes the DRM display for direct output.
   * @return true if DRM initialized successfully.
   */
  bool initDrmDisplay();

  /**
   * @brief Displays a decoded frame via DRM.
   * @param frame The decoded AVFrame with DRM Prime data.
   * @return true if frame displayed successfully.
   */
  bool displayFrame(AVFrame *frame);

  /**
   * @brief Cleans up FFmpeg resources.
   */
  void cleanupDecoder();

  /**
   * @brief Cleans up DRM resources.
   */
  void cleanupDrm();

  /**
   * @brief Sets up BT.709 color encoding on the DRM plane.
   * Prevents the 'purple tint' issue common with Rockchip VOP.
   */
  void setupColorEncoding();

  /**
   * @brief Gets video width based on configured resolution.
   * @return Video width in pixels.
   */
  int getVideoWidth() const;

  /**
   * @brief Gets video height based on configured resolution.
   * @return Video height in pixels.
   */
  int getVideoHeight() const;

  /**
   * @brief Initializes the DRM hardware cursor.
   * @return true if cursor initialized successfully.
   */
  bool initCursor();

  /**
   * @brief Cleans up cursor resources.
   */
  void cleanupCursor();

  /**
   * @brief Waits for VSync/page flip completion.
   * Ensures the previous frame is no longer in use before releasing its buffer.
   */
  void waitForPageFlip();

  /**
   * @brief Converts software-decoded frame to displayable format.
   * @param frame Source frame (YUV420P or similar)
   * @return true if conversion and display succeeded
   */
  bool displaySoftwareFrame(AVFrame *frame);

  // Thread synchronization
  std::mutex mutex_;

  // Pipeline state
  std::atomic<bool> isActive_;
  uint64_t frameCount_;
  uint64_t droppedFrames_; // Track frames dropped due to decoder lag

  // FFmpeg decoder state
  const AVCodec *codec_;
  AVCodecContext *codecCtx_;
  AVCodecParserContext *parser_;
  AVPacket *packet_;
  AVFrame *frame_;
  AVBufferRef *hwDeviceCtx_;

  // Buffer pooling: Keep reference to displayed frames until DRM is done
  // This prevents freeing DRM Prime buffers while still in use by display
  AVFrame *displayedFrame_;         // Currently being displayed
  AVFrame *previousDisplayedFrame_; // Previous frame (being released)

  // Software fallback state
  struct SwsContext *swsCtx_; // For YUV->RGB conversion if needed
  uint32_t swDumbHandle_;     // Dumb buffer handle for SW frames
  uint32_t swDumbFbId_;       // Framebuffer ID for SW frames
  void *swDumbMap_;           // Mapped memory for SW frames
  size_t swDumbSize_;         // Size of mapped memory
  uint32_t swDumbPitch_;      // Stride of dumb buffer (64-byte aligned)

  // DRM display state
  int drmFd_;
  uint32_t connectorId_;
  uint32_t crtcId_;
  uint32_t planeId_;
  drmModeModeInfo mode_;
  bool drmInitialized_;
  bool usingHwAccel_; // Track if HW accel is working

  // Frame buffer tracking for page flipping
  uint32_t currentFbId_;
  uint32_t previousFbId_;
  uint32_t currentHandle_;  // GEM handle for cleanup
  uint32_t previousHandle_; // Previous GEM handle

  // DRM cursor state (static for access from InputDevice)
  // Uses dedicated cursor plane (plane 41 on RK3229, zpos 2)
  static int *cursorDrmFdPtr_; // Pointer to shared drmFd_ (set during init)
  static uint32_t cursorCrtcId_;
  static uint32_t cursorPlaneId_; // Cursor plane ID (41 on RK3229)
  static uint32_t cursorBufferHandle_;
  static uint32_t cursorFbId_;
  static int cursorX_;      // Current cursor X position
  static int cursorY_;      // Current cursor Y position
  static int cursorWidth_;  // Cursor buffer width (64)
  static int cursorHeight_; // Cursor buffer height (64)
  static bool cursorInitialized_;
  static bool cursorVisible_;
  static bool cursorEnabled_; // Set from configuration - controls if DRM cursor
                              // is active
  static std::mutex cursorMutex_;
};

} // namespace projection
} // namespace autoapp
} // namespace openauto
} // namespace f1x

#endif // USE_FFMPEG_DRM
