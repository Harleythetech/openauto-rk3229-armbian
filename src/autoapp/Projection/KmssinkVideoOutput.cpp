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
 * KmssinkVideoOutput.cpp
 *
 * Implementation of the KMS/DRM video output backend for OpenAuto.
 *
 * This backend uses GStreamer to decode H.264 video frames received from
 * Android Auto and display them directly on a KMS/DRM plane. This is ideal
 * for embedded Linux systems without a full display server (X11/Wayland).
 *
 * Pipeline Architecture:
 * ┌─────────┐    ┌───────────┐    ┌──────────────┐    ┌─────────┐
 * │ appsrc  │───>│ h264parse │───>│ v4l2slh264dec│───>│ kmssink │
 * └─────────┘    └───────────┘    └──────────────┘    └─────────┘
 *     │                                                     │
 *     │ Raw H.264 NAL units                                 │ DRM/KMS
 *     │ from Android Auto                                   │ plane output
 *
 * Key Components:
 * - appsrc: Application source that receives H.264 data from OpenAuto
 * - h264parse: Parses H.264 bitstream and extracts NAL units
 * - v4l2slh264dec: V4L2 stateless H.264 decoder (hardware accelerated)
 * - kmssink: Renders decoded frames directly to DRM/KMS plane
 *
 * Alternative decoders (modify pipeline if needed):
 * - v4l2h264dec: V4L2 stateful H.264 decoder
 * - avdec_h264: Software decoder (libav/ffmpeg)
 * - vaapih264dec: VA-API hardware decoder
 *
 * Build Requirements:
 * - GStreamer 1.0 with gst-plugins-base and gst-plugins-bad
 * - libdrm for KMS/DRM support
 * - Kernel with V4L2 stateless codec support (for v4l2slh264dec)
 *
 * Compile with: -DUSE_KMSSINK to enable this backend
 */

#ifdef USE_KMSSINK

// GStreamer includes
// Note: GStreamer headers already have proper C++ guards internally,
// so we do NOT wrap them in extern "C" - doing so breaks C++ template includes
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

// Standard library includes
#include <cstring>
#include <chrono>

// OpenAuto includes
#include <aasdk/Common/Data.hpp>
#include <f1x/openauto/autoapp/Projection/KmssinkVideoOutput.hpp>
#include <f1x/openauto/Common/Log.hpp>

namespace f1x
{
    namespace openauto
    {
        namespace autoapp
        {
            namespace projection
            {

                // ============================================================================
                // Constructor
                // ============================================================================

                KmssinkVideoOutput::KmssinkVideoOutput(configuration::IConfiguration::Pointer configuration)
                    : VideoOutput(std::move(configuration)) // Initialize base class with config
                      ,
                      isActive_(false) // Pipeline not active initially
                      ,
                      frameCount_(0) // Reset frame counter
                      ,
                      pipeline_(nullptr) // No pipeline created yet
                      ,
                      appsrc_(nullptr) // Pipeline elements are null
                      ,
                      h264parse_(nullptr), decoder_(nullptr), videoconvert_(nullptr), kmssink_(nullptr), connectorId_(0) // 0 = auto-detect connector
                      ,
                      planeId_(0) // 0 = auto-detect plane
                {
                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Constructor - backend initialized";
                }

                // ============================================================================
                // Destructor
                // ============================================================================

                KmssinkVideoOutput::~KmssinkVideoOutput()
                {
                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Destructor called";

                    // Ensure pipeline is stopped and resources are released
                    stop();
                }

                // ============================================================================
                // open() - Initialize GStreamer and create the pipeline
                // ============================================================================

                bool KmssinkVideoOutput::open()
                {
                    std::lock_guard<decltype(mutex_)> lock(mutex_);

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] open() - Initializing GStreamer pipeline";

