#ifndef WORKLOGREPOSITORY_H
#define WORKLOGREPOSITORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QString>
#include "DatabaseManager.h"
#include "core/platform/WindowInfoProvider.h"

class WorkLogRepository : public QObject
{
    Q_OBJECT
public:
    explicit WorkLogRepository(QObject *parent = nullptr);
    ~WorkLogRepository() override = default;

    bool initialize();
    bool ensureDatabaseOpen() const;
    QSqlDatabase database() const { return DatabaseManager::instance().database(); }

    void logWindowChange(const WindowInfo &info, qint64 startTime, qint64 endTime, int userId);

    int logCount(int userId) const;
    QString logContent(int userId) const;
    int todayLogCount(int userId) const;
    QString todayLogContent(int userId) const;
    QSqlQueryModel* logModel() const { m_logModelUsed = true; return m_logModel; }

    void setLogFilter(const QString &startDate, const QString &endDate, int userId);
    void clearLogFilter(int userId);
    void showLogs(int userId);
    QString debugShowRawData(int userId) const;

    static void dateRangeToEpoch(const QString &startDate, const QString &endDate,
                                 qint64 &outStartEpoch, bool &hasStart,
                                 qint64 &outEndExclusiveEpoch, bool &hasEnd);

signals:
    void logsChanged();

private:
    void invalidateCache();
    int countLogsForRange(int userId, const QString &startDate, const QString &endDate) const;
    QString fetchLogContentForRange(int userId, const QString &startDate, const QString &endDate) const;

    QSqlQueryModel *m_logModel;
    QString m_startDateFilter;
    QString m_endDateFilter;

    mutable bool m_logModelUsed = false;
    mutable bool m_countCacheValid = false;
    mutable int m_cachedCountUserId = -1;
    mutable int m_cachedLogCount = 0;

    mutable bool m_contentCacheValid = false;
    mutable int m_cachedContentUserId = -1;
    mutable QString m_cachedLogContent;

    mutable bool m_todayCountCacheValid = false;
    mutable int m_cachedTodayCountUserId = -1;
    mutable QString m_cachedTodayCountDate;
    mutable int m_cachedTodayLogCount = 0;

    mutable bool m_todayContentCacheValid = false;
    mutable int m_cachedTodayContentUserId = -1;
    mutable QString m_cachedTodayContentDate;
    mutable QString m_cachedTodayLogContent;
};

#endif // WORKLOGREPOSITORY_H
