import QtQuick 2.15
import QtQuick.Controls 2.15
import "."

// MusicPage - Music player matching design reference
// Layout: Date/time top-right, centered album art + track info, large controls at bottom

Item {
    id: root
    objectName: "musicPage"

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
        font.weight: Font.ExtraLight
        color: Theme.textPrimary
    }

    // Back arrow (left side of album art)
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
                if (root.StackView.view)
                    root.StackView.view.pop();
            }
        }
    }

    // Center content area — album art + track info side by side
    Row {
        // 1. Center Vertically only
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -60

        // 2. Anchor to the left side
        anchors.left: parent.left
        anchors.leftMargin: Math.max(100, parent.width * 0.15)
        spacing: 48

        // Album art — large (250x250)
        Rectangle {
            width: 250
            height: 250
            color: "transparent"

            Image {
                id: albumArt
                anchors.fill: parent
                source: {
                    if (typeof audioPlayer !== "undefined" && audioPlayer.albumArtPath !== "")
                        return audioPlayer.albumArtPath;
                    return Theme.imgPath + "album-hot.png";
                }
                fillMode: Image.PreserveAspectCrop
                visible: status === Image.Ready
            }
        }

        // Track info column
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            width: Math.min(root.width - 450, 450)

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

    // Progress bar with time labels — above controls
    Column {
        id: progressArea
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: controlsRow.top
        anchors.bottomMargin: 16

        // CHANGE 1: Use full width minus margins (50px left + 50px right = 100)
        width: parent.width - 300
        spacing: 4

        Slider {
            id: progressSlider
            width: parent.width
            from: 0
            to: typeof audioPlayer !== "undefined" ? Math.max(audioPlayer.duration, 1) : 1
            enabled: typeof audioPlayer !== "undefined" && audioPlayer.playing

            onMoved: {
                if (typeof audioPlayer !== "undefined")
                    audioPlayer.seek(value);
            }

            // Update position only when user is not dragging
            Connections {
                target: typeof audioPlayer !== "undefined" ? audioPlayer : null
                function onPositionChanged() {
                    if (!progressSlider.pressed)
                        progressSlider.value = audioPlayer.position;
                }
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

        Row {
            width: parent.width

            Text {
                id: elapsedTimeLabel
                text: formatTime(progressSlider.value)
                font.pixelSize: Theme.fontSizeXSmall
                font.family: Theme.fontFamily
                color: Theme.textSecondary
            }
            Item {
                width: parent.width - elapsedTimeLabel.width - totalTimeLabel.width
                height: 1
            }
            Text {
                id: totalTimeLabel
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
        anchors.bottomMargin: 50

        // CHANGE 2: Match the width of the progress bar
        width: parent.width - 300

        // CHANGE 3: Use "SpaceBetween" logic instead of fixed spacing
        // We set spacing to 0 and use "Item" spacers to push buttons apart evenly
        spacing: 0

        // 1. Folder Button
        Image {
            width: 48
            height: 48
            source: Theme.imgPath + "File.png"
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                anchors.fill: parent
                anchors.margins: -20
                onClicked: root.openFileBrowser()
            }
        }

        // Spacer
        Item {
            width: (parent.width - (48 + 64 + 72 + 64 + 48)) / 4
            height: 1
        }

        // 2. Previous Button
        Image {
            width: 64
            height: 64
            source: Theme.imgPath + "prev-hot.png"
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: if (typeof audioPlayer !== "undefined")
                    audioPlayer.previousTrack()
            }
        }

        // Spacer
        Item {
            width: (parent.width - (48 + 64 + 72 + 64 + 48)) / 4
            height: 1
        }

        // 3. Play/Pause Button
        Image {
            width: 72
            height: 72
            source: {
                if (typeof audioPlayer !== "undefined" && audioPlayer.playing)
                    return Theme.imgPath + "pause-hot.png";
                return Theme.imgPath + "play-hot.png";
            }
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: if (typeof audioPlayer !== "undefined")
                    audioPlayer.togglePlayPause()
            }
        }

        // Spacer
        Item {
            width: (parent.width - (48 + 64 + 72 + 64 + 48)) / 4
            height: 1
        }

        // 4. Next Button
        Image {
            width: 64
            height: 64
            source: Theme.imgPath + "next-hot.png"
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: if (typeof audioPlayer !== "undefined")
                    audioPlayer.nextTrack()
            }
        }

        // Spacer
        Item {
            width: (parent.width - (48 + 64 + 72 + 64 + 48)) / 4
            height: 1
        }

        // 5. Repeat Button
        Image {
            width: 48
            height: 48
            source: {
                if (typeof audioPlayer !== "undefined" && audioPlayer.repeatMode === 2)
                    return Theme.imgPath + "Repeat1.png";
                return Theme.imgPath + "Repeat.png";
            }
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            opacity: (typeof audioPlayer !== "undefined" && audioPlayer.repeatMode > 0) ? 1.0 : 0.5
            MouseArea {
                anchors.fill: parent
                anchors.margins: -20
                onClicked: if (typeof audioPlayer !== "undefined")
                    audioPlayer.cycleRepeatMode()
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
