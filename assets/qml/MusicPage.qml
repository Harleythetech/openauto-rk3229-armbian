import QtQuick 2.15
import QtQuick.Controls 2.15
import "."

// MusicPage - Music player matching design reference
// Layout: Date/time top-right, centered album art + track info, large controls at bottom

Item {
    id: root

    signal openFileBrowser

    // Gradient background
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Theme.gradientTop
            }
            GradientStop {
                position: 1.0
                color: Theme.gradientBottom
            }
        }
    }

    // Date/Time in top-right corner (matches design: "January 30, 2025 | 00:00AM")
    Text {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.rightMargin: 24
        text: {
            var dateStr = typeof backend !== "undefined" ? backend.currentDate : "February 1, 2026";
            var timeStr = typeof backend !== "undefined" ? backend.currentTime : "00:00";
            var ampmStr = typeof backend !== "undefined" ? backend.amPm : "";
            if (ampmStr !== "")
                return dateStr + " | " + timeStr + ampmStr;
            else
                return dateStr + " | " + timeStr;
        }
        font.pixelSize: Theme.fontSizeLarge
        font.family: Theme.fontFamily
        font.weight: Font.Light
        color: Theme.textPrimary
    }

    // Center content area — album art + track info side by side
    Row {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -40
        spacing: 48

        // Album art — large (250x250)
        Rectangle {
            width: 250
            height: 250
            color: "transparent"
            border.width: 2
            border.color: Qt.rgba(1, 1, 1, 0.3)
            radius: 8

            // Placeholder CD icon
            Rectangle {
                anchors.centerIn: parent
                width: 140
                height: 140
                radius: 70
                color: "transparent"
                border.width: 2
                border.color: Qt.rgba(1, 1, 1, 0.3)
                visible: albumArt.status !== Image.Ready

                Rectangle {
                    anchors.centerIn: parent
                    width: 36
                    height: 36
                    radius: 18
                    color: Qt.rgba(1, 1, 1, 0.3)
                }
            }

            Image {
                id: albumArt
                anchors.fill: parent
                source: {
                    if (typeof audioPlayer !== "undefined" && audioPlayer.albumArtPath !== "")
                        return audioPlayer.albumArtPath;
                    return "";
                }
                fillMode: Image.PreserveAspectCrop
                visible: status === Image.Ready
            }
        }

        // Track info column
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            width: Math.min(root.width - 450, 380)

            // Track title
            Text {
                width: parent.width
                text: {
                    if (typeof audioPlayer !== "undefined" && audioPlayer.trackTitle !== "")
                        return audioPlayer.trackTitle;
                    return "No Track";
                }
                font.pixelSize: Theme.fontSizeXLarge
                font.family: Theme.fontFamily
                font.weight: Font.Normal
                color: Theme.textPrimary
                elide: Text.ElideRight
            }

            // Album name
            Text {
                width: parent.width
                text: {
                    if (typeof audioPlayer !== "undefined" && audioPlayer.albumName !== "")
                        return audioPlayer.albumName;
                    return "No Album";
                }
                font.pixelSize: Theme.fontSizeMedium
                font.family: Theme.fontFamily
                font.weight: Font.Light
                color: Theme.textPrimary
                elide: Text.ElideRight
            }

            // Artist name
            Text {
                width: parent.width
                text: {
                    if (typeof audioPlayer !== "undefined" && audioPlayer.artistName !== "")
                        return audioPlayer.artistName;
                    return "Unknown Artist";
                }
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
                font.weight: Font.Light
                color: Theme.textSecondary
                elide: Text.ElideRight
            }

            // Audio quality info (below artist)
            Text {
                width: parent.width
                text: {
                    var parts = [];
                    if (typeof audioPlayer !== "undefined" && audioPlayer.playlistCount > 0)
                        parts.push((audioPlayer.playlistIndex + 1) + " / " + audioPlayer.playlistCount);
                    if (typeof audioPlayer !== "undefined" && audioPlayer.sampleRate > 0) {
                        var rateKhz = (audioPlayer.sampleRate / 1000).toFixed(1) + "kHz";
                        var depth = audioPlayer.bitDepth + "-bit";
                        var quality = rateKhz + " / " + depth;
                        if (audioPlayer.nativeOffload)
                            quality += " · Offload";
                        parts.push(quality);
                    }
                    return parts.join("   ·   ");
                }
                font.pixelSize: Theme.fontSizeXSmall
                font.family: Theme.fontFamily
                font.weight: Font.Light
                color: Theme.textSecondary
                visible: text !== ""
            }
        }
    }

    // Media controls row: Folder | Prev | Play/Pause | Next | Repeat
    // Large controls matching design reference (~64px)
    Row {
        id: controlsRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 100
        spacing: 64

        // Folder button - opens file browser
        Image {
            width: 48
            height: 48
            source: "qrc:/File.png"
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                anchors.margins: -20
                onClicked: root.openFileBrowser()
            }
        }

        // Previous button
        Image {
            width: 64
            height: 64
            source: "qrc:/prev-hot.png"
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: {
                    if (typeof audioPlayer !== "undefined")
                        audioPlayer.previousTrack();
                }
            }
        }

        // Play/Pause button (largest)
        Image {
            width: 72
            height: 72
            source: {
                if (typeof audioPlayer !== "undefined" && audioPlayer.playing)
                    return "qrc:/pause-hot.png";
                return "qrc:/play-hot.png";
            }
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: {
                    if (typeof audioPlayer !== "undefined")
                        audioPlayer.togglePlayPause();
                }
            }
        }

        // Next button
        Image {
            width: 64
            height: 64
            source: "qrc:/next-hot.png"
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: {
                    if (typeof audioPlayer !== "undefined")
                        audioPlayer.nextTrack();
                }
            }
        }

        // Repeat button
        Image {
            width: 48
            height: 48
            source: {
                if (typeof audioPlayer !== "undefined" && audioPlayer.repeatMode === 2)
                    return "qrc:/Repeat1.png";
                return "qrc:/Repeat.png";
            }
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            opacity: {
                if (typeof audioPlayer !== "undefined" && audioPlayer.repeatMode > 0)
                    return 1.0;
                return 0.5;
            }

            MouseArea {
                anchors.fill: parent
                anchors.margins: -20
                onClicked: {
                    if (typeof audioPlayer !== "undefined")
                        audioPlayer.cycleRepeatMode();
                }
            }
        }
    }

    function formatTime(ms) {
        var totalSec = Math.floor(ms / 1000);
        var min = Math.floor(totalSec / 60);
        var sec = totalSec % 60;
        return min + ":" + (sec < 10 ? "0" : "") + sec;
    }
}
