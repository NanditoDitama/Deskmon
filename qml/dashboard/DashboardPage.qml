import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../layouts"

Rectangle {
    id: root
    anchors.fill: parent
    color: Theme.backgroundColor

    property string username: ""

    signal profileRequested()
    signal logoutRequested()
    signal needReviewRequested(int taskId)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        HeaderLayout {
            username: root.username
            onProfileClicked: root.profileRequested()
            onLogoutClicked: root.logoutRequested()
        }

        BodyLayout {
            onNeedReviewRequested: function(taskId) {
                root.needReviewRequested(taskId)
            }
        }
    }
}
