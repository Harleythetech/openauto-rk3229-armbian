#pragma once

#include <atomic>
#include <f1x/openauto/autoapp/Configuration/IConfiguration.hpp>
#include <f1x/openauto/autoapp/Projection/IAudioInput.hpp>
#include <f1x/openauto/autoapp/Projection/LockFreeRingBuffer.hpp>
#include <memory>
#include <mutex>
#include <rtaudio/RtAudio.h>
#include <vector>

namespace f1x
{
  namespace openauto
  {
    namespace autoapp
    {
      namespace projection
      {

        class RtAudioInput : public IAudioInput
        {
        public:
          RtAudioInput(uint32_t channelCount, uint32_t sampleSize, uint32_t sampleRate,
                       configuration::IConfiguration::Pointer configuration);
          ~RtAudioInput() override;

          bool open() override;
          bool isActive() const override;
          void read(ReadPromise::Pointer promise) override;
          void start(StartPromise::Pointer promise) override;
          void stop() override;
          uint32_t getSampleSize() const override;
          uint32_t getChannelCount() const override;
          uint32_t getSampleRate() const override;

        private:
          static int rtAudioCallback(void *outputBuffer, void *inputBuffer,
                                     unsigned int nBufferFrames, double streamTime,
                                     RtAudioStreamStatus status, void *userData);

          uint32_t channelCount_;
          uint32_t sampleSize_;
          uint32_t sampleRate_;
          configuration::IConfiguration::Pointer configuration_;
          std::shared_ptr<RtAudio> rtAudio_;
          ReadPromise::Pointer readPromise_;
          mutable std::mutex mutex_; // Only for promise state, not buffer access
          // Lock-free ring buffer for audio input - 64KB capacity
          // Producer: RT callback, Consumer: read() method
          LockFreeRingBuffer<65536> buffer_;
          bool isActive_;
          std::atomic<bool> isStopping_;
          std::atomic<bool> dataAvailable_{false}; // Signal from RT callback to main thread

          static constexpr size_t cChunkSize =
              2056; // Standard chunk size requested by AA
        };

      } // namespace projection
    } // namespace autoapp
  } // namespace openauto
} // namespace f1x
