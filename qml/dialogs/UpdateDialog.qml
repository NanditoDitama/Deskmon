import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Dialog {
    id: root
    title: ""
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    margins: 24
    implicitWidth: 450
    implicitHeight: contentCol.implicitHeight + 48
    width: Overlay.overlay ? Math.min(450, Math.max(320, Overlay.overlay.width - 48)) : 450
    height: contentCol.implicitHeight + 48

    padding: 0
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    property string currentVersion: ""
    property string updateState: "idle"
    property int updateProgress: 0
    property string newVersion: ""
    property string newReleaseNotes: ""
    property string errorMessage: ""

    readonly property bool isWorking: updateState === "checking" || updateState === "downloading" || updateState === "verifying" || updateState === "extracting"
    readonly property bool isReady: updateState === "ready"
    readonly property bool isManual: updateState === "manual"
    readonly property bool isError: updateState === "error"
    readonly property color statusAccent: isError ? Theme.dangerColor : (isManual ? Theme.warningColor : Theme.primaryColor)

    signal updateAccepted()
    signal restartRequested()
    signal retryRequested()

    function openWith(version, releaseNotes) {
        if (version !== undefined && version !== "")
            newVersion = version
        if (releaseNotes !== undefined)
            newReleaseNotes = releaseNotes
        open()
    }

    background: Rectangle {
        color: "transparent"
        radius: Theme.radiusLarge
    }

    contentItem: Rectangle {
        implicitWidth: root.width
        implicitHeight: contentCol.implicitHeight + 48
        color: Theme.cardColor
        radius: Theme.radiusLarge
        border.width: 1
        border.color: Theme.borderColor
        clip: true

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 3
            color: root.statusAccent
        }

        ColumnLayout {
            id: contentCol
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 24
            spacing: 16

            // Header: Icon + Title + Close button
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 42
                    Layout.preferredHeight: 42
                    radius: Theme.radiusMedium
                    color: Qt.alpha(root.statusAccent, 0.14)
                    border.width: 1
                    border.color: Qt.alpha(root.statusAccent, 0.32)

                    Image {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
                        source: root.isError ? ("image://icon/danger.svg?" + root.statusAccent) : "qrc:/icon.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        text: {
                            if (root.isError) return Lang.t("Update Error")
                            if (root.isReady) return Lang.t("Ready to Restart")
                            return Lang.t("Update Available")
                        }
                        font.pixelSize: 17
                        font.family: "Segoe UI"
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                    }

                    Label {
                        Layout.fillWidth: true
                        text: {
                            if (root.newVersion !== "" && root.currentVersion !== "")
                                return "v" + root.currentVersion + "  ->  v" + root.newVersion
                            if (root.newVersion !== "")
                                return Lang.t("Version") + " " + root.newVersion
                            return "Deskmon"
                        }
                        font.pixelSize: 12
                        font.family: "Segoe UI"
                        font.weight: Font.Medium
                        color: Theme.lightTextColor
                    }
                }

                Button {
                    id: closeBtn
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    Layout.alignment: Qt.AlignTop
                    padding: 0
                    flat: true
                    onClicked: root.close()
                    background: Rectangle {
                        radius: 6
                        color: closeBtn.hovered ? Qt.alpha(Theme.textColor, 0.08) : "transparent"
                    }
                    contentItem: Text {
                        text: "×"
                        font.pixelSize: 19
                        color: Theme.lightTextColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // Sanitized Status / Error Box (Zero internal URLs/paths/stacktraces)
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: statusCardCol.implicitHeight + 24
                radius: Theme.radiusMedium
                color: root.isError
                       ? Qt.alpha(Theme.dangerColor, 0.10)
                       : (Theme.isDarkMode ? Qt.rgba(1, 1, 1, 0.04) : Theme.cardHeaderColor)
                border.width: 1
                border.color: root.isError
                              ? Qt.alpha(Theme.dangerColor, 0.35)
                              : Theme.dividerColor

                ColumnLayout {
                    id: statusCardCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 12
                    spacing: 10

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        lineHeight: 1.4
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        color: root.isError
                               ? (Theme.isDarkMode ? "#FCA5A5" : Theme.dangerColor)
                               : Theme.textColor
                        text: {
                            if (root.isError) {
                                return root.errorMessage !== ""
                                    ? Lang.t(root.errorMessage)
                                    : Lang.t("Please check your internet connection or contact your administrator if this issue persists.")
                            }
                            if (root.isReady) {
                                return Lang.t("Update is ready (100%). Click Restart to Update now, or close and reopen Deskmon later to apply it.")
                            }
                            if (root.isWorking) {
                                return Lang.t("Downloading") + " " + root.updateProgress + "%"
                            }
                            if (root.isManual) {
                                return Lang.t("Download the latest installer from the Releases page to continue.")
                            }
                            return Lang.t("Checking...")
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 6
                        visible: root.isWorking || root.isReady
                        radius: 3
                        color: Qt.alpha(Theme.textColor, 0.12)

                        Rectangle {
                            height: parent.height
                            radius: 3
                            width: Math.round(parent.width * (Math.min(100, Math.max(0, root.updateProgress)) / 100.0))
                            color: root.statusAccent
                            Behavior on width { NumberAnimation { duration: 160 } }
                        }
                    }
                }
            }

            // Release Notes (only shown when non-error or if available for manual release)
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                visible: !root.isError && root.newReleaseNotes.trim() !== ""

                Label {
                    text: Lang.t("Release Notes")
                    font.pixelSize: 12
                    font.family: "Segoe UI"
                    font.weight: Font.DemiBold
                    color: Theme.mutedTextColor
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    clip: true
                    ScrollBar.vertical: CustomScrollBar {}
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    background: Rectangle {
                        color: Theme.isDarkMode ? Qt.rgba(0, 0, 0, 0.22) : Theme.cardHeaderColor
                        radius: Theme.radiusSmall
                        border.width: 1
                        border.color: Theme.dividerColor
                    }

                    Label {
                        width: parent.width
                        text: root.newReleaseNotes
                        wrapMode: Text.Wrap
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        color: Qt.alpha(Theme.textColor, 0.88)
                        lineHeight: 1.4
                        padding: 12
                    }
                }
            }

            // Footer: Close + Action button
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 2
                spacing: 10

                Item { Layout.fillWidth: true }

                CustomButton {
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 36
                    text: Lang.t("Close")
                    btnColor: Qt.alpha(Theme.textColor, 0.07)
                    btnHoverColor: Qt.alpha(Theme.textColor, 0.13)
                    btnTextColor: Theme.textColor
                    btnBorderColor: Theme.dividerColor
                    btnBorderWidth: 1
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    onClicked: root.close()
                }

                CustomButton {
                    visible: root.isReady || root.isManual || root.isError
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 36
                    text: {
                        if (root.isReady) return Lang.t("Restart Application")
                        if (root.isManual) return Lang.t("Open Releases")
                        return Lang.t("Retry Update")
                    }
                    btnColor: root.isError ? Theme.dangerColor : Theme.primaryColor
                    btnHoverColor: Qt.lighter(btnColor, 1.1)
                    btnTextColor: "#ffffff"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    onClicked: {
                        if (root.isReady) {
                            root.restartRequested()
                            root.close()
                        } else if (root.isManual) {
                            root.updateAccepted()
                            root.close()
                        } else if (root.isError) {
                            root.close()
                            root.retryRequested()
                        }
                    }
                }
            }
        }
    }
}
