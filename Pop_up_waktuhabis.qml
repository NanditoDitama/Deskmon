import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

Window {
    id: popUpWindow
    width: 400
    height: 220
    title: qsTr("Peringatan")
    visible: false
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.WindowStaysOnTopHint

    // Sistem theme aware
    readonly property bool isDarkMode: Qt.application.styleHints.colorScheme === Qt.Dark
    property string newText: ""

    // Animasi
    Behavior on opacity {
        NumberAnimation { duration: 150 }
    }

    // Background dengan efek shadow modern
    Rectangle {
        anchors.fill: parent
        color: isDarkMode ? "#2d2d2d" : "#ffffff"
        border.color: isDarkMode ? "#444" : "#ddd"
        border.width: 1
        radius: 12
    }

    Column {
        anchors {
            fill: parent
            margins: 20
        }
        spacing: 24

        // Icon peringatan
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "⚠️"
            font.pixelSize: 32
        }

        // Teks pesan
        Label {
            id: labelText
            width: parent.width
            text: newText
            font.pixelSize: 16
            font.weight: Font.Medium
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            color: isDarkMode ? "#f0f0f0" : "#333333"
            topPadding: 4
            bottomPadding: 4
        }

        // Tombol aksi
        Button {
            id: closeButton
            anchors.horizontalCenter: parent.horizontalCenter
            width: 120
            height: 40

            background: Rectangle {
                radius: 6
                color: isDarkMode ? (closeButton.down ? "#3a6ea5" : (closeButton.hovered ? "#4a7eb5" : "#2d5d8c")) :
                                   (closeButton.down ? "#1a73e8" : (closeButton.hovered ? "#2b83ea" : "#007bff"))
                Behavior on color { ColorAnimation { duration: 100 } }
            }

            contentItem: Text {
                text: "Tutup"
                color: "white"
                font.pixelSize: 14
                font.weight: Font.Medium
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                popUpWindow.close()
            }
        }
    }

    minimumWidth: 320
    minimumHeight: 180
    maximumWidth: 600
    maximumHeight: 400

    // Animasi saat muncul
    function showAnimated() {
        opacity = 0
        scale = 0.95
        show()

        // Animasi paralel
        parallelAnimation.start()
    }

    ParallelAnimation {
        id: parallelAnimation
        NumberAnimation {
            target: popUpWindow
            property: "opacity"
            to: 1
            duration: 150
            easing.type: Easing.OutQuad
        }
        NumberAnimation {
            target: popUpWindow
            property: "scale"
            to: 1
            duration: 200
            easing.type: Easing.OutBack
        }
    }

    // Override show untuk selalu muncul di tengah
    onVisibleChanged: {
        if (visible) {
            x = (Screen.width - width) / 2
            y = (Screen.height - height) / 2
        }
    }
}
