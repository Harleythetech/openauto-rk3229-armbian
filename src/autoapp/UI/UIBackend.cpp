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

#include <QDateTime>
#include <QFile>
#include <QProcess>
#include <QCoreApplication>
#include <QNetworkInterface>
#include <sys/sysinfo.h>
#include <f1x/openauto/autoapp/UI/UIBackend.hpp>
#include <f1x/openauto/Common/Log.hpp>

namespace f1x
{
    namespace openauto
    {
        namespace autoapp
        {
            namespace ui
            {

                UIBackend::UIBackend(configuration::IConfiguration::Pointer configuration,
                                     QObject *parent)
                    : QObject(parent), configuration_(std::move(configuration)), clockTimer_(new QTimer(this)), systemInfoTimer_(new QTimer(this)), currentTime_("00:00"), networkSSID_(""), wifiIP_(""), bluetoothConnected_(false), wifiConnected_(false), brightness_(50), volume_(80), use24HourFormat_(true), freeMemory_("N/A"), cpuFrequency_("N/A"), cpuTemperature_("N/A"), disconnectTimeout_(60), shutdownTimeout_(0), disableShutdown_(false), disableScreenOff_(false), debugMode_(false), hotspotEnabled_(false), bluetoothAutoPair_(false), trackTitle_(""), albumName_(""), artistName_(""), albumArtPath_(""), isPlaying_(false)
                {
                    // Update clock every second
                    connect(clockTimer_, &QTimer::timeout, this, &UIBackend::updateClock);
                    clockTimer_->start(1000);
                    updateClock();

                    // Update system info every 5 seconds
                    connect(systemInfoTimer_, &QTimer::timeout, this, &UIBackend::updateSystemInfo);
                    systemInfoTimer_->start(5000);
                    updateSystemInfo();

                    // Load crankshaft environment values if available
                    if (configuration_)
                    {
                        QString discTimeout = configuration_->getCSValue("DISCONNECTION_SCREEN_POWEROFF_SECS");
                        if (!discTimeout.isEmpty())
                            disconnectTimeout_ = discTimeout.toInt();

                        QString shutTimeout = configuration_->getCSValue("DISCONNECTION_POWEROFF_MINS");
                        if (!shutTimeout.isEmpty())
                            shutdownTimeout_ = shutTimeout.toInt();

                        QString disShutdown = configuration_->getCSValue("DISCONNECTION_POWEROFF_DISABLE");
                        disableShutdown_ = (disShutdown == "1");

                        QString disScreen = configuration_->getCSValue("DISCONNECTION_SCREEN_POWEROFF_DISABLE");
                        disableScreenOff_ = (disScreen == "1");

                        QString debug = configuration_->getCSValue("DEBUG_MODE");
                        debugMode_ = (debug == "1");

                        QString hotspot = configuration_->getCSValue("ENABLE_HOTSPOT");
                        hotspotEnabled_ = (hotspot == "1");

                        QString autopair = configuration_->getCSValue("ENABLE_PAIRABLE");
                        bluetoothAutoPair_ = (autopair == "1");
                    }

                    OPENAUTO_LOG(info) << "[UIBackend] Initialized with full property support";
                }

                UIBackend::~UIBackend()
                {
                    clockTimer_->stop();
                    systemInfoTimer_->stop();
                }

                // ========== Clock/Time Getters ==========
                QString UIBackend::currentTime() const
                {
                    if (use24HourFormat_)
                        return QDateTime::currentDateTime().toString("HH:mm");
                    else
                        return QDateTime::currentDateTime().toString("h:mm");
                }

                QString UIBackend::currentDate() const
                {
                    return QDateTime::currentDateTime().toString("MMMM d, yyyy");
                }

                QString UIBackend::amPm() const
                {
                    if (use24HourFormat_)
                        return "";
                    else
                        return QDateTime::currentDateTime().toString("AP");
                }

                bool UIBackend::use24HourFormat() const
                {
                    return use24HourFormat_;
                }

                // ========== Network/Status Getters ==========
                QString UIBackend::networkSSID() const
                {
                    return networkSSID_;
                }

                bool UIBackend::bluetoothConnected() const
                {
                    return bluetoothConnected_;
                }

