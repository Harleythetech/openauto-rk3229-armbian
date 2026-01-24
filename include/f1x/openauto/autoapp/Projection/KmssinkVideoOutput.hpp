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
 * KmssinkVideoOutput.hpp
 *
 * This header defines the KmssinkVideoOutput class, which provides video output
 * via Linux KMS/DRM using GStreamer's kmssink element. This backend is designed
 * for embedded Linux systems where hardware-accelerated video decoding and
 * direct DRM/KMS display output are desired.
 *
 * Pipeline: appsrc -> h264parse -> v4l2slh264dec -> kmssink
 *
 * The v4l2slh264dec element uses V4L2 stateless H.264 decoder (available on
 * platforms like Raspberry Pi 4, i.MX8, etc.), and kmssink renders directly
 * to a DRM/KMS plane without requiring X11 or Wayland.
 */

#pragma once

// Compile-time feature flag for KMS sink support
#ifdef USE_KMSSINK

// GStreamer includes - GStreamer headers already have proper C++ guards internally,
// so we do NOT wrap them in extern "C" - doing so breaks C++ template includes
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <mutex>
#include <atomic>
#include <cstdint>
#include <string>
#include <f1x/openauto/autoapp/Projection/VideoOutput.hpp>

namespace f1x
{
    namespace openauto
    {
        namespace autoapp
        {
            namespace projection
            {

                /**
                 * @class KmssinkVideoOutput
                 * @brief Video output backend using GStreamer with kmssink for KMS/DRM display.
                 *
                 * This class implements the IVideoOutput interface (via VideoOutput base class)
                 * and provides a GStreamer-based video rendering pipeline that outputs H.264
                 * video frames to a Linux DRM/KMS display using hardware-accelerated decoding.
                 *
                 * Features:
                 * - Hardware-accelerated H.264 decoding via V4L2 stateless decoder
                 * - Direct rendering to DRM/KMS plane (no compositing overhead)
                 * - Configurable DRM connector and plane IDs
                 * - Thread-safe frame writing
                 * - DRM hardware cursor support
                 *
                 * Usage:
                 * 1. Create instance with configuration
                 * 2. Call open() to initialize GStreamer and create pipeline
                 * 3. Call init() to start the pipeline
                 * 4. Call write() to push H.264 frames
                 * 5. Call stop() to cleanup
                 */
                class KmssinkVideoOutput : public VideoOutput
                {
                public:
                    /**
                     * @brief Constructs the KmssinkVideoOutput with the given configuration.
                     * @param configuration Pointer to the OpenAuto configuration interface.
                     *
                     * The configuration provides video resolution, FPS, DPI, and margins settings.
                     * Additional KMS-specific settings (connector-id, plane-id) can be extended
                     * in the configuration interface.
                     */
                    explicit KmssinkVideoOutput(configuration::IConfiguration::Pointer configuration);

                    /**
                     * @brief Destructor - ensures proper cleanup of GStreamer resources.
                     */
                    ~KmssinkVideoOutput() override;

                    // Prevent copying - GStreamer resources cannot be safely copied
                    KmssinkVideoOutput(const KmssinkVideoOutput &) = delete;
                    KmssinkVideoOutput &operator=(const KmssinkVideoOutput &) = delete;

                    /**
                     * @brief Initializes GStreamer and creates the video pipeline.
                     * @return true if initialization succeeded, false otherwise.
                     *
                     * This method:
                     * 1. Initializes GStreamer if not already done
                     * 2. Creates the pipeline: appsrc ! h264parse ! v4l2slh264dec ! kmssink
                     * 3. Configures appsrc for H.264 AVC streaming
                     * 4. Sets up kmssink with appropriate connector/plane IDs
                     */
                    bool open() override;

                    /**
                     * @brief Starts the GStreamer pipeline for video playback.
                     * @return true if pipeline started successfully, false otherwise.
                     *
                     * Transitions the pipeline to PLAYING state and prepares for frame input.
                     */
                    bool init() override;

