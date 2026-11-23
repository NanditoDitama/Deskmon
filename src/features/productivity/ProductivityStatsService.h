#ifndef PRODUCTIVITYSTATSSERVICE_H
#define PRODUCTIVITYSTATSSERVICE_H

#include <QObject>
#include <QVariantMap>
#include <QTimer>
#include "core/network/ApiClient.h"
#include "core/database/WorkLogRepository.h"
#include "core/database/ProductivityAppRepository.h"
#include "features/auth/AuthManager.h"

class ProductivityStatsService : public QObject
{
    Q_OBJECT
public:
    explicit ProductivityStatsService(ApiClient *apiClient,
                                      WorkLogRepository *workLogRepo,
                                      ProductivityAppRepository *prodRepo,
                                      AuthManager *authManager,
                                      QObject *parent = nullptr);
    ~ProductivityStatsService() override = default;

    QVariantMap productivityStats(const QString &startDateFilter = "", const QString &endDateFilter = "") const;
    int calculateTodayProductiveSeconds() const;

    void sendProductiveTimeToAPI();
    void sendDailyUsageReport(bool async = false);

    void startTimers();
    void stopTimers();
    int productivePingIntervalMs() const { return m_productivePingTimer.interval(); }

signals:
    void productivityStatsChanged();
    void refreshRequested();

private:
    void invalidateStatsCache();

    ApiClient *m_apiClient;
    WorkLogRepository *m_workLogRepo;
    ProductivityAppRepository *m_prodRepo;
    AuthManager *m_authManager;

    QTimer m_productivePingTimer;
    QTimer m_usageReportTimer;

    mutable bool m_statsCacheValid = false;
    mutable int m_cachedStatsUserId = -1;
    mutable QString m_cachedStartFilter;
    mutable QString m_cachedEndFilter;
    mutable QVariantMap m_cachedStats;
};

#endif // PRODUCTIVITYSTATSSERVICE_H
