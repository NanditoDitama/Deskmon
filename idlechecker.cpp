#include "idlechecker.h"
#include "logger.h"
#include <QDateTime>
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_LINUX)
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
    // Skip checking if tracking is not active or task is paused
    if (m_logger && (!m_logger->isTrackingActive() || m_logger->isTaskPaused())) {
        // Ensure idle state is reset when paused or tracking is off
        if (m_isIdle) {
            m_isIdle = false;
            m_lastIdleLogTime = 0;
            m_lastActiveTime = 0;
            qDebug() << "Idle checking stopped due to pause or tracking inactive";
        }
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

    if (idleTime >= m_idleThreshold) {
        if (!m_isIdle) {
            // Newly idle
            m_lastActiveTime = currentTime - idleTime;
            m_lastIdleLogTime = currentTime;
            m_isIdle = true;
            qDebug() << "Idle detected, started at:" << QDateTime::fromSecsSinceEpoch(m_lastActiveTime).toString();
            emit showIdleNotification("You have been idle");
            qDebug() << "Sent idle notification: You have been idle";
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
        if (m_isIdle) {
            // Returning from idle
            if (currentTime > m_lastIdleLogTime) {
                emit idleDetected(m_lastIdleLogTime, currentTime);
                qDebug() << "Logged final idle period, ended at:" << QDateTime::fromSecsSinceEpoch(currentTime).toString();
            }
            m_isIdle = false;
            m_lastIdleLogTime = 0;
            m_lastActiveTime = 0;
            qDebug() << "Returned from idle";
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
#ifdef Q_OS_MACOS
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
#endif
