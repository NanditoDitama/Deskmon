/****************************************************************************
** Meta object code from reading C++ file 'logger.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../logger.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'logger.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN6LoggerE_t {};
} // unnamed namespace

template <> constexpr inline auto Logger::qt_create_metaobjectdata<qt_meta_tag_ZN6LoggerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Logger",
        "currentAppNameChanged",
        "",
        "currentWindowTitleChanged",
        "logCountChanged",
        "logContentChanged",
        "productivityStatsChanged",
        "taskListChanged",
        "activeTaskChanged",
        "taskPausedChanged",
        "globalTimeUsageChanged",
        "trackingActiveChanged",
        "idleThresholdChanged",
        "currentUserIdChanged",
        "productivityAppsChanged",
        "loginCompleted",
        "success",
        "message",
        "authTokenChanged",
        "userEmailChanged",
        "currentUsernameChanged",
        "currentUserEmailChanged",
        "authTokenError",
        "profileImageChanged",
        "username",
        "newPath",
        "taskStatusChanged",
        "taskId",
        "newStatus",
        "taskReviewNotification",
        "taskTimeUpdated",
        "timeUsage",
        "showNotification",
        "workTimeElapsedSecondsChanged",
        "showTimeWarning",
        "earlyLeaveReasonSubmitted",
        "showStatusMessage",
        "statusMessageChanged",
        "updateAvailable",
        "newVersion",
        "releaseNotes",
        "requestLoginPage",
        "currentAppIconPathChanged",
        "showPingErrorDialog",
        "hidePingErrorDialog",
        "showAuthTokenErrorWindow",
        "requestTaskDetails",
        "action",
        "nextTaskId",
        "taskDetailsSubmissionSuccess",
        "taskDetailsSubmissionFailed",
        "error",
        "type",
        "readyToProceedWithQuit",
        "taskDetailsSubmissionFailedWithRetry",
        "errorMessage",
        "details",
        "readyToProceedWithLogout",
        "logActiveWindow",
        "logIdle",
        "startTime",
        "endTime",
        "refreshAll",
        "refreshTasks",
        "handleTaskStatusReply",
        "QNetworkReply*",
        "reply",
        "getPendingApplicationRequests",
        "QVariantList",
        "handleProductivityAppsResponse",
        "handleDailyUsageReportResponse",
        "submitEarlyLeaveReason",
        "reason",
        "startPingTimer",
        "handleTaskFetchReply",
        "fetchWorkTimeFromAPI",
        "handleFetchWorkTimeResponse",
        "debugShowRawData",
        "showLogs",
        "isUsernameTaken",
        "updateUserProfile",
        "currentUsername",
        "newUsername",
        "newPassword",
        "cropProfileImage",
        "imagePath",
        "x",
        "y",
        "imageWidth",
        "imageHeight",
        "cropWidth",
        "cropHeight",
        "clearLogFilter",
        "validateFilePath",
        "filePath",
        "setLogFilter",
        "startDate",
        "endDate",
        "updateProfileImage",
        "getProfileImagePath",
        "setActiveTask",
        "finishTask",
        "toggleTaskPause",
        "formatDuration",
        "seconds",
        "startGlobalTimer",
        "getUserPassword",
        "setIdleThreshold",
        "getAvailableApps",
        "addProductivityApp",
        "appName",
        "windowTitle",
        "url",
        "productivityType",
        "getProductivityApps",
        "authenticate",
        "loginInput",
        "password",
        "getUserDepartment",
        "getCurrentUsername",
        "getCurrentUserEmail",
        "getUserEmail",
        "fetchAndStoreTasks",
        "updateTaskStatus",
        "logout",
        "sendProductivityAppToAPI",
        "loadWorkTimeData",
        "checkAndCreateNewDayRecord",
        "calculateTodayProductiveSeconds",
        "sendProductiveTimeToAPI",
        "sendDailyUsageReport",
        "sendPing",
        "sendPausePlayDataToAPI",
        "status",
        "getAppProductivityType",
        "totalWorkSeconds",
        "launchMaintenanceTool",
        "checkForUpdates",
        "savedUsername",
        "savedPassword",
        "submitTaskDetails",
        "taskDetailsDialogClosed",
        "notify",
        "isTaskExpired",
        "getPendingStartedTaskCount",
        "currentAppName",
        "currentWindowTitle",
        "logCount",
        "logContent",
        "productivityStats",
        "QVariantMap",
        "taskList",
        "activeTaskId",
        "isTaskPaused",
        "globalTimeUsage",
        "isTrackingActive",
        "currentUserId",
        "productiveAppsModel",
        "QAbstractItemModel*",
        "nonProductiveAppsModel",
        "authToken",
        "userEmail",
        "currentUserEmail",
        "workTimeElapsedSeconds",
        "logModel",
        "QSqlQueryModel*"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'currentAppNameChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentWindowTitleChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'logCountChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'logContentChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'productivityStatsChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'taskListChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'activeTaskChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'taskPausedChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'globalTimeUsageChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'trackingActiveChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'idleThresholdChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentUserIdChanged'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'productivityAppsChanged'
        QtMocHelpers::SignalData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'loginCompleted'
        QtMocHelpers::SignalData<void(bool, const QString &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 16 }, { QMetaType::QString, 17 },
        }}),
        // Signal 'authTokenChanged'
        QtMocHelpers::SignalData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'userEmailChanged'
        QtMocHelpers::SignalData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentUsernameChanged'
        QtMocHelpers::SignalData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentUserEmailChanged'
        QtMocHelpers::SignalData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'authTokenError'
        QtMocHelpers::SignalData<void(const QString &)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Signal 'profileImageChanged'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 }, { QMetaType::QString, 25 },
        }}),
        // Signal 'taskStatusChanged'
        QtMocHelpers::SignalData<void(int, const QString &)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 }, { QMetaType::QString, 28 },
        }}),
        // Signal 'taskReviewNotification'
        QtMocHelpers::SignalData<void(const QString &)>(29, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Signal 'taskTimeUpdated'
        QtMocHelpers::SignalData<void(int, int)>(30, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 }, { QMetaType::Int, 31 },
        }}),
        // Signal 'showNotification'
        QtMocHelpers::SignalData<void(const QString &)>(32, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Signal 'workTimeElapsedSecondsChanged'
        QtMocHelpers::SignalData<void()>(33, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'showTimeWarning'
        QtMocHelpers::SignalData<void(const QString &)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Signal 'earlyLeaveReasonSubmitted'
        QtMocHelpers::SignalData<void()>(35, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'showStatusMessage'
        QtMocHelpers::SignalData<void(const QString &)>(36, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Signal 'statusMessageChanged'
        QtMocHelpers::SignalData<void()>(37, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'updateAvailable'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(38, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 39 }, { QMetaType::QString, 40 },
        }}),
        // Signal 'requestLoginPage'
        QtMocHelpers::SignalData<void()>(41, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentAppIconPathChanged'
        QtMocHelpers::SignalData<void()>(42, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'showPingErrorDialog'
        QtMocHelpers::SignalData<void(const QString &)>(43, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Signal 'hidePingErrorDialog'
        QtMocHelpers::SignalData<void()>(44, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'showAuthTokenErrorWindow'
        QtMocHelpers::SignalData<void(const QString &)>(45, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Signal 'requestTaskDetails'
        QtMocHelpers::SignalData<void(int, const QString &, int)>(46, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 }, { QMetaType::QString, 47 }, { QMetaType::Int, 48 },
        }}),
        // Signal 'taskDetailsSubmissionSuccess'
        QtMocHelpers::SignalData<void()>(49, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'taskDetailsSubmissionFailed'
        QtMocHelpers::SignalData<void(const QString &)>(50, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 51 },
        }}),
        // Signal 'showNotification'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(32, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 52 }, { QMetaType::QString, 17 },
        }}),
        // Signal 'readyToProceedWithQuit'
        QtMocHelpers::SignalData<void()>(53, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'taskDetailsSubmissionFailedWithRetry'
        QtMocHelpers::SignalData<void(const QString &, int, const QString &, const QString &, int)>(54, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 55 }, { QMetaType::Int, 27 }, { QMetaType::QString, 56 }, { QMetaType::QString, 47 },
            { QMetaType::Int, 48 },
        }}),
        // Signal 'readyToProceedWithLogout'
        QtMocHelpers::SignalData<void()>(57, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'logActiveWindow'
        QtMocHelpers::SlotData<void()>(58, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'logIdle'
        QtMocHelpers::SlotData<void(qint64, qint64)>(59, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::LongLong, 60 }, { QMetaType::LongLong, 61 },
        }}),
        // Slot 'refreshAll'
        QtMocHelpers::SlotData<void()>(62, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'refreshTasks'
        QtMocHelpers::SlotData<void()>(63, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'handleTaskStatusReply'
        QtMocHelpers::SlotData<void(QNetworkReply *, int)>(64, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 65, 66 }, { QMetaType::Int, 27 },
        }}),
        // Slot 'getPendingApplicationRequests'
        QtMocHelpers::SlotData<QVariantList()>(67, 2, QMC::AccessPublic, 0x80000000 | 68),
        // Slot 'handleProductivityAppsResponse'
        QtMocHelpers::SlotData<void(QNetworkReply *)>(69, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 65, 66 },
        }}),
        // Slot 'handleDailyUsageReportResponse'
        QtMocHelpers::SlotData<void(QNetworkReply *)>(70, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 65, 66 },
        }}),
        // Slot 'submitEarlyLeaveReason'
        QtMocHelpers::SlotData<void(const QString &)>(71, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 72 },
        }}),
        // Slot 'startPingTimer'
        QtMocHelpers::SlotData<void()>(73, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'handleTaskFetchReply'
        QtMocHelpers::SlotData<void(QNetworkReply *)>(74, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 65, 66 },
        }}),
        // Slot 'fetchWorkTimeFromAPI'
        QtMocHelpers::SlotData<void()>(75, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleFetchWorkTimeResponse'
        QtMocHelpers::SlotData<void(QNetworkReply *)>(76, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 65, 66 },
        }}),
        // Method 'debugShowRawData'
        QtMocHelpers::MethodData<QString() const>(77, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'showLogs'
        QtMocHelpers::MethodData<void()>(78, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'isUsernameTaken'
        QtMocHelpers::MethodData<bool(const QString &)>(79, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'updateUserProfile'
        QtMocHelpers::MethodData<QString(const QString &, const QString &, const QString &)>(80, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 81 }, { QMetaType::QString, 82 }, { QMetaType::QString, 83 },
        }}),
        // Method 'cropProfileImage'
        QtMocHelpers::MethodData<QString(const QString &, qreal, qreal, qreal, qreal, qreal, qreal)>(84, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 85 }, { QMetaType::QReal, 86 }, { QMetaType::QReal, 87 }, { QMetaType::QReal, 88 },
            { QMetaType::QReal, 89 }, { QMetaType::QReal, 90 }, { QMetaType::QReal, 91 },
        }}),
        // Method 'clearLogFilter'
        QtMocHelpers::MethodData<void()>(92, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'validateFilePath'
        QtMocHelpers::MethodData<bool(const QString &)>(93, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 94 },
        }}),
        // Method 'setLogFilter'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(95, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 96 }, { QMetaType::QString, 97 },
        }}),
        // Method 'updateProfileImage'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(98, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 24 }, { QMetaType::QString, 85 },
        }}),
        // Method 'getProfileImagePath'
        QtMocHelpers::MethodData<QString(const QString &)>(99, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'setActiveTask'
        QtMocHelpers::MethodData<void(int)>(100, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Method 'finishTask'
        QtMocHelpers::MethodData<void(int)>(101, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Method 'toggleTaskPause'
        QtMocHelpers::MethodData<void()>(102, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'formatDuration'
        QtMocHelpers::MethodData<QString(int) const>(103, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::Int, 104 },
        }}),
        // Method 'startGlobalTimer'
        QtMocHelpers::MethodData<void()>(105, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'getUserPassword'
        QtMocHelpers::MethodData<QString(const QString &)>(106, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'setIdleThreshold'
        QtMocHelpers::MethodData<void(int)>(107, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 104 },
        }}),
        // Method 'getAvailableApps'
        QtMocHelpers::MethodData<QVariantList() const>(108, 2, QMC::AccessPublic, 0x80000000 | 68),
        // Method 'addProductivityApp'
        QtMocHelpers::MethodData<void(const QString &, const QString &, const QString &, int)>(109, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 110 }, { QMetaType::QString, 111 }, { QMetaType::QString, 112 }, { QMetaType::Int, 113 },
        }}),
        // Method 'getProductivityApps'
        QtMocHelpers::MethodData<QVariantList() const>(114, 2, QMC::AccessPublic, 0x80000000 | 68),
        // Method 'authenticate'
        QtMocHelpers::MethodData<QString(const QString &, const QString &)>(115, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 116 }, { QMetaType::QString, 117 },
        }}),
        // Method 'getUserDepartment'
        QtMocHelpers::MethodData<QString(const QString &)>(118, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'getCurrentUsername'
        QtMocHelpers::MethodData<QString() const>(119, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'getCurrentUserEmail'
        QtMocHelpers::MethodData<QString() const>(120, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'getUserEmail'
        QtMocHelpers::MethodData<QString(const QString &)>(121, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'fetchAndStoreTasks'
        QtMocHelpers::MethodData<void()>(122, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'updateTaskStatus'
        QtMocHelpers::MethodData<void(int)>(123, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Method 'logout'
        QtMocHelpers::MethodData<void()>(124, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'sendProductivityAppToAPI'
        QtMocHelpers::MethodData<void(const QString &, const QString &, const QString &, int)>(125, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 110 }, { QMetaType::QString, 111 }, { QMetaType::QString, 112 }, { QMetaType::Int, 113 },
        }}),
        // Method 'loadWorkTimeData'
        QtMocHelpers::MethodData<void()>(126, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'checkAndCreateNewDayRecord'
        QtMocHelpers::MethodData<void()>(127, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'calculateTodayProductiveSeconds'
        QtMocHelpers::MethodData<int() const>(128, 2, QMC::AccessPublic, QMetaType::Int),
        // Method 'sendProductiveTimeToAPI'
        QtMocHelpers::MethodData<void()>(129, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'sendDailyUsageReport'
        QtMocHelpers::MethodData<void()>(130, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'sendPing'
        QtMocHelpers::MethodData<void(int)>(131, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Method 'sendPausePlayDataToAPI'
        QtMocHelpers::MethodData<void(int, const QString &, const QString &, const QString &)>(132, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 }, { QMetaType::QString, 60 }, { QMetaType::QString, 61 }, { QMetaType::QString, 133 },
        }}),
        // Method 'getAppProductivityType'
        QtMocHelpers::MethodData<int(const QString &, const QString &) const>(134, 2, QMC::AccessPublic, QMetaType::Int, {{
            { QMetaType::QString, 110 }, { QMetaType::QString, 112 },
        }}),
        // Method 'totalWorkSeconds'
        QtMocHelpers::MethodData<int() const>(135, 2, QMC::AccessPublic, QMetaType::Int),
        // Method 'launchMaintenanceTool'
        QtMocHelpers::MethodData<void()>(136, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'checkForUpdates'
        QtMocHelpers::MethodData<void()>(137, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'savedUsername'
        QtMocHelpers::MethodData<QString() const>(138, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'savedPassword'
        QtMocHelpers::MethodData<QString() const>(139, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'submitTaskDetails'
        QtMocHelpers::MethodData<void(int, const QString &, const QString &, int)>(140, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 }, { QMetaType::QString, 56 }, { QMetaType::QString, 47 }, { QMetaType::Int, 48 },
        }}),
        // Method 'submitTaskDetails'
        QtMocHelpers::MethodData<void(int, const QString &, const QString &)>(140, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::Int, 27 }, { QMetaType::QString, 56 }, { QMetaType::QString, 47 },
        }}),
        // Method 'taskDetailsDialogClosed'
        QtMocHelpers::MethodData<void(const QString &)>(141, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 47 },
        }}),
        // Method 'notify'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(142, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 52 }, { QMetaType::QString, 17 },
        }}),
        // Method 'isTaskExpired'
        QtMocHelpers::MethodData<bool(int)>(143, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::Int, 27 },
        }}),
        // Method 'getPendingStartedTaskCount'
        QtMocHelpers::MethodData<int()>(144, 2, QMC::AccessPublic, QMetaType::Int),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'currentAppName'
        QtMocHelpers::PropertyData<QString>(145, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'currentWindowTitle'
        QtMocHelpers::PropertyData<QString>(146, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'logCount'
        QtMocHelpers::PropertyData<int>(147, QMetaType::Int, QMC::DefaultPropertyFlags, 2),
        // property 'logContent'
        QtMocHelpers::PropertyData<QString>(148, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'productivityStats'
        QtMocHelpers::PropertyData<QVariantMap>(149, 0x80000000 | 150, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 4),
        // property 'taskList'
        QtMocHelpers::PropertyData<QVariantList>(151, 0x80000000 | 68, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 5),
        // property 'activeTaskId'
        QtMocHelpers::PropertyData<int>(152, QMetaType::Int, QMC::DefaultPropertyFlags, 6),
        // property 'isTaskPaused'
        QtMocHelpers::PropertyData<bool>(153, QMetaType::Bool, QMC::DefaultPropertyFlags, 7),
        // property 'globalTimeUsage'
        QtMocHelpers::PropertyData<qint64>(154, QMetaType::LongLong, QMC::DefaultPropertyFlags, 8),
        // property 'isTrackingActive'
        QtMocHelpers::PropertyData<bool>(155, QMetaType::Bool, QMC::DefaultPropertyFlags, 9),
        // property 'currentUserId'
        QtMocHelpers::PropertyData<int>(156, QMetaType::Int, QMC::DefaultPropertyFlags, 11),
        // property 'productiveAppsModel'
        QtMocHelpers::PropertyData<QAbstractItemModel*>(157, 0x80000000 | 158, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 12),
        // property 'nonProductiveAppsModel'
        QtMocHelpers::PropertyData<QAbstractItemModel*>(159, 0x80000000 | 158, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 12),
        // property 'authToken'
        QtMocHelpers::PropertyData<QString>(160, QMetaType::QString, QMC::DefaultPropertyFlags, 14),
        // property 'userEmail'
        QtMocHelpers::PropertyData<QString>(161, QMetaType::QString, QMC::DefaultPropertyFlags, 15),
        // property 'currentUsername'
        QtMocHelpers::PropertyData<QString>(81, QMetaType::QString, QMC::DefaultPropertyFlags, 16),
        // property 'currentUserEmail'
        QtMocHelpers::PropertyData<QString>(162, QMetaType::QString, QMC::DefaultPropertyFlags, 17),
        // property 'workTimeElapsedSeconds'
        QtMocHelpers::PropertyData<int>(163, QMetaType::Int, QMC::DefaultPropertyFlags, 24),
        // property 'logModel'
        QtMocHelpers::PropertyData<QSqlQueryModel*>(164, 0x80000000 | 165, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Logger, qt_meta_tag_ZN6LoggerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Logger::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6LoggerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6LoggerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6LoggerE_t>.metaTypes,
    nullptr
} };

void Logger::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Logger *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->currentAppNameChanged(); break;
        case 1: _t->currentWindowTitleChanged(); break;
        case 2: _t->logCountChanged(); break;
        case 3: _t->logContentChanged(); break;
        case 4: _t->productivityStatsChanged(); break;
        case 5: _t->taskListChanged(); break;
        case 6: _t->activeTaskChanged(); break;
        case 7: _t->taskPausedChanged(); break;
        case 8: _t->globalTimeUsageChanged(); break;
        case 9: _t->trackingActiveChanged(); break;
        case 10: _t->idleThresholdChanged(); break;
        case 11: _t->currentUserIdChanged(); break;
        case 12: _t->productivityAppsChanged(); break;
        case 13: _t->loginCompleted((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 14: _t->authTokenChanged(); break;
        case 15: _t->userEmailChanged(); break;
        case 16: _t->currentUsernameChanged(); break;
        case 17: _t->currentUserEmailChanged(); break;
        case 18: _t->authTokenError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 19: _t->profileImageChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 20: _t->taskStatusChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 21: _t->taskReviewNotification((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 22: _t->taskTimeUpdated((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 23: _t->showNotification((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: _t->workTimeElapsedSecondsChanged(); break;
        case 25: _t->showTimeWarning((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 26: _t->earlyLeaveReasonSubmitted(); break;
        case 27: _t->showStatusMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 28: _t->statusMessageChanged(); break;
        case 29: _t->updateAvailable((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 30: _t->requestLoginPage(); break;
        case 31: _t->currentAppIconPathChanged(); break;
        case 32: _t->showPingErrorDialog((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 33: _t->hidePingErrorDialog(); break;
        case 34: _t->showAuthTokenErrorWindow((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 35: _t->requestTaskDetails((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3]))); break;
        case 36: _t->taskDetailsSubmissionSuccess(); break;
        case 37: _t->taskDetailsSubmissionFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 38: _t->showNotification((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 39: _t->readyToProceedWithQuit(); break;
        case 40: _t->taskDetailsSubmissionFailedWithRetry((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[5]))); break;
        case 41: _t->readyToProceedWithLogout(); break;
        case 42: _t->logActiveWindow(); break;
        case 43: _t->logIdle((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 44: _t->refreshAll(); break;
        case 45: _t->refreshTasks(); break;
        case 46: _t->handleTaskStatusReply((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 47: { QVariantList _r = _t->getPendingApplicationRequests();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 48: _t->handleProductivityAppsResponse((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 49: _t->handleDailyUsageReportResponse((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 50: _t->submitEarlyLeaveReason((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 51: _t->startPingTimer(); break;
        case 52: _t->handleTaskFetchReply((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 53: _t->fetchWorkTimeFromAPI(); break;
        case 54: _t->handleFetchWorkTimeResponse((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 55: { QString _r = _t->debugShowRawData();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 56: _t->showLogs(); break;
        case 57: { bool _r = _t->isUsernameTaken((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 58: { QString _r = _t->updateUserProfile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 59: { QString _r = _t->cropProfileImage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[7])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 60: _t->clearLogFilter(); break;
        case 61: { bool _r = _t->validateFilePath((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 62: _t->setLogFilter((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 63: { bool _r = _t->updateProfileImage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 64: { QString _r = _t->getProfileImagePath((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 65: _t->setActiveTask((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 66: _t->finishTask((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 67: _t->toggleTaskPause(); break;
        case 68: { QString _r = _t->formatDuration((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 69: _t->startGlobalTimer(); break;
        case 70: { QString _r = _t->getUserPassword((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 71: _t->setIdleThreshold((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 72: { QVariantList _r = _t->getAvailableApps();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 73: _t->addProductivityApp((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 74: { QVariantList _r = _t->getProductivityApps();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 75: { QString _r = _t->authenticate((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 76: { QString _r = _t->getUserDepartment((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 77: { QString _r = _t->getCurrentUsername();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 78: { QString _r = _t->getCurrentUserEmail();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 79: { QString _r = _t->getUserEmail((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 80: _t->fetchAndStoreTasks(); break;
        case 81: _t->updateTaskStatus((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 82: _t->logout(); break;
        case 83: _t->sendProductivityAppToAPI((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 84: _t->loadWorkTimeData(); break;
        case 85: _t->checkAndCreateNewDayRecord(); break;
        case 86: { int _r = _t->calculateTodayProductiveSeconds();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 87: _t->sendProductiveTimeToAPI(); break;
        case 88: _t->sendDailyUsageReport(); break;
        case 89: _t->sendPing((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 90: _t->sendPausePlayDataToAPI((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 91: { int _r = _t->getAppProductivityType((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 92: { int _r = _t->totalWorkSeconds();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 93: _t->launchMaintenanceTool(); break;
        case 94: _t->checkForUpdates(); break;
        case 95: { QString _r = _t->savedUsername();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 96: { QString _r = _t->savedPassword();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 97: _t->submitTaskDetails((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 98: _t->submitTaskDetails((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 99: _t->taskDetailsDialogClosed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 100: _t->notify((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 101: { bool _r = _t->isTaskExpired((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 102: { int _r = _t->getPendingStartedTaskCount();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 46:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 48:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 49:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 52:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 54:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::currentAppNameChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::currentWindowTitleChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::logCountChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::logContentChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::productivityStatsChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::taskListChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::activeTaskChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::taskPausedChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::globalTimeUsageChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::trackingActiveChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::idleThresholdChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::currentUserIdChanged, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::productivityAppsChanged, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(bool , const QString & )>(_a, &Logger::loginCompleted, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::authTokenChanged, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::userEmailChanged, 15))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::currentUsernameChanged, 16))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::currentUserEmailChanged, 17))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & )>(_a, &Logger::authTokenError, 18))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & , const QString & )>(_a, &Logger::profileImageChanged, 19))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(int , const QString & )>(_a, &Logger::taskStatusChanged, 20))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & )>(_a, &Logger::taskReviewNotification, 21))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(int , int )>(_a, &Logger::taskTimeUpdated, 22))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & )>(_a, &Logger::showNotification, 23))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::workTimeElapsedSecondsChanged, 24))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & )>(_a, &Logger::showTimeWarning, 25))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::earlyLeaveReasonSubmitted, 26))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & )>(_a, &Logger::showStatusMessage, 27))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::statusMessageChanged, 28))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & , const QString & )>(_a, &Logger::updateAvailable, 29))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::requestLoginPage, 30))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::currentAppIconPathChanged, 31))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & )>(_a, &Logger::showPingErrorDialog, 32))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::hidePingErrorDialog, 33))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & )>(_a, &Logger::showAuthTokenErrorWindow, 34))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(int , const QString & , int )>(_a, &Logger::requestTaskDetails, 35))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::taskDetailsSubmissionSuccess, 36))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & )>(_a, &Logger::taskDetailsSubmissionFailed, 37))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & , const QString & )>(_a, &Logger::showNotification, 38))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::readyToProceedWithQuit, 39))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)(const QString & , int , const QString & , const QString & , int )>(_a, &Logger::taskDetailsSubmissionFailedWithRetry, 40))
            return;
        if (QtMocHelpers::indexOfMethod<void (Logger::*)()>(_a, &Logger::readyToProceedWithLogout, 41))
            return;
    }
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 12:
        case 11:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractItemModel* >(); break;
        case 18:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QSqlQueryModel* >(); break;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->currentAppName(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->currentWindowTitle(); break;
        case 2: *reinterpret_cast<int*>(_v) = _t->logCount(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->logContent(); break;
        case 4: *reinterpret_cast<QVariantMap*>(_v) = _t->productivityStats(); break;
        case 5: *reinterpret_cast<QVariantList*>(_v) = _t->taskList(); break;
        case 6: *reinterpret_cast<int*>(_v) = _t->activeTaskId(); break;
        case 7: *reinterpret_cast<bool*>(_v) = _t->isTaskPaused(); break;
        case 8: *reinterpret_cast<qint64*>(_v) = _t->globalTimeUsage(); break;
        case 9: *reinterpret_cast<bool*>(_v) = _t->isTrackingActive(); break;
        case 10: *reinterpret_cast<int*>(_v) = _t->currentUserId(); break;
        case 11: *reinterpret_cast<QAbstractItemModel**>(_v) = _t->productiveAppsModel(); break;
        case 12: *reinterpret_cast<QAbstractItemModel**>(_v) = _t->nonProductiveAppsModel(); break;
        case 13: *reinterpret_cast<QString*>(_v) = _t->authToken(); break;
        case 14: *reinterpret_cast<QString*>(_v) = _t->userEmail(); break;
        case 15: *reinterpret_cast<QString*>(_v) = _t->currentUsername(); break;
        case 16: *reinterpret_cast<QString*>(_v) = _t->currentUserEmail(); break;
        case 17: *reinterpret_cast<int*>(_v) = _t->workTimeElapsedSeconds(); break;
        case 18: *reinterpret_cast<QSqlQueryModel**>(_v) = _t->logModel(); break;
        default: break;
        }
    }
}

const QMetaObject *Logger::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Logger::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6LoggerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Logger::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 103)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 103;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 103)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 103;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void Logger::currentAppNameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Logger::currentWindowTitleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Logger::logCountChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void Logger::logContentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void Logger::productivityStatsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void Logger::taskListChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void Logger::activeTaskChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void Logger::taskPausedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void Logger::globalTimeUsageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void Logger::trackingActiveChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void Logger::idleThresholdChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void Logger::currentUserIdChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void Logger::productivityAppsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void Logger::loginCompleted(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1, _t2);
}

// SIGNAL 14
void Logger::authTokenChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void Logger::userEmailChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void Logger::currentUsernameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 16, nullptr);
}

// SIGNAL 17
void Logger::currentUserEmailChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 17, nullptr);
}

// SIGNAL 18
void Logger::authTokenError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 18, nullptr, _t1);
}

// SIGNAL 19
void Logger::profileImageChanged(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 19, nullptr, _t1, _t2);
}

// SIGNAL 20
void Logger::taskStatusChanged(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 20, nullptr, _t1, _t2);
}

// SIGNAL 21
void Logger::taskReviewNotification(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 21, nullptr, _t1);
}

// SIGNAL 22
void Logger::taskTimeUpdated(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 22, nullptr, _t1, _t2);
}

// SIGNAL 23
void Logger::showNotification(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 23, nullptr, _t1);
}

// SIGNAL 24
void Logger::workTimeElapsedSecondsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 24, nullptr);
}

// SIGNAL 25
void Logger::showTimeWarning(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 25, nullptr, _t1);
}

// SIGNAL 26
void Logger::earlyLeaveReasonSubmitted()
{
    QMetaObject::activate(this, &staticMetaObject, 26, nullptr);
}

// SIGNAL 27
void Logger::showStatusMessage(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 27, nullptr, _t1);
}

// SIGNAL 28
void Logger::statusMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 28, nullptr);
}

// SIGNAL 29
void Logger::updateAvailable(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 29, nullptr, _t1, _t2);
}

// SIGNAL 30
void Logger::requestLoginPage()
{
    QMetaObject::activate(this, &staticMetaObject, 30, nullptr);
}

// SIGNAL 31
void Logger::currentAppIconPathChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 31, nullptr);
}

// SIGNAL 32
void Logger::showPingErrorDialog(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 32, nullptr, _t1);
}

// SIGNAL 33
void Logger::hidePingErrorDialog()
{
    QMetaObject::activate(this, &staticMetaObject, 33, nullptr);
}

// SIGNAL 34
void Logger::showAuthTokenErrorWindow(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 34, nullptr, _t1);
}

// SIGNAL 35
void Logger::requestTaskDetails(int _t1, const QString & _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 35, nullptr, _t1, _t2, _t3);
}

// SIGNAL 36
void Logger::taskDetailsSubmissionSuccess()
{
    QMetaObject::activate(this, &staticMetaObject, 36, nullptr);
}

// SIGNAL 37
void Logger::taskDetailsSubmissionFailed(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 37, nullptr, _t1);
}

// SIGNAL 38
void Logger::showNotification(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 38, nullptr, _t1, _t2);
}

// SIGNAL 39
void Logger::readyToProceedWithQuit()
{
    QMetaObject::activate(this, &staticMetaObject, 39, nullptr);
}

// SIGNAL 40
void Logger::taskDetailsSubmissionFailedWithRetry(const QString & _t1, int _t2, const QString & _t3, const QString & _t4, int _t5)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 40, nullptr, _t1, _t2, _t3, _t4, _t5);
}

// SIGNAL 41
void Logger::readyToProceedWithLogout()
{
    QMetaObject::activate(this, &staticMetaObject, 41, nullptr);
}
QT_WARNING_POP
