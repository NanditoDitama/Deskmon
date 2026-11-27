import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Dialog {
    id: root
    title: ""
    modal: true
    width: 480
    height: 520
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    property string updateState: "idle"
    property int updateProgress: 0
    property string newVersion: ""
    property string newReleaseNotes: ""
    property string errorMessage: ""

    readonly property bool isWorking: updateState === "checking" || updateState === "downloading" || updateState === "verifying" || updateState === "extracting"
    readonly property bool isReady: updateState === "ready"
    readonly property bool isManual: updateState === "manual"
    readonly property bool isError: updateState === "error"

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
        color: Theme.cardColor
        radius: Theme.radiusMedium
        border.width: 1
        border.color: Qt.rgba(255, 255, 255, 0.08)

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(255, 255, 255, 0.03) }
                GradientStop { position: 1.0; color: "transparent" }
            }
            opacity: 0.8
        }
    }

    contentItem: Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: Theme.radiusMedium

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 0

            // Header section
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80

                Rectangle {
                    width: 56
                    height: 56
                    radius: Theme.radiusMedium
                    color: root.isReady ? Theme.primaryColor : Theme.accentColor
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter

                    Rectangle {
                        width: 24
                        height: 24
                        radius: 12
                        color: "#ffffff"
                        anchors.centerIn: parent

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: root.isReady ? Theme.primaryColor : Theme.accentColor
                            anchors.centerIn: parent
                        }
                    }
                }

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 72
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4

                    Label {
                        text: root.isReady ? Lang.t("Ready to Restart") : Lang.t("Update Available")
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                        color: Theme.textColor
                    }

                    Label {
                        text: root.newVersion !== ""
                              ? (Lang.t("Version") + " " + root.newVersion)
                              : Lang.t("Checking...")
                        font.pixelSize: 14
                        color: root.isReady ? Theme.primaryColor : Theme.accentColor
                        font.weight: Font.Medium
                    }
                }
            }

            // Progress & status banner
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 58 : 0
                Layout.topMargin: 8
                visible: root.isWorking || root.isReady || root.isError
                radius: Theme.radiusSmall
                color: Qt.alpha(root.isError ? Theme.dangerColor : Theme.primaryColor, 0.08)
                border.width: 1
                border.color: Qt.alpha(root.isError ? Theme.dangerColor : Theme.primaryColor, 0.25)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: {
                                if (root.isReady) {
                                    return Lang.t("Update is ready (100%). Click Restart to Update now, or close and reopen Deskmon later to apply it.")
                                }
                                if (root.isError) {
                                    return root.errorMessage !== "" ? root.errorMessage : Lang.t("An Error Occurred")
                                }
                                if (root.updateState === "verifying" || root.updateState === "extracting") {
                                    return Lang.t("Updating") + " " + root.updateProgress + "%..."
                                }
                                if (root.updateState === "downloading") {
                                    return Lang.t("Updating") + " " + root.updateProgress + "%"
                                }
                                return Lang.t("Checking...")
                            }
                            elide: Text.ElideRight
                            font.pixelSize: 12
                            color: root.isError ? Theme.dangerColor : Theme.textColor
                        }
                        Label {
                            visible: root.isWorking || root.isReady
                            text: root.updateProgress + "%"
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            color: Theme.primaryColor
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 6
                        visible: !root.isError
                        radius: 3
                        color: Qt.alpha(Theme.textColor, 0.12)

                        Rectangle {
                            height: parent.height
                            radius: 3
                            width: Math.round(parent.width * (Math.min(100, Math.max(0, root.updateProgress)) / 100.0))
                            color: Theme.primaryColor
                            Behavior on width { NumberAnimation { duration: 160 } }
                        }
                    }
                }
            }

            // Separator line
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.topMargin: 14
                Layout.bottomMargin: 16
                color: Theme.dividerColor
            }

            // Release notes section
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 120
                clip: true
                ScrollBar.vertical: CustomScrollBar {}
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                background: Rectangle {
                    color: Qt.alpha(Theme.cardColor, 0.5)
                    radius: Theme.radiusSmall
                    border.width: 1
                    border.color: Theme.dividerColor
                }

                Label {
                    width: parent.width
                    text: root.newReleaseNotes
                    wrapMode: Text.Wrap
                    font.pixelSize: 13
                    color: Qt.alpha(Theme.textColor, 0.85)
                    lineHeight: 1.4
                    padding: 16
                }
            }

            // Spacer
            Item {
                Layout.fillHeight: false
                Layout.preferredHeight: 16
            }

            // Action buttons
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                spacing: 12

                Item { Layout.fillWidth: true }

                CustomButton {
                    id: laterBtn
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: Theme.controlHeight
                    text: Lang.t("Later")
                    btnColor: Qt.alpha(Theme.textColor, 0.06)
                    btnHoverColor: Qt.alpha(Theme.textColor, 0.12)
                    btnTextColor: Theme.textColor
                    btnBorderColor: Theme.dividerColor
                    btnBorderWidth: 1
                    font.weight: Font.Medium
                    onClicked: root.close()
                }

                CustomButton {
                    id: primaryActionBtn
                    visible: root.isReady || root.isManual || root.isError
                    Layout.preferredWidth: 165
                    Layout.preferredHeight: Theme.controlHeight
                    text: {
                        if (root.isReady) return Lang.t("Restart to Update")
                        if (root.isManual) return Lang.t("Open Releases")
                        return Lang.t("Retry")
                    }
                    btnColor: root.isReady ? Theme.primaryColor : Theme.accentColor
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
                        } else {
                            root.retryRequested()
                        }
                    }
                }
            }
        }
    }
}
