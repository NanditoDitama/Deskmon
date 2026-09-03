pragma Singleton
import QtQuick

QtObject {
    id: theme

    // Mode
    property bool isDarkMode: false

    // Color Palette
    readonly property color primaryColor: "#00e0a8"
    readonly property color primaryHoverColor: "#00c493"
    readonly property color secondaryColor: "#3B82F6"
    readonly property color accentColor: "#F59E0B"
    readonly property color dangerColor: "#EF4444"
    readonly property color successColor: "#10B981"
    readonly property color warningColor: "#F59E0B"
    readonly property color infoColor: "#3B82F6"
    readonly property color neutralColor: "#bdbdbd"
    readonly property color nonProductiveColor: "#ff5100"
    readonly property color productiveColor: "#00e0a8"

    function alpha(colorVal, alphaVal) {
        return Qt.alpha(colorVal, alphaVal)
    }

    // Background & Card Colors
    readonly property color backgroundColor: isDarkMode ? "#121212" : "#F1F5F9"
    readonly property color cardColor: isDarkMode ? "#1E1E1E" : "#FFFFFF"
    readonly property color cardHeaderColor: isDarkMode ? "#252525" : "#F8FAFC"
    readonly property color surfaceColor: isDarkMode ? "#2A2A2A" : "#FFFFFF"

    // Text Colors
    readonly property color textColor: isDarkMode ? "#FFFFFF" : "#1F2937"
    readonly property color lightTextColor: isDarkMode ? "#B0B0B0" : "#6B7280"
    readonly property color mutedTextColor: isDarkMode ? "#808080" : "#9CA3AF"

    // Border & Divider Colors
    readonly property color dividerColor: isDarkMode ? "#333333" : "#E5E7EB"
    readonly property color borderColor: isDarkMode ? "#404040" : "#E2E8F0"
    readonly property color headerColor: isDarkMode ? "#1E1E1E" : "#00e0a8"
    readonly property color headers: isDarkMode ? "#1E1E1E" : "#00e0a8"

    // Selection Colors
    readonly property color selectedColor: "#3B82F6"
    readonly property color rangeColor: isDarkMode ? "#1E3A8A" : "#DBEAFE"

    // Radius
    readonly property real radiusSmall: 6
    readonly property real radiusMedium: 10
    readonly property real radiusLarge: 16
    readonly property real radiusRound: 9999

    // Font Sizes
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeBody: 14
    readonly property int fontSizeTitle: 16
    readonly property int fontSizeHeader: 20
    readonly property int fontSizeLargeHeader: 24

    // Helper method to toggle dark mode
    function toggleDarkMode() {
        isDarkMode = !isDarkMode
    }
}
