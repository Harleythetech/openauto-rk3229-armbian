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

#pragma once

#if __has_include(<rtaudio/RtAudio.h>)
#include <rtaudio/RtAudio.h>
#elif __has_include(<RtAudio.h>)
#include <RtAudio.h>
#endif

#include <atomic>
#include <mutex>
#include <f1x/openauto/autoapp/Projection/IAudioOutput.hpp>
#include <f1x/openauto/autoapp/Projection/LockFreeRingBuffer.hpp>

namespace f1x
{
  namespace openauto
  {
    namespace autoapp
    {
      namespace projection
      {

        class RtAudioOutput : public IAudioOutput
        {
        public:
          /**
           * @brief Construct RtAudioOutput
           * @param channelCount Number of audio channels
           * @param sampleSize Sample size in bits
           * @param sampleRate Sample rate in Hz
           * @param deviceId Device ID to use (0 = use default device)
           */
          RtAudioOutput(uint32_t channelCount, uint32_t sampleSize, uint32_t sampleRate,
                        uint32_t deviceId = 0);
          bool open() override;
          void write(aasdk::messenger::Timestamp::ValueType timestamp,
                     const aasdk::common::DataConstBuffer &buffer) override;
          void start() override;
          void stop() override;
          void suspend() override;
          uint32_t getSampleSize() const override;
          uint32_t getChannelCount() const override;
          uint32_t getSampleRate() const override;

        private:
          void doSuspend();
          static int audioBufferReadHandler(void *outputBuffer, void *inputBuffer,
                                            unsigned int nBufferFrames,
                                            double streamTime,
                                            RtAudioStreamStatus status, void *userData);

          uint32_t channelCount_;
          uint32_t sampleSize_;
          uint32_t sampleRate_;
          uint32_t deviceId_;
          // Lock-free ring buffer for audio - 256KB capacity (power of 2)
          // Allows ~2.7 seconds of 48kHz stereo 16-bit audio
          LockFreeRingBuffer<262144> audioBuffer_;
          std::unique_ptr<RtAudio> dac_;
          std::mutex mutex_; // Only for non-RT operations (open/close/start/stop)
          std::atomic<bool> isStopping_{
              false}; // Atomic flag for safe shutdown during USB disconnect
        };

      } // namespace projection
    } // namespace autoapp
  } // namespace openauto
} // namespace f1x
