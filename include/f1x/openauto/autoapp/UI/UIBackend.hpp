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

                    // ========== Clock/Time Properties ==========
                    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)
                    Q_PROPERTY(QString currentDate READ currentDate NOTIFY currentTimeChanged)
                    Q_PROPERTY(QString amPm READ amPm NOTIFY currentTimeChanged)
                    Q_PROPERTY(bool use24HourFormat READ use24HourFormat WRITE setUse24HourFormat NOTIFY use24HourFormatChanged)

                    // ========== Network/Status Properties ==========
                    Q_PROPERTY(QString networkSSID READ networkSSID NOTIFY networkChanged)
                    Q_PROPERTY(bool bluetoothConnected READ bluetoothConnected NOTIFY bluetoothChanged)
                    Q_PROPERTY(bool wifiConnected READ wifiConnected NOTIFY networkChanged)
                    Q_PROPERTY(QString wifiIP READ wifiIP NOTIFY networkChanged)

                    // ========== General Settings ==========
                    Q_PROPERTY(bool showClock READ showClock WRITE setShowClock NOTIFY settingsChanged)
                    Q_PROPERTY(bool showBigClock READ showBigClock WRITE setShowBigClock NOTIFY settingsChanged)
                    Q_PROPERTY(bool showCursor READ showCursor WRITE setShowCursor NOTIFY settingsChanged)
                    Q_PROPERTY(bool hideWarning READ hideWarning WRITE setHideWarning NOTIFY settingsChanged)
                    Q_PROPERTY(bool hideMenuToggle READ hideMenuToggle WRITE setHideMenuToggle NOTIFY settingsChanged)
                    Q_PROPERTY(bool showNetworkinfo READ showNetworkinfo WRITE setShowNetworkinfo NOTIFY settingsChanged)
                    Q_PROPERTY(bool showLux READ showLux WRITE setShowLux NOTIFY settingsChanged)
                    Q_PROPERTY(int alphaTrans READ alphaTrans WRITE setAlphaTrans NOTIFY settingsChanged)
                    Q_PROPERTY(bool leftHandDrive READ leftHandDrive WRITE setLeftHandDrive NOTIFY settingsChanged)

                    // ========== Video Settings ==========
                    Q_PROPERTY(QString videoResolution READ videoResolution NOTIFY videoResolutionChanged)
                    Q_PROPERTY(int videoFPS READ videoFPS WRITE setVideoFPS NOTIFY settingsChanged)
                    Q_PROPERTY(int screenDPI READ screenDPI WRITE setScreenDPI NOTIFY settingsChanged)
                    Q_PROPERTY(int videoMarginWidth READ videoMarginWidth NOTIFY settingsChanged)
                    Q_PROPERTY(int videoMarginHeight READ videoMarginHeight NOTIFY settingsChanged)
                    Q_PROPERTY(int omxLayerIndex READ omxLayerIndex NOTIFY settingsChanged)

                    // ========== Audio Settings ==========
                    Q_PROPERTY(QString audioOutputDevice READ audioOutputDevice NOTIFY settingsChanged)
                    Q_PROPERTY(QString audioInputDevice READ audioInputDevice NOTIFY settingsChanged)
                    Q_PROPERTY(bool musicChannelEnabled READ musicChannelEnabled WRITE setMusicChannelEnabled NOTIFY settingsChanged)
                    Q_PROPERTY(bool guidanceChannelEnabled READ guidanceChannelEnabled WRITE setGuidanceChannelEnabled NOTIFY settingsChanged)
                    Q_PROPERTY(bool telephonyChannelEnabled READ telephonyChannelEnabled WRITE setTelephonyChannelEnabled NOTIFY settingsChanged)
                    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
                    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)

                    // ========== Input Settings ==========
                    Q_PROPERTY(bool touchscreenEnabled READ touchscreenEnabled WRITE setTouchscreenEnabled NOTIFY settingsChanged)
                    Q_PROPERTY(bool playerButtonControl READ playerButtonControl WRITE setPlayerButtonControl NOTIFY settingsChanged)

                    // ========== Bluetooth Settings ==========
                    Q_PROPERTY(QString bluetoothAdapter READ bluetoothAdapter NOTIFY settingsChanged)
                    Q_PROPERTY(bool bluetoothAutoPair READ bluetoothAutoPair WRITE setBluetoothAutoPair NOTIFY settingsChanged)

                    // ========== WiFi Settings ==========
                    Q_PROPERTY(bool wirelessProjectionEnabled READ wirelessProjectionEnabled WRITE setWirelessProjectionEnabled NOTIFY settingsChanged)
                    Q_PROPERTY(bool hotspotEnabled READ hotspotEnabled WRITE setHotspotEnabled NOTIFY settingsChanged)

                    // ========== System Info (read-only, updated by timer) ==========
                    Q_PROPERTY(QString freeMemory READ freeMemory NOTIFY systemInfoChanged)
                    Q_PROPERTY(QString cpuFrequency READ cpuFrequency NOTIFY systemInfoChanged)
                    Q_PROPERTY(QString cpuTemperature READ cpuTemperature NOTIFY systemInfoChanged)
                    Q_PROPERTY(int disconnectTimeout READ disconnectTimeout WRITE setDisconnectTimeout NOTIFY settingsChanged)
                    Q_PROPERTY(int shutdownTimeout READ shutdownTimeout WRITE setShutdownTimeout NOTIFY settingsChanged)
                    Q_PROPERTY(bool disableShutdown READ disableShutdown WRITE setDisableShutdown NOTIFY settingsChanged)
                    Q_PROPERTY(bool disableScreenOff READ disableScreenOff WRITE setDisableScreenOff NOTIFY settingsChanged)
                    Q_PROPERTY(bool debugMode READ debugMode WRITE setDebugMode NOTIFY settingsChanged)

                    // ========== About Info ==========
                    Q_PROPERTY(QString versionString READ versionString CONSTANT)
                    Q_PROPERTY(QString buildDate READ buildDate CONSTANT)
                    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)

                    // ========== Music Player ==========
                    Q_PROPERTY(QString trackTitle READ trackTitle NOTIFY musicChanged)
                    Q_PROPERTY(QString albumName READ albumName NOTIFY musicChanged)
                    Q_PROPERTY(QString artistName READ artistName NOTIFY musicChanged)
                    Q_PROPERTY(QString albumArtPath READ albumArtPath NOTIFY musicChanged)
                    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY musicChanged)

                public:
                    explicit UIBackend(configuration::IConfiguration::Pointer configuration,
                                       QObject *parent = nullptr);
                    ~UIBackend() override;

                    // ========== Clock/Time Getters ==========
                    QString currentTime() const;
                    QString currentDate() const;
                    QString amPm() const;
                    bool use24HourFormat() const;

                    // ========== Network/Status Getters ==========
                    QString networkSSID() const;
                    bool bluetoothConnected() const;
                    bool wifiConnected() const;
                    QString wifiIP() const;

                    // ========== General Settings Getters ==========
                    bool showClock() const;
                    bool showBigClock() const;
                    bool showCursor() const;
                    bool hideWarning() const;
                    bool hideMenuToggle() const;
                    bool showNetworkinfo() const;
                    bool showLux() const;
                    int alphaTrans() const;
                    bool leftHandDrive() const;

                    // ========== Video Settings Getters ==========
                    QString videoResolution() const;
                    int videoFPS() const;
                    int screenDPI() const;
                    int videoMarginWidth() const;
                    int videoMarginHeight() const;
                    int omxLayerIndex() const;

                    // ========== Audio Settings Getters ==========
                    QString audioOutputDevice() const;
                    QString audioInputDevice() const;
                    bool musicChannelEnabled() const;
                    bool guidanceChannelEnabled() const;
                    bool telephonyChannelEnabled() const;
                    int brightness() const;
                    int volume() const;

                    // ========== Input Settings Getters ==========
                    bool touchscreenEnabled() const;
                    bool playerButtonControl() const;

                    // ========== Bluetooth Getters ==========
                    QString bluetoothAdapter() const;
                    bool bluetoothAutoPair() const;

                    // ========== WiFi Getters ==========
                    bool wirelessProjectionEnabled() const;
                    bool hotspotEnabled() const;

                    // ========== System Info Getters ==========
                    QString freeMemory() const;
                    QString cpuFrequency() const;
                    QString cpuTemperature() const;
                    int disconnectTimeout() const;
                    int shutdownTimeout() const;
                    bool disableShutdown() const;
                    bool disableScreenOff() const;
                    bool debugMode() const;

                    // ========== About Getters ==========
                    QString versionString() const;
                    QString buildDate() const;
                    QString qtVersion() const;

                    // ========== Music Getters ==========
                    QString trackTitle() const;
                    QString albumName() const;
                    QString artistName() const;
                    QString albumArtPath() const;
                    bool isPlaying() const;

                    // ========== Setters (Q_INVOKABLE for QML) ==========
                    Q_INVOKABLE void setUse24HourFormat(bool value);
                    Q_INVOKABLE void setShowClock(bool value);
                    Q_INVOKABLE void setShowBigClock(bool value);
                    Q_INVOKABLE void setShowCursor(bool value);
                    Q_INVOKABLE void setHideWarning(bool value);
                    Q_INVOKABLE void setHideMenuToggle(bool value);
                    Q_INVOKABLE void setShowNetworkinfo(bool value);
                    Q_INVOKABLE void setShowLux(bool value);
                    Q_INVOKABLE void setAlphaTrans(int value);
                    Q_INVOKABLE void setLeftHandDrive(bool value);
                    Q_INVOKABLE void setVideoResolution(const QString &value);
                    Q_INVOKABLE void setVideoFPS(int value);
                    Q_INVOKABLE void setScreenDPI(int value);
                    Q_INVOKABLE void setMusicChannelEnabled(bool value);
                    Q_INVOKABLE void setGuidanceChannelEnabled(bool value);
                    Q_INVOKABLE void setTelephonyChannelEnabled(bool value);
                    Q_INVOKABLE void setTouchscreenEnabled(bool value);
                    Q_INVOKABLE void setPlayerButtonControl(bool value);
                    Q_INVOKABLE void setBluetoothAutoPair(bool value);
                    Q_INVOKABLE void setWirelessProjectionEnabled(bool value);
                    Q_INVOKABLE void setHotspotEnabled(bool value);
                    Q_INVOKABLE void setDisconnectTimeout(int value);
                    Q_INVOKABLE void setShutdownTimeout(int value);
                    Q_INVOKABLE void setDisableShutdown(bool value);
                    Q_INVOKABLE void setDisableScreenOff(bool value);
                    Q_INVOKABLE void setDebugMode(bool value);
                    void setBrightness(int value);
                    void setVolume(int value);

                    // ========== Action Methods ==========
                    Q_INVOKABLE void startAndroidAutoUSB();
                    Q_INVOKABLE void startAndroidAutoWiFi();
                    Q_INVOKABLE void openSettings();
                    Q_INVOKABLE void openMediaPlayer();
                    Q_INVOKABLE void toggleDayNight();
                    Q_INVOKABLE void exitApp();
                    Q_INVOKABLE void goBack();
                    Q_INVOKABLE void saveSettings();
                    Q_INVOKABLE void resetSettings();
                    Q_INVOKABLE void unpairAll();

                    // ========== Music Control Methods ==========
                    Q_INVOKABLE void previousTrack();
                    Q_INVOKABLE void togglePlayPause();
                    Q_INVOKABLE void nextTrack();

                    // ========== Status Update Methods (called from main app) ==========
                    void setNetworkSSID(const QString &ssid);
                    void setBluetoothConnected(bool connected);
                    void setWifiConnected(bool connected);
                    void setWifiIP(const QString &ip);

                    // ========== Music Update Methods (called from media player) ==========
                    void setTrackTitle(const QString &title);
                    void setAlbumName(const QString &album);
                    void setArtistName(const QString &artist);
                    void setAlbumArtPath(const QString &path);
                    void setIsPlaying(bool playing);

                signals:
                    // Property change signals
                    void currentTimeChanged();
                    void use24HourFormatChanged();
                    void networkChanged();
                    void bluetoothChanged();
                    void videoResolutionChanged();
                    void brightnessChanged();
                    void volumeChanged();
                    void settingsChanged();
                    void systemInfoChanged();
                    void musicChanged();

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
                    void requestPreviousTrack();
                    void requestTogglePlayPause();
                    void requestNextTrack();
                    void requestUnpairAll();

                private slots:
                    void updateClock();
                    void updateSystemInfo();

                private:
                    configuration::IConfiguration::Pointer configuration_;
                    QTimer *clockTimer_;
                    QTimer *systemInfoTimer_;

                    // Cached values
                    QString currentTime_;
                    QString networkSSID_;
                    QString wifiIP_;
                    bool bluetoothConnected_;
                    bool wifiConnected_;
                    int brightness_;
                    int volume_;
                    bool use24HourFormat_;

                    // System info cache
                    QString freeMemory_;
                    QString cpuFrequency_;
                    QString cpuTemperature_;

                    // System settings cache (read from crankshaft env)
                    int disconnectTimeout_;
                    int shutdownTimeout_;
                    bool disableShutdown_;
                    bool disableScreenOff_;
                    bool debugMode_;
                    bool hotspotEnabled_;
                    bool bluetoothAutoPair_;

                    // Music player state
                    QString trackTitle_;
                    QString albumName_;
                    QString artistName_;
                    QString albumArtPath_;
                    bool isPlaying_;
                };

            } // namespace ui
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x
