import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "components"

// Main.qml - Root application window
// Automotive HMI with status bar and content area

ApplicationWindow {
    id: mainWindow

    visible: true
    visibility: Window.FullScreen
    title: "OpenAuto"
    color: Theme.backgroundColor

    // No animations - instant transitions for low-end hardware
    // StackView transitions disabled

    // Status bar at top
    StatusBar {
        id: statusBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        currentTime: typeof backend !== "undefined" ? backend.currentTime : "00:00"
        networkSSID: typeof backend !== "undefined" ? backend.networkSSID : ""
        bluetoothConnected: typeof backend !== "undefined" ? backend.bluetoothConnected : false
        wifiConnected: typeof backend !== "undefined" ? backend.wifiConnected : false
    }

    // Main content area
    StackView {
        id: stackView
        anchors.top: statusBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        // Disable all animations for performance
        pushEnter: null
        pushExit: null
        popEnter: null
        popExit: null
        replaceEnter: null
        replaceExit: null

        initialItem: homePageComponent
    }

    // Home page component
    Component {
        id: homePageComponent
        HomePage {}
    }

    // Settings page component (loaded on demand)
    Component {
        id: settingsPageComponent
        SettingsPage {}
    }

    // Functions for navigation (called from backend)
    function showHomePage() {
        stackView.replace(homePageComponent)
    }

    function showSettingsPage() {
        stackView.push(settingsPageComponent)
    }

    function goBack() {
        if (stackView.depth > 1) {
            stackView.pop()
        }
    }

    // Connection to handle backend signals
    Connections {
        target: typeof backend !== "undefined" ? backend : null

        function onShowSettings() {
            showSettingsPage()
        }

        function onShowHome() {
            showHomePage()
        }

        function onAndroidAutoStarted() {
            // Hide QML UI when Android Auto projection starts
            mainWindow.visible = false
        }

        function onAndroidAutoStopped() {
            // Show QML UI when Android Auto projection ends
            mainWindow.visible = true
            showHomePage()
        }
    }

    // Keyboard handling for back navigation
    Keys.onBackPressed: {
        if (stackView.depth > 1) {
            stackView.pop()
            event.accepted = true
        }
    }

    Keys.onEscapePressed: {
        if (stackView.depth > 1) {
            stackView.pop()
            event.accepted = true
        }
    }
}
