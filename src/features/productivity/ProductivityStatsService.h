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
    void sendDailyUsageReport();

    void startTimers();
    void stopTimers();

signals:
    void productivityStatsChanged();

private:
    ApiClient *m_apiClient;
    WorkLogRepository *m_workLogRepo;
    ProductivityAppRepository *m_prodRepo;
    AuthManager *m_authManager;

    QTimer m_productivePingTimer;
    QTimer m_usageReportTimer;
};

#endif // PRODUCTIVITYSTATSSERVICE_H
