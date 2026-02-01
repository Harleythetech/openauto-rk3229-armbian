#include <algorithm>
#include <cstring>
#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/Projection/AudioDeviceList.hpp>
#include <f1x/openauto/autoapp/Projection/RtAudioInput.hpp>

namespace f1x
{
  namespace openauto
  {
    namespace autoapp
    {
      namespace projection
      {

        RtAudioInput::RtAudioInput(uint32_t channelCount, uint32_t sampleSize,
                                   uint32_t sampleRate,
                                   configuration::IConfiguration::Pointer configuration)
            : channelCount_(channelCount), sampleSize_(sampleSize),
              sampleRate_(sampleRate), configuration_(std::move(configuration)),
              isActive_(false), isStopping_(false) {}

        RtAudioInput::~RtAudioInput() { stop(); }

        bool RtAudioInput::open()
        {
          try
          {
            rtAudio_ = std::make_shared<RtAudio>(RtAudio::LINUX_ALSA);
            return true; // RtAudio instance created successfully
          }
          catch (RtAudioError &error)
          {
            OPENAUTO_LOG(error) << "[RtAudioInput] Failed to create RtAudio instance: "
                                << error.getMessage();
            return false;
          }
        }

        bool RtAudioInput::isActive() const { return isActive_; }

        void RtAudioInput::read(ReadPromise::Pointer promise)
        {
          std::lock_guard<std::mutex> lock(mutex_);

          if (!isActive_)
          {
            promise->reject();
            return;
          }

          if (readPromise_)
          {
            // Already a pending read promise, reject new one
            promise->reject();
            return;
          }

          // Check if we have enough data in the lock-free buffer
          if (buffer_.available() >= cChunkSize)
          {
            // We have enough data, fulfill immediately
            aasdk::common::Data data(cChunkSize);
            buffer_.read(data.data(), cChunkSize);
            promise->resolve(std::move(data));
          }
          else
          {
            // Wait for more data - store promise for later fulfillment
            readPromise_ = std::move(promise);
          }
        }

        void RtAudioInput::start(StartPromise::Pointer promise)
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (isActive_)
          {
            promise->resolve();
            return;
          }

          if (!rtAudio_)
          {
            if (!open())
            {
              promise->reject();
              return;
            }
          }

          std::string deviceName = configuration_->getAudioInputDeviceName();
          unsigned int deviceId = 0;
          bool deviceFound = false;

          if (deviceName.empty())
          {
            deviceId = rtAudio_->getDefaultInputDevice();
            deviceFound = true;
          }
          else
          {
            AudioDeviceList deviceList;
            auto devices = deviceList.getInputDevices();
            for (const auto &device : devices)
            {
              if (device.name == deviceName)
              {
                deviceId = device.id;
                deviceFound = true;
                break;
              }
            }
            if (!deviceFound)
            {
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

          try
          {
            rtAudio_->openStream(nullptr, &parameters, RTAUDIO_SINT16, sampleRate_,
                                 &bufferFrames, &RtAudioInput::rtAudioCallback, this,
                                 &options);
            rtAudio_->startStream();
            isActive_ = true;
            isStopping_ = false;
            OPENAUTO_LOG(info) << "[RtAudioInput] Started stream on device ID "
                               << deviceId;
            promise->resolve();
          }
          catch (RtAudioError &error)
          {
            OPENAUTO_LOG(error) << "[RtAudioInput] Failed to start stream: "
                                << error.getMessage();
            promise->reject();
          }
        }

        void RtAudioInput::stop()
        {
          std::lock_guard<std::mutex> lock(mutex_);
          isStopping_ = true;
          if (isActive_ && rtAudio_)
          {
            try
            {
              if (rtAudio_->isStreamRunning())
              {
                rtAudio_->stopStream();
              }
              if (rtAudio_->isStreamOpen())
              {
                rtAudio_->closeStream();
              }
            }
            catch (RtAudioError &error)
            {
              OPENAUTO_LOG(error) << "[RtAudioInput] Error stopping stream: "
                                  << error.getMessage();
            }
          }
          isActive_ = false;
          buffer_.clear();
          dataAvailable_.store(false, std::memory_order_relaxed);

          if (readPromise_)
          {
            readPromise_->reject();
            readPromise_.reset();
          }
        }

        uint32_t RtAudioInput::getSampleSize() const { return sampleSize_; }

        uint32_t RtAudioInput::getChannelCount() const { return channelCount_; }

        uint32_t RtAudioInput::getSampleRate() const { return sampleRate_; }

        int RtAudioInput::rtAudioCallback(void *outputBuffer, void *inputBuffer,
                                          unsigned int nBufferFrames, double streamTime,
                                          RtAudioStreamStatus status, void *userData)
        {
          RtAudioInput *audioInput = static_cast<RtAudioInput *>(userData);
          if (!audioInput || audioInput->isStopping_)
            return 0;

          if (status)
          {
            OPENAUTO_LOG(warning)
                << "[RtAudioInput] Stream overflow/underflow detected.";
          }

          // Calculate total bytes (RTAUDIO_SINT16 = 2 bytes per sample)
          size_t bytes = nBufferFrames * audioInput->channelCount_ * 2;
          uint8_t *srcBytes = static_cast<uint8_t *>(inputBuffer);

          // Write to lock-free ring buffer - NO MUTEX (RT-safe)
          audioInput->buffer_.write(srcBytes, bytes);

          // Signal that data is available
          audioInput->dataAvailable_.store(true, std::memory_order_release);

          // Try to fulfill pending promise without blocking
          // Use try_lock to avoid blocking the RT thread
          std::unique_lock<std::mutex> lock(audioInput->mutex_, std::try_to_lock);
          if (lock.owns_lock() && audioInput->readPromise_ &&
              audioInput->buffer_.available() >= cChunkSize)
          {
            aasdk::common::Data data(cChunkSize);
            audioInput->buffer_.read(data.data(), cChunkSize);
            audioInput->readPromise_->resolve(std::move(data));
            audioInput->readPromise_.reset();
          }

          return 0;
        }

      } // namespace projection
    } // namespace autoapp
  } // namespace openauto
} // namespace f1x
