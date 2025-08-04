// idlechecker.cpp

#include "idlechecker.h"
#include "logger.h"
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

IdleChecker::IdleChecker(Logger *logger, QObject *parent) : QObject(parent), m_logger(logger)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &IdleChecker::checkIdleTime);
    if (m_logger) {
        connect(m_logger, &Logger::idleThresholdChanged, this, &IdleChecker::updateIdleThresholdFromDatabase);
        connect(m_logger, &Logger::trackingActiveChanged, this, [this]() {
            if (m_logger->isTrackingActive()) {
                // Reset idle state when tracking is reactivated
                m_isIdle = false;
                m_lastIdleLogTime = 0;
                m_lastActiveTime = 0;
            } else {
                // When tracking is stopped (e.g., due to pause), reset idle state
                m_isIdle = false;
                m_lastIdleLogTime = 0;
                m_lastActiveTime = 0;
            }
        });
        connect(m_logger, &Logger::taskPausedChanged, this, [this]() {
            if (m_logger->isTaskPaused()) {
                // When task is paused, reset idle state to prevent idle detection
                m_isIdle = false;
                m_lastIdleLogTime = 0;
                m_lastActiveTime = 0;
            }
        });
        updateIdleThresholdFromDatabase();
    } else {
        qWarning() << "IdleChecker initialized with null logger, using default threshold:" << m_idleThreshold << "seconds";
    }
    m_timer.start();
}

IdleChecker::~IdleChecker()
{
    m_timer.stop();
}

void IdleChecker::updateIdleThresholdFromDatabase()
{
    if (!m_logger) {
        qWarning() << "Logger is null, cannot update idle threshold";
        return;
    }
    int dbThreshold = m_logger->getIdleThreshold();
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
    if (!m_logger || m_logger->currentUserId() == -1) {
        if (m_isIdle) {
            m_isIdle = false;
            m_lastIdleLogTime = 0;
            m_lastActiveTime = 0;
        }
        qDebug() << "Idle check skipped: user not logged in";
        return;
    }

    qint64 currentTime = QDateTime::currentSecsSinceEpoch();

    // Check database threshold every 10 seconds
    if (currentTime - m_lastThresholdCheckTime >= 10) {
        updateIdleThresholdFromDatabase();
    }

    qint64 idleTime = getSystemIdleTime() / 1000; // Convert to seconds
    if (idleTime < 0) {
        qWarning() << "Skipping idle check due to system error";
        return;
    }

    // PERBAIKAN: Cek kondisi tracking/pause SETELAH mendapatkan idle time
    // Ini memungkinkan penanganan transisi dari idle ke aktif
    bool shouldSkipDueToState = m_logger && (!m_logger->isTrackingActive() || m_logger->isTaskPaused());

    if (idleTime >= m_idleThreshold) {
        // User sedang idle
        if (!m_isIdle) {
            // Newly idle
            m_lastActiveTime = currentTime - idleTime;
            m_lastIdleLogTime = currentTime;
            m_isIdle = true;
            qDebug() << "Idle detected, started at:" << QDateTime::fromSecsSinceEpoch(m_lastActiveTime).toString();
            emit showIdleNotification("Idle Terdeteksi");
            qDebug() << "Sent idle notification: You have been idle";

            // Auto-pause task ketika idle
            if (m_logger && !m_logger->isTaskPaused()) {
                qDebug() << "Idle state detected. Automatically pausing the active task.";
                m_logger->toggleTaskPause();
            }

            if (m_logger) {
                m_logger->stopPingTimer();
            }
        }

        // Skip logging jika tracking tidak aktif atau task di-pause manual
        if (shouldSkipDueToState) {
            qDebug() << "Idle logging skipped due to tracking state";
            return;
        }

        // Log idle every 60 seconds while idle
        if (currentTime - m_lastIdleLogTime >= 60) {
            emit idleDetected(m_lastIdleLogTime, currentTime);
            m_lastIdleLogTime = currentTime;
            int minutes = idleTime / 60;
            int seconds = idleTime % 60;
            QString durationText = QString("%1m %2s").arg(minutes).arg(seconds);
            qDebug() << "Logged idle period, duration:" << durationText;
        }
    } else {
        // User sedang aktif
        if (m_isIdle) {
            // Returning from idle - SELALU JALANKAN BAGIAN INI
            qDebug() << "Detecting return from idle state";

            if (currentTime > m_lastIdleLogTime) {
                emit idleDetected(m_lastIdleLogTime, currentTime);
                qDebug() << "Logged final idle period, ended at:" << QDateTime::fromSecsSinceEpoch(currentTime).toString();
            }

            // Auto-resume task ketika kembali aktif
            if (m_logger && m_logger->isTaskPaused()) {
                qDebug() << "Returned from idle. Automatically resuming the active task.";
                m_logger->toggleTaskPause();
            }

            // Start ping timer (pastikan ini dijalankan)
            if (m_logger) {
                qDebug() << "Starting ping timer after returning from idle";
                m_logger->startPingTimer();
            }

            m_isIdle = false;
            m_lastIdleLogTime = 0;
            m_lastActiveTime = 0;
            qDebug() << "Returned from idle - state reset complete";
        }

        // Setelah menangani transisi idle, cek apakah harus skip
        if (shouldSkipDueToState) {
            qDebug() << "Further processing skipped due to tracking state";
            return;
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
    LASTINPUTINFO lastInputInfo;
    lastInputInfo.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lastInputInfo)) {
        DWORD tickCount = GetTickCount();
        return static_cast<qint64>(tickCount - lastInputInfo.dwTime);
    }
    qWarning() << "Failed to get last input info";
    return -1;
}
#elif defined(Q_OS_MACOS)
qint64 IdleChecker::getSystemIdleTimeMacOS() const {
    QProcess process;
    process.start("ioreg", {"-c", "IOHIDSystem", "-r", "-k", "HIDIdleTime"});
    if (process.waitForFinished(100)) {
        QString output = QString(process.readAllStandardOutput());
        QRegularExpression regex("\"HIDIdleTime\" = (\\d+)");
        QRegularExpressionMatch match = regex.match(output);
        if (match.hasMatch()) {
            // Idle time in nanoseconds, convert to milliseconds
            return match.captured(1).toLongLong() / 1000000;
        }
    }
    return -1;
}
#elif defined(Q_OS_LINUX)
qint64 IdleChecker::getSystemIdleTimeLinux() const
{
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        qWarning() << "Failed to open X11 display";
        return -1;
    }
    XScreenSaverInfo *info = XScreenSaverAllocInfo();
    if (!info) {
        XCloseDisplay(display);
        qWarning() << "Failed to allocate XScreenSaverInfo";
        return -1;
    }
    if (XScreenSaverQueryInfo(display, DefaultRootWindow(display), info)) {
        qint64 idleTime = info->idle;
        XFree(info);
        XCloseDisplay(display);
        return idleTime;
    }
    XFree(info);
    XCloseDisplay(display);
    qWarning() << "Failed to query XScreenSaverInfo";
    return -1;
}
#endif
