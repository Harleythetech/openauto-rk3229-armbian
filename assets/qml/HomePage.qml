import QtQuick 2.15
import QtQuick.Layouts 1.15
import "."
import "components"

// HomePage - Main screen with large centered clock and date
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

    // Centered clock + date
    Column {
        anchors.centerIn: parent
        spacing: 8

        // Time display with AM/PM
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 4

            Text {
                text: typeof backend !== "undefined" ? backend.currentTime : "00:00"
                font.pixelSize: Theme.fontSizeClock
                font.family: Theme.fontFamily
                font.weight: Font.Bold
                color: Theme.textPrimary
            }

            Text {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 24
                text: typeof backend !== "undefined" ? backend.amPm : "AM"
                font.pixelSize: Theme.fontSizeXLarge
                font.family: Theme.fontFamily
                font.weight: Font.Bold
                color: Theme.textPrimary
            }
        }

        // Date display
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: typeof backend !== "undefined" ? backend.currentDate : "January 1, 2026"
            font.pixelSize: Theme.fontSizeXLarge
            font.family: Theme.fontFamily
            font.weight: Font.Light
            color: Theme.textPrimary
        }
    }

    // Right navigation arrow - goes to music player
    Text {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 32
        text: "›"
        font.pixelSize: 72
        font.weight: Font.Light
        color: Theme.textPrimary
        opacity: 0.7

        MouseArea {
            anchors.fill: parent
            anchors.margins: -20
            onClicked: navigateMusic()
        }
    }

    signal navigateSettings
    signal navigateMusic
}
