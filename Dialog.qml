import QtQuick 2.15
import QtQuick.Controls 2.15

Dialog {
    id: applicationsDialog
    title: "Applications List"
    modal: true
    standardButtons: Dialog.Ok

    property alias productiveAppsModel: productiveAppsModel
    property alias nonProductiveAppsModel: nonProductiveAppsModel

    Column {
        spacing: 10
        padding: 10

        ListView {
            id: productiveListView
            width: parent.width
            height: 200
            model: productiveAppsModel
            cacheBuffer: 50
            delegate: Item {
                width: parent.width
                height: 60
                Rectangle {
                    width: parent.width
                    height: 60
                    color: "#ffffff"
                    border.color: "#cccccc"
                    border.width: 1
                    radius: 8

                    Row {
                        spacing: 10
                        anchors.centerIn: parent
                        padding: 10

                        Text {
                            text: model.appName
                            font.bold: true
                            font.pointSize: 16
                            color: "#333333"
                        }

                        Text {
                            text: model.window_title ? model.window_title : "No Title"
                            font.pointSize: 14
                            color: "#666666"
                        }
                    }
                }
            }
        }

        ListView {
            id: nonProductiveListView
            width: parent.width
            height: 200
            model: nonProductiveAppsModel
            cacheBuffer: 50
            delegate: Item {
                width: parent.width
                height: 60
                Rectangle {
                    width: parent.width
                    height: 60
                    color: "#ffffff"
                    border.color: "#cccccc"
                    border.width: 1
                    radius: 8

                    Row {
                        spacing: 10
                        anchors.centerIn: parent
                        padding: 10

                        Text {
                            text: model.appName
                            font.bold: true
                            font.pointSize: 16
                            color: "#333333"
                        }

                        Text {
                            text: model.window_title ? model.window_title : "No Title"
                            font.pointSize: 14
                            color: "#666666"
                        }
                    }
                }
            }
        }
    }
}
