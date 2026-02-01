import QtQuick 2.15
import QtQuick.Layouts 1.15
import ".."

// StatusBar - Top status bar with clock, network, Bluetooth indicators
// Fixed at top of screen in Automotive HMI style

Rectangle {
    id: root

    // Properties bound from backend
    property string currentTime: "00:00"
    property string networkSSID: ""
    property bool bluetoothConnected: false
    property bool wifiConnected: false

    height: Theme.statusBarHeight
    color: Qt.rgba(0, 0, 0, Theme.overlayOpacity)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing
        anchors.rightMargin: Theme.spacing

        // Left: Clock
        Text {
            id: clockText
            text: root.currentTime
            font.pixelSize: Theme.fontSizeLarge
            font.family: Theme.fontFamily
            font.bold: true
            color: Theme.textPrimary
            Layout.alignment: Qt.AlignVCenter
        }

        // Center: Spacer
        Item {
            Layout.fillWidth: true
        }

        // Right: Status icons
        RowLayout {
            spacing: Theme.spacing
            Layout.alignment: Qt.AlignVCenter

            // WiFi indicator
            Rectangle {
                width: 32
                height: 32
                radius: 4
                color: root.wifiConnected ? Theme.successColor : "transparent"
                border.width: 1
                border.color: root.wifiConnected ? Theme.successColor : Theme.textSecondary
                visible: root.networkSSID !== "" || root.wifiConnected

                Text {
                    anchors.centerIn: parent
                    text: "W"
                    font.pixelSize: 14
                    font.bold: true
                    color: root.wifiConnected ? Theme.backgroundColor : Theme.textSecondary
                }
            }

            // Bluetooth indicator
            Rectangle {
                width: 32
                height: 32
                radius: 4
                color: root.bluetoothConnected ? Theme.primaryColor : "transparent"
                border.width: 1
                border.color: root.bluetoothConnected ? Theme.primaryColor : Theme.textSecondary

                Text {
                    anchors.centerIn: parent
                    text: "B"
                    font.pixelSize: 14
                    font.bold: true
                    color: root.bluetoothConnected ? Theme.backgroundColor : Theme.textSecondary
                }
            }

            // Network SSID (if connected)
            Text {
                text: root.networkSSID
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
                color: Theme.textSecondary
                visible: root.networkSSID !== ""
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }
}
