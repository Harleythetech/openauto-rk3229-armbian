import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "."
import "components"

// SettingsPage - Complete settings with all original tabs
// Left sidebar with categories, right panel with scrollable content

Item {
    id: root

    property int selectedCategory: 0
    property var categories: ["General", "Video", "Audio", "Input", "Bluetooth", "WiFi", "System", "About"]

    Rectangle {
        anchors.fill: parent
        color: Theme.settingsBackground
    }

    // Left sidebar
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

            Repeater {
                model: root.categories

                Rectangle {
                    width: parent.width
                    height: 52
                    color: root.selectedCategory === index ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 4
                        height: parent.height * 0.6
                        color: root.selectedCategory === index ? Theme.primaryColor : "transparent"
                        radius: 2
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacing
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData
                        font.pixelSize: Theme.fontSizeLarge
                        font.family: Theme.fontFamily
                        font.weight: root.selectedCategory === index ? Font.Medium : Font.Light
                        color: Theme.textPrimary
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.selectedCategory = index
                    }
                }
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

        Item {
            id: contentHeader
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Theme.spacing
            height: 48

            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: root.categories[root.selectedCategory]
                font.pixelSize: Theme.fontSizeXLarge
                font.family: Theme.fontFamily
                font.weight: Font.Normal
                color: Theme.textPrimary
            }

            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: {
                    var dateStr = typeof backend !== "undefined" ? backend.currentDate : "February 1, 2026";
                    var timeStr = typeof backend !== "undefined" ? backend.currentTime : "00:00";
                    var ampmStr = typeof backend !== "undefined" ? backend.amPm : "";
                    if (ampmStr !== "")
                        return dateStr + " | " + timeStr + " " + ampmStr;
                    else
                        return dateStr + " | " + timeStr;
                }
                font.pixelSize: Theme.fontSizeMedium
                font.family: Theme.fontFamily
                font.weight: Font.ExtraLight
                color: Theme.textPrimary
            }
        }

        Flickable {
            id: contentFlickable
            anchors.top: contentHeader.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Theme.spacing
            anchors.topMargin: Theme.spacingSmall
            contentHeight: contentLoader.height
            clip: true

            Loader {
                id: contentLoader
                width: parent.width
                sourceComponent: {
                    switch (root.selectedCategory) {
                    case 0:
                        return generalContent;
                    case 1:
                        return videoContent;
                    case 2:
                        return audioContent;
                    case 3:
                        return inputContent;
                    case 4:
                        return bluetoothContent;
                    case 5:
                        return wifiContent;
                    case 6:
                        return systemContent;
                    case 7:
                        return aboutContent;
                    default:
                        return generalContent;
                    }
                }
            }
        }
    }

    // ========== GENERAL ==========
    Component {
        id: generalContent
        Column {
            spacing: Theme.spacingSmall
            width: parent.width

            SettingToggle {
                label: "24-Hour Time Format"
                checked: typeof backend !== "undefined" ? backend.use24HourFormat : true
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setUse24HourFormat(value);
                }
            }
            SettingToggle {
                label: "Show Clock"
                checked: typeof backend !== "undefined" ? backend.showClock : true
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setShowClock(value);
                }
            }
            SettingToggle {
                label: "Show Big Clock"
                checked: typeof backend !== "undefined" ? backend.showBigClock : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setShowBigClock(value);
                }
            }
            SettingToggle {
                label: "Show Cursor"
                checked: typeof backend !== "undefined" ? backend.showCursor : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setShowCursor(value);
                }
            }
            SettingToggle {
                label: "Hide Warning"
                checked: typeof backend !== "undefined" ? backend.hideWarning : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setHideWarning(value);
                }
            }
            SettingToggle {
                label: "Hide Menu Toggle"
                checked: typeof backend !== "undefined" ? backend.hideMenuToggle : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setHideMenuToggle(value);
                }
            }
            SettingToggle {
                label: "Show Network Info"
                checked: typeof backend !== "undefined" ? backend.showNetworkinfo : true
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setShowNetworkinfo(value);
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            Text {
                text: "UI Transparency"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
            Row {
                width: parent.width
                spacing: Theme.spacing
                Slider {
                    width: parent.width - 60
                    from: 0
                    to: 100
                    value: typeof backend !== "undefined" ? backend.alphaTrans : 100
                    onValueChanged: {
                        if (typeof backend !== "undefined")
                            backend.setAlphaTrans(value);
                    }
                }
                Text {
                    text: Math.round(typeof backend !== "undefined" ? backend.alphaTrans : 100) + "%"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            Text {
                text: "Traffic Handedness"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
            Row {
                spacing: Theme.spacing
                SettingRadio {
                    text: "Left Hand Drive"
                    checked: typeof backend !== "undefined" ? backend.leftHandDrive : true
                    onClicked: {
                        if (typeof backend !== "undefined")
                            backend.setLeftHandDrive(true);
                    }
                }
                SettingRadio {
                    text: "Right Hand Drive"
                    checked: typeof backend !== "undefined" ? !backend.leftHandDrive : false
                    onClicked: {
                        if (typeof backend !== "undefined")
                            backend.setLeftHandDrive(false);
                    }
                }
            }
        }
    }

    // ========== VIDEO ==========
    Component {
        id: videoContent
        Column {
            spacing: Theme.spacingSmall
            width: parent.width

            Text {
                text: "Resolution"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
            Row {
                spacing: Theme.spacing
                SettingRadio {
                    text: "480p"
                    checked: typeof backend !== "undefined" && backend.videoResolution === "800x480"
                    onClicked: {
                        if (typeof backend !== "undefined")
                            backend.setVideoResolution("800x480");
                    }
                }
                SettingRadio {
                    text: "720p"
                    checked: typeof backend !== "undefined" && backend.videoResolution === "1280x720"
                    onClicked: {
                        if (typeof backend !== "undefined")
                            backend.setVideoResolution("1280x720");
                    }
                }
                SettingRadio {
                    text: "1080p"
                    checked: typeof backend !== "undefined" && backend.videoResolution === "1920x1080"
                    onClicked: {
                        if (typeof backend !== "undefined")
                            backend.setVideoResolution("1920x1080");
                    }
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            Text {
                text: "Frame Rate"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
            Row {
                spacing: Theme.spacing
                SettingRadio {
                    text: "30 FPS"
                    checked: typeof backend !== "undefined" && backend.videoFPS === 30
                    onClicked: {
                        if (typeof backend !== "undefined")
                            backend.setVideoFPS(30);
                    }
                }
                SettingRadio {
                    text: "60 FPS"
                    checked: typeof backend !== "undefined" && backend.videoFPS === 60
                    onClicked: {
                        if (typeof backend !== "undefined")
                            backend.setVideoFPS(60);
                    }
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            Text {
                text: "Screen DPI: " + (typeof backend !== "undefined" ? backend.screenDPI : 160)
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
            Slider {
                width: parent.width - Theme.spacing * 2
                from: 100
                to: 300
                stepSize: 10
                value: typeof backend !== "undefined" ? backend.screenDPI : 160
                onValueChanged: {
                    if (typeof backend !== "undefined")
                        backend.setScreenDPI(value);
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            SettingRow {
                label: "Video Margin Width"
                value: typeof backend !== "undefined" ? backend.videoMarginWidth + "px" : "0px"
            }
            SettingRow {
                label: "Video Margin Height"
                value: typeof backend !== "undefined" ? backend.videoMarginHeight + "px" : "0px"
            }
            SettingRow {
                label: "OMX Layer Index"
                value: typeof backend !== "undefined" ? backend.omxLayerIndex.toString() : "0"
            }
        }
    }

    // ========== AUDIO ==========
    Component {
        id: audioContent
        Column {
            spacing: Theme.spacingSmall
            width: parent.width

            SettingRow {
                label: "Output Device"
                value: typeof backend !== "undefined" ? backend.audioOutputDevice : "Default"
            }
            SettingRow {
                label: "Input Device"
                value: typeof backend !== "undefined" ? backend.audioInputDevice : "Default"
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            Text {
                text: "Audio Channels"
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
                color: Theme.textPrimary
            }
            SettingToggle {
                label: "Music Channel"
                checked: typeof backend !== "undefined" ? backend.musicChannelEnabled : true
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setMusicChannelEnabled(value);
                }
            }
            SettingToggle {
                label: "Guidance/Speech Channel"
                checked: typeof backend !== "undefined" ? backend.guidanceChannelEnabled : true
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setGuidanceChannelEnabled(value);
                }
            }
            SettingToggle {
                label: "Telephony Channel"
                checked: typeof backend !== "undefined" ? backend.telephonyChannelEnabled : true
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setTelephonyChannelEnabled(value);
                }
            }
        }
    }

    // ========== INPUT ==========
    Component {
        id: inputContent
        Column {
            spacing: Theme.spacingSmall
            width: parent.width

            SettingToggle {
                label: "Enable Touchscreen"
                checked: typeof backend !== "undefined" ? backend.touchscreenEnabled : true
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setTouchscreenEnabled(value);
                }
            }
            SettingToggle {
                label: "Player Button Control"
                checked: typeof backend !== "undefined" ? backend.playerButtonControl : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setPlayerButtonControl(value);
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            Text {
                text: "Button Codes"
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
                color: Theme.textPrimary
            }

            GridLayout {
                columns: 2
                columnSpacing: Theme.spacing
                rowSpacing: 4
                width: parent.width

                SettingToggle {
                    label: "Play"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Pause"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Play/Pause Toggle"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Next Track"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Previous Track"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Home"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Phone"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Call End"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Voice Command"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Navigation"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Back"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Enter"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Left"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Right"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Up"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Down"
                    Layout.fillWidth: true
                }
                SettingToggle {
                    label: "Scroll Wheel"
                    Layout.fillWidth: true
                }
            }
        }
    }

    // ========== BLUETOOTH ==========
    Component {
        id: bluetoothContent
        Column {
            spacing: Theme.spacingSmall
            width: parent.width

            SettingRow {
                label: "Status"
                value: typeof backend !== "undefined" && backend.bluetoothConnected ? "Connected" : "Not Connected"
                valueColor: typeof backend !== "undefined" && backend.bluetoothConnected ? Theme.successColor : Theme.textSecondary
            }

            SettingRow {
                label: "Adapter"
                value: typeof backend !== "undefined" ? backend.bluetoothAdapter : "None"
            }

            SettingToggle {
                label: "Auto-Pair"
                checked: typeof backend !== "undefined" ? backend.bluetoothAutoPair : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setBluetoothAutoPair(value);
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            Rectangle {
                width: 140
                height: 40
                radius: Theme.buttonRadius
                color: unpairMouseArea.pressed ? Qt.darker(Theme.cardColor, 1.2) : Theme.cardColor

                Text {
                    anchors.centerIn: parent
                    text: "Unpair All"
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.textPrimary
                }

                MouseArea {
                    id: unpairMouseArea
                    anchors.fill: parent
                    onClicked: {
                        if (typeof backend !== "undefined")
                            backend.unpairAll();
                    }
                }
            }
        }
    }

    // ========== WIFI ==========
    Component {
        id: wifiContent
        Column {
            spacing: Theme.spacingSmall
            width: parent.width

            SettingRow {
                label: "Status"
                value: typeof backend !== "undefined" && backend.wifiConnected ? "Connected" : "Not Connected"
                valueColor: typeof backend !== "undefined" && backend.wifiConnected ? Theme.successColor : Theme.textSecondary
            }

            SettingRow {
                label: "Network (SSID)"
                value: typeof backend !== "undefined" && backend.networkSSID !== "" ? backend.networkSSID : "None"
            }
            SettingRow {
                label: "IP Address"
                value: typeof backend !== "undefined" ? backend.wifiIP : "N/A"
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            SettingToggle {
                label: "Enable Wireless Projection"
                checked: typeof backend !== "undefined" ? backend.wirelessProjectionEnabled : true
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setWirelessProjectionEnabled(value);
                }
            }
            SettingToggle {
                label: "Enable Hotspot on Boot"
                checked: typeof backend !== "undefined" ? backend.hotspotEnabled : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setHotspotEnabled(value);
                }
            }
        }
    }

    // ========== SYSTEM ==========
    Component {
        id: systemContent
        Column {
            spacing: Theme.spacingSmall
            width: parent.width

            SettingRow {
                label: "Free Memory"
                value: typeof backend !== "undefined" ? backend.freeMemory : "N/A"
            }
            SettingRow {
                label: "CPU Frequency"
                value: typeof backend !== "undefined" ? backend.cpuFrequency : "N/A"
            }
            SettingRow {
                label: "CPU Temperature"
                value: typeof backend !== "undefined" ? backend.cpuTemperature : "N/A"
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            Text {
                text: "Timers"
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
                color: Theme.textPrimary
            }
            SettingRow {
                label: "Screen Off Timer (sec)"
                value: typeof backend !== "undefined" ? backend.disconnectTimeout.toString() : "60"
            }
            SettingRow {
                label: "Shutdown Timer (min)"
                value: typeof backend !== "undefined" ? backend.shutdownTimeout.toString() : "0"
            }

            SettingToggle {
                label: "Disable Auto Shutdown"
                checked: typeof backend !== "undefined" ? backend.disableShutdown : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setDisableShutdown(value);
                }
            }
            SettingToggle {
                label: "Disable Screen Off"
                checked: typeof backend !== "undefined" ? backend.disableScreenOff : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setDisableScreenOff(value);
                }
            }
            SettingToggle {
                label: "Debug Mode"
                checked: typeof backend !== "undefined" ? backend.debugMode : false
                onToggled: function (value) {
                    if (typeof backend !== "undefined")
                        backend.setDebugMode(value);
                }
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            Row {
                spacing: Theme.spacing

                Rectangle {
                    width: 120
                    height: 40
                    radius: Theme.buttonRadius
                    color: saveMouseArea.pressed ? Qt.darker(Theme.successColor, 1.2) : Theme.successColor

                    Text {
                        anchors.centerIn: parent
                        text: "Save"
                        font.pixelSize: Theme.fontSizeMedium
                        color: "#FFFFFF"
                    }

                    MouseArea {
                        id: saveMouseArea
                        anchors.fill: parent
                        onClicked: {
                            if (typeof backend !== "undefined")
                                backend.saveSettings();
                        }
                    }
                }

                Rectangle {
                    width: 120
                    height: 40
                    radius: Theme.buttonRadius
                    color: resetMouseArea.pressed ? Qt.darker(Theme.errorColor, 1.2) : Theme.errorColor

                    Text {
                        anchors.centerIn: parent
                        text: "Reset"
                        font.pixelSize: Theme.fontSizeMedium
                        color: "#FFFFFF"
                    }

                    MouseArea {
                        id: resetMouseArea
                        anchors.fill: parent
                        onClicked: {
                            if (typeof backend !== "undefined")
                                backend.resetSettings();
                        }
                    }
                }
            }
        }
    }

    // ========== ABOUT ==========
    Component {
        id: aboutContent
        Column {
            spacing: Theme.spacingSmall
            width: parent.width

            Text {
                text: "OpenAuto"
                font.pixelSize: Theme.fontSizeXLarge
                font.bold: true
                color: Theme.textPrimary
            }
            Text {
                text: "for RK3229 Armbian"
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textSecondary
            }

            Item {
                width: 1
                height: Theme.spacing
            }

            SettingRow {
                label: "Version"
                value: typeof backend !== "undefined" ? backend.versionString : "0.0.0"
            }
            SettingRow {
                label: "Build Date"
                value: typeof backend !== "undefined" ? backend.buildDate : "Unknown"
            }
            SettingRow {
                label: "Qt Version"
                value: typeof backend !== "undefined" ? backend.qtVersion : "5.15"
            }
            SettingRow {
                label: "Devs, Contributors"
                value: "f1x.studio, Opencardev, Harleythetech, ilmich, jock"
            }

            Item {
                width: 1
                height: Theme.spacingLarge
            }

            Text {
                text: "Based on f1x.studio OpenAuto Modified for RK3229 SoC"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
                lineHeight: 1.4
            }
        }
    }

    // ========== Reusable Components ==========

    component SettingRow: Rectangle {
        property string label: ""
        property string value: ""
        property color valueColor: Theme.textPrimary

        width: parent.width
        height: 40
        color: "transparent"

        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: label
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            font.weight: Font.Light
            color: Theme.textPrimary
        }

        Text {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: value
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            font.weight: Font.Light
            color: valueColor
        }
    }

    component SettingToggle: Rectangle {
        property string label: ""
        property bool checked: false
        signal toggled(bool value)

        width: parent.width
        height: 44
        color: "transparent"

        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: label
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            font.weight: Font.Light
            color: Theme.textPrimary
        }

        Rectangle {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 48
            height: 26
            radius: 13
            color: checked ? Theme.primaryColor : Qt.rgba(1, 1, 1, 0.2)

            Rectangle {
                x: checked ? parent.width - width - 3 : 3
                anchors.verticalCenter: parent.verticalCenter
                width: 20
                height: 20
                radius: 10
                color: "#FFFFFF"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    checked = !checked;
                    toggled(checked);
                }
            }
        }
    }

    component SettingRadio: Rectangle {
        property string text: ""
        property bool checked: false
        signal clicked

        width: radioText.width + 30
        height: 32
        radius: 16
        color: checked ? Theme.primaryColor : Qt.rgba(1, 1, 1, 0.1)
        border.width: 1
        border.color: checked ? Theme.primaryColor : Qt.rgba(1, 1, 1, 0.2)

        Text {
            id: radioText
            anchors.centerIn: parent
            text: parent.text
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            font.weight: Font.Normal
            color: Theme.textPrimary
        }

        MouseArea {
            anchors.fill: parent
            onClicked: parent.clicked()
        }
    }
}
