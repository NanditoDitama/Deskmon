// deskmon_window.cpp
// Menangani semua logika pemantauan jendela aktif:
//   - Pencatatan perpindahan window
//   - Pencatatan waktu idle
//   - Mendapatkan info window aktif dari OS
#include "deskmon.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

// ─────────────────────────────────────────────────
//  Properti Window Saat Ini
// ─────────────────────────────────────────────────

QString Deskmon::currentAppName() const
{
    return m_currentAppName;
}

QString Deskmon::currentWindowTitle() const
{
    return m_currentWindowTitle;
}

WindowManager::WindowInfo Deskmon::getActiveWindowInfo()
{
    return m_windowManager->getActiveWindowInfo();
}

// ─────────────────────────────────────────────────
//  Log Window Aktif (dipanggil oleh timer setiap detik)
// ─────────────────────────────────────────────────

void Deskmon::logActiveWindow()
{
    if (!m_isTrackingActive || m_isTaskPaused) {
        return;
    }

    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot log active window: Database is not open";
        return;
    }

    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    WindowManager::WindowInfo currentInfo = getActiveWindowInfo();

    if (m_isFirstCheck) {
        m_lastWindowInfo = currentInfo;
        m_lastActivityTime = currentTime;
        m_isFirstCheck = false;
        return;
    }

    if (currentInfo.appName != m_lastWindowInfo.appName ||
        currentInfo.title != m_lastWindowInfo.title) {
        logWindowChange(m_lastWindowInfo, m_lastActivityTime, currentTime - 1);
        m_lastActivityTime = currentTime;
        m_lastWindowInfo = currentInfo;
    }

    m_currentAppName = currentInfo.appName;
    m_currentWindowTitle = currentInfo.title;
    emit currentAppNameChanged();
    emit currentWindowTitleChanged();
    emit currentAppIconPathChanged();
}

// ─────────────────────────────────────────────────
//  Pencatatan Perubahan Window ke Database
// ─────────────────────────────────────────────────

void Deskmon::logWindowChange(const WindowManager::WindowInfo &info, qint64 startTime, qint64 endTime)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot log window change: Database is not open";
        return;
    }
    if (endTime <= startTime) {
        qWarning() << "Invalid window change period: endTime <= startTime";
        return;
    }
    if (m_currentUserId == -1) {
        qWarning() << "Cannot log window change: No user logged in";
        return;
    }

    QString urlToLog = info.url;
    // Tolak URL jika terlihat seperti query pencarian (ada spasi atau tidak ada titik)
    if (!urlToLog.isEmpty() && (urlToLog.contains(' ') || !urlToLog.contains('.'))) {
        qDebug() << "URL ditolak (dianggap query pencarian):" << urlToLog;
        urlToLog.clear();
    }

    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("INSERT INTO log (id_user, start_time, end_time, app_name, title, url) "
                  "VALUES (:id_user, :start, :end, :app, :title, :url)");
    query.bindValue(":id_user", m_currentUserId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);
    query.bindValue(":app", info.appName);
    query.bindValue(":title", info.title);
    query.bindValue(":url", urlToLog.isEmpty() ? QVariant() : urlToLog);

    if (!query.exec()) {
        qWarning() << "Failed to log window change:" << query.lastError().text();
    } else {
        emit logCountChanged();
        emit logContentChanged();
        emit productivityStatsChanged();
    }
}

// ─────────────────────────────────────────────────
//  Pencatatan Waktu Idle ke Database
// ─────────────────────────────────────────────────

void Deskmon::logIdle(qint64 startTime, qint64 endTime)
{
    if (!ensureDatabaseOpen()) {
        qWarning() << "Cannot log idle time: Database is not open";
        return;
    }
    if (endTime <= startTime) {
        qWarning() << "Invalid idle period: endTime <= startTime";
        return;
    }
    if (m_currentUserId == -1) {
        qWarning() << "Cannot log idle time: No user logged in";
        return;
    }

    QSqlQuery query(m_dataManager->activityDb());
    query.prepare("INSERT INTO log (id_user, start_time, end_time, app_name, title) "
                  "VALUES (:id_user, :start, :end, :app, :title)");
    query.bindValue(":id_user", m_currentUserId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);
    query.bindValue(":app", QString("Idle"));
    query.bindValue(":title", QString("No active window"));

    if (!query.exec()) {
        qWarning() << "Failed to log idle time:" << query.lastError().text();
    } else {
        emit logCountChanged();
        emit logContentChanged();
        emit productivityStatsChanged();
        refreshAll();
    }
}