                    // Step 1: Initialize GStreamer (safe to call multiple times)
                    // ----------------------------------------------------------------
                    // gst_init checks if already initialized, so this is safe
                    if (!gst_is_initialized())
                    {
                        GError *error = nullptr;
                        if (!gst_init_check(nullptr, nullptr, &error))
                        {
                            OPENAUTO_LOG(error) << "[KmssinkVideoOutput] GStreamer initialization failed: "
                                                << (error ? error->message : "Unknown error");
                            if (error)
                                g_error_free(error);
                            return false;
                        }
                        OPENAUTO_LOG(info) << "[KmssinkVideoOutput] GStreamer initialized successfully, version: "
                                           << gst_version_string();
                    }

                    // Step 2: Create the GStreamer pipeline
                    // ----------------------------------------------------------------
                    if (!createPipeline())
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to create GStreamer pipeline";
                        releasePipeline();
                        return false;
                    }

                    // Step 3: Configure the appsrc element for H.264 input
                    // ----------------------------------------------------------------
                    if (!configureAppsrc())
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to configure appsrc";
                        releasePipeline();
                        return false;
                    }

                    // Step 4: Configure the kmssink element
                    // ----------------------------------------------------------------
                    if (!configureKmssink())
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to configure kmssink";
                        releasePipeline();
                        return false;
                    }

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Pipeline created successfully";
                    return true;
                }

                // ============================================================================
                // createPipeline() - Create and link GStreamer elements
                // ============================================================================

                bool KmssinkVideoOutput::createPipeline()
                {
                    // Create the main pipeline container
                    // ----------------------------------------------------------------
                    pipeline_ = gst_pipeline_new("kmssink-video-pipeline");
                    if (!pipeline_)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to create pipeline";
                        return false;
                    }

                    // Create individual pipeline elements
                    // ----------------------------------------------------------------
                    // appsrc: Application source - we push H.264 data into this
                    appsrc_ = gst_element_factory_make("appsrc", "video-appsrc");
                    if (!appsrc_)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to create appsrc element";
                        return false;
                    }

                    // h264parse: H.264 bitstream parser
                    // Parses raw H.264 data and provides proper framing for the decoder
                    h264parse_ = gst_element_factory_make("h264parse", "h264-parser");
                    if (!h264parse_)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to create h264parse element";
                        return false;
                    }

                    // decoder: H.264 decoder
                    // ----------------------------------------------------------------
                    // Try hardware decoder first - outputs DMA-BUF NV12 which kmssink can use directly.
                    // Skip videoconvert for hardware decoders as it can't handle DMA-BUF memory.
                    bool usingHwDecoder = false;

                    // Try stateless V4L2 decoder first (best for RK3229/rkvdec)
                    decoder_ = gst_element_factory_make("v4l2slh264dec", "video-decoder");
                    if (decoder_)
                    {
                        OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Using v4l2slh264dec (stateless HW decoder)";
                        usingHwDecoder = true;
                    }
                    if (!decoder_)
                    {
                        // Fallback to stateful V4L2 decoder
                        OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] v4l2slh264dec not available, trying v4l2h264dec";
                        decoder_ = gst_element_factory_make("v4l2h264dec", "video-decoder");
                        if (decoder_)
                        {
                            OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Using v4l2h264dec (stateful HW decoder)";
                            usingHwDecoder = true;
                        }
                    }
                    if (!decoder_)
                    {
                        // Fallback to software decoder - try openh264dec first (available on this system)
                        OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] HW decoders not available, trying openh264dec";
                        decoder_ = gst_element_factory_make("openh264dec", "video-decoder");
                        if (decoder_)
                        {
                            OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Using openh264dec (software decoder)";
                            usingHwDecoder = false;
                        }
                    }
                    if (!decoder_)
                    {
                        // Last fallback - avdec_h264 (requires gstreamer1.0-libav)
                        OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] openh264dec not available, trying avdec_h264";
                        decoder_ = gst_element_factory_make("avdec_h264", "video-decoder");
                        if (decoder_)
                        {
                            OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Using avdec_h264 (software decoder)";
                            usingHwDecoder = false;
                        }
                    }
                    if (!decoder_)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] No H.264 decoder available";
                        return false;
                    }

                    // videoconvert: Only used for software decoders
                    // ----------------------------------------------------------------
                    // Hardware decoders output DMA-BUF NV12 which goes directly to kmssink.
                    // Software decoders may need format conversion.
                    videoconvert_ = nullptr;
                    if (!usingHwDecoder)
                    {
                        videoconvert_ = gst_element_factory_make("videoconvert", "video-converter");
                        if (!videoconvert_)
                        {
                            OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] Failed to create videoconvert";
                        }
                    }
                    else
                    {
                        OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Using HW decoder - skipping videoconvert (DMA-BUF path)";
                    }

                    // kmssink: KMS/DRM video sink
                    // Renders directly to a DRM plane without compositor
                    kmssink_ = gst_element_factory_make("kmssink", "video-sink");
                    if (!kmssink_)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to create kmssink element "
                                            << "- ensure gst-plugins-bad is installed";
                        return false;
                    }

                    // Add all elements to the pipeline
                    // ----------------------------------------------------------------
                    if (videoconvert_)
                    {
                        gst_bin_add_many(GST_BIN(pipeline_), appsrc_, h264parse_, decoder_, videoconvert_, kmssink_, nullptr);
                    }
                    else
                    {
                        gst_bin_add_many(GST_BIN(pipeline_), appsrc_, h264parse_, decoder_, kmssink_, nullptr);
                    }

                    // Link elements together: appsrc -> h264parse -> decoder -> [videoconvert] -> kmssink
                    // ----------------------------------------------------------------
                    bool linked = false;
                    if (videoconvert_)
                    {
                        linked = gst_element_link_many(appsrc_, h264parse_, decoder_, videoconvert_, kmssink_, nullptr);
                    }
                    else
                    {
                        linked = gst_element_link_many(appsrc_, h264parse_, decoder_, kmssink_, nullptr);
                    }

                    if (!linked)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to link pipeline elements";
                        return false;
                    }

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Pipeline elements created and linked";

                    // Add debug probes to track data flow through pipeline
                    // ----------------------------------------------------------------
                    GstPad *decoderSrcPad = gst_element_get_static_pad(decoder_, "src");
                    if (decoderSrcPad)
                    {
                        gst_pad_add_probe(decoderSrcPad, GST_PAD_PROBE_TYPE_BUFFER, [](GstPad *pad, GstPadProbeInfo *info, gpointer user_data) -> GstPadProbeReturn
                                          {
                                static int decoderFrameCount = 0;
                                if (++decoderFrameCount <= 5 || decoderFrameCount % 100 == 0) {
                                    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
                                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Decoder output frame " 
                                                       << decoderFrameCount << ", size: " << gst_buffer_get_size(buffer);
                                }
                                return GST_PAD_PROBE_OK; }, nullptr, nullptr);
                        gst_object_unref(decoderSrcPad);
                    }

                    GstPad *kmssinkPad = gst_element_get_static_pad(kmssink_, "sink");
                    if (kmssinkPad)
                    {
                        gst_pad_add_probe(kmssinkPad, GST_PAD_PROBE_TYPE_BUFFER, [](GstPad *pad, GstPadProbeInfo *info, gpointer user_data) -> GstPadProbeReturn
                                          {
                                static int sinkFrameCount = 0;
                                if (++sinkFrameCount <= 5 || sinkFrameCount % 100 == 0) {
                                    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
                                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] kmssink received frame " 
                                                       << sinkFrameCount << ", size: " << gst_buffer_get_size(buffer);
                                }
                                return GST_PAD_PROBE_OK; }, nullptr, nullptr);
                        gst_object_unref(kmssinkPad);
                    }

                    return true;
                }

                // ============================================================================
                // configureAppsrc() - Configure the application source for H.264 video
                // ============================================================================

                bool KmssinkVideoOutput::configureAppsrc()
                {
                    if (!appsrc_)
                    {
                        return false;
                    }

                    // Get video dimensions from configuration
                    int width = getVideoWidth();
                    int height = getVideoHeight();

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Configuring appsrc for "
                                       << width << "x" << height << " H.264 video";

                    // Create caps (capabilities) describing the input format
                    // ----------------------------------------------------------------
                    // stream-format=byte-stream: Raw H.264 NAL unit stream (Annex B format)
                    // alignment=au: Data is aligned on Access Units (complete frames)
                    GstCaps *caps = gst_caps_new_simple(
                        "video/x-h264",
                        "stream-format", G_TYPE_STRING, "byte-stream",
                        "alignment", G_TYPE_STRING, "au",
                        "width", G_TYPE_INT, width,
                        "height", G_TYPE_INT, height,
                        nullptr);

                    if (!caps)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to create caps";
                        return false;
                    }

                    // Configure appsrc properties
                    // ----------------------------------------------------------------
                    g_object_set(G_OBJECT(appsrc_),
                                 // Set the caps to describe the data format
                                 "caps", caps,
                                 // Stream type: This is a continuous stream, not seekable
                                 "stream-type", GST_APP_STREAM_TYPE_STREAM,
                                 // Format: We're providing time-stamped buffers
                                 "format", GST_FORMAT_TIME,
                                 // Enable live mode for real-time streaming
                                 "is-live", TRUE,
                                 // Don't block when pushing data (async mode)
                                 "block", FALSE,
                                 // Max bytes to queue (tune based on memory constraints)
                                 "max-bytes", (guint64)(4 * 1024 * 1024), // 4MB buffer
                                 // Min latency in nanoseconds (for live sources)
                                 "min-latency", (gint64)0,
                                 nullptr);

                    // Clean up caps reference (appsrc takes ownership)
                    gst_caps_unref(caps);

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] appsrc configured successfully";
                    return true;
                }

                // ============================================================================
                // configureKmssink() - Configure the KMS sink for display output
                // ============================================================================

                bool KmssinkVideoOutput::configureKmssink()
                {
                    if (!kmssink_)
                    {
                        return false;
                    }

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Configuring kmssink";

                    // Configure kmssink properties
                    // ----------------------------------------------------------------
                    // IMPORTANT: sync=FALSE is required because Android Auto timestamps
                    // are absolute (starting at ~150+ seconds), not relative to stream start.
                    // With sync=TRUE, kmssink would wait forever before displaying the first frame.
                    g_object_set(G_OBJECT(kmssink_),
                                 // Disable sync - Android Auto uses absolute timestamps
                                 "sync", FALSE,
                                 // Enable async state changes
                                 "async", TRUE,
                                 nullptr);

                    // Set connector and plane for Rockchip RK3229 with HDMI
                    // ----------------------------------------------------------------
                    // Connector 46: HDMI-A-1 (the only connected display)
                    // Plane 31: Primary (zpos=0) - supports NV12
                    // Plane 36: Overlay (zpos=1) - supports NV12, renders on top
                    // Plane 41: Cursor (zpos=2) - no NV12 support
                    //
                    // NOTE: Stop the GUI before running autoapp, as both compete for the same plane
                    int effectiveConnectorId = (connectorId_ > 0) ? connectorId_ : 46; // HDMI-A-1
                    int effectivePlaneId = (planeId_ > 0) ? planeId_ : 31;             // Primary plane

                    g_object_set(G_OBJECT(kmssink_),
                                 "connector-id", effectiveConnectorId,
                                 "plane-id", effectivePlaneId,
                                 nullptr);
                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Using connector-id: " << effectiveConnectorId
                                       << ", plane-id: " << effectivePlaneId;

                    // Note: To find available connectors and planes, run:
                    // modetest -M rockchip -c (for connectors)
                    // modetest -M rockchip -p (for planes)

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] kmssink configured successfully";
                    return true;
                }

                // ============================================================================
                // init() - Start the GStreamer pipeline
                // ============================================================================

                bool KmssinkVideoOutput::init()
                {
                    std::lock_guard<decltype(mutex_)> lock(mutex_);

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] init() - Starting pipeline, isActive: " << isActive_.load();

                    if (!pipeline_)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Cannot init - pipeline not created";
                        return false;
                    }

                    if (isActive_.load())
                    {
                        OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] Pipeline already active";
                        return true;
                    }

                    // Set up bus watch for error messages
                    GstBus *bus = gst_element_get_bus(pipeline_);
                    if (bus)
                    {
                        gst_bus_add_watch(bus, [](GstBus *bus, GstMessage *msg, gpointer user_data) -> gboolean
                                          {
                            switch (GST_MESSAGE_TYPE(msg)) {
                                case GST_MESSAGE_ERROR: {
                                    GError *err = nullptr;
                                    gchar *debug = nullptr;
                                    gst_message_parse_error(msg, &err, &debug);
                                    OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Pipeline ERROR: " 
                                                        << (err ? err->message : "Unknown")
                                                        << " - Debug: " << (debug ? debug : "none");
                                    if (err) g_error_free(err);
                                    g_free(debug);
                                    break;
                                }
                                case GST_MESSAGE_WARNING: {
                                    GError *err = nullptr;
                                    gchar *debug = nullptr;
                                    gst_message_parse_warning(msg, &err, &debug);
                                    OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] Pipeline WARNING: " 
                                                          << (err ? err->message : "Unknown");
                                    if (err) g_error_free(err);
                                    g_free(debug);
                                    break;
                                }
                                case GST_MESSAGE_STATE_CHANGED: {
                                    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(user_data)) {
                                        GstState old_state, new_state, pending_state;
                                        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                                        OPENAUTO_LOG(debug) << "[KmssinkVideoOutput] Pipeline state: "
                                                            << gst_element_state_get_name(old_state) << " -> "
                                                            << gst_element_state_get_name(new_state);
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                            return TRUE; }, pipeline_);
                        gst_object_unref(bus);
                    }

                    // Transition pipeline to PLAYING state
                    // ----------------------------------------------------------------
                    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);

                    if (ret == GST_STATE_CHANGE_FAILURE)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to set pipeline to PLAYING state";

                        // Get more detailed error information
                        GstBus *bus = gst_element_get_bus(pipeline_);
                        if (bus)
                        {
                            GstMessage *msg = gst_bus_poll(bus, GST_MESSAGE_ERROR, 0);
                            if (msg)
                            {
                                GError *err = nullptr;
                                gchar *debug = nullptr;
                                gst_message_parse_error(msg, &err, &debug);
                                OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Pipeline error: "
                                                    << (err ? err->message : "Unknown");
                                if (debug)
                                    OPENAUTO_LOG(debug) << "[KmssinkVideoOutput] Debug: " << debug;
                                if (err)
                                    g_error_free(err);
                                g_free(debug);
                                gst_message_unref(msg);
                            }
                            gst_object_unref(bus);
                        }
                        return false;
                    }

                    // Mark pipeline as active
                    isActive_.store(true);
                    frameCount_ = 0;

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Pipeline started successfully (state change: "
                                       << (ret == GST_STATE_CHANGE_SUCCESS ? "SUCCESS" : ret == GST_STATE_CHANGE_ASYNC ? "ASYNC"
                                                                                                                       : "NO_PREROLL")
                                       << ")";

                    return true;
                }

                // ============================================================================
                // write() - Push H.264 frame data to the pipeline
                // ============================================================================

                void KmssinkVideoOutput::write(uint64_t timestamp, const aasdk::common::DataConstBuffer &buffer)
                {
                    std::lock_guard<decltype(mutex_)> lock(mutex_);

                    // Check if pipeline is active
                    if (!isActive_.load() || !appsrc_)
                    {
                        return;
                    }

                    // Validate buffer
                    if (buffer.size == 0 || buffer.cdata == nullptr)
                    {
                        OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] Received empty buffer, skipping";
                        return;
                    }

                    // Log first 5 frames for debugging
                    if (frameCount_ < 5)
                    {
                        OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Frame " << frameCount_
                                           << " - size: " << buffer.size << " bytes, timestamp: " << timestamp;
                    }

                    // Create a GstBuffer to hold the frame data
                    // ----------------------------------------------------------------
                    GstBuffer *gstBuffer = gst_buffer_new_allocate(nullptr, buffer.size, nullptr);
                    if (!gstBuffer)
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to allocate GstBuffer";
                        return;
                    }

                    // Copy frame data into the GstBuffer
                    // ----------------------------------------------------------------
                    GstMapInfo map;
                    if (gst_buffer_map(gstBuffer, &map, GST_MAP_WRITE))
                    {
                        memcpy(map.data, buffer.cdata, buffer.size);
                        gst_buffer_unmap(gstBuffer, &map);
                    }
                    else
                    {
                        OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to map GstBuffer for writing";
                        gst_buffer_unref(gstBuffer);
                        return;
                    }

                    // Set buffer timestamps
                    // ----------------------------------------------------------------
                    // PTS (Presentation Time Stamp): When this frame should be displayed
                    // Android Auto provides timestamps in nanoseconds
                    GST_BUFFER_PTS(gstBuffer) = timestamp;

                    // DTS (Decode Time Stamp): For H.264 with B-frames, DTS != PTS
                    // For Android Auto streams (typically no B-frames), DTS = PTS
                    GST_BUFFER_DTS(gstBuffer) = timestamp;

                    // Duration: We don't have exact duration, let downstream elements calculate
                    GST_BUFFER_DURATION(gstBuffer) = GST_CLOCK_TIME_NONE;

                    // Push the buffer into appsrc
                    // ----------------------------------------------------------------
                    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc_), gstBuffer);
                    // Note: gst_app_src_push_buffer takes ownership of the buffer, don't unref

                    if (ret != GST_FLOW_OK)
                    {
                        if (ret == GST_FLOW_FLUSHING)
                        {
                            OPENAUTO_LOG(debug) << "[KmssinkVideoOutput] Pipeline flushing, buffer dropped";
                        }
                        else if (ret == GST_FLOW_EOS)
                        {
                            OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Pipeline at EOS";
                        }
                        else
                        {
                            OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Failed to push buffer, flow return: " << ret;
                        }
                        return;
                    }

                    // Update frame counter and log periodically
                    frameCount_++;
                    if (frameCount_ % 300 == 0) // Log every ~10 seconds at 30fps
                    {
                        OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Processed " << frameCount_
                                           << " frames, buffer size: " << buffer.size << " bytes";
                    }

                    // Synchronously poll for any pipeline errors (since we don't have a GLib main loop)
                    if (pipeline_ && (frameCount_ % 30 == 1)) // Check every second at 30fps
                    {
                        GstBus *bus = gst_element_get_bus(pipeline_);
                        if (bus)
                        {
                            GstMessage *msg = nullptr;
                            while ((msg = gst_bus_pop_filtered(bus, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING))) != nullptr)
                            {
                                if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR)
                                {
                                    GError *err = nullptr;
                                    gchar *debug = nullptr;
                                    gst_message_parse_error(msg, &err, &debug);
                                    OPENAUTO_LOG(error) << "[KmssinkVideoOutput] Pipeline ERROR: "
                                                        << (err ? err->message : "Unknown")
                                                        << " - Debug: " << (debug ? debug : "none");
                                    if (err)
                                        g_error_free(err);
                                    g_free(debug);
                                }
                                else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_WARNING)
                                {
                                    GError *err = nullptr;
                                    gchar *debug = nullptr;
                                    gst_message_parse_warning(msg, &err, &debug);
                                    OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] Pipeline WARNING: "
                                                          << (err ? err->message : "Unknown");
                                    if (err)
                                        g_error_free(err);
                                    g_free(debug);
                                }
                                gst_message_unref(msg);
                            }
                            gst_object_unref(bus);
                        }
                    }
                }

                // ============================================================================
                // renderFrame() - Alternative interface for rendering frames
                // ============================================================================

                bool KmssinkVideoOutput::renderFrame(const uint8_t *frameData, size_t frameSize, uint64_t timestamp)
                {
                    // Create a DataConstBuffer wrapper and delegate to write()
                    // ----------------------------------------------------------------
                    if (!frameData || frameSize == 0)
                    {
                        OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] renderFrame called with invalid data";
                        return false;
                    }

                    // Create a const buffer wrapper for the frame data
                    aasdk::common::DataConstBuffer buffer(frameData, frameSize);

                    // Delegate to the main write method
                    write(timestamp, buffer);

                    // Check if pipeline is still active (write succeeded)
                    return isActive_.load();
                }

                // ============================================================================
                // stop() - Stop the pipeline and release resources
                // ============================================================================

                void KmssinkVideoOutput::stop()
                {
                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] stop() called";

                    std::lock_guard<decltype(mutex_)> lock(mutex_);

                    if (!isActive_.load() && !pipeline_)
                    {
                        OPENAUTO_LOG(debug) << "[KmssinkVideoOutput] Already stopped";
                        return;
                    }

                    isActive_.store(false);

                    // Send End-Of-Stream to allow graceful shutdown
                    // ----------------------------------------------------------------
                    if (appsrc_)
                    {
                        OPENAUTO_LOG(debug) << "[KmssinkVideoOutput] Sending EOS to pipeline";
                        gst_app_src_end_of_stream(GST_APP_SRC(appsrc_));
                    }

                    // Give the pipeline a moment to process EOS
                    if (pipeline_)
                    {
                        GstBus *bus = gst_element_get_bus(pipeline_);
                        if (bus)
                        {
                            // Wait up to 1 second for EOS message
                            GstMessage *msg = gst_bus_timed_pop_filtered(
                                bus,
                                GST_SECOND, // 1 second timeout
                                static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
                            if (msg)
                            {
                                if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR)
                                {
                                    GError *err = nullptr;
                                    gst_message_parse_error(msg, &err, nullptr);
                                    OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] Error during shutdown: "
                                                          << (err ? err->message : "Unknown");
                                    if (err)
                                        g_error_free(err);
                                }
                                gst_message_unref(msg);
                            }
                            gst_object_unref(bus);
                        }
                    }

                    // Release all pipeline resources
                    releasePipeline();

                    OPENAUTO_LOG(info) << "[KmssinkVideoOutput] Stopped. Total frames processed: " << frameCount_;
                }

                // ============================================================================
                // releasePipeline() - Clean up GStreamer resources
                // ============================================================================

                void KmssinkVideoOutput::releasePipeline()
                {
                    if (pipeline_)
                    {
                        OPENAUTO_LOG(debug) << "[KmssinkVideoOutput] Setting pipeline to NULL state";

                        // Set pipeline to NULL state (releases all resources)
                        gst_element_set_state(pipeline_, GST_STATE_NULL);

                        // Wait for state change to complete
                        GstState state, pending;
                        gst_element_get_state(pipeline_, &state, &pending, GST_SECOND);

                        // Unref the pipeline (this also unrefs all elements added to it)
                        gst_object_unref(GST_OBJECT(pipeline_));
                        pipeline_ = nullptr;

                        OPENAUTO_LOG(debug) << "[KmssinkVideoOutput] Pipeline resources released";
                    }

                    // Clear element pointers (they were owned by the pipeline)
                    appsrc_ = nullptr;
                    h264parse_ = nullptr;
                    decoder_ = nullptr;
                    videoconvert_ = nullptr;
                    kmssink_ = nullptr;
                }

                // ============================================================================
                // getVideoWidth() - Get video width from configuration
                // ============================================================================

                int KmssinkVideoOutput::getVideoWidth() const
                {
                    // Get resolution from configuration and return appropriate width
                    // ----------------------------------------------------------------
                    auto resolution = configuration_->getVideoResolution();

                    switch (resolution)
                    {
                    case aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_800x480:
                        return 800;
                    case aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_1280x720:
                        return 1280;
                    case aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_1920x1080:
                        return 1920;
                    default:
                        OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] Unknown resolution, defaulting to 800x480";
                        return 800;
                    }
                }

                // ============================================================================
                // getVideoHeight() - Get video height from configuration
                // ============================================================================

                int KmssinkVideoOutput::getVideoHeight() const
                {
                    // Get resolution from configuration and return appropriate height
                    // ----------------------------------------------------------------
                    auto resolution = configuration_->getVideoResolution();

                    switch (resolution)
                    {
                    case aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_800x480:
                        return 480;
                    case aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_1280x720:
                        return 720;
                    case aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_1920x1080:
                        return 1080;
                    default:
                        OPENAUTO_LOG(warning) << "[KmssinkVideoOutput] Unknown resolution, defaulting to 800x480";
                        return 480;
                    }
                }

            } // namespace projection
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x

#endif // USE_KMSSINK
