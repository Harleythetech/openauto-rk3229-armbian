import QtQuick 2.15
import QtQuick.Layouts 1.15
import "."
import "components"

// HomePage - Main menu with large tile buttons
// Automotive HMI style grid layout for touch interaction

Item {
    id: root

    // Grid of main action buttons
    GridLayout {
        anchors.centerIn: parent
        columns: 3
        rowSpacing: Theme.spacingLarge
        columnSpacing: Theme.spacingLarge

        // Android Auto USB
        TileButton {
            label: "Android Auto"
            buttonColor: Theme.buttonAndroidAuto
            Layout.preferredWidth: 180
            Layout.preferredHeight: 160
            onClicked: {
                if (typeof backend !== "undefined") {
                    backend.startAndroidAutoUSB()
                }
            }
        }

        // Android Auto WiFi
        TileButton {
            label: "AA Wireless"
            buttonColor: Theme.buttonWifi
            Layout.preferredWidth: 180
            Layout.preferredHeight: 160
            onClicked: {
                if (typeof backend !== "undefined") {
                    backend.startAndroidAutoWiFi()
                }
            }
        }

        // Settings
        TileButton {
            label: "Settings"
            buttonColor: Theme.buttonSettings
            Layout.preferredWidth: 180
            Layout.preferredHeight: 160
            onClicked: {
                if (typeof backend !== "undefined") {
                    backend.openSettings()
                }
            }
        }

        // Day/Night Mode
        TileButton {
            label: "Day/Night"
            buttonColor: Theme.buttonDayNight
            Layout.preferredWidth: 180
            Layout.preferredHeight: 160
            onClicked: {
                if (typeof backend !== "undefined") {
                    backend.toggleDayNight()
                }
            }
        }

        // Media Player
        TileButton {
            label: "Media"
            buttonColor: Theme.buttonMedia
            Layout.preferredWidth: 180
            Layout.preferredHeight: 160
            onClicked: {
                if (typeof backend !== "undefined") {
                    backend.openMediaPlayer()
                }
            }
        }

        // Exit / Power
        TileButton {
            label: "Exit"
            buttonColor: Theme.buttonExit
            Layout.preferredWidth: 180
            Layout.preferredHeight: 160
            onClicked: {
                if (typeof backend !== "undefined") {
                    backend.exitApp()
                }
            }
        }
    }

    // Version info at bottom
    Text {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: Theme.spacing
        text: typeof backend !== "undefined" ? backend.versionString : "OpenAuto"
        font.pixelSize: Theme.fontSizeSmall
        font.family: Theme.fontFamily
        color: Theme.textSecondary
    }
}
