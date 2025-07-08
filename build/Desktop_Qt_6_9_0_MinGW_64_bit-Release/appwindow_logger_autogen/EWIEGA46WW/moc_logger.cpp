/****************************************************************************
** Meta object code from reading C++ file 'logger.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../logger.h"
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
        "logActiveWindow",
        "logIdle",
        "startTime",
        "endTime",
        "updateTaskTime",
        "debugShowRawData",
        "showLogs",
        "authenticate",
        "username",
        "password",
        "registerUser",
        "isUsernameTaken",
        "updateUserProfile",
        "currentUsername",
        "newUsername",
        "newPassword",
        "getUserDepartment",
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
        "taskId",
        "finishTask",
        "toggleTaskPause",
        "formatDuration",
        "seconds",
        "startGlobalTimer",
        "getUserPassword",
        "setIdleThreshold",
        "currentAppName",
        "currentWindowTitle",
        "logCount",
        "logContent",
        "productivityStats",
        "QVariantMap",
        "taskList",
        "QVariantList",
        "activeTaskId",
        "isTaskPaused",
        "globalTimeUsage",
        "isTrackingActive"
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
        // Slot 'logActiveWindow'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'logIdle'
        QtMocHelpers::SlotData<void(qint64, qint64)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::LongLong, 15 }, { QMetaType::LongLong, 16 },
        }}),
        // Slot 'updateTaskTime'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'debugShowRawData'
        QtMocHelpers::MethodData<QString() const>(18, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'showLogs'
        QtMocHelpers::MethodData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'authenticate'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(20, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 21 }, { QMetaType::QString, 22 },
        }}),
        // Method 'registerUser'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(23, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 21 }, { QMetaType::QString, 22 },
        }}),
        // Method 'isUsernameTaken'
        QtMocHelpers::MethodData<bool(const QString &)>(24, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 21 },
        }}),
        // Method 'updateUserProfile'
        QtMocHelpers::MethodData<QString(const QString &, const QString &, const QString &)>(25, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 26 }, { QMetaType::QString, 27 }, { QMetaType::QString, 28 },
        }}),
        // Method 'getUserDepartment'
        QtMocHelpers::MethodData<QString(const QString &)>(29, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 21 },
        }}),
        // Method 'cropProfileImage'
        QtMocHelpers::MethodData<QString(const QString &, qreal, qreal, qreal, qreal, qreal, qreal)>(30, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 31 }, { QMetaType::QReal, 32 }, { QMetaType::QReal, 33 }, { QMetaType::QReal, 34 },
            { QMetaType::QReal, 35 }, { QMetaType::QReal, 36 }, { QMetaType::QReal, 37 },
        }}),
        // Method 'clearLogFilter'
        QtMocHelpers::MethodData<void()>(38, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'validateFilePath'
        QtMocHelpers::MethodData<bool(const QString &)>(39, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 40 },
        }}),
        // Method 'setLogFilter'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(41, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 42 }, { QMetaType::QString, 43 },
        }}),
        // Method 'updateProfileImage'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(44, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 21 }, { QMetaType::QString, 31 },
        }}),
        // Method 'getProfileImagePath'
        QtMocHelpers::MethodData<QString(const QString &)>(45, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 21 },
        }}),
        // Method 'setActiveTask'
        QtMocHelpers::MethodData<void(int)>(46, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 47 },
        }}),
        // Method 'finishTask'
        QtMocHelpers::MethodData<void(int)>(48, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 47 },
        }}),
        // Method 'toggleTaskPause'
        QtMocHelpers::MethodData<void()>(49, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'formatDuration'
        QtMocHelpers::MethodData<QString(int) const>(50, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::Int, 51 },
        }}),
        // Method 'startGlobalTimer'
        QtMocHelpers::MethodData<void()>(52, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'getUserPassword'
        QtMocHelpers::MethodData<QString(const QString &)>(53, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 21 },
        }}),
        // Method 'setIdleThreshold'
        QtMocHelpers::MethodData<void(int)>(54, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 51 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'currentAppName'
        QtMocHelpers::PropertyData<QString>(55, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'currentWindowTitle'
        QtMocHelpers::PropertyData<QString>(56, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'logCount'
        QtMocHelpers::PropertyData<int>(57, QMetaType::Int, QMC::DefaultPropertyFlags, 2),
        // property 'logContent'
        QtMocHelpers::PropertyData<QString>(58, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'productivityStats'
        QtMocHelpers::PropertyData<QVariantMap>(59, 0x80000000 | 60, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 4),
        // property 'taskList'
        QtMocHelpers::PropertyData<QVariantList>(61, 0x80000000 | 62, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 5),
        // property 'activeTaskId'
        QtMocHelpers::PropertyData<int>(63, QMetaType::Int, QMC::DefaultPropertyFlags, 6),
        // property 'isTaskPaused'
        QtMocHelpers::PropertyData<bool>(64, QMetaType::Bool, QMC::DefaultPropertyFlags, 7),
        // property 'globalTimeUsage'
        QtMocHelpers::PropertyData<qint64>(65, QMetaType::LongLong, QMC::DefaultPropertyFlags, 8),
        // property 'isTrackingActive'
        QtMocHelpers::PropertyData<bool>(66, QMetaType::Bool, QMC::DefaultPropertyFlags, 9),
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
        case 11: _t->logActiveWindow(); break;
        case 12: _t->logIdle((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 13: _t->updateTaskTime(); break;
        case 14: { QString _r = _t->debugShowRawData();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 15: _t->showLogs(); break;
        case 16: { bool _r = _t->authenticate((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 17: { bool _r = _t->registerUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 18: { bool _r = _t->isUsernameTaken((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 19: { QString _r = _t->updateUserProfile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 20: { QString _r = _t->getUserDepartment((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 21: { QString _r = _t->cropProfileImage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[7])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 22: _t->clearLogFilter(); break;
        case 23: { bool _r = _t->validateFilePath((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 24: _t->setLogFilter((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 25: { bool _r = _t->updateProfileImage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 26: { QString _r = _t->getProfileImagePath((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 27: _t->setActiveTask((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 28: _t->finishTask((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 29: _t->toggleTaskPause(); break;
        case 30: { QString _r = _t->formatDuration((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 31: _t->startGlobalTimer(); break;
        case 32: { QString _r = _t->getUserPassword((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 33: _t->setIdleThreshold((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
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
        if (_id < 34)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 34;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 34)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 34;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
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
QT_WARNING_POP
