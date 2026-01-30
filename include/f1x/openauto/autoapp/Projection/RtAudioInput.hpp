#pragma once

#include <atomic>
#include <deque>
#include <f1x/openauto/autoapp/Configuration/IConfiguration.hpp>
#include <f1x/openauto/autoapp/Projection/IAudioInput.hpp>
#include <memory>
#include <mutex>
#include <rtaudio/RtAudio.h>
#include <vector>


namespace f1x {
namespace openauto {
namespace autoapp {
namespace projection {

class RtAudioInput : public IAudioInput {
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
  mutable std::mutex mutex_;
  std::deque<uint8_t> buffer_;
  bool isActive_;
  std::atomic<bool> isStopping_;

  static constexpr size_t cChunkSize =
      2056; // Standard chunk size requested by AA
};

} // namespace projection
} // namespace autoapp
} // namespace openauto
} // namespace f1x
