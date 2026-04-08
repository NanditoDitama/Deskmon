// EarlyLogoutDialog.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dialog
    title: "Early Logout Reason"
    modal: true
    width: 400
    height: 300
    closePolicy: Popup.NoAutoClose

    property bool isClosingApp: false

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        Label {
            text: "You haven't completed your 8-hour work time yet."
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Label {
            text: "Time worked: " + workTimer.getFormattedElapsed()
            Layout.fillWidth: true
        }

        Label {
            text: "Please provide a reason for leaving early:"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        TextArea {
            id: reasonInput
            placeholderText: "Enter your reason here..."
            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: TextArea.Wrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10

            Button {
                text: "Cancel"
                onClicked: {
                    dialog.close()
                }
            }

            Button {
                text: "Submit"
                onClicked: {
                    if (reasonInput.text.trim().length > 0) {
                        logger.submitEarlyLogoutReason(reasonInput.text)

                        if (dialog.isClosingApp) {
                            Qt.quit()
                        } else {
                            // Proceed with normal logout
                            logger.logout()
                        }
                        dialog.close()
                    }
                }
            }
        }
    }
}
