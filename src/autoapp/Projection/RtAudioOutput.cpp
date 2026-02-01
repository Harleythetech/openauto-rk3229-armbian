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

#include <cstring> // for memset
#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/Projection/RtAudioOutput.hpp>

#if defined(RTAUDIO_VERSION_MAJOR) && (RTAUDIO_VERSION_MAJOR >= 6)
#define OA_RTAUDIO_V6 1
#endif

namespace f1x
{
  namespace openauto
  {
    namespace autoapp
    {
      namespace projection
      {

        RtAudioOutput::RtAudioOutput(uint32_t channelCount, uint32_t sampleSize,
                                     uint32_t sampleRate, uint32_t deviceId)
            : channelCount_(channelCount), sampleSize_(sampleSize),
              sampleRate_(sampleRate), deviceId_(deviceId)
        {
          std::vector<RtAudio::Api> apis;
          RtAudio::getCompiledApi(apis);

          // Prefer ALSA over PulseAudio for lower latency and less overhead on embedded
          // systems
          if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_ALSA) != apis.end())
          {
            OPENAUTO_LOG(info) << "[RtAudioOutput] Using ALSA backend";
            dac_ = std::make_unique<RtAudio>(RtAudio::LINUX_ALSA);
          }
          else if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_PULSE) !=
                   apis.end())
          {
            OPENAUTO_LOG(info) << "[RtAudioOutput] Using PulseAudio backend";
            dac_ = std::make_unique<RtAudio>(RtAudio::LINUX_PULSE);
          }
          else
          {
            OPENAUTO_LOG(info) << "[RtAudioOutput] Using default audio backend";
            dac_ = std::make_unique<RtAudio>();
          }
        }

        bool RtAudioOutput::open()
        {
          std::lock_guard<decltype(mutex_)> lock(mutex_);

          if (dac_->getDeviceCount() <= 0)
          {
            OPENAUTO_LOG(error) << "[RtAudioOutput] No output devices found.";
            return false;
          }

          RtAudio::StreamParameters parameters;
          // Use specified device ID, fall back to default if 0 or invalid
          uint32_t selectedDevice =
              (deviceId_ != 0 && deviceId_ < dac_->getDeviceCount())
                  ? deviceId_
                  : dac_->getDefaultOutputDevice();
          parameters.deviceId = selectedDevice;
          parameters.nChannels = channelCount_;
          parameters.firstChannel = 0;

          OPENAUTO_LOG(info) << "[RtAudioOutput] Using device ID: " << selectedDevice;

          RtAudio::StreamOptions streamOptions;
          streamOptions.flags = RTAUDIO_SCHEDULE_REALTIME; // Try RT scheduling, fallback is fine
          streamOptions.numberOfBuffers = 4;               // More buffers = less underruns on RK3229

          // Larger buffers to prevent crackling on embedded systems
          // Trade-off: slightly higher latency but no audio glitches
          uint32_t bufferFrames =
              sampleRate_ == 16000 ? 2048
                                   : 4096;

#if defined(OA_RTAUDIO_V6)
          // RtAudio 6+: methods return RtAudioErrorType instead of throwing.
          RtAudioErrorType err = dac_->openStream(
              &parameters, /*input*/ nullptr, RTAUDIO_SINT16, sampleRate_,
              &bufferFrames, &RtAudioOutput::audioBufferReadHandler,
              static_cast<void *>(this), &streamOptions);

          if (err != RTAUDIO_NO_ERROR)
          {
            OPENAUTO_LOG(error) << "[RtAudioOutput] openStream failed, code="
                                << static_cast<int>(err)
                                << " msg=" << dac_->getErrorText();
            return false;
          }
#else
          try
          {
            dac_->openStream(&parameters, /*input*/ nullptr, RTAUDIO_SINT16,
                             sampleRate_, &bufferFrames,
                             &RtAudioOutput::audioBufferReadHandler,
                             static_cast<void *>(this), &streamOptions);
          }
          catch (const RtAudioError &e)
          {
            // Older RtAudio throws exceptions.
            OPENAUTO_LOG(error) << "[RtAudioOutput] Failed to open audio output, what: "
                                << e.what();
            return false;
          }
#endif

          OPENAUTO_LOG(info) << "[RtAudioOutput] Sample Rate: " << sampleRate_;
          return audioBuffer_.open(QIODevice::ReadWrite);
        }

        void RtAudioOutput::write(aasdk::messenger::Timestamp::ValueType timestamp,
                                  const aasdk::common::DataConstBuffer &buffer)
        {
          // Early exit if we're in the process of stopping - prevents crash during USB
          // disconnect
          if (isStopping_.load(std::memory_order_acquire))
          {
            return;
          }

          // Don't acquire mutex here - audioBuffer_ is thread-safe and holding the lock
          // can cause deadlock with the audio callback
          audioBuffer_.write(reinterpret_cast<const char *>(buffer.cdata), buffer.size);
        }

        void RtAudioOutput::start()
        {
          std::lock_guard<decltype(mutex_)> lock(mutex_);

          if (dac_->isStreamOpen() && !dac_->isStreamRunning())
          {
#if defined(OA_RTAUDIO_V6)
            RtAudioErrorType err = dac_->startStream();
            if (err != RTAUDIO_NO_ERROR)
            {
              OPENAUTO_LOG(error) << "[RtAudioOutput] startStream failed, code="
                                  << static_cast<int>(err)
                                  << " msg=" << dac_->getErrorText();
            }
#else
            try
            {
              dac_->startStream();
            }
            catch (const RtAudioError &e)
            {
              OPENAUTO_LOG(error)
                  << "[RtAudioOutput] Failed to start audio output, what: " << e.what();
            }
#endif
          }
        }

        void RtAudioOutput::stop()
        {
          // Set stopping flag BEFORE acquiring lock - allows write() to exit early
          isStopping_.store(true, std::memory_order_release);

          std::lock_guard<decltype(mutex_)> lock(mutex_);

          try
          {
            this->doSuspend();
          }
          catch (...)
          {
            OPENAUTO_LOG(warning)
                << "[RtAudioOutput] Exception during suspend in stop()";
          }

          try
          {
            if (dac_ && dac_->isStreamOpen())
            {
              dac_->closeStream();
            }
          }
          catch (...)
          {
            OPENAUTO_LOG(warning)
                << "[RtAudioOutput] Exception during closeStream in stop()";
          }

          // Clear the audio buffer to prevent stale data on restart
          audioBuffer_.close();
        }

        void RtAudioOutput::suspend()
        {
          // not needed
        }

        uint32_t RtAudioOutput::getSampleSize() const { return sampleSize_; }

        uint32_t RtAudioOutput::getChannelCount() const { return channelCount_; }

        uint32_t RtAudioOutput::getSampleRate() const { return sampleRate_; }

        void RtAudioOutput::doSuspend()
        {
          if (dac_->isStreamOpen() && dac_->isStreamRunning())
          {
#if defined(OA_RTAUDIO_V6)
            RtAudioErrorType err = dac_->stopStream();
            if (err != RTAUDIO_NO_ERROR)
            {
              OPENAUTO_LOG(error) << "[RtAudioOutput] stopStream failed, code="
                                  << static_cast<int>(err)
                                  << " msg=" << dac_->getErrorText();
            }
#else
            try
            {
              dac_->stopStream();
            }
            catch (const RtAudioError &e)
            {
              OPENAUTO_LOG(error)
                  << "[RtAudioOutput] Failed to suspend audio output, what: "
                  << e.what();
            }
#endif
          }
        }

        int RtAudioOutput::audioBufferReadHandler(void *outputBuffer, void *inputBuffer,
                                                  unsigned int nBufferFrames,
                                                  double streamTime,
                                                  RtAudioStreamStatus status,
                                                  void *userData)
        {
          RtAudioOutput *self = static_cast<RtAudioOutput *>(userData);

          // Check if we're stopping before doing anything
          if (!self || self->isStopping_.load(std::memory_order_acquire))
          {
            // Fill with silence and signal stream should stop
            if (outputBuffer)
            {
              memset(outputBuffer, 0,
                     nBufferFrames * sizeof(int16_t) * 2); // Assume max 2 channels
            }
            return 1; // Non-zero return tells RtAudio to stop the stream
          }

          try
          {
            std::lock_guard<decltype(self->mutex_)> lock(self->mutex_);

            const auto bufferSize =
                nBufferFrames * (self->sampleSize_ / 8) * self->channelCount_;

            // Fill with silence if buffer read fails
            qint64 bytesRead = self->audioBuffer_.read(
                reinterpret_cast<char *>(outputBuffer), bufferSize);
            if (bytesRead < static_cast<qint64>(bufferSize))
            {
              // Zero out any remaining bytes
              memset(reinterpret_cast<char *>(outputBuffer) + bytesRead, 0,
                     bufferSize - bytesRead);
            }
            return 0;
          }
          catch (...)
          {
            // On any exception, fill with silence and continue
            if (outputBuffer)
            {
              memset(outputBuffer, 0, nBufferFrames * sizeof(int16_t) * 2);
            }
            return 0; // Return 0 to try to keep stream alive
          }
        }

      } // namespace projection
    } // namespace autoapp
  } // namespace openauto
} // namespace f1x
