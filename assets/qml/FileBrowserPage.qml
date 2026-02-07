import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "."

// FileBrowserPage - Full-width file browser matching design reference
// Layout: Top bar (< Return | date/time), USB drive sidebar, full-width file list
// When no volume selected: shows available USB drives list
// When volume selected: breadcrumb header + folder/file listing
Item {
    id: root
    objectName: "fileBrowserPage"

    signal playFile(string filePath)
    signal playFolder(string folderPath)

    // Reusable folder icon component
    Component {
        id: folderIconComponent
        Image {
            source: Theme.imgPath + "File.png"
            fillMode: Image.PreserveAspectFit
        }
    }

    // Background
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

    // ─── Top bar: < Return (left) | Date/Time (right) ───
    Item {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56

        // < Return button
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            Text {
                text: "‹" // This is NOT the keyboard "<". It is Alt+0139
                font.pixelSize: 48 // Make it larger than the text so it stands out
                font.family: Theme.fontFamily
                font.weight: Font.Thin
                color: Theme.textPrimary

                // This forces the arrow to center perfectly with the word "Return"
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -4 // Slight lift because fonts usually render this glyph low
            }
            Text {
                text: "Return"
                font.pixelSize: Theme.fontSizeLarge
                font.family: Theme.fontFamily
                font.weight: Font.Light
                color: Theme.textPrimary
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 180
            onClicked: {
                if (root.StackView.view)
                    root.StackView.view.pop();
            }
        }

        // Date/Time (right side)
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.verticalCenter: parent.verticalCenter
            text: {
                var dateStr = typeof backend !== "undefined" ? backend.currentDate : "February 1, 2026";
                var timeStr = typeof backend !== "undefined" ? backend.currentTime : "00:00";
                var ampmStr = typeof backend !== "undefined" ? backend.amPm : "";
                if (ampmStr !== "")
                    return dateStr + " | " + timeStr + ampmStr;
                else
                    return dateStr + " | " + timeStr;
            }
            font.pixelSize: Theme.fontSizeLarge
            font.family: Theme.fontFamily
            font.weight: Font.ExtraLight
            color: Theme.textPrimary
        }
    }

    // Thin separator below top bar
    Rectangle {
        id: topSeparator
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Qt.rgba(1, 1, 1, 0.08)
    }

    // ─── Main content: sidebar + file list ───
    Row {
        anchors.top: topSeparator.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        // Left sidebar — USB drives (narrow)
        Rectangle {
            id: driveSidebar
            width: 220
            height: parent.height
            color: Qt.rgba(0, 0, 0, 0.15)

            // Separator line on right
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Qt.rgba(1, 1, 1, 0.08)
            }

            Column {
                anchors.fill: parent
                anchors.topMargin: 16
                spacing: 4

                // USB Drives header
                Text {
                    id: usbheader
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    text: "USB Drives"
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                    font.weight: Font.Medium
                    color: Theme.textSecondary
                }

                Item {
                    width: 1
                    height: 8
                }

                // Volume list
                Repeater {
                    model: typeof fileBrowser !== "undefined" ? fileBrowser.mountedVolumes : []

                    Rectangle {
                        width: driveSidebar.width
                        height: 60
                        color: {
                            if (typeof fileBrowser !== "undefined" && fileBrowser.currentVolumeName === modelData.name)
                                return Qt.rgba(1, 1, 1, 0.08);
                            return "transparent";
                        }

                        // Active indicator bar
                        Rectangle {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 3
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
                            anchors.leftMargin: 20
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 12

                            Image {
                                width: 28
                                height: 28
                                source: Theme.imgPath + "USB.png"
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
                                    text: modelData.size ? "(" + modelData.size + ")" : ""
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
                    anchors.top: usbheader.bottom
                    anchors.topMargin: 10
                    anchors.leftMargin: 20
                    text: "No USB drives\ndetected"
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                    font.weight: Font.Light
                    color: Theme.textMuted
                    lineHeight: 1.4
                    visible: typeof fileBrowser === "undefined" || fileBrowser.mountedVolumes.length === 0
                }

                // Spacer
                Item {
                    width: 1
                    height: 16
                }

                // Refresh button
                Rectangle {
                    width: driveSidebar.width - 40
                    height: 36
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 10
                    radius: 8
                    color: refreshMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.06)

                    Text {
                        anchors.centerIn: parent
                        text: "Refresh"
                        font.pixelSize: Theme.fontSizeXSmall
                        font.family: Theme.fontFamily
                        color: Theme.textSecondary
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
            }
        }

        // Right panel — breadcrumb + file list (fills remaining width)
        Item {
            id: contentPanel
            width: parent.width - driveSidebar.width
            height: parent.height

            // Breadcrumb header
            Item {
                id: breadcrumbArea
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 52

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10

                    // Up button
                    Rectangle {
                        width: 32
                        height: 32
                        radius: 6
                        color: upMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(1, 1, 1, 0.05)
                        visible: (typeof fileBrowser !== "undefined") ? (fileBrowser.currentPath !== "") : true

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

                    // Breadcrumb path (shows "DriveName / path / ...")
                    Repeater {
                        model: typeof fileBrowser !== "undefined" ? fileBrowser.breadcrumb : []

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4

                            Text {
                                text: index > 0 ? " / " : ""
                                font.pixelSize: Theme.fontSizeMedium
                                font.family: Theme.fontFamily
                                color: Theme.textSecondary
                                visible: index > 0
                            }

                            Text {
                                text: modelData
                                font.pixelSize: Theme.fontSizeMedium
                                font.family: Theme.fontFamily
                                font.weight: index === (typeof fileBrowser !== "undefined" ? fileBrowser.breadcrumb.length - 1 : 0) ? Font.Normal : Font.Light
                                color: Theme.textPrimary
                            }
                        }
                    }
                }

                // Bottom separator
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.08)
                }
            }

            // File/Folder listing — larger rows (64px)
            ListView {
                id: fileListView
                anchors.top: breadcrumbArea.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                clip: true
                model: typeof fileBrowser !== "undefined" ? fileBrowser.currentEntries : []

                delegate: Rectangle {
                    width: fileListView.width
                    height: 64
                    color: fileMouseArea.pressed ? Qt.rgba(1, 1, 1, 0.08) : "transparent"

                    // Bottom border
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        height: 1
                        color: Qt.rgba(1, 1, 1, 0.04)
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 16

                        // Folder or file icon
                        Loader {
                            width: 32
                            height: 32
                            anchors.verticalCenter: parent.verticalCenter
                            sourceComponent: modelData.isDir ? folderIconComponent : null
                            active: modelData.isDir
                        }
                        Image {
                            width: 28
                            height: 28
                            source: Theme.imgPath + "mp3-hot.png"
                            fillMode: Image.PreserveAspectFit
                            anchors.verticalCenter: parent.verticalCenter
                            visible: !modelData.isDir
                        }

                        Column {
                            spacing: 3
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
                                    var files = fileBrowser.currentAudioFiles();
                                    audioPlayer.setPlaylist(files);
                                    var idx = files.indexOf(modelData.path);
                                    audioPlayer.playIndex(idx >= 0 ? idx : 0);
                                    if (root.StackView.view)
                                        root.StackView.view.pop();
                                }
                            }
                        }
                    }
                }
            }

            // Empty state
            Column {
                anchors.centerIn: parent
                spacing: 12
                visible: typeof fileBrowser === "undefined" || fileBrowser.currentEntries.length === 0

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: {
                        if (typeof fileBrowser === "undefined" || fileBrowser.currentPath === "")
                            return "Select a USB drive to browse";
                        return "No audio files found";
                    }
                    font.pixelSize: Theme.fontSizeMedium
                    font.family: Theme.fontFamily
                    font.weight: Font.Light
                    color: Theme.textMuted
                }
            }
        }
    }
}
