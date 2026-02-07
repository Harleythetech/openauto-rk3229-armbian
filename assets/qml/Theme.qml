pragma Singleton
import QtQuick 2.15

QtObject {
    // ============================================================================
    // OpenAuto Automotive HMI Theme
    // Dark blue gradient background matching original UI
    // Designed for touch interfaces with large targets
    // ============================================================================

    // Background gradient colors (from original UI)
    readonly property color gradientTop: "#00021A"
    readonly property color gradientBottom: "#001D3F"

    // Solid backgrounds
    readonly property color backgroundColor: "#0D0D0D"
    readonly property color settingsBackground: "#1A1A1A"

    // Card and component colors
    readonly property color cardColor: "#1A1F2E"
    readonly property color cardHoverColor: "#242B3D"
    readonly property color dockColor: "#1A1A1A"
    readonly property color dockBorderColor: "#333333"

    // Accent colors
    readonly property color primaryColor: "#00B4D8"
    readonly property color secondaryColor: "#0077B6"
    readonly property color accentBlue: "#4A9FD4"

    // Text colors
    readonly property color textPrimary: "#FFFFFF"
    readonly property color textSecondary: "#9CA3AF"
    readonly property color textMuted: "#666666"

    // Status colors
    readonly property color dangerColor: "#E63946"
    readonly property color successColor: "#10B981"
    readonly property color warningColor: "#F59E0B"
    readonly property color borderColor: "#2A3040"

    // Button-specific colors
    readonly property color buttonAndroidAuto: "#0077B6"
    readonly property color buttonWifi: "#F59E0B"
    readonly property color buttonSettings: "#10B981"
    readonly property color buttonDayNight: "#6366F1"
    readonly property color buttonExit: "#E63946"
    readonly property color buttonMedia: "#8B5CF6"
    readonly property color buttonCamera: "#EC4899"

    // Dimensions - Large touch targets for automotive use
    readonly property int buttonHeight: 64
    readonly property int buttonRadius: 16
    readonly property int cardRadius: 12
    readonly property int spacing: 16
    readonly property int spacingSmall: 8
    readonly property int spacingLarge: 24
    readonly property int iconSize: 48
    readonly property int iconSizeSmall: 32
    readonly property int iconSizeLarge: 64

    // Dock bar
    readonly property int dockHeight: 80
    readonly property int dockIconSize: 40

    // Settings sidebar
    readonly property int sidebarWidth: 280
    readonly property int statusBarHeight: 56

    // Typography - Readex Pro variable font
    readonly property string fontFamily: "Readex Pro"

    // Font sizes (from Figma)
    readonly property int fontSizeClock: 128       // Home clock
    readonly property int fontSizeXXLarge: 96      // Fallback large
    readonly property int fontSizeXLarge: 36       // Titles, tabs
    readonly property int fontSizeLarge: 24        // Return button, date/time header
    readonly property int fontSizeMedium: 20       // Album name, content
    readonly property int fontSizeSmall: 16        // Artist, small text
    readonly property int fontSizeXSmall: 14       // Very small text

    // Borders
    readonly property int borderWidth: 1

    // Transparency for overlays
    readonly property real overlayOpacity: 0.85

    // Asset path resolver — works in both qrc:// (compiled) and local file (Design Studio/qmlscene)
    readonly property string imgPath: {
        var url = Qt.resolvedUrl(".");
        if (url.toString().indexOf("qrc:") === 0)
            return "qrc:/";
        return url + "../";
    }
}
