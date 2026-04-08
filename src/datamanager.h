#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include "windowmanager.h"

class DataManager : public QObject
{
    Q_OBJECT
public:
    explicit DataManager(QObject *parent = nullptr);
    ~DataManager();

    bool initializeActivityDatabase();
    bool initializeProductivityDatabase();

    QSqlDatabase activityDb() const { return m_activityDb; }
    QSqlDatabase productivityDb() const { return m_productivityDb; }

    bool ensureActivityDbOpen();
    bool ensureProductivityDbOpen();

    // New Data Management Methods
    void checkAndCreateNewDayRecord(int userId);
    int loadWorkTimeData(int userId);
    void saveWorkTimeData(int userId, int elapsedSeconds);
    void updateProductivityCache(int userId);
    int getAppProductivityType(const QString &appName, const QString &url) const;
    bool logWindowActivity(int userId, const WindowManager::WindowInfo &info, qint64 startTime, qint64 endTime);
    void logIdleActivity(int userId, qint64 startTime, qint64 endTime);
    void syncProductivityApps(const QJsonArray &apps, int userId);

    // Reporting & Statistics
    QVariantMap getProductivityStats(int userId, const QString& startDate, const QString& endDate);
    QString getLogAsCsv(int userId, const QString& startDate, const QString& endDate);
    int calculateProductiveSeconds(int userId, const QString& date);
    QJsonArray getDailyUsageReportData(int userId, const QString& date);

private:
    void migrateProductivityDatabase();
    QString normalizeString(const QString &str) const;
    QString extractDomain(const QString &url) const;

    QSqlDatabase m_activityDb;
    QSqlDatabase m_productivityDb;
    QMap<QString, int> m_appCache;
    QMap<QString, int> m_domainCache;
};

#endif // DATAMANAGER_H
