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

        OPENAUTO_LOG(debug)
            << "[AudioDeviceList] Device " << i << ": " << info.name
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

uint32_t AudioDeviceList::findDeviceByName(const std::string &name) {
  if (name.empty()) {
    return getDefaultDeviceId();
  }

  auto devices = getOutputDevices();
  for (const auto &device : devices) {
    if (device.name == name) {
      OPENAUTO_LOG(info) << "[AudioDeviceList] Found device by name: " << name
                         << " (ID: " << device.id << ")";
      return device.id;
    }
  }

  OPENAUTO_LOG(warning) << "[AudioDeviceList] Device not found by name: "
                        << name << ", using default";
  return getDefaultDeviceId();
}

uint32_t AudioDeviceList::getDefaultDeviceId() {
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
    OPENAUTO_LOG(error) << "[AudioDeviceList] Error getting default device: "
                        << e.what();
    return 0;
  }
}

} // namespace f1x::openauto::autoapp::projection
