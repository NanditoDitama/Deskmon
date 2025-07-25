import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

Dialog {
    id: mainDialog
    title: "Dialog"
    width: 600
    height: 400
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    
    property alias dialogTitle: mainDialog.title
    property alias dialogContent: contentArea.children
    property alias acceptButtonText: acceptButton.text
    property alias cancelButtonText: cancelButton.text
    property bool showCancelButton: true
    property bool showAcceptButton: true
    
    // Signals for custom handling
    signal dialogAccepted()
    signal dialogRejected()
    signal dialogClosed()
    
    // Material design theming support
    Material.theme: window.isDarkMode ? Material.Dark : Material.Light
    
    background: Rectangle {
        color: window.cardColor
        radius: 12
        border.color: window.dividerColor
        border.width: 1
        
        // Shadow effect
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            color: "transparent"
            radius: parent.radius + 2
            border.color: Qt.rgba(0, 0, 0, 0.1)
            border.width: 1
            z: -1
        }
    }
    
    header: Rectangle {
        width: parent.width
        height: 60
        color: window.headers
        radius: 12
        
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: parent.radius
            color: parent.color
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            
            Text {
                text: mainDialog.title
                font.pixelSize: 18
                font.weight: Font.Medium
                color: "white"
                Layout.fillWidth: true
            }
            
            Button {
                id: closeButton
                width: 32
                height: 32
                background: Rectangle {
                    radius: 16
                    color: closeButton.hovered ? Qt.rgba(255, 255, 255, 0.2) : "transparent"
                }
                
                Text {
                    text: "×"
                    font.pixelSize: 20
                    color: "white"
                    anchors.centerIn: parent
                }
                
                onClicked: {
                    mainDialog.reject()
                    dialogClosed()
                }
            }
        }
    }
    
    contentItem: ColumnLayout {
        spacing: 20
        
        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            ColumnLayout {
                id: contentArea
                width: scrollView.width
                spacing: 15
                
                // Default content - can be replaced by setting dialogContent
                Text {
                    text: "Dialog content area. Set dialogContent property to customize."
                    color: window.textColor
                    font.pixelSize: 14
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }
        
        // Custom footer with themed buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            spacing: 10
            
            Button {
                id: cancelButton
                text: "Batal"
                visible: showCancelButton
                
                background: Rectangle {
                    radius: 6
                    color: cancelButton.pressed ? Qt.darker(window.dividerColor, 1.2) : 
                           cancelButton.hovered ? Qt.lighter(window.dividerColor, 1.1) : window.dividerColor
                    border.color: window.dividerColor
                    border.width: 1
                }
                
                contentItem: Text {
                    text: cancelButton.text
                    color: window.textColor
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    mainDialog.reject()
                    dialogRejected()
                }
            }
            
            Button {
                id: acceptButton
                text: "OK"
                visible: showAcceptButton
                
                background: Rectangle {
                    radius: 6
                    color: acceptButton.pressed ? Qt.darker(window.primaryColor, 1.2) : 
                           acceptButton.hovered ? Qt.lighter(window.primaryColor, 1.1) : window.primaryColor
                }
                
                contentItem: Text {
                    text: acceptButton.text
                    color: "white"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    mainDialog.accept()
                    dialogAccepted()
                }
            }
        }
    }
    
    // Animation for smooth appearance
    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: 200
            easing.type: Easing.OutQuad
        }
        NumberAnimation {
            property: "scale"
            from: 0.9
            to: 1
            duration: 200
            easing.type: Easing.OutQuad
        }
    }
    
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: 150
            easing.type: Easing.InQuad
        }
        NumberAnimation {
            property: "scale"
            from: 1
            to: 0.95
            duration: 150
            easing.type: Easing.InQuad
        }
    }
    
    // Convenience functions
    function showMessage(title, message) {
        dialogTitle = title
        // Clear existing content and add message
        for (var i = contentArea.children.length - 1; i >= 0; i--) {
            contentArea.children[i].destroy()
        }
        
        var messageText = Qt.createQmlObject('
            import QtQuick 2.15
            Text {
                text: "' + message + '"
                color: window.textColor
                font.pixelSize: 14
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        ', contentArea)
        
        open()
    }
    
    function showConfirmation(title, message, onConfirm) {
        dialogTitle = title
        showCancelButton = true
        showAcceptButton = true
        acceptButtonText = "Ya"
        cancelButtonText = "Tidak"
        
        // Clear existing content and add message
        for (var i = contentArea.children.length - 1; i >= 0; i--) {
            contentArea.children[i].destroy()
        }
        
        var messageText = Qt.createQmlObject('
            import QtQuick 2.15
            Text {
                text: "' + message + '"
                color: window.textColor
                font.pixelSize: 14
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        ', contentArea)
        
        // Connect to confirmation callback
        dialogAccepted.connect(onConfirm)
        
        open()
    }
    
    // Clean up connections when dialog is closed
    onClosed: {
        dialogAccepted.disconnect()
        dialogRejected.disconnect()
    }
}