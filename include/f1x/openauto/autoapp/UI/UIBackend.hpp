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

#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include <f1x/openauto/autoapp/Configuration/IConfiguration.hpp>

namespace f1x
{
    namespace openauto
    {
        namespace autoapp
        {
            namespace ui
            {

                /**
                 * @brief UIBackend - Bridge between QML UI and C++ backend
                 *
                 * Exposes properties and methods to QML for the Automotive HMI interface.
                 * Handles clock updates, system status, and UI actions.
                 */
                class UIBackend : public QObject
                {
                    Q_OBJECT

                    // Read-only status properties
                    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)
                    Q_PROPERTY(QString networkSSID READ networkSSID NOTIFY networkChanged)
                    Q_PROPERTY(bool bluetoothConnected READ bluetoothConnected NOTIFY bluetoothChanged)
                    Q_PROPERTY(bool wifiConnected READ wifiConnected NOTIFY networkChanged)
                    Q_PROPERTY(QString versionString READ versionString CONSTANT)
                    Q_PROPERTY(QString videoResolution READ videoResolution NOTIFY videoResolutionChanged)

                    // Settings properties
                    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
                    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)

                public:
                    explicit UIBackend(configuration::IConfiguration::Pointer configuration,
                                       QObject *parent = nullptr);
                    ~UIBackend() override;

                    // Property getters
                    QString currentTime() const;
                    QString networkSSID() const;
                    bool bluetoothConnected() const;
                    bool wifiConnected() const;
                    QString versionString() const;
                    QString videoResolution() const;
                    int brightness() const;
                    int volume() const;

                    // Property setters
                    void setBrightness(int value);
                    void setVolume(int value);

                    // Methods exposed to QML
                    Q_INVOKABLE void startAndroidAutoUSB();
                    Q_INVOKABLE void startAndroidAutoWiFi();
                    Q_INVOKABLE void openSettings();
                    Q_INVOKABLE void openMediaPlayer();
                    Q_INVOKABLE void toggleDayNight();
                    Q_INVOKABLE void exitApp();
                    Q_INVOKABLE void goBack();

                    // Status update methods (called from main app)
                    void setNetworkSSID(const QString &ssid);
                    void setBluetoothConnected(bool connected);
                    void setWifiConnected(bool connected);

                signals:
                    // Property change signals
                    void currentTimeChanged();
                    void networkChanged();
                    void bluetoothChanged();
                    void videoResolutionChanged();
                    void brightnessChanged();
                    void volumeChanged();

                    // Navigation signals
                    void showSettings();
                    void showHome();

                    // Android Auto lifecycle signals
                    void androidAutoStarted();
                    void androidAutoStopped();

                    // Action requests (handled by main app)
                    void requestAndroidAuto(bool usb);
                    void requestStartAndroidAutoUSB();
                    void requestStartAndroidAutoWiFi();
                    void requestOpenMediaPlayer();
                    void requestToggleDayNight();
                    void exitRequested();
                    void requestGoBack();

                private slots:
                    void updateClock();

                private:
                    configuration::IConfiguration::Pointer configuration_;
                    QTimer *clockTimer_;
                    QString currentTime_;
                    QString networkSSID_;
                    bool bluetoothConnected_;
                    bool wifiConnected_;
                    int brightness_;
                    int volume_;
                };

            } // namespace ui
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x
