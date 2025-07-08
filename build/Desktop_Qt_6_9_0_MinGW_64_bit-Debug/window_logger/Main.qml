import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 400
    height: 300
    title: qsTr("Window Activity Logger")
    visible: true

    onClosing: function(close) {
        close.accepted = false
        window.hide()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        Label {
            text: "Activity Log Status"
            font.bold: true
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "Current Window: " + (logger.currentAppName || "Unknown")
            Layout.fillWidth: true
        }

        Label {
            text: "Window Title: " + (logger.currentWindowTitle || "Unknown")
            Layout.fillWidth: true
        }

        Label {
            text: "Log Count: " + logger.logCount
            Layout.fillWidth: true
        }

        Button {
            text: "Show Logs"
            Layout.alignment: Qt.AlignHCenter
            onClicked: logger.showLogs()
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: logView
                readOnly: true
                text: logger.logContent
                font.family: "Courier"
            }
        }
    }
}
