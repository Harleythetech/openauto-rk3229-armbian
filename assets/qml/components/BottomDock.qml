import QtQuick 2.15
import ".."

// BottomDock - Navigation bar at bottom of screen
// 5 buttons: Home, Music, Android Auto, Volume, Settings

Rectangle {
    id: root

    signal homeClicked
    signal musicClicked
    signal androidAutoClicked
    signal volumeClicked
    signal settingsClicked

    property int currentIndex: 0

    height: Theme.dockHeight
    color: Theme.dockColor

    // Top border line
    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.dockBorderColor
    }

    Row {
        anchors.fill: parent

        // Home button
        Rectangle {
            width: root.width / 5
            height: root.height
            color: homeMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.6
                height: 3
                color: root.currentIndex === 0 ? Theme.primaryColor : "transparent"
            }

            Image {
                anchors.centerIn: parent
                width: Theme.dockIconSize
                height: Theme.dockIconSize
                source: "qrc:/home-hot.png"
                fillMode: Image.PreserveAspectFit
                opacity: root.currentIndex === 0 ? 1.0 : 0.7
            }

            MouseArea {
                id: homeMouseArea
                anchors.fill: parent
                onClicked: {
                    root.currentIndex = 0;
                    root.homeClicked();
                }
            }
        }

        // Music button
        Rectangle {
            width: root.width / 5
            height: root.height
            color: musicMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.6
                height: 3
                color: root.currentIndex === 1 ? Theme.primaryColor : "transparent"
            }

            Image {
                anchors.centerIn: parent
                width: Theme.dockIconSize
                height: Theme.dockIconSize
                source: "qrc:/mp3-hot.png"
                fillMode: Image.PreserveAspectFit
                opacity: root.currentIndex === 1 ? 1.0 : 0.7
            }

            MouseArea {
                id: musicMouseArea
                anchors.fill: parent
                onClicked: {
                    root.currentIndex = 1;
                    root.musicClicked();
                }
            }
        }

        // Android Auto button (canvas icon - no SVG dependency)
        Rectangle {
            width: root.width / 5
            height: root.height
            color: aaMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.6
                height: 3
                color: root.currentIndex === 2 ? Theme.primaryColor : "transparent"
            }

            Image {
                anchors.centerIn: parent
                width: Theme.dockIconSize
                height: Theme.dockIconSize
                source: "qrc:/Android_Auto_icon.png"
                fillMode: Image.PreserveAspectFit
                opacity: root.currentIndex === 2 ? 1.0 : 0.7
            }

            MouseArea {
                id: aaMouseArea
                anchors.fill: parent
                onClicked: {
                    root.currentIndex = 2;
                    root.androidAutoClicked();
                }
            }
        }

        // Volume button
        Rectangle {
            width: root.width / 5
            height: root.height
            color: volumeMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.6
                height: 3
                color: root.currentIndex === 3 ? Theme.primaryColor : "transparent"
            }

            Image {
                anchors.centerIn: parent
                width: Theme.dockIconSize
                height: Theme.dockIconSize
                source: "qrc:/volume-hot.png"
                fillMode: Image.PreserveAspectFit
                opacity: root.currentIndex === 3 ? 1.0 : 0.7
            }

            MouseArea {
                id: volumeMouseArea
                anchors.fill: parent
                onClicked: {
                    root.currentIndex = 3;
                    root.volumeClicked();
                }
            }
        }

        // Settings button
        Rectangle {
            width: root.width / 5
            height: root.height
            color: settingsMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.6
                height: 3
                color: root.currentIndex === 4 ? Theme.primaryColor : "transparent"
            }

            Image {
                anchors.centerIn: parent
                width: Theme.dockIconSize
                height: Theme.dockIconSize
                source: "qrc:/settings-hot.png"
                fillMode: Image.PreserveAspectFit
                opacity: root.currentIndex === 4 ? 1.0 : 0.7
            }

            MouseArea {
                id: settingsMouseArea
                anchors.fill: parent
                onClicked: {
                    root.currentIndex = 4;
                    root.settingsClicked();
                }
            }
        }
    }
}
