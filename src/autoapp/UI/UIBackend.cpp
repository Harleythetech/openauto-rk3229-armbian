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
                    : QObject(parent), configuration_(std::move(configuration)), clockTimer_(new QTimer(this)), currentTime_("00:00"), networkSSID_(""), bluetoothConnected_(false), wifiConnected_(false), brightness_(50), volume_(80)
                {
                    // Update clock every second
                    connect(clockTimer_, &QTimer::timeout, this, &UIBackend::updateClock);
                    clockTimer_->start(1000);
                    updateClock();

                    OPENAUTO_LOG(info) << "[UIBackend] Initialized";
                }

                UIBackend::~UIBackend()
                {
                    clockTimer_->stop();
                }

                QString UIBackend::currentTime() const
                {
                    return currentTime_;
                }

                QString UIBackend::currentDate() const
                {
                    return QDateTime::currentDateTime().toString("MMMM d, yyyy");
                }

                QString UIBackend::amPm() const
                {
                    return QDateTime::currentDateTime().toString("AP");
                }

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

                QString UIBackend::versionString() const
                {
                    // Return version from configuration or build info
                    return QString::fromStdString("OpenAuto 2026.02.01");
                }

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

                int UIBackend::brightness() const
                {
                    return brightness_;
                }

                int UIBackend::volume() const
                {
                    return volume_;
                }

                void UIBackend::setBrightness(int value)
                {
                    if (brightness_ != value)
                    {
                        brightness_ = value;

                        // Try to set system brightness
                        // Common paths for backlight control
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

                void UIBackend::updateClock()
                {
                    QString newTime = QDateTime::currentDateTime().toString("HH:mm");
                    if (currentTime_ != newTime)
                    {
                        currentTime_ = newTime;
                        emit currentTimeChanged();
                    }
                }

            } // namespace ui
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x
