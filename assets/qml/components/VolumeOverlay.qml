import QtQuick 2.15
import QtQuick.Controls 2.15
import ".."

// VolumeOverlay - Horizontal volume bar that appears at the bottom
// Auto-hides after 3 seconds of inactivity

Rectangle {
    id: root

    property int currentVolume: typeof backend !== "undefined" ? backend.volume : 80
    property bool showing: false

    visible: showing
    width: parent.width * 0.6
    height: 56
    radius: 12
    color: Qt.rgba(0.1, 0.12, 0.18, 0.95)
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, 0.15)

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 16

    Timer {
        id: hideTimer
        interval: 3000
        onTriggered: root.showing = false
    }

    function show() {
        showing = true;
        hideTimer.restart();
    }

    function toggle() {
        if (showing) {
            showing = false;
            hideTimer.stop();
        } else {
            show();
        }
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12

        // Volume icon
        Image {
            anchors.verticalCenter: parent.verticalCenter
            width: 28
            height: 28
            source: "qrc:/volume-hot.png"
            fillMode: Image.PreserveAspectFit
        }

        // Volume slider
        Slider {
            id: volumeSlider
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - 28 - 50 - 36
            from: 0
            to: 100
            stepSize: 1
            value: root.currentVolume

            onMoved: {
                hideTimer.restart();
                if (typeof backend !== "undefined" && backend !== null)
                    backend.setVolume(Math.round(value));
            }

            background: Rectangle {
                x: volumeSlider.leftPadding
                y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                width: volumeSlider.availableWidth
                height: 6
                radius: 3
                color: Qt.rgba(1, 1, 1, 0.2)

                Rectangle {
                    width: volumeSlider.visualPosition * parent.width
                    height: parent.height
                    radius: 3
                    color: Theme.primaryColor
                }
            }

            handle: Rectangle {
                x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                width: 18
                height: 18
                radius: 9
                color: "#FFFFFF"
            }
        }

        // Percentage text
        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: 50
            text: Math.round(volumeSlider.value) + "%"
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            font.weight: Font.Normal
            color: Theme.textPrimary
            horizontalAlignment: Text.AlignRight
        }
    }

    // Update from backend changes
    Connections {
        target: typeof backend !== "undefined" ? backend : null
        function onVolumeChanged() {
            volumeSlider.value = backend.volume;
        }
    }
}
