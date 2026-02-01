import QtQuick 2.15
import QtQuick.Layouts 1.15
import ".."

// SettingsCard - Expandable card for settings category
// Automotive HMI style

Rectangle {
    id: root

    property string title: ""
    property string description: ""
    property bool expanded: false
    default property alias content: contentArea.data

    radius: Theme.cardRadius
    color: Theme.cardColor
    border.width: Theme.borderWidth
    border.color: Theme.borderColor

    Column {
        anchors.fill: parent
        spacing: 0

        // Header (always visible)
        Rectangle {
            width: parent.width
            height: 60
            color: "transparent"
            radius: Theme.cardRadius

            MouseArea {
                anchors.fill: parent
                onClicked: root.expanded = !root.expanded
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacing
                spacing: Theme.spacing

                Column {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter

                    Text {
                        text: root.title
                        font.pixelSize: Theme.fontSizeMedium
                        font.family: Theme.fontFamily
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Text {
                        text: root.description
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
                        visible: !root.expanded
                    }
                }

                // Expand indicator
                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: root.expanded ? "▲" : "▼"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }
            }
        }

        // Content area (shown when expanded)
        Item {
            id: contentArea
            width: parent.width
            height: root.expanded ? implicitHeight : 0
            clip: true
            visible: root.expanded

            // Content is placed here via default property
        }
    }
}
