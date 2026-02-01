import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15
import ".."

// TileButton - Large touch-friendly button with icon and label
// Used for main menu navigation in Automotive HMI style

Rectangle {
    id: root

    // Public properties
    property string iconSource: ""
    property string label: ""
    property color buttonColor: Theme.cardColor
    property color iconColor: Theme.textPrimary

    signal clicked()

    width: 160
    height: 140
    radius: Theme.buttonRadius
    color: mouseArea.pressed ? Qt.darker(buttonColor, 1.2) : buttonColor
    border.width: Theme.borderWidth
    border.color: Theme.borderColor

    Column {
        anchors.centerIn: parent
        spacing: Theme.spacingSmall

        // Icon with ColorOverlay for tinting
        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.iconSize
            height: Theme.iconSize
            visible: root.iconSource !== ""

            Image {
                id: iconImage
                anchors.fill: parent
                source: root.iconSource
                fillMode: Image.PreserveAspectFit
                visible: false
            }

            ColorOverlay {
                anchors.fill: iconImage
                source: iconImage
                color: root.iconColor
            }
        }

        // Fallback icon placeholder if no source
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.iconSize
            height: Theme.iconSize
            radius: 8
            color: "transparent"
            border.width: 2
            border.color: Theme.primaryColor
            visible: root.iconSource === ""

            Text {
                anchors.centerIn: parent
                text: root.label.length > 0 ? root.label.charAt(0).toUpperCase() : "?"
                font.pixelSize: Theme.fontSizeLarge
                font.family: Theme.fontFamily
                font.bold: true
                color: Theme.primaryColor
            }
        }

        // Label
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            font.pixelSize: Theme.fontSizeMedium
            font.family: Theme.fontFamily
            color: Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
