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

#include <cstdint>
#include <string>
#include <vector>

namespace f1x {
namespace openauto {
namespace autoapp {
namespace projection {

/**
 * Information about an audio output device
 */
struct AudioDeviceInfo {
  uint32_t id;
  std::string name;
  bool isDefault;
  uint32_t outputChannels;
  uint32_t inputChannels;
};

/**
 * Utility class to enumerate audio output/input devices using RTAudio
 */
class AudioDeviceList {
public:
  /**
   * Get a list of all available audio output devices
   * @return Vector of AudioDeviceInfo structs
   */
  static std::vector<AudioDeviceInfo> getOutputDevices();

  /**
   * Get a list of all available audio input devices
   * @return Vector of AudioDeviceInfo structs
   */
  static std::vector<AudioDeviceInfo> getInputDevices();

  /**
   * Find an output device ID by its name
   * @param name The device name to search for
   * @return The device ID, or getDefaultOutputDeviceId() if not found
   */
  static uint32_t findOutputDeviceByName(const std::string &name);

  /**
   * Find an input device ID by its name
   * @param name The device name to search for
   * @return The device ID, or getDefaultInputDeviceId() if not found
   */
  static uint32_t findInputDeviceByName(const std::string &name);

  /**
   * Get the default output device ID
   * @return The default device ID
   */
  static uint32_t getDefaultOutputDeviceId();

  /**
   * Get the default input device ID
   * @return The default device ID
   */
  static uint32_t getDefaultInputDeviceId();
};

} // namespace projection
} // namespace autoapp
} // namespace openauto
} // namespace f1x
