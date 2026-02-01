pragma Singleton
import QtQuick 2.15

QtObject {
    // ============================================================================
    // OpenAuto Automotive HMI Theme
    // Dark background with cyan/blue accents
    // Designed for touch interfaces with large targets
    // ============================================================================

    // Colors - Dark Automotive HMI with Cyan Accent
    readonly property color backgroundColor: "#0D0D0D"
    readonly property color cardColor: "#1A1F2E"
    readonly property color cardHoverColor: "#242B3D"
    readonly property color primaryColor: "#00B4D8"
    readonly property color secondaryColor: "#0077B6"
    readonly property color textPrimary: "#FFFFFF"
    readonly property color textSecondary: "#9CA3AF"
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
    readonly property int statusBarHeight: 56

    // Typography
    readonly property string fontFamily: "Roboto"
    readonly property int fontSizeXLarge: 32
    readonly property int fontSizeLarge: 24
    readonly property int fontSizeMedium: 18
    readonly property int fontSizeSmall: 14

    // Borders
    readonly property int borderWidth: 2

    // Transparency for overlays
    readonly property real overlayOpacity: 0.85
}