                bool UIBackend::wifiConnected() const
                {
                    return wifiConnected_;
                }

                QString UIBackend::wifiIP() const
                {
                    if (!wifiIP_.isEmpty())
                        return wifiIP_;

                    // Try to get IP from network interfaces
                    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
                    for (const QHostAddress &address : addresses)
                    {
                        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback())
                            return address.toString();
                    }
                    return "N/A";
                }

                // ========== General Settings Getters ==========
                bool UIBackend::showClock() const
                {
                    return configuration_ ? configuration_->showClock() : true;
                }

                bool UIBackend::showBigClock() const
                {
                    return configuration_ ? configuration_->showBigClock() : false;
                }

                bool UIBackend::showCursor() const
                {
                    return configuration_ ? configuration_->showCursor() : false;
                }

                bool UIBackend::hideWarning() const
                {
                    return configuration_ ? configuration_->hideWarning() : false;
                }

                bool UIBackend::hideMenuToggle() const
                {
                    return configuration_ ? configuration_->hideMenuToggle() : false;
                }

                bool UIBackend::showNetworkinfo() const
                {
                    return configuration_ ? configuration_->showNetworkinfo() : true;
                }

                bool UIBackend::showLux() const
                {
                    return configuration_ ? configuration_->showLux() : false;
                }

                int UIBackend::alphaTrans() const
                {
                    return configuration_ ? static_cast<int>(configuration_->getAlphaTrans()) : 100;
                }

                bool UIBackend::leftHandDrive() const
                {
                    if (!configuration_)
                        return true;
                    return configuration_->getHandednessOfTrafficType() == configuration::HandednessOfTrafficType::LEFT_HAND_DRIVE;
                }

                // ========== Video Settings Getters ==========
                QString UIBackend::videoResolution() const
                {
                    if (!configuration_)
                        return "1280x720";

                    auto resolution = configuration_->getVideoResolution();
                    switch (resolution)
                    {
                    case aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_800x480:
                        return "800x480";
                    case aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_1280x720:
                        return "1280x720";
                    case aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_1920x1080:
                        return "1920x1080";
                    default:
                        return "1280x720";
                    }
                }

                int UIBackend::videoFPS() const
                {
                    if (!configuration_)
                        return 30;

                    auto fps = configuration_->getVideoFPS();
                    switch (fps)
                    {
                    case aap_protobuf::service::media::sink::message::VideoFrameRateType::VIDEO_FPS_30:
                        return 30;
                    case aap_protobuf::service::media::sink::message::VideoFrameRateType::VIDEO_FPS_60:
                        return 60;
                    default:
                        return 30;
                    }
                }

                int UIBackend::screenDPI() const
                {
                    return configuration_ ? static_cast<int>(configuration_->getScreenDPI()) : 160;
                }

                int UIBackend::videoMarginWidth() const
                {
                    if (!configuration_)
                        return 0;
                    return configuration_->getVideoMargins().width();
                }

                int UIBackend::videoMarginHeight() const
                {
                    if (!configuration_)
                        return 0;
                    return configuration_->getVideoMargins().height();
                }

                int UIBackend::omxLayerIndex() const
                {
                    return configuration_ ? configuration_->getOMXLayerIndex() : 0;
                }

                // ========== Audio Settings Getters ==========
                QString UIBackend::audioOutputDevice() const
                {
                    if (!configuration_)
                        return "Default";
                    std::string device = configuration_->getAudioOutputDeviceName();
                    return device.empty() ? "Default" : QString::fromStdString(device);
                }

                QString UIBackend::audioInputDevice() const
                {
                    if (!configuration_)
                        return "Default";
                    std::string device = configuration_->getAudioInputDeviceName();
                    return device.empty() ? "Default" : QString::fromStdString(device);
                }

                bool UIBackend::musicChannelEnabled() const
                {
                    return configuration_ ? configuration_->musicAudioChannelEnabled() : true;
                }

                bool UIBackend::guidanceChannelEnabled() const
                {
                    return configuration_ ? configuration_->guidanceAudioChannelEnabled() : true;
                }

                bool UIBackend::telephonyChannelEnabled() const
                {
                    return configuration_ ? configuration_->telephonyAudioChannelEnabled() : true;
                }

