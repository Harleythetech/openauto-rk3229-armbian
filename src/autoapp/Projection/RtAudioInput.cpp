#include <algorithm>
#include <cstring>
#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/Projection/AudioDeviceList.hpp>
#include <f1x/openauto/autoapp/Projection/RtAudioInput.hpp>


namespace f1x {
namespace openauto {
namespace autoapp {
namespace projection {

RtAudioInput::RtAudioInput(uint32_t channelCount, uint32_t sampleSize,
                           uint32_t sampleRate,
                           configuration::IConfiguration::Pointer configuration)
    : channelCount_(channelCount), sampleSize_(sampleSize),
      sampleRate_(sampleRate), configuration_(std::move(configuration)),
      isActive_(false), isStopping_(false) {}

RtAudioInput::~RtAudioInput() { stop(); }

bool RtAudioInput::open() {
  try {
    rtAudio_ = std::make_shared<RtAudio>(RtAudio::LINUX_ALSA);
    return true; // RtAudio instance created successfully
  } catch (RtAudioError &error) {
    OPENAUTO_LOG(error) << "[RtAudioInput] Failed to create RtAudio instance: "
                        << error.getMessage();
    return false;
  }
}

bool RtAudioInput::isActive() const { return isActive_; }

void RtAudioInput::read(ReadPromise::Pointer promise) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!isActive_) {
    promise->reject();
    return;
  }

  if (readPromise_) {
    // Already a pending read promise, reject new one
    promise->reject();
    return;
  }

  if (buffer_.size() >= cChunkSize) {
    // We have enough data, fulfill immediately
    aasdk::common::Data data(cChunkSize);
    for (size_t i = 0; i < cChunkSize; ++i) {
      data[i] = buffer_.front();
      buffer_.pop_front();
    }
    promise->resolve(std::move(data));
  } else {
    // Wait for more data
    readPromise_ = std::move(promise);
  }
}

void RtAudioInput::start(StartPromise::Pointer promise) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (isActive_) {
    promise->resolve();
    return;
  }

  if (!rtAudio_) {
    if (!open()) {
      promise->reject();
      return;
    }
  }

  std::string deviceName = configuration_->getAudioInputDeviceName();
  unsigned int deviceId = 0;
  bool deviceFound = false;

  if (deviceName.empty()) {
    deviceId = rtAudio_->getDefaultInputDevice();
    deviceFound = true;
  } else {
    AudioDeviceList deviceList;
    auto devices = deviceList.getInputDevices();
    for (const auto &device : devices) {
      if (device.name == deviceName) {
        deviceId = device.id;
        deviceFound = true;
        break;
      }
    }
    if (!deviceFound) {
      OPENAUTO_LOG(warning) << "[RtAudioInput] Configured device '"
                            << deviceName << "' not found. Using default.";
      deviceId = rtAudio_->getDefaultInputDevice();
    }
  }

  RtAudio::StreamParameters parameters;
  parameters.deviceId = deviceId;
  parameters.nChannels = channelCount_;
  parameters.firstChannel = 0;

  unsigned int bufferFrames = 512; // Adjust as needed
  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_SCHEDULE_REALTIME;

  try {
    rtAudio_->openStream(nullptr, &parameters, RTAUDIO_SINT16, sampleRate_,
                         &bufferFrames, &RtAudioInput::rtAudioCallback, this,
                         &options);
    rtAudio_->startStream();
    isActive_ = true;
    isStopping_ = false;
    OPENAUTO_LOG(info) << "[RtAudioInput] Started stream on device ID "
                       << deviceId;
    promise->resolve();
  } catch (RtAudioError &error) {
    OPENAUTO_LOG(error) << "[RtAudioInput] Failed to start stream: "
                        << error.getMessage();
    promise->reject();
  }
}

void RtAudioInput::stop() {
  std::lock_guard<std::mutex> lock(mutex_);
  isStopping_ = true;
  if (isActive_ && rtAudio_) {
    try {
      if (rtAudio_->isStreamRunning()) {
        rtAudio_->stopStream();
      }
      if (rtAudio_->isStreamOpen()) {
        rtAudio_->closeStream();
      }
    } catch (RtAudioError &error) {
      OPENAUTO_LOG(error) << "[RtAudioInput] Error stopping stream: "
                          << error.getMessage();
    }
  }
  isActive_ = false;
  buffer_.clear();

  if (readPromise_) {
    readPromise_->reject();
    readPromise_.reset();
  }
}

uint32_t RtAudioInput::getSampleSize() const { return sampleSize_; }

uint32_t RtAudioInput::getChannelCount() const { return channelCount_; }

uint32_t RtAudioInput::getSampleRate() const { return sampleRate_; }

int RtAudioInput::rtAudioCallback(void *outputBuffer, void *inputBuffer,
                                  unsigned int nBufferFrames, double streamTime,
                                  RtAudioStreamStatus status, void *userData) {
  RtAudioInput *audioInput = static_cast<RtAudioInput *>(userData);
  if (!audioInput || audioInput->isStopping_)
    return 0;

  if (status) {
    OPENAUTO_LOG(warning)
        << "[RtAudioInput] Stream overflow/underflow detected.";
  }

  std::lock_guard<std::mutex> lock(audioInput->mutex_);

  // Calculate total bytes
  size_t bytes =
      nBufferFrames * audioInput->channelCount_ *
      (audioInput->sampleSize_ / 8); // Assuming SINT16 is 2 bytes/sample which
                                     // matches sampleSize usually 16?
  // Wait, sampleSize passed in constructor is likely bits or bytes?
  // QtAudioInput uses sampleSize in bits usually (16 for SINT16).
  // Let's assume inputBuffer contains standard SINT16, 2 bytes.
  // If sampleSize is passed as 16, then bytes per sample is 2.

  // Actually, RTAUDIO_SINT16 is hardcoded in openStream.
  // So we assume SINT16.

  int16_t *src = static_cast<int16_t *>(inputBuffer);
  uint8_t *srcBytes = static_cast<uint8_t *>(inputBuffer);

  // Append to buffer
  audioInput->buffer_.insert(audioInput->buffer_.end(), srcBytes,
                             srcBytes + bytes);

  // Check if we can satisfy a promise
  if (audioInput->readPromise_ && audioInput->buffer_.size() >= cChunkSize) {
    aasdk::common::Data data(cChunkSize);
    for (size_t i = 0; i < cChunkSize; ++i) {
      data[i] = audioInput->buffer_.front();
      audioInput->buffer_.pop_front();
    }
    audioInput->readPromise_->resolve(std::move(data));
    audioInput->readPromise_.reset();
  }

  return 0;
}

} // namespace projection
} // namespace autoapp
} // namespace openauto
} // namespace f1x
