import QtQuick 2.15
import QtQuick.Controls 2.15
import "."
import "components"

// MusicPage - Built-in music player with album art and controls
// Matches original OpenAuto UI design

Item {
    id: root

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
            var ampmStr = typeof backend !== "undefined" ? backend.amPm : "AM";
            return dateStr + " | " + timeStr + ampmStr;
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
        anchors.verticalCenterOffset: -30
        spacing: 40

        // Album art placeholder
        Rectangle {
            width: 200
            height: 200
            color: "transparent"
            border.width: 3
            border.color: Theme.textPrimary
            radius: 8

            // CD icon placeholder
            Rectangle {
                anchors.centerIn: parent
                width: 120
                height: 120
                radius: 60
                color: "transparent"
                border.width: 3
                border.color: Theme.textPrimary

                Rectangle {
                    anchors.centerIn: parent
                    width: 30
                    height: 30
                    radius: 15
                    color: Theme.textPrimary
                }
            }

            // If album art available, show it instead
            Image {
                id: albumArt
                anchors.fill: parent
                source: typeof backend !== "undefined" && backend.albumArtPath !== "" ? backend.albumArtPath : ""
                visible: source !== ""
                fillMode: Image.PreserveAspectCrop
            }
        }

        // Track info
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            // Track title
            Text {
                text: typeof backend !== "undefined" && backend.trackTitle !== "" ? backend.trackTitle : "No Track"
                font.pixelSize: Theme.fontSizeXLarge
                font.family: Theme.fontFamily
                font.weight: Font.Normal
                color: Theme.textPrimary
            }

            // Album name
            Text {
                text: typeof backend !== "undefined" && backend.albumName !== "" ? backend.albumName : "No Album"
                font.pixelSize: Theme.fontSizeMedium
                font.family: Theme.fontFamily
                font.weight: Font.Light
                color: Theme.textPrimary
            }

            // Artist name
            Text {
                text: typeof backend !== "undefined" && backend.artistName !== "" ? backend.artistName : "Unknown Artist"
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
                font.weight: Font.Light
                color: Theme.textPrimary
            }
        }
    }

    // Media controls
    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 120
        spacing: 80

        // Previous button
        Text {
            text: "⏮"
            font.pixelSize: 48
            color: Theme.textPrimary

            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: {
                    if (typeof backend !== "undefined")
                        backend.previousTrack();
                }
            }
        }

        // Play/Pause button
        Text {
            text: typeof backend !== "undefined" && backend.isPlaying ? "⏸" : "▶"
            font.pixelSize: 48
            color: Theme.textPrimary

            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: {
                    if (typeof backend !== "undefined")
                        backend.togglePlayPause();
                }
            }
        }

        // Next button
        Text {
            text: "⏭"
            font.pixelSize: 48
            color: Theme.textPrimary

            MouseArea {
                anchors.fill: parent
                anchors.margins: -15
                onClicked: {
                    if (typeof backend !== "undefined")
                        backend.nextTrack();
                }
            }
        }
    }
}
