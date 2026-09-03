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
    QSqlQueryModel* logModel() const { return m_logModel; }

    void setLogFilter(const QString &startDate, const QString &endDate, int userId);
    void clearLogFilter(int userId);
    void showLogs(int userId);
    QString debugShowRawData(int userId) const;

signals:
    void logsChanged();

private:
    QSqlQueryModel *m_logModel;
    QString m_startDateFilter;
    QString m_endDateFilter;
};

#endif // WORKLOGREPOSITORY_H
