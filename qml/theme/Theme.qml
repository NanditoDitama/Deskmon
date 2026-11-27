pragma Singleton
import QtQuick
import QtCore

QtObject {
    id: theme

    // Mode (persisted via QSettings)
    property bool isDarkMode: false
    property bool hasCustomTheme: false

    readonly property Settings appSettings: Settings {
        category: "ui"
        property alias isDarkMode: theme.isDarkMode
        property alias hasCustomTheme: theme.hasCustomTheme
    }

    // Color Palette
    readonly property color primaryColor: isDarkMode ? "#00e0a8" : "#047857"
    readonly property color primaryHoverColor: isDarkMode ? "#00c493" : "#065F46"
    readonly property color secondaryColor: "#3B82F6"
    readonly property color accentColor: isDarkMode ? "#F59E0B" : "#B45309"
    readonly property color dangerColor: isDarkMode ? "#EF4444" : "#DC2626"
    readonly property color successColor: isDarkMode ? "#10B981" : "#059669"
    readonly property color warningColor: isDarkMode ? "#F59E0B" : "#B45309"
    readonly property color infoColor: isDarkMode ? "#3B82F6" : "#2563EB"
    readonly property color neutralColor: isDarkMode ? "#bdbdbd" : "#94A3B8"
    readonly property color nonProductiveColor: isDarkMode ? "#ff5100" : "#DC2626"
    readonly property color productiveColor: isDarkMode ? "#00e0a8" : "#059669"

    function alpha(colorVal, alphaVal) {
        return Qt.alpha(colorVal, alphaVal)
    }

    // Background & Card Colors
    readonly property color backgroundColor: isDarkMode ? "#121212" : "#E8EDF3"
    readonly property color cardColor: isDarkMode ? "#1E1E1E" : "#FFFFFF"
    readonly property color cardHeaderColor: isDarkMode ? "#252525" : "#F8FAFC"
    readonly property color surfaceColor: isDarkMode ? "#2A2A2A" : "#FFFFFF"

    // Text Colors
    readonly property color textColor: isDarkMode ? "#FFFFFF" : "#0F172A"
    readonly property color lightTextColor: isDarkMode ? "#B0B0B0" : "#475569"
    readonly property color mutedTextColor: isDarkMode ? "#808080" : "#64748B"

    // Border & Divider Colors
    readonly property color dividerColor: isDarkMode ? "#333333" : "#E2E8F0"
    readonly property color borderColor: isDarkMode ? "#404040" : "#CBD5E1"
    readonly property color headerColor: isDarkMode ? "#1E1E1E" : "#FFFFFF"
    readonly property color headers: isDarkMode ? "#1E1E1E" : "#FFFFFF"

    // Selection Colors
    readonly property color selectedColor: "#3B82F6"
    readonly property color rangeColor: isDarkMode ? "#1E3A8A" : "#DBEAFE"

    // Radius
    readonly property real radiusSmall: 6
    readonly property real radiusMedium: 10
    readonly property real radiusLarge: 16
    readonly property real radiusRound: 9999

    // Spacing & Control Sizing
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 12
    readonly property int spacingLarge: 16
    readonly property int dialogPadding: 20
    readonly property int controlHeight: 32
    readonly property int controlHeightSmall: 28

    // Font Sizes
    readonly property int fontSizeSmall: 13
    readonly property int fontSizeBody: 14
    readonly property int fontSizeTitle: 16
    readonly property int fontSizeHeader: 20
    readonly property int fontSizeLargeHeader: 24

    signal themeToggleRequested()

    // Helper method to toggle dark mode
    function toggleDarkMode() {
        themeToggleRequested()
    }
}