                    /**
                     * @brief Writes an H.264 video frame to the GStreamer pipeline.
                     * @param timestamp The presentation timestamp in nanoseconds.
                     * @param buffer The raw H.264 frame data to be decoded and displayed.
                     *
                     * This method pushes raw H.264 NAL units into the appsrc element.
                     * The data will be parsed, decoded, and rendered by the pipeline.
                     * Thread-safe - can be called from any thread.
                     */
                    void write(uint64_t timestamp, const aasdk::common::DataConstBuffer &buffer) override;

                    /**
                     * @brief Stops the pipeline and releases all GStreamer resources.
                     *
                     * This method:
                     * 1. Stops the pipeline gracefully (sends EOS)
                     * 2. Waits for pipeline to reach NULL state
                     * 3. Releases all GStreamer objects
                     *
                     * Safe to call multiple times.
                     */
                    void stop() override;

                    /**
                     * @brief Renders a raw frame (alternative interface for frame data).
                     * @param frameData Pointer to raw H.264 frame data.
                     * @param frameSize Size of the frame data in bytes.
                     * @param timestamp Presentation timestamp in nanoseconds.
                     * @return true if frame was successfully pushed to pipeline, false otherwise.
                     *
                     * This is a convenience method that provides a direct interface for
                     * pushing frame data without using the DataConstBuffer wrapper.
                     */
                    bool renderFrame(const uint8_t *frameData, size_t frameSize, uint64_t timestamp);

                    /**
                     * @brief Updates the hardware cursor position.
                     * @param x X coordinate in screen pixels.
                     * @param y Y coordinate in screen pixels.
                     *
                     * This updates the DRM hardware cursor position on the cursor plane.
                     * Static method so it can be called from InputDevice.
                     */
                    static void updateCursorPosition(int x, int y);

                    /**
                     * @brief Shows or hides the hardware cursor.
                     * @param visible true to show cursor, false to hide.
                     */
                    static void setCursorVisible(bool visible);

                private:
                    /**
                     * @brief Creates and links all GStreamer pipeline elements.
                     * @return true if pipeline creation succeeded, false otherwise.
                     */
                    bool createPipeline();

                    /**
                     * @brief Configures the appsrc element for H.264 video input.
                     * @return true if configuration succeeded, false otherwise.
                     */
                    bool configureAppsrc();

                    /**
                     * @brief Configures the kmssink element with display settings.
                     * @return true if configuration succeeded, false otherwise.
                     */
                    bool configureKmssink();

                    /**
                     * @brief Releases all GStreamer resources.
                     */
                    void releasePipeline();

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

                    // Thread synchronization
                    std::mutex mutex_;

                    // Pipeline state flag
                    std::atomic<bool> isActive_;

                    // Frame counter for debugging/statistics
                    uint64_t frameCount_;

                    // GStreamer pipeline and elements
                    GstElement *pipeline_;     ///< Main GStreamer pipeline
                    GstElement *appsrc_;       ///< Application source for feeding H.264 data
                    GstElement *h264parse_;    ///< H.264 bitstream parser
                    GstElement *decoder_;      ///< V4L2 stateless H.264 decoder
                    GstElement *videoconvert_; ///< Video format converter
                    GstElement *kmssink_;      ///< KMS/DRM video sink

                    // Optional: DRM connector and plane IDs (0 = auto-detect)
                    int connectorId_;
                    int planeId_;

                    // DRM cursor state (static for access from InputDevice)
                    static int drmFd_;
                    static uint32_t cursorPlaneId_;
                    static uint32_t cursorCrtcId_;
                    static uint32_t cursorBufferHandle_;
                    static uint32_t cursorFbId_;
                    static bool cursorInitialized_;
                    static bool cursorVisible_;
                    static std::mutex cursorMutex_;
                };

            } // namespace projection
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x

#endif // USE_KMSSINK
