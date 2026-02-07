import QtQuick 2.15
import ".."

// BottomDock - Navigation bar at bottom of screen
// Matches original OpenAuto UI with 5 buttons

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

        Repeater {
            model: [
                {
                    icon: "qrc:/home-hot.png",
                    action: "home"
                },
                {
                    icon: "qrc:/mp3-hot.png",
                    action: "music"
                },
                {
                    icon: "qrc:/Android_Auto_icon.svg",
                    action: "aa"
                },
                {
                    icon: "qrc:/volume-hot.png",
                    action: "volume"
                },
                {
                    icon: "qrc:/settings-hot.png",
                    action: "settings"
                }
            ]

            Rectangle {
                width: root.width / 5
                height: root.height
                color: mouseArea.pressed ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

                // Active indicator line at top
                Rectangle {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width * 0.6
                    height: 3
                    color: root.currentIndex === index ? Theme.primaryColor : "transparent"
                }

                // Icon
                Image {
                    anchors.centerIn: parent
                    width: Theme.dockIconSize
                    height: Theme.dockIconSize
                    source: modelData.icon
                    fillMode: Image.PreserveAspectFit
                    opacity: root.currentIndex === index ? 1.0 : 0.7
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    onClicked: {
                        root.currentIndex = index;
                        if (modelData.action === "home")
                            root.homeClicked();
                        else if (modelData.action === "music")
                            root.musicClicked();
                        else if (modelData.action === "aa")
                            root.androidAutoClicked();
                        else if (modelData.action === "volume")
                            root.volumeClicked();
                        else if (modelData.action === "settings")
                            root.settingsClicked();
                    }
                }
            }
        }
    }
}
