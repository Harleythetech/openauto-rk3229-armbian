import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "."
import "components"

// Main.qml - Root application window
// Fullscreen layout with bottom dock navigation

ApplicationWindow {
    id: mainWindow

    visible: true
    visibility: Window.FullScreen
    title: "OpenAuto"
    color: Theme.gradientTop

    // Load Readex Pro font
    FontLoader {
        id: readexProFont
        source: "qrc:/ReadexPro-VariableFont_HEXP,wght.ttf"
    }

    // No animations - instant transitions for low-end hardware

    // Main content area (above dock)
    StackView {
        id: stackView
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomDock.top

        // Disable all animations for performance
        pushEnter: null
        pushExit: null
        popEnter: null
        popExit: null
        replaceEnter: null
        replaceExit: null

        initialItem: homePageComponent
    }

    // Bottom navigation dock
    BottomDock {
        id: bottomDock
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Theme.dockHeight

        onHomeClicked: showHomePage()
        onMusicClicked: showMusicPage()
        onAndroidAutoClicked: {
            if (typeof backend !== "undefined")
                backend.startAndroidAutoUSB();
        }
        onVolumeClicked: volumeOverlay.toggle()
        onSettingsClicked: showSettingsPage()
    }

    // Volume overlay (floats above dock)
    VolumeOverlay {
        id: volumeOverlay
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: bottomDock.top
        anchors.bottomMargin: 8
    }

    // Home page component
    Component {
        id: homePageComponent
        HomePage {
            onNavigateSettings: showSettingsPage()
            onNavigateMusic: showMusicPage()
        }
    }

    // Music player page component
    Component {
        id: musicPageComponent
        MusicPage {
            onOpenFileBrowser: showFileBrowserPage()
        }
    }

    // Settings page component (loaded on demand)
    Component {
        id: settingsPageComponent
        SettingsPage {}
    }

    // File browser page component
    Component {
        id: fileBrowserPageComponent
        FileBrowserPage {}
    }

    // Functions for navigation (called from backend)
    function showHomePage() {
        if (stackView.depth > 1) {
            stackView.pop(null); // Pop to root
        } else {
            stackView.replace(homePageComponent);
        }
    }

    function showSettingsPage() {
        stackView.push(settingsPageComponent);
    }

    function showMusicPage() {
        stackView.push(musicPageComponent);
    }

    function showFileBrowserPage() {
        stackView.push(fileBrowserPageComponent);
    }

    function goBack() {
        if (stackView.depth > 1) {
            stackView.pop();
        }
    }

    // Connection to handle backend signals
    Connections {
        target: typeof backend !== "undefined" ? backend : null

        function onShowSettings() {
            showSettingsPage();
        }

        function onShowHome() {
            showHomePage();
        }

        function onRequestGoBack() {
            goBack();
        }

        function onAndroidAutoStarted() {
            // Hide QML UI when Android Auto projection starts
            mainWindow.visible = false;
        }

        function onAndroidAutoStopped() {
            // Show QML UI when Android Auto projection ends
            // Don't navigate — preserve whatever page was active before AA started
            mainWindow.visible = true;
        }
    }

    // Keyboard handling for back navigation
    Keys.onBackPressed: {
        if (stackView.depth > 1) {
            stackView.pop();
            event.accepted = true;
        }
    }

    Keys.onEscapePressed: {
        if (stackView.depth > 1) {
            stackView.pop();
            event.accepted = true;
        }
    }
}
