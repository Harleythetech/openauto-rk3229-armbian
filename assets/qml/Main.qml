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
    //visibility: Window.FullScreen
    width: 1024
    height: 600
    title: "OpenAuto - " + width + "x" + height
    color: Theme.gradientTop

    // Load Readex Pro static fonts (variable font not supported on Qt 5.15)
    FontLoader {
        source: Theme.imgPath + "font/ReadexPro-ExtraLight.ttf"
    }
    FontLoader {
        source: Theme.imgPath + "font/ReadexPro-Light.ttf"
    }
    FontLoader {
        source: Theme.imgPath + "font/ReadexPro-Regular.ttf"
    }
    FontLoader {
        source: Theme.imgPath + "font/ReadexPro-Medium.ttf"
    }
    FontLoader {
        source: Theme.imgPath + "font/ReadexPro-SemiBold.ttf"
    }
    FontLoader {
        source: Theme.imgPath + "font/ReadexPro-Bold.ttf"
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

        // Sync dock indicator whenever the active page changes
        onCurrentItemChanged: {
            if (!currentItem)
                return;
            switch (currentItem.objectName) {
            case "homePage":
                bottomDock.currentIndex = 0;
                break;
            case "musicPage":
                bottomDock.currentIndex = 1;
                break;
            case "fileBrowserPage":
                bottomDock.currentIndex = 1;
                break;
            case "settingsPage":
                bottomDock.currentIndex = 4;
                break;
            }
        }
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
        if (stackView.depth > 1)
            stackView.pop(null);
    }

    function showSettingsPage() {
        if (stackView.currentItem && stackView.currentItem.objectName === "settingsPage")
            return;
        if (stackView.depth > 1)
            stackView.pop(null);
        stackView.push(settingsPageComponent);
    }

    function showMusicPage() {
        if (stackView.currentItem && stackView.currentItem.objectName === "musicPage")
            return;
        // If on FileBrowser (sub-page of Music), pop to reveal Music beneath
        if (stackView.currentItem && stackView.currentItem.objectName === "fileBrowserPage") {
            stackView.pop();
            return;
        }
        if (stackView.depth > 1)
            stackView.pop(null);
        stackView.push(musicPageComponent);
    }

    function showFileBrowserPage() {
        if (stackView.currentItem && stackView.currentItem.objectName === "fileBrowserPage")
            return;
        stackView.push(fileBrowserPageComponent);
    }

    function goBack() {
        if (stackView.depth > 1)
            stackView.pop();
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
