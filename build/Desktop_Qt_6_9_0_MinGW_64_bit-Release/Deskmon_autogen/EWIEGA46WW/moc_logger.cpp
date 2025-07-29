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
        "email",
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
        "workTimeElapsedSeconds"
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
        // Slot 'logActiveWindow'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'logIdle'
        QtMocHelpers::SlotData<void(qint64, qint64)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::LongLong, 38 }, { QMetaType::LongLong, 39 },
        }}),
        // Slot 'refreshAll'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'refreshTasks'
        QtMocHelpers::SlotData<void()>(41, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'handleTaskStatusReply'
        QtMocHelpers::SlotData<void(QNetworkReply *, int)>(42, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 43, 44 }, { QMetaType::Int, 27 },
        }}),
        // Slot 'getPendingApplicationRequests'
        QtMocHelpers::SlotData<QVariantList()>(45, 2, QMC::AccessPublic, 0x80000000 | 46),
        // Slot 'handleProductivityAppsResponse'
        QtMocHelpers::SlotData<void(QNetworkReply *)>(47, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 43, 44 },
        }}),
        // Slot 'handleDailyUsageReportResponse'
        QtMocHelpers::SlotData<void(QNetworkReply *)>(48, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 43, 44 },
        }}),
        // Slot 'submitEarlyLeaveReason'
        QtMocHelpers::SlotData<void(const QString &)>(49, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 50 },
        }}),
        // Slot 'handleTaskFetchReply'
        QtMocHelpers::SlotData<void(QNetworkReply *)>(51, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 43, 44 },
        }}),
        // Slot 'fetchWorkTimeFromAPI'
        QtMocHelpers::SlotData<void()>(52, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleFetchWorkTimeResponse'
        QtMocHelpers::SlotData<void(QNetworkReply *)>(53, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 43, 44 },
        }}),
        // Method 'debugShowRawData'
        QtMocHelpers::MethodData<QString() const>(54, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'showLogs'
        QtMocHelpers::MethodData<void()>(55, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'isUsernameTaken'
        QtMocHelpers::MethodData<bool(const QString &)>(56, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'updateUserProfile'
        QtMocHelpers::MethodData<QString(const QString &, const QString &, const QString &)>(57, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 58 }, { QMetaType::QString, 59 }, { QMetaType::QString, 60 },
        }}),
        // Method 'cropProfileImage'
        QtMocHelpers::MethodData<QString(const QString &, qreal, qreal, qreal, qreal, qreal, qreal)>(61, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 62 }, { QMetaType::QReal, 63 }, { QMetaType::QReal, 64 }, { QMetaType::QReal, 65 },
            { QMetaType::QReal, 66 }, { QMetaType::QReal, 67 }, { QMetaType::QReal, 68 },
        }}),
        // Method 'clearLogFilter'
        QtMocHelpers::MethodData<void()>(69, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'validateFilePath'
        QtMocHelpers::MethodData<bool(const QString &)>(70, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 71 },
        }}),
        // Method 'setLogFilter'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(72, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 73 }, { QMetaType::QString, 74 },
        }}),
        // Method 'updateProfileImage'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(75, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 24 }, { QMetaType::QString, 62 },
        }}),
        // Method 'getProfileImagePath'
        QtMocHelpers::MethodData<QString(const QString &)>(76, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'setActiveTask'
        QtMocHelpers::MethodData<void(int)>(77, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Method 'finishTask'
        QtMocHelpers::MethodData<void(int)>(78, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Method 'toggleTaskPause'
        QtMocHelpers::MethodData<void()>(79, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'formatDuration'
        QtMocHelpers::MethodData<QString(int) const>(80, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::Int, 81 },
        }}),
        // Method 'startGlobalTimer'
        QtMocHelpers::MethodData<void()>(82, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'getUserPassword'
        QtMocHelpers::MethodData<QString(const QString &)>(83, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'setIdleThreshold'
        QtMocHelpers::MethodData<void(int)>(84, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 81 },
        }}),
        // Method 'getAvailableApps'
        QtMocHelpers::MethodData<QVariantList() const>(85, 2, QMC::AccessPublic, 0x80000000 | 46),
        // Method 'addProductivityApp'
        QtMocHelpers::MethodData<void(const QString &, const QString &, const QString &, int)>(86, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 87 }, { QMetaType::QString, 88 }, { QMetaType::QString, 89 }, { QMetaType::Int, 90 },
        }}),
        // Method 'getProductivityApps'
        QtMocHelpers::MethodData<QVariantList() const>(91, 2, QMC::AccessPublic, 0x80000000 | 46),
        // Method 'authenticate'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(92, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 93 }, { QMetaType::QString, 94 },
        }}),
        // Method 'getUserDepartment'
        QtMocHelpers::MethodData<QString(const QString &)>(95, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'getCurrentUsername'
        QtMocHelpers::MethodData<QString() const>(96, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'getCurrentUserEmail'
        QtMocHelpers::MethodData<QString() const>(97, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'getUserEmail'
        QtMocHelpers::MethodData<QString(const QString &)>(98, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 24 },
        }}),
        // Method 'fetchAndStoreTasks'
        QtMocHelpers::MethodData<void()>(99, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'updateTaskStatus'
        QtMocHelpers::MethodData<void(int)>(100, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Method 'logout'
        QtMocHelpers::MethodData<void()>(101, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'sendProductivityAppToAPI'
        QtMocHelpers::MethodData<void(const QString &, const QString &, const QString &, int)>(102, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 87 }, { QMetaType::QString, 88 }, { QMetaType::QString, 89 }, { QMetaType::Int, 90 },
        }}),
        // Method 'loadWorkTimeData'
        QtMocHelpers::MethodData<void()>(103, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'checkAndCreateNewDayRecord'
        QtMocHelpers::MethodData<void()>(104, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'calculateTodayProductiveSeconds'
        QtMocHelpers::MethodData<int() const>(105, 2, QMC::AccessPublic, QMetaType::Int),
        // Method 'sendProductiveTimeToAPI'
        QtMocHelpers::MethodData<void()>(106, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'sendDailyUsageReport'
        QtMocHelpers::MethodData<void()>(107, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'sendPing'
        QtMocHelpers::MethodData<void(int)>(108, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Method 'sendPausePlayDataToAPI'
        QtMocHelpers::MethodData<void(int, const QString &, const QString &, const QString &)>(109, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 27 }, { QMetaType::QString, 38 }, { QMetaType::QString, 39 }, { QMetaType::QString, 110 },
        }}),
        // Method 'getAppProductivityType'
        QtMocHelpers::MethodData<int(const QString &, const QString &) const>(111, 2, QMC::AccessPublic, QMetaType::Int, {{
            { QMetaType::QString, 87 }, { QMetaType::QString, 89 },
        }}),
        // Method 'totalWorkSeconds'
        QtMocHelpers::MethodData<int() const>(112, 2, QMC::AccessPublic, QMetaType::Int),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'currentAppName'
        QtMocHelpers::PropertyData<QString>(113, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'currentWindowTitle'
        QtMocHelpers::PropertyData<QString>(114, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'logCount'
        QtMocHelpers::PropertyData<int>(115, QMetaType::Int, QMC::DefaultPropertyFlags, 2),
        // property 'logContent'
        QtMocHelpers::PropertyData<QString>(116, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'productivityStats'
        QtMocHelpers::PropertyData<QVariantMap>(117, 0x80000000 | 118, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 4),
        // property 'taskList'
        QtMocHelpers::PropertyData<QVariantList>(119, 0x80000000 | 46, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 5),
        // property 'activeTaskId'
        QtMocHelpers::PropertyData<int>(120, QMetaType::Int, QMC::DefaultPropertyFlags, 6),
        // property 'isTaskPaused'
        QtMocHelpers::PropertyData<bool>(121, QMetaType::Bool, QMC::DefaultPropertyFlags, 7),
        // property 'globalTimeUsage'
        QtMocHelpers::PropertyData<qint64>(122, QMetaType::LongLong, QMC::DefaultPropertyFlags, 8),
        // property 'isTrackingActive'
        QtMocHelpers::PropertyData<bool>(123, QMetaType::Bool, QMC::DefaultPropertyFlags, 9),
        // property 'currentUserId'
        QtMocHelpers::PropertyData<int>(124, QMetaType::Int, QMC::DefaultPropertyFlags, 11),
        // property 'productiveAppsModel'
        QtMocHelpers::PropertyData<QAbstractItemModel*>(125, 0x80000000 | 126, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 12),
        // property 'nonProductiveAppsModel'
        QtMocHelpers::PropertyData<QAbstractItemModel*>(127, 0x80000000 | 126, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 12),
        // property 'authToken'
        QtMocHelpers::PropertyData<QString>(128, QMetaType::QString, QMC::DefaultPropertyFlags, 14),
        // property 'userEmail'
        QtMocHelpers::PropertyData<QString>(129, QMetaType::QString, QMC::DefaultPropertyFlags, 15),
        // property 'currentUsername'
        QtMocHelpers::PropertyData<QString>(58, QMetaType::QString, QMC::DefaultPropertyFlags, 16),
        // property 'currentUserEmail'
        QtMocHelpers::PropertyData<QString>(130, QMetaType::QString, QMC::DefaultPropertyFlags, 17),
        // property 'workTimeElapsedSeconds'
        QtMocHelpers::PropertyData<int>(131, QMetaType::Int, QMC::DefaultPropertyFlags, 24),
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
        case 27: _t->logActiveWindow(); break;
        case 28: _t->logIdle((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 29: _t->refreshAll(); break;
        case 30: _t->refreshTasks(); break;
        case 31: _t->handleTaskStatusReply((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 32: { QVariantList _r = _t->getPendingApplicationRequests();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 33: _t->handleProductivityAppsResponse((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 34: _t->handleDailyUsageReportResponse((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 35: _t->submitEarlyLeaveReason((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 36: _t->handleTaskFetchReply((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 37: _t->fetchWorkTimeFromAPI(); break;
        case 38: _t->handleFetchWorkTimeResponse((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 39: { QString _r = _t->debugShowRawData();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 40: _t->showLogs(); break;
        case 41: { bool _r = _t->isUsernameTaken((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 42: { QString _r = _t->updateUserProfile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 43: { QString _r = _t->cropProfileImage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[7])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 44: _t->clearLogFilter(); break;
        case 45: { bool _r = _t->validateFilePath((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 46: _t->setLogFilter((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 47: { bool _r = _t->updateProfileImage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 48: { QString _r = _t->getProfileImagePath((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 49: _t->setActiveTask((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 50: _t->finishTask((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 51: _t->toggleTaskPause(); break;
        case 52: { QString _r = _t->formatDuration((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 53: _t->startGlobalTimer(); break;
        case 54: { QString _r = _t->getUserPassword((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 55: _t->setIdleThreshold((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 56: { QVariantList _r = _t->getAvailableApps();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 57: _t->addProductivityApp((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 58: { QVariantList _r = _t->getProductivityApps();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 59: { bool _r = _t->authenticate((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 60: { QString _r = _t->getUserDepartment((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 61: { QString _r = _t->getCurrentUsername();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 62: { QString _r = _t->getCurrentUserEmail();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 63: { QString _r = _t->getUserEmail((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 64: _t->fetchAndStoreTasks(); break;
        case 65: _t->updateTaskStatus((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 66: _t->logout(); break;
        case 67: _t->sendProductivityAppToAPI((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 68: _t->loadWorkTimeData(); break;
        case 69: _t->checkAndCreateNewDayRecord(); break;
        case 70: { int _r = _t->calculateTodayProductiveSeconds();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 71: _t->sendProductiveTimeToAPI(); break;
        case 72: _t->sendDailyUsageReport(); break;
        case 73: _t->sendPing((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 74: _t->sendPausePlayDataToAPI((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 75: { int _r = _t->getAppProductivityType((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 76: { int _r = _t->totalWorkSeconds();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 31:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 33:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 34:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 36:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 38:
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
    }
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 12:
        case 11:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractItemModel* >(); break;
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
        if (_id < 77)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 77;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 77)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 77;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
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
QT_WARNING_POP
