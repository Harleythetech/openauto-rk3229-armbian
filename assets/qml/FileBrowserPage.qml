import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "."
import "components"

// FileBrowserPage - Two-column file browser for USB media
// Left sidebar: mounted USB volumes
// Right panel: breadcrumb navigation + folder/file list

Item {
    id: root

    signal playFile(string filePath)
    signal playFolder(string folderPath)

    Rectangle {
        anchors.fill: parent
        color: Theme.settingsBackground
    }

    // Left sidebar - USB volumes
    Rectangle {
        id: sidebar
        width: Theme.sidebarWidth
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        color: Theme.settingsBackground

        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: Theme.dockBorderColor
        }

        Column {
            anchors.fill: parent
            anchors.topMargin: Theme.spacing
            spacing: 0

            // Return button
            Rectangle {
                width: parent.width
                height: 48
                color: returnMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spacing
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    Text {
                        text: "‹"
                        font.pixelSize: Theme.fontSizeLarge
                        font.weight: Font.Normal
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Return"
                        font.pixelSize: Theme.fontSizeLarge
                        font.family: Theme.fontFamily
                        font.weight: Font.Normal
                        color: Theme.textPrimary
                    }
                }

                MouseArea {
                    id: returnMouseArea
                    anchors.fill: parent
                    onClicked: {
                        if (typeof backend !== "undefined")
                            backend.goBack();
                    }
                }
            }

            Item {
                width: 1
                height: Theme.spacingLarge
            }

            // USB Drives header
            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.spacing
                text: "USB Drives"
                font.pixelSize: Theme.fontSizeMedium
                font.family: Theme.fontFamily
                font.weight: Font.Medium
                color: Theme.textSecondary
            }

            Item {
                width: 1
                height: Theme.spacingSmall
            }

            // Volume list
            Repeater {
                model: typeof fileBrowser !== "undefined" ? fileBrowser.mountedVolumes : []

                Rectangle {
                    width: parent.width
                    height: 52
                    color: {
                        if (typeof fileBrowser !== "undefined" && fileBrowser.currentVolumeName === modelData.name)
                            return Qt.rgba(1, 1, 1, 0.1);
                        return "transparent";
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 4
                        height: parent.height * 0.6
                        radius: 2
                        color: {
                            if (typeof fileBrowser !== "undefined" && fileBrowser.currentVolumeName === modelData.name)
                                return Theme.primaryColor;
                            return "transparent";
                        }
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacing
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 10

                        Image {
                            width: 24
                            height: 24
                            source: "qrc:/USB.svg"
                            fillMode: Image.PreserveAspectFit
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Column {
                            spacing: 2

                            Text {
                                text: modelData.name
                                font.pixelSize: Theme.fontSizeSmall
                                font.family: Theme.fontFamily
                                font.weight: Font.Normal
                                color: Theme.textPrimary
                            }
                            Text {
                                text: modelData.size ? modelData.size : ""
                                font.pixelSize: Theme.fontSizeXSmall
                                font.family: Theme.fontFamily
                                color: Theme.textSecondary
                                visible: text !== ""
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (typeof fileBrowser !== "undefined")
                                fileBrowser.selectVolume(modelData.path);
                        }
                    }
                }
            }

            // No drives message
            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.spacing
                text: "No USB drives found"
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
                color: Theme.textMuted
                visible: typeof fileBrowser === "undefined" || fileBrowser.mountedVolumes.length === 0
            }

            Item {
                Layout.fillHeight: true
            }

            // Refresh button
            Rectangle {
                width: parent.width - Theme.spacing * 2
                height: 40
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 8
                color: refreshMouseArea.pressed ? Qt.darker(Theme.cardColor, 1.2) : Theme.cardColor

                Text {
                    anchors.centerIn: parent
                    text: "Refresh"
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                    color: Theme.textPrimary
                }

                MouseArea {
                    id: refreshMouseArea
                    anchors.fill: parent
                    onClicked: {
                        if (typeof fileBrowser !== "undefined")
                            fileBrowser.refreshVolumes();
                    }
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }
        }
    }

    // Right content panel
    Item {
        id: contentPanel
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        // Breadcrumb header
        Item {
            id: breadcrumbArea
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Theme.spacing
            height: 48

            Row {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                // Up button
                Rectangle {
                    width: 32
                    height: 32
                    radius: 6
                    color: upMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(1, 1, 1, 0.05)
                    visible: typeof fileBrowser !== "undefined" && fileBrowser.currentPath !== ""

                    Text {
                        anchors.centerIn: parent
                        text: "↑"
                        font.pixelSize: 18
                        color: Theme.textPrimary
                    }

                    MouseArea {
                        id: upMouseArea
                        anchors.fill: parent
                        onClicked: {
                            if (typeof fileBrowser !== "undefined")
                                fileBrowser.navigateUp();
                        }
                    }
                }

                // Breadcrumb path
                Repeater {
                    model: typeof fileBrowser !== "undefined" ? fileBrowser.breadcrumb : []

                    Row {
                        spacing: 4

                        Text {
                            text: index > 0 ? " › " : ""
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: Theme.textSecondary
                            visible: index > 0
                        }

                        Text {
                            text: modelData
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            font.weight: index === (typeof fileBrowser !== "undefined" ? fileBrowser.breadcrumb.length - 1 : 0) ? Font.Medium : Font.Light
                            color: Theme.textPrimary
                        }
                    }
                }
            }

            // Play all button
            Rectangle {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 100
                height: 32
                radius: 8
                color: playAllMouseArea.pressed ? Qt.darker("#2A5F8F", 1.2) : "#2A5F8F"
                visible: typeof fileBrowser !== "undefined" && fileBrowser.currentPath !== ""

                Text {
                    anchors.centerIn: parent
                    text: "Play All"
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                    color: "#FFFFFF"
                }

                MouseArea {
                    id: playAllMouseArea
                    anchors.fill: parent
                    onClicked: {
                        if (typeof fileBrowser !== "undefined" && typeof audioPlayer !== "undefined") {
                            var files = fileBrowser.currentAudioFiles();
                            if (files.length > 0) {
                                audioPlayer.setPlaylist(files);
                                audioPlayer.playIndex(0);
                                backend.goBack(); // Return to music page
                            }
                        }
                    }
                }
            }
        }

        // File/Folder listing
        ListView {
            id: fileListView
            anchors.top: breadcrumbArea.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Theme.spacing
            anchors.topMargin: Theme.spacingSmall
            clip: true
            model: typeof fileBrowser !== "undefined" ? fileBrowser.currentEntries : []

            delegate: Rectangle {
                width: fileListView.width
                height: 48
                color: fileMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.05)
                }

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 12

                    // Folder or file icon
                    Image {
                        width: 24
                        height: 24
                        source: modelData.isDir ? "qrc:/Folder.svg" : "qrc:/mp3-hot.png"
                        fillMode: Image.PreserveAspectFit
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Column {
                        spacing: 2
                        anchors.verticalCenter: parent.verticalCenter

                        Text {
                            text: modelData.name
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            font.weight: Font.Normal
                            color: Theme.textPrimary
                        }

                        Text {
                            text: {
                                if (modelData.isDir && modelData.audioCount > 0)
                                    return modelData.audioCount + " audio files";
                                return "";
                            }
                            font.pixelSize: Theme.fontSizeXSmall
                            font.family: Theme.fontFamily
                            color: Theme.textSecondary
                            visible: text !== ""
                        }
                    }
                }

                MouseArea {
                    id: fileMouseArea
                    anchors.fill: parent
                    onClicked: {
                        if (modelData.isDir) {
                            if (typeof fileBrowser !== "undefined")
                                fileBrowser.navigateTo(modelData.path);
                        } else if (modelData.isAudio) {
                            if (typeof audioPlayer !== "undefined" && typeof fileBrowser !== "undefined") {
                                // Set playlist to all audio files in this folder, play clicked file
                                var files = fileBrowser.currentAudioFiles();
                                audioPlayer.setPlaylist(files);
                                var idx = files.indexOf(modelData.path);
                                audioPlayer.playIndex(idx >= 0 ? idx : 0);
                                backend.goBack(); // Return to music page
                            }
                        }
                    }
                }
            }
        }

        // Empty state
        Text {
            anchors.centerIn: parent
            text: {
                if (typeof fileBrowser === "undefined" || fileBrowser.currentPath === "")
                    return "Select a USB drive to browse";
                return "No audio files found";
            }
            font.pixelSize: Theme.fontSizeMedium
            font.family: Theme.fontFamily
            color: Theme.textMuted
            visible: typeof fileBrowser === "undefined" || fileBrowser.currentEntries.length === 0
        }
    }
}