                int UIBackend::brightness() const
                {
                    return brightness_;
                }

                int UIBackend::volume() const
                {
                    return volume_;
                }

                // ========== Input Settings Getters ==========
                bool UIBackend::touchscreenEnabled() const
                {
                    return configuration_ ? configuration_->getTouchscreenEnabled() : true;
                }

                bool UIBackend::playerButtonControl() const
                {
                    return configuration_ ? configuration_->playerButtonControl() : false;
                }

                // ========== Bluetooth Getters ==========
                QString UIBackend::bluetoothAdapter() const
                {
                    if (!configuration_)
                        return "None";
                    auto type = configuration_->getBluetoothAdapterType();
                    switch (type)
                    {
                    case configuration::BluetoothAdapterType::NONE:
                        return "None";
                    case configuration::BluetoothAdapterType::LOCAL:
                        return "Local";
                    case configuration::BluetoothAdapterType::EXTERNAL:
                        return "External";
                    default:
                        return "Unknown";
                    }
                }

                bool UIBackend::bluetoothAutoPair() const
                {
                    return bluetoothAutoPair_;
                }

                // ========== WiFi Getters ==========
                bool UIBackend::wirelessProjectionEnabled() const
                {
                    return configuration_ ? configuration_->getWirelessProjectionEnabled() : true;
                }

                bool UIBackend::hotspotEnabled() const
                {
                    return hotspotEnabled_;
                }

                // ========== System Info Getters ==========
                QString UIBackend::freeMemory() const
                {
                    return freeMemory_;
                }

                QString UIBackend::cpuFrequency() const
                {
                    return cpuFrequency_;
                }

                QString UIBackend::cpuTemperature() const
                {
                    return cpuTemperature_;
                }

                int UIBackend::disconnectTimeout() const
                {
                    return disconnectTimeout_;
                }

                int UIBackend::shutdownTimeout() const
                {
                    return shutdownTimeout_;
                }

                bool UIBackend::disableShutdown() const
                {
                    return disableShutdown_;
                }

                bool UIBackend::disableScreenOff() const
                {
                    return disableScreenOff_;
                }

                bool UIBackend::debugMode() const
                {
                    return debugMode_;
                }

                // ========== About Getters ==========
                QString UIBackend::versionString() const
                {
                    return QString::fromStdString("OpenAuto 2026.02.02");
                }

                QString UIBackend::buildDate() const
                {
                    return QString(__DATE__) + " " + QString(__TIME__);
                }

                QString UIBackend::qtVersion() const
                {
                    return QString(qVersion());
                }

                // ========== Music Getters ==========
                QString UIBackend::trackTitle() const
                {
                    return trackTitle_;
                }

                QString UIBackend::albumName() const
                {
                    return albumName_;
                }

                QString UIBackend::artistName() const
                {
                    return artistName_;
                }

                QString UIBackend::albumArtPath() const
                {
                    return albumArtPath_;
                }

                bool UIBackend::isPlaying() const
                {
                    return isPlaying_;
                }

                // ========== Setters ==========
                void UIBackend::setUse24HourFormat(bool value)
                {
                    if (use24HourFormat_ != value)
                    {
                        use24HourFormat_ = value;
                        emit use24HourFormatChanged();
                        emit currentTimeChanged();
                    }
                }

