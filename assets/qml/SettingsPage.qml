import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"

// SettingsPage - Simplified settings with category cards
// Automotive HMI style for touch interaction

Item {
    id: root

    // Back button
    Rectangle {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: Theme.spacing
        width: 100
        height: 48
        radius: Theme.buttonRadius
        color: mouseAreaBack.pressed ? Qt.darker(Theme.cardColor, 1.2) : Theme.cardColor
        border.width: Theme.borderWidth
        border.color: Theme.borderColor

        Text {
            anchors.centerIn: parent
            text: "← Back"
            font.pixelSize: Theme.fontSizeMedium
            font.family: Theme.fontFamily
            color: Theme.textPrimary
        }

        MouseArea {
            id: mouseAreaBack
            anchors.fill: parent
            onClicked: {
                if (typeof backend !== "undefined") {
                    backend.goBack()
                }
            }
        }
    }

    // Title
    Text {
        id: titleText
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: Theme.spacing
        text: "Settings"
        font.pixelSize: Theme.fontSizeXLarge
        font.family: Theme.fontFamily
        font.bold: true
        color: Theme.textPrimary
    }

    // Settings categories in scrollable list
    ScrollView {
        anchors.top: titleText.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spacing
        anchors.topMargin: Theme.spacingLarge
        clip: true

        GridLayout {
            width: parent.width
            columns: 2
            rowSpacing: Theme.spacing
            columnSpacing: Theme.spacing

            // Display Settings
            SettingsCard {
                title: "Display"
                description: "Brightness, day/night mode"
                Layout.fillWidth: true
                Layout.preferredHeight: 120

                Column {
                    anchors.fill: parent
                    anchors.margins: Theme.spacing
                    spacing: Theme.spacingSmall

                    Text {
                        text: "Brightness"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }

                    Slider {
                        width: parent.width
                        from: 0
                        to: 100
                        value: typeof backend !== "undefined" ? backend.brightness : 50
                        onValueChanged: {
                            if (typeof backend !== "undefined") {
                                backend.setBrightness(value)
                            }
                        }
                    }
                }
            }

            // Audio Settings
            SettingsCard {
                title: "Audio"
                description: "Volume, output device"
                Layout.fillWidth: true
                Layout.preferredHeight: 120

                Column {
                    anchors.fill: parent
                    anchors.margins: Theme.spacing
                    spacing: Theme.spacingSmall

                    Text {
                        text: "Volume"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }

                    Slider {
                        width: parent.width
                        from: 0
                        to: 100
                        value: typeof backend !== "undefined" ? backend.volume : 80
                        onValueChanged: {
                            if (typeof backend !== "undefined") {
                                backend.setVolume(value)
                            }
                        }
                    }
                }
            }

            // Video Settings
            SettingsCard {
                title: "Video"
                description: "Resolution, FPS"
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                Text {
                    anchors.centerIn: parent
                    text: typeof backend !== "undefined" ? backend.videoResolution : "1280x720"
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.textPrimary
                }
            }

            // Bluetooth Settings
            SettingsCard {
                title: "Bluetooth"
                description: "Pairing, devices"
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                Text {
                    anchors.centerIn: parent
                    text: typeof backend !== "undefined" && backend.bluetoothConnected ? "Connected" : "Not Connected"
                    font.pixelSize: Theme.fontSizeMedium
                    color: typeof backend !== "undefined" && backend.bluetoothConnected ? Theme.successColor : Theme.textSecondary
                }
            }

            // WiFi Settings
            SettingsCard {
                title: "WiFi"
                description: "Networks, AA Wireless"
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                Text {
                    anchors.centerIn: parent
                    text: typeof backend !== "undefined" && backend.networkSSID !== "" ? backend.networkSSID : "Not Connected"
                    font.pixelSize: Theme.fontSizeMedium
                    color: typeof backend !== "undefined" && backend.networkSSID !== "" ? Theme.successColor : Theme.textSecondary
                }
            }

            // About
            SettingsCard {
                title: "About"
                description: "Version, system info"
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                Column {
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "OpenAuto"
                        font.pixelSize: Theme.fontSizeMedium
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: typeof backend !== "undefined" ? backend.versionString : ""
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }
                }
            }
        }
    }
}
