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
#include <AvailabilityMacros.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOKitKeys.h>

#elif defined(DESKMON_HAS_X11_IDLE)
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

void IdleChecker::setIdleTimeProvider(IdleTimeProvider provider)
{
    m_idleTimeProvider = std::move(provider);
}

void IdleChecker::checkIdleTime()
{
    checkIdleTimeAt(QDateTime::currentSecsSinceEpoch());
}

void IdleChecker::flushPendingIdle()
{
    if (m_isIdle) {
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        if (m_lastIdleLogTime > 0 && now > m_lastIdleLogTime && m_appController && m_appController->currentUserId() != -1) {
            emit idleDetected(m_lastIdleLogTime, now);
        }
        m_isIdle = false;
        m_lastIdleLogTime = 0;
        m_lastActiveTime = 0;
        m_autoPausedByIdle = false;
    }
}

void IdleChecker::checkIdleTimeAt(qint64 currentTime)
{
    if (!m_appController || m_appController->currentUserId() == -1) {
        if (m_isIdle) {
            m_isIdle = false;
            m_lastIdleLogTime = 0;
            m_lastActiveTime = 0;
        }
        m_autoPausedByIdle = false;
        return;
    }

    if (m_appController->activeTaskId() != -1 && m_appController->isTaskPaused() && !m_isIdle) {
        return;
    }

    if (currentTime - m_lastThresholdCheckTime >= 10) {
        updateIdleThresholdFromDatabase();
    }

    qint64 idleSeconds = getSystemIdleTime();
    if (idleSeconds < 0) {
        qWarning() << "Skipping idle check due to system error";
        return;
    }

    if (idleSeconds >= m_idleThreshold) {
        if (!m_isIdle) {
            m_isIdle = true;
            m_lastActiveTime = currentTime - idleSeconds;
            m_lastIdleLogTime = currentTime;

            if (m_appController->activeTaskId() != -1 && !m_appController->isTaskPaused()) {
                m_appController->toggleTaskPause();
                m_autoPausedByIdle = true;
                qDebug() << "Task automatically paused due to idle";
            }

            emit showIdleNotification(QStringLiteral("Idle Terdeteksi"));
            qDebug() << "Idle state started. Idle seconds:" << idleSeconds << "Threshold:" << m_idleThreshold;
        }
    } else {
        if (m_isIdle) {
            m_isIdle = false;
            if (m_lastIdleLogTime > 0 && currentTime > m_lastIdleLogTime) {
                emit idleDetected(m_lastIdleLogTime, currentTime);
            }
            m_lastIdleLogTime = 0;
            m_lastActiveTime = 0;

            if (m_autoPausedByIdle) {
                if (m_appController->activeTaskId() != -1 && m_appController->isTaskPaused()) {
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
    if (m_idleTimeProvider) {
        return m_idleTimeProvider();
    }
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
        return static_cast<qint64>(tickCount - lii.dwTime) / 1000;
    }
    return -1;
}
#elif defined(Q_OS_MACOS)
qint64 IdleChecker::getSystemIdleTimeMacOS() const
{
    int64_t idlesecs = -1;
    io_iterator_t iter = IO_OBJECT_NULL;
#if defined(MAC_OS_VERSION_12_0) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_12_0
    mach_port_t mainPort = kIOMainPortDefault;
#else
    mach_port_t mainPort = 0;
#endif
    if (IOServiceGetMatchingServices(mainPort, IOServiceMatching("IOHIDSystem"), &iter) == KERN_SUCCESS) {
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
    return idlesecs >= 0 ? idlesecs : -1;
}
#else
qint64 IdleChecker::getSystemIdleTimeLinux() const
{
    // Wayland session fallback via GNOME Mutter IdleMonitor D-Bus interface
    const bool isWayland = qEnvironmentVariableIsSet("WAYLAND_DISPLAY")
        || qEnvironmentVariable("XDG_SESSION_TYPE").compare(QStringLiteral("wayland"), Qt::CaseInsensitive) == 0;
    if (isWayland) {
        QProcess proc;
        proc.start(QStringLiteral("gdbus"), {
            QStringLiteral("call"), QStringLiteral("--session"),
            QStringLiteral("--dest"), QStringLiteral("org.gnome.Mutter.IdleMonitor"),
            QStringLiteral("--object-path"), QStringLiteral("/org/gnome/Mutter/IdleMonitor/Core"),
            QStringLiteral("--method"), QStringLiteral("org.gnome.Mutter.IdleMonitor.GetIdletime")
        });
        if (proc.waitForFinished(300) && proc.exitCode() == 0) {
            const QString out = QString::fromUtf8(proc.readAllStandardOutput());
            static const QRegularExpression numRe(QStringLiteral(R"((\d+))"));
            const QRegularExpressionMatch match = numRe.match(out);
            if (match.hasMatch()) {
                bool ok = false;
                const qint64 ms = match.captured(1).toLongLong(&ok);
                if (ok && ms >= 0) {
                    return ms / 1000;
                }
            }
        }
    }

#ifdef DESKMON_HAS_X11_IDLE
    Display *display = XOpenDisplay(nullptr);
    if (!display) return -1;

    XScreenSaverInfo *info = XScreenSaverAllocInfo();
    if (!info) {
        XCloseDisplay(display);
        return -1;
    }

    qint64 idleTime = -1;
    if (XScreenSaverQueryInfo(display, DefaultRootWindow(display), info)) {
        idleTime = static_cast<qint64>(info->idle) / 1000;
    }
    XFree(info);
    XCloseDisplay(display);
    return idleTime;
#else
    return -1;
#endif
}
#endif
