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

#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/Projection/AudioDeviceList.hpp>

#if __has_include(<rtaudio/RtAudio.h>)
#include <rtaudio/RtAudio.h>
#elif __has_include(<RtAudio.h>)
#include <RtAudio.h>
#endif

#include <algorithm>

namespace f1x::openauto::autoapp::projection {

std::vector<AudioDeviceInfo> AudioDeviceList::getOutputDevices() {
  std::vector<AudioDeviceInfo> devices;

  try {
    // Prefer ALSA backend for lower latency
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);

    std::unique_ptr<RtAudio> dac;
    if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_ALSA) !=
        apis.end()) {
      dac = std::make_unique<RtAudio>(RtAudio::LINUX_ALSA);
    } else {
      dac = std::make_unique<RtAudio>();
    }

    uint32_t deviceCount = dac->getDeviceCount();
    uint32_t defaultDevice = dac->getDefaultOutputDevice();

    OPENAUTO_LOG(info) << "[AudioDeviceList] Found " << deviceCount
                       << " audio devices";

    for (uint32_t i = 0; i < deviceCount; i++) {
#if defined(RTAUDIO_VERSION_MAJOR) && (RTAUDIO_VERSION_MAJOR >= 6)
      RtAudio::DeviceInfo info = dac->getDeviceInfo(i);
#else
      RtAudio::DeviceInfo info;
      try {
        info = dac->getDeviceInfo(i);
      } catch (const RtAudioError &e) {
        OPENAUTO_LOG(warning) << "[AudioDeviceList] Error getting device " << i
                              << " info: " << e.what();
        continue;
      }
#endif

      // Only include devices that support output
      if (info.outputChannels > 0) {
        AudioDeviceInfo deviceInfo;
        deviceInfo.id = i;
        deviceInfo.name = info.name;
        deviceInfo.isDefault = (i == defaultDevice);
        deviceInfo.outputChannels = info.outputChannels;
        deviceInfo.inputChannels = info.inputChannels;

        OPENAUTO_LOG(debug)
            << "[AudioDeviceList] Output Device " << i << ": " << info.name
            << " (outputs: " << info.outputChannels << ")"
            << (deviceInfo.isDefault ? " [DEFAULT]" : "");

        devices.push_back(deviceInfo);
      }
    }
  } catch (const std::exception &e) {
    OPENAUTO_LOG(error) << "[AudioDeviceList] Error enumerating devices: "
                        << e.what();
  }

  return devices;
}

std::vector<AudioDeviceInfo> AudioDeviceList::getInputDevices() {
  std::vector<AudioDeviceInfo> devices;

  try {
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);

    std::unique_ptr<RtAudio> dac;
    if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_ALSA) !=
        apis.end()) {
      dac = std::make_unique<RtAudio>(RtAudio::LINUX_ALSA);
    } else {
      dac = std::make_unique<RtAudio>();
    }

    uint32_t deviceCount = dac->getDeviceCount();
    uint32_t defaultDevice = dac->getDefaultInputDevice();

    OPENAUTO_LOG(info) << "[AudioDeviceList] Found " << deviceCount
                       << " audio devices";

    for (uint32_t i = 0; i < deviceCount; i++) {
#if defined(RTAUDIO_VERSION_MAJOR) && (RTAUDIO_VERSION_MAJOR >= 6)
      RtAudio::DeviceInfo info = dac->getDeviceInfo(i);
#else
      RtAudio::DeviceInfo info;
      try {
        info = dac->getDeviceInfo(i);
      } catch (const RtAudioError &e) {
        continue;
      }
#endif

      // Only include devices that support input
      if (info.inputChannels > 0) {
        AudioDeviceInfo deviceInfo;
        deviceInfo.id = i;
        deviceInfo.name = info.name;
        deviceInfo.isDefault = (i == defaultDevice);
        deviceInfo.outputChannels = info.outputChannels;
        deviceInfo.inputChannels = info.inputChannels;

        OPENAUTO_LOG(debug)
            << "[AudioDeviceList] Input Device " << i << ": " << info.name
            << " (inputs: " << info.inputChannels << ")"
            << (deviceInfo.isDefault ? " [DEFAULT]" : "");

        devices.push_back(deviceInfo);
      }
    }
  } catch (const std::exception &e) {
    OPENAUTO_LOG(error) << "[AudioDeviceList] Error enumerating input devices: "
                        << e.what();
  }

  return devices;
}

uint32_t AudioDeviceList::findOutputDeviceByName(const std::string &name) {
  if (name.empty()) {
    return getDefaultOutputDeviceId();
  }

  auto devices = getOutputDevices();
  for (const auto &device : devices) {
    if (device.name == name) {
      OPENAUTO_LOG(info) << "[AudioDeviceList] Found output device by name: "
                         << name << " (ID: " << device.id << ")";
      return device.id;
    }
  }

  OPENAUTO_LOG(warning) << "[AudioDeviceList] Output device not found by name: "
                        << name << ", using default";
  return getDefaultOutputDeviceId();
}

uint32_t AudioDeviceList::findInputDeviceByName(const std::string &name) {
  if (name.empty()) {
    return getDefaultInputDeviceId();
  }

  auto devices = getInputDevices();
  for (const auto &device : devices) {
    if (device.name == name) {
      OPENAUTO_LOG(info) << "[AudioDeviceList] Found input device by name: "
                         << name << " (ID: " << device.id << ")";
      return device.id;
    }
  }

  OPENAUTO_LOG(warning) << "[AudioDeviceList] Input device not found by name: "
                        << name << ", using default";
  return getDefaultInputDeviceId();
}

uint32_t AudioDeviceList::getDefaultOutputDeviceId() {
  try {
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);

    std::unique_ptr<RtAudio> dac;
    if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_ALSA) !=
        apis.end()) {
      dac = std::make_unique<RtAudio>(RtAudio::LINUX_ALSA);
    } else {
      dac = std::make_unique<RtAudio>();
    }

    return dac->getDefaultOutputDevice();
  } catch (const std::exception &e) {
    OPENAUTO_LOG(error)
        << "[AudioDeviceList] Error getting default output device: "
        << e.what();
    return 0;
  }
}

uint32_t AudioDeviceList::getDefaultInputDeviceId() {
  try {
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);

    std::unique_ptr<RtAudio> dac;
    if (std::find(apis.begin(), apis.end(), RtAudio::LINUX_ALSA) !=
        apis.end()) {
      dac = std::make_unique<RtAudio>(RtAudio::LINUX_ALSA);
    } else {
      dac = std::make_unique<RtAudio>();
    }

    return dac->getDefaultInputDevice();
  } catch (const std::exception &e) {
    OPENAUTO_LOG(error)
        << "[AudioDeviceList] Error getting default input device: " << e.what();
    return 0;
  }
}

} // namespace f1x::openauto::autoapp::projection