                void UIBackend::setShowClock(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->showClock(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setShowBigClock(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->showBigClock(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setShowCursor(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->showCursor(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setHideWarning(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->hideWarning(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setHideMenuToggle(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->hideMenuToggle(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setShowNetworkinfo(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->showNetworkinfo(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setShowLux(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->showLux(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setAlphaTrans(int value)
                {
                    if (configuration_)
                    {
                        configuration_->setAlphaTrans(static_cast<size_t>(value));
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setLeftHandDrive(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->setHandednessOfTrafficType(
                            value ? configuration::HandednessOfTrafficType::LEFT_HAND_DRIVE
                                  : configuration::HandednessOfTrafficType::RIGHT_HAND_DRIVE);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setVideoResolution(const QString &value)
                {
                    if (configuration_)
                    {
                        aap_protobuf::service::media::sink::message::VideoCodecResolutionType res;
                        if (value == "800x480")
                            res = aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_800x480;
                        else if (value == "1920x1080")
                            res = aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_1920x1080;
                        else
                            res = aap_protobuf::service::media::sink::message::VideoCodecResolutionType::VIDEO_1280x720;

                        configuration_->setVideoResolution(res);
                        configuration_->save();
                        emit videoResolutionChanged();
                    }
                }

                void UIBackend::setVideoFPS(int value)
                {
                    if (configuration_)
                    {
                        auto fps = (value == 60)
                                       ? aap_protobuf::service::media::sink::message::VideoFrameRateType::VIDEO_FPS_60
                                       : aap_protobuf::service::media::sink::message::VideoFrameRateType::VIDEO_FPS_30;
                        configuration_->setVideoFPS(fps);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setScreenDPI(int value)
                {
                    if (configuration_)
                    {
                        configuration_->setScreenDPI(static_cast<size_t>(value));
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setMusicChannelEnabled(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->setMusicAudioChannelEnabled(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setGuidanceChannelEnabled(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->setGuidanceAudioChannelEnabled(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setTelephonyChannelEnabled(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->setTelephonyAudioChannelEnabled(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setTouchscreenEnabled(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->setTouchscreenEnabled(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setPlayerButtonControl(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->playerButtonControl(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setBluetoothAutoPair(bool value)
                {
                    bluetoothAutoPair_ = value;
                    emit settingsChanged();
                }

                void UIBackend::setWirelessProjectionEnabled(bool value)
                {
                    if (configuration_)
                    {
                        configuration_->setWirelessProjectionEnabled(value);
                        configuration_->save();
                        emit settingsChanged();
                    }
                }

                void UIBackend::setHotspotEnabled(bool value)
                {
                    hotspotEnabled_ = value;
                    emit settingsChanged();
                }

                void UIBackend::setDisconnectTimeout(int value)
                {
                    disconnectTimeout_ = value;
                    emit settingsChanged();
                }

                void UIBackend::setShutdownTimeout(int value)
                {
                    shutdownTimeout_ = value;
                    emit settingsChanged();
                }

                void UIBackend::setDisableShutdown(bool value)
                {
                    disableShutdown_ = value;
                    emit settingsChanged();
                }

                void UIBackend::setDisableScreenOff(bool value)
                {
                    disableScreenOff_ = value;
                    emit settingsChanged();
                }

                void UIBackend::setDebugMode(bool value)
                {
                    debugMode_ = value;
                    emit settingsChanged();
                }

                void UIBackend::setBrightness(int value)
                {
                    if (brightness_ != value)
                    {
                        brightness_ = value;

                        // Try to set system brightness
                        const char *brightnessPath = "/sys/class/backlight/backlight/brightness";
                        QFile file(brightnessPath);
                        if (file.open(QIODevice::WriteOnly))
                        {
                            file.write(QString::number(value).toUtf8());
                            file.close();
                        }

                        emit brightnessChanged();
                    }
                }

                void UIBackend::setVolume(int value)
                {
                    if (volume_ != value)
                    {
                        volume_ = value;

                        // Set system volume via amixer
                        QString cmd = QString("amixer set Master %1%").arg(value);
                        QProcess::execute("sh", QStringList() << "-c" << cmd);

                        emit volumeChanged();
                    }
                }

                // ========== Action Methods ==========
                void UIBackend::startAndroidAutoUSB()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Start Android Auto USB requested";
                    emit requestAndroidAuto(true);
                }

                void UIBackend::startAndroidAutoWiFi()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Start Android Auto WiFi requested";
                    emit requestAndroidAuto(false);
                }

                void UIBackend::openSettings()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Open settings requested";
                    emit showSettings();
                }

                void UIBackend::openMediaPlayer()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Open media player requested";
                    emit requestOpenMediaPlayer();
                }

                void UIBackend::toggleDayNight()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Toggle day/night requested";
                    emit requestToggleDayNight();
                }

                void UIBackend::exitApp()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Exit app requested";
                    emit exitRequested();
                }

                void UIBackend::goBack()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Go back requested";
                    emit requestGoBack();
                    emit showHome();
                }

                void UIBackend::saveSettings()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Save settings requested";
                    if (configuration_)
                        configuration_->save();
                }

                void UIBackend::resetSettings()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Reset settings requested";
                    if (configuration_)
                    {
                        configuration_->reset();
                        emit settingsChanged();
                    }
                }

                void UIBackend::unpairAll()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Unpair all Bluetooth devices requested";
                    emit requestUnpairAll();
                }

                // ========== Music Control Methods ==========
                void UIBackend::previousTrack()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Previous track requested";
                    emit requestPreviousTrack();
                }

                void UIBackend::togglePlayPause()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Toggle play/pause requested";
                    emit requestTogglePlayPause();
                }

                void UIBackend::nextTrack()
                {
                    OPENAUTO_LOG(info) << "[UIBackend] Next track requested";
                    emit requestNextTrack();
                }

                // ========== Status Update Methods ==========
                void UIBackend::setNetworkSSID(const QString &ssid)
                {
                    if (networkSSID_ != ssid)
                    {
                        networkSSID_ = ssid;
                        emit networkChanged();
                    }
                }

                void UIBackend::setBluetoothConnected(bool connected)
                {
                    if (bluetoothConnected_ != connected)
                    {
                        bluetoothConnected_ = connected;
                        emit bluetoothChanged();
                    }
                }

                void UIBackend::setWifiConnected(bool connected)
                {
                    if (wifiConnected_ != connected)
                    {
                        wifiConnected_ = connected;
                        emit networkChanged();
                    }
                }

                void UIBackend::setWifiIP(const QString &ip)
                {
                    if (wifiIP_ != ip)
                    {
                        wifiIP_ = ip;
                        emit networkChanged();
                    }
                }

                // ========== Music Update Methods ==========
                void UIBackend::setTrackTitle(const QString &title)
                {
                    if (trackTitle_ != title)
                    {
                        trackTitle_ = title;
                        emit musicChanged();
                    }
                }

                void UIBackend::setAlbumName(const QString &album)
                {
                    if (albumName_ != album)
                    {
                        albumName_ = album;
                        emit musicChanged();
                    }
                }

                void UIBackend::setArtistName(const QString &artist)
                {
                    if (artistName_ != artist)
                    {
                        artistName_ = artist;
                        emit musicChanged();
                    }
                }

                void UIBackend::setAlbumArtPath(const QString &path)
                {
                    if (albumArtPath_ != path)
                    {
                        albumArtPath_ = path;
                        emit musicChanged();
                    }
                }

                void UIBackend::setIsPlaying(bool playing)
                {
                    if (isPlaying_ != playing)
                    {
                        isPlaying_ = playing;
                        emit musicChanged();
                    }
                }

                // ========== Timer Slots ==========
                void UIBackend::updateClock()
                {
                    QString newTime = use24HourFormat_
                                          ? QDateTime::currentDateTime().toString("HH:mm")
                                          : QDateTime::currentDateTime().toString("h:mm");
                    if (currentTime_ != newTime)
                    {
                        currentTime_ = newTime;
                        emit currentTimeChanged();
                    }
                }

                void UIBackend::updateSystemInfo()
                {
                    // Free memory
                    struct sysinfo info;
                    if (sysinfo(&info) == 0)
                    {
                        long freeMB = info.freeram / 1024 / 1024;
                        freeMemory_ = QString::number(freeMB) + " MB";
                    }

                    // CPU frequency
                    QFile freqFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq");
                    if (freqFile.open(QIODevice::ReadOnly))
                    {
                        QString freqStr = QString::fromUtf8(freqFile.readAll()).trimmed();
                        int freqKHz = freqStr.toInt();
                        cpuFrequency_ = QString::number(freqKHz / 1000) + " MHz";
                        freqFile.close();
                    }

                    // CPU temperature
                    QFile tempFile("/sys/class/thermal/thermal_zone0/temp");
                    if (tempFile.open(QIODevice::ReadOnly))
                    {
                        QString tempStr = QString::fromUtf8(tempFile.readAll()).trimmed();
                        int tempMilliC = tempStr.toInt();
                        cpuTemperature_ = QString::number(tempMilliC / 1000) + " °C";
                        tempFile.close();
                    }

                    emit systemInfoChanged();
                }

            } // namespace ui
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x
