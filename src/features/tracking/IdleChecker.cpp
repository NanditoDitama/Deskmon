#include "IdleChecker.h"
#include "app/AppController.h"
#include <QDateTime>
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <windows.h>

#elif defined(Q_OS_MACOS)
#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOKitLib.h>

#else
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#endif

IdleChecker::IdleChecker(AppController *appController, QObject *parent)
    : QObject(parent)
    , m_appController(appController)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &IdleChecker::checkIdleTime);
    if (m_appController) {
        connect(m_appController, &AppController::idleThresholdChanged, this, &IdleChecker::updateIdleThresholdFromDatabase);
        updateIdleThresholdFromDatabase();
    } else {
        qWarning() << "IdleChecker initialized with null appController, using default threshold:" << m_idleThreshold << "seconds";
    }
    m_timer.start();
}

IdleChecker::~IdleChecker()
{
    m_timer.stop();
}

void IdleChecker::updateIdleThresholdFromDatabase()
{
    if (!m_appController) {
        qWarning() << "AppController is null, cannot update idle threshold";
        return;
    }
    int dbThreshold = m_appController->getIdleThreshold();
    qDebug() << "Retrieved idle threshold from database:" << dbThreshold << "seconds";
    if (dbThreshold > 0) {
        setIdleThreshold(dbThreshold);
        qDebug() << "Idle threshold set to:" << dbThreshold << "seconds";
    } else {
        qDebug() << "Invalid threshold from database, using default:" << m_idleThreshold << "seconds";
    }
    m_lastThresholdCheckTime = QDateTime::currentSecsSinceEpoch();
}

int IdleChecker::idleThreshold() const
{
    return m_idleThreshold;
}

void IdleChecker::setIdleThreshold(int seconds)
{
    if (m_idleThreshold != seconds && seconds > 0) {
        m_idleThreshold = seconds;
        qDebug() << "Idle threshold updated to:" << m_idleThreshold << "seconds";
        emit idleThresholdChanged();
    }
}

bool IdleChecker::isIdle() const
{
    return m_isIdle;
}

void IdleChecker::checkIdleTime()
{
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();

    if (currentTime - m_lastThresholdCheckTime >= 60) {
        updateIdleThresholdFromDatabase();
    }

    qint64 idleSeconds = getSystemIdleTime();

    if (idleSeconds >= m_idleThreshold) {
        if (!m_isIdle) {
            m_isIdle = true;
            m_lastIdleLogTime = currentTime - idleSeconds;

            if (m_appController && m_appController->activeTaskId() != -1 && !m_appController->isTaskPaused()) {
                m_appController->toggleTaskPause();
                m_autoPausedByIdle = true;
                qDebug() << "Task automatically paused due to idle";
            }

            emit showIdleNotification(QString("Sistem tidak aktif selama %1 detik").arg(m_idleThreshold));
            qDebug() << "Idle state started. Idle seconds:" << idleSeconds << "Threshold:" << m_idleThreshold;
        }

        if (currentTime - m_lastIdleLogTime >= 10) {
            emit idleDetected(m_lastIdleLogTime, currentTime);
            m_lastIdleLogTime = currentTime;
        }
    } else {
        if (m_isIdle) {
            m_isIdle = false;
            if (m_lastIdleLogTime > 0 && currentTime > m_lastIdleLogTime) {
                emit idleDetected(m_lastIdleLogTime, currentTime);
            }
            m_lastIdleLogTime = 0;

            if (m_autoPausedByIdle) {
                if (m_appController && m_appController->activeTaskId() != -1 && m_appController->isTaskPaused()) {
                    m_appController->toggleTaskPause();
                    qDebug() << "Task automatically resumed after idle";
                }
                m_autoPausedByIdle = false;
            }

            emit hideIdleNotification();
            qDebug() << "User returned from idle. Idle seconds:" << idleSeconds;
        }
    }
}

qint64 IdleChecker::getSystemIdleTime() const
{
#ifdef Q_OS_WIN
    return getSystemIdleTimeWindows();
#elif defined(Q_OS_MACOS)
    return getSystemIdleTimeMacOS();
#else
    return getSystemIdleTimeLinux();
#endif
}

#ifdef Q_OS_WIN
qint64 IdleChecker::getSystemIdleTimeWindows() const
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
        DWORD tickCount = GetTickCount();
        return (tickCount - lii.dwTime) / 1000;
    }
    return 0;
}
#elif defined(Q_OS_MACOS)
qint64 IdleChecker::getSystemIdleTimeMacOS() const
{
    int64_t idlesecs = -1;
    io_iterator_t iter = IO_OBJECT_NULL;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching("IOHIDSystem"), &iter) == KERN_SUCCESS) {
        io_registry_entry_t entry = IOIteratorNext(iter);
        if (entry) {
            CFMutableDictionaryRef dict = NULL;
            if (IORegistryEntryCreateCFProperties(entry, &dict, kCFAllocatorDefault, 0) == KERN_SUCCESS) {
                CFNumberRef obj = (CFNumberRef)CFDictionaryGetValue(dict, CFSTR("HIDIdleTime"));
                if (obj) {
                    int64_t nanoseconds = 0;
                    if (CFNumberGetValue(obj, kCFNumberSInt64Type, &nanoseconds)) {
                        idlesecs = nanoseconds / 1000000000;
                    }
                }
                CFRelease(dict);
            }
            IOObjectRelease(entry);
        }
        IOObjectRelease(iter);
    }
    return idlesecs >= 0 ? idlesecs : 0;
}
#else
qint64 IdleChecker::getSystemIdleTimeLinux() const
{
    Display *display = XOpenDisplay(NULL);
    if (!display) return 0;

    XScreenSaverInfo *info = XScreenSaverAllocInfo();
    XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
    qint64 idleTime = info->idle / 1000;
    XFree(info);
    XCloseDisplay(display);
    return idleTime;
}
#endif
