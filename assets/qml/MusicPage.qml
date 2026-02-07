import QtQuick 2.15
import QtQuick.Controls 2.15
import "."
import "components"

// MusicPage - Music player with album art, controls and progress bar
// Controls: Folder | Prev | Play/Pause | Next | Repeat

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

    // Date/Time in top-right corner
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
                return dateStr + " | " + timeStr + " " + ampmStr;
            else
                return dateStr + " | " + timeStr;
        }
        font.pixelSize: Theme.fontSizeLarge
        font.family: Theme.fontFamily
        font.weight: Font.Light
        color: Theme.textPrimary
    }

    // Back arrow (left side)
    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 32
        text: "‹"
        font.pixelSize: 72
        font.weight: Font.Light
        color: Theme.textPrimary
        opacity: 0.7

        MouseArea {
            anchors.fill: parent
            anchors.margins: -20
            onClicked: {
                if (typeof backend !== "undefined")
                    backend.goBack();
            }
        }
    }

    // Center content area
    Row {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -50
        spacing: 40

        // Album art
        Rectangle {
            width: 200
            height: 200
            color: "transparent"
            border.width: 2
            border.color: Qt.rgba(1, 1, 1, 0.3)
            radius: 8

            // Placeholder CD icon
            Rectangle {
                anchors.centerIn: parent
                width: 120
                height: 120
                radius: 60
                color: "transparent"
                border.width: 2
                border.color: Qt.rgba(1, 1, 1, 0.3)
                visible: albumArt.status !== Image.Ready

                Rectangle {
                    anchors.centerIn: parent
                    width: 30
                    height: 30
                    radius: 15
                    color: Qt.rgba(1, 1, 1, 0.3)
                }
            }

            // Album art image
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

        // Track info
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            width: Math.min(root.width - 400, 360)

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
        }
    }

    // Progress bar
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: controlsRow.top
        anchors.bottomMargin: 20
        width: parent.width * 0.6
        spacing: 4

        Slider {
            id: progressSlider
            width: parent.width
            from: 0
            to: typeof audioPlayer !== "undefined" ? Math.max(audioPlayer.duration, 1) : 1
            value: typeof audioPlayer !== "undefined" ? audioPlayer.position : 0
            enabled: typeof audioPlayer !== "undefined" && audioPlayer.playing

            onMoved: {
                if (typeof audioPlayer !== "undefined")
                    audioPlayer.seek(value);
            }

            background: Rectangle {
                x: progressSlider.leftPadding
                y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                width: progressSlider.availableWidth
                height: 4
                radius: 2
                color: Qt.rgba(1, 1, 1, 0.15)

                Rectangle {
                    width: progressSlider.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    color: Theme.primaryColor
                }
            }

            handle: Rectangle {
                x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                width: 12
                height: 12
                radius: 6
                color: "#FFFFFF"
                visible: progressSlider.enabled
            }
        }

        // Time labels
        Row {
            width: parent.width
            Text {
                text: formatTime(typeof audioPlayer !== "undefined" ? audioPlayer.position : 0)
                font.pixelSize: Theme.fontSizeXSmall
                font.family: Theme.fontFamily
                color: Theme.textSecondary
            }
            Item {
                width: parent.width - 80
                height: 1
            }
            Text {
                text: formatTime(typeof audioPlayer !== "undefined" ? audioPlayer.duration : 0)
                font.pixelSize: Theme.fontSizeXSmall
                font.family: Theme.fontFamily
                color: Theme.textSecondary
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    // Media controls row: Folder | Prev | Play/Pause | Next | Repeat
    Row {
        id: controlsRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 100
        spacing: 60

        // Folder button - opens file browser
        Image {
            width: 36
            height: 36
            source: "qrc:/Folder.svg"
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: root.openFileBrowser()
            }
        }

        // Previous button
        Image {
            width: 48
            height: 48
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

        // Play/Pause button
        Image {
            width: 48
            height: 48
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
            width: 48
            height: 48
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
            width: 36
            height: 36
            source: {
                if (typeof audioPlayer !== "undefined" && audioPlayer.repeatMode === 2)
                    return "qrc:/Repeat1.svg";
                return "qrc:/Repeat.svg";
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
                anchors.margins: -15
                onClicked: {
                    if (typeof audioPlayer !== "undefined")
                        audioPlayer.cycleRepeatMode();
                }
            }
        }
    }

    // Playlist info + audio quality
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 70
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
    }

    function formatTime(ms) {
        var totalSec = Math.floor(ms / 1000);
        var min = Math.floor(totalSec / 60);
        var sec = totalSec % 60;
        return min + ":" + (sec < 10 ? "0" : "") + sec;
    }
}
